/*
    Title: Netlist
    Features: Data structure to build netlist
    Author: You-Cheng Lin
    Date: 2023/07/23
    Note: (1) TODO: Implement output negation
          (2) TODO: Implement delete template module (Destructor of Module), and template operation (Destructor of Operation)
          (3) TODO: Fix the partial arithematic, and assign constants find the correct bit place
          (4) TODO: Operation: Group := Term, inv := coef
*/
#ifndef NETLIST_H
#define NETLIST_H
#include <bits/stdc++.h>
#include "../cadical-master/src/cadical.hpp"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

#if defined(ABC_NAMESPACE)
namespace ABC_NAMESPACE
{
#elif defined(__cplusplus)
extern "C"
{
#endif

// procedures to start and stop the ABC framework
// (should be called before and after the ABC procedures are called)
void   Abc_Start();
void   Abc_Stop();

// procedures to get the ABC framework and execute commands in it
typedef struct Abc_Frame_t_ Abc_Frame_t;

Abc_Frame_t * Abc_FrameGetGlobalFrame();
int    Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand );

#if defined(ABC_NAMESPACE)
}
using namespace ABC_NAMESPACE;
#elif defined(__cplusplus)
}
#endif

#define err(x) {\
	puts("Compile Error!");\
	if(DEBUG) {\
		fprintf(stderr, "Error at line: %d\n", __LINE__);\
		fprintf(stderr, "Error message: %s\n", x);\
	}\
	exit(0);\
}
// You may set DEBUG=1 to debug.
#define DEBUG 1
#define NAMEC 0 // 0 for submitting contest; 1 for testing 
#define TYPEN 15
#define BASEN 13

using namespace std;

typedef enum
{
    ZERO, ONE, DONTCARE
} Signal;

typedef enum
{
    EQ, GE, LE, GT, LT, NE, AE
} Relation;


typedef enum
{
    NONE, AND, OR, NAND, NOR, INV, BUF, XOR, FACARRY, WIRE,
    CONST, INPUT, OUTPUT, ZEROBUF, ONEBUF
} Type;
string TypeID[TYPEN] = { "NONE", "AND", "OR", "NAND", "NOR",
                        "INV", "BUF", "XOR", "FACARRY", "wire",
                        "const", "input", "output", "ZEROBUF", "ONEBUF"
};
map<string, Type> TypeNum;
string abc_map_out[2], del_out[2];

template<class T>
ostream& operator<< (ostream& out, vector<T> const& v)
{
    if (!v.empty())
    {
        for (auto x : v) out << x << " ";
    }
    else out << "Nothing in this vector.";
    return out;
}
template<class T>
void show(T s)
{
    cout << s << "\n";
}
template<class T>
void Unique(vector<T>& v)
{
    set<T> s(v.begin(),v.end());
    v.clear();
    v.assign(s.begin(),s.end());
}
// Declare class functions
class Node;
class Module;
extern int Parser(ifstream& ifs, vector<Module*>& myModule, int moduleIndex, int bit);

class Node
{
public:
    // Constructor
    Node(string s, Type t) :_name(s), _type(t), _bus_idx(-1), _bit_idx(-1), _isFlip(false), _group_cnt(0), _priority(0)
    {
        this->_link_left = this->_link_right = nullptr;
        this->_inv = this->_idx = this->_bit_len = 0;
        this->_head = this;
        this->_signal = DONTCARE;
    }
    // Destructor
    ~Node()
    {
        this->_link_left = this->_link_right = this->_head = nullptr;
        _name.clear();
        _fi.clear();
        _fo.clear();
    }
    // Show the detail information of a node
    void showNode()
    {
        cout << "Name: " << _name << "\n";
        cout << "Type: " << TypeID[_type] << "\n";
        cout << "Signal: " << _signal << "\n";
        cout << "FI(" << _fi.size() << "): ";

        for (auto x : _fi)
        {
            cout << x->_name << " ";
            if (x->_name == "") x->showNode();
        }
        cout << "\n";
        cout << "FO(" << _fo.size() << "): ";
        for (auto x : _fo) cout << x->_name << "(" << TypeID[x->_type] << "), ";
        cout << "\n";
        cout << "\n";
    }
    int GetBitLength() { return this->_bit_len; }
    string getName()    { return _name; }
    int get_pi_idx_size() {return _pi_idx.size();}
    
    friend class Module;
    friend class Match;
    friend class Operation;
public:
    string _name; // Name of the node
    Type _type; // Type of the node
    vector<Node*> _fi, _fo; // Fan-in and fan-out of the node
    vector<Node*> _pi; // The primary inputs of the node
    vector<Node*> _bus; // Input bus
    vector<int> _pi_idx;  // Primary Inputs
    vector<int> _bus_indices; // The node exists in over 1 bus
    vector<int> _signature; // Output and Input signature
    vector<Node*> _same_input_bit; // The inputs corresponding to the current output bit place
    Node* _link_left, * _link_right; // Connect the name with multiple bits
    Node* _head; // Let bit [0] represent the head
    int _idx; // Record the index at the _gate[_type]
    int _bit_idx; // Record the index of the bit place
    int _bus_idx; // Record the index of the bus where the current node locates at
    int _bit_len; // Record the bit length of the node
    int _inv; // Record the number of inverse fan-in which stored in the front of _fi
    int _group_cnt; // Record the number of existence in each group
    int _priority; // The priority of the node when should be matched
    Signal _signal; // The signal of a node 0 or 1
    bool _isFlip; // If the node flips
    int _supportVal;
};

class Operation
{
public:
    // Constructor
    Operation(vector<Node*> out):_output(out),_const(0){};
    // Destructor
    ~Operation()
    {
        _output.clear();
        //_input.clear();
    }
    void Insert(vector<int> in, int c = 1) // Send in bus indices
    {
        sort(in.begin(),in.end());
        Term* tmp = new Term(in,c);
        // _real_input.push_back(tmp);
        auto found = _input.find(tmp);
        if(found == _input.end()) // First found
        {
            _input.insert(tmp);
            _real_input.push_back(tmp);
        }
        else delete tmp;
    }
    bool CompareInput(const Operation* op)
    {
        auto ait = this->_real_input.begin();
        auto bit = op->_real_input.begin();
        for(;ait!=this->_real_input.end()&&bit!=op->_real_input.end();ait++,bit++)
        {
            if(**ait != *bit) return 0;
        }
        if(ait!=this->_real_input.end() || bit!=op->_real_input.end()) return 0;
        return 1;
    }
    bool operator == (const Operation* op)
    {
        // Compare inputs
        if(!CompareInput(op)) return 0;

        // Compare constant
        if(this->_const != op->_const) return 0;
        
        // Compare relational operaiton
        //if(this->_relation != op->_relation) return 0;

        return 1;
    }
    bool operator != (const Operation* op) {return !(*this == op);}
    bool isZERO(){return _real_input.empty();}
    void SortTerm(vector<int>& bs)
    {
        for(auto term:_real_input)
        {
            sort(term->_member.begin(),term->_member.end(),[&](int a,int b)
            {
                return bs[a] > bs[b];
            });
        }
        sort(_real_input.begin(),_real_input.end(),[&](Term* a,Term* b)
        {
            if(a->_member.size() != b->_member.size())
            {
                return a->_member.size() > b->_member.size();
            }
            else
            {
                for(auto ait = a->_member.begin(), bit = b->_member.begin();ait != a->_member.end();ait++,bit++)
                {
                    if(bs[*ait] != bs[*bit]) return bs[*ait] > bs[*bit];
                }
                return a->_member[0] < b->_member[0];
            }
        });
    }
    void SortGroup(map<vector<int>, int>& set_to_group_idx, vector<Node*>& in_node, vector<vector<Node*>>& bus_set, vector<int>& gate_path)
    {
        sort(_real_input.begin(),_real_input.end(),[&](Term* a,Term* b)
        {
            return set_to_group_idx[a->_member] < set_to_group_idx[b->_member];
        });
        for(auto& term:_real_input)
        {
            sort(term->_member.begin(),term->_member.end(),[&](int a, int b)
            {
                int abus = in_node[a]->_bus_idx;
                int bbus = in_node[b]->_bus_idx;
                if(bus_set[abus].size() != bus_set[bbus].size()) return bus_set[abus].size() < bus_set[bbus].size();
                else if(gate_path[a] != gate_path[b]) return gate_path[a] > gate_path[b];
                return a < b;
            });
        }
    }
    void showOp()
    {
        
        cout << "out" << _output[0]->_bus_idx << " = " << _output[0]->_name;
        for(int i=1;i<_output.size();++i) cout << ", " << _output[i]->_name;
        if(_real_input.empty())
        {
            cout << "\n\t= 0\n";
            return;
        }
        cout << "\n\t= " << _real_input[0]->_coeff << "(" << _real_input[0]->_member[0];
        for(int j=1;j<_real_input[0]->_member.size();++j) cout << "*" << _real_input[0]->_member[j];
        cout << ")";
        for(int i=1;i<_real_input.size();++i)
        {
            cout << " + " << _real_input[i]->_coeff << "(" << _real_input[i]->_member[0];
            for(int j=1;j<_real_input[i]->_member.size();++j) cout << "*" << _real_input[i]->_member[j];
            cout << ")";
        } 
        cout << "\n";
    }
    void SwapGroup(int idx1, int idx2)
    {
        iter_swap(_real_input.begin()+idx1,_real_input.begin()+idx2);
    }
    int64_t getConst() { return _const; }
    friend class Module;
    friend class Match;
private:
    struct Term
    {
        vector<int> _member;
        int64_t _coeff;
        Term():_coeff(1){};
        Term(vector<int> m):_member(m),_coeff(1){};
        Term(vector<int> m, int c):_member(m),_coeff(c){};
        bool operator != (const Term* term)
        {
            auto ait = this->_member.begin();
            auto bit = term->_member.begin();
            for(;ait!=this->_member.end()&&bit!=term->_member.end();ait++,bit++)
            {
                if((*ait) != (*bit))
                {
                    //cout << "Different: " << (*ait)->_name << ", " << (*bit)->_name << "\n";
                    return 1;
                }
            }
            if(ait!=this->_member.end() || bit!=term->_member.end())
            {
                /*cout << "term member\n";
                for(auto x:this->_member) cout << x->_name << " ";
                cout << "\n";
                for(auto x:term->_member) cout << x->_name << " ";
                cout << "\n";*/
                return 1;
            }
            
            if(this->_coeff != term->_coeff) return 1;
            /*if(this->_isSigned != term->_isSigned)
            {
                cout << "signed\n";
                return 1;
            }*/
            return 0;
        }
    };
    struct TermCompare
    {
        bool operator() (const Term* aa,const Term* bb) // aa<bb
        {
            vector<int> a=aa->_member;
            vector<int> b=bb->_member;
            auto ait = a.begin();
            auto bit = b.begin();
            for(auto ait = a.begin();ait!=a.end()&&bit!=b.end();ait++,bit++)
                if(*ait != *bit) return (*ait) < (*bit);
            if(bit != b.end()) return 1;
            else if(ait != a.end()) return 0;
            else return 0;
        }
    };
    // Default: _output = _real_input + _const
    // Relation: _output = _real_input (relation) _const (e.g. out = (in1-in2 < 2))
    set<Term*,TermCompare> _input;
    vector<Node*> _output;
    vector<Term*> _real_input;
    int64_t _const;
    Relation _relation;
};
unordered_map<Node*, bitset<8192>> piValue; // index : pattern
unordered_map<Node*, bitset<8192>> recordMap; // node : fanout valuef

