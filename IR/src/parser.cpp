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

#include <IR.h>
#include <pegtl.hh>
#include <pegtl/analyze.hh>
#include <pegtl/contrib/raw_string.hh>

using namespace pegtl;
using namespace std;

namespace IR {

  /* 
   * Grammar rules from now on.
   */

  struct var :
    pegtl::seq<
      pegtl::one< '%' >,
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
      // pegtl_string_t("allocate"),
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

  struct type :
    pegtl::sor<
      pegtl::seq<
        pegtl_string_t("int64"),
        pegtl::star<
          pegtl_string_t("[]")
        >
      >,
      pegtl_string_t("tuple"),
      pegtl_string_t("code")
    > {};

  struct T :
    pegtl::sor<
      type,
      pegtl_string_t("void")
    > {};

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

  struct type_instruction :
    pegtl::seq<
      seps,
      type,
      seps,
      opd,
      seps
    > {};

  struct entry_point_label : pegtl_string_t(":main") {};

  struct return_type : T {};

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

  struct br_label : label {};

  struct br_var : var {};

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

  struct br1_instruction :
    pegtl::seq<
      seps,
      pegtl_string_t("br"),
      seps,
      br_label,
      seps
    > {};

  struct bb_label :
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

  struct IR_instruction_rule:
    pegtl::sor<
      left_array_assign_instruction,
      right_array_assign_instruction,
      new_array_assign_instruction,
      new_tuple_assign_instruction,
      type_instruction,
      call_instruction,
      call_assign_instruction,
      length_assign_instruction,
      op_assign_instruction,
      simple_assign_instruction
    > {};

  struct IR_instructions_rule:
    pegtl::seq<
      seps,
      pegtl::star< IR_instruction_rule >
    > {};

  struct te :
    pegtl::sor<
      var_return_instruction,
      return_instruction,
      br2_instruction,
      br1_instruction
    > {};

  struct bb_rule :
    pegtl::seq<
      seps,
      bb_label,
      seps,
      IR_instructions_rule,
      seps,
      te,
      seps
    > {};

  struct IR_function_rule:
    pegtl::seq<
      seps,
      pegtl_string_t("define"),
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
      pegtl_string_t("{"),
      seps,
      pegtl::plus<
        bb_rule
      >,
      seps,
      pegtl_string_t("}"),
      seps
    > {};

  struct IR_functions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< IR_function_rule >
    > {};

  struct grammar : 
    pegtl::must< 
      IR_functions_rule
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
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "return type: ";
      cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      IR::Function *newF = new IR::Function();
      newF->return_type = o;
      p.functions.push_back(newF);

      items.clear();
    }
  };

  template<> struct action < function_name > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "function name: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      currentF->name = o;

      items.clear();
    }
  };

  template<> struct action < function_args > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "function arg: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      
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
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "opd: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();

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
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "assign_op: ";
      cout << in.string() << endl;
      items.push_back(in.string());
    }
  };


  template<> struct action < return_t > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "return_t: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();

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
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "type: ";
      cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      items.push_back(o);
    }
  };

  template<> struct action < br_label > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "br_label: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      currentF->labels.insert(o);

      items.push_back(o);
    }
  };

  template<> struct action < br_var > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "br_var: ";
      cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      items.push_back(o);
    }
  };

  template<> struct action < op > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "op: ";
      cout << in.string() << endl;

      items.push_back(in.string());
    }
  };

  template<> struct action < bb_label > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "bb label: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *newB = new IR::BB();
      newB->label = in.string();
      
      currentF->bbs.push_back(newB);

      items.clear();
    }
  };

  template<> struct action < type_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "type_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Type_Instruction *newI = new IR::Type_Instruction();
      
      newI->var = items.back();
      items.pop_back();
      newI->type = items.back();
      items.pop_back();

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };
  

  template<> struct action < simple_assign_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "simple_assign_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Simple_Assign_Instruction *newI = new IR::Simple_Assign_Instruction();
   
      newI->assign_right = items.back();
      items.pop_back();

      newI->assign_left = items.back();
      items.pop_back();

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < op_assign_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "op_assign_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Op_Assign_Instruction *newI = new IR::Op_Assign_Instruction();

      newI->op_right = items.back();
      items.pop_back();

      newI->op = items.back();
      items.pop_back();

      newI->op_left = items.back();
      items.pop_back();
      
      newI->assign_left = items.back();
      items.pop_back();

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < right_array_assign_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "right_array_assign_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Right_Array_Assign_Instruction *newI = new IR::Right_Array_Assign_Instruction();

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

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < left_array_assign_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "left_array_assign_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Left_Array_Assign_Instruction *newI = new IR::Left_Array_Assign_Instruction();
      
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

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < length_assign_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "length_assign_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Length_Assign_Instruction *newI = new IR::Length_Assign_Instruction();
      
      newI->t = items.back();
      items.pop_back();

      newI->var = items.back();
      items.pop_back();
      
      newI->assign_left = items.back();
      items.pop_back();

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };
  
  template<> struct action < new_array_assign_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "new_array_assign_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::New_Array_Assign_Instruction *newI = new IR::New_Array_Assign_Instruction();
      
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

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < new_tuple_assign_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "new_tuple_assign_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::New_Tuple_Assign_Instruction *newI = new IR::New_Tuple_Assign_Instruction();
      
      newI->t = items.back();
      items.pop_back();

      newI->assign_left = items.back();
      items.pop_back();

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < call_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "call_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Call_Instruction *newI = new IR::Call_Instruction();
      
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

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < call_assign_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "call_assign_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Call_Assign_Instruction *newI = new IR::Call_Assign_Instruction();
      
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

      currentB->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < br1_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "br1_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Br1_Instruction *newI = new IR::Br1_Instruction();
      
      newI->label = items.back();
      items.pop_back();

      currentB->terminator = newI;

      items.clear();
    }
  };

  template<> struct action < br2_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "br2_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Br2_Instruction *newI = new IR::Br2_Instruction();
      
      newI->label2 = items.back();
      items.pop_back();

      newI->label1 = items.back();
      items.pop_back();

      newI->var = items.back();
      items.pop_back();
      
      currentB->terminator = newI;

      items.clear();
    }
  };

  template<> struct action < return_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "return_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Return_Instruction *newI = new IR::Return_Instruction();
      
      currentB->terminator = newI;

      items.clear();
    }
  };

  template<> struct action < var_return_instruction > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "var_return_instruction: ";
      cout << in.string() << endl;

      IR::Function *currentF = p.functions.back();
      IR::BB *currentB = currentF->bbs.back();
      
      IR::Var_Return_Instruction *newI = new IR::Var_Return_Instruction();
      
      newI->var = items.back();
      items.pop_back();

      currentB->terminator = newI;

      items.clear();
    }
  };

  template<> struct action < te > {
    static void apply( const pegtl::input & in, IR::Program & p){
      cout << "te: ";
      cout << in.string() << endl;
    }
  };
  

  Program IR_parse_file (char *fileName){

    /* 
     * Check the grammar for some possible issues.
     */
    pegtl::analyze< IR::grammar >();

    /*
     * Parse.
     */   
    IR::Program p;
    pegtl::file_parser(fileName).parse< IR::grammar, IR::action >(p);

    return p;
  }
}