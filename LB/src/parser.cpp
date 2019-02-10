#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <iterator>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <stack>
#include <ctype.h>

#include <LB.h>
#include <pegtl.hh>
#include <pegtl/analyze.hh>
#include <pegtl/contrib/raw_string.hh>

using namespace pegtl;
using namespace std;

namespace LB {

  /* 
   * Grammar rules from now on.
   */

  std::stack<LB::Scope *> scope_stack;

  struct comment: 
    pegtl::disable< 
      pegtl::one< ';' >, 
      pegtl::until< pegtl::eolf > 
    > {};

  struct seps : 
    pegtl::star< 
      pegtl::sor< 
        pegtl::ascii::space, 
        comment 
      > 
    > {};

  struct name :
    pegtl::seq<
      pegtl::plus< 
        pegtl::sor<
          pegtl::alpha,
          pegtl::one< '_' >
        >
      >,
      pegtl::star<
        pegtl::sor<
          pegtl::alpha,
          pegtl::one< '_' >,
          pegtl::digit
        >
      >
    > {};

  struct var :
    pegtl::seq<
      pegtl::one< '%' >,
      name
    > {};

  struct label :
    pegtl::seq<
      pegtl::one<':'>,
      name
    > {};

  struct number :
    pegtl::seq<
      pegtl::opt<
        pegtl::sor<
          pegtl::one< '-' >,
          pegtl::one< '+' >
        >
      >,
      pegtl::plus< 
        pegtl::digit
      >
    >{};

  struct t :
    pegtl::sor<
      var,
      number
    > {};

  struct s :
    pegtl::sor<
      t,
      label
    > {};

  // struct u :
  //   pegtl::sor<
  //     var,
  //     label
  //   > {};

  struct callee : 
    pegtl::sor<
      var,
      name
    > {};

  struct opd :
    pegtl::sor<
      var,
      label,
      name,
      number
    > {};

  struct vars :
    pegtl::sor<
      pegtl::seq<
        seps,
        opd,
        seps,
        pegtl::star<
          pegtl::seq<
            seps,
            pegtl::one< ',' >,
            seps,
            opd,
            seps
          >
        >,
        seps
      >,
      seps
    > {};

  struct args :
    pegtl::sor<
      pegtl::seq<
        seps,
        opd,
        seps,
        pegtl::star<
          pegtl::seq<
            seps,
            pegtl::one< ',' >,
            seps,
            opd,
            seps
          >
        >,
        seps
      >,
      seps
    > {};

  struct type :
    pegtl::sor<
      pegtl::seq<
        pegtl_string_t("int64"),
        pegtl::star<
          pegtl_string_t("[]")
        >
      >,
      pegtl_string_t("tuple"),
      pegtl_string_t("code"),
      pegtl_string_t("void")
    > {};

  // struct T :
  //   pegtl::sor<
  //     type,
  //     pegtl_string_t("void")
  //   > {};

  struct return_t : t {};

  struct array_element :
    pegtl::seq<
      seps,
      opd,
      seps,
      pegtl::plus<
        pegtl::seq<
          seps,
          pegtl::one< '[' >,
          seps,
          opd,
          seps,
          pegtl::one< ']' >,
          seps
        >
      >,
      seps
    > {};

  struct op:
    pegtl::sor<
      pegtl_string_t("<<"),
      pegtl_string_t(">>"),
      pegtl::one< '+' >,
      pegtl::one< '-' >,
      pegtl::one< '*' >,
      pegtl::one< '&' >,
      pegtl_string_t("<="),
      pegtl_string_t(">="),
      pegtl::one< '<' >,
      pegtl::one< '>' >,
      pegtl::one< '=' >
    > {};

  struct cond:
    pegtl::seq<
      seps,
      opd,
      seps,
      op,
      seps,
      opd,
      seps
    > {};

  struct entry_point_label : pegtl_string_t(":main") {};

  struct return_type : type {};

  struct function_name : opd {};

