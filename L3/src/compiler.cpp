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

std::map<std::string, std::string> reference_map;

std::set<std::string> not_losing_reference;

std::string get_reference(std::string v) {
  if (reference_map.find(v) != reference_map.end() && not_losing_reference.find(v) == not_losing_reference.end()) {
    return reference_map[v];
  }
  return v;
}

bool label_exist(std::vector<L3::Function *> functions, std::string l) {
  for (int j = 0; j < functions.size(); j++) {
    if (functions[j]->labels.find(l) != functions[j]->labels.end()) {
      return true;
    }
  }
  return false;
}

void add_suffix_to_label(L3::Program &p) {
  for (int i = 0; i < p.functions.size(); i++) {
    // cout << p.functions[i]->name << endl;
    
    std::vector<std::string> newLabels = std::vector<std::string> (p.functions[i]->labels.cbegin(), p.functions[i]->labels.cend());

    for (auto l : newLabels) {
      // cout << l << endl;
      p.functions[i]->labels.erase(l);
      // cout << "wtf1" << endl;
      
      std::string new_label = l;
      while (label_exist(p.functions, new_label)) {
        new_label += std::to_string(p.label_suffix++);
      }
      // cout << "wtf2" << endl;
      if (new_label != l) {
        for (auto instr : p.functions[i]->instructions) {
          instr->replace_label(l,new_label);
        }
      }
      
      p.functions[i]->labels.insert(new_label);
      // cout << "wtf3" << endl;
    }
  }
}

bool isStrDigit(std::string s) {
    if (s.empty()) return false;
    if (isdigit(s.at(0))) {
        return true;
    }
    else if (s.size() > 1 && (s.at(0)=='+' || s.at(0)=='-') && isdigit(s.at(1))) {
        return true;
    }
    else return false;
}

bool isop(std::string s) {
  return (s == "+" || s == "-" || s == "*" || s == "&"
    || s == "<" || s == "<=" || s == "="
    || s == "<<" || s == ">>" || s == "load" || s == "store");
}

bool is_t(std::string s) {
  if (s == "+" || s == "-" || s == "*" || s == "&"
    || s == "<" || s == "<=" || s == "="
    || s == "<<" || s == ">>" || s == "load" || s == "store"
    || s == "call" || s == "return" || s == "<-"
    || s == "br" || s == "label") {
    return false;
  }
  return true;
}

void swapItem(std::string &s1, std::string &s2) {
  std::string tmp;
  tmp = s1;
  s1 = s2;
  s2 = tmp;
}

bool item_type_eq(L3::Item_Type i1, L3::Item_Type i2) {
  if (i1 >= L3::ASGN && i2 >= L3::ASGN) {
    return i1 == i2;
  }
  else if (i1 < L3::ASGN && i2 < L3::ASGN) {
    return i1 >= i2;
  }
  else {
    return false;
  }
}

bool node_eq(L3::Node * n1, L3::Node * n2) {
  if (item_type_eq(n1->ittp, n2->ittp) || item_type_eq(n1->second_ittp, n2->ittp)) {
    return true;
  }
  if (!n1->val.empty()) {
    // if (n2->ittp == L3::N1) {
    //   cout << "n2 is N1" << endl;
    //   n1->printNode();
    //   cout << endl;
    // }
    return item_type_eq(n1->val_ittp, n2->ittp);
  }
}

void print_instruction(L3::Instruction *i) {
  cout << i->toString() << endl;
}

vector<L3::Tree *> create_forest(L3::Function * f) {
  vector<L3::Tree *> forest;

  for (auto i : f->instructions) {
    i->genTree();
    forest.push_back(i->getTree());
  }

  return forest;
}

bool replace_node(L3::Node * node, L3::Node * var, L3::Node * alter_var) {
  if (!node) {
    return false;
  }

  bool success = false;
  for (int i = 0; i < node->children.size(); i++) {
    if (node->children[i]) {
      if (node->children[i]->item == var->item) {
        // cout << "hererererere" << endl;
        alter_var->val = var->item;
        node->children[i] = alter_var;
        // alter_var->printNode();
        // cout << endl;
        success = true;

        if (is_t(alter_var->item)) {
          reference_map[var->item] = alter_var->item;
        }
      }
      else if(replace_node(node->children[i], var, alter_var)) {
        success = true;
      }
    }
  }

  return success;
}

bool merge_tree(L3::Tree * t, L3::Tree * merged_tree) {
  // t->printTree();
  // merged_tree->printTree();
  if (!t->root || t->root->item != "<-") {
    return false;
  }
  if (t->root->children[1]->item == "call") {
    // cout << "111111111111" << endl;
    return false;
  }

  L3::Node * var = t->root->children[0];
  L3::Node * alter_var = t->root->children[1];

  if ((merged_tree->root->item == "return" 
    || merged_tree->root->item == "call" 
    || merged_tree->root->item == "br") && (alter_var->ittp >= L3::ASGN)) {
    // cout << "00000000000000" << endl;
      return false;
  }

  if ((merged_tree->root->item == "<-" 
    && merged_tree->root->children[1]->item == "call")
     && (alter_var->ittp >= L3::ASGN)) {
    // cout << "00000000000000" << endl;
    // alter_var->printNode();
    // cout << endl;
      return false;
  }

  if (merged_tree->root && merged_tree->root->item == "<-") {
    return replace_node(merged_tree->root->children[1], var, alter_var);
  }
  return replace_node(merged_tree->root, var, alter_var);
}

vector<L3::Tree *> merge_forest(vector<L3::Tree *> forest) {

  vector<L3::Tree *> new_forest;
  
  if (forest.empty()) {
    return new_forest;
  }

  for (int i = 1; i < forest.size(); i++) {
    if (!merge_tree(forest[i-1],forest[i])) {
      new_forest.push_back(forest[i-1]);
    }
  }
  new_forest.push_back(forest.back());

  return new_forest;
}

L3::Tree * L2_gen_simple_assign_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::SIMPLE_ASSIGN;
  t->root = new L3::Node("<-", L3::ASGN);
  t->root->children.push_back(new L3::Node(L3::VAR));
  t->root->children.push_back(new L3::Node(L3::S));
  return t;
  // t->printTree();  
}

