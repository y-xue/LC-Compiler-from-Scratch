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

#include <L1.h>
#include <pegtl.hh>
#include <pegtl/analyze.hh>
#include <pegtl/contrib/raw_string.hh>

using namespace pegtl;
using namespace std;

namespace L1 {

  /* 
   * Grammar rules from now on.
   */

  struct label:
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

  struct entry_point_label : label {};

  struct function_name:
    label {};

  struct number:
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

  struct argument_number:
    number {};

  struct local_number:
    number {} ;

  struct comment: 
    pegtl::disable< 
      pegtl::one< ';' >, 
      pegtl::until< pegtl::eolf > 
    > {};

  struct seps: 
    pegtl::star< 
      pegtl::sor< 
        pegtl::ascii::space, 
        comment 
      > 
    > {};

  struct a :
    pegtl::sor<
      pegtl::string< 'r', 'd', 'i' >,
      pegtl::string< 'r', 's', 'i' >,
      pegtl::string< 'r', 'd', 'x' >,
      pegtl::string< 'r', 'c', 'x' >,
      pegtl::string< 'r', '8' >,
      pegtl::string< 'r', '9' >
    > {};

  struct w :
    pegtl::sor<
      a,
      pegtl::string< 'r', 'a', 'x' >,
      pegtl::string< 'r', 'b', 'x' >,
      pegtl::string< 'r', 'b', 'p' >,
      pegtl::string< 'r', '1', '0' >,
      pegtl::string< 'r', '1', '1' >,
      pegtl::string< 'r', '1', '2' >,
      pegtl::string< 'r', '1', '3' >,
      pegtl::string< 'r', '1', '4' >,
      pegtl::string< 'r', '1', '5' >
    > {};

  struct x :
    pegtl::sor<
      w,
      pegtl::string< 'r', 's', 'p' >
    > {};

  struct u :
    pegtl::sor<
      w,
      label
    > {};

  struct t :
    pegtl::sor<
      x,
      number
    > {};

  struct s :
    pegtl::sor<
      x,
      number,
      label
    > {};

  struct aop :
    pegtl::sor<
      pegtl::string< '+', '=' >,
      pegtl::string< '-', '=' >,
      pegtl::string< '*', '=' >,
      pegtl::string< '&', '=' >
    > {};

  struct sop :
    pegtl::sor<
      pegtl::string< '<', '<', '=' >,
      pegtl::string< '>', '>', '=' >
    > {};

  struct cmp :
    pegtl::sor<
      pegtl::string< '<', '=' >,
      pegtl::one< '<' >,
      pegtl::one< '=' >
    > {};

  struct E :
    pegtl::sor<
      pegtl::one< '0' >,
      pegtl::one< '2' >,
      pegtl::one< '4' >,
      pegtl::one< '8' >
    > {};
  
  struct M :
   number {};


  // "mem" instruction rule
  struct mem_opt :
    pegtl::string< 'm', 'e', 'm' > {};

  struct mem_operand_1 : x {};

  struct mem_operand_2 : M {};

  struct instr_mem :
    pegtl::seq<
      seps,
      pegtl::one< '(' >,
      seps,
      mem_opt,
      seps,
      mem_operand_1,
      seps,
      mem_operand_2,
      seps,
      pegtl::one< ')' >,
      seps
    > {};


  // cmp instruction rule
  struct cmp_opt : 
    cmp {};

  struct cmp_left :
    t{};

  struct cmp_right :
    t{};

  struct instr_cmp :
    pegtl::seq<
      seps,
      cmp_left,
      seps,
      cmp_opt,
      seps,
      cmp_right,
      seps
    > {};


  // assign instruction rule
  struct assign_left :
    pegtl::sor<
      w,
      instr_mem
    > {};

  struct assign_right :
    pegtl::sor<
      instr_cmp,
      s,
      instr_mem
    > {};