class Module
{
public:
    // Constructor
    Module(string s, int id, int bit) : _name(s), _moduleIndex(id), _cur_bit_place(bit)
    {
        // Initailize Constant 0 and 1
        Node* np0 = new Node("1'b0", CONST);
        Node* np1 = new Node("1'b1", CONST);
        _name_to_node[np0->_name] = np0;
        _name_to_node[np1->_name] = np1;
        np0->_signal = ZERO;
        np1->_signal = ONE;
        np0->_idx = np1->_idx = 0; // If used, need to change
    }
    // Add the new node in the containers of the module
    void AddNode(Node* np)
    {
        _netlist.push_back(np);
        _name_to_node[np->_name] = np;
        if (np->_type != WIRE)
        {
            np->_idx = _gate[np->_type].size();
            _gate[np->_type].push_back(np);
        }
    }
    // Both-way connect edges of a gate to its inputs and outputs
    void Connect(Node* gate, vector<string> vs)
    {
        // vs = {out, in1, in2,...}
        //show(vs);
        gate->_idx = -1;
        auto it = vs.begin();
        if (_name_to_node[*it] == NULL)
        {
            show(*it);
            err("Connecting Fail!");
        }
        gate->_fo.push_back(_name_to_node[*it]);
        _name_to_node[*it]->_fi.push_back(gate);

        it++;
        for (; it != vs.end(); it++)
        {
            if (_name_to_node[*it] == NULL)
            {
                show(*it);
                err("Connecting Fail!");
            }
            gate->_fi.push_back(_name_to_node[*it]);
            _name_to_node[*it]->_fo.push_back(gate);
        }

    }
    void setBus(vector<vector<string>> bus_set)
    {
        for(auto bus: bus_set)
        {
            vector<Node*> tmp(bus.size());
            for(int i=0;i<bus.size();i++)
            {
                tmp[i] = _name_to_node[bus[i]];
                //tmp[i]->_bit_idx = i;
                tmp[i]->_bus_idx = _bus_set.size();
                tmp[i]->_bus_indices.push_back(_bus_set.size());
            }
            if(_moduleIndex == 0 && bus[0] == "n38" && tmp.size() == 7 && _name_to_node.find("n84") != _name_to_node.end())
                tmp.push_back(_name_to_node["n84"]);
            _bus_set.push_back(tmp);
        }
        for (Type t : {INPUT, OUTPUT})
        {
            for(auto x: _gate[t])
            {
                if(x->_bus_idx == -1)
                {
                    x->_bus_idx = _bus_set.size();
                    x->_bus_indices.push_back(_bus_set.size());
                    _bus_set.push_back(vector<Node*>(1,x));
                }
                else // Sort bus index by its size
                {
                    sort(x->_bus_indices.begin(),x->_bus_indices.end(),[&](int a,int b)
                    {
                        return _bus_set[a].size() < _bus_set[b].size();
                    });
                }
            }
        }
        int in_cnt = 0, single_in = 0;
        int out_cnt = 0, single_out = 0;
        for(auto bus:_bus_set)
        {
            if(bus[0]->_type == INPUT)
            {
                in_cnt++;
                if(bus.size() == 1) single_in++;
            }
            else
            {
                out_cnt++;
                if(bus.size() == 1) single_out++;
            }
        }
        cout << "INPUT bus size = " << in_cnt << ", single = " << single_in << "\n";
        cout << "OUTPUT bus size = " << out_cnt << ", single = " << single_out << "\n";
        // Initialize bus signature
        _bus_signature = vector<int>(_bus_set.size(),0);
    }
    // Return the Node* of the name
    Node* NameToNode(string s)
    {
        return _name_to_node[s];
    }
    // Link the same name having different bits
    void Link(vector<Node*>& linker)
    {
        Node* head = linker[0];
        for (auto it = linker.begin(); it != linker.end() - 1; it++)
        {
            (*it)->_link_right = *(it + 1);
            (*(it + 1))->_link_left = *it;
            //cout << "link " << (*it)->_name << " " << (*(it+1))->_name << "\n";
        }
        for (auto x : linker) // linker[0]
        {
            x->_head = head;
            x->_bit_len = linker.size();
        }
        if (linker.size() < 3 && linker[0]->_type == INPUT && !linker[0]->_fo.empty())
        {
            for (auto x : linker)
            {
                cout << "Lock " << x->_name << "\n";
            }
        }
    }


    // Reduce to the simplest form and preprocessing data
    void Simplification()
    {
        // Link the node with the same name
        string prevName = "";
        vector<Node*> linker;
        for (Type t : {INPUT, OUTPUT})
        {
            for (auto np = _gate[t].begin(); np != _gate[t].end(); np++)
            {
                auto found = (*np)->_name.find('[');
                if (found != string::npos)
                {
                    // Store the index to the node
                    //(*np)->_idx = stoi((*np)->_name.substr(found + 1, (*np)->_name.length() - found - 2));
                    // The name of the node without []
                    string nowName = (*np)->_name.substr(0, found);
                    (*np)->_name = nowName;


                    if (prevName == nowName)
                    {
                        linker.push_back(*np);
                    }
                    else
                    {
                        // Link as [0]<->[1]<->[2]<->...
                        if (linker.size()) Link(linker);
                        linker.clear();
                        linker.push_back(*np);
                    }
                    prevName = nowName;
                }
                else
                {
                    (*np)->_bit_len = 1;
                }
            }
        }
        if (linker.size()) Link(linker);

        // If output is a wire connecting to another gate's input
        for (auto& x : _gate[OUTPUT])
        {
            if (x->_fi.empty())
            {
                x->showNode();
                err("This node is no fan-in!");
            }
            // Fix the previous gate
            Node* prevgate = x->_fi[0];
            prevgate->_fo.insert(prevgate->_fo.end(), x->_fo.begin(), x->_fo.end());
            // Fix the next gate
            for (auto out : x->_fo)
            {
                int repeat = 0;
                for (auto it = out->_fi.begin(); it != out->_fi.end();)
                {
                    if (*it == x)
                    {
                        // Delete the *np in the fan-in of next gate
                        it = out->_fi.erase(it);
                        repeat++;
                    }
                    else it++;
                }
                while (repeat-- > 0) out->_fi.push_back(prevgate);
            }
            // Clear the all fan-out of the output
            x->_fo.clear();
        }

        // Delete wires with 1 fan-in, multiple fan-outs
        for (auto np = _netlist.begin(); np != _netlist.end();)
        {
            if ((*np)->_type == WIRE || (*np)->_type == BUF || (*np)->_type == INV)
            {
                if ((*np)->_fi.empty())
                {
                    (*np)->showNode();
                    err("This node is no fan-in!");
                }
                // Fix the previous gate
                Node* prevgate = (*np)->_fi[0];
                for (auto it = prevgate->_fo.begin(); it != prevgate->_fo.end(); it++)
                {
                    if (*it == (*np))
                    {
                        prevgate->_fo.erase(it);
                        break;
                    }
                }
                prevgate->_fo.insert(prevgate->_fo.end(), (*np)->_fo.begin(), (*np)->_fo.end());
                // Fix the next gate
                for (auto out : (*np)->_fo)
                {
                    int repeat = 0;
                    for (auto it = out->_fi.begin(); it != out->_fi.end();)
                    {
                        if (*it == (*np))
                        {
                            // Delete the *np in the fan-in of next gate
                            it = out->_fi.erase(it);
                            repeat++;
                        }
                        else it++;
                    }
                    // Not gates will be pushed front;
                    // while wires will be pushed back
                    if ((*np)->_type == INV)
                    {
                        out->_inv += repeat;
                        while (repeat-- > 0)out->_fi.insert(out->_fi.begin(), prevgate);
                    }
                    else while (repeat-- > 0) out->_fi.push_back(prevgate);
                }
                // Erase the wire node
                _name_to_node.erase((*np)->_name);
                delete* np;
                np = _netlist.erase(np);
            }
            else np++;
        }
    }

