#pragma once

#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <map>

using namespace std;

namespace LA {

class Instruction;

enum VAR_TYPE { INT64, TUPLE, CODE };

enum INST_TYPE { OTHER, TE, BR2, LBL };

class Function{
    public:
        std::string name;
        std::string return_type;
        std::vector<std::string> args;
        std::vector<LA::Instruction *> instructions;
        std::set<std::string> labels;

        std::map<std::string, VAR_TYPE> var_type_map;
        std::set<std::string> var_set;
        std::set<std::string> temp_var_set;

        std::map<std::string, int64_t> arr_dim_map;
        std::map<std::string, std::string> tuple_length_map;

        int64_t var_suffix;
        int64_t label_suffix;

        std::string remove_percent(std::string s) {
        if (!s.empty() && s.at(0) == '%') {
            s = s.substr(1);
        }
            return s;
        }

        int count_dim(std::string s) {
            int n = 0;
            for (int i = 0; i < s.length(); i++) {
                if (s.at(i) == '[') {
                    n++;
                }
            }
            return n;
        }

        void declare_var() {
            for (int i = 0; i < args.size(); i++) {
                std::string type = args[i];
                std::string v = remove_percent(args[++i]);
                if (type == "tuple") {
                    var_type_map[v] = LA::TUPLE;
                    arr_dim_map[v] = 1;
                    // cout << "cool" << endl;
                }
                else if (type == "code") {
                    var_type_map[v] = LA::CODE;
                }
                else {
                    var_type_map[v] = LA::INT64;
                    arr_dim_map[v] = count_dim(type);
                }
                temp_var_set.insert(v);
            }
        }
    //std::set<string> callee_registers_to_save;
};

class Program{
    public:
        std::string entryPointLabel;
        std::vector<LA::Function *> functions;
        // int64_t label_suffix;
};

class Instruction {
    public:
        INST_TYPE i_type;

        std::vector<std::string> to_encode_vec;
        std::vector<std::string> to_decode_vec;
        std::vector<Instruction *> en_decoded_instructions;
        std::vector<Instruction *> array_check_instructions;

        virtual std::string toString() const { return ""; }
        virtual std::string translate_to_IR(LA::Function *f) { return ""; }
        virtual void encode_constant() { return; }
        virtual void to_encode(LA::Function *f) { return; }
        virtual void to_decode(LA::Function *f) { return; }
        virtual void encoding_instruction(LA::Function *f, std::string & v) { return; }
        virtual void decoding_instruction(LA::Function *f, std::string & v) { return; }
        virtual void check_array(LA::Function *f) { return; }
        virtual std::string get_label() { return ""; }

        INST_TYPE get_inst_type() {
            return i_type;
        }
        // virtual std::vector<Instruction *> get_encoded_instruction(LA::Function *f) { return; }
        
        bool is_constant(std::string s) {
            if (s.empty()) return false;
            if (isdigit(s.at(0))) {
                return true;
            }
            else if (s.size() > 1 && (s.at(0)=='+' || s.at(0)=='-') && isdigit(s.at(1))) {
                return true;
            }
            else return false;
        }

        // void append_vec(std::vector<Instruction *> A, std::vector<Instruction *> B) {
        //     std::vector<Instruction *> AB;
        //     AB.reserve( A.size() + B.size() ); // preallocate memory
        //     AB.insert( AB.end(), A.begin(), A.end() );
        //     AB.insert( AB.end(), B.begin(), B.end() );
        //     return AB;
        // }
        // void encode(std::string s) {

        // }

        std::string create_var_with_suffix(LA::Function *f) {
            std::string v = "%tv" + std::to_string(f->var_suffix++);
            while (f->var_set.find(v) != f->var_set.end() || f->temp_var_set.find(v) != f->temp_var_set.end()) {
                v = "%tv" + std::to_string(f->var_suffix++);
            }
            f->temp_var_set.insert(v);
            return v;
        }

        void clear_temp_var_set(LA::Function *f) {
            f->temp_var_set.clear();
            f->var_suffix = 0;
        }