L3::Tree * L2_gen_left_mem_assign_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::LEFT_MEM_ASSIGN;

  t->root = new L3::Node("store", L3::STORE);

  return t;
  // t->printTree();  
}

L3::Tree * L2_gen_right_mem_assign_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::RIGHT_MEM_ASSIGN;
  t->root = new L3::Node("load", L3::LOAD);

  return t;
  // t->printTree();  
}

L3::Tree * L2_gen_aop_sop_cmp_asgn(L3::Item_Type ittp) {
  L3::Tree * t = new L3::Tree();
  switch (ittp) {
    case L3::PLUS:
      t->tile_type = L3::AOP_SOP_CMP_ASGN_PLUS;
      break;
    case L3::MINUS:
      t->tile_type = L3::AOP_SOP_CMP_ASGN_MINUS;
      break;
    case L3::MUL:
      t->tile_type = L3::AOP_SOP_CMP_ASGN_MUL;
      break;
    case L3::AD:
      t->tile_type = L3::AOP_SOP_CMP_ASGN_AD;
      break;
    case L3::CMP:
      t->tile_type = L3::AOP_SOP_CMP_ASGN_CMP;
      break;
    case L3::SHIFT:
      t->tile_type = L3::AOP_SOP_CMP_ASGN_SHIFT;
      break;
    default:
      cout << "Unknown item type" << endl;
  }

  t->root = new L3::Node("<-", L3::ASGN);
  
  L3::Node * assign_right = new L3::Node(ittp);

  assign_right->children.push_back(new L3::Node(L3::T));
  assign_right->children.push_back(new L3::Node(L3::T));
  
  t->root->children.push_back(new L3::Node( L3::VAR));
  t->root->children.push_back(assign_right);

  return t;
}

L3::Tree * L2_gen_aop_sop_cmp(L3::Item_Type ittp) {
  L3::Tree * t = new L3::Tree();
  switch (ittp) {
    case L3::PLUS:
      t->tile_type = L3::AOP_SOP_CMP_PLUS;
      break;
    case L3::MINUS:
      t->tile_type = L3::AOP_SOP_CMP_MINUS;
      break;
    case L3::MUL:
      t->tile_type = L3::AOP_SOP_CMP_MUL;
      break;
    case L3::AD:
      t->tile_type = L3::AOP_SOP_CMP_AD;
      break;
    case L3::CMP:
      t->tile_type = L3::AOP_SOP_CMP_CMP;
      break;
    case L3::SHIFT:
      t->tile_type = L3::AOP_SOP_CMP_SHIFT;
      break;
    default:
      cout << "Unknown item type" << endl;
  }
  
  
  t->root = new L3::Node(ittp);

  t->root->children.push_back(new L3::Node(L3::T));
  t->root->children.push_back(new L3::Node(L3::T));
  
  return t;
}


L3::Tree * L2_gen_left_mem_plus_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::LEFT_MEM_PLUS;
  t->root = new L3::Node("<-", L3::ASGN);

  L3::Node *assign_left = new L3::Node("load", L3::LOAD);
  L3::Node * left_load_var_node = new L3::Node("+", L3::PLUS);
  left_load_var_node->children.push_back(new L3::Node(L3::VAR));
  left_load_var_node->children.push_back(new L3::Node(L3::M));
  assign_left->children.push_back(left_load_var_node);

  L3::Node *assign_right = new L3::Node("+", L3::PLUS);
  L3::Node *assign_right_left = new L3::Node("load", L3::LOAD);
  L3::Node * right_load_var_node = new L3::Node("+", L3::PLUS);
  right_load_var_node->children.push_back(new L3::Node(L3::VAR));
  right_load_var_node->children.push_back(new L3::Node(L3::M));
  assign_right_left->children.push_back(right_load_var_node);
  
  assign_right->children.push_back(assign_right_left);
  assign_right->children.push_back(new L3::Node(L3::T));
  
  t->root->children.push_back(assign_left);
  t->root->children.push_back(assign_right);

  return t;
  // t->printTree();  
}

L3::Tree * L2_gen_left_mem_minus_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::LEFT_MEM_MINUS;
  t->root = new L3::Node("<-", L3::ASGN);

  L3::Node *assign_left = new L3::Node("load", L3::LOAD);
  L3::Node * left_load_var_node = new L3::Node("+", L3::PLUS);
  left_load_var_node->children.push_back(new L3::Node(L3::VAR));
  left_load_var_node->children.push_back(new L3::Node(L3::M));
  assign_left->children.push_back(left_load_var_node);

  L3::Node *assign_right = new L3::Node("-", L3::MINUS);
  L3::Node *assign_right_left = new L3::Node("load", L3::LOAD);
  L3::Node * right_load_var_node = new L3::Node("+", L3::PLUS);
  right_load_var_node->children.push_back(new L3::Node(L3::VAR));
  right_load_var_node->children.push_back(new L3::Node(L3::M));
  assign_right_left->children.push_back(right_load_var_node);
  assign_right->children.push_back(assign_right_left);
  assign_right->children.push_back(new L3::Node(L3::T));
  
  t->root->children.push_back(assign_left);
  t->root->children.push_back(assign_right);

  return t;
  // t->printTree();  
}

L3::Tree * L2_gen_right_mem_plus_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::RIGHT_MEM_PLUS;
  t->root = new L3::Node("<-", L3::ASGN);

  L3::Node *assign_right = new L3::Node("+", L3::PLUS);
  L3::Node *assign_right_right = new L3::Node("load", L3::LOAD);
  L3::Node * load_var_node = new L3::Node("+", L3::PLUS);
  load_var_node->children.push_back(new L3::Node(L3::VAR));
  load_var_node->children.push_back(new L3::Node(L3::M));
  assign_right_right->children.push_back(load_var_node);
  assign_right->children.push_back(new L3::Node(L3::T));
  assign_right->children.push_back(assign_right_right);
  
  t->root->children.push_back(new L3::Node(L3::VAR));
  t->root->children.push_back(assign_right);

  return t;
  // t->printTree();  
}

