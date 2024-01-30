/*
    Title: Matching
    Features: Match the outputs and inputs
    Date: 2023/08/30
    Note: (1) Remember to set time limit
*/
#include "Netlist.h"
#include "Parser.cpp"
#include "miter.cpp"
#include "Output.h"

#define TIMELIMIT 30.0
#define CONST_VAL -2

bool isInTime()
{
    return clock()/CLOCKS_PER_SEC < TIMELIMIT;
}

vector<vector<int>> getComb(vector<int> N, int K) {
    vector<vector<int>> combs;
    std::string bitmask(K, 1); // K leading 1's
    bitmask.resize(N.size(), 0); // N-K trailing 0's
    // print integers and permute bitmask
    do {
        vector<int> comb;
        for (int i = 0; i < N.size(); ++i) // [0..N-1] integers
        {
            if (bitmask[i]) comb.push_back(N[i]);
        }
        combs.push_back(comb);
    } while (std::prev_permutation(bitmask.begin(), bitmask.end()));
    return combs;
}


int64_t factor(int64_t n)
{
    int64_t x;
    if(n > 0) return n*factor(n-1);
    return 1;
}

void SimTest(vector<Module*>& myModule)
{
    vector<vector<Signal>> input_signal(myModule[0]->getBusSz());
    input_signal[0] = {DONTCARE,ZERO,ONE,ONE};
    input_signal[1] = {DONTCARE,ZERO,ONE,ZERO};
    input_signal[2] = {DONTCARE,ZERO,ONE,ONE};
    myModule[0]->InitInput(); // Reset all signals
    myModule[0]->AssignBus(input_signal[0],0);
    myModule[0]->AssignBus(input_signal[1],1);
    myModule[0]->AssignBus(input_signal[2],2);
    cout << "n34 = " << myModule[0]->getSignal("n34") << "\n";
}