        std::string translate_array_assign(LA::Function *f, 
            std::string array_name, std::vector<std::string> indices,
            std::string var, bool is_left) { 

            std::string inst = "";

            std::string A = remove_percent(array_name);

            int num_dim = indices.size();

            std::vector<int> dim_addrs;
            for (int i = 1; i < num_dim; i++) {
                dim_addrs.push_back(16+i*8);
            }

            std::string v = create_var_with_suffix(f);
            inst += v + " <- " + remove_percent(indices.back()) + "\n";
            indices.pop_back();

            std::string M_N = "1";

            for (int i = dim_addrs.size()-1; i >= 0; i--) {
                std::string N = M_N;

                std::string ADDR_M = create_var_with_suffix(f);
                std::string M_ = create_var_with_suffix(f);
                std::string M = create_var_with_suffix(f);
                M_N = create_var_with_suffix(f);
                
                inst += ADDR_M + " <- " + A + " + " + std::to_string(dim_addrs[i]) + "\n";
                inst += M_ + " <- load " + ADDR_M + "\n";
                inst += M  + " <- " + M_ + " >> 1\n";

                std::string N_var = create_var_with_suffix(f);
                inst += N_var + " <- " + N + "\n";
                inst += M_N + " <- " + M + " * " + N_var + "\n";

                std::string new_var = create_var_with_suffix(f);

                inst += new_var + " <- " + remove_percent(indices.back()) + " * " + M_N + "\n";
                inst += v + " <- " + v + " + " + new_var + "\n";
                indices.pop_back();
            }

            std::string offsetAfterB = create_var_with_suffix(f);
            std::string offset = create_var_with_suffix(f);
            std::string addr = create_var_with_suffix(f);
            inst += offsetAfterB + " <- " + v + " * 8\n";
            inst += offset + " <- " + offsetAfterB + " + " + std::to_string(16+num_dim*8) + "\n";
            inst += addr + " <- " + A + " + " + offset + "\n";

            if (is_left) {
                inst += "store " + addr + " <- " + var + "\n";
            }
            else {
                inst += var + " <- load " + addr + "\n";
            }

            
            clear_temp_var_set(f);
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
        Type_Instruction(LA::Function *f, std::string t, std::string v) {
            var = v;
            type = t;
            v = remove_percent(var);
            if (type == "tuple") {
                f->var_type_map[v] = LA::TUPLE;
                f->arr_dim_map[v] = 0;
                // cout << "cool1" << endl;
            }
            else if (type == "code") {
                f->var_type_map[v] = LA::CODE;
            }
            else {
                f->var_type_map[v] = LA::INT64;
                f->arr_dim_map[v] = 0;
            }
            f->var_set.insert(v);
        }

        std::string toString() const {
            std::string s("");
            s += type + " " + var;
            return s;
        }

        std::string translate_to_IR(LA::Function *f) {
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

        std::string translate_to_IR(LA::Function *f) {
            std::string s("");
            std::string al = remove_percent(assign_left);
            std::string ar = remove_percent(assign_right);

            s += al + " <- " + ar + "\n";
            return s;
        }

        void encode_constant() {
            if (is_constant(assign_right)) {
                assign_right = std::to_string(stoi(assign_right) * 2 + 1);
            }
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

        std::string translate_to_IR(LA::Function *f) {
            return remove_percent(assign_left) + " <- " + remove_percent(op_left) + " " + op + " " + remove_percent(op_right) + "\n";
        }

        void encode_constant() {
            if (is_constant(op_left)) {
                op_left = std::to_string(stoi(op_left) * 2 + 1);
            }
            if (is_constant(op_right)) {
                op_right = std::to_string(stoi(op_right) * 2 + 1);
            }
        }

        void encoding_instruction(LA::Function *f, std::string & v) {
            // std::string tv = create_var_with_suffix(f);
            Op_Assign_Instruction * i1 = new Op_Assign_Instruction(
                v, "<<", v, "1");
            en_decoded_instructions.push_back(i1);
            Op_Assign_Instruction * i2 = new Op_Assign_Instruction(
                v, "+", v, "1");
            en_decoded_instructions.push_back(i2);
        }

        void decoding_instruction(LA::Function *f, std::string & v) {
            std::string tv = create_var_with_suffix(f);

            Type_Instruction * ti = new Type_Instruction(f,"int64",tv);
            en_decoded_instructions.push_back(ti);

            Op_Assign_Instruction * i = new Op_Assign_Instruction(
                tv, ">>", v, "1");
            v = tv;
            en_decoded_instructions.push_back(i);
        }

        void to_decode(LA::Function *f) {
            // if (!is_constant(op_left)) {
            //     to_decode_vec.push_back(op_left);
            // }
            // if (!is_constant(op_right)) {
            //     to_decode_vec.push_back(op_right);
            // }
            if (!is_constant(op_left)) {
                decoding_instruction(f,op_left);
            }
            else {
                op_left = std::to_string(stoi(op_left) / 2);
            }
            if (!is_constant(op_right)) {
                decoding_instruction(f,op_right);
            }
            else {
                op_right = std::to_string(stoi(op_right) / 2);
            }
            en_decoded_instructions.push_back(this);
        }

        void to_encode(LA::Function *f) {
            if (!is_constant(assign_left)) {
                encoding_instruction(f,assign_left);
            }
        }
};

class Length_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::string var;
        std::string t;

        Length_Assign_Instruction() {}
        Length_Assign_Instruction(std::string l, std::string v, std::string t) : assign_left(l), var(v), t(t) {}

        std::string toString() const {
            std::string s("");
            s += assign_left + " <- length " + var + " " + t;
            return s;
        }

        std::string translate_to_IR(LA::Function *f) {
            std::string v1,v2,lv,v,inst;
            lv = remove_percent(assign_left);
            v = remove_percent(var);

            v1 = create_var_with_suffix(f);
            v2 = create_var_with_suffix(f);
            
            inst = "";
            inst += v1 + " <- " + t + " * 8\n"; 
            inst += v1 + " <- " + v1 + " + " + "16\n";
            inst += v2 + " <- " + v + " + " + v1 + "\n";
            inst += lv + " <- load " + v2 + "\n";

            
            clear_temp_var_set(f);
            return inst;
        }

        void decoding_instruction(LA::Function *f, std::string & v) {
            std::string tv = create_var_with_suffix(f);

            Type_Instruction * ti = new Type_Instruction(f,"int64",tv);
            en_decoded_instructions.push_back(ti);

            Op_Assign_Instruction * i = new Op_Assign_Instruction(
                tv, ">>", v, "1");
            v = tv;
            en_decoded_instructions.push_back(i);
        }

        void to_decode(LA::Function *f) {
            if (!is_constant(t)) {
                // to_decode_vec.push_back(t);
                decoding_instruction(f,t);
            }
            en_decoded_instructions.push_back(this);
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
            if (callee == "print" || callee == "array-error" || callee.at(0) == '%') {
                s += assign_left + " <- call " + callee + " (" + oss.str() + ")";
            }
            else {
                s += assign_left + " <- call :" + callee + " (" + oss.str() + ")";
            }
            return s;
        }

        std::string translate_to_IR(LA::Function *f) {
            std::ostringstream oss;

            if (!args.empty()) {
                // Convert all but the last element to avoid a trailing ","
                std::copy(args.begin(), args.end()-1, std::ostream_iterator<std::string>(oss, ","));
                // Now add the last element with no delimiter
                oss << remove_percent(args.back());
            }

            std::string s("");
            s += remove_percent(assign_left) + " <- call " + remove_percent(callee) + " (" + remove_percent(oss.str()) + ")\n";
            return s;
        }

        void encode_constant() {
            if (callee == "array-error" && args[0] == "0" && args[1] == "0") {
                return;
            }
            for (int i = 0; i < args.size(); i++) {
                if (is_constant(args[i])) {
                    args[i] = std::to_string(stoi(args[i]) * 2 + 1);
                }
            }
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
            if (callee == "print" || callee == "array-error" || callee.at(0) == '%') {
                s = s + "call " + callee + " (" + oss.str() + ")";
            }
            else {
                s = s + "call :" + callee + " (" + oss.str() + ")";
            }
            
            return s;
        }

        std::string translate_to_IR(LA::Function *f) {
            std::ostringstream oss;

            if (!args.empty()) {
                // Convert all but the last element to avoid a trailing ","
                std::copy(args.begin(), args.end()-1, std::ostream_iterator<std::string>(oss, ","));
                // Now add the last element with no delimiter
                oss << remove_percent(args.back());
            }

            std::string s("");
            s = s + "call " + remove_percent(callee) + " (" + remove_percent(oss.str()) + ")\n";
            return s;
        }

        void encode_constant() {
            if (callee == "array-error" && args[0] == "0" && args[1] == "0") {
                return;
            }
            for (int i = 0; i < args.size(); i++) {
                if (is_constant(args[i])) {
                    args[i] = std::to_string(stoi(args[i]) * 2 + 1);
                }
            }
        }
};

class Br2_Instruction : public Instruction {
    public:
        std::string var;
        std::string label1;
        std::string label2;

        Br2_Instruction() {}
        Br2_Instruction(std::string v, std::string l1, std::string l2) 
        : var(v), label1(l1), label2(l2) {
            i_type = TE;
        }
        
        std::string toString() const {
            std::string s("");
            s += "br " + var + " " + label1 + " " + label2;
            return s;
        }

        std::string translate_to_IR(LA::Function *f) {
            std::string s("");
            s += "br " + remove_percent(var) + " " + label1 + " " + label2 + "\n";
            return s;
        }

        void encode_constant() {
            if (is_constant(var)) {
                cout << "encode br2 t?" << endl; 
            }
        }

        void decoding_instruction(LA::Function *f, std::string & v) {
            std::string tv = create_var_with_suffix(f);

            Type_Instruction * ti = new Type_Instruction(f,"int64",tv);
            en_decoded_instructions.push_back(ti);

            Op_Assign_Instruction * i = new Op_Assign_Instruction(
                tv, ">>", v, "1");
            v = tv;
            en_decoded_instructions.push_back(i);
        }

        void to_decode(LA::Function *f) {
            if (!is_constant(var)) {
                // to_decode_vec.push_back(var);
                decoding_instruction(f,var);
            }
            en_decoded_instructions.push_back(this);
        }
};

class Label_Instruction : public Instruction {
    public:
        std::string label;

        Label_Instruction() {}
        Label_Instruction(std::string l) : label(l) {
            i_type = LBL;
        }

        std::string toString() const {
            return label;
        }

        std::string translate_to_IR(LA::Function *f) {
            return std::string(label) + "\n";
        }

        std::string get_label() {
            return label;
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

        std::string translate_to_IR(LA::Function *f) {
            if (f->var_type_map[remove_percent(array_name)] == LA::INT64) {
                return translate_array_assign(f, array_name, indices, remove_percent(assign_left), false);
            }
            else if (f->var_type_map[remove_percent(array_name)] == LA::TUPLE) {
                std::string newVar = create_var_with_suffix(f);
                std::string inst = "";
                inst += newVar + " <- " + remove_percent(array_name) + " + " + std::to_string((std::stoi(indices[0])+1)*8) + "\n";
                inst += remove_percent(assign_left) + " <- load " + newVar + "\n";
                
                clear_temp_var_set(f);
                return inst;
            }
            else {
                cout << "Wrong var type in Right_Array_Assign_Instruction." << endl;
                return "";
            }
        }

        // void encoding_instruction(LA::Function *f, std::string & v) {
        //     std::string tv = create_var_with_suffix(f);
        //     Op_Assign_Instruction * i1 = new Op_Assign_Instruction(
        //         tv, "<<", v, "1");
        //     en_decoded_instructions.push_back(i1);
        //     Op_Assign_Instruction * i2 = new Op_Assign_Instruction(
        //         tv, "+", tv, "1");
        //     en_decoded_instructions.push_back(i2);
        // }

        void decoding_instruction(LA::Function *f, std::string & v) {
            std::string tv = create_var_with_suffix(f);

            Type_Instruction * ti = new Type_Instruction(f,"int64",tv);
            en_decoded_instructions.push_back(ti);

            Op_Assign_Instruction * i = new Op_Assign_Instruction(
                tv, ">>", v, "1");
            v = tv;
            en_decoded_instructions.push_back(i);
        }

        void to_decode(LA::Function *f) {
            for (int i = 0; i < indices.size(); i++) {
                if (!is_constant(indices[i])) {
                    // to_decode.push_back(indices[i]);
                    decoding_instruction(f,indices[i]);
                }
            }
            en_decoded_instructions.push_back(this);
        }

        void check_array(LA::Function *f) {
            // cout << "checking array" << endl;
            // cout << array_name << endl;
            // for (auto p : f->var_type_map) {
            //     cout << p.first << " " << p.second << endl;
            // }
            int num_dim = indices.size();

            if (f->var_type_map.find(remove_percent(array_name)) == f->var_type_map.end()
                || f->arr_dim_map[remove_percent(array_name)] == 0) {
                array_check_instructions.push_back(
                    new Call_Instruction("array-error",std::vector<std::string> (2,"0")));
            }
            else if (f->arr_dim_map[remove_percent(array_name)] != num_dim) {
                std::vector<std::string> targs;
                targs.push_back(array_name);
                targs.push_back("-1");
                array_check_instructions.push_back(
                    new Call_Instruction("array-error",targs));
            }
            else if (f->var_type_map[remove_percent(array_name)] == TUPLE) {
                return;
            //     std::string len = create_var_with_suffix(f);
            //         array_check_instructions.push_back(
            //             new Length_Assign_Instruction(len,array_name,"0"));

            //     std::string cmp = create_var_with_suffix(f);
            //     array_check_instructions.push_back(
            //         new Op_Assign_Instruction(cmp,">=",indices[0],len));

            //     std::string lbl1 = ":";
            //     lbl1 += remove_percent(create_var_with_suffix(f));
            //     std::string lbl2 = ":";
            //     lbl2 += remove_percent(create_var_with_suffix(f));

            //     array_check_instructions.push_back(
            //         new Br2_Instruction(cmp,lbl1,lbl2));
                
            //     // cout << "here1" << endl;

            //     array_check_instructions.push_back(
            //         new Label_Instruction(lbl1));

            //     // cout << "here2" << endl;
            //     std::vector<std::string> targs;
            //     targs.push_back(array_name);
            //     targs.push_back(indices[0]);

            //     array_check_instructions.push_back(
            //         new Call_Instruction("array-error",targs));

            //     // cout << "here3" << endl;
            //     array_check_instructions.push_back(
            //         new Label_Instruction(lbl2));
            }
            else {
                
                // cout << num_dim << endl;

                // std::vector<int> dim_addrs;
                // for (int i = 0; i < num_dim; i++) {
                //     dim_addrs.push_back(16+i*8);
                // }

                for (int i = 0; i < num_dim; i++) {
                    // cout << i << endl;

                    std::string len = create_var_with_suffix(f);
                    array_check_instructions.push_back(
                        new Length_Assign_Instruction(len,array_name,std::to_string(i)));
                    
                    std::string cmp = create_var_with_suffix(f);
                    array_check_instructions.push_back(
                        new Op_Assign_Instruction(cmp,">=",indices[i],len));

                    // need to change, might have bug
                    std::string lbl1 = ":";
                    lbl1 += remove_percent(create_var_with_suffix(f));
                    std::string lbl2 = ":";
                    lbl2 += remove_percent(create_var_with_suffix(f));

                    array_check_instructions.push_back(
                        new Br2_Instruction(cmp,lbl1,lbl2));
                    
                    // cout << "here1" << endl;

                    array_check_instructions.push_back(
                        new Label_Instruction(lbl1));

                    // cout << "here2" << endl;
                    std::vector<std::string> targs;
                    targs.push_back(array_name);
                    targs.push_back(indices[i]);

                    array_check_instructions.push_back(
                        new Call_Instruction("array-error",targs));

                    // cout << "here3" << endl;
                    array_check_instructions.push_back(
                        new Label_Instruction(lbl2));

                    
                }
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
            s += array_name + "[" + oss.str() + "]" + " <- " + assign_right;
            return s;
        }

        std::string translate_to_IR(LA::Function *f) {
            if (f->var_type_map[remove_percent(array_name)] == LA::INT64) {
                return translate_array_assign(f, array_name, indices, remove_percent(assign_right), true);
            }
            else if (f->var_type_map[remove_percent(array_name)] == LA::TUPLE) {
                std::string newVar = create_var_with_suffix(f);
                std::string inst = "";
                inst += newVar + " <- " + remove_percent(array_name) + " + " + std::to_string((std::stoi(indices[0])+1)*8) + "\n";
                inst += "store " + newVar + " <- " + remove_percent(assign_right) + "\n";
                
                clear_temp_var_set(f);
                return inst;
            }
            else {
                cout << "Wrong var type in Right_Array_Assign_Instruction." << endl;
                return "";
            }
        }

        void encode_constant() {
            if (is_constant(assign_right)) {
                assign_right = std::to_string(stoi(assign_right) * 2 + 1);
            }
        }


        void decoding_instruction(LA::Function *f, std::string & v) {
            std::string tv = create_var_with_suffix(f);

            Type_Instruction * ti = new Type_Instruction(f,"int64",tv);
            en_decoded_instructions.push_back(ti);

            Op_Assign_Instruction * i = new Op_Assign_Instruction(
                tv, ">>", v, "1");

            v = tv;
            en_decoded_instructions.push_back(i);
        }

        void to_decode(LA::Function *f) {
            for (int i = 0; i < indices.size(); i++) {
                if (!is_constant(indices[i])) {
                    // to_decode.push_back(indices[i]);
                    decoding_instruction(f,indices[i]);
                }
            }
            en_decoded_instructions.push_back(this);
        }

        void check_array(LA::Function *f) {
            int num_dim = indices.size();
            if (f->var_type_map.find(remove_percent(array_name)) == f->var_type_map.end()
                || f->arr_dim_map[remove_percent(array_name)] == 0) {
                array_check_instructions.push_back(
                    new Call_Instruction("array-error",std::vector<std::string> (2,"0")));
            }
            else if (f->arr_dim_map[remove_percent(array_name)] != num_dim) {
                std::vector<std::string> targs;
                targs.push_back(array_name);
                targs.push_back("-1");
                array_check_instructions.push_back(
                    new Call_Instruction("array-error",targs));
            }
            else if (f->var_type_map[remove_percent(array_name)] == TUPLE) {
                return;
            //     std::string len = create_var_with_suffix(f);
            //         array_check_instructions.push_back(
            //             new Length_Assign_Instruction(len,array_name,"0"));
                    
            //     std::string cmp = create_var_with_suffix(f);
            //     array_check_instructions.push_back(
            //         new Op_Assign_Instruction(cmp,">=",indices[0],len));

            //     std::string lbl1 = ":";
            //     lbl1 += remove_percent(create_var_with_suffix(f));
            //     std::string lbl2 = ":";
            //     lbl2 += remove_percent(create_var_with_suffix(f));

            //     array_check_instructions.push_back(
            //         new Br2_Instruction(cmp,lbl1,lbl2));
                
            //     // cout << "here1" << endl;

            //     array_check_instructions.push_back(
            //         new Label_Instruction(lbl1));

            //     // cout << "here2" << endl;
            //     std::vector<std::string> targs;
            //     targs.push_back(array_name);
            //     targs.push_back(indices[0]);

            //     array_check_instructions.push_back(
            //         new Call_Instruction("array-error",targs));

            //     // cout << "here3" << endl;
            //     array_check_instructions.push_back(
            //         new Label_Instruction(lbl2));
            }
            else {

                for (int i = 0; i < num_dim; i++) {
                    std::string len = create_var_with_suffix(f);
                    array_check_instructions.push_back(
                        new Length_Assign_Instruction(len,array_name,std::to_string(i)));
                    
                    std::string cmp = create_var_with_suffix(f);
                    array_check_instructions.push_back(
                        new Op_Assign_Instruction(cmp,">=",indices[i],len));

                    // need to change, might have bug
                    std::string lbl1 = ":";
                    lbl1 += remove_percent(create_var_with_suffix(f));
                    std::string lbl2 = ":";
                    lbl2 += remove_percent(create_var_with_suffix(f));

                    array_check_instructions.push_back(
                        new Br2_Instruction(cmp,lbl1,lbl2));
                    
                    array_check_instructions.push_back(
                        new Label_Instruction(lbl1));

                    std::vector<std::string> targs;
                    targs.push_back(array_name);
                    targs.push_back(indices[i]);
                    array_check_instructions.push_back(
                        new Call_Instruction("array-error",targs));

                    array_check_instructions.push_back(
                        new Label_Instruction(lbl2));

                }
            }
        }

};

class Br1_Instruction : public Instruction {
    public:
        std::string label;

        Br1_Instruction() {}
        Br1_Instruction(std::string l) : label(l) {
            i_type = TE;
        }
        
        std::string toString() const {
            std::string s("");
            s += "br " + label;
            return s;
        }

        std::string translate_to_IR(LA::Function *f) {
            std::string s("");
            s += "br " + label + "\n";
            return s;
        }


};

class Return_Instruction : public Instruction {
    public:
        std::string toString() const {
            return "return";
        }

        std::string translate_to_IR(LA::Function *f) {
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

        std::string translate_to_IR(LA::Function *f) {
            std::string s("");
            s += "return " + remove_percent(var) + "\n";
            return s;
        }

        void encode_constant() {
            if (is_constant(var)) {
                var = std::to_string(stoi(var) * 2 + 1);
            }
        }
};

class New_Array_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::vector<std::string> args;

        New_Array_Assign_Instruction() {}

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

        void set_array_size(Function *f) {
            f->arr_dim_map[remove_percent(assign_left)] = args.size();
        }

        std::string translate_to_IR(LA::Function *f) {
            int arg_num = args.size();

            std::vector<std::string> pd_vec;
            int i = 0;
            while (i < arg_num) {
                pd_vec.push_back(create_var_with_suffix(f));
                i++;
            }

            std::string v0 = create_var_with_suffix(f);
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

            inst += v0 + " <- " + v0 + " * 2 + 1\n";
            // inst += v0 + " <- " + v0 + " + 1\n";
            inst += v0 + " <- " + v0 + " + " + std::to_string(2*(1+arg_num)+1) + "\n";
            inst += a + " <- call allocate(" + v0 + ",1)\n";
            inst += v0 + " <- " + a + " + 8\n";
            inst += "store " + v0 + " <- " + std::to_string(2*arg_num+1) + "\n";

            for (int i = 0; i < arg_num; i++) {
                inst += v0 + " <- " + a + " + " + std::to_string((i+2)*8) + "\n";
                inst += "store " + v0 + " <- " + p_vec[i] + "\n";
            }

            
            clear_temp_var_set(f);
            
            return inst;
        }

        void encode_constant() {
            for (int i = 0; i < args.size(); i++) {
                if (is_constant(args[i])) {
                    args[i] = std::to_string(stoi(args[i]) * 2 + 1);
                }
            }
        }
};

class New_Tuple_Assign_Instruction : public Instruction {
    public:
        std::string assign_left;
        std::string t;

        New_Tuple_Assign_Instruction() {}
        New_Tuple_Assign_Instruction(std::string l, std::string t) 
        : assign_left(l), t(t) {}
        New_Tuple_Assign_Instruction(Function *f, std::string l, std::string t) 
        : assign_left(l), t(t) {
            // f->tuple_length_map[remove_percent(assign_left)] = t;
            f->arr_dim_map[remove_percent(assign_left)] = 1;
        }

        std::string toString() const {
            

            std::string s("");
            s += assign_left + " <- new Tuple(" + t + ")";

            return s;
        }

        std::string translate_to_IR(LA::Function *f) {
            return remove_percent(assign_left) + " <- call allocate(" + t + ",1)\n";
        }

        void encode_constant() {
            if (is_constant(t)) {
                t = std::to_string(stoi(t) * 2 + 1);
            }
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
} // LA
