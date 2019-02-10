#pragma once

#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <map>

using namespace std;

namespace IR {

class Instruction;

enum VAR_TYPE { INT64, TUPLE, CODE };

class BB{
    public:
        std::string label;
        std::vector<IR::Instruction *> instructions;
        IR::Instruction * terminator;

        std::map<std::string, VAR_TYPE> var_type_map;
        std::set<std::string> var_set;
        std::set<std::string> temp_var_set;
        int64_t var_suffix;
};

class Function{
    public:
        std::string name;
        std::string return_type;
        std::vector<std::string> args;
        std::vector<BB *> bbs;
        std::set<std::string> labels;

        std::string remove_percent(std::string s) {
        if (!s.empty() && s.at(0) == '%') {
            s = s.substr(1);
        }
            return s;
        }

        void declare_var() {
            for (auto b : bbs) {
                for (int i = 0; i < args.size(); i++) {
                    std::string type = args[i];
                    std::string v = remove_percent(args[++i]);
                    if (type == "tuple") {
                        b->var_type_map[v] = IR::TUPLE;
                    }
                    else if (type == "code") {
                        b->var_type_map[v] = IR::CODE;
                    }
                    else {
                        b->var_type_map[v] = IR::INT64;
                    }
                    b->temp_var_set.insert(v);
                }
            }
        }
    //std::set<string> callee_registers_to_save;
};

class Program{
    public:
        std::string entryPointLabel;
        std::vector<IR::Function *> functions;
        int64_t label_suffix;
};

class Instruction {
    public:
        virtual std::string toString() const { return ""; }
        virtual std::string translate_to_L3(IR::BB *b) { return ""; }
        
        std::string create_var_with_suffix(IR::BB *b) {
            std::string v = "tv" + std::to_string(b->var_suffix++);
            while (b->var_set.find(v) != b->var_set.end() || b->temp_var_set.find(v) != b->temp_var_set.end()) {
                v = "tv" + std::to_string(b->var_suffix++);
            }
            b->temp_var_set.insert(v);
            return v;
        }
        void clear_temp_var_set(IR::BB *b) {
            b->temp_var_set.clear();
            b->var_suffix = 0;
        }
        std::string translate_array_assign(IR::BB *b, 
            std::string array_name, std::vector<std::string> indices,
            std::string var, bool is_left) { 

            std::string inst = "";

            std::string A = remove_percent(array_name);

            int num_dim = indices.size();

            std::vector<int> dim_addrs;
            for (int i = 1; i < num_dim; i++) {
                dim_addrs.push_back(16+i*8);
            }

            std::string v = create_var_with_suffix(b);
            inst += v + " <- " + remove_percent(indices.back()) + "\n";
            indices.pop_back();

            std::string M_N = "1";

            for (int i = dim_addrs.size()-1; i >= 0; i--) {
                std::string N = M_N;

                std::string ADDR_M = create_var_with_suffix(b);
                std::string M_ = create_var_with_suffix(b);
                std::string M = create_var_with_suffix(b);
                M_N = create_var_with_suffix(b);
                
                inst += ADDR_M + " <- " + A + " + " + std::to_string(dim_addrs[i]) + "\n";
                inst += M_ + " <- load " + ADDR_M + "\n";
                inst += M  + " <- " + M_ + " >> 1\n";

                std::string N_var = create_var_with_suffix(b);
                inst += N_var + " <- " + N + "\n";
                inst += M_N + " <- " + M + " * " + N_var + "\n";

                std::string new_var = create_var_with_suffix(b);

                inst += new_var + " <- " + remove_percent(indices.back()) + " * " + M_N + "\n";
                inst += v + " <- " + v + " + " + new_var + "\n";
                indices.pop_back();
            }

            std::string offsetAfterB = create_var_with_suffix(b);
            std::string offset = create_var_with_suffix(b);
            std::string addr = create_var_with_suffix(b);
            inst += offsetAfterB + " <- " + v + " * 8\n";
            inst += offset + " <- " + offsetAfterB + " + " + std::to_string(16+num_dim*8) + "\n";
            inst += addr + " <- " + A + " + " + offset + "\n";

            if (is_left) {
                inst += "store " + addr + " <- " + var + "\n";
            }
            else {
                inst += var + " <- load " + addr + "\n";
            }

            clear_temp_var_set(b);
            return inst;
        }

        std::string remove_percent(std::string s) {
            if (!s.empty() && s.at(0) == '%') {
                s = s.substr(1);
            }
            return s;
        }
};

class Type_Instruction : public Instruction {
    public:
        std::string type;
        std::string var;