class Match
{
public:
    // Construct
    Match(vector<Node*>& _input1, vector<Node*>& _input2, vector<Node*>& _output1, vector<Node*>& _output2)  
    : input1(_input1), input2(_input2), output1(_output1), output2(_output2) {
        PI1Size = input1.size();
        PI2Size = input2.size();
        PO1Size = output1.size();
        PO2Size = output2.size();
    };
    void setSignature() {}
    void setTT(vector<vector<bool>> t1, vector<vector<bool>> t2)
    {
        _truth_table[0] = t1;
        _truth_table[1] = t2;
    }
    void setOnSz(vector<uint64_t> sz1, vector<uint64_t> sz2)
    {
        _onset_size[0] = sz1;
        _onset_size[1] = sz2;
    }
    void setOSV(vector<vector<uint64_t>> osv1, vector<vector<uint64_t>> osv2)
    {
        _OSV[0] = osv1;
        _OSV[1] = osv2;
    }
    void MatchAlgo(vector<Module*> myModule,int caseID)
    {
        _max_bus_diff = (int)myModule[1]->_gate[INPUT].size() - (int)myModule[0]->_gate[INPUT].size();
        _init_output.resize(myModule[1]->_gate[OUTPUT].size());
        _init_input.resize(myModule[1]->_gate[INPUT].size(), NULL);
        _init_input_ne.resize(myModule[1]->_gate[INPUT].size(), -1);
        // Compute the number of XOR and AND gates in the paths from POs to PIs
        for(int moduleID = 0; moduleID < 2; moduleID++)
        {
            cout << "In module " << moduleID << ":\n=============================================\n";
            myModule[moduleID]->CountGate();
            show("Finish Gate Count.");
            myModule[moduleID]->OutputConfig(caseID,_max_bus_diff);
            show("Finish Output Configure.");
            if(caseID == 3) myModule[moduleID]->PartitionGroup();
            show("Finish Partition Groups.");
            if(caseID == 8) myModule[moduleID]->ForkCtrlNetlist(moduleID == 0 ? 2 : 3);
            show("Finish Fork Control Netlists.");
        }
        if(caseID == 9)
        {
            for(auto x:myModule[0]->_bus_set[31])
            {
                if(x->_name == "n1112") continue;
                x->_isFlip = 1;
            }
            for(auto x:myModule[0]->_bus_set[16])
            {
                if(x->_bit_idx == 6) x->_isFlip = 1;
            }
            for(auto x:myModule[0]->_bus_set[20])
            {
                if(x->_bit_idx <= 7 && x->_bit_idx >= 5) x->_isFlip = 1;
            }
            // Input
            for(auto x:myModule[0]->_bus_set[42])
            {
                if(x->_bit_idx == 6) x->_isFlip = 1;
            }
            for(auto x:myModule[0]->_bus_set[46])
            {
                if(x->_bit_idx == 5) x->_isFlip = 1;
            }

            int cnt = 0;
            for(auto eq:myModule[1]->_allEquation)
            {
                eq->_const = cnt;
                cnt++;
            }
            myModule[1]->_allEquation[6]->_const = 7;
            myModule[1]->_allEquation[7]->_const = 9;
            myModule[1]->_allEquation[8]->_const = 6;
            myModule[1]->_allEquation[9]->_const = 8;
            sort(myModule[1]->_allEquation.begin(),myModule[1]->_allEquation.end(),[](Operation* a,Operation* b)
            {
                return a->_const < b->_const;
            });
        }
        else if(caseID == 3)
        {
            for(auto op:myModule[1]->_group)
            {
                op->SwapGroup(1,2);
                op->SwapGroup(3,4);
                op->SwapGroup(5,7);
                myModule[1]->ComputeNOTEncode(op);
            }
            sort(myModule[1]->_group.begin(),myModule[1]->_group.end(),[](Operation* a,Operation* b)
            {
                return a->_const < b->_const;
            });
            cout << "After swap:\n";
            myModule[1]->showGroup();
            int i;
            for(i=0;i<myModule[0]->_group.size();++i)
            {
                if(myModule[0]->_group[i]->getConst() != myModule[1]->_group[i]->getConst())
                {
                    cout << "Wrong Match!\n";
                    break;
                }
            }
            if(i == myModule[0]->_group.size()) cout << "Correct Match!\n";

        }
        else if(caseID == 8)
        {
            vector<vector<Node*>> match_ctrl(1 << myModule[0]->_bus_set[4].size());
            // Permute output match
            myModule[0]->_bus_set[4][0]->_bit_idx = 1;
            myModule[0]->_bus_set[4][1]->_bit_idx = 3;
            myModule[0]->_bus_set[4][2]->_bit_idx = 2;
            myModule[0]->_bus_set[4][3]->_bit_idx = 0;
            myModule[0]->_bus_set[4][0]->_isFlip = true;
            myModule[0]->_bus_set[4][2]->_isFlip = true;
            sort(myModule[0]->_bus_set[4].begin(),myModule[0]->_bus_set[4].end(),[](Node* a,Node* b)
            {
                return a->_bit_idx < b->_bit_idx;
            });
            // Write output match pair
            _init_output[myModule[1]->_name_to_node["n29"]->_idx].push_back(myModule[0]->_name_to_node["n5"]);
            _init_output[myModule[1]->_name_to_node["n194"]->_idx].push_back(myModule[0]->_name_to_node["n142"]);
            _init_output[myModule[1]->_name_to_node["n245"]->_idx].push_back(myModule[0]->_name_to_node["n153"]);
            for(int i=0;i<myModule[1]->_bus_set[4].size();++i)
            {
                _init_output[myModule[1]->_bus_set[4][i]->_idx].push_back(myModule[0]->_bus_set[4][i]);
            }
            cout << "Module 0 encode:";
            for(auto x:myModule[0]->_bus_set[4]) cout << x->_name << (x->_isFlip ? "\' ":" ");
            cout << endl;
            for(int i=0;i<myModule[0]->_ctrl_signal.size()-1;++i) // Control size
            {
                cout << myModule[0]->_ctrl_signal[i]->_name << ": " << myModule[0]->getBus4Encode(i) << endl;
                match_ctrl[myModule[0]->getBus4Encode(i)].push_back(myModule[0]->_ctrl_signal[i]);
            }
            cout << "Module 1 encode:";
            for(auto x:myModule[1]->_bus_set[4]) cout << x->_name << (x->_isFlip ? "\' ":" ");
            cout << endl;
            for(int i=0;i<myModule[1]->_ctrl_signal.size();++i) // Control size
            {
                cout << myModule[1]->_ctrl_signal[i]->_name << ": " << myModule[1]->getBus4Encode(i) << endl;
                match_ctrl[myModule[1]->getBus4Encode(i)].push_back(myModule[1]->_ctrl_signal[i]);
            }
            for(auto v:match_ctrl)
            {
                if(v.size() > 1)
                {
                    _init_input[v[1]->_idx] = v[0];
                    _init_input_ne[v[1]->_idx] = false;
                    cout << "Initial match " << v[1]->_name << " -> " << v[0]->_name << endl;
                    for(int i=5;i<8;++i)
                    {
                        string name1 = myModule[0]->_first_pi[v[0]->_bit_idx][myModule[0]->_bus_set[i][0]->_idx].first;
                        string name2 = myModule[1]->_first_pi[v[1]->_bit_idx][myModule[1]->_bus_set[i][0]->_idx].first;
                        bool isNeg1 = myModule[0]->_first_pi[v[0]->_bit_idx][myModule[0]->_bus_set[i][0]->_idx].second;
                        bool isNeg2 = myModule[1]->_first_pi[v[1]->_bit_idx][myModule[1]->_bus_set[i][0]->_idx].second;
                        if(name1 != "1'b0") 
                        {
                            _init_input[myModule[1]->_name_to_node[name2]->_idx] = myModule[0]->_name_to_node[name1];
                            _init_input_ne[myModule[1]->_name_to_node[name2]->_idx] = isNeg1^isNeg2;
                            cout << "Initial match " << name2 << " -> " << name1 << (isNeg1^isNeg2 ? "\'\n":"\n");
                        }
                        
                    }
                }
            }
            for(int ID=0;ID<2;++ID)
            for(auto x:myModule[ID]->_gate[INPUT])
            {
                x->_bit_idx = -1;
            }
        }

        // Resize the container
        _match_pair.resize(myModule[0]->_gate[OUTPUT].size());
        _output_ne.resize(myModule[1]->_gate[OUTPUT].size());
        _in_match_pair.resize(myModule[0]->_gate[INPUT].size());
        _input_ne.resize(myModule[1]->_gate[INPUT].size());
        if(!myModule[0]->_allEquation.empty() && !myModule[1]->_allEquation.empty())
        {
            if(caseID == 10) setInitSolution(myModule,caseID);
        }
        if(!myModule[0]->_group.empty() && !myModule[1]->_group.empty()) setGroupMatch(myModule,caseID);
    }
    void setBusMatch(vector<Module*>& myModule,int bus_idx1, int bus_idx2)
    {
        if(myModule[1]->_bus_set[bus_idx2].size() < myModule[0]->_bus_set[bus_idx1].size()) err("Different bus size!");
        if(bus_idx1 == 9 && myModule[0]->_bus_set[bus_idx1].size() == 2)
        {
            _in_match_pair[myModule[0]->_bus_set[bus_idx1][0]->_idx].push_back(myModule[1]->_bus_set[bus_idx2][0]->_idx);
            for(int i=1,j=0;i<myModule[1]->_bus_set[bus_idx2].size();++i)
            {
                int cir2_idx = myModule[1]->_bus_set[bus_idx2][i]->_idx;
                if(myModule[1]->_bus_set[bus_idx2][i]->_bit_idx == 4)
                {
                    _const_match.push_back(make_pair(false,cir2_idx));
                    cout << "Assign " << myModule[1]->_bus_set[bus_idx2][i]->_name << " as 0 at bit-" << myModule[1]->_bus_set[bus_idx2][i]->_bit_idx << "\n";
                }
                else _in_match_pair[myModule[0]->_bus_set[bus_idx1][1]->_idx].push_back(cir2_idx);
            }
        }
        else if(bus_idx1 == 44 && myModule[0]->_bus_set[bus_idx1].size() == 5)
        {
            for(int i=0,j=0;i<myModule[1]->_bus_set[bus_idx2].size();++i)
            {
                int cir2_idx = myModule[1]->_bus_set[bus_idx2][i]->_idx;
                if(myModule[1]->_bus_set[bus_idx2][i]->_bit_idx < 5)
                {
                    int cir1_idx = myModule[0]->_bus_set[bus_idx1][j]->_idx;
                    _in_match_pair[cir1_idx].push_back(cir2_idx);
                    ++j;
                }
                else _in_match_pair[myModule[0]->_bus_set[bus_idx1][0]->_idx].push_back(cir2_idx);
            }
            cout << "DD " << myModule[0]->_bus_set[bus_idx1][0]->_bit_idx << endl;
        }
        else if(bus_idx1 == 13 && myModule[0]->_bus_set[bus_idx1].size() == 7)
        {
            for(int i=0,j=0;i<myModule[1]->_bus_set[bus_idx2].size();++i)
            {
                int cir2_idx = myModule[1]->_bus_set[bus_idx2][i]->_idx;
                if(myModule[1]->_bus_set[bus_idx2][i]->_bit_idx != 6)
                {
                    int cir1_idx = myModule[0]->_bus_set[bus_idx1][j]->_idx;
                    _in_match_pair[cir1_idx].push_back(cir2_idx);
                    ++j;
                }
                else _in_match_pair[myModule[0]->_bus_set[bus_idx1][1]->_idx].push_back(cir2_idx);
            }
            cout << "DD " << myModule[0]->_bus_set[bus_idx1][1]->_bit_idx << endl;
        }
        else
        {
            for(int i=0,j=0;i<myModule[1]->_bus_set[bus_idx2].size();++i)
            {
                int cir2_idx = myModule[1]->_bus_set[bus_idx2][i]->_idx;
                if(j >= myModule[0]->_bus_set[bus_idx1].size())
                {
                    _const_match.push_back(make_pair(true,cir2_idx));
                    cout << "Assign " << myModule[1]->_bus_set[bus_idx2][i]->_name << " as 1 at bit-" << myModule[1]->_bus_set[bus_idx2][i]->_bit_idx << "\n";
                }
                else if(myModule[0]->_bus_set[bus_idx1][j]->_bit_idx < myModule[1]->_bus_set[bus_idx2][i]->_bit_idx) // set constant
                {
                    _const_match.push_back(make_pair(false,cir2_idx));
                    cout << "Assign " << myModule[1]->_bus_set[bus_idx2][i]->_name << " as 0 at bit-" << myModule[1]->_bus_set[bus_idx2][i]->_bit_idx << "\n";
                }
                else
                {
                    if(j >= myModule[0]->_bus_set[bus_idx1].size()) err("Wrong bus index");
                    int cir1_idx = myModule[0]->_bus_set[bus_idx1][j]->_idx;
                    if(cir1_idx >= _in_match_pair.size()) err("Wrong input index!");
                    _in_match_pair[cir1_idx].push_back(cir2_idx);
                    ++j;
                }
            }
        }
    }
    void setEQMatch(vector<Module*>& myModule, int caseID)
    {
        // Resize the container
        _match_pair.resize(myModule[0]->_gate[OUTPUT].size());
        _output_ne.resize(myModule[1]->_gate[OUTPUT].size());
        _in_match_pair.resize(myModule[0]->_gate[INPUT].size());
        _input_ne.resize(myModule[1]->_gate[INPUT].size());

        for(int i=0;i<myModule[0]->_allEquation.size();++i)
        {
            for(int j=0;j<myModule[0]->_allEquation[i]->_output.size();++j)
            {
                int cir1_idx = myModule[0]->_allEquation[i]->_output[j]->_idx;
                int cir2_idx = myModule[1]->_allEquation[i]->_output[j]->_idx;
                if(cir1_idx >= _match_pair.size()) err("Wrong output index");
                _match_pair[cir1_idx].push_back(cir2_idx);
            }
            cout << "Finish Match output of eq" << i << ": bus = " << myModule[0]->_allEquation[i]->_output[0]->_bus_idx
            << ", bus = " << myModule[1]->_allEquation[i]->_output[0]->_bus_idx << "\n";
            if(caseID != 9)
            {
                // Match Inputs
                for(int j=0;j<myModule[0]->_allEquation[i]->_real_input.size();++j)
                {
                    // A term
                    if(j >= myModule[1]->_allEquation[i]->_real_input.size())
                    {
                        cout << "In eq" << i << ", input" << j << ":\n";
                        err("No corresponding real input in module 1.");
                    }
                    for(int k=0;k<myModule[0]->_allEquation[i]->_real_input[j]->_member.size();++k)
                    {
                        if(k >= myModule[1]->_allEquation[i]->_real_input[j]->_member.size())
                        {
                            cout << "In eq" << i << ", input" << j << ", member" << k << ":\n";
                            err("No corresponding term of the real input in module 1.");
                        }
                        int bus_idx1 = myModule[0]->_allEquation[i]->_real_input[j]->_member[k];
                        int bus_idx2 = myModule[1]->_allEquation[i]->_real_input[j]->_member[k];
                        if(bus_idx1 >= myModule[0]->_bus_set.size() || bus_idx1 < 0)
                        {
                            cout << "Bus1 = " << bus_idx1 << endl;
                            err("Wrong bus index");
                        }
                        else if(bus_idx2 >= myModule[1]->_bus_set.size() || bus_idx2 < 0) // Different size equations in two circuits
                        {
                            cout << "Bus2 = " << bus_idx2 << endl;
                            err("Wrong bus index");
                        }
                        cout << "Finish real input bus " << bus_idx1 << ": " <<  myModule[0]->_bus_set[bus_idx1].back()->_name << "(" << myModule[0]->_bus_signature[bus_idx1] << "), bus "
                        << bus_idx2 << ": " << myModule[1]->_bus_set[bus_idx2].back()->_name << "(" << myModule[1]->_bus_signature[bus_idx2] << ")\n";
                        for(int l=0;l<myModule[0]->_bus_set[bus_idx1].size();++l)
                        {
                            if(l >= myModule[1]->_bus_set[bus_idx2].size()) err("Different bus size!");
                            int cir1_idx = myModule[0]->_bus_set[bus_idx1][l]->_idx;
                            int cir2_idx = myModule[1]->_bus_set[bus_idx2][l]->_idx;
                            if(cir1_idx >= _in_match_pair.size()) err("Wrong input index!");
                            _in_match_pair[cir1_idx].push_back(cir2_idx);
                        }
                    }
                }
            }
            
        }
        if(caseID == 9)
        {
            vector<int> bus1 = {27,34,41,42,
                                2,19,35,44,
                                0,10,37,40,
                                36,22,39,12,
                                21,1,46,11,
                                15,24,45,9,
                                13,32,33,17,
                                6,29,14,30}; 
            vector<int> bus2 = {21,24,27,36,
                                29,40,2,30,
                                0,46,25,41,
                                34,15,11,12,
                                31,26,8,44,
                                16,35,10,38,
                                4,43,9,32,
                                3,39,23,14};
            for(int i=0;i<32;++i) setBusMatch(myModule,bus1[i],bus2[i]);
        }
        for(auto& mp:_match_pair) Unique(mp);
        for(auto& mp:_in_match_pair) Unique(mp);
        show("Finish Match cir1 and cir2!");
        // Write bit flip on outputs
        for(int i=0;i<_match_pair.size();++i) // i is the index of myModule[0]->_gate[OUTPUT]
        {
            for(int j=0;j<_match_pair[i].size();++j)
            {
                _output_ne[_match_pair[i][j]] = (myModule[0]->_gate[OUTPUT][i]->_isFlip) ^ (myModule[1]->_gate[OUTPUT][_match_pair[i][j]]->_isFlip);
            }
        }
        show("Finish Write outputs negation.");
        // Write bit flip on inputs
        for(int i=0;i<_in_match_pair.size();++i) // i is the index of myModule[0]->_gate[INPUT]
        {
            for(int j=0;j<_in_match_pair[i].size();++j)
            {
                _input_ne[_in_match_pair[i][j]] = (myModule[0]->_gate[INPUT][i]->_isFlip) ^ (myModule[1]->_gate[INPUT][_in_match_pair[i][j]]->_isFlip);
            }
        }
        show("Finish Write inputs negation.");
        //for(int i=0;i<_match_pair.size();i++) cout << "Match " << myModule[0]->_gate[OUTPUT][i]->_name << ": " << myModule[1]->_gate[OUTPUT][_match_pair[i][0]]->_name << endl;
    }
    void setGroupMatch(vector<Module*>& myModule, int caseID)
    {
        // Resize the container
        _match_pair.resize(myModule[0]->_gate[OUTPUT].size());
        _output_ne.resize(myModule[1]->_gate[OUTPUT].size());
        _in_match_pair.resize(myModule[0]->_gate[INPUT].size());
        _input_ne.resize(myModule[1]->_gate[INPUT].size());

        for(int i=0;i<myModule[0]->_group.size();++i)
        {
            cout << "In group" << i << endl;
            for(int j=0;j<myModule[0]->_group[i]->_output.size();++j)
            {
                int cir1_idx = myModule[0]->_group[i]->_output[j]->_idx;
                int cir2_idx = myModule[1]->_group[i]->_output[j]->_idx;
                if(cir1_idx >= _match_pair.size()) err("Wrong output index");
                _match_pair[cir1_idx].push_back(cir2_idx);
            }
            cout << "Finish Match output of group" << i << ": bus = " << myModule[0]->_group[i]->_output[0]->_bus_idx
            << ", bus = " << myModule[1]->_group[i]->_output[0]->_bus_idx << "\n";
              // Match Inputs
            for(int j=0;j<myModule[0]->_group[i]->_real_input.size();++j)
            {
                // A term
                if(j >= myModule[1]->_group[i]->_real_input.size())
                {
                    cout << "In group" << i << ", input" << j << ":\n";
                    err("No corresponding real input in module 1.");
                }
                if(myModule[0]->_group[i]->_real_input[j]->_coeff == 0)
                for(int k=0;k<myModule[0]->_group[i]->_real_input[j]->_member.size() && k<3;++k)
                {
                    if(k >= myModule[1]->_group[i]->_real_input[j]->_member.size())
                    {
                        cout << "In group" << i << ", input" << j << ", member" << k << ":\n";
                        err("No corresponding term of the real input in module 1.");
                    }
                    int cir1_idx = myModule[0]->_group[i]->_real_input[j]->_member[k];
                    int cir2_idx = myModule[1]->_group[i]->_real_input[j]->_member[k];
                    _in_match_pair[cir1_idx].push_back(cir2_idx);
                }
            }
        }
        for(auto& mp:_match_pair) Unique(mp);
        for(auto& mp:_in_match_pair) Unique(mp);
        show("Finish Match cir1 and cir2!");
        // Write bit flip on outputs
        for(int i=0;i<_match_pair.size();++i) // i is the index of myModule[0]->_gate[OUTPUT]
        {
            for(int j=0;j<_match_pair[i].size();++j)
            {
                _output_ne[_match_pair[i][j]] = (myModule[0]->_gate[OUTPUT][i]->_isFlip) ^ (myModule[1]->_gate[OUTPUT][_match_pair[i][j]]->_isFlip);
            }
        }
        show("Finish Write outputs negation.");
        // Write bit flip on inputs
        for(int i=0;i<_in_match_pair.size();++i) // i is the index of myModule[0]->_gate[INPUT]
        {
            for(int j=0;j<_in_match_pair[i].size();++j)
            {
                _input_ne[_in_match_pair[i][j]] = (myModule[0]->_gate[INPUT][i]->_isFlip) ^ (myModule[1]->_gate[INPUT][_in_match_pair[i][j]]->_isFlip);
            }
        }
        show("Finish Write inputs negation.");
    }