L3::Tree * L2_gen_right_mem_minus_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::RIGHT_MEM_MINUS;
  t->root = new L3::Node("<-", L3::ASGN);

  L3::Node *assign_right = new L3::Node("-", L3::MINUS);
  L3::Node *assign_right_right = new L3::Node("load", L3::LOAD);
  L3::Node * load_var_node = new L3::Node("+", L3::PLUS);
  load_var_node->children.push_back(new L3::Node(L3::VAR));
  load_var_node->children.push_back(new L3::Node(L3::M));
  assign_right_right->children.push_back(load_var_node);
  assign_right->children.push_back(new L3::Node(L3::T));
  assign_right->children.push_back(assign_right_right);
  
  t->root->children.push_back(new L3::Node(L3::VAR));
  t->root->children.push_back(assign_right);

  return t;
  // t->printTree();
}

L3::Tree * L2_gen_cjump_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::CJUMP;
  t->root = new L3::Node("br", L3::BR);
  t->root->children.push_back(new L3::Node(L3::VAR));
  t->root->children.push_back(new L3::Node(L3::LBL));
  t->root->children.push_back(new L3::Node(L3::LBL));
  return t;
}

L3::Tree * L2_gen_label_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::TILE_LABEL;
  t->root = new L3::Node("label", L3::LBL);
  return t;
}

L3::Tree * L2_gen_goto_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::GOTO;
  t->root = new L3::Node("br", L3::BR);
  t->root->children.push_back(new L3::Node(L3::LBL));
  return t;
}

L3::Tree * L2_gen_return_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::TILE_RETURN;
  t->root = new L3::Node("return", L3::RETURN);
  return t;
}

L3::Tree * L2_gen_return_var_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::TILE_VAR_RETURN;
  t->root = new L3::Node("return", L3::RETURN);
  t->root->children.push_back(new L3::Node(L3::T));
  return t;
}

L3::Tree * L2_gen_call_assign_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::TILE_CALL_ASGN;
  t->root = new L3::Node("<-", L3::ASGN);
  t->root->children.push_back(new L3::Node(L3::VAR));
  t->root->children.push_back(new L3::Node("call", L3::CALL));
  return t;
}

L3::Tree * L2_gen_call_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::TILE_CALL;
  t->root = new L3::Node("call", L3::CALL);
  return t;
}

L3::Tree * L2_gen_inc_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::INC;
  t->root = new L3::Node("<-", L3::ASGN);
  
  L3::Node * assign_right = new L3::Node(L3::PLUS);

  assign_right->children.push_back(new L3::Node(L3::VAR));
  assign_right->children.push_back(new L3::Node(L3::N1));
  
  t->root->children.push_back(new L3::Node(L3::VAR));
  t->root->children.push_back(assign_right);
  return t;
}

L3::Tree * L2_gen_dec_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::DEC;
  t->root = new L3::Node("<-", L3::ASGN);
  
  L3::Node * assign_right = new L3::Node(L3::MINUS);

  assign_right->children.push_back(new L3::Node(L3::VAR));
  assign_right->children.push_back(new L3::Node(L3::N1));
  
  t->root->children.push_back(new L3::Node(L3::VAR));
  t->root->children.push_back(assign_right);
  return t;
}

L3::Tree * L2_gen_lea_tree() {
  L3::Tree * t = new L3::Tree();
  t->tile_type = L3::LEA;
  t->root = new L3::Node("<-", L3::ASGN);

  L3::Node *assign_right = new L3::Node("+", L3::PLUS);
  L3::Node *assign_right_right = new L3::Node("*", L3::MUL);
  
  assign_right_right->children.push_back(new L3::Node(L3::VAR));
  assign_right_right->children.push_back(new L3::Node(L3::E));
  
  assign_right->children.push_back(new L3::Node(L3::VAR));
  assign_right->children.push_back(assign_right_right);
  
  t->root->children.push_back(new L3::Node(L3::VAR));
  t->root->children.push_back(assign_right);

  return t;
  // t->printTree();  
}

L3::Tree * generate_L2_tile_by_type(L3::L2_Tile_Type tt) {
  switch (tt) {
    case L3::LEFT_MEM_PLUS :
      return L2_gen_left_mem_plus_tree();
    case L3::LEFT_MEM_MINUS :
      return L2_gen_right_mem_minus_tree();
    case L3::RIGHT_MEM_PLUS :
      return L2_gen_right_mem_plus_tree();
    case L3::RIGHT_MEM_MINUS :
      return L2_gen_right_mem_minus_tree();
    case L3::LEFT_MEM_ASSIGN :
      return L2_gen_left_mem_assign_tree();
    case L3::RIGHT_MEM_ASSIGN :
      return L2_gen_right_mem_assign_tree();
    case L3::LEA :
      return L2_gen_lea_tree();
    case L3::INC :
      return L2_gen_inc_tree();
    case L3::DEC :
      return L2_gen_dec_tree();
    case L3::AOP_SOP_CMP_ASGN_PLUS :
      return L2_gen_aop_sop_cmp_asgn(L3::PLUS);
    case L3::AOP_SOP_CMP_ASGN_MINUS :
      return L2_gen_aop_sop_cmp_asgn(L3::MINUS);
    case L3::AOP_SOP_CMP_ASGN_MUL :
      return L2_gen_aop_sop_cmp_asgn(L3::MUL);
    case L3::AOP_SOP_CMP_ASGN_AD :
      return L2_gen_aop_sop_cmp_asgn(L3::AD);
    case L3::AOP_SOP_CMP_ASGN_CMP :
      return L2_gen_aop_sop_cmp_asgn(L3::CMP);
    case L3::AOP_SOP_CMP_ASGN_SHIFT :
      return L2_gen_aop_sop_cmp_asgn(L3::SHIFT);
    case L3::AOP_SOP_CMP_PLUS :
      return L2_gen_aop_sop_cmp(L3::PLUS);
    case L3::AOP_SOP_CMP_MINUS :
      return L2_gen_aop_sop_cmp(L3::MINUS);
    case L3::AOP_SOP_CMP_MUL :
      return L2_gen_aop_sop_cmp(L3::MUL);
    case L3::AOP_SOP_CMP_AD :
      return L2_gen_aop_sop_cmp(L3::AD);
    case L3::AOP_SOP_CMP_CMP :
      return L2_gen_aop_sop_cmp(L3::CMP);
    case L3::AOP_SOP_CMP_SHIFT :
      return L2_gen_aop_sop_cmp(L3::SHIFT);
    case L3::SIMPLE_ASSIGN :
      return L2_gen_simple_assign_tree();
    case L3::CJUMP :
      return L2_gen_cjump_tree();
    case L3::TILE_LABEL :
      return L2_gen_label_tree();
    case L3::GOTO :
      return L2_gen_goto_tree();
    case L3::TILE_RETURN :
      return L2_gen_return_tree();
    case L3::TILE_VAR_RETURN:
      return L2_gen_return_var_tree();
    case L3::TILE_CALL:
      return L2_gen_call_tree();
    case L3::TILE_CALL_ASGN:
      return L2_gen_call_assign_tree();
    default:
      // cout << tt << endl;
      cout << "Unknown tile type." << endl;
  }
}

