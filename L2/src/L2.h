#pragma once

#include <vector>

namespace L2 {

  struct Instruction {
    // flag of complete matched instruction, empty is
    // the instruction is incomplete
    // Used in "mem" instructions.
    std::string instr_string;

    // store the operator of the instruction.
    // "<-", aop, sop, "cjump", "goto", "label", "return", "call", "++", "--", "@".   
    std::string opt;

    // store cmp operator. ("<", "<=" and "=")
    std::string cmp_opt;

    // store mem operator. ("mem")
    std::string mem_opt;

    // store operator of instruction stack_arg, which is "stack_arg"
    std::string stack_opt;

    // true if the "mem" instruction is at the right side of an instruction
    bool is_right_mem;

    // store first variable in cmp instruction
    // rdi <- rsi will first match tri_left <- cmp_left but not tri_left <- tri_right,
    // because in assign_right, we first match instr_cmp. 
    // So we store cmp_left in temp_operand.
    // If this is a cmp instruction, we push temp_operand into operands list 
    // in the action of cmp_right.
    std::string temp_operand;

    // store variables in instruction (e.g. w, t, label, etc.)
    std::vector<std::string> operands;

    std::set<std::string> GEN;
    std::set<std::string> KILL;
  };

  struct L2_item {
    std::string labelName;
  };

  struct Function{
    std::string name;
    int64_t arguments;
    int64_t locals;
    int64_t caller_locals;
    std::vector<L2::Instruction *> instructions;
    std::set<std::string> vars;
    // int64_t suffix;
    //std::set<string> callee_registers_to_save;
  };

  struct Program{
    std::string entryPointLabel;
    std::vector<L2::Function *> functions;
  };

} // L2