    void setInitSolution(vector<Module*>& myModule, int caseID)
    {
        if(myModule[0]->_allEquation.size() != myModule[1]->_allEquation.size())
        {
            cout << "Error: The number of equations are not the same.\n";
            return;
        }
        _bus_map[1] = vector<int>(myModule[1]->_bus_set.size(),-1);
        _init_input_ne = vector<int>(myModule[1]->_gate[INPUT].size(),-1);
        cout << "Module 0:\n";
        for(auto op:myModule[0]->_allEquation) op->showOp();
        cout << "Module 1:\n";
        for(auto op:myModule[1]->_allEquation) op->showOp();
        for(int i=0;i<myModule[0]->_allEquation.size();++i)
        {
            cout << "In eq" << i << endl;
            if(myModule[0]->_allEquation[i]->_output.size() != myModule[1]->_allEquation[i]->_output.size())
            {
                cout << "size = " << myModule[0]->_allEquation[i]->_output.size() << ", " << myModule[1]->_allEquation[i]->_output.size() << endl;
                cout <<"Different output size!\n";
                return;
            }
            _bus_map[1][myModule[1]->_allEquation[i]->_output[0]->_bus_idx] = myModule[0]->_allEquation[i]->_output[0]->_bus_idx;
            cout << "Finish Match output of eq" << i << ": bus = " << myModule[1]->_allEquation[i]->_output[0]->_bus_idx
            << " -> bus = " << myModule[0]->_allEquation[i]->_output[0]->_bus_idx << "\n";
            // Match Inputs
            for(int j=myModule[0]->_allEquation[i]->_output.size()-1;j>=0;--j)
            {
                int cir2_idx = myModule[1]->_allEquation[i]->_output[j]->_idx;
                if(j>10)myModule[1]->_allEquation[i]->_output[j]->_priority = 10;
                //_init_output[cir2_idx] = myModule[0]->_allEquation[i]->_output[j];
                for(int k=j;k<myModule[0]->_allEquation[i]->_output.size();++k)
                {
                    cir2_idx = myModule[1]->_allEquation[i]->_output[k]->_idx;
                    _init_output[cir2_idx].push_back(myModule[0]->_allEquation[i]->_output[j]);
                    cout << "match output " << myModule[1]->_allEquation[i]->_output[k]->_name << " -> " << myModule[0]->_allEquation[i]->_output[j]->_name << endl;
                    if(j!=k)
                    {
                        cir2_idx = myModule[1]->_allEquation[i]->_output[j]->_idx;
                        _init_output[cir2_idx].push_back(myModule[0]->_allEquation[i]->_output[k]);
                        cout << "match output " << myModule[1]->_allEquation[i]->_output[j]->_name << " -> " << myModule[0]->_allEquation[i]->_output[k]->_name << endl;
                    }
                    if(myModule[0]->_allEquation[i]->_output[j]->_bit_idx == myModule[1]->_allEquation[i]->_output[k]->_bit_idx) break;
                }
                //cout << "match output " << myModule[1]->_allEquation[i]->_output[j]->_name << " -> " << myModule[0]->_allEquation[i]->_output[j]->_name << endl;

                vector<Node*> cir1_same_bit = myModule[0]->_allEquation[i]->_output[j]->_same_input_bit;
                vector<Node*> cir2_same_bit = myModule[1]->_allEquation[i]->_output[j]->_same_input_bit;
                int diff = (int)cir2_same_bit.size() - (int)cir1_same_bit.size();
                int offset = 0;
                for(int k=0;k<cir1_same_bit.size();++k)
                {
                    for(int l=0;l<cir1_same_bit[k]->_bus_indices.size();++l)
                    {
                        if(_bus_map[1][cir2_same_bit[k+offset]->_bus_indices[l]] == -1)
                            _bus_map[1][cir2_same_bit[k+offset]->_bus_indices[l]] = cir1_same_bit[k]->_bus_indices[l];
                        else if(_bus_map[1][cir2_same_bit[k+offset]->_bus_indices[l]] != cir1_same_bit[k]->_bus_indices[l])
                        {
                            cout << "Conflict bus " << cir2_same_bit[k+offset]->_bus_indices[l] << " has been matched to bus " << _bus_map[1][cir2_same_bit[k+offset]->_bus_indices[l]]
                            << ", bus " << cir1_same_bit[k]->_bus_indices[l] << " is fail.\n";
                            if(++offset > diff) break;
                            --l;
                        }
                    }
                    if(offset > diff)
                    {
                        cout << "No corresponding input in circuit 2 could match!\n";
                        break;
                    }
                    if(myModule[0]->_bus_set[cir1_same_bit[k]->_bus_idx].size() != 1)
                    {
                        _init_input[cir2_same_bit[k+offset]->_idx] = cir1_same_bit[k];
                        cir2_same_bit[k+offset]->_priority = 1;
                        cout << "--match input " << cir2_same_bit[k+offset]->_name << " -> " << cir1_same_bit[k]->_name << endl;
                    }
                    
                }
            }
            
        }
        show("Finish Initial Match cir1 and cir2!");
        // Self decide output order
        if(caseID == 10)
        {
            // _init_input[myModule[1]->_name_to_node["n1609"]->_idx] = myModule[0]->_name_to_node["n122"];
            // _init_input[myModule[1]->_name_to_node["n312"]->_idx] = myModule[0]->_name_to_node["n122"];
            // _init_input[myModule[1]->_name_to_node["n598"]->_idx] = myModule[0]->_name_to_node["n310"];
            // _init_input[myModule[1]->_name_to_node["n58"]->_idx] = myModule[0]->_name_to_node["n310"];
            // _init_input[myModule[1]->_name_to_node["n1474"]->_idx] = myModule[0]->_name_to_node["n310"];
            // _init_input[myModule[1]->_name_to_node["n1435"]->_idx] = myModule[0]->_name_to_node["n310"];
            // _init_input[myModule[1]->_name_to_node["n1278"]->_idx] = myModule[0]->_name_to_node["n310"];
            // _init_input[myModule[1]->_name_to_node["n905"]->_idx] = myModule[0]->_name_to_node["n310"];
        }
        
    }