std::vector<L3::Tree *> generate_L2_tiles() {
  std::vector<L3::Tree *> tiles;
  
  tiles.push_back(L2_gen_left_mem_plus_tree());
  tiles.push_back(L2_gen_left_mem_minus_tree());
  tiles.push_back(L2_gen_right_mem_plus_tree());
  tiles.push_back(L2_gen_right_mem_minus_tree());

  tiles.push_back(L2_gen_left_mem_assign_tree());
  tiles.push_back(L2_gen_right_mem_assign_tree());

  tiles.push_back(L2_gen_lea_tree());

  tiles.push_back(L2_gen_inc_tree());
  tiles.push_back(L2_gen_dec_tree());

  tiles.push_back(L2_gen_aop_sop_cmp_asgn(L3::PLUS));
  tiles.push_back(L2_gen_aop_sop_cmp_asgn(L3::MINUS));
  tiles.push_back(L2_gen_aop_sop_cmp_asgn(L3::MUL));
  tiles.push_back(L2_gen_aop_sop_cmp_asgn(L3::AD));
  tiles.push_back(L2_gen_aop_sop_cmp_asgn(L3::CMP));
  tiles.push_back(L2_gen_aop_sop_cmp_asgn(L3::SHIFT));

  tiles.push_back(L2_gen_aop_sop_cmp(L3::PLUS));
  tiles.push_back(L2_gen_aop_sop_cmp(L3::MINUS));
  tiles.push_back(L2_gen_aop_sop_cmp(L3::MUL));
  tiles.push_back(L2_gen_aop_sop_cmp(L3::AD));
  tiles.push_back(L2_gen_aop_sop_cmp(L3::CMP));
  tiles.push_back(L2_gen_aop_sop_cmp(L3::SHIFT));

  tiles.push_back(L2_gen_call_assign_tree());

  tiles.push_back(L2_gen_simple_assign_tree());

  tiles.push_back(L2_gen_cjump_tree());
  tiles.push_back(L2_gen_goto_tree());
  tiles.push_back(L2_gen_return_var_tree());
  tiles.push_back(L2_gen_return_tree());
  tiles.push_back(L2_gen_call_tree());

  tiles.push_back(L2_gen_label_tree());

  return tiles;
}

bool match_call_node(L3::Node * node, L3::Node * pattern, std::vector<L3::Tree *> & rest_trees) {
  if (pattern->item != "call") return false;

  pattern->ittp = node->ittp;
  pattern->second_ittp = node->second_ittp;
  pattern->val = node->val;
  pattern->val_ittp = node->val_ittp;

  for (auto ch : node->children) {
    pattern->children.push_back(ch);
  }

  return true;
}

bool match_load_store_node(L3::Node * node, L3::Node * pattern, std::vector<L3::Tree *> & rest_trees) {

  if (pattern->item != "load" && pattern->item != "store") return false;
  if (pattern->item != node->item) return false;

  pattern->ittp = node->ittp;
  pattern->second_ittp = node->second_ittp;
  pattern->val = node->val;
  pattern->val_ittp = node->val_ittp;

  // for (auto ch : node->children) {
  //   pattern->children.push_back(ch);
  // }

  // analyse load_store pattern
  if (node->children[0]->item != "+") { 
    // load
    //   |
    //   x
    pattern->children.push_back(node->children[0]);
    return true;
  }
  else {
    if (node->children[0]->children[0]->ittp < L3::ASGN && node->children[0]->children[1]->ittp < L3::ASGN) {
      pattern->children.push_back(node->children[0]);
      return true;
    }
    if (node->children[0]->children[0]->ittp >= L3::ASGN && node->children[0]->children[1]->ittp < L3::ASGN) {
      // load
      //   |
      //   +
      //  / \
      // op  x
      std::string assign_left = node->children[0]->children[0]->val;
      std::string assign_right = node->children[0]->children[1]->item;
      L3::Node * n = new L3::Node("+",L3::PLUS);
      n->val = node->children[0]->val;
      n->children.push_back(new L3::Node(assign_left,node->children[0]->children[0]->second_ittp));
      n->children.push_back(new L3::Node(assign_right,node->children[0]->children[1]->ittp));

      pattern->children.push_back(n);
      
      L3::Tree * rest_tree = new L3::Tree();
      rest_tree->root = node->children[0]->children[0];
      rest_trees.push_back(rest_tree);

      return true;
    }
    if (node->children[0]->children[0]->ittp < L3::ASGN && node->children[0]->children[1]->ittp >= L3::ASGN) {
      // cout << "should see here" << endl;
      // load
      //   |
      //   +
      //  / \
      // x  op
      std::string assign_left = node->children[0]->children[0]->item;
      std::string assign_right = node->children[0]->children[1]->val;
      L3::Node * n = new L3::Node("+",L3::PLUS);
      n->val = node->children[0]->val;
      n->children.push_back(new L3::Node(assign_left,node->children[0]->children[0]->ittp));
      n->children.push_back(new L3::Node(assign_right,node->children[0]->children[1]->second_ittp));

      pattern->children.push_back(n);
      
      L3::Tree * rest_tree = new L3::Tree();
      rest_tree->root = node->children[0]->children[1];
      rest_trees.push_back(rest_tree);

      // rest_tree->printTree();

      return true;
    }
    // load
    //   |
    //   +
    //  / \
    // op  op
    std::string assign_left = node->children[0]->children[0]->val;
    std::string assign_right = node->children[0]->children[1]->val;
    L3::Node * n = new L3::Node("+",L3::PLUS);
    n->children.push_back(new L3::Node(assign_left,node->children[0]->children[0]->second_ittp));
    n->children.push_back(new L3::Node(assign_right,node->children[0]->children[1]->second_ittp));

    pattern->children.push_back(n);
    
    L3::Tree * rest_tree1 = new L3::Tree();
    rest_tree1->root = node->children[0]->children[0];
    rest_trees.push_back(rest_tree1);

    L3::Tree * rest_tree2 = new L3::Tree();
    rest_tree2->root = node->children[0]->children[1];
    rest_trees.push_back(rest_tree2);

    return true;
  }

  return true;
}

