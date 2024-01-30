#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include "Netlist.h"
#include "../cadical-master/src/cadical.hpp"
using namespace std;
#define PNUM 8192

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

bool miter(string miter_verilog_file_name, string miter_cnf_file_name, string c1_file_name, string c2_file_name,
    vector<Node*>& input1, vector<Node*>& input2, vector<Node*>& output1, vector<Node*>& output2,
    vector<vector<int>>& in_match_pair, vector<bool>& in_match_neg,
    vector<vector<int>>& out_match_pair, vector<bool>& out_match_neg,
    vector<pair<bool, size_t>>& const_match
)
{

    ofstream miter_file(miter_verilog_file_name);

    //module
    miter_file << "module miter (";
    for (size_t i = 0; i < in_match_pair.size(); i++)
    {
        miter_file << "in" + to_string(i) + ", ";
    }

    miter_file << "out" + to_string(0) + ") ;" << endl;

    //input line
    miter_file << "\t" << "input ";
    if (in_match_pair.size() > 0)
    {
        miter_file << "in" + to_string(0);
    }
    for (int i = 1; i < in_match_pair.size(); i++)
    {
        miter_file << ", in" + to_string(i);
    }
    miter_file << " ;" << endl;

    //output line
    miter_file << "\t" << "output ";
    miter_file << "out0";
    miter_file << " ;" << endl;

    vector<string> inop;
    vector<string> outop;
    vector<string> c2_input(input2.size());
    size_t wire_count = 0;

    //input connect
    size_t inGroupNum = in_match_pair.size();
    for (size_t i = 0; i < inGroupNum; ++i)
    {
        size_t matchNum = in_match_pair[i].size();
        for (size_t j = 0; j < matchNum; ++j)
        {
            size_t port_idx = in_match_pair[i][j];
            if (in_match_neg[port_idx])
            {
                string name = "n" + to_string(wire_count);
                inop.push_back("not ( " + name + ", in" + to_string(i) + " ) ;");
                c2_input[port_idx] = name;
                wire_count++;
            }
            else
            {
                c2_input[port_idx] = "in" + to_string(i);
            }
        }
    }
    for (size_t i = 0; i < const_match.size(); ++i)
    {
        size_t port_idx = const_match[i].second;
        if (const_match[i].first)   //-
        {
            c2_input[port_idx] = "1'b1";
        }
        else  //+
        {
            c2_input[port_idx] = "1'b0";
        }
    }

    //output xor
    size_t or_start = wire_count;
    size_t outGroupNum = out_match_pair.size();
    for (size_t i = 0; i < outGroupNum; ++i)
    {
        string o1 = "o" + to_string(i);
        size_t matchNum = out_match_pair[i].size();
        for (size_t j = 0; j < matchNum; ++j)
        {
            size_t port_idx = out_match_pair[i][j];
            string o2 = "o" + to_string(outGroupNum + port_idx);
            if (out_match_neg[port_idx])
            {
                string name = "n" + to_string(wire_count);
                wire_count++;
                outop.push_back("xnor ( " + name + ", " + o1 + ", " + o2 + " ) ;");
            }
            else
            {
                string name = "n" + to_string(wire_count);
                wire_count++;
                outop.push_back("xor ( " + name + ", " + o1 + ", " + o2 + " ) ;");
            }
        }
    }

    //or all
    string or_all = "or ( out0";
    for (size_t i = or_start; i < wire_count; i++)
    {
        or_all += ", n" + to_string(i);
    }
    or_all += " ) ;";
    outop.push_back(or_all);

    //puts wire
    miter_file << "\t" << "wire ";
    if (wire_count > 0)
    {
        miter_file << "n0";
    }
    for (size_t i = 1; i < wire_count; i++)
    {
        miter_file << ", n" + to_string(i);
    }
    for (size_t i = 0; i < outGroupNum + output2.size(); i++)
    {
        miter_file << ", o" + to_string(i);
    }
    miter_file << " ;" << endl;

    //puts input operation
    for (size_t i = 0; i < inop.size(); i++)
    {
        miter_file << "\t" << inop[i] << endl;
    }

    string C1_module_name, C2_module_name;

    //call module 1
    ifstream c1_file(c1_file_name);
    string line;
    while (getline(c1_file, line))
    {
        string item;
        while (line.find(';') == string::npos)
        {
            getline(c1_file, item);
            line.append(item);
        }
        if (line.find("module") != string::npos)
        {
            int pos = line.find("(");
            C1_module_name = line.substr(6, pos - 6);
            line = line.substr(pos + 1);
            line.erase(remove(line.begin(), line.end(), ' '), line.end());
            miter_file << "\tCircuit1 circuit_1( ";

            for (size_t i = 0; i < in_match_pair.size(); i++)
            {
                string in_name = input1[i]->getName();
                //cout << "in name: " << in_name << endl;
                //cout << "line: " << line << endl;
                int pos;
                if ((pos = line.find(in_name + ",")) != string::npos)
                {
                    line.erase(pos, in_name.size() + 1);
                    line.insert(pos, "." + in_name + " (in" + to_string(i) + ") , ");
                }
                else if ((pos = line.find(in_name + ");")) != string::npos)
                {
                    line.erase(pos, in_name.size());
                    line.insert(pos, "." + in_name + " (in" + to_string(i) + ") ");
                }
                //cout << "pos: " << pos << endl;
            }

            for (size_t i = 0; i < outGroupNum; ++i)
            {
                string out_name = output1[i]->getName();
                //cout << "out name: " << out_name << endl;
                //cout << "line: " << line << endl;
                int pos;
                if ((pos = line.find(out_name + ",")) != string::npos)
                {
                    line.erase(pos, out_name.size() + 1);
                    line.insert(pos, "." + out_name + " (o" + to_string(i) + ") , ");
                }
                else if ((pos = line.find(out_name + ");")) != string::npos)
                {
                    line.erase(pos, out_name.size());
                    line.insert(pos, "." + out_name + " (o" + to_string(i) + ") ");
                }
                //cout << "pos: " << pos << endl;
            }
            miter_file << line << endl;
            break;
        }
    }
    c1_file.close();

    //call module 2
    ifstream c2_file(c2_file_name);
    while (getline(c2_file, line))
    {
        string item;
        while (line.find(';') == string::npos)
        {
            getline(c2_file, item);
            line.append(item);
        }
        if (line.find("module") != string::npos)
        {
            int pos = line.find("(");
            C2_module_name = line.substr(6, pos - 6);
            line = line.substr(pos + 1);
            line.erase(remove(line.begin(), line.end(), ' '), line.end());
            miter_file << "\tCircuit2 circuit_2( ";

            for (size_t i = 0; i < c2_input.size(); i++)
            {
                string in_name = input2[i]->getName();
                //cout << "in name: " << in_name << endl;
                //cout << "line: " << line << endl;
                int pos;
                if ((pos = line.find(in_name + ",")) != string::npos)
                {
                    line.erase(pos, in_name.size() + 1);
                    line.insert(pos, "." + in_name + " (" + c2_input[i] + ") , ");
                }
                else if ((pos = line.find(in_name + ");")) != string::npos)
                {
                    line.erase(pos, in_name.size());
                    line.insert(pos, "." + in_name + " (" + c2_input[i] + ") ");
                }
                //cout << "pos: " << pos << endl;
            }

            for (size_t i = 0; i < output2.size(); ++i)
            {
                string out_name = output2[i]->getName();
                //cout << "out name: " << out_name << endl;
                //cout << "line: " << line << endl;
                int pos;
                if ((pos = line.find(out_name + ",")) != string::npos)
                {
                    line.erase(pos, out_name.size() + 1);
                    line.insert(pos, "." + out_name + " (o" + to_string(i + outGroupNum) + ") , ");
                }
                else if ((pos = line.find(out_name + ");")) != string::npos)
                {
                    line.erase(pos, out_name.size());
                    line.insert(pos, "." + out_name + " (o" + to_string(i + outGroupNum) + ") ");
                }
                //cout << "pos: " << pos << endl;
            }
            miter_file << line << endl;
            break;
        }
    }
    c2_file.close();

    //puts output xor and or all of them
    for (size_t i = 0; i < outop.size(); i++)
    {
        miter_file << "\t" << outop[i] << endl;
    }
    miter_file << "endmodule" << endl << endl;

    //puts c1 module
    ifstream c1(c1_file_name);
    while (getline(c1, line))
    {
        if (line.find("endmodule") != string::npos)
        {
            miter_file << line << endl;
            break;
        }
        string item;
        while (line.find(';') == string::npos)
        {
            getline(c1, item);
            line.append(item);
        }
        if (line.find("module") != string::npos)
        {
            line.erase(6, C1_module_name.size());
            line.insert(6, " Circuit1");
        }
        miter_file << line << endl;
    }
    c1.close();
    miter_file << endl;

    ifstream c2(c2_file_name);
    while (getline(c2, line))
    {
        if (line.find("endmodule") != string::npos)
        {
            miter_file << line << endl;
            break;
        }
        string item;
        while (line.find(';') == string::npos)
        {
            getline(c2, item);
            line.append(item);
        }
        if (line.find("module") != string::npos)
        {
            line.erase(6, C2_module_name.size());
            line.insert(6, " Circuit2");
        }
        miter_file << line << endl;
    }
    c2.close();

    miter_file.close();

    // string cmd = "./abc -c \"read_verilog " + miter_verilog_file_name + ";strash;" + " write_cnf " + miter_cnf_file_name + "\"";
    // system(cmd.c_str());

    Abc_Frame_t * pAbc;
    char Command[1000];

    //////////////////////////////////////////////////////////////////////////
    // start the ABC framework
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();

    // string cmd = "./abc -c \"read_verilog " + miter_verilog_file_name + ";strash;" + " write_cnf " + miter_cnf_file_name + "\"";
    // system(cmd.c_str());
    sprintf( Command, "read_verilog %s; strash; write_cnf %s", &miter_verilog_file_name[0], &miter_cnf_file_name[0]);
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
    }

    //////////////////////////////////////////////////////////////////////////
    // stop the ABC framework
    Abc_Stop();

    CaDiCaL::Solver* solver = new CaDiCaL::Solver;
    int vars;
    solver->read_dimacs(miter_cnf_file_name.c_str(), vars);
    int res = solver->solve();
    delete solver;
    if (res == 10) //SAT non-eq
    {
        return 0;
    }
    else    //eq
    {
        return 1;
    }
    /*string cmd = "./cadical " + miter_cnf_file_name + " > eq.log";
    system(cmd.c_str());
    ifstream log_file("eq.log");
    while (getline(log_file, line)) {
        if (line.find("UNSATISFIABLE") != string::npos) {
            return 1;
        }
    }
    return 0;*/

}