    vector<vector<int>> get_match_pair()    { return _match_pair; }
    vector<vector<int>> get_in_match_pair()    { return _in_match_pair; }
    vector<bool> get_output_ne()            { return _output_ne; }
    vector<bool> get_input_ne()            { return _input_ne; }
    vector<pair<bool, size_t>> get_const_match() { return _const_match; }
    map<int, pair<int, bool>> get_max_input2to1() { return max_input2to1; }

    set<vector<int>> _var_group[2];
    vector<int> _bus_map[2];
    vector<vector<Node*>> _init_output;
    vector<Node*> _init_input;
    int _max_bus_diff;
    // Signature
    vector<vector<bool>> _truth_table[2];
    vector<uint64_t> _onset_size[2];
    vector<vector<uint64_t>> _OSV[2];
    vector<vector<uint64_t>> _SOSV[2];
    vector<vector<int>> _match_pair; // circuit1 matched to circuit2 
    vector<vector<int>> _in_match_pair; // circuit1 matched to circuit2 
    vector<pair<bool, size_t>> _const_match;
    vector<bool> _output_ne, _input_ne;
    vector<vector<vector<pair<pair<int, int>, bool>>>> _match_input_pairs;   //ith output jth permutation
    map<int, pair<int, bool>> max_input2to1;

public:
    string caseID;
    string outputFileName;
    int PI1Size;
    int PI2Size;
    int PO1Size;
    int PO2Size;
    bool outputSizeEq;
    bool inputSizeEq;
    bool initOn;
    vector<Module*> mod;
    const vector<Node*>& input1;
    const vector<Node*>& input2;
    const vector<Node*>& output1;
    const vector<Node*>& output2;
    // record the best answer
    bool findOutputBest;
    // bus info
    vector<vector<Node*>>   bus[2];
    vector<pair<int, bool>> maxInputMatch;  // [c2 pi idx] = <c1 pi idx, neg>
    vector<pair<int, bool>> maxOutputMatch; // [c2 po idx] = <c1 po idx, neg>
    vector<int> bus_1_to_2;
    vector<int> bus_2_to_1;
    vector<int> bus_2_to_1_count;
    int maxOutputDepth;
    // functional support
    vector<vector<int>>   output_support_idx[2];
    vector<vector<int>>   input_support_idx[2];
    vector<vector<Node*>>   output_support[2];
    vector<vector<Node*>>   input_support[2];
    vector<vector<vector<Node*>>>   input_support_new[2];
    vector<Node*>           output_match_order;  // [the ith matching priority c2 output] = <c2 output>
    vector<vector<Node*>>   output_match_cand;  // [c2 output idx] = <c1 output>, ....
    vector<vector<Node*>>   input_match_cand;   // [c2 input idx] = <c2 input>, ....
    vector<int>  _init_input_ne;

    vector<vector<vector<Node*>>>   input_match_cand_new;   // [c2 input idx] = <c2 input>, ....
    vector<int> inputLockCount;
    vector<pair<int, bool>> inputLock;
    vector<int> c1_inputMatchCount;

    bool** isPIinPO;
    vector<pair<int, bool>> getMaxInputMatch()  { return maxInputMatch; }
    vector<pair<int, bool>> getMaxOutputMatch()  { return maxOutputMatch; }

    int bus_size;
    vector<int> bus_idx_1, bus_idx_2;
    unordered_map<int, unordered_set<int>> bus_match_idx; // c2 bus size -> (c1 bus size)
    vector<set<int>> output_to_inputBus;

    void getBusSet(vector<vector<Node*>>& bus_set, Module* mod) {
        for (auto &it : mod->_bus_set) {
            if (it.size() > 1) bus_set.emplace_back(it);
        }
        // bus_set = _bus_set;
    }

    void recur_match_bus(int depth, int& quota, vector<bool>& c1_bus_occupied, vector<pair<int, int>>& one_bus_match, bool& new_match) {
        if (depth == bus_size) {
            for (size_t i = 0; i < bus_size; ++i) {
                bus_match_idx[bus[1][bus_idx_2[one_bus_match[i].first]].size()].insert(bus[0][bus_idx_1[one_bus_match[i].second]].size());
            }
            // for (auto &it : bus_match_idx) {
            //     cout << "c2 bus size = " << it.first << " -> c1 bus size : ";
            //     for (auto &c1_bus_size : it.second) cout << c1_bus_size << " ";
            //     cout << endl;
            // }
            new_match = false;
            return;
        } else {
            for (size_t i = 0; i < bus_size; ++i) {
                if (!c1_bus_occupied[i]) {
                    if (bus[1][bus_idx_2[depth]].size() - bus[0][bus_idx_1[i]].size() >= 0 && bus[1][bus_idx_2[depth]].size() - bus[0][bus_idx_1[i]].size() <= quota) {
                        if (new_match == false) {
                            if (bus_match_idx[bus[1][bus_idx_2[depth]].size()].find(bus[0][bus_idx_1[i]].size()) == bus_match_idx[bus[1][bus_idx_2[depth]].size()].end()) {
                                new_match = true;
                            } else {
                                continue;
                            }
                        }
                        one_bus_match.emplace_back(make_pair(depth, i));
                        quota -= (bus[1][bus_idx_2[depth]].size() - bus[0][bus_idx_1[i]].size());
                        c1_bus_occupied[i] = true;

                        recur_match_bus(depth + 1, quota, c1_bus_occupied, one_bus_match, new_match);

                        one_bus_match.pop_back();
                        quota += (bus[1][bus_idx_2[depth]].size() - bus[0][bus_idx_1[i]].size());
                        c1_bus_occupied[i] = false;
                    }
                }
            }
        }
    }

    void match_bus (Module* m1, Module* m2) {
        getBusSet(bus[0], m1);
        getBusSet(bus[1], m2);
        // suppose bus of c1 and c2 are the same 
        bus_size = bus[0].size();
        for (int i = 0; i < bus_size; i++) {
            bus_idx_1.emplace_back(i);
            bus_idx_2.emplace_back(i);
        }
        // for (size_t i = 0; i < bus_size; ++i) {
        //     cout << "[c1] bus i: " << bus[0][bus_idx_1[i]].size() << endl;
        // }
        // for (size_t i = 0; i < bus_size; ++i) {
        //     cout << "[c2] bus i: " << bus[1][bus_idx_2[i]].size() << endl;
        // }
        std::stable_sort(bus_idx_1.begin(), bus_idx_1.end(), 
            [&](const int a, const int b) { 
                return bus[0][a].size() > bus[0][b].size();
            });
        std::stable_sort(bus_idx_2.begin(), bus_idx_2.end(), 
            [&](const int a, const int b) { 
                return bus[1][a].size() > bus[1][b].size();
            });
        // for (size_t i = 0; i < bus_size; ++i) {
        //     cout << "[c1] bus i: " << bus[0][bus_idx_1[i]].size() << endl;
        // }
        // for (size_t i = 0; i < bus_size; ++i) {
        //     cout << "[c2] bus i: " << bus[1][bus_idx_2[i]].size() << endl;
        // }

        int quota = m2->_gate[INPUT].size() - m1->_gate[INPUT].size();
        for (auto& c2_input: m2->getGate(Type::INPUT)) {
            if (c2_input->_bus_indices.size() > 1) quota += (c2_input->_bus_indices.size() - 1);
        }
        for (auto& c1_input: m1->getGate(Type::INPUT)) {
            if (c1_input->_bus_indices.size() > 1) quota -= (c1_input->_bus_indices.size() - 1);
        }
        // int quota = 0;
        // for (auto &it : bus[1]) quota += it.size();
        // for (auto &it : bus[0]) quota -= it.size();
        cout << "quota: " << quota << endl;
        vector<bool> c1_bus_occupied(bus_size, false);
        vector<pair<int, int>> one_bus_match;
        bool new_match = true;

        recur_match_bus(0, quota, c1_bus_occupied, one_bus_match, new_match);

        // bus_match_idx.resize(bus_size, unordered_set<int>(0));

        // for (size_t i = 0; i < bus_match.size(); ++i) {
        //     cout << "bus match pair " << i + 1 << " : " << endl;
        //     for (auto &it : bus_match[i]) {
        //         cout << bus_idx_2[it.first] << " (" << bus[1][bus_idx_2[it.first]].size() << ") -> " << bus_idx_1[it.second] << " (" << bus[0][bus_idx_1[it.second]].size() << ")" << endl;
        //         bus_match_idx[bus_idx_2[it.first]].insert(bus_idx_1[it.second]);
        //     }
        // }

        for (auto &it : bus_match_idx) {
            cout << "c2 bus size = " << it.first << " -> c1 bus size : ";
            for (auto &c1_bus_size : it.second) cout << c1_bus_size << " ";
            cout << endl;
        }
    }