bool match_node(L3::Node * node, L3::Node * pattern, std::vector<L3::Tree *> & rest_trees) {
  if (!pattern) {
    return true;
  }

  if (!node || !node_eq(node,pattern)) {
    return false;
  }

  pattern->item = node->item;
  pattern->val = node->val;

  if (pattern->children.empty()) {
    if (!node->children.empty()) {
      L3::Tree * rest_tree = new L3::Tree();
      rest_tree->root = node;
      rest_trees.push_back(rest_tree);
    }
    return true;
  }
  
  if (node->children.size() != pattern->children.size()) {
    return false;
  }

  for (int i = 0; i < node->children.size(); i++) {
    if (!match_node(node->children[i], pattern->children[i], rest_trees)) {
      return false;
    }
  }

  return true;
}


void tiling(L3::Node * L3_tree, vector<L3::Tree *> L2_tiles, vector<L3::Tree *> &tiles) {
  if (!L3_tree) return;

  for (auto tile : L2_tiles) {
    std::vector<L3::Tree *> rest_trees;// = new L3::Tree();
    L3::Tree * tile_copy = generate_L2_tile_by_type(tile->tile_type);
    if (L3_tree->item == "call" && match_call_node(L3_tree, tile_copy->root, rest_trees)) {
      tiles.push_back(tile_copy);
      return;
    }
    else if ((L3_tree->item == "load" || L3_tree->item == "store") && match_load_store_node(L3_tree, tile_copy->root, rest_trees)) {
      tiles.push_back(tile_copy);
      for (auto rest_tree : rest_trees) {
        tiling(rest_tree->root, L2_tiles, tiles);
      }
      return;
    }
    else if (match_node(L3_tree, tile_copy->root, rest_trees)) {
      tiles.push_back(tile_copy);
      for (auto rest_tree : rest_trees) {
        tiling(rest_tree->root, L2_tiles, tiles);
      }
      return;
    }
  }
}

std::string write_L2_gen_left_mem_plus_minus_tree(L3::Tree * t, L3::L2_Tile_Type tt) {
  std::string x, M, t1, inst;
  
  x = t->root->children[0]->children[0]->children[0]->item;
  M = t->root->children[0]->children[0]->children[1]->item;
  
  if (t->root->children[1]->children[0]->item == "load") {
    t1 = t->root->children[1]->children[1]->item;
  }
  else {
    t1 = t->root->children[1]->children[0]->item;
  }

  x = get_reference(x);
  // M = get_reference(M);
  t1 = get_reference(t1);

  inst = "";
  if (tt == L3::LEFT_MEM_PLUS) {
    inst += "((mem " + x + " " + M + ") += " + t1 + ")\n";
  }
  else {
    inst += "((mem " + x + " " + M + ") -= " + t1 + ")\n";
  }

  return inst;
}

std::string write_L2_gen_right_mem_plus_minus_tree(L3::Tree * t, L3::L2_Tile_Type tt) {
  std::string w, x, M, inst;
  
  w = t->root->children[0]->item;
  
  
  if (t->root->children[1]->children[1]->item == "load") {
    x = t->root->children[1]->children[1]->children[0]->children[0]->item;
    M = t->root->children[1]->children[1]->children[0]->children[1]->item;
  }
  else {
    x = t->root->children[1]->children[0]->children[0]->children[0]->item;
    M = t->root->children[1]->children[0]->children[0]->children[1]->item;
  }

  if (isStrDigit(x)) {
    swapItem(x,M);
  }

  // x = get_reference(x);
  // M = get_reference(M);
  w = get_reference(w);
  // not_losing_reference.insert(w);

  x = get_reference(x);

  inst = "";
  if (tt == L3::LEFT_MEM_PLUS) {
    inst += "(" + w + " += (mem " + x + " " + M + "))\n";
  }
  else {
    inst += "(" + w + " -= (mem " + x + " " + M + "))\n";
  }

  return inst;
}

std::string write_L2_gen_left_mem_assign_tree(L3::Tree * t) {
  std::string var, x, M, inst;

  var = t->root->val;

  if (t->root->children[0]->item != "+") {
    x = t->root->children[0]->item;
    M = "0";
  }
  else {
    x = t->root->children[0]->children[0]->item;
    M = t->root->children[0]->children[1]->item;
  }

  if (isStrDigit(x)) {
    swapItem(x,M);
  }

  x = get_reference(x);
  // M = get_reference(M);
  var = get_reference(var);

  if (var == "dim") {
    cout << "wtfwtfwtfwtfwtfwtfwtfwtf" << endl;
  }

  inst = "";
  inst += "((mem " + x + " " + M + ") <- " + var + ")\n";

  return inst;
}

std::string write_L2_gen_right_mem_assign_tree(L3::Tree * t) {
  std::string var, x, M, op_var,inst;

  var = t->root->val;
  if (t->root->children[0]->item != "+") {
    x = t->root->children[0]->item;
    M = "0";
  }
  else {
    x = t->root->children[0]->children[0]->item;
    M = t->root->children[0]->children[1]->item;
    op_var = t->root->children[0]->val;
    // cout << "HHHHHHHHHHHHHHHHH" << endl;
    // t->printTree();
  }

  if (isStrDigit(x)) {
    swapItem(x,M);
  }

  not_losing_reference.insert(var);
  not_losing_reference.insert(op_var);
  x = get_reference(x);
  // M = get_reference(M);

  inst = "";
  if (!isStrDigit(M)) {

    inst += "(" + op_var + " <- " + x + ")\n";
    inst += "(" + op_var + " += " + M + ")\n"; 
    inst += "(" + var + " <- (mem " + op_var + " 0))\n";
  }
  else {
    inst += "(" + var + " <- (mem " + x + " " + M + "))\n";
  }

  return inst;
}