  struct function_args :
    pegtl::seq<
      pegtl::star<
        pegtl::seq<
          pegtl::opt< pegtl::one< ',' > >,
          seps,
          type,
          seps,
          opd,
          seps
        >
      >
    > {};

  struct if_while_label : label {};

  // struct br_var : var {};

  struct type_instruction :
    pegtl::seq<
      seps,
      type,
      seps,
      vars,
      seps
    > {};

  struct simple_assign_instruction :
    pegtl::seq<
      seps,
      opd,
      seps,
      pegtl_string_t("<-"),
      seps,
      opd,
      seps
    > {};

  struct op_assign_instruction :
    pegtl::seq<
      seps,
      opd,
      seps,
      pegtl_string_t("<-"),
      seps,
      cond,
      seps
    > {};

  struct instruction_label : label {};

  struct label_instruction :
    pegtl::seq<
      seps,
      instruction_label,
      seps
    > {};

  struct if_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("if"),
      seps,
      pegtl_string_t("("),
      seps,
      cond,
      seps,
      pegtl_string_t(")"),
      seps,
      if_while_label,
      seps,
      if_while_label,
      seps
    > {};

  struct return_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("return"),
      seps
    > {};

  struct var_return_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("return"),
      seps,
      return_t,
      seps
    > {};

  struct while_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("while"),
      seps,
      pegtl_string_t("("),
      seps,
      cond,
      seps,
      pegtl_string_t(")"),
      seps,
      if_while_label,
      seps,
      if_while_label,
      seps
    > {};

  struct continue_instruction :
    pegtl_string_t("continue") {};

  struct break_instruction :
    pegtl_string_t("break") {};

  struct assign_op : pegtl_string_t("<-") {};

  struct right_array_assign_instruction :
    pegtl::seq<
      seps,
      opd,
      seps,
      assign_op,
      seps,
      array_element,
      seps
    > {};

  struct left_array_assign_instruction :
    pegtl::seq<
      seps,
      array_element,
      seps,
      assign_op,
      seps,
      opd,
      seps
    > {};

  struct length_assign_instruction :
     pegtl::seq<
      seps,
      opd,
      seps,
      pegtl_string_t("<-"),
      seps,
      pegtl_string_t("length"),
      seps,
      opd,
      seps,
      opd,
      seps
    > {};

  struct call_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("call"),
      seps,
      opd,
      seps,
      pegtl_string_t("("),
      seps,
      args,
      seps,
      pegtl_string_t(")"),
      seps
    > {};

  struct call_assign_instruction :
    pegtl::seq<
      seps,
      opd,
      seps,
      assign_op,
      seps,
      pegtl_string_t("call"),
      seps,
      opd,
      seps,
      pegtl_string_t("("),
      seps,
      args,
      seps,
      pegtl_string_t(")"),
      seps
    > {};

  struct new_array_assign_instruction :
    pegtl::seq<
      seps,
      opd,
      seps,
      assign_op,
      seps,
      pegtl_string_t("new"),
      seps,
      pegtl_string_t("Array"),
      seps,
      pegtl_string_t("("),
      seps,
      args,
      seps,
      pegtl_string_t(")"),
      seps
    > {};

  struct new_tuple_assign_instruction :
    pegtl::seq<
      seps,
      opd,
      seps,
      pegtl_string_t("<-"),
      seps,
      pegtl_string_t("new"),
      seps,
      pegtl_string_t("Tuple"),
      seps,
      pegtl_string_t("("),
      seps,
      opd,
      seps,
      pegtl_string_t(")"),
      seps
    > {};

  struct scope;

  struct LB_instruction_rule:
    pegtl::sor<
      scope,
      type_instruction,
      var_return_instruction,
      return_instruction,
      continue_instruction,
      break_instruction,
      if_instruction,
      while_instruction,
      call_instruction,
      label_instruction,
      left_array_assign_instruction,
      right_array_assign_instruction,
      new_array_assign_instruction,
      new_tuple_assign_instruction,
      call_assign_instruction,
      length_assign_instruction,
      op_assign_instruction,
      simple_assign_instruction
    > {};

