#include "assembler.cpp"

// Linking Loader Pass 1
void Linking_Loader_Pass_1()
{
    ifstream fin("output.txt");

    string progaddr;
    cout << "Enter PROGADDR for the program: " << endl;
    cin >> progaddr;
    PROGADDR = stoi(progaddr, NULL, 16);

    int CSADDR = PROGADDR;
    int CSLTH = 0;
    string record = "";

    while (getline(fin, record))
    {
        if (record[0] == 'H')
        {
            CSADDR += CSLTH;

            string CSECT = strip_whitespaces(record.substr(1, 6));
            CSLTH = stoi(record.substr(13, 6), NULL, 16);

            if (ESTAB.find(CSECT) == ESTAB.end())
            {
                ESTAB[CSECT] = CSADDR;
            }
            else
            {
                cout << "Duplicate Header Record name!" << endl;
                // exit(0);
            }
        }
        if (record[0] == 'D')
        {
            vector<pair<string,int>> Define_Records = Get_Define_Records(record);

            for(pair<string,int> def_record : Define_Records)
            {
                string name = strip_whitespaces(def_record.first);
                int rel_addr = def_record.second;

                if (ESTAB.find(name) == ESTAB.end())
                {
                    ESTAB[name] = CSADDR + rel_addr;
                }
                else
                {
                    cout << "Duplicate Define Record name!" << endl;
                }
                
            }
        }
    }
    LAST_ADDR = CSADDR + CSLTH;

    
}