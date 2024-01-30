/*
    Title: Parser
    Features: Parse gate-level netlist .v  into our structure
    Author: You-Cheng Lin
    Date: 2023/07/14
    Note: (1) Speed up by taking out the TraceInput()
*/
#ifndef PARSER_CPP
#define PARSER_CPP
#include "Netlist.h"

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

// Parse the input file and return the path of two circuits
void ParseInput(string in, vector<string>& file_name, vector<vector<string>>* bus)
{
    string line;
    int bus_num, bus_sz;
    // Open the input file
    ifstream ifs(in);
    // ./release/casexx/
    int found = in.find_last_of('/');
    // ./release/
    int head = in.substr(0,found).find_last_of('/');
    // casexx
    if(NAMEC) file_name.push_back(in.substr(head+1,6));
    else
    {
        file_name.push_back(string("case"));
        found = in.length()-6;
    }
    for(int cir = 0;cir<2;cir++)
    {
        // Get the line of circuit.v
        ifs >> line;
        cout << "Circuit " << cir << " = " << line << endl;
        file_name.push_back(in.substr(0,found+1) + line);
        // Get bus
        ifs >> bus_num;
        bus[cir].resize(bus_num);
        for(int i=0;i<bus_num;i++)
        {
            ifs >> bus_sz;
            bus[cir][i].resize(bus_sz);
            for(int j=0;j<bus_sz;j++)
            {
                ifs >> bus[cir][i][j];
            }
        }
    }

    show(file_name);
}
// Remove all space in a string
void RemoveSpace(string& str)
{
    str.erase(remove(str.begin(),str.end(),' '),str.end());
    if(str[0] == '\\') str.erase(str.begin());
}
// Remove '\n' until ';' (Don't add anything in the back of ';')
void MakeLine(ifstream& ifs, string& line)
{
    if(line == "endmodule") return;
    string item;
    while(line.find(";") == string::npos)
    {
        getline(ifs,item);
        line.append(item);
    }
}
// Delete gate names and form a new verilog file
void DeleteGateName(string in, string out)
{
    ifstream ifs(in);
    ofstream ofs(out);
    string line;
    stringstream ss;
    
    // Define the type mapping from type name to enum type

    while(getline(ifs,line))
    {
        // Make a correct line with ;
        MakeLine(ifs,line);
        string item;
        // Initialize streamstring ss
        ss.str("");
        ss.clear();
        ss << line;
        // Get operation
        while(item == "") getline(ss,item,' ');

        //show(item);

        // Judge the operations
        if(item == "module")
        {
            // Get the name of the module
            getline(ss,item,'(');
            ofs << "module " << item << "(";

            // Get the parameters of the module
            while(getline(ss,item,','))
            {
                RemoveSpace(item);
                if(item.back() == ';')
                {
                    ofs << item << "\n";
                    item.erase(item.end()-2,item.end());
                }
                else ofs << item << ", ";
            }
        }
        else if(item == "wire" || item == "input" || item == "output")
        {
            ofs << line << "\n";
        }
        else if(item == "endmodule")
        {

            ofs << line << "\n";

            // Change to the next module
            // Close the file
            ofs.close();
            ifs.close();
            break;
        }
        else
        {
            // Record the length from 0 to first '('
            auto offset = line.find('(');
            ofs << "  " << item << " ";
            // Save the gate type
            // Find gate name
            getline(ss,item,'(');
            ofs << &line[offset] << "\n";
            // Create a gate node
            RemoveSpace(item);
            // tmp name

            // Collect names of parameters of the gate
            vector<string> tmpStr;
            //tmpStr.push_back(item);

            while(getline(ss,item,','))
            {
                if(item.back() == ';')
                {
                    item.erase(item.end()-2,item.end());
                }
                RemoveSpace(item);
                tmpStr.push_back(item);
                //show(item);
            }
            
           
        }
    }
}
void ABCRebuild(vector<string>& file_name, string* del_out, string* abc_map_out)
{
    Abc_Frame_t * pAbc;
    char Command[1000];
    
    //////////////////////////////////////////////////////////////////////////
    // start the ABC framework
    Abc_Start();
    pAbc = Abc_FrameGetGlobalFrame();
    
    // string map_path = "./map/" + file_name[0];
    string map_path = file_name[0];

    del_out[0] = map_path + "_del_out1.v";
    del_out[1] = map_path + "_del_out2.v";
    abc_map_out[0] = map_path + "_abc_map_out1.v";
    abc_map_out[1] = map_path + "_abc_map_out2.v";
    DeleteGateName(file_name[1], del_out[0]);
    DeleteGateName(file_name[2], del_out[1]);
    
    // mapping using ABC tool
    // string cmd = "chmod 777 ./abc";
    string resync2 = "balance; rewrite -l; refactor -l; balance; rewrite -l; rewrite -lz; balance; refactor -lz; rewrite -lz; balance;";
    // system(cmd.c_str());

    // cmd = "./abc -c \"read_library cadence.genlib; read_verilog " + del_out[0] + ";strash;" + resync2 + " map; write_verilog " + abc_map_out[0] + "\"";
    // system(cmd.c_str());
    sprintf( Command, "read_library cadence.genlib; read_verilog %s; strash", &del_out[0][0]);
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
    }
    sprintf( Command, "%s", &resync2[0]);
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
    }
    sprintf( Command, "map; write_verilog %s", &abc_map_out[0][0]);
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
    }


    // cmd = "./abc -c \"read_library cadence.genlib; read_verilog " + del_out[1] + ";strash;" + resync2 + " map; write_verilog " + abc_map_out[1] + "\"";
    // system(cmd.c_str());
    sprintf( Command, "read_library cadence.genlib; read_verilog %s; strash", &del_out[1][0]);
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
    }
    sprintf( Command, "%s", &resync2[0]);
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
    }
    sprintf( Command, "map; write_verilog %s", &abc_map_out[1][0]);
    if ( Cmd_CommandExecute( pAbc, Command ) )
    {
        fprintf( stdout, "Cannot execute command \"%s\".\n", Command );
    }

    //////////////////////////////////////////////////////////////////////////
    // stop the ABC framework
    Abc_Stop();
}