        Type_Instruction() {}
        Type_Instruction(std::string v) : var(v) {}

        std::string toString() const {
            std::string s("");
            s += type + " " + var;
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            std::string v = remove_percent(var);
            if (type == "tuple") {
                b->var_type_map[v] = IR::TUPLE;
            }
            else if (type == "code") {
                b->var_type_map[v] = IR::CODE;
            }
            else {
                b->var_type_map[v] = IR::INT64;
            }
            b->var_set.insert(v);
            return "";
        }
};

class Simple_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::string assign_right;

        Simple_Assign_Instruction() {}
        Simple_Assign_Instruction(std::string l, std::string r) 
        : assign_left(l), assign_right(r) {}

        std::string toString() const {
            std::string s("");
            s += assign_left + " <- " + assign_right;
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            std::string s("");
            std::string al = remove_percent(assign_left);
            std::string ar = remove_percent(assign_right);

            s += al + " <- " + ar + "\n";
            return s;
        }
};

class Op_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::string op;
        std::string op_left;
        std::string op_right;

        Op_Assign_Instruction() {}
        Op_Assign_Instruction(std::string l, std::string o, std::string oleft, std::string oright)
        : assign_left(l), op(o), op_left(oleft), op_right(oright) {}

        std::string toString() const {
            std::string s("");
            s += assign_left + " <- " + op_left + " " + op + " " + op_right;
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            return remove_percent(assign_left) + " <- " + remove_percent(op_left) + " " + op + " " + remove_percent(op_right) + "\n";
        }
};

class Right_Array_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::string array_name;
        std::vector<std::string> indices;

        Right_Array_Assign_Instruction() {}
        Right_Array_Assign_Instruction(std::string l, std::string an, std::vector<std::string> ind)
        : assign_left(l), array_name(an), indices(ind) {}

        std::string toString() const {
            std::ostringstream oss;

            if (!indices.empty()) {
                // Convert all but the last element to avoid a trailing ","
                std::copy(indices.begin(), indices.end()-1, std::ostream_iterator<std::string>(oss, "]["));
                // Now add the last element with no delimiter
                oss << indices.back();
            }

            std::string s("");
            s += assign_left + " <- " + array_name + "[" + oss.str() + "]";
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            if (b->var_type_map[remove_percent(array_name)] == IR::INT64) {
                return translate_array_assign(b, array_name, indices, remove_percent(assign_left), false);
            }
            else if (b->var_type_map[remove_percent(array_name)] == IR::TUPLE) {
                std::string newVar = create_var_with_suffix(b);
                std::string inst = "";
                inst += newVar + " <- " + remove_percent(array_name) + " + " + std::to_string((std::stoi(indices[0])+1)*8) + "\n";
                inst += remove_percent(assign_left) + " <- load " + newVar + "\n";
                clear_temp_var_set(b);
                return inst;
            }
            else {
                cout << "Wrong var type in Right_Array_Assign_Instruction." << endl;
                return "";
            }
        }

};

class Left_Array_Assign_Instruction : public Instruction {
    public:
        std::string assign_right;
        std::string array_name;
        std::vector<std::string> indices;

        Left_Array_Assign_Instruction() {}
        Left_Array_Assign_Instruction(std::string r, std::string an, std::vector<std::string> ind)
        : assign_right(r), array_name(an), indices(ind) {}

        std::string toString() const {
            std::ostringstream oss;

            if (!indices.empty()) {
                // Convert all but the last element to avoid a trailing ","
                std::copy(indices.begin(), indices.end()-1, std::ostream_iterator<std::string>(oss, "]["));
                // Now add the last element with no delimiter
                oss << indices.back();
            }

            std::string s("");
            s +=  array_name + "[" + oss.str() + "]" + " <- " + assign_right;
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            if (b->var_type_map[remove_percent(array_name)] == IR::INT64) {
                return translate_array_assign(b, array_name, indices, remove_percent(assign_right), true);
            }
            else if (b->var_type_map[remove_percent(array_name)] == IR::TUPLE) {
                std::string newVar = create_var_with_suffix(b);
                std::string inst = "";
                inst += newVar + " <- " + remove_percent(array_name) + " + " + std::to_string((std::stoi(indices[0])+1)*8) + "\n";
                inst += "store " + newVar + " <- " + remove_percent(assign_right) + "\n";
                clear_temp_var_set(b);
                return inst;
            }
            else {
                cout << "Wrong var type in Right_Array_Assign_Instruction." << endl;
                return "";
            }
        }

};