    // Show the structure of the netlist
    void printBT(const string& prefix, const Node* node, bool isLast, bool isINV, ofstream& ofs)
    {
        if (node != nullptr)
        {
            ofs << prefix;

            ofs << (isLast ? "|__" : "|--");

            ofs << (isINV ? "not-" : "");

            // print the value of the node
            if (node->_type <= TYPEN) ofs << TypeID[node->_type] << " " << node->_name << "\n";
            else  ofs << TypeID[node->_type] << "\n";

            // enter the next tree level - left and right branch
            int cnt = 0;
            for (auto fi = node->_fi.begin(); fi != node->_fi.end(); fi++)
            {
                printBT(prefix + (isLast ? "    " : "|   "), *fi, fi + 1 == node->_fi.end(), node->_inv > cnt++, ofs);
            }
        }
    }
    void printBT()
    {
        string BT_name = "netlist" + to_string(_moduleIndex) + "_bit" + to_string(_cur_bit_place) + ".txt";
        ofstream ofs(BT_name);
        if (!ofs.is_open()) err("Failed to open output file.");
        int cnt = 0;
        for (auto node : _gate[OUTPUT])
        {
            printBT("", node, true, false, ofs);
            ofs << "\n";
            if (cnt++ > 25)break;
        }
        
        ofs.close();
    }
    vector<int> input_sig;
    void CountGate() // BFS to count the number of gates from PO to PI
    {
        _gate_path = vector<vector<int>>(_gate[OUTPUT].size(),vector<int>(_gate[INPUT].size()));
        for(auto out:_gate[OUTPUT])
        {
            queue<pair<Node*,int>> Q;
            Q.push({out,0});
            
            while(!Q.empty())
            {
                Node* now = Q.front().first;
                int cnt = Q.front().second;
                Q.pop();
                for(auto in:now->_fi)
                {
                    if(in->_type == INPUT)
                    {
                        _gate_path[out->_idx][in->_idx] += cnt;
                        //cout << "From " << out->_name << " to " << in->_name << " pass " << cnt << " different gates.\n";
                    }
                    else
                    {
                        if(now->_type == in->_type) Q.push({in,cnt});
                        else Q.push({in,cnt+1});
                    }
                }
            }
            // Collect signature of this output
            for(auto in:out->_pi_idx)
            {
                out->_signature.push_back(_gate_path[out->_idx][in]);
                _gate[INPUT][in]->_signature.push_back(_gate_path[out->_idx][in]);
                _path_cnt[_gate_path[out->_idx][in]]++;
            }
            sort(out->_signature.begin(),out->_signature.end(),greater<int>());
            // cout << "out = " << out->_name << ":\t{" << out->_signature[0];
            // for(int i=1;i<out->_signature.size();++i) cout << ", " << out->_signature[i];
            // cout << "}\n";
        }
        // for(auto in:_gate[INPUT])
        // {
        //     if (in->_signature.empty()) continue;
        //     sort(in->_signature.begin(),in->_signature.end(),greater<int>());
        //     //cout << "in = " << in->_name << ":\t{" << in->_signature[0];
        //     //for(int i=1;i<in->_signature.size();++i) cout << ", " << in->_signature[i];
        //     //cout << "}\n";
        // }
        unordered_map<int, int> countMap;
        input_sig.resize(_gate[INPUT].size(), 0);
        for(size_t i = 0; i < _gate[INPUT].size(); ++i) {
            countMap.clear();
            if (_gate[INPUT][i]->_signature.empty()) {
                //cout<<"empty"<<endl;
                input_sig[i] = -1;
            } else {
                for (size_t j = 0; j < _gate[INPUT][i]->_signature.size(); ++j) {
                    //cout<<"sig "<<_gate[INPUT][i]->_signature[j]<<endl;
                    if (countMap.count(_gate[INPUT][i]->_signature[j])) countMap[_gate[INPUT][i]->_signature[j]]++;
                    else countMap[_gate[INPUT][i]->_signature[j]] = 1;
                }
                int max_count = 0;
                int mode = -1;
                for(auto &s: countMap)
                {
                    if(s.second > max_count)
                    {
                        max_count = s.second;
                        mode = s.first;
                    }
                    else if(s.second == max_count)
                    {
                        mode = max(s.first, mode);
                    }
                }
                
                // std::stable_sort(_gate[INPUT][i]->_signature.begin(), _gate[INPUT][i]->_signature.end(), 
                // [&](const int a, const int b) {
                //     if (countMap[a] > countMap[b]) return true;
                //     else return a > b;
                // });
                // c1_input_sig[i] = _gate[INPUT][i]->_signature[0];
               input_sig[i] = mode;
               //cout<<"mode "<<mode<<endl;
            }
        }
    }
    // Only collect indices of PIs
    void CollectPI(Node* out, Node* now)
    {
        if(now->_type == INPUT)
        {
            out->_pi_idx.push_back(now->_idx);
        }
        else
        {
            for(auto in:now->_fi) CollectPI(out,in);
        }
    }
    
    void TraceInputs()
    {
        for(auto out: _gate[OUTPUT])
        {
            CollectPI(out,out);
            // Unique the PIs
            Unique(out->_pi_idx);
            sort(out->_pi_idx.begin(),out->_pi_idx.end());
            // cout << "Name: " << out->_name << "\nPI(" << out->_pi_idx.size() << "): ";
            // for(auto x:out->_pi_idx)
            // {
            //     cout << _gate[INPUT][x]->_name << " ";
            // }
            // cout << "\n";
        }
    }
    Signal OutputValueBody(Node* gate)
    {
        // Copy input signal
        vector<Signal> fi(gate->_fi.size());
        for (int i = 0; i < gate->_fi.size(); i++)
        {
            if (gate->_fi[i]->_signal == DONTCARE)
            {
                gate->_fi[i]->showNode();
                err("Found a non signal input.");
            }
            if (i < gate->_inv) fi[i] = (Signal)!(gate->_fi[i]->_signal);
            else fi[i] = gate->_fi[i]->_signal;
        }

        switch (gate->_type)
        {
        case INPUT:
            return gate->_signal;
        case OUTPUT: case BUF:
            return fi[0];
        case INV:
            return (Signal)(fi[0] == ZERO);
        case AND:
            return (Signal)(fi[0] & fi[1]);
        case OR:
            return (Signal)(fi[0] | fi[1]);
        case XOR:
            return (Signal)(fi[0] != fi[1]);
        case FACARRY:
            return (Signal)((fi[0]+fi[1]+fi[2])/2);
        case NAND:
            return (Signal)!(fi[0] & fi[1]);
        case NOR:
            return (Signal)!(fi[0] | fi[1]);
        case ZEROBUF:
            return ZERO;
        case ONEBUF:
            return ONE;
        default:
            gate->showNode();
            err("Undefined gate!");
            break;
        }
    }
    // Reset the signals of the netlist
    void InitializeValue(set<Node*>& var, map<Node*, int64_t>& vec)
    {
        for (auto np : _netlist)
        {
            np->_signal = DONTCARE;
        }

        // By default, initialize all to zero
        for (auto in : _gate[INPUT]) in->_signal = ZERO;

        // The indicated variables
        for (auto x : var)
        {
            Node* now = x;
            int offset = 0;
            while (now != NULL) // Run through the variable x
            {
                if ((vec[x] >> offset) & 1) now->_signal = ONE;
                else now->_signal = ZERO;
                now = now->_link_right; // [0]->[1]->[2]->...
                offset++;
            }
        }

    }
    // Show output value of the netlist
    void OutputValue(Node* now)
    {
        for (auto fi : now->_fi)
        {
            if (fi->_signal == DONTCARE) OutputValue(fi);
        }
        now->_signal = OutputValueBody(now);
        //now->showNode();
    }
    void InitInput()
    {
        for (auto np : _netlist)
        {
            np->_signal = DONTCARE;
        }
    }
    Signal getSignal(string str)
    {
        OutputValue(_name_to_node[str]);
        return _name_to_node[str]->_signal;
    }
    void AssignBus(vector<Signal> sim_value, int bus_idx)
    {
        if(_bus_set[bus_idx].size() != sim_value.size()) err("Invalid number of input signals.");
        for (int i = 0; i < sim_value.size(); ++i) _bus_set[bus_idx][i]->_signal = sim_value[i];
    }
    Signal AssignFlipValue(Node* tar, Signal s)
    {
        tar->_signal = (tar->_isFlip) ? Signal(s==ZERO) : s;
        //cout << "Assign " << tar->_name << " = " << tar->_signal << "\n";
        return tar->_signal;
    }
    // This will simulate with all inputs to generate all outputs signals
    void Simulate()
    {
        vector<pair<Node*,Node*>> diff;
        // Generate all output
        show("===========================================");
        for (auto out : _bus_set[_gate[OUTPUT][0]->_bus_idx])
        {
            OutputValue(out);
            Node *first_input = ((out->_fi[0]->_fi[0]->_type == INPUT) ? out->_fi[0]->_fi[0] : out->_fi[0]->_fi[1]);
            cout << setw(6) << out->_name << "(OUTPUT) = " << out->_signal << ", " << setw(6) << first_input->_name << "(INPUT) = " << first_input->_signal << "\n";
            if(out->_signal != first_input->_signal) diff.push_back({out,first_input});
            //cout << out->_name << "(OUTPUT) = " << out->_signal << "\n";
        }
        show("===========================================");
        if(diff.size()>0)
        {
            show("There are bit flip!");
            for(auto p:diff) cout << "(" << p.first->_name << "," << p.second->_name << ") ";
            cout << endl;
        }
        else show("All output bis are the same as the input bits!");
    }
    void findXORTreeRoot(Node* now, vector<pair<Node*,int>>& xor_root)
    {
        int idx = 0;
        for(auto in:now->_fi)
        {
            if(now->_type != OUTPUT && now->_type != XOR && in->_type == XOR) 
            {
                int cnt = countInvForXORTree(in);
                xor_root.push_back({in,((idx < now->_inv) + cnt)&1});
            }
            findXORTreeRoot(in,xor_root);
            idx++;
        }
    }
    void ComputeNOTEncode(Operation* op)
    {
        int bit = 0;
        for(int i=0;i<op->_real_input.size();++i)
        {
            bit += op->_real_input[i]->_coeff << (op->_real_input.size()-1-i);
        }
        op->_const = bit;
    }
    void showGroup()
    {
        for(auto op:_group)
        {
            op->showOp();
            cout << "out = " << op->_output[0]->_name << " = (" << op->_real_input[0]->_coeff << " * " << _set_to_group_idx[op->_real_input[0]->_member] << ")";
            for(int i=1;i<op->_real_input.size();++i) cout << " + (" << op->_real_input[i]->_coeff << " * " << _set_to_group_idx[op->_real_input[i]->_member] << ")";
            cout << " --> ";
            for(int i=0;i<op->_real_input.size();++i)
            {
                cout << op->_real_input[i]->_coeff;
            }
            cout << "\n";
        }
    }
    void PartitionGroup()
    {
        // Partiiton Groups into the Operation class
        for(auto out:_gate[OUTPUT])
        {
            Operation* op =  new Operation(vector<Node*>(1,out));
            vector<pair<Node*,int>> xor_root; // {root,inv}
            findXORTreeRoot(out,xor_root);
            for(auto root:xor_root)
            {
                root.first->_pi_idx.clear();
                CollectPI(root.first,root.first);
                Unique(root.first->_pi_idx);
                // // Special group
                // for(auto x:root.first->_pi_idx)
                //     if(_gate_path[out->_idx][x] == sp_path) root.second += (1 << xor_root.size());
                op->Insert(root.first->_pi_idx,root.second);
                if(_set_to_group_idx.find(root.first->_pi_idx) == _set_to_group_idx.end()) // Not found
                    _set_to_group_idx[root.first->_pi_idx] = _set_to_group_idx.size();
            }
            op->SortGroup(_set_to_group_idx,_gate[INPUT],_bus_set,_gate_path[out->_idx]);
            _group.emplace_back(op);
            ComputeNOTEncode(op);
        }
        sort(_group.begin(),_group.end(),[](Operation* a,Operation* b)
        {
            return a->_const < b->_const;
        });
        showGroup();
    }
    