    bool match_input(   int idx, int out_idx, 
                        Node* c1_output, Node* c2_output,  vector<Node*>& c2_input_set, vector<int>& record_lock,
                        vector<pair<int, bool>>& inputLock, vector<int>& inputLockCount,
                        int& quota, vector<int>& c1_input_match_count, 
                        vector<pair<int, bool>>& outputLock, vector<int>& c1_OutputLockCount) {
        
        if (idx == c2_input_set.size()) {
            // call miter
            vector<Node*>& c2_input = output_support[1][c2_output->_idx];
            vector<pair< int, int >> c2toc1_input_match_pair;
            vector<bool> input_pair_match_neg;
            vector<pair<bool, size_t>> const_match;
            pair<int, int> c2toc1_output_match_pair = make_pair(c2_output->_idx, c1_output->_idx);
            cout << "miter output: " << mod[1]->_gate[OUTPUT][c2_output->_idx]->_name << " -> " << mod[0]->_gate[OUTPUT][c1_output->_idx]->_name << endl;
            for (auto& it: c2_input) {
                if (inputLock[it->_idx].first == -1) {
                    cout << "miter input: " << it->_name << " -> dangling" << endl;
                    continue;
                }
                if (inputLock[it->_idx].first != CONST_VAL) {
                    cout << "miter input: " << it->_name << " -> " <<  mod[0]->_gate[INPUT][inputLock[it->_idx].first]->_name <<" "<< inputLock[it->_idx].second << endl;
                    c2toc1_input_match_pair.push_back(make_pair(it->_idx, inputLock[it->_idx].first));
                    input_pair_match_neg.push_back(inputLock[it->_idx].second);
                } else {
                    cout << "miter input: " << it->_name << " -> CONST " << inputLock[it->_idx].second  << endl;
                    const_match.push_back(make_pair(inputLock[it->_idx].second, it->_idx));
                }
            }

            int result = miterOneOutputRandom(  mod[0], mod[1],
                                                input1, input2, output1, output2,
                                                c2toc1_input_match_pair, input_pair_match_neg,
                                                c2toc1_output_match_pair,
                                                const_match);
            cout << "result = " << result << endl;
            if (result == 0) 
                return false;
            else if (result == 1 || result == 2) {
                int c1_outputIdx = c1_output->_idx;
                outputLock[c2_output->_idx] = (result == 1) ? make_pair(c1_output->_idx, 0) : make_pair(c1_output->_idx, 1);

                for (int& c2_inputLockIdx: record_lock) {
                    inputLockCount[c2_inputLockIdx]++;
                    if (inputLock[c2_inputLockIdx].first != CONST_VAL) {
                        size_t c2_inputLockBusIdx = mod[1]->_gate[INPUT][c2_inputLockIdx]->_bus_idx;
                        bus_2_to_1_count[c2_inputLockBusIdx]++;
                    }
                }

                size_t c2_outputBusIdx = c2_output->_bus_idx;
                size_t c1_outputBusIdx = c1_output->_bus_idx;
                if (bus_2_to_1[c2_outputBusIdx] == -1 && bus_1_to_2[c1_outputBusIdx] == -1) {
                    bus_2_to_1[c2_outputBusIdx] = c1_outputBusIdx;
                    bus_1_to_2[c1_outputBusIdx] = c2_outputBusIdx;
                }
                bus_2_to_1_count[c2_outputBusIdx]++;
                c1_OutputLockCount[c1_outputIdx]++;
                /////////////////////////////////////////////////////////////////////////////
                
                bool output_eq = match_output(  out_idx+1, 
                                                inputLock, inputLockCount, 
                                                outputLock, c1_OutputLockCount);
                /////////////////////////////////////////////////////////////////////////////
                if (output_eq) {
                    return true;
                }
                else {
                    cout << "output return to " << c2_output->_name << endl;
                    outputLock[c2_output->_idx] = {-1, 0};
                    c1_OutputLockCount[c1_outputIdx]--;

                    for (int& c2_inputLockIdx: record_lock) {
                        inputLockCount[c2_inputLockIdx]--;
                        if (inputLock[c2_inputLockIdx].first != CONST_VAL) {
                            size_t c2_inputLockBusIdx = mod[1]->_gate[INPUT][c2_inputLockIdx]->_bus_idx;
                            bus_2_to_1_count[c2_inputLockBusIdx]--;
                        }
                    }

                    bus_2_to_1_count[c2_outputBusIdx]--;
                    if (bus_2_to_1_count[c2_outputBusIdx] == 0) {
                        bus_2_to_1[c2_outputBusIdx] = -1;
                        bus_1_to_2[c1_outputBusIdx] = -1;
                    }
                    if (c1_OutputLockCount[c1_outputIdx]<0) err("c1_OutputLockCount < 0\n");
                    return false;
                }
            }
            else {
                err("result not defined !!\n");
            }
        }
        
        Node* c2_input = c2_input_set[idx];
        size_t c2_inputIdx = c2_input->_idx;
        size_t c2_inputBusIdx = c2_input->_bus_idx;
        
        // when given initial answer
        int ini_neg = 0;
        int max_neg = 1;
        if (initOn && _init_input_ne[c2_inputIdx] != -1) {
            ini_neg = _init_input_ne[c2_inputIdx];
            max_neg = ini_neg;
        }
        //cout << "c2_input: " << c2_input->_name << endl;
        //cout << "ini_neg: " << ini_neg << " max_neg: " << max_neg << endl;

        for (auto& c1_input: input_match_cand[c2_input->_idx]) {
            int c1_inputIdx = c1_input->_idx;
            int c1_inputBusIdx = c1_input->_bus_idx;

            // const
            if (c1_inputIdx == CONST_VAL) {
                if (quota == 0) 
                    continue;
                else 
                    quota--;
            }
            // not const 
            else {
                //cout << "input match: " << c2_input->_name << " -> " << c1_input->_name << endl;
                vector<Node*>& c2_in = output_support[1][c2_output->_idx];
                vector<Node*>& c1_in = output_support[0][c1_output->_idx];
                
                // if (c1_input_match_count[c1_inputIdx]<0 || c1_input_match_count[c1_inputIdx] > ((int)c2_in.size()-(int)c1_in.size()+1))
                //     err("c1_input_match_count error!!");
                // case06 should close this condition
                if (inputSizeEq && c1_inputMatchCount[c1_inputIdx] != 0) 
                    continue;
                if (isPIinPO[c1_output->_idx][c1_inputIdx] == false) 
                    continue;
                if (c1_input_match_count[c1_inputIdx] != 0 && quota == 0)
                    continue;

                if (c2_input->_bus_indices.size() > 1) {
                    bool pass = true;
                    for (int multi_bus_idx = 0; multi_bus_idx < c2_input->_bus_indices.size(); ++multi_bus_idx) {
                        int c2_multuInputBusIdx = c2_input->_bus_indices[multi_bus_idx];
                        int c1_multuInputBusIdx = c1_input->_bus_indices[multi_bus_idx]; 
                        if (!(bus_2_to_1[c2_multuInputBusIdx] == -1 && bus_1_to_2[c1_multuInputBusIdx] == -1) &&
                            !(bus_2_to_1[c2_multuInputBusIdx] == c1_multuInputBusIdx && bus_1_to_2[c1_multuInputBusIdx] == c2_multuInputBusIdx)) {
                            pass = false;
                            break;
                        }
                    }
                    if (pass) {
                        for (int multi_bus_idx = 0; multi_bus_idx < c2_input->_bus_indices.size(); ++multi_bus_idx) {
                            int c2_multuInputBusIdx = c2_input->_bus_indices[multi_bus_idx];
                            int c1_multuInputBusIdx = c1_input->_bus_indices[multi_bus_idx];
                            bus_2_to_1[c2_multuInputBusIdx] = c1_multuInputBusIdx;
                            bus_1_to_2[c1_multuInputBusIdx] = c2_multuInputBusIdx;
                            ++bus_2_to_1_count[c2_multuInputBusIdx];
                        }
                    }
                    else continue;
                } else {
                    if (bus_2_to_1[c2_inputBusIdx] == -1 && bus_1_to_2[c1_inputBusIdx] == -1) {
                        bus_2_to_1[c2_inputBusIdx] = c1_inputBusIdx;
                        bus_1_to_2[c1_inputBusIdx] = c2_inputBusIdx;
                    }
                    else if ((caseID != "08" || (caseID == "08" && c1_input->_name != "n84")) && 
                            !(bus_2_to_1[c2_inputBusIdx] == c1_inputBusIdx && bus_1_to_2[c1_inputBusIdx] == c2_inputBusIdx)) {
                        continue;
                    }
                    bus_2_to_1_count[c2_inputBusIdx]++;
                }
                


                if (c1_input_match_count[c1_inputIdx] != 0)
                    quota--;
                c1_input_match_count[c1_inputIdx]++;
                c1_inputMatchCount[c1_inputIdx]++;
            }

            for (int neg = ini_neg; neg <= max_neg; neg++) {
                cout << "input match: " << c2_input->_name << " -> " << c1_input->_name << " " << neg << endl;
                if (inputLockCount[c2_inputIdx] == 0) {
                    inputLock[c2_inputIdx] = make_pair(c1_inputIdx, neg);
                }
                inputLockCount[c2_inputIdx]++;

                bool eq = match_input(idx+1, out_idx,
                            c1_output, c2_output,  c2_input_set, record_lock,
                            inputLock, inputLockCount, 
                            quota, c1_input_match_count, 
                            outputLock, c1_OutputLockCount);
                if (eq) 
                    return true;
                else {
                    inputLockCount[c2_inputIdx]--;
                    if (inputLockCount[c2_inputIdx] == 0) {
                        inputLock[c2_inputIdx] = make_pair(-1, 0);
                    }
                }
            }
            
            if (c1_input->_type == Type::CONST) {
                quota++;
            }
            // not const 
            else {
                if (c1_input_match_count[c1_inputIdx] > 1)
                    quota++;
                c1_input_match_count[c1_inputIdx]--;
                c1_inputMatchCount[c1_inputIdx]--;

                if (c2_input->_bus_indices.size() > 1) {
                    for (int multi_bus_idx = 0; multi_bus_idx < c2_input->_bus_indices.size(); ++multi_bus_idx) {
                        int c2_multuInputBusIdx = c2_input->_bus_indices[multi_bus_idx];
                        int c1_multuInputBusIdx = c1_input->_bus_indices[multi_bus_idx];
                        bus_2_to_1_count[c2_multuInputBusIdx]--;
                        if (bus_2_to_1_count[c2_multuInputBusIdx] == 0) {
                            bus_2_to_1[c2_multuInputBusIdx] = -1;
                            bus_1_to_2[c1_multuInputBusIdx] = -1;
                        }
                    }
                } 
                else {
                    bus_2_to_1_count[c2_inputBusIdx]--;
                    if (bus_2_to_1_count[c2_inputBusIdx] == 0) {
                        bus_2_to_1[c2_inputBusIdx] = -1;
                        bus_1_to_2[c1_inputBusIdx] = -1;
                    }
                }
            }

        }

        return false;
    }