std::string write_L2_gen_lea_tree(L3::Tree * t) {
  std::string v1, v2, v3, e, inst;

  v1 = t->root->children[0]->item;

  if (t->root->children[1]->children[1]->item == "*") {
    v2 = t->root->children[1]->children[0]->item;
    v3 = t->root->children[1]->children[1]->children[0]->item;
    e = t->root->children[1]->children[1]->children[1]->item;
  }
  else {
    v2 = t->root->children[1]->children[1]->item;
    v3 = t->root->children[1]->children[0]->children[0]->item;
    e = t->root->children[1]->children[0]->children[1]->item;
  }

  if (e != "0" && e != "2" && e != "4" && e != "8") {
    swapItem(v3,e);
  }

  not_losing_reference.insert(v1);
  v2 = get_reference(v2);
  v3 = get_reference(v3);

  inst = "";
  inst += "(" + v1 + " @ " + v2 + " " + v3 + " " + e + ")\n";

  return inst;
}

std::string write_L2_gen_inc_tree(L3::Tree * t) {
  std::string w, tmp, inst, var_str;

  w = t->root->children[0]->item;
  
  tmp = t->root->children[1]->children[0]->item;
  var_str = t->root->children[1]->children[0]->val;
  if (tmp == "1") {
    tmp = t->root->children[1]->children[1]->item;
    var_str = t->root->children[1]->children[1]->val;
  }

  inst = "";
  if(isStrDigit(tmp) || (w != tmp && w != var_str)) {
    inst += "(" + w + " <- " + tmp + ")\n";
  }
  
  inst += "(" + w + "++)\n";

  not_losing_reference.insert(w);
  // not_losing_reference.insert(tmp);

  return inst;
}

std::string write_L2_gen_dec_tree(L3::Tree * t) {
  std::string w, tmp, inst, var_str;

  w = t->root->children[0]->item;
  
  tmp = t->root->children[1]->children[0]->item;
  var_str = t->root->children[1]->children[0]->val;
  if (tmp == "1") {
    tmp = t->root->children[1]->children[1]->item;
    var_str = t->root->children[1]->children[1]->val;
  }

  inst = "";
  if(isStrDigit(tmp) || (w != tmp && w != var_str)) {
    inst += "(" + w + " <- " + tmp + ")\n";
  }
  
  inst += "(" + w + "--)\n";

  not_losing_reference.insert(w);
  // not_losing_reference.insert(tmp);

  return inst;
}

std::string write_L2_gen_aop_sop_cmp_asgn(L3::Tree * t, L3::Item_Type tt) {
  std::string var, op, t1, t2, tmp, inst;

  var = t->root->children[0]->item;
  op = t->root->children[1]->item;
  t1 = t->root->children[1]->children[0]->item;
  t2 = t->root->children[1]->children[1]->item;

  if (isop(t1)) {
    t1 = t->root->children[1]->children[0]->val;
  }
  if (isop(t2)) {
    t2 = t->root->children[1]->children[1]->val;
  }

  // var = get_reference(var);
  

  inst = "";

  if (tt == L3::CMP) {
    // cout << "omgomgomgomgomgomg" << endl;
    // t->printTree();
    not_losing_reference.insert(var);
    t1 = get_reference(t1);
    t2 = get_reference(t2);
    inst += "(" + var + " <- " + t1 + " " + op + " " + t2 + ")\n";
  }
  else {
    if (t1 == var) {
      // not_losing_reference.insert(var);
      var = get_reference(var);
      t2 = get_reference(t2);
      not_losing_reference.insert(var);
      inst += "(" + var + " " + op + "= " + t2 + ")\n";
    }
    else if (t2 == var) {
      var = get_reference(var);
      t1 = get_reference(t1);
      not_losing_reference.insert(var);
      inst += "(" + var + " " + op + "= " + t1 + ")\n";
    }
    else {
        t1 = get_reference(t1);
        t2 = get_reference(t2);
        not_losing_reference.insert(var);
        inst += "(" + var + " <- " + t1 + ")\n";
        inst += "(" + var + " " + op + "= " + t2 + ")\n";
    }
  }

  return inst;

}

std::string write_L2_gen_aop_sop_cmp(L3::Tree * t, L3::Item_Type tt) {
  std::string op, t1, t2, val;
  std::string inst = "";

  op = t->root->item;
  t1 = t->root->children[0]->item;
  t2 = t->root->children[1]->item;
  val = t->root->val;

  if (isop(t1)) {
    t1 = t->root->children[0]->val;
  }
  if (isop(t2)) {
    t2 = t->root->children[1]->val;
  }

 
  // val = get_reference(val);

  if (op == "<" || op == "=" || op == "<=") {
    not_losing_reference.insert(val);
    t1 = get_reference(t1);
    t2 = get_reference(t2);
    inst += "(" + val + " <- " + t1 + " " + op + " " + t2 + ")\n";
  }
  else {
    if (val != t1) {
      t1 = get_reference(t1);
      not_losing_reference.insert(val);
      inst += "(" + val + " <- " + t1 + ")\n";
    }
    t2 = get_reference(t2);
    inst += "(" + val + " " + op + "= " + t2 + ")\n";
  }

  return inst;
}

std::string write_L2_gen_simple_assign_tree(L3::Tree * t) {
  std::string inst = "";
  std::string left = t->root->children[0]->item;
  std::string right = t->root->children[1]->item;
  if (isop(left)) {
    left = t->root->children[0]->val;
  }
  if (isop(right)) {
    right = t->root->children[1]->val;
  }

  if (left != right) {
    not_losing_reference.insert(left);
    right = get_reference(right);
    inst += "(" + left + " <- " + right + ")\n";
  }

  return inst;
}

std::string write_L2_gen_cjump_tree(L3::Tree * t) {
  std::string var, l1, l2, inst;
  var = t->root->children[0]->item;
  l1 = t->root->children[1]->item;
  l2 = t->root->children[2]->item;

  var = get_reference(var);

  inst = "";
  inst += "(cjump " + var + " = 1 " + l1 + " " + l2 + ")\n";
  return inst;
}

