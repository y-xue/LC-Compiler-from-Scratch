#pragma once

#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <map>

using namespace std;

namespace LB {

class Instruction;

enum VAR_TYPE { INT64, TUPLE, CODE };

enum INST_TYPE { OTHER, TYPE, SCOPE, WHILE, IF, LBL, CONTINUE, BREAK };

class Scope;

class Function{
    public:
        std::string name;
        std::string return_type;
        std::vector<std::string> args;
        LB::Scope * scope;
        
        // std::vector<LB::Instruction *> instructions;
        std::set<std::string> labels;

        // std::map<std::string, VAR_TYPE> var_type_map;
        std::set<std::string> var_set;
        // std::set<std::string> temp_var_set;

        // std::map<std::string, int64_t> arr_dim_map;
        // std::map<std::string, std::string> tuple_length_map;

        int64_t var_suffix;
        int64_t label_suffix;

        std::string remove_percent(std::string s) {
        if (!s.empty() && s.at(0) == '%') {
            s = s.substr(1);
        }
            return s;
        }

        void declare_var() {
            for (int i = 0; i < args.size(); i++) {
                var_set.insert(args[++i]);
            }
        }
};

class Program{
    public:
        std::string entryPointLabel;
        std::vector<LB::Function *> functions;
        int64_t label_suffix;
};

class Instruction {
    public:
        INST_TYPE i_type;

        virtual std::string toString() const { return ""; }
        virtual std::vector<std::string> get_items() { return std::vector<std::string>(); }
        virtual void set_tempvar(std::string) {return;}
        virtual void bind(std::map<std::string, std::string>) { return; }
        virtual void bind() { return; }
        virtual void rename_var(LB::Function *, std::map<std::string, std::string> &) { return; }
        virtual void rename_var(LB::Function *) { return; }
        virtual void append_new_name_map(std::map<std::string, std::string> &) { return; }
        virtual std::vector<LB::Instruction *> remove_scope() { return std::vector<LB::Instruction *>(); }


        INST_TYPE get_inst_type() {
            return i_type;
        }

        std::string create_var_with_suffix(LB::Function *f, std::string v) {
            std::string newv = v + std::to_string(f->var_suffix++);
            while (f->var_set.find(newv) != f->var_set.end()) {
                newv = v + std::to_string(f->var_suffix++);
            }
            f->var_set.insert(newv);
            return newv;
        }

        // void clear_temp_var_set(LB::Function *f) {
        //     f->temp_var_set.clear();
        //     f->var_suffix = 0;
        // }

        std::string remove_percent(std::string s) {
            if (!s.empty() && s.at(0) == '%') {
                s = s.substr(1);
            }
            return s;
        }
};

class Scope : public Instruction {
    public:
        std::vector<LB::Instruction *> instructions;

        std::map<std::string, std::string> new_name_map;

        std::string toString() const {
            std::string s("{\n");
            for (auto i : instructions) {
                s += i->toString() + "\n";
            }
            s += "}\n";
            return s;
        }

        void rename_var(LB::Function *f) {
            // cout << "rename_var" << endl;
            for (auto i : instructions) {
                if (i->get_inst_type() == LB::TYPE) {
                    // cout << i->toString() << endl;
                    i->rename_var(f,new_name_map);
                    // for (auto p : new_name_map) {
                    //     cout << p.first << " " << p.second << endl;
                    // }
                }
                else if (i->get_inst_type() == LB::SCOPE) {
                    i->rename_var(f);
                }
                // else {
                //     cout << i->toString() << endl;
                // }
            }
        }

        void bind() {
            // cout << "bind" << endl;
            for (auto i : instructions) {
                if (i->get_inst_type() == LB::SCOPE) {
                    i->bind();
                }
                else {
                    // cout << "6666666" << endl;
                    i->bind(new_name_map);
                }
            }
        }

        void append_new_name_map(std::map<std::string, std::string> & outer_new_name_map) {
            // cout << "append_new_name_map" << endl;
            // cout << "current map: ";
            // for (auto p : new_name_map) {
            //     cout << p.first << " " << p.second << endl;
            // }
            // cout << endl;
            // cout << toString();
            // cout << "outer map: ";
            for (auto p : outer_new_name_map) {
                // cout << p.first << " " << p.second << endl;
                if (new_name_map.find(p.first) == new_name_map.end()) {
                    new_name_map[p.first] = p.second;
                }
            }
            // cout << endl;
            for (auto i : instructions) {
                if (i->get_inst_type() == LB::SCOPE) {
                    i->append_new_name_map(new_name_map);
                }
            }
        }

