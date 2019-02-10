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

// void print_instruction(LB::Instruction *i) {
//   if (!i->array_check_instructions.empty()) {
//     for (auto ti : i->array_check_instructions) {
//       print_instruction(ti);
//     }
//   }
//   if (!i->en_decoded_instructions.empty()) {
//     for (auto ti : i->en_decoded_instructions) {
//       cout << ti->toString() << endl;
//     }
//   }
//   else {
//     cout << i->toString() << endl;
//   }
// }

// std::string get_instruction_str(LB::Instruction *i) {
//   std::string i_str = "";
//   if (!i->array_check_instructions.empty() || !i->en_decoded_instructions.empty()) {
//     for (auto ti : i->array_check_instructions) {
//       i_str += get_instruction_str(ti);
//     }
//     for (auto ti : i->en_decoded_instructions) {
//       i_str += ti->toString() + "\n";
//     }
//   }
//   else {
//     i_str += i->toString() + "\n";
//   }
//   return i_str;
// }

// std::string remove_percent(std::string s) {
//     if (!s.empty() && s.at(0) == '%') {
//         s = s.substr(1);
//     }
//     return s;
// }

std::string create_label_with_suffix(LB::Function *f) {
  std::string l = ":lbl" + std::to_string(f->label_suffix++);
  while (f->labels.find(l) != f->labels.end()) {
      l = ":lbl" + std::to_string(f->label_suffix++);
  }
  f->labels.insert(l);
  return l;
}

std::string create_temp_var(LB::Function *f) {
  int var_suffix = 0;
  std::string v = "%tmpvar" + std::to_string(var_suffix++);
  while (f->var_set.find(v) != f->var_set.end()) {
      v = "%tmpvar" + std::to_string(var_suffix++);
  }
  f->var_set.insert(v);
  return v;
}

// std::vector<LB::Instruction *> generate_bb(LB::Function *f) {
//   cout << "generating bb" << endl;
//   std::vector<LB::Instruction *> newInsts;
//   bool startBB = true;

//   for (auto i : f->instructions) {
//     print_instruction(i);
//     if (startBB) {
//       if (LB::LBL != i->get_inst_type()) {
//         newInsts.push_back(new LB::Label_Instruction(create_label_with_suffix(f)));
//       }
//       startBB = false;
//     }
//     else if (LB::LBL == i->get_inst_type()) {
//       cout << "herererererere: ";
//       cout << i->toString() << endl;
//       // cout << i->get_label() << endl;
//       newInsts.push_back(new LB::Br1_Instruction(i->get_label()));
//     }
//     newInsts.push_back(i);
//     if (LB::TE == i->get_inst_type()) {
//       startBB = true;
//     }
//   }

//   return newInsts;
// }

// std::vector<LB::Instruction *> unfold_instructions(LB::Function *f) {
//   std::vector<LB::Instruction *> newInsts;
//   for (auto i : f->instructions) {
//     if (!i->array_check_instructions.empty() || !i->en_decoded_instructions.empty()) {
//       for (auto ti : i->array_check_instructions) {
//         if (ti->en_decoded_instructions.empty()) {
//           newInsts.push_back(ti);
//         }
//         else {
//           for (auto tii : ti->en_decoded_instructions) {
//             newInsts.push_back(tii);
//           }
//         }
//         // newInsts = append_vec(newInsts,ti->en_decoded_instructions);
//       }
//       for (auto ti : i->en_decoded_instructions) {
//         newInsts.push_back(ti);
//       }
//     }
//     else {
//       newInsts.push_back(i);
//     }
//   }
//   return newInsts;
// }

// std::map<std::string, std::string> var_reference_map;

// void get_var_reference_map(Program p) {

// }

int find_index_of_while(std::string l, std::map<int, std::string> m) {
  for (auto p : m) {
    if (p.second == l) {
      return p.first;
    }
  }
  return -1;
}