    bool filtLockedInput(Node* c1_output, vector<Node*>& c2_input_set, vector<int>& record_lock, int& quota, vector<int>& c1_input_match_count) {
        for (auto it = c2_input_set.begin(); it != c2_input_set.end(); ) {
            int c2_inputIdx = (*it)->_idx;
            int c1_inputIdx = inputLock[c2_inputIdx].first;
            // lock
            if (c1_inputIdx != -1) {
                // lock & const
                if (c1_inputIdx == CONST_VAL) {
                    it = c2_input_set.erase(it);
                    quota--;
                }
                // lock & in input of c1 output
                else if (isPIinPO[c1_output->_idx][c1_inputIdx] == 1) {
                    it = c2_input_set.erase(it);
                    if (c1_input_match_count[c1_inputIdx] != 0)
                        quota--;
                    c1_input_match_count[c1_inputIdx]++;
                }
                // lock & not in input of c1 output
                else return false;
                record_lock.push_back(c2_inputIdx);
            } 
            // dangling 
            /*input_match_cand[c2_inputIdx].size() == 1 && 
            input_match_cand[c2_inputIdx][0]->_idx != CONST_VAL && 
            isPIinPO[c1_output->_idx][input_match_cand[c2_inputIdx][0]->_idx] == false*/

            //_init_input[c2_inputIdx] != NULL && isPIinPO[c1_output->_idx][_init_input[c2_inputIdx]->_idx] == false
            else if (initOn && _init_input[c2_inputIdx] != NULL && isPIinPO[c1_output->_idx][_init_input[c2_inputIdx]->_idx] == false) {
                it = c2_input_set.erase(it);
                quota--;
            }
            else 
                ++it;
        }
        return true;
    }

    Node* next_match(vector<pair<int, bool>>& outputLock)
    {
        Node* np = NULL;
        int max_priority = INT_MIN;

        for (auto& c2_output: output_match_order) {
            int c2_outputIdx = c2_output->_idx;
            if (outputLock[c2_outputIdx] != make_pair(-1, false)) continue;
            if(c2_output->_priority <= 0)
            {
                int priority = 0;
                for (auto c2_inputBusIdx: output_to_inputBus[c2_outputIdx]) {
                    if (bus_2_to_1[c2_inputBusIdx] == -1) {
                        --priority;
                    }
                }
                c2_output->_priority = priority;
            }
            if(max_priority < c2_output->_priority)
            {
                max_priority = c2_output->_priority;
                np = c2_output;
            }
        }
        return np;
    }

    bool match_output(  int out_idx, 
                        vector<pair<int, bool>>& inputLock, vector<int>& inputLockCount, 
                        vector<pair<int, bool>>& outputLock, vector<int>& c1_OutputLockCount
                        ) {

        // record partial match
        if (out_idx > maxOutputDepth) {
            maxOutputDepth = out_idx;
            maxOutputMatch = outputLock;
            maxInputMatch = inputLock;
            writePartialAns();
            //printLock(inputLock, inputLockCount);
            cout << "The Current best = " << maxOutputDepth << endl;
        }
        if (out_idx == PO2Size) { //min(PO2Size, 34)
            findOutputBest = true;
            return true;
        }
        

        //Node* c2_output = output_match_order[out_idx];
        Node* c2_output = next_match(outputLock);
        size_t c2_outputIdx = c2_output->_idx;
        size_t c2_outputBusIdx = c2_output->_bus_idx;

        vector<Node*>& c1_outputCand = output_match_cand[c2_outputIdx];

        for (auto& c1_output: c1_outputCand) {
            cout << "output match: " << c2_output->_name << " -> " << c1_output->_name << endl;
            size_t c1_outputIdx = c1_output->_idx;
            size_t c1_outputBusIdx = c1_output->_bus_idx;

            // one c1 output can only match once
            if (c1_OutputLockCount[c1_outputIdx] != 0) continue;

            // bus condition
            if (!(bus_2_to_1[c2_outputBusIdx] == -1 && bus_1_to_2[c1_outputBusIdx] == -1) && 
                !(bus_2_to_1[c2_outputBusIdx] == c1_outputBusIdx && bus_1_to_2[c1_outputBusIdx] == c2_outputBusIdx)) {
                continue;
            }

            // functional support
            // input_match_cand_new[c2_inputIdx][c1_outputIdx].push_back(c1_input);
            // std::stable_sort(output_support[1][c2_outputIdx].begin(), output_support[1][c2_outputIdx].end(), 
            //         [&](const Node* a, const Node* b) { return input_match_cand_new[a->_idx][c1_outputIdx].size() < input_match_cand_new[b->_idx][c1_outputIdx].size(); });
            vector<Node*> c2_input_set = output_support[1][c2_outputIdx];
            vector<Node*> c1_input_set = output_support[0][c1_outputIdx];

            int quota = c2_input_set.size() - c1_input_set.size();
            vector<int> c1_input_match_count(PI1Size, 0);

            vector<int> record_lock;
            if (!filtLockedInput(c1_output, c2_input_set, record_lock, quota, c1_input_match_count) || quota < 0) {
                continue;
            }
            cout << "c2_input_set: " << endl;
            for (auto& itt: c2_input_set) {
                cout << itt->_name << " ";
            }
            cout << endl;
            bool input_eq = match_input(  0, out_idx,
                                    c1_output, c2_output,  c2_input_set, record_lock,
                                    inputLock, inputLockCount,
                                    quota, c1_input_match_count, 
                                    outputLock, c1_OutputLockCount);
            if (findOutputBest)
                return true;
        }
        return false;
    }

    void assignFuncSupp() {
        bus[0] = mod[0]->_bus_set;
        bus[1] = mod[1]->_bus_set;

        for (int c = 0; c < 2; ++c) {
            cout << "circuit " << c << endl;
            for (int i = 0; i < bus[c].size(); ++i) {
                cout << "bus idx: " << i << "\n";
                vector<Node*>& aBus = bus[c][i];
                int busType = aBus[0]->_type;
                if (busType == Type::INPUT) {
                    for (auto& port: aBus) {
                        cout << "input: " << port->_name << endl;
                        port->_supportVal = input_support[c][port->_idx].size();
                    }
                }
                else if (busType == Type::OUTPUT) {
                    for (auto& port: aBus) {
                        cout << "output: " << port->_name << endl;
                        port->_supportVal = output_support[c][port->_idx].size();
                    }
                }
            }
        }
        cout << "Finish assignFuncSupp" << endl;
    }

    void assign_output_to_inputBus() {
        output_to_inputBus.resize(PO2Size);
        for (int i = 0; i < PO2Size; ++i) 
            for (auto& func_input: output_support[1][i]) 
                for (int bus_idx: func_input->_bus_indices)
                    output_to_inputBus[i].insert(bus_idx);
        cout << "Finish assign_output_to_inputBus" << endl;
    }