std::string write_L2_gen_label_tree(L3::Tree * t) {
  return t->root->item + "\n";
}

std::string write_L2_gen_goto_tree(L3::Tree * t) {
  std::string l, inst;
  l = t->root->children[0]->item;
  inst = "";
  inst += "(goto " + l + ")\n";
  return inst;
}

std::string write_L2_gen_return_tree(L3::Tree * t) {
  return "(return)\n";
}

std::string write_L2_gen_return_var_tree(L3::Tree * t) {
  std::string l, inst;
  l = t->root->children[0]->item;
  l = get_reference(l);
  inst = "";
  inst += "(rax <- " + l + ")\n";
  inst += "(return)\n";
  return inst;
}

std::string write_L2_gen_call_tree(L3::Tree * t) {
  std::string inst = "";
  std::vector<std::string> args_vec;
  std::string callee = t->root->children[0]->item;
  for (int i = 1; i < t->root->children.size(); i++) {
    L3::Node * ch = t->root->children[i];
    // if (ch->ittp >= L3::ASGN) {
    //   args_vec.push_back(ch->val);
      // t->root->children[i]->printNode();
      // cout << endl;
    // }
    // else {
      args_vec.push_back(t->root->children[i]->item);
    // }
  }


  for (int i = 0; i < args_vec.size() && i < 6; i++) {
    args_vec[i] = get_reference(args_vec[i]);
    inst += "(" + args[i] + " <- " + args_vec[i] + ")\n";
  }
  
  if (function_invokeNum.find(callee) == function_invokeNum.end()) {
    function_invokeNum[callee] = 1;
  }
  else {
    function_invokeNum[callee]++;
  }

  if (callee != "print" && callee != "allocate" && callee != "array-error") {
    if (callee.at(0) != ':') {
      inst += "((mem rsp -8) <- :" +
        callee + "_ret" + std::to_string(function_invokeNum[callee]) + ")\n";
    }
    else {
      inst += "((mem rsp -8) <- " + 
        callee + "_ret" + std::to_string(function_invokeNum[callee]) + ")\n";
    }
  }

  if (args_vec.size() > 6) {
    int offset = 16;
    for (int i = 6; i < args_vec.size(); i++) {
      offset += (i-6)*8;
      inst += "((mem rsp -" + std::to_string(offset) + ") <- " + args_vec[i] + ")\n";
    }
  }
  inst += "(call " + callee + " " + std::to_string(args_vec.size()) + ")\n";
  
  if (callee != "print" && callee != "allocate" && callee != "array-error") {
    if (callee.at(0) != ':') {
      inst += std::string(":") + callee + "_ret" + std::to_string(function_invokeNum[callee]) + "\n";
    }
    else {
      inst += callee + "_ret" + std::to_string(function_invokeNum[callee]) + "\n";
    }
  }

  return inst;
}

std::string write_L2_gen_call_assign_tree(L3::Tree * t) {
  std::string var, inst;

  var = t->root->children[0]->item;

  inst = "";
  inst += "(" + var + " <- " + "rax)\n";

  not_losing_reference.insert(var);

  return inst;
}

// void write_tile(L3::Tree * t) {
//   switch (t->tile_type) {
//     case L3::LEFT_MEM_PLUS :
//     case L3::LEFT_MEM_MINUS :
//       s += write_L2_gen_left_mem_plus_minus_tree(t,t->tile_type);
//       break;
//     case L3::RIGHT_MEM_PLUS :
//     case L3::RIGHT_MEM_MINUS :
//       s +=  write_L2_gen_right_mem_plus_minus_tree(t,t->tile_type);
//       break;
//     case L3::LEFT_MEM_ASSIGN :
//       s +=  write_L2_gen_left_mem_assign_tree(t);
//       break;
//     case L3::RIGHT_MEM_ASSIGN :
//       s +=  write_L2_gen_right_mem_assign_tree(t);
//       break;
//     case L3::LEA :
//       s +=  write_L2_gen_lea_tree(t);
//       break;
//     case L3::INC :
//       s +=  write_L2_gen_inc_tree(t);
//       break;
//     case L3::DEC :
//       s +=  write_L2_gen_dec_tree(t);
//       break;
//     case L3::AOP_SOP_CMP_ASGN_PLUS :
//       s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::PLUS);
//       break;
//     case L3::AOP_SOP_CMP_ASGN_MINUS :
//       s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::MINUS);
//       break;
//     case L3::AOP_SOP_CMP_ASGN_MUL :
//       s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::MUL);
//       break;
//     case L3::AOP_SOP_CMP_ASGN_AD :
//       s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::AD);
//       break;
//     case L3::AOP_SOP_CMP_ASGN_CMP :
//       s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::CMP);
//       break;
//     case L3::AOP_SOP_CMP_ASGN_SHIFT :
//       s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::SHIFT);
//       break;
//     case L3::AOP_SOP_CMP_PLUS :
//       s +=  write_L2_gen_aop_sop_cmp(t,L3::PLUS);
//       break;
//     case L3::AOP_SOP_CMP_MINUS :
//       s +=  write_L2_gen_aop_sop_cmp(t,L3::MINUS);
//       break;
//     case L3::AOP_SOP_CMP_MUL :
//       s +=  write_L2_gen_aop_sop_cmp(t,L3::MUL);
//       break;
//     case L3::AOP_SOP_CMP_AD :
//       s +=  write_L2_gen_aop_sop_cmp(t,L3::AD);
//       break;
//     // case L3::AOP_SOP_CMP_CMP :
//     //   s +=  write_L2_gen_aop_sop_cmp(t,L3::CMP);
//     case L3::AOP_SOP_CMP_SHIFT :
//       s +=  write_L2_gen_aop_sop_cmp(t,L3::SHIFT);
//       break;
//     case L3::SIMPLE_ASSIGN :
//       s +=  write_L2_gen_simple_assign_tree(t);
//       break;
//     case L3::CJUMP :
//       s +=  write_L2_gen_cjump_tree(t);
//       break;
//     case L3::TILE_LABEL :
//       s +=  write_L2_gen_label_tree(t);
//       break;
//     case L3::GOTO :
//       s +=  write_L2_gen_goto_tree(t);
//       break;
//     case L3::TILE_RETURN :
//       s +=  write_L2_gen_return_tree(t);
//       break;
//     case L3::TILE_VAR_RETURN:
//       s +=  write_L2_gen_return_var_tree(t);
//       break;
//     case L3::TILE_CALL:
//       s +=  write_L2_gen_call_tree(t);
//       break;
//     case L3::TILE_CALL_ASGN:
//       s += write_L2_gen_call_assign_tree(t);
//       break;
//     default:
//       cout << "Unknown tile type." << t->tile_type << endl;
//   }
// }