class Length_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::string var;
        std::string t;

        Length_Assign_Instruction() {}
        Length_Assign_Instruction(std::string l, std::string v) : assign_left(l), var(v) {}

        std::string toString() const {
            std::string s("");
            s += assign_left + " <- length " + var + " " + t;
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            std::string v1,v2,lv,v,inst;
            lv = remove_percent(assign_left);
            v = remove_percent(var);

            inst = "";
            if (b->var_type_map[v] == TUPLE) {
                inst += lv + " <- load " + v + "\n";
            }
            else {
                v1 = create_var_with_suffix(b);
                v2 = create_var_with_suffix(b);
                
                inst += v1 + " <- " + t + " * 8\n"; 
                inst += v1 + " <- " + v1 + " + " + "16\n";
                inst += v2 + " <- " + v + " + " + v1 + "\n";
                inst += lv + " <- load " + v2 + "\n"; 
            }
            clear_temp_var_set(b);
            return inst;
        }
};

class Call_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::string callee;
        std::vector<std::string> args;

        Call_Assign_Instruction() {}
        Call_Assign_Instruction(std::string l, std::string cle, std::vector<std::string> a) 
        : assign_left(l), callee(cle), args(a) {}


        std::string toString() const {

            std::ostringstream oss;

            if (!args.empty()) {
                // Convert all but the last element to avoid a trailing ","
                std::copy(args.begin(), args.end()-1, std::ostream_iterator<std::string>(oss, ","));
                // Now add the last element with no delimiter
                oss << args.back();
            }

            std::string s("");
            s += assign_left + " <- call " + callee + " (" + oss.str() + ")";
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {

            for (int i = 0; i < args.size(); i++) {
                args[i] = remove_percent(args[i]);
            }

            std::ostringstream oss;

            if (!args.empty()) {
                // Convert all but the last element to avoid a trailing ","
                std::copy(args.begin(), args.end()-1, std::ostream_iterator<std::string>(oss, ","));
                // Now add the last element with no delimiter
                oss << args.back();
            }

            std::string s("");
            s += remove_percent(assign_left) + " <- call " + remove_percent(callee) + " (" + oss.str() + ")\n";
            return s;
        }

};

class Br1_Instruction : public Instruction {
    public:
        std::string label;

        Br1_Instruction() {}
        Br1_Instruction(std::string l) : label(l) {}
        
        std::string toString() const {
            std::string s("");
            s += "br " + label;
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            std::string s("");
            s += "br " + label + "\n";
            return s;
        }


};

class Br2_Instruction : public Instruction {
    public:
        std::string var;
        std::string label1;
        std::string label2;

        Br2_Instruction() {}
        Br2_Instruction(std::string v, std::string l1, std::string l2) 
        : var(v), label1(l1), label2(l2) {}
        
        std::string toString() const {
            std::string s("");
            s += "br " + var + " " + label1 + " " + label2;
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            std::string s("");
            s += "br " + remove_percent(var) + " " + label1 + " " + label2 + "\n";
            return s;
        }
};

class Return_Instruction : public Instruction {
    public:
        std::string toString() const {
            return "return";
        }

        std::string translate_to_L3(IR::BB *b) {
            return "return\n";
        }
};

class Var_Return_Instruction : public Return_Instruction {
    public:
        std::string var;

        Var_Return_Instruction() {}
        Var_Return_Instruction(std::string v) : var(v) {}
        
        std::string toString() const {
            std::string s("");
            s += "return " + var;
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            std::string s("");
            s += "return " + remove_percent(var) + "\n";
            return s;
        }
};

class Call_Instruction : public Instruction {
    public:
        std::string callee;
        std::vector<std::string> args;

        Call_Instruction() {}
        Call_Instruction(std::string cle, std::vector<std::string> a) : callee(cle), args(a) {}

        std::string toString() const {

            std::ostringstream oss;

            if (!args.empty()) {

                // Convert all but the last element to avoid a trailing ","
                std::copy(args.begin(), args.end()-1, std::ostream_iterator<std::string>(oss, ","));
                // Now add the last element with no delimiter
                oss << args.back();
            }

            std::string s("");
            s = s + "call " + callee + " (" + oss.str() + ")";
            
            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            std::ostringstream oss;

            for (int i = 0; i < args.size(); i++) {
                args[i] = remove_percent(args[i]);
            }

            if (!args.empty()) {
                // Convert all but the last element to avoid a trailing ","
                std::copy(args.begin(), args.end()-1, std::ostream_iterator<std::string>(oss, ","));
                // Now add the last element with no delimiter
                oss << args.back();
            }

            std::string s("");
            s = s + "call " + remove_percent(callee) + " (" + oss.str() + ")\n";
            return s;
        }
};

