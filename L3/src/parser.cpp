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

#include <L3.h>
#include <pegtl.hh>
#include <pegtl/analyze.hh>
#include <pegtl/contrib/raw_string.hh>

using namespace pegtl;
using namespace std;

namespace L3 {

  /* 
   * Grammar rules from now on.
   */

  struct var :
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

  struct label :
    pegtl::seq<
      pegtl::one<':'>,
      var
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

  struct u :
    pegtl::sor<
      var,
      label
    > {};

  struct callee : 
    pegtl::sor<
      pegtl_string_t("print"),
      pegtl_string_t("allocate"),
      pegtl_string_t("array-error"),
      u
    > {};

  struct opd :
    pegtl::sor<
      callee,
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

  struct entry_point_label : pegtl_string_t(":main") {};

  struct function_name : opd {};

  struct function_args : vars {};

  struct br_label : label {};

  struct br_var : var {};

  struct return_t : t {};

  struct op:
    pegtl::sor<
      pegtl_string_t("<<"),
      pegtl_string_t(">>"),
      pegtl::one< '+' >,
      pegtl::one< '-' >,
      pegtl::one< '*' >,
      pegtl::one< '&' >
    > {};

  struct cmp :
    pegtl::sor<
      pegtl_string_t("<="),
      pegtl_string_t(">="),
      pegtl::one< '<' >,
      pegtl::one< '>' >,
      pegtl::one< '=' >
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
      opd,
      seps,
      op,
      seps,
      opd,
      seps
    > {};


  struct cmp_assign_instruction :
    pegtl::seq<
      seps,
      opd,
      seps,
      pegtl_string_t("<-"),
      seps,
      opd,
      seps,
      cmp,
      seps,
      opd,
      seps
    > {};


  struct load_assign_instruction :
    pegtl::seq<
      seps,
      opd,
      seps,
      pegtl_string_t("<-"),
      seps,
      pegtl_string_t("load"),
      seps,
      opd,
      seps
    > {};

  struct store_assign_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("store"),
      seps,
      opd,
      seps,
      pegtl_string_t("<-"),
      seps,
      opd,
      seps
    > {};

  struct br1_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("br"),
      seps,
      br_label,
      seps
    > {};

  struct label_instruction :
    pegtl::seq<
      seps,
      opd,
      seps
    > {};

  struct br2_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("br"),
      seps,
      br_var,
      seps,
      br_label,
      seps,
      br_label,
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

  struct call_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("call "),
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
      pegtl_string_t("<-"),
      seps,
      pegtl_string_t("call "),
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

  struct L3_instruction_rule:
    pegtl::sor<
      call_instruction,
      call_assign_instruction,
      var_return_instruction,
      return_instruction,
      op_assign_instruction,
      cmp_assign_instruction,
      load_assign_instruction,
      store_assign_instruction,
      simple_assign_instruction,
      br2_instruction,
      br1_instruction,
      label_instruction
    > {};

  struct L3_instructions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< L3_instruction_rule >
    > {};
 
  struct L3_function_rule:
    pegtl::seq<
      seps,
      pegtl_string_t("define"),
      seps,
      function_name,
      seps,
      pegtl_string_t("("),
      seps,
      function_args,
      seps,
      pegtl_string_t(")"),
      seps,
      pegtl_string_t("{"),
      seps,
      L3_instructions_rule,
      seps,
      pegtl_string_t("}"),
      seps
    > {};

  struct L3_functions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< L3_function_rule >
    > {};

  // struct entry_point_rule:
  //   pegtl::seq<
  //     seps,
  //     pegtl::one< '(' >,
  //     seps,
  //     entry_point_label,
  //     seps,
  //     L3_functions_rule,
  //     seps,
  //     pegtl::one< ')' >,
  //     seps
  //   > {};

  struct grammar : 
    pegtl::must< 
      L3_functions_rule
      // entry_point_rule
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

  // template<> struct action < entry_point_label > {
  //   static void apply( const pegtl::input & in, L3::Program & p){   
  //     if (p.entryPointLabel.empty()){
  //       p.entryPointLabel = in.string();

  //     }
  //   }
  // };