int Parser(ifstream& ifs, vector<Module*>& myModule, int moduleIndex, int bit)
{
    string line;
    stringstream ss;
    int gateIndex = 0; // Number the gates
    int wireCounter = 0;

    while(getline(ifs,line))
    {
        // Make a correct line with ;
        MakeLine(ifs,line);
        string item;
        // Initialize streamstring ss
        ss.str("");
        ss.clear();
        ss << line;
        // Get operation
        while(item == "") getline(ss,item,' ');

        //show(item);

        // Judge the operations
        if(item == "module")
        {
            // Get the name of the module
            getline(ss,item,'(');
            Module* newModule = new Module(item,moduleIndex,bit);
            // Push the new module to myModule
            myModule.emplace_back(newModule);
            cout << "Start module " << moduleIndex << "...\n";
        }
        else if(item == "wire" || item == "input" || item == "output")
        {
            //if(item != "wire") ofs << line << "\n";
            string op = item;
            getline(ss,item,',');
            //show(item);
            RemoveSpace(item);

            int n = 0,m = 0;// 0: single bit ; >0: multi bits
            if(item[0] == '[')
            {
                auto found = item.find(':')+1;
                n = stoi(&item[1],0);
                m = stoi(&item[found],0);
                found = item.find(']');
                item.erase(0,found+1);
            }
            // Put items in container
            do
            {
                if(item.back() == ';') item.pop_back();
                RemoveSpace(item);

                //show(item);
                // Create a new node, add to the netlist and show its information
                if(n == 0 && m == 0)
                {
                    // Check if the name have existed
                    if(myModule[moduleIndex]->NameToNode(item) != NULL) continue;
                    Node* np = new Node(item,TypeNum[op]);
                    myModule[moduleIndex]->AddNode(np);
                    //np->showNode();
                    if(TypeNum[op] == WIRE) wireCounter++;
                }
                else
                {
                    if(n>m) swap(n,m);
                    for(int i=n; i<=m; i++) // name[i]
                    {
                        // Deal with index
                        string tmp = item;
                        tmp.append("[]");
                        tmp.insert(tmp.length()-1,to_string(i));
                        // Create node
                        if(myModule[moduleIndex]->NameToNode(tmp) != NULL) break;
                        Node* np = new Node(tmp,TypeNum[op]);
                        myModule[moduleIndex]->AddNode(np);
                        //np->showNode();
                        if(TypeNum[op] == WIRE) wireCounter++;
                    }
                }

            }
            while(getline(ss,item,','));
        }
        else if(item == "endmodule")
        {
            cout << "The number of wires is " << wireCounter << ".\n";
            cout << "End module " << moduleIndex << "...\n\n";

            // Delete wires
            myModule[moduleIndex]->Simplification();
            show("Finish simplification.");

            // Show structure
            if(bit == 0)
            {
                myModule[moduleIndex]->printBT();
                show("Finish BT.");
            }

            // Close the file
            ifs.close();
            show("Finish parsing.");
            break;
        }
        else
        {
            // Record the length from 0 to first '('
            // Save the gate type
            Type gateType = TypeNum[item];
            if(gateType == NONE) err("Syntax error!");
            // Find gate name
            getline(ss,item,'(');
            // Create a gate node
            RemoveSpace(item);
            // tmp name
            if(item == "") item = "g" + to_string(gateIndex++);
            Node* gate = new Node(item,gateType);
            myModule[moduleIndex]->AddNode(gate);

            // Collect names of parameters of the gate
            vector<string> tmpStr;
            //tmpStr.push_back(item);

            while(getline(ss,item,','))
            {
                if(item.back() == ';')
                {
                    item.erase(item.end()-2,item.end());
                }
                RemoveSpace(item);
                tmpStr.push_back(item);
                //show(item);
            }

            // Use tmpStr to connect nodes
            myModule[moduleIndex]->Connect(gate,tmpStr);
        }
    }
    myModule[moduleIndex]->TraceInputs();
    return wireCounter;
}
#endif