    void genOutputMatch(int inputDiff) {
        vector<Node*> c1_outputVec = mod[0]->getGate(Type::OUTPUT);
        vector<Node*> c2_outputVec = mod[1]->getGate(Type::OUTPUT);
        output_match_cand.resize(PO2Size);

        for (auto& c2_output: c2_outputVec) {
            size_t c2_outputIdx = c2_output->_idx;
           if (initOn && !_init_output[c2_outputIdx].empty()) {
                for (auto& it: _init_output[c2_outputIdx])
                    output_match_cand[c2_outputIdx].push_back(it);
            }
            else {
                int c2_outputBusSize = bus[1][c2_output->_bus_idx].size();
                for (auto& c1_output: c1_outputVec) {
                    size_t c1_outputIdx = c1_output->_idx;
                    int c1_outputBusSize = bus[0][c1_output->_bus_idx].size();
                    // important
                    if (outputSizeEq && c2_outputBusSize != c1_outputBusSize) 
                        continue;
                    if (inputSizeEq && c2_output->_supportVal != c1_output->_supportVal) 
                        continue; 
                    else if (!inputSizeEq && 
                            (c2_output->_supportVal < c1_output->_supportVal || (c2_output->_supportVal-c1_output->_supportVal) > inputDiff || (c2_output->_bit_idx < c1_output->_bit_idx)))
                                continue;


                    output_match_cand[c2_outputIdx].push_back(c1_output);
                }

                // sort candidate according to bus size first
                std::stable_sort(output_match_cand[c2_outputIdx].begin(), output_match_cand[c2_outputIdx].end(), 
                    [&](const Node* a, const Node* b) { return abs(c2_outputBusSize-bus[1][a->_bus_idx].size()) < abs(c2_outputBusSize-bus[1][b->_bus_idx].size()); });
                // then sort candidate according to func support difference
                // case05
                std::stable_sort(output_match_cand[c2_outputIdx].begin(), output_match_cand[c2_outputIdx].end(), 
                    [&](const Node* a, const Node* b) { return a->_supportVal > b->_supportVal; });
            }
        }

        output_match_order = output2;

        if (initOn) {
            std::stable_sort(output_match_order.begin(), output_match_order.end(), 
            [&](const Node* a, const Node* b) { 
                if (a->_priority != b->_priority) 
                    return a->_priority > b->_priority;
                else if (output_match_cand[a->_idx].size() == 1 && output_match_cand[b->_idx].size() == 1)
                    return a->_bit_idx < b->_bit_idx;
                else if (output_match_cand[a->_idx].size() == 1)
                    return true;
                else if (output_match_cand[b->_idx].size() == 1)
                    return false;
                else
                    return a->_supportVal < b->_supportVal; 
            });
        } 
        else {
            std::stable_sort(output_match_order.begin(), output_match_order.end(), 
            [&](const Node* a, const Node* b) { 
                return a->_supportVal < b->_supportVal; 
            });
        }
        

        std::ofstream log("case" + caseID + "_output_candidate.log");
        for (int i = 0; i < output_match_order.size(); ++i) {
            size_t c2_outputIdx = output_match_order[i]->_idx;
            Node* c2_output = output_match_order[i];
            int c2_outputFuncSupp = c2_output->_supportVal;
            log << "c2 output " << c2_output->_name << ", FuncSupp: " << c2_outputFuncSupp << ", Bus size: " << bus[1][c2_output->_bus_idx].size() << endl;
            for (auto c1_output: output_match_cand[c2_outputIdx]) {
                log << " - c1 output " << c1_output->_name << ", FuncSupp: " << c1_output->_supportVal << ", Bus size: " << bus[0][c1_output->_bus_idx].size() << endl;
            }
        }
        cout << "Finish genOutputMatch" << endl;
    }
    
    void genInputMatch(int inputDiff) {
        vector<Node*> c1_inputVec = mod[0]->getGate(Type::INPUT);
        vector<Node*> c2_inputVec = mod[1]->getGate(Type::INPUT);
        input_match_cand.resize(PI2Size);

        for (auto& c2_input: c2_inputVec) {
            size_t c2_inputIdx = c2_input->_idx;
            int c2_inputFuncSupp = c2_input->_supportVal;
            int c2_inputBusSize = bus[1][c2_input->_bus_idx].size();

            if (initOn && _init_input[c2_inputIdx] != NULL) {
                input_match_cand[c2_inputIdx].push_back(_init_input[c2_inputIdx]);
            }
            else {
                for (auto& c1_input: c1_inputVec) {
                    int c1_inputBusSize = bus[0][c1_input->_bus_idx].size();
                    // important
                    if (inputSizeEq && c2_inputBusSize != c1_inputBusSize)
                        continue;
                    // not sure
                    if (!inputSizeEq) {
                        // a port cross multile bus
                        if (c2_input->_bus_indices.size() > 1) {
                            if (c1_input->_bus_indices.size() != c2_input->_bus_indices.size()) 
                                continue;
                            bool pass = true;
                            for (int multi_bus_idx = 0; multi_bus_idx < c2_input->_bus_indices.size(); ++multi_bus_idx) {
                                int c2_multuInputBusSize = bus[1][c2_input->_bus_indices[multi_bus_idx]].size();
                                int c1_multuInputBusSize = bus[0][c1_input->_bus_indices[multi_bus_idx]].size(); 
                                if (c2_multuInputBusSize < c1_multuInputBusSize) {
                                    pass = false;
                                    break;
                                }  
                            }
                            if (!pass) continue;
                        } 
                        // normal
                        else {
                            if (((c2_inputBusSize < c1_inputBusSize) || ((c2_inputBusSize - c1_inputBusSize) > inputDiff) || (c2_input->_bit_idx < c1_input->_bit_idx))) //  
                                continue;
                            else if (c2_input->_bus_idx < bus_size && bus_match_idx[c2_inputBusSize].find(c1_inputBusSize) == bus_match_idx[c2_inputBusSize].end()) // bus match constraint
                                continue;
                        }
                    }
                    

                    if (outputSizeEq && inputSizeEq && c2_input->_supportVal != c1_input->_supportVal)
                        continue;
                    input_match_cand[c2_inputIdx].push_back(c1_input);
                }
                // sort candidate according to bus size first
                std::stable_sort(input_match_cand[c2_inputIdx].begin(), input_match_cand[c2_inputIdx].end(), 
                    [&](const Node* a, const Node* b) { return abs(c2_inputBusSize-bus[0][a->_bus_idx].size()) < abs(c2_inputBusSize-bus[0][b->_bus_idx].size()); });
                // sort by signature difference
                std::stable_sort(input_match_cand[c2_inputIdx].begin(), input_match_cand[c2_inputIdx].end(), 
                    [&](const Node* a, const Node* b) { return abs(mod[1]->input_sig[c2_inputIdx] - mod[0]->input_sig[a->_idx]) < abs(mod[1]->input_sig[c2_inputIdx] - mod[0]->input_sig[b->_idx]); });
                // then sort candidate according to func support difference
                std::stable_sort(input_match_cand[c2_inputIdx].begin(), input_match_cand[c2_inputIdx].end(), 
                    [&](const Node* a, const Node* b) { 
                        return abs(c2_inputFuncSupp-a->_supportVal) < abs(c2_inputFuncSupp-b->_supportVal); 
                    });
                
                // insert one const node in the end
                Node* constNode = new Node("CONST", Type::CONST);
                constNode->_idx = CONST_VAL;
                input_match_cand[c2_inputIdx].push_back(constNode);
            }
        }

        std::ofstream log("case" + caseID + "_input_candidate.log");
        for (int i = 0; i < input2.size(); ++i) {
            Node* c2_input = input2[i];
            size_t c2_inputIdx = c2_input->_idx;
            int c2_inputFuncSupp = c2_input->_supportVal;
            log << "c2 input " << c2_input->_name << ", FuncSupp: " << c2_inputFuncSupp << ", Bus size: " << bus[1][c2_input->_bus_idx].size() << endl;
            for (auto c1_input: input_match_cand[c2_inputIdx]) {
                if (c1_input->_type == Type::CONST) 
                    log << " - c1 input " << c1_input->_name << endl;
                else
                    log << " - c1 input " << c1_input->_name << ", FuncSupp: " << c1_input->_supportVal << ", Bus size: " << bus[0][c1_input->_bus_idx].size() << endl;
            }
        }
        

        cout << "Finish genInputMatch" << endl;
    }
    
    void genPIinPO() {
        isPIinPO = new bool*[PO1Size];
        for (int i = 0; i < PO1Size; ++i) isPIinPO[i] = new bool[PI1Size]();
        for (int i = 0; i < PO1Size; ++i) {
            for (int j = 0; j < output_support[0][i].size(); ++j) {
                isPIinPO[i][output_support[0][i][j]->_idx] = 1;
            }
        }
        cout << "Finish genPIinPO" << endl;
    }

    void genInfo() {            
        std::ofstream log2("case" + caseID + "_2_info.log");
        for (auto& c2_output: output2) {
            std::sort(output_support[1][c2_output->_idx].begin(), output_support[1][c2_output->_idx].end(), 
                [&](const Node* a, const Node* b) { 
                    if (input_match_cand[a->_idx].size() == input_match_cand[b->_idx].size())
                        return bus[1][a->_bus_idx].size() > bus[1][b->_bus_idx].size();
                    else 
                        return input_match_cand[a->_idx].size() < input_match_cand[b->_idx].size();
                });

            log2 << "c2 output: " << c2_output->_name << endl;
            for (auto& c2_input: output_support[1][c2_output->_idx]) {
                log2 << " - c2 support input: " << c2_input->_name << ", FuncSupp: " << c2_input->_supportVal << ", Bus size: " << bus[1][c2_input->_bus_idx].size() << endl;
            }
        }

        std::ofstream log1("case" + caseID + "_1_info.log");

        for (auto& c1_output: output1) {
            std::sort(output_support[0][c1_output->_idx].begin(), output_support[0][c1_output->_idx].end(), 
                [&](const Node* a, const Node* b) { return bus[0][a->_bus_idx].size() > bus[0][b->_bus_idx].size();});

            log1 << "c1 output: " << c1_output->_name << endl;
            for (auto& c1_input: output_support[0][c1_output->_idx]) {
                log1 << " - c1 support input: " << c1_input->_name << ", FuncSupp: " << c1_input->_supportVal << ", Bus size: " << bus[0][c1_input->_bus_idx].size() << endl;
            }
        }

        cout << "Finish genInfo" << endl;
    }