bool miterOneOutput(string miter_verilog_file_name, string miter_cnf_file_name, string c1_file_name, string c2_file_name,
    vector<Node*>& input1, vector<Node*>& input2, vector<Node*>& output1, vector<Node*>& output2,
    vector<pair< int, int >>& c2toc1_input_match_pair, vector<bool>& input_pair_match_neg,
    pair<int, int>& c2toc1_output_match_pair, bool output_pair_match_neg,
    vector<pair<bool, size_t>>& const_match
)
{
    ofstream miter_file(miter_verilog_file_name);
    size_t c1_pi_size = output1[c2toc1_output_match_pair.second]->get_pi_idx_size();

    //module
    miter_file << "module miter (";
    for (size_t i = 0; i < c1_pi_size; i++)
    {
        miter_file << "in" + to_string(i) + ", ";
    }

    miter_file << "out" + to_string(0) + ") ;" << endl;

    //input line
    miter_file << "\t" << "input ";
    if (c1_pi_size > 0)
    {
        miter_file << "in" + to_string(0);
    }
    for (int i = 1; i < c1_pi_size; i++)
    {
        miter_file << ", in" + to_string(i);
    }
    miter_file << " ;" << endl;

    //output line
    miter_file << "\t" << "output ";
    miter_file << "out0";
    miter_file << " ;" << endl;

    map<string, string> c1_input2input;
    map<string, string> c2_input2input;
    vector<string> inop;
    int using_node = 0;
    int using_input = 0;

    //input connect
    for (int i = 0; i < c2toc1_input_match_pair.size(); i++)
    {
        string c1_in = input1[c2toc1_input_match_pair[i].second]->getName();
        string c2_in = input2[c2toc1_input_match_pair[i].first]->getName();

        if (c1_input2input.count(c1_in) == 0)
        {
            c1_input2input[c1_in] = "in" + to_string(using_input);
            using_input++;
        }
        if (input_pair_match_neg[i])
        {
            string node_name = "n" + to_string(using_node);
            using_node++;
            inop.push_back("not ( " + node_name + ", " + c1_input2input[c1_in] + " ) ;");
            c2_input2input[c2_in] = node_name;
        }
        else
        {
            c2_input2input[c2_in] = c1_input2input[c1_in];
        }
    }
    for (size_t i = 0; i < const_match.size(); ++i)
    {
        size_t port_idx = const_match[i].second;
        string name = input2[port_idx]->getName();
        if (const_match[i].first)   //-
        {
            c2_input2input[name] = "1'b1";
        }
        else  //+
        {
            c2_input2input[name] = "1'b0";
        }
    }

    //write wire line

    miter_file << "\t" << "wire ";
    if (using_node > 0)
    {
        miter_file << "n0";
    }
    for (size_t i = 1; i < using_node; i++)
    {
        miter_file << ", n" + to_string(i);
    }
    miter_file << ", o" + to_string(0);
    miter_file << ", o" + to_string(1);
    miter_file << " ;" << endl;

    //puts input operation
    for (size_t i = 0; i < inop.size(); i++)
    {
        miter_file << "\t" << inop[i] << endl;
    }


    //call module 1
    string C1_module_name;
    ifstream c1_file(c1_file_name);
    string line;
    while (getline(c1_file, line))
    {
        string item;
        while (line.find(';') == string::npos)
        {
            getline(c1_file, item);
            line.append(item);
        }
        if (line.find("module") != string::npos)
        {
            int pos = line.find("(");
            C1_module_name = line.substr(6, pos - 6);
            line = line.substr(pos + 1);
            line.erase(remove(line.begin(), line.end(), ' '), line.end());
           miter_file << "\tCircuit1 circuit_1( ";

            for (auto& s : c1_input2input)
            {
                string in_name = s.first;
                int pos;
                if ((pos = line.find(in_name + ",")) != string::npos)
                {
                    line.erase(pos, in_name.size() + 1);
                    line.insert(pos, "." + in_name + " (" + s.second + ") , ");
                }
                else if ((pos = line.find(in_name + ");")) != string::npos)
                {
                    line.erase(pos, in_name.size());
                    line.insert(pos, "." + in_name + " (" + s.second + ") ");
                }
            }
            string out_name = output1[c2toc1_output_match_pair.second]->getName();
            //int pos;
            if ((pos = line.find(out_name + ",")) != string::npos)
            {
                line.erase(pos, out_name.size() + 1);
                line.insert(pos, "." + out_name + " (o0) , ");
            }
            else if ((pos = line.find(out_name + ");")) != string::npos)
            {
                line.erase(pos, out_name.size());
                line.insert(pos, "." + out_name + " (o0) ");
            }
            miter_file << line << endl;
            break;
        }
    }
    c1_file.close();

    //call module 2
    string C2_module_name;
    ifstream c2_file(c2_file_name);
    while (getline(c2_file, line))
    {
        string item;
        while (line.find(';') == string::npos)
        {
            getline(c2_file, item);
            line.append(item);
        }
        if (line.find("module") != string::npos)
        {
            int pos = line.find("(");
            C2_module_name = line.substr(6, pos - 6);
            line = line.substr(pos + 1);
            line.erase(remove(line.begin(), line.end(), ' '), line.end());
            miter_file << "\tCircuit2 circuit_2( ";

            for (auto& s : c2_input2input)
            {
                string in_name = s.first;
                int pos;
                if ((pos = line.find(in_name + ",")) != string::npos)
                {
                    line.erase(pos, in_name.size() + 1);
                    line.insert(pos, "." + in_name + " (" + s.second + ") , ");
                }
                else if ((pos = line.find(in_name + ");")) != string::npos)
                {
                    line.erase(pos, in_name.size());
                    line.insert(pos, "." + in_name + " (" + s.second + ") ");
                }
            }
            string out_name = output2[c2toc1_output_match_pair.first]->getName();
            //int pos;
            if ((pos = line.find(out_name + ",")) != string::npos)
            {
                line.erase(pos, out_name.size() + 1);
                line.insert(pos, "." + out_name + " (o1) , ");
            }
            else if ((pos = line.find(out_name + ");")) != string::npos)
            {
                line.erase(pos, out_name.size());
                line.insert(pos, "." + out_name + " (o1) ");
            }
            miter_file << line << endl;
            break;
        }
    }
    c2_file.close();

    //output xor
    if (output_pair_match_neg)
    {
        miter_file << "\t" << "xnor ( out0, o0, o1 ) ;" << endl;
    }
    else
    {
        miter_file << "\t" << "xor ( out0, o0, o1 ) ;" << endl;
    }
    miter_file << "endmodule" << endl << endl;
    //puts c1 module
    ifstream c1(c1_file_name);
    while (getline(c1, line))
    {
       if (line.find("endmodule") != string::npos)
        {
            miter_file << line << endl;
            break;
        }
        string item;
        while (line.find(';') == string::npos)
        {
            getline(c1, item);
            line.append(item);
        }
        if (line.find("module") != string::npos)
        {
            line.erase(6, C1_module_name.size());
            line.insert(6, " Circuit1");
        }
        miter_file << line << endl;
    }
    c1.close();

    miter_file << endl;

    ifstream c2(c2_file_name);
    while (getline(c2, line))
    {
       if (line.find("endmodule") != string::npos)
        {
            miter_file << line << endl;
            break;
        }
        string item;
        while (line.find(';') == string::npos)
        {
            getline(c2, item);
            line.append(item);
        }
        if (line.find("module") != string::npos)
        {
            line.erase(6, C2_module_name.size());
            line.insert(6, " Circuit2");
        }
        miter_file << line << endl;
    }
    c2.close();
    miter_file.close();

    Abc_Frame_t * pAbc;
    char Command[1000];
    
    //////////////////////////////////////////////////////////////////////////
    // start the ABC framework
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();

    // string cmd = "./abc -c \"read_verilog " + miter_verilog_file_name + ";strash;" + " write_cnf " + miter_cnf_file_name + "\"";
    // system(cmd.c_str());
    sprintf( Command, "read_verilog %s; strash; write_cnf %s", &miter_verilog_file_name[0], &miter_cnf_file_name[0]);
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
    }

    //////////////////////////////////////////////////////////////////////////
    // stop the ABC framework
    Abc_Stop();
    CaDiCaL::Solver* solver = new CaDiCaL::Solver;
    int vars;
    solver->read_dimacs(miter_cnf_file_name.c_str(), vars);
    int res = solver->solve();
    delete solver;
    if (res == 10) //SAT non-eq
    {
        return 0;
    }
    else    //eq
    {
        return 1;
    }
    /*string cmd = "./cadical " + miter_cnf_file_name + " > eq.log";
    system(cmd.c_str());
    ifstream log_file("eq.log");
    while (getline(log_file, line)) {
        if (line.find("UNSATISFIABLE") != string::npos) {
            return 1;
        }
    }
    return 0;*/
}