        std::vector<LB::Instruction *> remove_scope() {
            std::vector<LB::Instruction *> new_instructions;
            for (auto i : instructions) {
                // cout << i->toString() << endl;
                if (i->get_inst_type() == SCOPE) {
                    for (auto si : i->remove_scope()) {
                        // cout << si << endl;
                        new_instructions.push_back(si);
                    }
                    
                }
                else {
                    new_instructions.push_back(i);
                }
            }
            return new_instructions;
        }

};

class Type_Instruction : public Instruction {
    public:
        std::string type;
        std::vector<std::string> vars;

        Type_Instruction() {}
        // Type_Instruction(std::string v) : var(v) {}
        // Type_Instruction(LB::Scope *s, std::string t, std::string v) {
        //     var = v;
        //     type = t;
        //     v = remove_percent(var);
        //     if (type == "tuple") {
        //         s->var_type_map[v] = LB::TUPLE;
        //         s->arr_dim_map[v] = 0;
        //         // cout << "cool1" << endl;
        //     }
        //     else if (type == "code") {
        //         s->var_type_map[v] = LB::CODE;
        //     }
        //     else {
        //         s->var_type_map[v] = LB::INT64;
        //         s->arr_dim_map[v] = 0;
        //     }
        //     s->var_set.insert(v);
        // }

        std::string toString() const {
            std::string s("");
            for (auto v : vars) {
                s += type + " " + v + "\n";
            }
            return s;
        }

        void rename_var(LB::Function *f, std::map<std::string, std::string> & new_name_map) {
            // cout << "9999999999" << endl;
            // cout << "var_set:" << endl;
            // for (auto v : f->var_set) {
            //   cout << v << " ";
            // }
            // cout << endl;
            for (int k = 0; k < vars.size(); k++) {
                // cout << vars[k] << endl;
                if (f->var_set.find(vars[k]) != f->var_set.end()) {
                    // cout << "found 111111111111111111111111111111" << endl;
                    std::string new_var_name = create_var_with_suffix(f,vars[k]);
                    new_name_map[vars[k]] = new_var_name;
                    vars[k] = new_var_name;
                }
                else {
                    // cout << "not found 111111111111111111111111111111" << endl;
                    f->var_set.insert(vars[k]);
                    new_name_map[vars[k]] = vars[k];
                }
            }
        }

        // void declare_var(LB::Function *f) {
        //     for (auto v : vars) {
        //         if (f->var_set.find(v) != f->var_set.end()) {
        //             rename_var(f, )
        //         }
        //         else {
        //             f->var_set.insert(v);
        //         }
        //     }
        // }

        void bind(std::map<std::string, std::string> new_name_map) {
            return;
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
            s += assign_left + " <- " + assign_right + "\n";
            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(assign_left) != new_name_map.end()) {
                assign_left = new_name_map[assign_left];
            }
            if (new_name_map.find(assign_right) != new_name_map.end()) {
                assign_right = new_name_map[assign_right];
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
            s += assign_left + " <- " + op_left + " " + op + " " + op_right + "\n";
            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(assign_left) != new_name_map.end()) {
                assign_left = new_name_map[assign_left];
            }
            if (new_name_map.find(op_left) != new_name_map.end()) {
                op_left = new_name_map[op_left];
            }
            if (new_name_map.find(op_right) != new_name_map.end()) {
                op_right = new_name_map[op_right];
            }
        }
};

class If_Instruction : public Instruction {
    public:
        std::string op;
        std::string op_left;
        std::string op_right;
        std::string label1;
        std::string label2;
        std::string tempvar;

        std::string toString() const {
            std::string s("");
            s += "br " + tempvar + " " + label1 + " " + label2 + "\n";
            // s += "if (" + op_left + " " + op + " " + op_right + ") " + label1 + " " + label2  + "\n";
            return s;
        }

        void set_tempvar(std::string v) {
            tempvar = v;
        }

        std::vector<std::string> get_items() {
            std::vector<std::string> items;
            items.push_back(op_left);
            items.push_back(op);
            items.push_back(op_right);
            items.push_back(label1);
            items.push_back(label2);
            return items;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(op_left) != new_name_map.end()) {
                op_left = new_name_map[op_left];
            }
            if (new_name_map.find(op_right) != new_name_map.end()) {
                op_right = new_name_map[op_right];
            }
        }
};

class While_Instruction : public Instruction {
    public:
        std::string op;
        std::string op_left;
        std::string op_right;
        std::string label1;
        std::string label2;
        std::string tempvar;

        std::string toString() const {
            std::string s("");
            // s += "while (" + op_left + " " + op + " " + op_right + ") " + label1 + " " + label2;
            s += "br " + tempvar + " " + label1 + " " + label2 + "\n";
            return s;
        }

        void set_tempvar(std::string v) {
            tempvar = v;
        }