  struct assign_operator :
    pegtl::string< '<', '-' > {};

 
  // aop instruction rule
  struct aop_left :
    assign_left {};

  struct aop_right :
    pegtl::sor<
      t,
      instr_mem
    > {};

  struct aop_operator :
    aop {};


  // sop instruction rule
  struct sop_left :
    w {};

  struct sop_right :
    pegtl::sor<
      number,
      pegtl::string< 'r', 'c', 'x' >
    > {};

  struct sop_operator :
    sop {};


  // inc and dec instruction rule
  struct inc_dec_regi :
    w {};

  struct inc_dec_opt :
    pegtl::sor<
      pegtl::string< '+','+' >,
      pegtl::string< '-','-' >
    >{};

  struct instr_inc_dec :          // w++ | w--
    pegtl::seq<
      seps,
      inc_dec_regi,
      seps,
      inc_dec_opt,
      seps
    > {};


  // @ instrction rule
  struct lea_left :
    w {};

  struct lea_opt :
    pegtl::string< '@' > {};

  struct lea_operands_2 :
    w {};

  struct lea_operands_3 :
    w {};  

  struct lea_operands_4 :
    E {};

  struct lea_right :          // w @ w w E
    pegtl::seq<
      seps,
      lea_operands_2,
      seps,
      lea_operands_3,
      seps,
      lea_operands_4,
      seps
    > {};

  // tri instruction rule
  // having structure: left opt (right)?
  struct tri_operand_left :
    pegtl::sor<
      assign_left,
      aop_left,
      sop_left,
      inc_dec_regi,
      lea_left
    > {};

  struct tri_operand_right :
    pegtl::sor<
      lea_right,      // must be the first
      assign_right,
      aop_right,
      sop_right
    > {};

  struct tri_operator_rule :
    pegtl::sor<
      assign_operator,
      aop_operator,
      sop_operator,
      inc_dec_opt,
      lea_opt
    > {};

  struct tri_instr :
    pegtl::seq<
      seps,
      tri_operand_left,
      seps,
      tri_operator_rule,
      seps,
      pegtl::opt<
        pegtl::seq<
          tri_operand_right,
          seps
        >
      >
    > {};


  // call instruction rule
  struct call_function_name :
    pegtl::sor<
      pegtl::string< 'p','r','i','n','t' >, 
      pegtl::string< 'a','l','l','o','c','a','t','e' >,
      pegtl::string< 'a','r','r','a','y','-','e','r','r','o','r' >,
      u
    > {};

  struct call_function_paramter :
    number {};

  struct call_name :
      pegtl::string<'c','a','l','l'> {};
  
  struct instr_call :          // (call u N) | (call print 1) | (call allocate 2) | (call array-error 2)
    pegtl::seq<
      seps,
      call_name,
      seps,
      call_function_name,
      seps,
      call_function_paramter,
      seps
    > {};



  // cjump instruction rule
  struct cjump_name :
    pegtl::string<'c','j','u','m','p'> {};

  struct cjump_true :
    label {};

  struct cjump_false :
    label {};

  struct instr_cjump :         // cjump t cmp t label label
    pegtl::seq<
      seps,
      cjump_name,
      seps,
      instr_cmp,
      seps,
      cjump_true,
      seps,
      cjump_false,
      seps
    > {};



  // goto instruction rule
  struct goto_opt :
    pegtl::string< 'g','o','t','o' > {};

  struct goto_label :
    label {};

  struct instr_goto :         // goto label
    pegtl::seq<
      seps,
      goto_opt,
      seps,
      goto_label,
      seps
    > {};



  // return instruction rule
  struct instr_ret : pegtl::string<'r','e','t','u','r','n'> {};

  // instuction
  struct instr :
    pegtl::sor<
      tri_instr,
      instr_call,
      instr_cjump,
      instr_goto,
      instr_ret
    > {};


  // lable instruction rule
  struct instr_label : label {};