class Label_Instruction : public Instruction {
    public:
        std::string label;

        Label_Instruction() {}
        Label_Instruction(std::string l) : label(l) {}

        std::string toString() const {
            return label;
        }

        std::string translate_to_L3(IR::BB *b) {
            return std::string(label) + "\n";
        }
};

class New_Array_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::vector<std::string> args;

        New_Array_Assign_Instruction() {}
        New_Array_Assign_Instruction(std::string l, std::vector<std::string> a)
         : assign_left(l), args(a) {}

        std::string toString() const {
            std::ostringstream oss;

            if (!args.empty()) {
                // Convert all but the last element to avoid a trailing ","
                std::copy(args.begin(), args.end()-1, std::ostream_iterator<std::string>(oss, ","));
                // Now add the last element with no delimiter
                oss << args.back();
            }

            std::string s("");
            s += assign_left + " <- new Array(" + oss.str() + ")";

            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            int arg_num = args.size();

            std::vector<std::string> pd_vec;
            int i = 0;
            while (i < arg_num) {
                pd_vec.push_back(create_var_with_suffix(b));
                i++;
            }

            std::string v0 = create_var_with_suffix(b);
            std::string a = remove_percent(assign_left);

            std::vector<std::string> p_vec;
            for (auto arg : args) {
                p_vec.push_back(remove_percent(arg));
            }

            std::string inst = "";

            for (int i = 0; i < arg_num; i++) {
                inst += pd_vec[i] + " <- " + p_vec[i] + " >> 1\n";
            }

            // std::ostringstream oss;
            // if (!pd_vec.empty()) {
            //     // Convert all but the last element to avoid a trailing ","
            //     std::copy(pd_vec.begin(), pd_vec.end()-1, std::ostream_iterator<std::string>(oss, " * "));
            //     // Now add the last element with no delimiter
            //     oss << pd_vec.back();
            // }
            if (!pd_vec.empty()) {
                inst += v0 + " <- " + pd_vec[0] + "\n";
                for (int i = 1; i < pd_vec.size(); i++) {
                    inst += v0 + " <- " + v0 + " * " + pd_vec[i] + "\n";
                }
            }

            inst += v0 + " <- " + v0 + " << 1\n";
            // inst += v0 + " <- " + v0 + " + 1\n";
            inst += v0 + " <- " + v0 + " + " + std::to_string(2*(1+arg_num)+1) + "\n";
            inst += a + " <- call allocate(" + v0 + ",1)\n";
            inst += v0 + " <- " + a + " + 8\n";
            inst += "store " + v0 + " <- " + std::to_string(2*arg_num+1) + "\n";

            for (int i = 0; i < arg_num; i++) {
                inst += v0 + " <- " + a + " + " + std::to_string((i+2)*8) + "\n";
                inst += "store " + v0 + " <- " + p_vec[i] + "\n";
            }

            clear_temp_var_set(b);
            
            return inst;
        }
};

class New_Tuple_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::string t;

        New_Tuple_Assign_Instruction() {}
        New_Tuple_Assign_Instruction(std::string l, std::string t) 
        : assign_left(l), t(t) {}

        std::string toString() const {
            

            std::string s("");
            s += assign_left + " <- new Tuple(" + t + ")";

            return s;
        }

        std::string translate_to_L3(IR::BB *b) {
            return remove_percent(assign_left) + " <- call allocate(" + t + ",1)\n";
        }
};

// class New_Array_Instruction : public Instruction {
//     public:
//         std::vector<std::string> args;

//         New_Array_Instruction() {}
//         New_Array_Instruction(std::vector<std::string> a) : args(a) {}

//         std::string toString() const {
//             std::ostringstream oss;

//             if (!args.empty()) {
//                 // Convert all but the last element to avoid a trailing ","
//                 std::copy(args.begin(), args.end()-1, std::ostream_iterator<std::string>(oss, ","));
//                 // Now add the last element with no delimiter
//                 oss << args.back();
//             }

//             std::string s("");
//             s = s + "new Array(" + oss.str() + ")";

//             return s;
//         }
// };

// class New_Tuple_Instruction : public Instruction {
//     public:
//         std::string t;

//         New_Tuple_Instruction() {}
//         New_Tuple_Instruction(std::string t) : t(t) {}

//         std::string toString() const {
//             std::string s("");
//             s = s + "new Tuple(" + t + ")";

//             return s;
//         }
// };
} // IR