void translate_while(LB::Function *f) {
  std::map<int, std::string> beginWhile;
  std::map<int, std::string> endWhile;
  std::map<int, std::string> condLabels;

  std::vector<LB::Instruction *> new_instructions;

  // translate while instruction
  int j = 1;
  for (int i = 0; i < f->scope->instructions.size(); i++) {
    if (f->scope->instructions[i]->get_inst_type() == LB::WHILE) {
      int w = i + 3 * (j++);

      std::vector<std::string> labels = f->scope->instructions[i]->get_items();

      std::string op_left = labels[0];
      std::string op = labels[1];
      std::string op_right = labels[2];
      beginWhile[w] = labels[3];
      endWhile[w] = labels[4];

      LB::Label_Instruction *newLI = new LB::Label_Instruction();
      newLI->i_type = LB::OTHER;
      std::string l = create_label_with_suffix(f);
      newLI->label = l;
      condLabels[w] = l;


      new_instructions.push_back(newLI);

      std::string var = create_temp_var(f);

      LB::Type_Instruction *newTI = new LB::Type_Instruction();
      newTI->i_type = LB::TYPE;
      newTI->type = "int64";
      newTI->vars = std::vector<std::string>(1,var);

      new_instructions.push_back(newTI);



      LB::Op_Assign_Instruction *newOI = new LB::Op_Assign_Instruction(var,op,op_left,op_right);
      newOI->i_type = LB::OTHER;

      f->scope->instructions[i]->set_tempvar(var);

      new_instructions.push_back(newOI);
    }

    new_instructions.push_back(f->scope->instructions[i]);
  }


  // map instructions to their loop
  std::stack<int> loopStack;
  std::map<int, int> loop;
  std::set<int> whileSeen;
  
  for (int i = 0; i < new_instructions.size(); i++) {
    if (!loopStack.empty()) {
      loop[i] = loopStack.top();
    }

    if (new_instructions[i]->get_inst_type() == LB::WHILE) {
      loopStack.push(i);
      whileSeen.insert(i);
      continue;
    }

    if (new_instructions[i]->get_inst_type() == LB::LBL) {
      std::string l = (new_instructions[i]->get_items())[0];
      int w = find_index_of_while(l,beginWhile);
      if (w != -1 && whileSeen.find(w) == whileSeen.end()) {
        loopStack.push(w);
        continue;
      }
    }

    if (new_instructions[i]->get_inst_type() == LB::LBL) {
      std::string l = (new_instructions[i]->get_items())[0];
      int w = find_index_of_while(l,endWhile);
      if (w != -1) {
        loopStack.pop();
        continue;
      }
    }
  }

  // cout << "new_instructions:" << endl;
  // int k = 0;
  // for (auto i : new_instructions) {
  //   cout << k++ << ": " << i->toString();
  // }

  // cout << "begin whiles:" << endl;
  // for (auto p : beginWhile) {
  //   cout << p.first << " " << p.second << endl;
  // }
  // cout << "end whiles:" << endl;
  // for (auto p : endWhile) {
  //   cout << p.first << " " << p.second << endl;
  // }
  // cout << "condLabels:" << endl;
  // for (auto p : condLabels) {
  //   cout << p.first << " " << p.second << endl;
  // }
  // cout << "loop:" << endl;
  // for (auto p : loop) {
  //   cout << p.first << ": " << new_instructions[p.first]->toString() << " " << p.second << endl;
  // }

  // translate continue
  for (int i = 0; i < new_instructions.size(); i++) {
    if (new_instructions[i]->get_inst_type() == LB::CONTINUE) {
      int w = loop[i];
      std::string l_cond = condLabels[w];
      new_instructions[i]->set_tempvar(l_cond);
    }
    else if (new_instructions[i]->get_inst_type() == LB::BREAK) {
      int w = loop[i];
      std::string l_exit = endWhile[w];
      new_instructions[i]->set_tempvar(l_exit);
    }
  }

  f->scope->instructions = new_instructions;
}