std::string write_tiles(std::vector<L3::Tree *> matched_tiles) {
  std::string s = "";

  for (auto t : matched_tiles) {
    // t->printTree();
    // cout << t->tile_type << endl;
    switch (t->tile_type) {
      case L3::LEFT_MEM_PLUS :
      case L3::LEFT_MEM_MINUS :
        s += write_L2_gen_left_mem_plus_minus_tree(t,t->tile_type);
        break;
      case L3::RIGHT_MEM_PLUS :
      case L3::RIGHT_MEM_MINUS :
        s +=  write_L2_gen_right_mem_plus_minus_tree(t,t->tile_type);
        break;
      case L3::LEFT_MEM_ASSIGN :
        s +=  write_L2_gen_left_mem_assign_tree(t);
        break;
      case L3::RIGHT_MEM_ASSIGN :
        s +=  write_L2_gen_right_mem_assign_tree(t);
        break;
      case L3::LEA :
        s +=  write_L2_gen_lea_tree(t);
        break;
      case L3::INC :
        s +=  write_L2_gen_inc_tree(t);
        break;
      case L3::DEC :
        s +=  write_L2_gen_dec_tree(t);
        break;
      case L3::AOP_SOP_CMP_ASGN_PLUS :
        s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::PLUS);
        break;
      case L3::AOP_SOP_CMP_ASGN_MINUS :
        s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::MINUS);
        break;
      case L3::AOP_SOP_CMP_ASGN_MUL :
        s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::MUL);
        break;
      case L3::AOP_SOP_CMP_ASGN_AD :
        s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::AD);
        break;
      case L3::AOP_SOP_CMP_ASGN_CMP :
        s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::CMP);
        break;
      case L3::AOP_SOP_CMP_ASGN_SHIFT :
        s +=  write_L2_gen_aop_sop_cmp_asgn(t,L3::SHIFT);
        break;
      case L3::AOP_SOP_CMP_PLUS :
        s +=  write_L2_gen_aop_sop_cmp(t,L3::PLUS);
        break;
      case L3::AOP_SOP_CMP_MINUS :
        s +=  write_L2_gen_aop_sop_cmp(t,L3::MINUS);
        break;
      case L3::AOP_SOP_CMP_MUL :
        s +=  write_L2_gen_aop_sop_cmp(t,L3::MUL);
        break;
      case L3::AOP_SOP_CMP_AD :
        s +=  write_L2_gen_aop_sop_cmp(t,L3::AD);
        break;
      case L3::AOP_SOP_CMP_CMP :
        // cout << "aaaaaaaaaaaaaa" << endl;
        // t->printTree();
        // break;
        s +=  write_L2_gen_aop_sop_cmp(t,L3::CMP);
        break;
      case L3::AOP_SOP_CMP_SHIFT :
        s +=  write_L2_gen_aop_sop_cmp(t,L3::SHIFT);
        break;
      case L3::SIMPLE_ASSIGN :
        s +=  write_L2_gen_simple_assign_tree(t);
        break;
      case L3::CJUMP :
        s +=  write_L2_gen_cjump_tree(t);
        break;
      case L3::TILE_LABEL :
        s +=  write_L2_gen_label_tree(t);
        break;
      case L3::GOTO :
        s +=  write_L2_gen_goto_tree(t);
        break;
      case L3::TILE_RETURN :
        s +=  write_L2_gen_return_tree(t);
        break;
      case L3::TILE_VAR_RETURN:
        s +=  write_L2_gen_return_var_tree(t);
        break;
      case L3::TILE_CALL:
        s +=  write_L2_gen_call_tree(t);
        break;
      case L3::TILE_CALL_ASGN:
        s += write_L2_gen_call_assign_tree(t);
        break;
      default:
        cout << "Unknown tile type." << t->tile_type << endl;
    }
  }
  // cout << s << endl;
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

  L3::Program p = L3::L3_parse_file(argv[optind]);
  p.label_suffix = 0;

  add_suffix_to_label(p);

  std::string program_str = "";
  program_str += "(:main\n";

  for (auto f : p.functions) {

    reference_map.clear();

    program_str += "(" + f->name + " " + std::to_string(f->args.size()) + " 0\n";
    for (int i = 0; i < f->args.size() && i < 6; i++) {
      program_str += "(" + f->args[i] + " <- " + args[i] + ")\n";
    }
    
    for (int i = 6; i < f->args.size(); i++) {
      int offset = (f->args.size() - i) * 8;
      program_str += "(" + f->args[i] + " <- (stack-arg " + std::to_string(offset) + "))\n";
    }

    vector<L3::Tree *> forest = create_forest(f);

    vector<L3::Tree *> merged_forest = merge_forest(forest);

    std::vector<L3::Tree *> tiles =  generate_L2_tiles();

    std::vector<L3::Tree *> matched_tiles;

    cout << "printing new forest:" << endl;
    for (auto t : merged_forest) {
      if (t)
        t->printTree();
    }
    cout << endl;

    std::vector<L3::Tree *> current_matched_tiles;

    for (auto tr : merged_forest) {
      tiling(tr->root, tiles, current_matched_tiles);
      
      while (!current_matched_tiles.empty()) {
        matched_tiles.push_back(current_matched_tiles.back());
        current_matched_tiles.pop_back();
      }
    }

    program_str += write_tiles(matched_tiles);
    program_str += ")\n";

    cout << "reference_map: " << endl;
    for (auto pr : reference_map) {
      cout << pr.first << " " << pr.second << endl;
    }

    cout << "not_losing_reference: " << endl;
    for (auto x : not_losing_reference) {
      cout << x << " ";
    }
    cout << endl;

  }
  program_str += ")\n";


  ofstream out;
  out.open("prog.L2", ios::out);
  out << program_str;
  out.close();

  
  return 0;
}