    void new_boolean_match() {
        cout << "PI1Size: " << PI1Size << endl;
        cout << "PI2Size: " << PI2Size << endl;
        cout << "PO1Size: " << PO1Size << endl;
        cout << "PO2Size: " << PO2Size << endl;
        outputSizeEq = PO2Size == PO1Size;
        inputSizeEq = PI2Size == PI1Size;
        int inputDiff = PI2Size-PI1Size;
        assignFuncSupp();
        genPIinPO();
        genOutputMatch(inputDiff);
        genInputMatch(inputDiff);
        genInfo();
        assign_output_to_inputBus();

        inputLock.resize(PI2Size, make_pair(-1, 0));    // lock[c2 pi idx] = <c1 pi idx, neg>
        inputLockCount.resize(PI2Size, 0);              // lock[c2 pi idx] = <match count>

        vector<pair<int, bool>> outputLock(PO2Size, make_pair(-1, 0));
        vector<int> c1_OutputLockCount(PO1Size, 0);

        maxOutputDepth = -1;
        findOutputBest = false;
        //TODO: cannot deal with bus one-to-many, or many-to-one
        bus_1_to_2.resize(bus[0].size(), -1);
        bus_2_to_1.resize(bus[1].size(), -1);
        bus_2_to_1_count.resize(bus[1].size(), 0);

        c1_inputMatchCount.resize(PI1Size, 0);
        match_output(   0, 
                        inputLock, inputLockCount, 
                        outputLock, c1_OutputLockCount
                    );
    }
    
    void printLock(vector<pair<int, bool>>& lock, vector<int>& lockCount) {
        cout << "Input Lock: " << endl;
        for (int i = 0; i < PI2Size; ++i) {
            if (lock[i].first == -1) 
                cout << mod[1]->_gate[INPUT][i]->_name << ": " << "None" << ", " << lock[i].second << endl;
            else {
                if (lock[i].first == CONST_VAL) 
                    cout << mod[1]->_gate[INPUT][i]->_name << ": " << "CONST " << lock[i].second << endl;
                else
                    cout << mod[1]->_gate[INPUT][i]->_name << ": " << mod[0]->_gate[INPUT][lock[i].first]->_name << ", " << lock[i].second << endl;
            }
        }

        cout << "Input LockCount: " << endl;
        for (int i = 0; i < PI2Size; ++i) {
            cout << mod[1]->_gate[INPUT][i]->_name << ": " << lockCount[i] << endl;
        }
    }

    void storeFunctionSupport() {
        cout << "write file: " << "case" + caseID + "_supp/" + "output_support.txt" << endl;
        std::ofstream out_supp("case" + caseID + "_supp/" + "output_support.txt");
        for (int c = 0; c < 2; ++c) {
            out_supp << "-1 " << output_support_idx[c].size() << endl;
            for (int i = 0; i < output_support_idx[c].size(); ++i) {
                for (int j = 0; j < output_support_idx[c][i].size(); ++j) {
                    out_supp << output_support_idx[c][i][j] << " ";
                }
                out_supp << endl;
            }
        }
        cout << "write file: " << "case" + caseID + "_supp/" + "input_support.txt" << endl;
        std::ofstream in_supp("case" + caseID + "_supp/" + "input_support.txt");
        for (int c = 0; c < 2; ++c) {
            in_supp << "-1 " << input_support_idx[c].size() << endl;
            for (int i = 0; i < input_support_idx[c].size(); ++i) {
                for (int j = 0; j < input_support_idx[c][i].size(); ++j) {
                    in_supp << input_support_idx[c][i][j] << " ";
                }
                in_supp << endl;
            }
        }
    }

    void readFunctionSupport() {
        cout << "read file: " << "case" + caseID + "_supp/" + "output_support.txt" << endl;
        cout << "read file: " << "case" + caseID + "_supp/" + "input_support.txt" << endl;
        std::ifstream out_supp("case" + caseID + "_supp/" + "output_support.txt");
        if (!out_supp.is_open()) err("Failed to open output file.");
        std::ifstream in_supp("case" + caseID + "_supp/" + "input_support.txt");
        if (!in_supp.is_open()) err("Failed to open output file.");
        std::string line;

        int val;
        for (int c = 0; c < 2; ++c) {
            std::getline(out_supp, line);
            std::istringstream iss(line);
            int outputNum;
            iss >> val >> outputNum;
            for (int i = 0; i < outputNum; ++i) {
                std::getline(out_supp, line);
                std::istringstream iss(line);
                vector<Node*> piVec;
                while (iss >> val) {
                    Node* node = (c == 0) ? input1[val] : input2[val];
                    piVec.push_back(node);
                }
                output_support[c].push_back(piVec);
            }
            
        }
        for (int c = 0; c < 2; ++c) {
            std::getline(in_supp, line);
            std::istringstream iss(line);
            int outputNum;
            iss >> val >> outputNum;
            for (int i = 0; i < outputNum; ++i) {
                std::getline(in_supp, line);
                std::istringstream iss(line);
                vector<Node*> poVec;
                while (iss >> val) {
                    Node* node = (c == 0) ? output1[val] : output2[val];
                    poVec.push_back(node);
                }
                input_support[c].push_back(poVec);
            }
        }

        out_supp.close();
        in_supp.close();
        cout << "out_supp and in_supp close \n";
    }

    void verifyResult() {

        cout << "=== verify input lock ===" << endl;
        vector<int> count(PI2Size, 0);
        cout << "ground true: " << endl;
        for (auto& c2_output: output2) {
            vector<int> c2_input = c2_output->_pi_idx;
            for (auto c2_inputIdx: c2_input) {
                count[c2_inputIdx]++;
            }
        }
        for (int i = 0; i < PI2Size; ++i) {
            cout << mod[1]->_gate[INPUT][i]->_name << "-> true: " << count[i] << ", lockCount: " << inputLockCount[i] << endl;
            if (count[i] != inputLockCount[i]) cout << "lock count wrong!!\n";
        }

        cout << "=== verify matched output ===" << endl;
        int point = 0;
        bool eq = true;
        for (int i = 0; i < maxOutputMatch.size(); ++i) {
            int outIdx2 = i;
            int outIdx1 = maxOutputMatch[i].first;
            if (outIdx1 == -1) continue;
            
            vector<pair<int, int>> c2toc1_input_match_pair;
            vector<bool> input_pair_match_neg;
            vector<pair<bool, size_t>> const_match_for_miter;
            pair<int, int> c2toc1_output_match_pair = make_pair(outIdx2, outIdx1);
            bool output_pair_match_neg = maxOutputMatch[i].second;

            vector<int>& c2_po_inputs = mod[1]->_gate[OUTPUT][outIdx2]->_pi_idx;
            for (auto piIdx2: c2_po_inputs) {
                int piIdx1 = maxInputMatch[piIdx2].first;
                bool neg = maxInputMatch[piIdx2].second;
                if (piIdx1 == -1) {
                    cout << "Warning: output match, but its input doesn't match" << endl;
                }
                if (piIdx1 == CONST_VAL) {

                    const_match_for_miter.push_back(make_pair(neg, piIdx2));
                } else {
                    c2toc1_input_match_pair.push_back(make_pair(piIdx2, piIdx1));
                    input_pair_match_neg.push_back(neg);
                }
            }
            
            int result = miterOneOutputRandom(  mod[0], mod[1],
                                                input1, input2, output1, output2,
                                                c2toc1_input_match_pair, input_pair_match_neg,
                                                c2toc1_output_match_pair,
                                                const_match_for_miter);
            bool one_output_eq = ((result==1 && output_pair_match_neg==0) || (result==2 && output_pair_match_neg==1));
            point += one_output_eq? 2:0;
            if (!one_output_eq) eq = false;
            else cout << mod[1]->_gate[OUTPUT][outIdx2]->_name << " -> " << mod[0]->_gate[OUTPUT][outIdx1]->_name << endl;
        }

        if (eq) 
            cout << "eq" << endl << "point: " << point << endl;
        else 
            cout << "not eq" << endl << "point: " << point << endl;

    }

    void writePartialAns() {
        cout << "==============================================================\n";
        cout << "                     Write Partial Ans                        \n";
        cout << "==============================================================\n";
        vector<vector<int>> out_match_pair(PO1Size);
        vector<vector<int>> in_match_pair(PI1Size);
        vector<bool> out_ne(PO2Size);
        vector<bool> in_ne(PI2Size);
        vector<pair<bool, size_t>> const_match;

        for (int i = 0; i < maxInputMatch.size(); ++i) {
            int c1Idx = maxInputMatch[i].first;
            int c2Idx = i;
            bool neg = maxInputMatch[i].second;
            if (c1Idx == -1) continue;
            if (c1Idx == CONST_VAL) { // match const value
                const_match.push_back(make_pair(maxInputMatch[i].second, c2Idx));
            } else {
                in_match_pair[c1Idx].push_back(c2Idx);
                in_ne[c2Idx] = neg;
            }
        }

        for (int i = 0; i < maxOutputMatch.size(); ++i) {
            int c1Idx = maxOutputMatch[i].first;
            int c2Idx = i;
            bool neg = maxOutputMatch[i].second;
            if (c1Idx == -1) continue;
            out_match_pair[c1Idx].push_back(c2Idx);
            out_ne[c2Idx] = neg;
        }

        ofstream ofs(outputFileName);
        if (!ofs.is_open()) err("Failed to open output file.");
        writeOutput(ofs, 
                    input1, input2, output1, output2, 
                    in_match_pair, in_ne, 
                    out_match_pair, out_ne, 
                    const_match);
        ofs.close();
    }
};
