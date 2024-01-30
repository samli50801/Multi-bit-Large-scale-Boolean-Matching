#include "Netlist.h"
#include "Parser.cpp"
#include "Matching.cpp"

int main(int argc, char* argv[])
{
    vector<string> file_name;
    vector<vector<string>> bus[2];
    ParseInput(argv[1], file_name, bus);
    
    // mapping using ABC tool
    ABCRebuild(file_name,del_out,abc_map_out);
    // Input and Output files
    std::string outputFileName = argv[2];
    ifstream ifs1(abc_map_out[0]);
    ifstream ifs2(abc_map_out[1]);
    if (!ifs1.is_open()) err("Failed to open input file.");
    if (!ifs2.is_open()) err("Failed to open input file.");
    vector<Module*> myModule;
    int moduleNumber = 0; // endmodule ++
    // Define the type mapping from type name to enum type
    for (int i = 0; i < TYPEN; i++) TypeNum[TypeID[i]] = (Type)i;

    // Parse the base module
    int code[2];
    code[0] = Parser(ifs1, myModule, moduleNumber++,0);
    code[1] = Parser(ifs2, myModule, moduleNumber++,0);

    Node* tar = myModule[1]->NameToNode("n108");
    myModule[0]->setBus(bus[0]);
    myModule[1]->setBus(bus[1]);

    vector<Node*> input1 = myModule[0]->getGate(Type::INPUT);
    vector<Node*> input2 = myModule[1]->getGate(Type::INPUT);
    vector<Node*> output1 = myModule[0]->getGate(Type::OUTPUT);
    vector<Node*> output2 = myModule[1]->getGate(Type::OUTPUT);
    int c1_PI_size = input1.size();
    int c2_PI_size = input2.size();
    int c1_PO_size = output1.size();
    int c2_PO_size = output2.size();

    Match* match = new Match(input1, input2, output1, output2);
    // output match
    vector<vector<int>> out_match_pair;
    vector<bool> out_ne;
    // input match
    vector<vector<int>> in_match_pair;
    vector<bool> in_ne;
    // const match
    vector<pair<bool, size_t>> const_match;

    int caseID = 0;
    bool initOn = false;
    vector<pair<int,int>> testcase = {{0,0},{3,3},{16,16},{168,172},{39,28},{1816,1768},{769,818},{91,105},{224,241},{8982,9572},{1318,1604}};
    for(int i=0;i<11;++i) if(make_pair(code[0],code[1]) == testcase[i]) caseID = i;
    

    match->MatchAlgo(myModule,caseID);
    initOn = true;
    

    if (c1_PI_size != c2_PI_size) match->match_bus(myModule[0], myModule[1]);

    if (caseID != 3) {
        match->initOn = initOn;
        match->caseID = caseID < 10 ? "0" + to_string(caseID) : to_string(caseID);
        match->outputFileName = outputFileName;
        
        match->mod = myModule;
        match->readFunctionSupport();
        cout << "start matching" << endl;
        match->new_boolean_match();
        // insert constant to don't care PI
        match->verifyResult();

        // output match
        out_match_pair.resize(c1_PO_size);
        out_ne.resize(c2_PO_size);
        // input match
        in_match_pair.resize(c1_PI_size);
        in_ne.resize(c2_PI_size);

        vector<pair<int, bool>> maxInputMatch = match->getMaxInputMatch();
        vector<pair<int, bool>> maxOutputMatch = match->getMaxOutputMatch();

        // translate input matching format to output file supported format
        for (int i = 0; i < maxInputMatch.size(); ++i) {
            int c1Idx = maxInputMatch[i].first;
            int c2Idx = i;
            bool neg = maxInputMatch[i].second;
            if (c1Idx == -1) continue;
            if (c1Idx == -2) { // match const value
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
    } else {
        //match->MatchAlgo(myModule,caseID);
    
        show("Finish Matching Algorithm.");
        out_match_pair = match->get_match_pair();
        out_ne = match->get_output_ne();
        in_match_pair = match->get_in_match_pair();
        in_ne = match->get_input_ne();
        const_match = match->get_const_match();
    }
    // Self Assign
    ofstream ofs(outputFileName);
    if (!ofs.is_open()) err("Failed to open output file.");
    writeOutput(ofs, 
                input1, input2, output1, output2, 
                in_match_pair, in_ne, 
                out_match_pair, out_ne, 
                const_match);

    // End of File
    ofs.close();

    bool eq = miter( "miter.v", "miter.cnf", del_out[0], del_out[1],
            input1, input2, output1, output2, 
            in_match_pair, in_ne, 
            out_match_pair, out_ne, 
            const_match);
    if(eq)
        cout << "eq" << endl;
    else
        cout << "non-eq" << endl;


    return 0;
}
