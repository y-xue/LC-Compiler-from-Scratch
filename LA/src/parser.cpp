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

#include <LA.h>
#include <pegtl.hh>
#include <pegtl/analyze.hh>
#include <pegtl/contrib/raw_string.hh>

using namespace pegtl;
using namespace std;

namespace LA {

  /* 
   * Grammar rules from now on.
   */

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

  struct type_instruction :
    pegtl::seq<
      seps,
      type,
      seps,
      opd,
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

  struct label_instruction :
    pegtl::seq<
      seps,
      label,
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

  struct LA_instruction_rule:
    pegtl::sor<
      type_instruction,
      var_return_instruction,
      return_instruction,
      br2_instruction,
      br1_instruction,
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

  struct LA_instructions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< LA_instruction_rule >
    > {};

  // struct te :
  //   pegtl::sor<
      
  //   > {};

  // struct bb_rule :
  //   pegtl::seq<
  //     seps,
  //     bb_label,
  //     seps,
  //     LA_instructions_rule,
  //     seps,
  //     te,
  //     seps
  //   > {};

  struct LA_function_rule:
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
      pegtl_string_t("{"),
      seps,
      LA_instructions_rule,
      seps,
      pegtl_string_t("}"),
      seps
    > {};

  struct LA_functions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< LA_function_rule >
    > {};

  struct grammar : 
    pegtl::must< 
      LA_functions_rule
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
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "return type: ";
      cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      LA::Function *newF = new LA::Function();
      newF->return_type = o;
      p.functions.push_back(newF);

      items.clear();
    }
  };

  template<> struct action < function_name > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "function name: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      currentF->name = o;

      items.clear();
    }
  };

  template<> struct action < function_args > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "function arg: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
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
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "opd: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();

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
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "assign_op: ";
      cout << in.string() << endl;
      items.push_back(in.string());
    }
  };


  template<> struct action < return_t > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "return_t: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();

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
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "type: ";
      cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      items.push_back(o);
    }
  };

  template<> struct action < br_label > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "br_label: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      currentF->labels.insert(o);

      items.push_back(o);
    }
  };

  template<> struct action < br_var > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "br_var: ";
      cout << in.string() << endl;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      items.push_back(o);
    }
  };

  template<> struct action < op > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "op: ";
      cout << in.string() << endl;

      items.push_back(in.string());
    }
  };

  template<> struct action < label_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "label_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      LA::Label_Instruction *newI = new LA::Label_Instruction();

      newI->i_type = LA::LBL;

      std::string o = in.string();
      o.erase(remove(o.begin(), o.end(), '\n'), o.end());
      o.erase(remove(o.begin(), o.end(), ' '), o.end());

      newI->label = o;
      
      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < type_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "type_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      std::string v = items.back();
      items.pop_back();
      std::string tp = items.back();
      items.pop_back();

      LA::Type_Instruction *newI = new LA::Type_Instruction(currentF,tp,v);
      newI->i_type = LA::OTHER;
      // newI->var = items.back();
      // items.pop_back();
      // newI->type = items.back();
      // items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };
  

  template<> struct action < simple_assign_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "simple_assign_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Simple_Assign_Instruction *newI = new LA::Simple_Assign_Instruction();
      newI->i_type = LA::OTHER;
   
      newI->assign_right = items.back();
      items.pop_back();

      newI->assign_left = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < op_assign_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "op_assign_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Op_Assign_Instruction *newI = new LA::Op_Assign_Instruction();
      newI->i_type = LA::OTHER;

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

  template<> struct action < right_array_assign_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "right_array_assign_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Right_Array_Assign_Instruction *newI = new LA::Right_Array_Assign_Instruction();
      newI->i_type = LA::OTHER;

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

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < left_array_assign_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "left_array_assign_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Left_Array_Assign_Instruction *newI = new LA::Left_Array_Assign_Instruction();
      newI->i_type = LA::OTHER;
      
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

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < length_assign_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "length_assign_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Length_Assign_Instruction *newI = new LA::Length_Assign_Instruction();
      newI->i_type = LA::OTHER;
      
      newI->t = items.back();
      items.pop_back();

      newI->var = items.back();
      items.pop_back();
      
      newI->assign_left = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };
  
  template<> struct action < new_array_assign_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "new_array_assign_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::New_Array_Assign_Instruction *newI = new LA::New_Array_Assign_Instruction();
      newI->i_type = LA::OTHER;

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

      currentF->instructions.push_back(newI);

      newI->set_array_size(currentF);

      items.clear();
    }
  };

  template<> struct action < new_tuple_assign_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "new_tuple_assign_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      std::string tt = items.back();
      items.pop_back();

      std::string al = items.back();
      items.pop_back();

      LA::New_Tuple_Assign_Instruction *newI = new LA::New_Tuple_Assign_Instruction(currentF,al,tt);
      newI->i_type = LA::OTHER;

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < call_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "call_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Call_Instruction *newI = new LA::Call_Instruction();
      newI->i_type = LA::OTHER;

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
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "call_assign_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Call_Assign_Instruction *newI = new LA::Call_Assign_Instruction();
      newI->i_type = LA::OTHER;

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

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < br1_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "br1_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      LA::Br1_Instruction *newI = new LA::Br1_Instruction();
      newI->i_type = LA::TE;

      newI->label = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < br2_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "br2_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Br2_Instruction *newI = new LA::Br2_Instruction();
      newI->i_type = LA::TE;

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

  template<> struct action < return_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "return_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Return_Instruction *newI = new LA::Return_Instruction();
      newI->i_type = LA::TE;

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  template<> struct action < var_return_instruction > {
    static void apply( const pegtl::input & in, LA::Program & p){
      cout << "var_return_instruction: ";
      cout << in.string() << endl;

      LA::Function *currentF = p.functions.back();
      
      
      LA::Var_Return_Instruction *newI = new LA::Var_Return_Instruction();
      newI->i_type = LA::TE;

      newI->var = items.back();
      items.pop_back();

      currentF->instructions.push_back(newI);

      items.clear();
    }
  };

  // template<> struct action < te > {
  //   static void apply( const pegtl::input & in, LA::Program & p){
  //     cout << "te: ";
  //     cout << in.string() << endl;
  //   }
  // };
  

  Program LA_parse_file (char *fileName){

    /* 
     * Check the grammar for some possible issues.
     */
    pegtl::analyze< LA::grammar >();

    /*
     * Parse.
     */   
    LA::Program p;
    pegtl::file_parser(fileName).parse< LA::grammar, LA::action >(p);

    return p;
  }
}