  struct L1_instruction_rule:
    pegtl::sor<
      pegtl::seq<
        seps,
        pegtl::one< '(' >,
        seps,
        instr,
        seps,
        pegtl::one< ')' >,
        seps
      >,
      pegtl::seq<
        seps,
        instr_label,
        seps
      >
    > {};


  struct L1_instructions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< L1_instruction_rule >
    > {};

  struct L1_label_rule:
    label {};
 
  struct L1_function_rule:
    pegtl::seq<
      seps,
      pegtl::one< '(' >,
      seps,
      function_name,
      seps,
      argument_number,
      seps,
      local_number,
      seps,
      L1_instructions_rule,
      seps,
      pegtl::one< ')' >,
      seps
    > {};

  struct L1_functions_rule:
    pegtl::seq<
      seps,
      pegtl::plus< L1_function_rule >
    > {};

  struct entry_point_rule:
    pegtl::seq<
      seps,
      pegtl::one< '(' >,
      seps,
      entry_point_label,
      seps,
      L1_functions_rule,
      seps,
      pegtl::one< ')' >,
      seps
    > { };

  struct grammar : 
    pegtl::must< 
      entry_point_rule
    > {};

  /* 
   * Data structures required to parse
   */ 
  std::vector<L1_item> parsed_registers;

  /* 
   * Actions attached to grammar rules.
   */
  template< typename Rule >
  struct action : pegtl::nothing< Rule > {};

  template<> struct action < entry_point_label > {                              // entry point of the entire L1 program
    static void apply( const pegtl::input & in, L1::Program & p){   
      if (p.entryPointLabel.empty()){                               // at beginning, store the first appeared label
        p.entryPointLabel = in.string();
      }
    }
  };

  template<> struct action < function_name > {                      // get the function name (label) after entry point
    static void apply( const pegtl::input & in, L1::Program & p){
      L1::Function *newF = new L1::Function();
      newF->name = in.string();                                     // store the name in the object               
      p.functions.push_back(newF);                                  // push into the program's vector
    }
  };

  template<> struct action < L1_label_rule > {                      // store the label beside the function name
    static void apply( const pegtl::input & in, L1::Program & p){   
      L1_item i;                                                    
      i.labelName = in.string();
      parsed_registers.push_back(i);
    }
  };

  template<> struct action < argument_number > {                    // store argument in current function
    static void apply( const pegtl::input & in, L1::Program & p){
      L1::Function *currentF = p.functions.back();                  // get the pointer of current function 
      currentF->arguments = std::stoll(in.string());                // get function's address, stoll:convert the string to Long Long(8 bytes)
    }
  };

  template<> struct action < local_number > {                       // second parameter in function, similar as the argument
    static void apply( const pegtl::input & in, L1::Program & p){
      L1::Function *currentF = p.functions.back();
      currentF->locals = std::stoll(in.string());
    }
  };