        std::vector<std::string> get_items() {
            std::vector<std::string> items;
            items.push_back(op_left);
            items.push_back(op);
            items.push_back(op_right);
            items.push_back(label1);
            items.push_back(label2);
            return items;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(op_left) != new_name_map.end()) {
                op_left = new_name_map[op_left];
            }
            if (new_name_map.find(op_right) != new_name_map.end()) {
                op_right = new_name_map[op_right];
            }
        }
};

class Continue_Instruction : public Instruction {
    public:
        std::string tempvar;

        std::string toString() const {
            return std::string("br ") + tempvar + "\n";
        }

        void set_tempvar(std::string v) {
            tempvar = v;
        }
};

class Break_Instruction : public Instruction {
    public:
        std::string tempvar;

        std::string toString() const {
            return std::string("br ") + tempvar + "\n";
        }

        void set_tempvar(std::string v) {
            tempvar = v;
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
            s += assign_left + " <- length " + var + " " + t + "\n";
            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(assign_left) != new_name_map.end()) {
                assign_left = new_name_map[assign_left];
            }
            if (new_name_map.find(var) != new_name_map.end()) {
                var = new_name_map[var];
            }
            if (new_name_map.find(t) != new_name_map.end()) {
                t = new_name_map[t];
            }
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
            s += assign_left + " <- call " + callee + " (" + oss.str() + ")" + "\n";
            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(assign_left) != new_name_map.end()) {
                assign_left = new_name_map[assign_left];
            }
            if (new_name_map.find(callee) != new_name_map.end()) {
                callee = new_name_map[callee];
            }

            for (int i = 0; i < args.size(); i++) {
                if (new_name_map.find(args[i]) != new_name_map.end()) {
                    args[i] = new_name_map[args[i]];
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
            s = s + "call " + callee + " (" + oss.str() + ")" + "\n";
            
            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(callee) != new_name_map.end()) {
                callee = new_name_map[callee];
            }

            for (int i = 0; i < args.size(); i++) {
                if (new_name_map.find(args[i]) != new_name_map.end()) {
                    args[i] = new_name_map[args[i]];
                }
            }
        }
};

class Label_Instruction : public Instruction {
    public:
        std::string label;

        Label_Instruction() {}
        // Label_Instruction(std::string l) : label(l) {
        //     i_type = LBL;
        // }

        std::string toString() const {
            return label + "\n";
        }

        std::vector<std::string> get_items() {
            std::vector<std::string> items;
            items.push_back(label);
            return items;
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
            s += assign_left + " <- " + array_name + "[" + oss.str() + "]" + "\n";
            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(assign_left) != new_name_map.end()) {
                assign_left = new_name_map[assign_left];
            }
            if (new_name_map.find(array_name) != new_name_map.end()) {
                array_name = new_name_map[array_name];
            }

            for (int i = 0; i < indices.size(); i++) {
                if (new_name_map.find(indices[i]) != new_name_map.end()) {
                    indices[i] = new_name_map[indices[i]];
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
            s += array_name + "[" + oss.str() + "]" + " <- " + assign_right + "\n";
            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(assign_right) != new_name_map.end()) {
                assign_right = new_name_map[assign_right];
            }
            if (new_name_map.find(array_name) != new_name_map.end()) {
                array_name = new_name_map[array_name];
            }

            for (int i = 0; i < indices.size(); i++) {
                if (new_name_map.find(indices[i]) != new_name_map.end()) {
                    indices[i] = new_name_map[indices[i]];
                }
            }
        }
};

class Return_Instruction : public Instruction {
    public:
        std::string toString() const {
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
            s += "return " + var + "\n";
            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(var) != new_name_map.end()) {
                var = new_name_map[var];
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
            s += assign_left + " <- new Array(" + oss.str() + ")\n";

            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(assign_left) != new_name_map.end()) {
                assign_left = new_name_map[assign_left];
            }

            for (int i = 0; i < args.size(); i++) {
                if (new_name_map.find(args[i]) != new_name_map.end()) {
                    args[i] = new_name_map[args[i]];
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
        // New_Tuple_Assign_Instruction(LB::Scope *s, std::string l, std::string t) 
        // : assign_left(l), t(t) {
        //     // f->tuple_length_map[remove_percent(assign_left)] = t;
        //     s->arr_dim_map[remove_percent(assign_left)] = 1;
        // }

        std::string toString() const {
            

            std::string s("");
            s += assign_left + " <- new Tuple(" + t + ")\n";

            return s;
        }

        void bind(std::map<std::string, std::string> new_name_map) {
            if (new_name_map.find(assign_left) != new_name_map.end()) {
                assign_left = new_name_map[assign_left];
            }
            if (new_name_map.find(t) != new_name_map.end()) {
                t = new_name_map[t];
            }
        }
};
} // LB