int randint(int n) {
  if ((n - 1) == RAND_MAX) {
    return rand();
  } else {
    long end = RAND_MAX / n;
    assert (end > 0L);
    end *= n;

    int r;
    while ((r = rand()) >= end);

    return r % n;
  }
}

int miterOneOutputRandom(
    Module* m1, Module* m2,
    const vector<Node*>& input1, const vector<Node*>& input2, const vector<Node*>& output1, const vector<Node*>& output2,
    vector<pair< int, int >>& c2toc1_input_match_pair, vector<bool>& input_pair_match_neg,
    pair<int, int>& c2toc1_output_match_pair, /*bool output_pair_match_neg,*/
    vector<pair<bool, size_t>>& const_match
) 
{
    srand(0);
    random_device rd;   // non-deterministic generator
    mt19937 gen(rd());  // to seed mersenne twister.
    uniform_int_distribution<int> uniform_rand(0,1);
    unordered_map<Node*, bitset<PNUM>> piValue;  
    unordered_set<Node*> c1_input_set;
    piValue.clear();
    c1_input_set.clear();
    // initialize all pattern to zero
    bitset<PNUM> b_0(0);
    for (auto &it : input1) {
        piValue[it] = b_0;
    }
    for (auto &it : input2) {
        piValue[it] = b_0;
    }
    // assign constant value to const_match
    for (size_t i = 0; i < const_match.size(); ++i) {
        size_t port_idx = const_match[i].second;
        if (const_match[i].first) { // 1
            piValue[input2[port_idx]] = ~b_0;
        } else { // 0
            piValue[input2[port_idx]] = b_0;
        }
    }
    // calculate input size of c1
    int c1_input_size = 0;
    for (auto &p : c2toc1_input_match_pair) { // (c2_input_index, c1_input_index)
        if (!c1_input_set.count(input1[p.second])) {
            c1_input_size++;
            c1_input_set.insert(input1[p.second]);
        }
    }
    bitset<PNUM> c1_out; 
    bitset<PNUM> c2_out;
    int type = 0;
    if (c1_input_size < 18) {
        // assign all c1_input 
        int input_pattern_size = 1 << c1_input_size;
        int end_index = (c1_input_size <= 13) ? (1 << c1_input_size) : PNUM;  
        for (int time = 0; time <= (input_pattern_size >> 13); ++time) {
            for (int idx = 0; idx < end_index; ++idx) {
                int val = PNUM * time + idx;
                for (auto &c1_input : c1_input_set) {
                    piValue[c1_input][idx] = val & 1;
                    val = val >> 1;
                }
            }
            // cout << "=========================================================" << endl;
            // for (auto &c1_input : c1_input_set) {
            //     cout << piValue[c1_input] << endl;
            // }
            // cout << "=========================================================" << endl;
            // each (c2_input, c1_input) pair
            // int pair_idx = 0;
            // for (auto &p : c2toc1_input_match_pair) { 
            //     piValue[input2[p.first]] = input_pair_match_neg[pair_idx++] ? ~piValue[input1[p.second]] : piValue[input1[p.second]];
            // }
            for (size_t i = 0; i < c2toc1_input_match_pair.size(); i++) {
                piValue[input2[c2toc1_input_match_pair[i].first]] = input_pair_match_neg[i] ? ~piValue[input1[c2toc1_input_match_pair[i].second]] : piValue[input1[c2toc1_input_match_pair[i].second]];
            }
            c1_out = m1->miterRandomSimulate(output1[c2toc1_output_match_pair.second], piValue); 
            c2_out = m2->miterRandomSimulate(output2[c2toc1_output_match_pair.first], piValue);
            if (c1_out == c2_out) { // non-neg
                if (type == 0) type = 1;
                else if (type != 1) return 0;
            } else if (c1_out == ~c2_out) { // neg
                if (type == 0) type = 2;
                else if (type != 2) return 0;
            } else { // non-eq
                return 0;
            }
        }
        return type;
    } else {
        
        for (int time = 0; time < 30; time++) {
            c1_input_set.clear();
            for (size_t i = 0; i < c2toc1_input_match_pair.size(); i++) {
                // cout << "input: " << input2[c2toc1_input_match_pair[i].first]->_name << " ";
                // cout << input1[c2toc1_input_match_pair[i].second]->_name << " " << input_pair_match_neg[i] << endl;
                if (c1_input_set.count(input1[c2toc1_input_match_pair[i].second])) {
                    piValue[input2[c2toc1_input_match_pair[i].first]] = input_pair_match_neg[i] ? ~piValue[input1[c2toc1_input_match_pair[i].second]] : piValue[input1[c2toc1_input_match_pair[i].second]];
                    // for (int idx = 0; idx < 8192; idx++) {
                    //     int val = piValue[input1[c2toc1_input_match_pair[i].second]][idx];
                    //     piValue[input2[c2toc1_input_match_pair[i].first]][idx] = input_pair_match_neg[i] ? !val : val;
                    // }
                } else {
                    for (int idx = 0; idx < 8192; idx++) {
                        int val = rand() & 1;
                        piValue[input1[c2toc1_input_match_pair[i].second]][idx] = val;
                        piValue[input2[c2toc1_input_match_pair[i].first]][idx] = input_pair_match_neg[i] ? !val : val;
                    }
                    c1_input_set.insert(input1[c2toc1_input_match_pair[i].second]);
                }
            }
            c1_out = m1->miterRandomSimulate(output1[c2toc1_output_match_pair.second], piValue); 
            c2_out = m2->miterRandomSimulate(output2[c2toc1_output_match_pair.first], piValue);

            if (c1_out == c2_out) { // non-neg
                if (type == 0) type = 1;
                else if (type != 1) return 0;
            } else if (c1_out == ~c2_out) { // neg
                if (type == 0) type = 2;
                else if (type != 2) return 0;
            } else { // non-eq
                return 0;
            }
        }
        return type;
    }
}