  template<> struct action < function_name > {
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << "function name: ";
      // cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      L3::Function *newF = new L3::Function();
      newF->name = o;
      p.functions.push_back(newF);

      items.clear();
    }
  };

  template<> struct action < function_args > {
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << "function arg: ";
      // cout << in.string() << endl;

      L3::Function *currentF = p.functions.back();
      
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
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << "opd: ";
      // cout << in.string() << endl;
      L3::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      if (!o.empty() && o.at(0) == ':') {
        currentF->labels.insert(o);
      }

      items.push_back(o);
    }
  };

  template<> struct action < br_label > {
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << "br_label: ";
      // cout << in.string() << endl;
      L3::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      currentF->labels.insert(o);

      items.push_back(o);
    }
  };

  template<> struct action < br_var > {
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << "br_var: ";
      // cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      items.push_back(o);
    }
  };

  template<> struct action < op > {
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << "op: ";
      // cout << in.string() << endl;

      items.push_back(in.string());
    }
  };

  template<> struct action < cmp > {
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << "cmp: ";
      // cout << in.string() << endl;

      items.push_back(in.string());
    }
  };


  template<> struct action < simple_assign_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << in.string() << ": ";
      L3::Function *currentF = p.functions.back();
      
      L3::Simple_Assign_Instruction *newI = new L3::Simple_Assign_Instruction();
      // newI->instr_type = L3::SIMPLE_ASSIGN;
      
      newI->assign_right = items.back();
      items.pop_back();
      // cout << newI->assign_right << " ";

      newI->assign_left = items.back();
      items.pop_back();
      // cout << newI->assign_left << endl;

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < op_assign_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Op_Assign_Instruction *newI = new L3::Op_Assign_Instruction();
      // newI->instr_type = L3::OP_ASSIGN;
      
      newI->op_right = items.back();
      items.pop_back();

      newI->op = items.back();
      items.pop_back();

      newI->op_left = items.back();
      items.pop_back();
      
      newI->assign_left = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < cmp_assign_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Cmp_Assign_Instruction *newI = new L3::Cmp_Assign_Instruction();
      // newI->instr_type = L3::CMP_ASSIGN;
      
      newI->cmp_right = items.back();
      items.pop_back();

      newI->cmp = items.back();
      items.pop_back();

      newI->cmp_left = items.back();
      items.pop_back();
      
      newI->assign_left = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < load_assign_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Load_Assign_Instruction *newI = new L3::Load_Assign_Instruction();
      // newI->instr_type = L3::LOAD_ASSIGN;
      
      newI->var = items.back();
      items.pop_back();

      newI->assign_left = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < store_assign_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Store_Assign_Instruction *newI = new L3::Store_Assign_Instruction();
      // newI->instr_type = L3::STORE_ASSIGN;
      
      newI->assign_right = items.back();
      items.pop_back();

      newI->var = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < br1_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Br1_Instruction *newI = new L3::Br1_Instruction();
      // newI->instr_type = L3::BR1;
      
      newI->label = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < br2_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Br2_Instruction *newI = new L3::Br2_Instruction();
      // newI->instr_type = L3::BR2;
      
      newI->label2 = items.back();
      items.pop_back();

      newI->label1 = items.back();
      items.pop_back();

      newI->var = items.back();
      items.pop_back();
      
      currentF->instructions.push_back(newI);

      items.clear();
    }
  };
  
  template<> struct action < label_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Label_Instruction *newI = new L3::Label_Instruction();
      // newI->instr_type = L3::LABEL;
      
      newI->label = items.back();
      items.pop_back();
      
      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < return_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Return_Instruction *newI = new L3::Return_Instruction();
      // newI->instr_type = L3::RETURN;
      
      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < var_return_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      // cout << p.functions.size() << endl;
      // cout << currentF->name << endl;
      
      L3::Var_Return_Instruction *newI = new L3::Var_Return_Instruction();
      // newI->instr_type = L3::VAR_RETURN;
      
      newI->var = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < call_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Call_Instruction *newI = new L3::Call_Instruction();
      // newI->instr_type = L3::CALL;
      
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

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < call_assign_instruction > {
    static void apply( const pegtl::input & in, L3::Program & p){
      L3::Function *currentF = p.functions.back();
      
      L3::Call_Assign_Instruction *newI = new L3::Call_Assign_Instruction();
      // newI->instr_type = L3::CALL_ASSIGN;
      
      stack<std::string> arg_stack;
      while (!items.empty()) {
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

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < return_t > {
    static void apply( const pegtl::input & in, L3::Program & p){
      // cout << "return_t: ";
      // cout << in.string() << endl;

      L3::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      if (!o.empty() && o.at(0) == ':') {
        currentF->labels.insert(o);
      }

      items.push_back(o);
    }
  };
  // template<> struct action < L3_instruction_rule > {
  //   static void apply( const pegtl::input & in, L3::Program & p){
  //     cout << in.string() << endl;
  //   }
  // };

  // template<> struct action < L3_function_rule > {
  //   static void apply( const pegtl::input & in, L3::Program & p){
  //     cout << in.string() << endl;
  //   }
  // };
  

  Program L3_parse_file (char *fileName){

    /* 
     * Check the grammar for some possible issues.
     */
    pegtl::analyze< L3::grammar >();

    /*
     * Parse.
     */   
    L3::Program p;
    pegtl::file_parser(fileName).parse< L3::grammar, L3::action >(p);

    return p;
  }
}