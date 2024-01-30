#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <set>
#include "Netlist.h"
using namespace std;

/*
add getName in Netlist.h
add getGate in Netlist.h
not sure if circuit1 has + or -
*/

void writeOutput(   ofstream& ofs, 
                    const vector<Node*>& input1, const vector<Node*>& input2, const vector<Node*>& output1, const vector<Node*>& output2, 
                    vector<vector<int>>& in_match_pair, vector<bool>& in_match_neg, 
                    vector<vector<int>>& out_match_pair, vector<bool>& out_match_neg, 
                    vector<pair<bool, size_t>>& const_match
                    ) {

    set<int> c2_matchedInput;
    set<int> c2_matchedOutput;

    cout << "==============================================================\n";
    cout << "                       write INGROUP                          \n";
    cout << "==============================================================\n";
    size_t inGroupNum = in_match_pair.size();
    for (size_t i = 0; i < inGroupNum; ++i) {
        size_t matchNum = in_match_pair[i].size();

        ofs << "INGROUP" << endl;
        ofs << "1 + " << input1[i]->getName() << endl;

        for (size_t j = 0; j < matchNum; ++j) {
            size_t port_idx = in_match_pair[i][j];
            string state = in_match_neg[port_idx] == 0 ? "+" : "-";
            // match port of circuit2
            ofs << "2 " << state << " " << input2[port_idx]->getName() << endl;
            if (c2_matchedInput.find(port_idx) != c2_matchedInput.end()) 
                cout << "ERROR: circuit2 input " << port_idx << " has been matched!" << endl;
            else
                c2_matchedInput.insert(port_idx);
        }
        ofs << "END" << endl;
    }

    cout << "==============================================================\n";
    cout << "                       write OUTGROUP                         \n";
    cout << "==============================================================\n";
    size_t outGroupNum = out_match_pair.size();
    for (size_t i = 0; i < outGroupNum; ++i) {
        size_t matchNum = out_match_pair[i].size();

        ofs << "OUTGROUP" << endl;
        ofs << "1 + " << output1[i]->getName() << endl;

        for (size_t j = 0; j < matchNum; ++j) {
            size_t port_idx = out_match_pair[i][j];
            string state = out_match_neg[port_idx] == 0 ? "+" : "-";
            // match port of circuit2
            ofs << "2 " << state << " " << output2[port_idx]->getName() << endl;
            if (c2_matchedOutput.find(port_idx) != c2_matchedOutput.end()) 
                cout << "ERROR: circuit2 output " << port_idx << " has been matched!" << endl;
            else
                c2_matchedOutput.insert(port_idx);
        }
        ofs << "END" << endl;
    }
    if(const_match.size() == 0)
    {
        cout << "==============================================================\n";
        cout << "                   No write const_match                       \n";
        cout << "==============================================================\n";
        return;
    }
    cout << "==============================================================\n";
    cout << "                       write const_match                       \n";
    cout << "==============================================================\n";
    ofs << "CONSTGROUP" << endl;
    for (size_t i = 0; i < const_match.size(); ++i) {
        size_t port_idx = const_match[i].second;
        string constVal = const_match[i].first == 0 ? "+" : "-";
        ofs << constVal << " " << input2[port_idx]->getName() << endl;
        if (c2_matchedInput.find(port_idx) != c2_matchedInput.end()) 
                cout << "ERROR: circuit2 input " << port_idx << " has been matched!" << endl;
        else
            c2_matchedInput.insert(port_idx);
    }
    ofs << "END" << endl;
}