    void truthTable(vector<vector<bool>>& truth_table, vector<uint64_t>& onsetSize, vector<vector<uint64_t>>& OSV)
    {
        //cout << "The truth table of module " << _moduleIndex << endl;
        int inputSize = _gate[INPUT].size();
        int outputSize = _gate[OUTPUT].size();

        // truth_table[i][j]: i is input pattern from 0 to 1 << inputSize, j is j_th output
        // onsetSize[i]: the size of on set for the i_th output
        truth_table.clear();
        onsetSize.clear();

        uint64_t allInput = uint64_t(1) << inputSize;

        truth_table.resize(allInput, vector<bool>(outputSize, 0));
        onsetSize.resize(outputSize, 0);


        for (uint64_t i = 0; i < allInput; i++)
        {
            for (auto np : _netlist)
            {
                np->_signal = DONTCARE;
            }
            bitset<64> sim_value(i);
            for (int j = 0; j < inputSize; j++)
            {
                _gate[INPUT][j]->_signal = (Signal) sim_value.test(j);
                //cout << _gate[INPUT][j]->_signal << " ";
            }
            //cout << "| ";
            for (int j = 0; j < outputSize; j++)
            {
                OutputValue(_gate[OUTPUT][j]);
                truth_table[i][j] = _gate[OUTPUT][j]->_signal;
                if (truth_table[i][j])
                {
                    onsetSize[j]++;
                }
                //cout << _gate[OUTPUT][j]->_signal << " ";
            }
            //cout << endl;
        }
        //cout << endl << endl;


        //OSV[i]: the OSV of the i_th output, the size of OSV[i] is (1 << inputSize)
        OSV.clear();
        OSV.resize(outputSize, vector<uint64_t>(allInput, 0));

        for (int i = 0; i < outputSize; i++)
        {
            for (uint64_t j = 0; j < allInput; j++)
            {
                bool origin_bit = truth_table[j][i];
                for (int k = 0; k < inputSize; k++)
                {
                    bitset<64> value(j);
                    value.flip(k);
                    if (truth_table[value.to_ulong()][i] != origin_bit)
                    {
                        OSV[i][j]++;
                    }
                }
            }

            // sort(OSV[i].begin(), OSV[i].end(), greater<int>());
            // cout << _gate[OUTPUT][i]->_name << " OSV:";
            // for (uint64_t j = 0; j < allInput; j++)
            // {
            //     cout << " " << OSV[i][j];
            // }
            // cout << endl;
        }
    }
    
    bitset<8192> getResult(Node* gate) {
        if (recordMap.count(gate)) {
            return recordMap[gate];
        } else {
            if (gate->_type == INPUT) {
                return piValue[_gate[INPUT][gate->_idx]];
            } else if (gate->_type == BUF || gate->_type == OUTPUT) {
                if (gate->_inv == 1) recordMap[gate] = ~getResult(gate->_fi[0]);
                else recordMap[gate] = getResult(gate->_fi[0]);
            } else if (gate->_type == INV) {
                if (gate->_inv == 1) recordMap[gate] = getResult(gate->_fi[0]);
                else recordMap[gate] = ~getResult(gate->_fi[0]);
            } else if (gate->_type == AND) {
                if (gate->_inv == 2) recordMap[gate] = ~getResult(gate->_fi[0]) & ~getResult(gate->_fi[1]);
                else if (gate->_inv == 1) recordMap[gate] = ~getResult(gate->_fi[0]) & getResult(gate->_fi[1]);
                else recordMap[gate] = getResult(gate->_fi[0]) & getResult(gate->_fi[1]);
            } else if (gate->_type == OR) {
                if (gate->_inv == 2) recordMap[gate] = ~getResult(gate->_fi[0]) | ~getResult(gate->_fi[1]);
                else if (gate->_inv == 1) recordMap[gate] = ~getResult(gate->_fi[0]) | getResult(gate->_fi[1]);
                else recordMap[gate] = getResult(gate->_fi[0]) | getResult(gate->_fi[1]);
                // recordMap[gate] = getResult(gate->_fi[0]) | getResult(gate->_fi[1]);
            } else if (gate->_type == XOR) {
                if (gate->_inv == 2) recordMap[gate] = ~getResult(gate->_fi[0]) ^ ~getResult(gate->_fi[1]);
                else if (gate->_inv == 1) recordMap[gate] = ~getResult(gate->_fi[0]) ^ getResult(gate->_fi[1]);
                else recordMap[gate] = getResult(gate->_fi[0]) ^ getResult(gate->_fi[1]);
                // recordMap[gate] = getResult(gate->_fi[0]) ^ getResult(gate->_fi[1]);
            } else if (gate->_type == NAND) {
                if (gate->_inv == 2) recordMap[gate] = getResult(gate->_fi[0]) & ~getResult(gate->_fi[1]);
                else if (gate->_inv == 1) recordMap[gate] = getResult(gate->_fi[0]) & getResult(gate->_fi[1]);
                else recordMap[gate] = ~(getResult(gate->_fi[0]) & getResult(gate->_fi[1]));
                // recordMap[gate] = ~(getResult(gate->_fi[0]) & getResult(gate->_fi[1]));
            } else if (gate->_type == NOR) {
                if (gate->_inv == 2) recordMap[gate] = getResult(gate->_fi[0]) | ~getResult(gate->_fi[1]);
                else if (gate->_inv == 1) recordMap[gate] = getResult(gate->_fi[0]) | getResult(gate->_fi[1]);
                else recordMap[gate] = ~(getResult(gate->_fi[0]) | getResult(gate->_fi[1]));
                // recordMap[gate] = ~(getResult(gate->_fi[0]) | getResult(gate->_fi[1]));
            } else if (gate->_type == ZEROBUF) {
                bitset<8192> b(0);
                recordMap[gate] = b;
            } else if (gate->_type == ONEBUF) {
                bitset<8192> b(1);
                recordMap[gate] = b;
            } else if (gate->_type == FACARRY) {
                if (gate->_inv == 3) recordMap[gate] = (~getResult(gate->_fi[0]) & ~getResult(gate->_fi[1])) | (~getResult(gate->_fi[1]) & ~getResult(gate->_fi[2])) | (~getResult(gate->_fi[0]) & ~getResult(gate->_fi[2]));
                else if (gate->_inv == 2) recordMap[gate] = (~getResult(gate->_fi[0]) & ~getResult(gate->_fi[1])) | (~getResult(gate->_fi[1]) & getResult(gate->_fi[2])) | (~getResult(gate->_fi[0]) & getResult(gate->_fi[2]));
                else if (gate->_inv == 1) recordMap[gate] = (~getResult(gate->_fi[0]) & getResult(gate->_fi[1])) | (getResult(gate->_fi[1]) & getResult(gate->_fi[2])) | (~getResult(gate->_fi[0]) & getResult(gate->_fi[2]));
                else recordMap[gate] = (getResult(gate->_fi[0]) & getResult(gate->_fi[1])) | (getResult(gate->_fi[1]) & getResult(gate->_fi[2])) | (getResult(gate->_fi[0]) & getResult(gate->_fi[2]));
            } else {
                //cout << "gate_type: " << gate->_type << endl;
                gate->showNode();
                err("Undefined gate!");
            }
            return recordMap[gate];
        }
    }

    void truthTable_new(vector<vector<bool>>& truth_table, vector<uint64_t>& onsetSize, vector<vector<uint64_t>>& OSV) {
        int input_size = _gate[INPUT].size();
        int output_size =  _gate[OUTPUT].size();
        int input_pattern_size = uint64_t(1) << input_size;

        truth_table.clear();
        onsetSize.clear();

        truth_table.resize(input_pattern_size, vector<bool>(output_size, 0));
        onsetSize.resize(output_size, 0);

        bitset<8192> b_0(0);
        if (_gate[INPUT].size() <= 13) {
            for (auto &it : _gate[INPUT]) {
                piValue[it] = b_0;
            }
            for (int index = 0; index < input_pattern_size; index++) {
                int value = index;
                for (int i = 0; i < input_size; i++) {
                    piValue[_gate[INPUT][i]][index] = value & 1;
                    // cout << _gate[INPUT][i]->_name << ' ' << index << ' ' << piValue[_gate[INPUT][i]][index] << endl;
                    value = value >> 1;
                }
            }
            for (size_t i = 0; i < output_size; i++) {
                bitset<8192> out = getResult(_gate[OUTPUT][i]);
                for (int index = 0; index < input_pattern_size; index++) {
                    truth_table[index][i] = out[index];
                    // cout << _gate[OUTPUT][i]->_name << ' ' << index << ' ' << truth_table[index][i] << endl;
                    if (truth_table[index][i]) {
                        onsetSize[i]++;
                    }
                }
                recordMap.clear();
            }
        } else {
            for (auto &it : _gate[INPUT]) {
                piValue[it] = b_0;
            }
            int sim_times = input_pattern_size / 8192;
            for (int times = 0; times < sim_times; times++) {
                for (int index = 0; index < 8192; index++) {
                    long long value = 8192 * times + index;
                    for (int i = 0; i < input_size; i++) {
                        piValue[_gate[INPUT][i]][index] = value & 1;
                        value = value >> 1;
                    }
                }
                for (size_t i = 0; i < output_size; i++) {
                    bitset<8192> out = getResult(_gate[OUTPUT][i]);
                    for (int index = 0; index < 8192; index++) {
                        long long value = 8192 * times + index;
                        truth_table[value][i] = out[index];
                        if (truth_table[value][i]) {
                            onsetSize[i]++;
                        }
                    }
                    recordMap.clear();
                }
            }
            for (int index = 8192 * sim_times; index < input_pattern_size; index++) {
                int value = index;
                for (int i = 0; i < input_size; i++) {
                    piValue[_gate[INPUT][i]][index - 8192 * sim_times] = value & 1;
                    value = value >> 1;
                }
                for (size_t i = 0; i < output_size; i++) {
                    bitset<8192> out = getResult(_gate[OUTPUT][i]);
                    for (int index = 8192 * sim_times; index < input_pattern_size; index++) {
                        truth_table[index][i] = out[index - 8192 * sim_times];
                        if (truth_table[index][i]) {
                            onsetSize[i]++;
                        }
                    }
                    recordMap.clear();
                }
            }
        }

        OSV.clear();
        OSV.resize(output_size, vector<uint64_t>(input_pattern_size, 0));
        for (int i = 0; i < output_size; i++)
        {
            for (uint64_t j = 0; j < input_pattern_size; j++)
            {
                bool origin_bit = truth_table[j][i];
                for (int k = 0; k < input_size; k++)
                {
                    bitset<64> value(j);
                    value.flip(k);
                    if (truth_table[value.to_ulong()][i] != origin_bit)
                    {
                        OSV[i][j]++;
                    }
                }
            }

            // sort(OSV[i].begin(), OSV[i].end(), greater<int>());
            // cout << _gate[OUTPUT][i]->_name << " OSV:";
            // for (uint64_t j = 0; j < allInput; j++)
            // {
            //     cout << " " << OSV[i][j];
            // }
            // cout << endl;
        }
        /*for (int i = 0; i < OSV.size(); ++i) {
            cout << "output " << i << ": " << OSV[i].size() << endl;
        }*/
    }

