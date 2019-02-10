#include <algorithm>
#include <set>
#include <iterator>
#include <iostream>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <fstream> 
#include <parser.h>
#include <map>

using namespace std;

// bool printArray (int64_t a[]) {
//   if (a is not an array) return false;
//   for (int64_t c=0; c < a.length; c++){
//     printArrayElement(a, c);
//   }
//   return true;
// }

// void printArrayElement (int64_t a[], int64_t i) {
//   print(i); // Print the index
//   print((i * 8) + 8); // Print the offset of the array element in the L1 data layout
//   if (a[i] is not an array){
//     print(a[i]);
//   } else {
//     printArray(a[i]);
//   }
//   return;
// }

bool myisdigit(std::string s) {
  if (s.at(0) == '+' || s.at(0) == '-') {
    return isdigit(s.at(1));
  }
  return isdigit(s.at(0));
}

void decompose_t(std::string t, std::string &out) {
  if (myisdigit(t)) {
    // number
    out = "$" + t;
  }
  else {
    // regi
    out = "%" + t;
  }
}

void decompose_s(std::string s, std::string &out) {
  if (myisdigit(s)) {
    // number
    out = "$" + s;
  }
  else if ((s.at(0)) == ':') {
    // label
    out = "$_" + s.substr(1);
  }
  else {
    // regi
    out = "%" + s;
  }
}

int mycmp(int s1, int s2, std::string opt) {
  if (opt == "<") {
    return (s1 < s2 ? 1 : 0);
  }
  else if (opt == "<=") {
    return (s1 <= s2 ? 1 : 0);
  }
  else if (opt == "=") {
    return (s1 == s2 ? 1 : 0);
  }
  else {
    cout << "Unkonw cmp opt: " << opt << ". (compiler.cpp/mycmp)" << endl;
    exit(0);
  }
}

std::string out_assign_with_cmp(std::string r1, std::string r2, std::string r3, std::string r4, std::string set_opt) {
  std::string out = "";
  out += "  cmpq " + r3 + ", " + r2 + "\n";
  out += "  " + set_opt + " " + r4 + "\n";
  out += "  movzbq " + r4 + ", " + r1 + "\n";
  // cout << out;
  return out;
}

std::string out_cjump_with_cmp(std::string r1, std::string r2, std::string r3, std::string r4, std::string j_opt) {
  std::string out = "";
  out += "  cmpq " + r2 + ", " + r1 + "\n";
  out += "  " + j_opt + " " + r3 + "\n";
  out += "  jmp " + r4 + "\n";
  // cout << out;
  return out;
}