void translate_if(LB::Function *f) {
  std::vector<LB::Instruction *> new_instructions;

  for (int i = 0; i < f->scope->instructions.size(); i++) {
    if (f->scope->instructions[i]->get_inst_type() == LB::IF) {
      std::vector<std::string> labels = f->scope->instructions[i]->get_items();

      std::string op_left = labels[0];
      std::string op = labels[1];
      std::string op_right = labels[2];

      std::string var = create_temp_var(f);
      LB::Type_Instruction *newTI = new LB::Type_Instruction();
      newTI->i_type = LB::TYPE;
      newTI->type = "int64";
      newTI->vars = std::vector<std::string>(1,var);
      new_instructions.push_back(newTI);


      LB::Op_Assign_Instruction *newOI = new LB::Op_Assign_Instruction(var,op,op_left,op_right);
      newOI->i_type = LB::OTHER;
      new_instructions.push_back(newOI);

      f->scope->instructions[i]->set_tempvar(var);
    }

    new_instructions.push_back(f->scope->instructions[i]);
  }

  f->scope->instructions = new_instructions;

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

  LB::Program p = LB::LB_parse_file(argv[optind]);

  for (auto f : p.functions) {
    // cout << f->return_type << " ";
    // cout << f->name;
    // for (auto arg : f->args) {
    //   cout << " " << arg;
    // }
    // cout << endl;

    f->declare_var();

    f->scope->rename_var(f);
    // f->scope->
    f->scope->append_new_name_map(f->scope->new_name_map);
    f->scope->bind();

    // for (auto i : f->scope->instructions) {
    //   if (i->get_inst_type() == LB::SCOPE) {
    //     i->rename_var(f);
    //     i->append_new_name_map(f->scope->new_name_map);
    //     i->bind();
    //   }
    // }
    f->scope->instructions = f->scope->remove_scope();

    translate_while(f);
    translate_if(f);

    // cout << f->scope->toString();
    // for (auto i : f->scope->instructions) {
    //   cout << i->toString() << endl;
    // }
  }

  std::string program_str = "";
  for (auto f : p.functions) {
    program_str += f->return_type + " ";
    program_str += f->name + " (";
    for (int i = 0; i < f->args.size(); i++) {
      // cout << f->args[i] << " ";
      if (i == 0) {
        program_str += f->args[i] + " " + f->args[i+1];
      }
      else {
        program_str += ", " + f->args[i] + " " + f->args[i+1];
      }
      i++;
    }
    program_str += ")";

    program_str += f->scope->toString();
  }

  // cout << program_str << endl;

  ofstream out;
  out.open("prog.a", ios::out);
  out << program_str;
  out.close();

  // for (auto f : p.functions) {
  //   f->declare_var();

  //   for (auto i : f->instructions) {
  //     i->check_array(f);
  //     for (auto ca : i->array_check_instructions) {
  //       ca->encode_constant();
  //     }
  //     i->encode_constant();
  //     i->to_decode(f);
  //     i->to_encode(f);
  //   }
  //   f->instructions = unfold_instructions(f);
  //   f->instructions = generate_bb(f);
  // }

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

  // std::string program_str = "";
  // for (auto f : p.functions) {
  //   f->declare_var();

  //   program_str += "define " + f->return_type + " :" + f->name + "(";

  //   for (int i = 0; i < f->args.size(); i++) {
  //     if (i % 2 == 0) {
  //       program_str += f->args[i] + " ";
  //     }
  //     else if (i != f->args.size() - 1) {
  //       program_str += f->args[i] + ", ";
  //     }
  //     else {
  //       program_str += f->args[i];
  //     }
  //     // if (i == 0) {
  //     //   program_str += f->args[i] + " " + f->args[++i];
  //     // }
  //     // else {
  //     //   program_str += ", " + f->args[i] + " " + f->args[++i];
  //     // }
  //   }

  //   program_str += ") {\n";

  //   for (auto i : f->instructions) {
  //     // program_str += get_instruction_str(i);
  //     program_str += i->toString() + "\n";
  //   }

  //   program_str += "}\n";
  // }

  // ofstream out;
  // out.open("prog.IR", ios::out);
  // out << program_str;
  // out.close();

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