  template<> struct action < tri_operand_left > {                   // for left element at tri_instruction
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();

      // check the last instruction. 
      // If it's incomplete:
      //   If it's created by "mem" instruction, do nothing. 
      //   Variables have taken care by "mem" instruction.
      // else it's complete:
      //   create a new instruction.
      if (!currentF->instructions.empty()) {                        // **
        L1::Instruction *lastI = currentF->instructions.back();   
        // if (!lastI->mem_opt.empty()) {  // this doesn't work 
                                           // when two mem instruction in a row)
        if (lastI->instr_string.empty() && !lastI->mem_opt.empty()) {
        // if (lastI->instr_string.empty() { // this is enough,
                                             // don't need to check mem_opt
          return;
        }
      }
      L1::Instruction *newI = new L1::Instruction();
      newI->operands.push_back(in.string());
      currentF->instructions.push_back(newI);
    }
  };

  template<> struct action < tri_operator_rule > {                        // for opertator 
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->opt = in.string();
    }
  };

  template<> struct action < tri_operand_right > {                    // for right element at tri_instruction
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      
      // if right hand side is not "mem" instruction and not "cmp" instruction 
      // and it is not lea instruction
      if (!currentI->is_right_mem && currentI->cmp_opt.empty() && currentI->opt != "@") {
        currentI->operands.push_back(in.string());
      }
    }
  };

  template<> struct action < mem_opt > {                   // for left element at tri_instruction
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();

      // find out the position of "mem" instruction -- left or right.
      // If this is not the first instruction:
      //   1. If "mem" at right, set mem_opt and is_right_mem flag.
      //   2. If "mem" at left, create a new instruction.
      // else:
      //   create a new instruction.
      if (!currentF->instructions.empty()) {
        L1::Instruction *currentI = currentF->instructions.back();
        // If last instruction is incomplete, it means 
         // this "mem" instruction is on the right side
        if (currentI->instr_string.empty()) {
          currentI->mem_opt = in.string();
          currentI->is_right_mem = true;
          return;
        }
      }
      L1::Instruction *newI = new L1::Instruction();
      newI->mem_opt = in.string();                        // store the operator
      currentF->instructions.push_back(newI);
    }
  };

  template<> struct action < mem_operand_1 > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < mem_operand_2 > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < call_function_name > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *newI = new L1::Instruction();
      newI->operands.push_back(in.string());
      newI->opt = "call";
      currentF->instructions.push_back(newI);
    }
  };

  template<> struct action < call_function_paramter > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < cmp_left > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      // rdi <- rsi will first match tri_left <- cmp_left but not tri_left <- tri_right,
      // because in assign_right, we first match instr_cmp. 
      // So we store cmp_left in temp_operand.
      // If this is a cmp instruction, we push temp_operand into operands list 
      // in the action of cmp_right.
      currentI->temp_operand = in.string();
    }
  };

  template<> struct action < cmp_opt > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->cmp_opt = in.string();
    }
  };

  template<> struct action < cmp_right > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(currentI->temp_operand);
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < cjump_name > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *newI = new L1::Instruction();
      newI->opt = in.string();
      currentF->instructions.push_back(newI);
    }
  };

  template<> struct action < cjump_true > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < cjump_false > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < goto_opt > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *newI = new L1::Instruction();
      newI->opt = in.string();
      currentF->instructions.push_back(newI);
    }
  };

  template<> struct action < goto_label > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < lea_operands_2 > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      // currentI->operands.push_back(in.string());
      currentI->temp_operand = in.string();
    }
  };

  template<> struct action < lea_operands_3 > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(currentI->temp_operand);
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < lea_operands_4 > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->operands.push_back(in.string());
    }
  };

  template<> struct action < instr_ret > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *newI = new L1::Instruction();
      newI-> opt = in.string();
      currentF->instructions.push_back(newI);
    }
  };

  template<> struct action < instr_label > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      L1::Function *currentF = p.functions.back();
      L1::Instruction *newI = new L1::Instruction();
      newI->instr_string = in.string();
      newI->opt = "label";
      newI->operands.push_back(in.string());
      currentF->instructions.push_back(newI);
    }
  };

  template<> struct action < instr > {
    static void apply( const pegtl::input & in, L1::Program & p) {
      // When instr action is activated, it means an instruction is successfully matched.
      // Set "instr_string" with the whole matched instruction
      L1::Function *currentF = p.functions.back();
      L1::Instruction *currentI = currentF->instructions.back();
      currentI->instr_string = in.string();
    }
  };

  Program L1_parse_file (char *fileName){

    /* 
     * Check the grammar for some possible issues.
     */
    pegtl::analyze< L1::grammar >();

    /*
     * Parse.
     */   
    L1::Program p;
    pegtl::file_parser(fileName).parse< L1::grammar, L1::action >(p);
  
    return p;
  }

} // L1