int main(
  int argc, 
  char **argv
  ){
  bool verbose;

  /* Check the input.
   */
  if( argc < 2 ) {
    std::cerr << "Usage: " << argv[ 0 ] << " SOURCE [-v]" << std::endl;
    return 1;
  }
  int32_t opt;
  while ((opt = getopt(argc, argv, "v")) != -1) {
    switch (opt){
      case 'v':
        verbose = true;
        break ;

      default:
        std::cerr << "Usage: " << argv[ 0 ] << "[-v] SOURCE" << std::endl;
        return 1;
    }
  }


  /* Parse the L1 program.
   */
  L1::Program p = L1::L1_parse_file(argv[optind]);

  /* Generate x86_64 code
   */

  std::map<std::string, std::string> _8bitregi;

  _8bitregi["r10"] = "%r10b";
  _8bitregi["r13"] = "%r13b";
  _8bitregi["r8"] = "%r8b";
  _8bitregi["rbp"] = "%bpl";
  _8bitregi["rdi"] = "%dil";
  _8bitregi["r11"] = "%r11b";
  _8bitregi["r14"] = "%r14b";
  _8bitregi["r9"] = "%r9b";
  _8bitregi["rbx"] = "%bl";
  _8bitregi["rdx"] = "%dl";
  _8bitregi["r12"] = "%r12b";
  _8bitregi["r15"] = "%r15b";
  _8bitregi["rax"] = "%al";
  _8bitregi["rcx"] = "%cl";
  _8bitregi["rsi"] = "%sil";

  ofstream out;
  out.open("prog.S", ios::out);

  out<<".text \n";
  out<<"  .globl go \n";
  out<<"go: \n";
  out<<"  pushq %rbx \n";
  out<<"  pushq %rbp \n";
  out<<"  pushq %r12 \n";
  out<<"  pushq %r13 \n";
  out<<"  pushq %r14 \n";
  out<<"  pushq %r15 \n";
  out<<"  call _"<< p.entryPointLabel.substr(1)<<endl;
  out<<"  popq %r15 \n";
  out<<"  popq %r14 \n";
  out<<"  popq %r13 \n";
  out<<"  popq %r12 \n";
  out<<"  popq %rbp \n";
  out<<"  popq %rbx \n";
  out<<"  retq \n";

  // cout << p.entryPointLabel << endl;
  for (auto f : p.functions){
    
    out << "_" << (f->name).substr(1) << ":" << endl;
    out << "  subq $" << std::to_string(f->locals * 8) << ", %rsp" << endl;
    
    for (auto i : f->instructions) {
      std::string opt = i->opt;

      // assignment instruction
      if (opt == "<-") {
        std::string left = "";
        std::string right = "";
        if (!i->cmp_opt.empty()) {
          // assign cmp
          std::string cmp_opt = i->cmp_opt;
          std::string w = i->operands[0];
          std::string cmp_left = i->operands[1];
          std::string cmp_right = i->operands[2];

          if (myisdigit(cmp_left) && myisdigit(cmp_right)) {
            // number cmp number
            // cout << cmp_left << " " << cmp_right << endl;
            // cout << i->cmp_opt << endl;
            // cout << mycmp(std::stoi(cmp_left),std::stoi(cmp_right),i->cmp_opt) << endl;
            out << "  movq $" << std::to_string(mycmp(std::stoi(cmp_left),std::stoi(cmp_right),i->cmp_opt)) << ", %" << w << endl;
          }
          else if (myisdigit(cmp_left) && !myisdigit(cmp_right)) {
            // number cmp regi
            cmp_left = "$" + cmp_left;
            cmp_right = "%" + cmp_right;
            if (cmp_opt == "<") {
              out << out_assign_with_cmp("%"+w,cmp_right,cmp_left,_8bitregi[w],"setg");
            }
            else if (cmp_opt == "<=") {
              out << out_assign_with_cmp("%"+w,cmp_right,cmp_left,_8bitregi[w],"setge");
            }
            else if (cmp_opt == "=") {
              out << out_assign_with_cmp("%"+w,cmp_right,cmp_left,_8bitregi[w],"sete");
            }
          }
          else  {
            // regi cmp number | regi cmp regi
            // cout << opt << endl;
            if (myisdigit(cmp_right)) {
              cmp_right = "$" + cmp_right;
            }
            else {
              cmp_right = "%" + cmp_right;
            }

            cmp_left = "%" + cmp_left;

            if (cmp_opt == "<") {
              out << out_assign_with_cmp("%"+w,cmp_left,cmp_right,_8bitregi[w],"setl");
            }
            else if (cmp_opt == "<=") {
              out << out_assign_with_cmp("%"+w,cmp_left,cmp_right,_8bitregi[w],"setle");
            }
            else if (cmp_opt == "=") {
              out << out_assign_with_cmp("%"+w,cmp_left,cmp_right,_8bitregi[w],"sete");
            }
          }
        }
        else if (!i->mem_opt.empty()) {
          // assign mem
          // "mem" at left
          if (!i->is_right_mem) {
            std::string x = i->operands[0]; // x
            std::string M = i->operands[1]; // M
            std::string s = i->operands[2]; // s
            
            left = M + "(%" + x + ")";
            decompose_s(s,right);

          }
          // "mem" at right
          else {
            std::string x = i->operands[1]; // x
            std::string M = i->operands[2]; // M
            std::string w = i->operands[0]; // w
            
            left = "%" + w;
            right = M + "(%" + x + ")";
          }
          out << "  movq " << right << ", " << left << endl;
        }
        else {
          // simple assign
          std::string s = i->operands.back();
          std::string w = i->operands.front();
          
          left = "%" + w;
          decompose_s(s,right);
          out << "  movq " << right << ", " << left << endl;
        }
      } 

      // call instruction
      else if (opt == "call") {                              // call _label
        // std::string::size_type sz;
        std::string call_function_name = i->operands.front();
        if (call_function_name == "print" || call_function_name == "allocate") {
          out << "  call " << call_function_name << endl;
        }
        else if (call_function_name == "array-error") {
          out << "  call array_error" << endl;
        }
        else {
          int call_argument = std::stoi(i->operands.back());
          out << "  subq $" << std::to_string(call_argument < 6 ? 8 : (call_argument - 5) * 8) << ", " << "%rsp" << endl;
          if (call_function_name.at(0) == ':') {
            out << "  jmp _" << call_function_name.substr(1) << endl;
          } else {
            out << "  jmp *%" << call_function_name << endl;
          }
        }
      }
      else if (opt == "return") {                            // retq
        // if (f->locals != 0) {
        out << "  addq $" << std::to_string((f->arguments < 6 ? 0 : (f->arguments - 6) * 8) + f->locals * 8) << ", %rsp" << endl;
        // }
        out << "  retq" << endl;
      }
      else if (opt == "goto") {
        out << "  jmp _" << i->operands.back().substr(1) << endl;
      }
      else if (opt == "label") {
        out << "_" << i->operands.back().substr(1) << ":" << endl;
      }
      else if (opt == "cjump") {
        if (i->cmp_opt.empty()) {
          cout << "cjump error." << endl;
          return 0;
        }
        // std::string cmp_jmp = "";
        std::string cmp_opt = i->cmp_opt;
        std::string cmp_left = i->operands[0];
        std::string cmp_right = i->operands[1];
        std::string l1 = "_"+(i->operands[2]).substr(1);
        std::string l2 = "_"+(i->operands[3]).substr(1);
        if (myisdigit(cmp_left) && myisdigit(cmp_right)) {
          // number cmp number
          int cmp_res = mycmp(std::stoi(cmp_left),std::stoi(cmp_right),cmp_opt);
          if (cmp_res == 1) {
            out << "  jmp " << l1 << endl;
          }
          else {
            out << "  jmp " << l2 << endl;
          }
        }
        else if (myisdigit(cmp_left) && !myisdigit(cmp_right)) {
          // number cmp regi
          // inverse two operands
          cmp_left = "$" + cmp_left;
          cmp_right = "%" + cmp_right;
          if (cmp_opt == "<") {
            out << out_cjump_with_cmp(cmp_right,cmp_left,l1,l2,"jg");
          }
          else if (cmp_opt == "<=") {
            out << out_cjump_with_cmp(cmp_right,cmp_left,l1,l2,"jge");
          }
          else if (cmp_opt == "=") {
            out << out_cjump_with_cmp(cmp_right,cmp_left,l1,l2,"je");
          }
        }
        else  {
          // regi cmp number | regi cmp regi
          if (myisdigit(cmp_right)) {
            cmp_right = "$" + cmp_right;
          }
          else {
            cmp_right = "%" + cmp_right;
          }

          cmp_left = "%" + cmp_left;

          if (cmp_opt == "<") {
            out << out_cjump_with_cmp(cmp_left,cmp_right,l1,l2,"jl");
          }
          else if (cmp_opt == "<=") {
            out << out_cjump_with_cmp(cmp_left,cmp_right,l1,l2,"jle");
          }
          else if (cmp_opt == "=") {
            out << out_cjump_with_cmp(cmp_left,cmp_right,l1,l2,"je");
          }
        }
      }
      else if (opt == "++") {                                // inc %rdi
        out << "  inc %" << i->operands.front() << endl;
      } 
      else if (opt == "--") {                                // dec %rdi
        out << "  dec %" << i->operands.front() << endl;
      }
      else if (opt == "@") {
        out << "  lea (%" << i->operands[1] << ", %" << i->operands[2] << ", " << i->operands[3] << "), %" << i->operands[0] << endl;
      }
      
      // aop instruction
      // addq(subq, imulq, andq) %rax, %rdi(number) | addq(subq) M(%rsp), %rdi | addq(subq) %rdi, M(%rsp)
      else if (opt == "+=" || opt == "-=" || opt == "*=" || opt == "&=") {     
        std::string aop_left = "";
        std::string aop_right = "";

        // "mem" at left
        if (!i->mem_opt.empty() && !i->is_right_mem) {
          std::string x = i->operands[0]; // x
          std::string M = i->operands[1]; // M
          std::string t = i->operands[2]; // t
          
          aop_left = M + "(%" + x + ")";

          decompose_t(t, aop_right);
        }
        // "mem" at right
        else if (!i->mem_opt.empty() && i->is_right_mem) {
          std::string x = i->operands[1]; // x
          std::string M = i->operands[2]; // M
          std::string w = i->operands[0]; // w
          
          aop_left = "%" + w;
          aop_right = M + "(%" + x + ")";
        }
        // no "mem"
        else {
          std::string w = i->operands[0]; // M
          std::string t = i->operands[1]; // t

          aop_left = "%" + w;
          decompose_t(t, aop_right);
        }
        if (opt == "+=") {
          out << "  addq " << aop_right << ", " << aop_left << endl;
        }
        else if (opt == "-=") {
          out << "  subq " << aop_right << ", " << aop_left << endl;
        }
        else if (opt == "*=") {
          out << "  imulq " << aop_right << ", " << aop_left << endl;
        }
        else if (opt == "&=") {
          out << "  andq " << aop_right << ", " << aop_left << endl;
        }
        else {
          cout << "You should never see this." << endl;
        }
      }


      // sop instrction
      else if (opt == "<<=" || opt == ">>=") {
        std::string sop_left = "";
        std::string sop_right = "";

        sop_left = i->operands[0]; // x
        sop_right = i->operands[1]; // M

        if (myisdigit(sop_right)) {
          sop_right = "$" + sop_right;
        }
        else {
          sop_right = _8bitregi[sop_right];
        }

        if (opt == "<<=") {
          out << "  salq " << sop_right << ", %" << sop_left << endl;
        } else {
          out << "  sarq " << sop_right << ", %" << sop_left << endl;
        }
      }
    }
  }
  out.close();

  // cout << p.functions[0]->name << " has " << p.functions[0]->arguments << " parameters and " << p.functions[0]->locals << " local variables" << "; instructions:" << endl;
  // for (auto i : p.functions[0]->instructions) {
  //   cout << i->instr_string << " has opt " << i->opt;
  //   if (!i->mem_opt.empty()) {
  //      cout << " and " << i->mem_opt;
  //   }
  //   if (!i->cmp_opt.empty()) {
  //      cout << " and " << i->cmp_opt;
  //   }
  //   cout << ", has operands: ";
  //   for (auto optind : i->operands) {
  //     cout << optind << ", ";
  //   }
  //   cout << endl;
  // }

  // system("gcc -c runtime.c -o runtime.o");
  system("as -o prog.o prog.S");
  system("gcc -o a.out prog.o runtime.o");
  return 0;
}