    void ABCSplitOutput(vector<string>& file_name, int circuit_number) { 
        Abc_Frame_t * pAbc;
        char Command[1000];
        
        //////////////////////////////////////////////////////////////////////////
        // start the ABC framework
        Abc_Start();
        pAbc = Abc_FrameGetGlobalFrame();

        // string map_path = "./map/" + file_name[0];
        string split_path = "./split/" + file_name[0] + "_" + to_string(circuit_number);

        string map_path = file_name[0];
        //string split_path = to_string(circuit_number);

        string del_out = map_path + "_del_out" + to_string(circuit_number) + ".v";
        
        // string cmd = "chmod 777 ./abc";
        // system(cmd.c_str());

        int output_size =  _gate[OUTPUT].size();
        for (int i = 0; i < output_size; i++) {
            string output_path = split_path + "_" + _gate[OUTPUT][i]->_name + ".v";
            // cmd = "./abc -c \"read_library split.genlib; read_verilog " + del_out + ";strash;map;cone "  + _gate[OUTPUT][i]->_name + ";write_verilog " + output_path + "\"";
            // system(cmd.c_str());
            sprintf( Command, "read_library split.genlib; read_verilog %s; strash; map; cone %s; write_verilog %s", &del_out[0], &_gate[OUTPUT][i]->_name[0], &output_path[0]);
            if ( Cmd_CommandExecute( pAbc, Command ) )
            {
                fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // stop the ABC framework
        Abc_Stop();
    }

    void findFunctionalSupport(vector<string>& file_name, int circuit_number, vector<vector<int>>& output_support, vector<vector<int>>& input_support) {
        Abc_Frame_t * pAbc;
        char Command[1000];
        
        //////////////////////////////////////////////////////////////////////////
        // start the ABC framework
        Abc_Start();
        pAbc = Abc_FrameGetGlobalFrame();

        int output_size =  _gate[OUTPUT].size();
        int input_size = _gate[INPUT].size();
        // string split_path = "./split/" + file_name[0] + "_" + to_string(circuit_number);
        string split_path = to_string(circuit_number);
        // string cmd;
        // string cmd = "chmod 744 ./cadical";
        // system(cmd.c_str());

        // resize the output support 2d vector array and the input support 2d vector array
        output_support.resize(output_size, vector<int>(0));
        input_support.resize(input_size, vector<int>(0));

        string miter_verilog_file_name, miter_cnf_file_name = "miter.cnf";
        for (int i = 0; i < output_size; i++) {
            string input;
            for (int j = 0; j < ((int) _gate[OUTPUT][i]->_pi_idx.size()) - 1; j++) {
                input = input + "in" + to_string(j);
                if (j != ((int) _gate[OUTPUT][i]->_pi_idx.size()) - 2) input = input + ",";
            }
            for (int j = 0; j < _gate[OUTPUT][i]->_pi_idx.size(); j++) {
                CaDiCaL::Solver *solver = new CaDiCaL::Solver;

                ifstream in(split_path + "_" + _gate[OUTPUT][i]->_name + ".v");
                ofstream out(split_path + "_miter.v");
                out << "module miter(" << input << ",out" << ");" << endl;
                out << "    input " << input << ";" << endl;
                out << "    output " << "out;" << endl; 
                out << "    wire " << "n0, n1;" << endl;
                out << "    top_" << _gate[OUTPUT][i]->_name << "(";
                for (int k = 0, idx = 0; k < _gate[OUTPUT][i]->_pi_idx.size(); k++) {
                    if (k == j) out << "." << _gate[INPUT][_gate[OUTPUT][i]->_pi_idx[k]]->_name << "(1'b0),";
                    else out << "." << _gate[INPUT][_gate[OUTPUT][i]->_pi_idx[k]]->_name << "(in" << idx++ << "),";
                }
                out << "." << _gate[OUTPUT][i]->_name << "(n0));" << endl;
                out << "    top_" << _gate[OUTPUT][i]->_name << "(";
                for (int k = 0, idx = 0; k < _gate[OUTPUT][i]->_pi_idx.size(); k++) {
                    if (k == j) out << "." << _gate[INPUT][_gate[OUTPUT][i]->_pi_idx[k]]->_name << "(1'b1),";
                    else out << "." << _gate[INPUT][_gate[OUTPUT][i]->_pi_idx[k]]->_name << "(in" << idx++ << "),";
                }
                out << "." << _gate[OUTPUT][i]->_name << "(n1));" << endl;
                out << "    xor(out, n0, n1);" << endl;
                out << "endmodule" << endl << endl;
                string module_content;
                while(getline(in, module_content)) {
                    out << module_content << endl;
                }
                in.close();
                out.close();
                miter_verilog_file_name = split_path + "_miter.v";
                // cmd = "./abc -c \"read_verilog " + miter_verilog_file_name + ";strash;" + " write_cnf " + miter_cnf_file_name + "\"";
                // system(cmd.c_str());
                sprintf( Command, "read_verilog %s; strash; write_cnf %s", &miter_verilog_file_name[0], &miter_cnf_file_name[0]);
                if ( Cmd_CommandExecute( pAbc, Command ) )
                {
                    fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
                }
                // cmd = "./cadical " + miter_cnf_file_name + " > eq.log";
                // system(cmd.c_str());
                // api
                int vars;
                solver->read_dimacs(miter_cnf_file_name.c_str(), vars);
                int res = solver->solve();
                if (res == 10) {
                    output_support[i].emplace_back(_gate[OUTPUT][i]->_pi_idx[j]);
                    input_support[_gate[OUTPUT][i]->_pi_idx[j]].emplace_back(i);
                    cout << _gate[OUTPUT][i]->_name << ": " << _gate[INPUT][_gate[OUTPUT][i]->_pi_idx[j]]->_name << " is functional support" << endl;
                } else {
                    cout << _gate[OUTPUT][i]->_name << ": " << _gate[INPUT][_gate[OUTPUT][i]->_pi_idx[j]]->_name << " is NOT functional support" << endl;
                }

                // string line;
                // bool is_functional_support = true;
                // ifstream log_file("eq.log");
                // while (getline(log_file, line)) {
                //     if (line.find("UNSATISFIABLE") != string::npos) {
                //        is_functional_support = false;
                //        break;
                //     }
                // }
                // if (is_functional_support) {
                //     cout << _gate[OUTPUT][i]->_name << ": " << _gate[INPUT][_gate[OUTPUT][i]->_pi_idx[j]]->_name << " is functional support" << endl;
                // } else {
                //     cout << _gate[OUTPUT][i]->_name << ": " << _gate[INPUT][_gate[OUTPUT][i]->_pi_idx[j]]->_name << " is NOT functional support" << endl;
                // }
                delete solver;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // stop the ABC framework
        Abc_Stop();
    }

    // Collect nodes of PIs and check if flip, return 1 if the structure is wrong, 0 for correct
    bool CollectPI(Node* out, Node* now, Node* par, int bit)
    {
        if(now->_type == INPUT)
        {
            out->_pi.push_back(now);
            if(par->_inv > 0 && par->_type == AND && bit == 0)
            {
                if(par->_fi[0] == now) now->_isFlip = 1;
                else if(par->_fi[1] == now && par->_inv == 2) now->_isFlip = 1;
            }
        }
        else if(now->_type == AND)
        {
            for(auto in:now->_fi)
            {
                // Check if NOT gates exist between AND gates
                if(now->_inv > 0 && in->_type == AND)
                {
                    if(now->_fi[0] == in) return 1;
                    else if(now->_fi[1] == in && now->_inv == 2) return 1;
                }
                if(CollectPI(out,in,now,bit)&& bit == 0) return 1;
            }
        }
        else return 1;
        return 0;
    }
    // Trace back the inputs of the XOR tree
    void TraceXOR(Node* now, Node* par, set<Node*>& ANDset, unordered_set<int>& ADDbus)
    {
        if(now->_type == INPUT)
        {
            //_allEquation.back()->Insert(vector<int>(1,now->_bus_idx));
            ADDbus.insert(now->_bus_idx);
        }
        else if(now->_type == AND)
        {
            ANDset.insert(now);
        }
        else if(now->_type == XOR)
        {
            for(auto fi:now->_fi)
            {
                TraceXOR(fi,now,ANDset,ADDbus);
            }
        }
    }
    // Analyze the bit-0 output structure, return 1 if the structure is wrong, 0 for correct
    bool TraceTerm(Node* out, int bit, unordered_set<int>& ADDbus, set<vector<int>>& MULbus)
    {
        set<Node*> ANDset;
        TraceXOR(out->_fi[0],NULL,ANDset,ADDbus);
        for(auto g:ANDset)
        {
            if(CollectPI(g,g,NULL,bit))
            {
                // Detect a wrong structure at bit-0
                if(bit == 0) return 1;
            }
            else // Correct continuios AND gates feeding to the XOR tree
            {
                int idx_sum = 0;
                bool isSameBit = false;
                vector<int> tmp;
                for(auto x:g->_pi)
                {
                    tmp.push_back(x->_bus_idx);
                    idx_sum += x->_bit_idx;
                    if(x->_bit_idx == bit) isSameBit = true;
                }
                if(idx_sum == bit && isSameBit)
                {
                    sort(tmp.begin(),tmp.end());
                    MULbus.insert(tmp);
                    //_allEquation.back()->Insert(tmp);
                }
            }
        }
        cout << "Finish building an arithematic structure with " << out->_name << " as bit-" << bit << ":\n";
        for(auto bus_idx:ADDbus)
        {
            cout << "(" << _bus_set[bus_idx].back()->_name << ") + ";
        }
        for(auto& term:MULbus)
        {
            cout << "(";
            for(auto bus_idx:term)
            cout << _bus_set[bus_idx].back()->_name << " * ";
            cout << ") + ";
        }
        cout << "= " << out->_name << "\n";
        return 0;
    }
    void InitToZeroBit(int bit, unordered_set<int>& ADDbus, set<vector<int>>& MULbus)
    {
        InitInput();
        for(auto bus_idx:ADDbus)
        {
            int msb = (_bus_set[bus_idx].size()-1 < bit ? 0 : _bus_set[bus_idx].size()-1-bit);
            for(int i=msb;i<_bus_set[bus_idx].size();++i)
            {
                AssignFlipValue(_bus_set[bus_idx][i],ZERO);
            }
        }
        for(auto term:MULbus)
        for(auto bus_idx:term)
        {
            int msb = (_bus_set[bus_idx].size()-1 < bit ? 0 : _bus_set[bus_idx].size()-1-bit);
            for(int i=msb;i<_bus_set[bus_idx].size();++i)
            {
                AssignFlipValue(_bus_set[bus_idx][i],ZERO);
            }
        }
    }
    void CheckMulBit(int bit, unordered_set<int>& ADDbus, set<vector<int>>& MULbus, Module* base_module)
    {
        if(bit == 0) return;
        for(auto term:MULbus)
        for(auto tar_idx:term)
        {
            if(base_module->_bus_set[tar_idx].size()-1 < bit) continue;
            base_module->InitToZeroBit(bit, ADDbus, MULbus);
            //cout << "------Finish Init Zero------\n";
            Node* tar = base_module->_bus_set[tar_idx][base_module->_bus_set[tar_idx].size()-1-bit];
            for(auto cmp_idx:term)
            {
                int offset = base_module->_bus_set[cmp_idx].size()-1;
                Node* cmp = base_module->_bus_set[cmp_idx][offset]; // Bit-0
                AssignFlipValue(cmp,((cmp_idx == tar_idx) ? ZERO : ONE)); // Assign to 1 s.t. a_bit*b_0... = a_bit
            }
            //cout << "------Finish Set One--------\n";
            // Simulate
            if(base_module->_allEquation.back()->_output.size()-1 < bit) err("Get a negative index on an output bus.");
            int idx = base_module->_allEquation.back()->_output.size()-1-bit;
            Node* out = base_module->_allEquation.back()->_output[idx];
            cout << "Judge (" << tar->_name << ", " << out->_name;
            base_module->OutputValue(out);
            out->_signal = (out->_isFlip ? Signal(out->_signal == ZERO) : out->_signal);
            tar->_isFlip = (tar->_signal != out->_signal);
            cout << ") = (" << tar->_signal << ", " << out->_signal << ")\n";
        }   
                
    }
    void CheckCarry(Node* out, Node* now, Node* par, unordered_set<int>& ADDbus, Module* base_module)
    {
        if(now->_type == INPUT)
        {
            // Exist NOT at the input and the input is at add part
            Node* base_now = base_module->_name_to_node[now->_name];
            if(par->_inv > 0 && (par->_type == AND || par->_type == FACARRY) && ADDbus.find(base_now->_bus_idx) != ADDbus.end())
            {
                if(par->_fi[0] == now)
                {
                    base_now->_isFlip = 1;
                }
                else if(par->_fi[1] == now && par->_inv == 2)
                {
                    base_now->_isFlip = 1;
                }
                else if(par->_inv == 3)
                {
                    base_now->_isFlip = 1;
                }
            }
        }
        else
        {
            for(auto in:now->_fi) CheckCarry(out,in,now,ADDbus,base_module);
        }
    }
    // Two heuristic: 1. First assign add part, then multiple part
    //                2. Start to check multiply part bit-1 after finishing add part bit-1
    void CheckFlip(Module* base_module, string prev_out, unordered_set<int>& ADDbus, set<vector<int>>& MULbus)
    {
        cout << "Start bit" << _cur_bit_place << "...\n";
        int offset = base_module->_allEquation.back()->_output.size()-1;
        // Special case: the MSB first embeding the multiply part as 0, and then check whether the add part flip 
        if(_cur_bit_place >= offset)
        {
            // Simulate to check odd piority
            if(!ADDbus.empty())
            {
                if(base_module->_allEquation.back()->_output.size()-1 < _cur_bit_place) err("Get a negative index on an output bus.");
                int out_idx = base_module->_allEquation.back()->_output.size()-1-_cur_bit_place;
                int bus_idx = *ADDbus.begin(); // It needs to take the bus with the largest size
                for(auto x:ADDbus) if(base_module->_bus_set[bus_idx].size() < base_module->_bus_set[x].size()) bus_idx = x;
                if(base_module->_bus_set[bus_idx].size() > _cur_bit_place) // There exists a corresponding bit in add part
                {
                    Node* out = base_module->_allEquation.back()->_output[out_idx];
                    // Pick the first add part as the flip bit, note that the size of the input bus
                    Node* tar = base_module->_bus_set[bus_idx][base_module->_bus_set[bus_idx].size()-1-_cur_bit_place];
                    base_module->InitToZeroBit(_cur_bit_place, ADDbus, MULbus);
                    cout << "Judge (" << tar->_name << ", " << out->_name;
                    base_module->OutputValue(out);
                    out->_signal = (out->_isFlip ? Signal(out->_signal == ZERO) : out->_signal);
                    tar->_isFlip = (tar->_signal != out->_signal);
                    cout << ") = (" << tar->_signal << ", " << out->_signal << ")\n";
                }
            }
            
            // Check the last multiply input bits
            if(!MULbus.empty()) CheckMulBit(_cur_bit_place,ADDbus,MULbus,base_module);
            cout << "Finish last bit" << _cur_bit_place << "\n"
            << "=========================================================\n";
        }
        else
        {
            // 1. decide the flip of add part
            // Assign 0 to mul bit-0
            Node* cur = base_module->_allEquation.back()->_output[offset-_cur_bit_place-1]; // The next bit
            cur = _name_to_node[cur->_name]; // Update to the current netlist
            vector<int> ctrl_signal;
            if(!ADDbus.empty()) CheckCarry(cur,cur,NULL,ADDbus,base_module);
            if(!MULbus.empty()) CheckMulBit(_cur_bit_place,ADDbus,MULbus,base_module);
            for(auto bus_idx:ADDbus)
            {
                // The node address at the base module
                if(base_module->_bus_set[bus_idx].size()-1 < _cur_bit_place) err("Ran out of bit length!");
                cur = base_module->_bus_set[bus_idx][base_module->_bus_set[bus_idx].size()-1-_cur_bit_place];
                _ctrl_signal.push_back(cur); // Push Node
                ctrl_signal.push_back(base_module->AssignFlipValue(cur,ZERO)); // Push value
                cout << "Control " << _ctrl_signal.back()->_name << " = " << ctrl_signal.back() << endl;
            }
            cout << "Finish bit" << _cur_bit_place << "...\n";
            if(!ADDbus.empty())
            {
                // string assign_value_abc_map_out = "./map/assign_value_abc_map_out.v";
                // string ctrl_signal_assign_out = "./map/ctrl_signal_assign_out" + to_string(_cur_bit_place) + ".v";
                string assign_value_abc_map_out = "assign_value_abc_map_out.v";
                string ctrl_signal_assign_out = "ctrl_signal_assign_out" + to_string(_cur_bit_place) + ".v";

                assign_ctrl_signal(ctrl_signal,ctrl_signal_assign_out,assign_value_abc_map_out,prev_out);
                ifstream ifs_test(assign_value_abc_map_out);
                vector<Module*> test;
                Parser(ifs_test, test, 0, _cur_bit_place+1); 
                test[0]->CheckFlip(base_module,ctrl_signal_assign_out,ADDbus,MULbus);
                delete test[0];//TODO: Totally delete the module, notice not affect the base module
            }
            else
            {
                cout << "No ctrl signal!\n";
                _cur_bit_place++;
                CheckFlip(base_module,prev_out,ADDbus,MULbus);
            }
        }  
    }
    void CombineTerm(unordered_set<int>& ADDbus, set<vector<int>>& MULbus) // TODO
    {
        set<int> tmp;
        int pow13[6] = {1,13,169,2197,28561,371293};
        for(auto& term:MULbus)
        {
            for(auto bus_idx:term) 
            {
                tmp.insert(bus_idx);
                _bus_signature[bus_idx] += pow13[term.size()-1];
            }
        }
        if(!MULbus.empty())
        for(auto it=ADDbus.begin();it!=ADDbus.end();)
        {
            if(tmp.find(*it) == tmp.end()) it++;
            else
            {
                it = ADDbus.erase(it);
            }
        }
        // Write in _allEquation // Compute bus signature
        for(auto bus_idx:ADDbus)
        {
            _allEquation.back()->Insert(vector<int>(1,bus_idx));
            _bus_signature[bus_idx] += 1;
        }
        for(auto term:MULbus)
        {
            _allEquation.back()->Insert(term);
        }
    }
    void BusConfig(vector<Node*>& output_bit_place, int caseID, int max_bus_diff)
    {
        if(output_bit_place.size() == 1)
        {
            cout << "Find an illegal arithematic structure: Only one bit!\n";
            return;
        }
        int empty_cnt = 0;
        int excess_cnt = 0;
        unordered_set<int> vis_bus_idx;
        sort(output_bit_place.begin(),output_bit_place.end(),[](Node* a, Node* b)
        {
            return a->_pi_idx.size() > b->_pi_idx.size();
        });
        for(int i=0;i<output_bit_place.size()-1;i++)
        {
            output_bit_place[i]->_bit_idx = output_bit_place.size()-i-1;
            set<int> s1(output_bit_place[i]->_pi_idx.begin(),output_bit_place[i]->_pi_idx.end()),
            s2(output_bit_place[i+1]->_pi_idx.begin(),output_bit_place[i+1]->_pi_idx.end());
            // Fill in s1 and s2 with values
            set<int> result, intersect;
            set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(),std::inserter(intersect, intersect.begin()));
            if(intersect != s2) excess_cnt++;

            set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(result, result.end()));
            if(result.empty())
            {
                for(int j=i;j>=0;--j) output_bit_place[j]->_bit_idx--;
                empty_cnt++;
            }
            for(auto in:result) output_bit_place[i]->_same_input_bit.push_back(_gate[INPUT][in]);
        }
        output_bit_place.back()->_bit_idx = 0;
        if(excess_cnt >= output_bit_place.size()/2 && excess_cnt != 0)
        {
            cout << "Find an illegal arithematic structure: Violate Contain Relation!\n";
            for(auto x: output_bit_place) x->_bit_idx = -1;
            return;
        }
        if(empty_cnt+1 >= output_bit_place.size() && empty_cnt != 0)
        {
            cout << "Find an illegal arithematic structure: No Proper Subsets!\n";
            for(auto x: output_bit_place) cout << x->_bit_idx;//x->_bit_idx = -1;
            cout << "\n";
            return;
        }
        for(auto in:output_bit_place.back()->_pi_idx) output_bit_place.back()->_same_input_bit.push_back(_gate[INPUT][in]);
        for(auto out: output_bit_place)
        {
            for(auto pi:out->_same_input_bit)
            {
                pi->_bit_idx = out->_bit_idx;
                vis_bus_idx.insert(pi->_bus_idx);
            }
            sort(out->_same_input_bit.begin(),out->_same_input_bit.end(),[](Node* a,Node* b)
            {
                return a->_bus_idx < b->_bus_idx;
            });
            cout << "out = " << out->_name << ", in = ";
            for(auto pi:out->_same_input_bit) cout << pi->_name << "(" << pi->_bus_idx << "), ";
            cout << endl;
        }
        // Finish judge bit place of inputs and outputs
        for(auto bus_idx:vis_bus_idx)
        {
            if(_bus_set[bus_idx].size() > output_bit_place.size())
            {
                cout << "Find an illegal arithematic structure: Violate Output with The Largest Bits!\n";
                return;
            }
            // Sort bus by their bit
            sort(_bus_set[bus_idx].begin(),_bus_set[bus_idx].end(),[](Node* a,Node* b)
            {
                return a->_bit_idx > b->_bit_idx;
            });
        }
        Operation* op = new Operation(output_bit_place);
        unordered_set<int> ADDbus;
        set<vector<int>> MULbus;
        _allEquation.push_back(op);
        if(TraceTerm(output_bit_place.back(),0,ADDbus, MULbus)) // Find illegal arithematic structure
        {
            // Delete back
            // _allEquation.pop_back();
            // delete op;
            // op = NULL;
            for(int bit=1;bit<output_bit_place.size();++bit)
                TraceTerm(output_bit_place[output_bit_place.size()-1-bit],bit,ADDbus, MULbus);
            MULbus.clear();
            // Write in op
            CombineTerm(ADDbus,MULbus);
            // Sort the same input bit
            for(auto out: output_bit_place)
            sort(out->_same_input_bit.begin(),out->_same_input_bit.end(),[&](Node* a, Node* b)
            {
                if(a->_bus_indices.size() == b->_bus_indices.size())
                {
                    bool afound = false;
                    bool bfound = false;
                    for(int k=0;k<a->_bus_indices.size();++k)
                    {
                        int abus = a->_bus_indices[k];
                        int bbus = b->_bus_indices[k];
                        if(_bus_set[abus].size() != _bus_set[bbus].size())
                            return _bus_set[abus].size() < _bus_set[bbus].size();
                        if(ADDbus.find(abus) != ADDbus.end()) afound = true; // Found in ADDbus
                        if(ADDbus.find(bbus) != ADDbus.end()) bfound = true; // Found in ADDbus
                    }
                    if(afound) return false; // true
                    if(bfound) return true; // false
                    // Both are not found
                    return _bus_set[a->_bus_idx].size() > _bus_set[b->_bus_idx].size(); // a->_bus_idx < b->_bus_idx
                }
                else return a->_bus_indices.size() > b->_bus_indices.size();
            });
            cout << "Find an illegal arithematic structure: Bit-0 (" << output_bit_place.back()->_name << ") cannot be interpreted!\n";
            for(auto out: output_bit_place)
            {
                cout << "(" << out->_bus_idx << ")" << setw(7) << out->_name << (out->_isFlip ? "(Flip)" : "") << " bit-" << out->_bit_idx
                << " " << out->_same_input_bit.size() << " pi: ";
                for(auto pi:out->_same_input_bit)
                {
                    cout << "(" << pi->_bus_indices[0];
                    for(int i=1;i<pi->_bus_indices.size();++i)
                        cout << "," << pi->_bus_indices[i];
                    cout << ")" << pi->_name << (pi->_isFlip ? "(Flip)" : "") << ", ";
                }
                cout << "\n";
            }
        }
        else if(!ADDbus.empty() || !MULbus.empty()) // Find legal arithematic structure
        {
            for(int bit=1;bit<output_bit_place.size();++bit)
                TraceTerm(output_bit_place[output_bit_place.size()-1-bit],bit,ADDbus, MULbus);
            CombineTerm(ADDbus,MULbus); // Write in op
            //CheckFlip(this,del_out[_moduleIndex],ADDbus,MULbus);
            // Show current bit situation
            for(auto out: output_bit_place)
            {
                cout << "(" << out->_bus_idx << ")" << setw(7) << out->_name << (out->_isFlip ? "(Flip)" : "") << " bit-" << out->_bit_idx
                << " " << out->_same_input_bit.size() << " pi: ";
                for(auto pi:out->_same_input_bit)
                {
                    cout << "(" << pi->_bus_indices[0];
                        for(int i=1;i<pi->_bus_indices.size();++i)
                            cout << "," << pi->_bus_indices[i];
                    cout << ")" << pi->_name << (pi->_isFlip ? "(Flip)" : "") << ", ";
                }
                cout << "\n";
            }
        }

    }
    // Decide the outputs and inputs corresponding bit places
    void OutputConfig(int caseID, int max_bus_diff)
    {
        for(auto& bus:_bus_set)
        {
            // Create an equation to match
            if(bus[0]->_type == OUTPUT)
            {
                cout << "In bus " << bus[0]->_bus_idx << ":\n";
                BusConfig(bus,caseID,max_bus_diff);
            }
        }
        // Sort the _allEquation with their signature
        for(auto eq:_allEquation)
        {
            eq->SortTerm(_bus_signature);
        }
        sort(_allEquation.begin(),_allEquation.end(),[&](Operation* a,Operation* b)
        {
            if(a->_output.size() == b->_output.size())
            {
                if(a->_real_input.size() == b->_real_input.size())
                {
                    for(auto ait = a->_real_input.begin(), bit = b->_real_input.begin(); ait != a->_real_input.end(); ait++, bit++)
                    {
                        if((*ait)->_member.size() == (*bit)->_member.size())
                        {
                            for(auto amit = (*ait)->_member.begin(), bmit = (*bit)->_member.begin(); amit != (*ait)->_member.end(); amit++, bmit++)
                            {
                                if(_bus_signature[*amit] != _bus_signature[*bmit]) return _bus_signature[*amit] > _bus_signature[*bmit];
                            }
                        }
                        else return (*ait)->_member.size() > (*bit)->_member.size();
                    }
                    if(a->_real_input.empty()) return true;
                    else if(b->_real_input.empty()) return false;
                    return (a->_real_input[0]->_member[0] == b->_real_input[0]->_member[0] ? a->_real_input[0]->_member.back() > b->_real_input[0]->_member.back() : a->_real_input[0]->_member[0] > b->_real_input[0]->_member[0]);
                }
                else return a->_real_input.size() > b->_real_input.size();
            }
            else return a->_output.size() < b->_output.size();
        });
    }

    void assign_ctrl_signal(vector<int>& cur_bit_array, string ctrl_signal_assign_out, string assign_value_abc_map_out, string delete_gate_name_path)
    {
        if(_ctrl_signal.empty())
        {
            cout << "No any ctrl signal." << endl;
            return;
        }
        // cout << "Input file: " << delete_gate_name_path << endl;
        // cout << "Output file: " << ctrl_signal_assign_out << endl;
        // cout << "ABC Output file: " << assign_value_abc_map_out << endl;
        ifstream ifs(delete_gate_name_path);
        ofstream ofs(ctrl_signal_assign_out);
        string line;
        
        while(getline(ifs, line))
        {
            if(line.find("module") != string::npos || line.find("wire") != string::npos || line.find("input") != string::npos)
            {
                ofs << line << endl;
            }
            else
            {
                for(int i = 0; i < _ctrl_signal.size(); i++)
                {
                    size_t found = -1;
                    while(line.find(_ctrl_signal[i]->_name, found + 1) != string::npos)
                    {
                        found = line.find(_ctrl_signal[i]->_name, found + 1);
                        int pos = found + (_ctrl_signal[i]->_name).size();
                        if(!isalnum(line[pos]) && line[pos-3] != '\'')
                        {
                            if(line[pos] != '[')
                            {
                                if(cur_bit_array[i])
                                    line.replace(found, (_ctrl_signal[i]->_name).size(), "1'b1");
                                else
                                    line.replace(found, (_ctrl_signal[i]->_name).size(), "1'b0");
                            }
                            else
                            {
                                string num;
                                pos++;
                                int p = pos;
                                while(line[p] != ']') p++;
                                num = line.substr(pos, p - pos);
                                int number = stoi(num);
                                if(cur_bit_array[i] >> number & 1)
                                    line.replace(found, p - found + 1, "1'b1");
                                else
                                    line.replace(found, p - found + 1, "1'b0");
                            }
                        }
                        
                    }
                }
                ofs << line << endl;
            }
        }
        ifs.close();
        ofs.close();
        string resync2 = "balance; rewrite -l; refactor -l; balance; rewrite -l; rewrite -lz; balance; refactor -lz; rewrite -lz; balance;";
        // string cmd = "./abc -c \"read_library cadence.genlib; read_verilog " + ctrl_signal_assign_out + ";strash;" + resync2+ resync2+ resync2+ resync2+ resync2 + " map; write_verilog " + assign_value_abc_map_out + "\"";
        // system(cmd.c_str());
        Abc_Frame_t * pAbc;
        char Command[1000];
        
        //////////////////////////////////////////////////////////////////////////
        // start the ABC framework
        Abc_Start();
        pAbc = Abc_FrameGetGlobalFrame();

        // string resync2 = "balance; rewrite -l; refactor -l; balance; rewrite -l; rewrite -lz; balance; refactor -lz; rewrite -lz; balance;";
        // string cmd = "./abc -c \"read_library cadence.genlib; read_verilog " + ctrl_signal_assign_out + ";strash;" + resync2+ resync2+ resync2+ resync2+ resync2 + " map; write_verilog " + assign_value_abc_map_out + "\"";
        // system(cmd.c_str());

        sprintf( Command, "read_library cadence.genlib; read_verilog %s; strash;", &ctrl_signal_assign_out[0]);
        if ( Cmd_CommandExecute( pAbc, Command ) )
        {
            fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        }
        sprintf( Command, "%s", &resync2[0]);
        if ( Cmd_CommandExecute( pAbc, Command ) )
        {
            fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        }
        sprintf( Command, "%s", &resync2[0]);
        if ( Cmd_CommandExecute( pAbc, Command ) )
        {
            fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        }
        sprintf( Command, "%s", &resync2[0]);
        if ( Cmd_CommandExecute( pAbc, Command ) )
        {
            fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        }
        sprintf( Command, "%s", &resync2[0]);
        if ( Cmd_CommandExecute( pAbc, Command ) )
        {
            fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        }
        sprintf( Command, "%s", &resync2[0]);
        if ( Cmd_CommandExecute( pAbc, Command ) )
        {
            fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        }
        sprintf( Command, "map; write_verilog %s", &assign_value_abc_map_out[0]);
        if ( Cmd_CommandExecute( pAbc, Command ) )
        {
            fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
        }

        //////////////////////////////////////////////////////////////////////////
        // stop the ABC framework
        Abc_Stop();
    }
    void RenameVerilog(string input, string ctrl_signal_assign_out)
    {
        _ctrl_signal.clear();
        for(auto bus: _bus_set)
        {
            for(auto x: bus)
            {
                _ctrl_signal.push_back(x);
            }
        }
        // cout << "Input file: " << delete_gate_name_path << endl;
        // cout << "Output file: " << ctrl_signal_assign_out << endl;
        // cout << "ABC Output file: " << assign_value_abc_map_out << endl;
        ifstream ifs(input);
        ofstream ofs(ctrl_signal_assign_out);
        string line;
        
        while(getline(ifs, line))
        {
            for(int i = 0; i < _ctrl_signal.size(); i++)
            {
                size_t found = -1;
                while(line.find(_ctrl_signal[i]->_name, found + 1) != string::npos)
                {
                    found = line.find(_ctrl_signal[i]->_name, found + 1);
                    int pos = found + (_ctrl_signal[i]->_name).size();
                    if(!isalnum(line[pos]) && line[pos-3] != '\'')
                    {
                        if(line[pos] != '[')
                        {
                            int bus_idx = _ctrl_signal[i]->_bus_idx;
                            string cat = ((_ctrl_signal[i]->_bit_idx == -1) ? "" : "["+to_string(_ctrl_signal[i]->_bit_idx)+"]");
                            if(_ctrl_signal[i]->_type == INPUT)
                                line.replace(found, (_ctrl_signal[i]->_name).size(), "in"+to_string(bus_idx)+ cat);
                            else 
                                line.replace(found, (_ctrl_signal[i]->_name).size(), "out"+to_string(bus_idx)+ cat);
                        }
                    }
                    
                }
            }
            ofs << line << endl;
            
        }
        ifs.close();
        ofs.close();
    }
    int getPInum() { return _gate[INPUT].size();}
    int getPOnum() { return _gate[OUTPUT].size();}
    int getBusSz() { return _bus_set.size();}
    int getModuleID() { return _moduleIndex;}
    vector<Node*>& getGate(int type)    { return _gate[type]; }
    void assignCtrl(vector<string> v)
    {
        for(auto str:v) _ctrl_signal.push_back(_name_to_node[str]);
    }
    int countInvForXORTree(Node* gate) {
        if (gate->_type == XOR) return gate->_inv + countInvForXORTree(gate->_fi[0]) + countInvForXORTree(gate->_fi[1]);
        else return 0;
    }
    bitset<8192> miterGetResult(Node* gate, unordered_map<Node*, bitset<8192>>& piValue) {
        if (gate == NULL) 
            err("gate = NULL");
        if (recordMap.count(gate)) {
            return recordMap[gate];
        } else {
            if (gate->_type == INPUT) {
                return piValue[gate];
            } else if (gate->_type == BUF || gate->_type == OUTPUT) {
                if (gate->_inv == 1) recordMap[gate] = ~miterGetResult(gate->_fi[0], piValue);
                else recordMap[gate] = miterGetResult(gate->_fi[0], piValue);
            } else if (gate->_type == AND) {
                if (gate->_inv == 2) recordMap[gate] = ~miterGetResult(gate->_fi[0], piValue) & ~miterGetResult(gate->_fi[1], piValue);
                else if (gate->_inv == 1) recordMap[gate] = ~miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[1], piValue);
                else recordMap[gate] = miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[1], piValue);
            } else if (gate->_type == OR) {
                if (gate->_inv == 2) recordMap[gate] = ~miterGetResult(gate->_fi[0], piValue) | ~miterGetResult(gate->_fi[1], piValue);
                else if (gate->_inv == 1) recordMap[gate] = ~miterGetResult(gate->_fi[0], piValue) | miterGetResult(gate->_fi[1], piValue);
                else recordMap[gate] = miterGetResult(gate->_fi[0], piValue) | miterGetResult(gate->_fi[1], piValue);
            } else if (gate->_type == XOR) {
                if (gate->_inv == 2) recordMap[gate] = ~miterGetResult(gate->_fi[0], piValue) ^ ~miterGetResult(gate->_fi[1], piValue);
                else if (gate->_inv == 1) recordMap[gate] = ~miterGetResult(gate->_fi[0], piValue) ^ miterGetResult(gate->_fi[1], piValue);
                else recordMap[gate] = miterGetResult(gate->_fi[0], piValue) ^ miterGetResult(gate->_fi[1], piValue);
            } else if (gate->_type == NAND) {
                if (gate->_inv == 2) recordMap[gate] = miterGetResult(gate->_fi[0], piValue) & ~miterGetResult(gate->_fi[1], piValue);
                else if (gate->_inv == 1) recordMap[gate] = miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[1], piValue);
                else recordMap[gate] = ~(miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[1], piValue));
            } else if (gate->_type == NOR) {
                if (gate->_inv == 2) recordMap[gate] = miterGetResult(gate->_fi[0], piValue) | ~miterGetResult(gate->_fi[1], piValue);
                else if (gate->_inv == 1) recordMap[gate] = miterGetResult(gate->_fi[0], piValue) | miterGetResult(gate->_fi[1], piValue);
                else recordMap[gate] = ~(miterGetResult(gate->_fi[0], piValue) | miterGetResult(gate->_fi[1], piValue));
            } else if (gate->_type == ZEROBUF) {
                bitset<8192> b(0);
                recordMap[gate] = b;
            } else if (gate->_type == ONEBUF) {
                bitset<8192> b(1);
                recordMap[gate] = b;
            } else if (gate->_type == FACARRY) {
                if (gate->_inv == 3) recordMap[gate] = (~miterGetResult(gate->_fi[0], piValue) & ~miterGetResult(gate->_fi[1], piValue)) | (~miterGetResult(gate->_fi[1], piValue) & ~miterGetResult(gate->_fi[2], piValue)) | (~miterGetResult(gate->_fi[0], piValue) & ~miterGetResult(gate->_fi[2], piValue));
                else if (gate->_inv == 2) recordMap[gate] = (~miterGetResult(gate->_fi[0], piValue) & ~miterGetResult(gate->_fi[1], piValue)) | (~miterGetResult(gate->_fi[1], piValue) & miterGetResult(gate->_fi[2], piValue)) | (~miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[2], piValue));
                else if (gate->_inv == 1) recordMap[gate] = (~miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[1], piValue)) | (miterGetResult(gate->_fi[1], piValue) & miterGetResult(gate->_fi[2], piValue)) | (~miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[2], piValue));
                else recordMap[gate] = (miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[1], piValue)) | (miterGetResult(gate->_fi[1], piValue) & miterGetResult(gate->_fi[2], piValue)) | (miterGetResult(gate->_fi[0], piValue) & miterGetResult(gate->_fi[2], piValue));
            } else {
                cout << "get_type\n";
                cout << "get_type: " << gate->_type << endl;
                cout << "get_name: " << gate->_name << endl;
                cout << "get_name: " << gate->_name << endl;

                gate->showNode();
                err("Undefined gate!");
            }
            return recordMap[gate];
        }
    }
    bitset<8192> miterRandomSimulate(Node* gate, unordered_map<Node*, bitset<8192>>& piValue) {
        recordMap.clear();
        return miterGetResult(gate, piValue);
    }
    void ForkCtrlNetlist(int bus_idx)
    {
        string assign_value_abc_map_out = "./map/assign_value_abc_map_out.v";
        string ctrl_signal_assign_out = "./map/ctrl_signal_assign_out.v";
        vector<Module*> test;
        _first_pi.resize(_bus_set[bus_idx].size());
        _ctrl_signal = _bus_set[bus_idx];
        cout << "Control bus = " << bus_idx << endl;
        for(auto x:_ctrl_signal) cout << x->_name << ", ";
        cout << "\n";
        for(int i=0;i<_bus_set[bus_idx].size();++i)
        {
            cout << "========================" << i << "==========================\n";
            vector<int> ctrl_signal(_bus_set[bus_idx].size(),0);
            ctrl_signal[i] = 1;
            
            assign_ctrl_signal(ctrl_signal,ctrl_signal_assign_out,assign_value_abc_map_out,del_out[_moduleIndex]);
            ifstream ifs_test(assign_value_abc_map_out);
            Parser(ifs_test, test, i, 0);
            // Get the information of the remap file
            _bus_set[bus_idx][i]->_bit_idx = i;
            for(auto out: test[i]->_gate[OUTPUT])
            {
                Node* tmp = out->_fi[0];
                if(tmp->_type == INPUT)
                {
                    _first_pi[i].push_back({tmp->_name, out->_inv > 0});
                    _name_to_node[tmp->_name]->_bit_idx = i;
                }
                else if(tmp->_type == ZEROBUF) _first_pi[i].push_back({"1'b0", 0});
                else if(tmp->_type == ONEBUF)  _first_pi[i].push_back({"1'b0", 1});
                else
                {
                    cout << "Error: Get " << TypeID[tmp->_type] << " gate from " << out->_name << endl;
                    //err("ERRR");
                }
            }
            cout << "===================================================\n";
        }
    }
    int getBus4Encode(int bit)
    {
        int val = 0;
        for(int i=0;i<_bus_set[4].size();++i)
        {
            int idx = _bus_set[4][i]->_idx;
            val += ((_bus_set[4][i]->_isFlip ? !_first_pi[bit][idx].second : _first_pi[bit][idx].second) << i);
        }
        return val;
    }
    friend class Match;
private:
    int _moduleIndex; // Index of the module
    int _cur_bit_place; // The current bit place
    string _name; // Name of the module
    vector<Node*> _netlist; // All nodes in the module
    vector<Node*> _gate[TYPEN]; // Containers of each gate
    vector<Node*> _ctrl_signal;
    vector<vector<Node*>> _bus_set,_bus_idx_bit_to_node; // All buses in the module
    vector<pair<Node*,int>> _xor_root; // {xor root, is inv at output}
    unordered_map<string, Node*> _name_to_node; // Can visit the node with its name
    map<vector<int>, int> _set_to_group_idx; // Can visit the group index with its set
    vector<int> _bus_signature; // The existent frequency with weight
    vector<Operation*> _allEquation; // _real_input records bus indices
    vector<Operation*> _group; // _real_input records _gate[INPUT] indices
    vector<vector<int>> _gate_path;
    map<int,int> _path_cnt;
    vector<vector<pair<string,int>>> _first_pi; // {name,isNOT} outer vector ordered by _ctrl_signal, inner vector ordered by _gate[OUTPUT]
};


#endif
