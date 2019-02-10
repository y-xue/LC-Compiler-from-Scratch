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

std::vector<std::string> args = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
std::vector<std::string> caller_save = {"r10", "r11", "r8", "r9", "rax", "rcx", "rdi", "rdx", "rsi"};
std::vector<std::string> callee_save = {"r12", "r13", "r14", "r15", "rbp", "rbx"};

std::vector<std::string> GP_registers = {"rdi", "rsi", "rdx", "rcx", "r8", "r9", "rax", "r10", "r11", "r12", "r13", "r14", "r15", "rbp", "rbx"};
std::vector<std::string> registers_without_rcx = {"rdi", "rsi", "rdx", "r8", "r9", "rax", "r10", "r11", "r12", "r13", "r14", "r15", "rbp", "rbx"};
std::vector<std::string> H1_sorted_color = {"r10", "r11", "r8", "r9", "rax", "rcx", "rdi", "rdx", "rsi", "r12", "r13", "r14", "r15", "rbp", "rbx"};

std::map<std::string, int> function_invokeNum;

void print_instruction(IR::Instruction *i) {
  cout << i->toString() << endl;
}

std::string remove_percent(std::string s) {
    if (!s.empty() && s.at(0) == '%') {
        s = s.substr(1);
    }
    return s;
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

  IR::Program p = IR::IR_parse_file(argv[optind]);

  cout << "printing program:" << endl;
  for (auto f : p.functions) {
    cout << f->return_type << " ";
    cout << f->name;
    
    for (auto arg : f->args) {
      cout << " " << arg;
    }
    cout << endl;
    for (auto bb : f->bbs) {
      cout << bb->label << endl;
      int n = 1;
      for (auto i : bb->instructions) {
        cout << n++ << ": ";
        print_instruction(i);
      }
      print_instruction(bb->terminator);
    }
    cout << endl;
  }

  std::string program_str = "";
  cout << "After translating:" << endl;
  for (auto f : p.functions) {
    f->declare_var();
    
    cout << f->return_type << " ";
    cout << f->name;
    program_str += "define " + f->name + "(";
    
    for (int i = 0; i < f->args.size(); i++) {
      cout << " " << remove_percent(f->args[i]);
      if (i == 1) {
        program_str += remove_percent(f->args[i]);
      }
      else if (i % 2 == 1) {
        program_str += ", " + remove_percent(f->args[i]);
      }
    }
    program_str += ") {\n";
    
    cout << endl;
    for (auto bb : f->bbs) {
      cout << bb->label << endl;
      program_str += bb->label + "\n";
      int n = 1;
      for (auto i : bb->instructions) {
        cout << n++ << ": ";
        cout << i->translate_to_L3(bb);
        program_str += i->translate_to_L3(bb);
      }
      cout << bb->terminator->translate_to_L3(bb);
      program_str += bb->terminator->translate_to_L3(bb);
    }
    program_str += "}\n";
    cout << endl;
  }

  ofstream out;
  out.open("prog.L3", ios::out);
  out << program_str;
  out.close();

  return 0;
}
