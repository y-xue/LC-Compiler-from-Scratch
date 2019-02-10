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
#include <vector>
#include <map>
#include <stack>
#include <climits>

using namespace std;

std::map<std::string, int> function_invokeNum;

void print_instruction(LA::Instruction *i) {
  if (!i->array_check_instructions.empty()) {
    for (auto ti : i->array_check_instructions) {
      print_instruction(ti);
    }
  }
  if (!i->en_decoded_instructions.empty()) {
    for (auto ti : i->en_decoded_instructions) {
      cout << ti->toString() << endl;
    }
  }
  else {
    cout << i->toString() << endl;
  }
}

std::string get_instruction_str(LA::Instruction *i) {
  std::string i_str = "";
  if (!i->array_check_instructions.empty() || !i->en_decoded_instructions.empty()) {
    for (auto ti : i->array_check_instructions) {
      i_str += get_instruction_str(ti);
    }
    for (auto ti : i->en_decoded_instructions) {
      i_str += ti->toString() + "\n";
    }
  }
  else {
    i_str += i->toString() + "\n";
  }
  return i_str;
}

std::string remove_percent(std::string s) {
    if (!s.empty() && s.at(0) == '%') {
        s = s.substr(1);
    }
    return s;
}

std::string create_label_with_suffix(LA::Function *f) {
    std::string l = ":tlb" + std::to_string(f->label_suffix++);
    while (f->labels.find(l) != f->labels.end()) {
        l = ":tlb" + std::to_string(f->label_suffix++);
    }
    f->labels.insert(l);
    return l;
}

std::vector<LA::Instruction *> generate_bb(LA::Function *f) {
  cout << "generating bb" << endl;
  std::vector<LA::Instruction *> newInsts;
  bool startBB = true;

  for (auto i : f->instructions) {
    print_instruction(i);
    if (startBB) {
      if (LA::LBL != i->get_inst_type()) {
        newInsts.push_back(new LA::Label_Instruction(create_label_with_suffix(f)));
      }
      startBB = false;
    }
    else if (LA::LBL == i->get_inst_type()) {
      cout << "herererererere: ";
      cout << i->toString() << endl;
      // cout << i->get_label() << endl;
      newInsts.push_back(new LA::Br1_Instruction(i->get_label()));
    }
    newInsts.push_back(i);
    if (LA::TE == i->get_inst_type()) {
      startBB = true;
    }
  }

  return newInsts;
}

std::vector<LA::Instruction *> unfold_instructions(LA::Function *f) {
  std::vector<LA::Instruction *> newInsts;
  for (auto i : f->instructions) {
    if (!i->array_check_instructions.empty() || !i->en_decoded_instructions.empty()) {
      for (auto ti : i->array_check_instructions) {
        if (ti->en_decoded_instructions.empty()) {
          newInsts.push_back(ti);
        }
        else {
          for (auto tii : ti->en_decoded_instructions) {
            newInsts.push_back(tii);
          }
        }
        // newInsts = append_vec(newInsts,ti->en_decoded_instructions);
      }
      for (auto ti : i->en_decoded_instructions) {
        newInsts.push_back(ti);
      }
    }
    else {
      newInsts.push_back(i);
    }
  }
  return newInsts;
}

std::map<std::string, std::string> var_reference_map;

// void get_var_reference_map(Program p) {

// }

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

  LA::Program p = LA::LA_parse_file(argv[optind]);

  for (auto f : p.functions) {
    f->declare_var();

    for (auto i : f->instructions) {
      i->check_array(f);
      for (auto ca : i->array_check_instructions) {
        ca->encode_constant();
      }
      i->encode_constant();
      i->to_decode(f);
      i->to_encode(f);
    }
    f->instructions = unfold_instructions(f);
    f->instructions = generate_bb(f);
  }

  // cout << "printing program:" << endl;
  // for (auto f : p.functions) {
  //   cout << f->return_type << " ";
  //   cout << f->name;
  //   f->declare_var();

  //   for (auto arg : f->args) {
  //     cout << " " << arg;
  //   }
  //   int n = 1;
  //   cout << endl;
  //   for (auto i : f->instructions) {
  //     cout << n++ << ": ";
  //     // print_instruction(i);
  //     cout << i->toString() << endl;
  //   }
  //   cout << endl;
  // }

  std::string program_str = "";
  for (auto f : p.functions) {
    f->declare_var();

    program_str += "define " + f->return_type + " :" + f->name + "(";

    for (int i = 0; i < f->args.size(); i++) {
      if (i % 2 == 0) {
        program_str += f->args[i] + " ";
      }
      else if (i != f->args.size() - 1) {
        program_str += f->args[i] + ", ";
      }
      else {
        program_str += f->args[i];
      }
      // if (i == 0) {
      //   program_str += f->args[i] + " " + f->args[++i];
      // }
      // else {
      //   program_str += ", " + f->args[i] + " " + f->args[++i];
      // }
    }

    program_str += ") {\n";

    for (auto i : f->instructions) {
      // program_str += get_instruction_str(i);
      program_str += i->toString() + "\n";
    }

    program_str += "}\n";
  }

  ofstream out;
  out.open("prog.IR", ios::out);
  out << program_str;
  out.close();

  // std::string program_str = "";
  // cout << "After translating:" << endl;
  // for (auto f : p.functions) {
  //   f->declare_var();
    
  //   cout << f->return_type << " ";
  //   cout << f->name;
  //   program_str += "define " + f->name + "(";
    
  //   for (int i = 0; i < f->args.size(); i++) {
  //     cout << " " << remove_percent(f->args[i]);
  //     if (i == 1) {
  //       program_str += remove_percent(f->args[i]);
  //     }
  //     else if (i % 2 == 1) {
  //       program_str += ", " + remove_percent(f->args[i]);
  //     }
  //   }
  //   program_str += ") {\n";
    
  //   cout << endl;
  //   for (auto bb : f->bbs) {
  //     cout << bf->label << endl;
  //     program_str += bf->label + "\n";
  //     int n = 1;
  //     for (auto i : bf->instructions) {
  //       cout << n++ << ": ";
  //       cout << i->translate_to_IR(bb);
  //       program_str += i->translate_to_IR(bb);
  //     }
  //     cout << bf->terminator->translate_to_IR(bb);
  //     program_str += bf->terminator->translate_to_IR(bb);
  //   }
  //   program_str += "}\n";
  //   cout << endl;
  // }

  // ofstream out;
  // out.open("prog.L3", ios::out);
  // out << program_str;
  // out.close();

  return 0;
}