  struct LB_instructions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< LB_instruction_rule >
    > {};

  struct scope_start : pegtl_string_t("{") {};

  struct scope_end : pegtl_string_t("}") {};

  struct scope :
    pegtl::seq<
      seps,
      scope_start,
      seps,
      LB_instructions_rule,
      seps,
      scope_end,
      seps
    > {};

  struct LB_function_rule:
    pegtl::seq<
      seps,
      return_type,
      seps,
      function_name,
      seps,
      pegtl_string_t("("),
      seps,
      function_args,
      seps,
      pegtl_string_t(")"),
      seps,
      scope,
      seps
    > {};

  struct LB_functions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< LB_function_rule >
    > {};

  struct grammar : 
    pegtl::must< 
      LB_functions_rule
    > {};

  /* 
   * Data structures required to parse
   */ 
  std::vector<std::string> items;

  /* 
   * Actions attached to grammar rules.
   */
  template< typename Rule >
  struct action : pegtl::nothing< Rule > {};


  template<> struct action < return_type > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "return type: ";
      cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      LB::Function *newF = new LB::Function();
      newF->return_type = o;

      p.functions.push_back(newF);

      items.clear();
    }
  };

  template<> struct action < function_name > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "function name: ";
      cout << in.string() << endl;

      LB::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      currentF->name = o;

      items.clear();
    }
  };

  template<> struct action < function_args > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "function arg: ";
      cout << in.string() << endl;

      LB::Function *currentF = p.functions.back();
      
      stack<std::string> arg_stack;
      while (!items.empty()) {
        arg_stack.push(items.back());
        items.pop_back();
      }

      while (!arg_stack.empty()) {
        currentF->args.push_back(arg_stack.top());
        arg_stack.pop();
      }

      items.clear();
    }
  };


  template<> struct action < opd > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "opd: ";
      cout << in.string() << endl;

      LB::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      if (!o.empty() && o.at(0) == ':') {
        currentF->labels.insert(o);
      }

      items.push_back(o);
    }
  };

  template<> struct action < assign_op > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "assign_op: ";
      cout << in.string() << endl;
      items.push_back(in.string());
    }
  };


  template<> struct action < return_t > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "return_t: ";
      cout << in.string() << endl;

      LB::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      if (!o.empty() && o.at(0) == ':') {
        currentF->labels.insert(o);
      }

      items.push_back(o);
    }
  };
  

  template<> struct action < type > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "type: ";
      cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      items.push_back(o);
    }
  };

  template<> struct action < if_while_label > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "if_while_label: ";
      cout << in.string() << endl;

      LB::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      currentF->labels.insert(o);

      items.push_back(o);
    }
  };

  template<> struct action < op > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "op: ";
      cout << in.string() << endl;

      items.push_back(in.string());
    }
  };

  template<> struct action < instruction_label > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "instruction_label: ";
      cout << in.string() << endl;

      items.push_back(in.string());
    }
  };

  template<> struct action < scope_start > {
    static void apply( const pegtl::input & in, LB::Program & p){
      
      LB::Scope * currentS;
      LB::Function * currentF = p.functions.back();

      LB::Scope * newS = new LB::Scope();
      newS->i_type = LB::SCOPE;

      if (scope_stack.empty()) {
        currentF->scope = newS;
      }
      else {
        currentS = scope_stack.top();
        currentS->instructions.push_back(newS);
      }

      scope_stack.push(newS);
    }
  };

  template<> struct action < scope_end > {
    static void apply( const pegtl::input & in, LB::Program & p){
      scope_stack.pop();
    }
  };

  template<> struct action < label_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "label_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();

      LB::Scope *currentS = scope_stack.top();

      LB::Label_Instruction *newI = new LB::Label_Instruction();
      newI->i_type = LB::LBL;

      std::string o = items.back();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      newI->label = o;
      
      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < type_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "type_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      stack<std::string> var_stack;
      while (!items.empty()) {
        var_stack.push(items.back());
        items.pop_back();
      }

      LB::Type_Instruction *newI = new LB::Type_Instruction();
      newI->i_type = LB::TYPE;

      newI->type = var_stack.top();
      var_stack.pop();

      while (!var_stack.empty()) {
        newI->vars.push_back(var_stack.top());
        var_stack.pop();
      }

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };
  

  template<> struct action < simple_assign_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "simple_assign_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Simple_Assign_Instruction *newI = new LB::Simple_Assign_Instruction();
      newI->i_type = LB::OTHER;
   
      newI->assign_right = items.back();
      items.pop_back();

      newI->assign_left = items.back();
      items.pop_back();

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < op_assign_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "op_assign_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Op_Assign_Instruction *newI = new LB::Op_Assign_Instruction();
      newI->i_type = LB::OTHER;

      newI->op_right = items.back();
      items.pop_back();

      newI->op = items.back();
      items.pop_back();

      newI->op_left = items.back();
      items.pop_back();
      
      newI->assign_left = items.back();
      items.pop_back();

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < if_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "if_instruction: ";
      LB::Scope *currentS = scope_stack.top();

      LB::If_Instruction *newI = new LB::If_Instruction();
      newI->i_type = LB::IF;

      newI->label2 = items.back();
      items.pop_back();

      newI->label1 = items.back();
      items.pop_back();

      newI->op_right = items.back();
      items.pop_back();

      newI->op = items.back();
      items.pop_back();

      newI->op_left = items.back();
      items.pop_back();

      currentS->instructions.push_back(newI);

      items.clear();

    }
  };

  template<> struct action < while_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "while_instruction: ";
      LB::Scope *currentS = scope_stack.top();

      LB::While_Instruction *newI = new LB::While_Instruction();
      newI->i_type = LB::WHILE;

      newI->label2 = items.back();
      items.pop_back();

      newI->label1 = items.back();
      items.pop_back();

      newI->op_right = items.back();
      items.pop_back();

      newI->op = items.back();
      items.pop_back();

      newI->op_left = items.back();
      items.pop_back();

      currentS->instructions.push_back(newI);

      items.clear();

    }
  };

  template<> struct action < continue_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "continue_instruction: ";
      LB::Scope *currentS = scope_stack.top();

      LB::Continue_Instruction *newI = new LB::Continue_Instruction();
      newI->i_type = LB::CONTINUE;

      currentS->instructions.push_back(newI);
      items.clear();

    }
  };

  template<> struct action < break_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "break_instruction: ";
      LB::Scope *currentS = scope_stack.top();

      LB::Break_Instruction *newI = new LB::Break_Instruction();
      newI->i_type = LB::BREAK;

      currentS->instructions.push_back(newI);
      items.clear();

    }
  };

  template<> struct action < right_array_assign_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "right_array_assign_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Right_Array_Assign_Instruction *newI = new LB::Right_Array_Assign_Instruction();
      newI->i_type = LB::OTHER;

      stack<std::string> ind_stack;
      while (!items.empty()) {
        if (items.back() == "<-") {
          items.pop_back();
          ind_stack.push(items.back());
          break;
        }
        ind_stack.push(items.back());
        items.pop_back();
      }

      newI->assign_left = ind_stack.top();
      ind_stack.pop();

      newI->array_name = ind_stack.top();
      ind_stack.pop();

      while (!ind_stack.empty()) {
        newI->indices.push_back(ind_stack.top());
        ind_stack.pop();
      }

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < left_array_assign_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "left_array_assign_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Left_Array_Assign_Instruction *newI = new LB::Left_Array_Assign_Instruction();
      newI->i_type = LB::OTHER;
      
      newI->assign_right = items.back();
      items.pop_back();

      stack<std::string> ind_stack;
      while (!items.empty()) {
        if (items.back() != "<-") {
          ind_stack.push(items.back());
        }
        items.pop_back();
      }

      newI->array_name = ind_stack.top();
      ind_stack.pop();

      while (!ind_stack.empty()) {
        newI->indices.push_back(ind_stack.top());
        ind_stack.pop();
      }

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < length_assign_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "length_assign_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Length_Assign_Instruction *newI = new LB::Length_Assign_Instruction();
      newI->i_type = LB::OTHER;
      
      newI->t = items.back();
      items.pop_back();

      newI->var = items.back();
      items.pop_back();
      
      newI->assign_left = items.back();
      items.pop_back();

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };
  
  template<> struct action < new_array_assign_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "new_array_assign_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::New_Array_Assign_Instruction *newI = new LB::New_Array_Assign_Instruction();
      newI->i_type = LB::OTHER;

      stack<std::string> arg_stack;
      while (!items.empty()) {
        if (items.back() == "<-") {
          items.pop_back();
          arg_stack.push(items.back());
          break;
        }
        arg_stack.push(items.back());
        items.pop_back();
      }

      newI->assign_left = arg_stack.top();
      arg_stack.pop();

      while (!arg_stack.empty()) {
        newI->args.push_back(arg_stack.top());
        arg_stack.pop();
      }

      currentS->instructions.push_back(newI);

      // newI->set_array_size(currentF);

      items.clear();
    }
  };

  template<> struct action < new_tuple_assign_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "new_tuple_assign_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();

      LB::New_Tuple_Assign_Instruction *newI = new LB::New_Tuple_Assign_Instruction();
      newI->i_type = LB::OTHER;

      newI->t = items.back();
      items.pop_back();

      newI->assign_left = items.back();
      items.pop_back();

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < call_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "call_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Call_Instruction *newI = new LB::Call_Instruction();
      newI->i_type = LB::OTHER;

      stack<std::string> arg_stack;
      while (!items.empty()) {
        arg_stack.push(items.back());
        items.pop_back();
      }

      std::string callee = arg_stack.top();
      arg_stack.pop();

      newI->callee = callee;

      while (!arg_stack.empty()) {
        newI->args.push_back(arg_stack.top());
        arg_stack.pop();
      }

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < call_assign_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "call_assign_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Call_Assign_Instruction *newI = new LB::Call_Assign_Instruction();
      newI->i_type = LB::OTHER;

      stack<std::string> arg_stack;
      while (!items.empty()) {
        if (items.back() == "<-") {
          items.pop_back();
          arg_stack.push(items.back());
          break;
        }
        arg_stack.push(items.back());
        items.pop_back();
      }

      newI->assign_left = arg_stack.top();
      arg_stack.pop();

      std::string callee = arg_stack.top();
      arg_stack.pop();

      newI->callee = callee;

      while (!arg_stack.empty()) {
        newI->args.push_back(arg_stack.top());
        arg_stack.pop();
      }

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < return_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "return_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Return_Instruction *newI = new LB::Return_Instruction();
      newI->i_type = LB::OTHER;

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < var_return_instruction > {
    static void apply( const pegtl::input & in, LB::Program & p){
      cout << "var_return_instruction: ";
      cout << in.string() << endl;

      // LB::Function *currentF = p.functions.back();
      LB::Scope *currentS = scope_stack.top();
      
      LB::Var_Return_Instruction *newI = new LB::Var_Return_Instruction();
      newI->i_type = LB::OTHER;

      newI->var = items.back();
      items.pop_back();

      currentS->instructions.push_back(newI);

      items.clear();
    }
  };

  Program LB_parse_file (char *fileName){

    /* 
     * Check the grammar for some possible issues.
     */
    pegtl::analyze< LB::grammar >();

    /*
     * Parse.
     */   
    LB::Program p;
    pegtl::file_parser(fileName).parse< LB::grammar, LB::action >(p);

    return p;
  }
}