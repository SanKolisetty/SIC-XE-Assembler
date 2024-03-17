#include "linking_loader_Pass1.cpp"

// Linking Loader Pass 2
void Linking_Loader_Pass_2()
{
    ifstream fin("output.txt");

    int CSADDR = PROGADDR;
    int EXECADDR = PROGADDR;
    int CSLTH = 0;

    string record = "";

    while (getline(fin, record))
    {
        if (record[0] == 'H')
        {
            CSLTH = stoi(record.substr(13, 6), NULL, 16);
        }
        else if (record[0] == 'T')
        {
            int START_ADDR = stoi(record.substr(1, 6), NULL, 16) + CSADDR;

            for (int i = 9; i < record.length(); i += 2)
            {
                Memory[START_ADDR++] = record.substr(i, 2);
            }
        }
        else if (record[0] == 'M')
        {
            string ext_ref = strip_whitespaces(record.substr(10, 6));

            if (ESTAB.find(ext_ref) != ESTAB.end())
            {
                int address = stoi(record.substr(1, 6), NULL, 16) + CSADDR;
                int length = stoi(record.substr(7, 2), NULL, 16);

                char left = ' ';
                if (length % 2)
                {
                    left = Memory[address][0];
                }

                string modification_record = "";
                for (int i = 0; i < (length + 1) / 2; i++)
                {
                    modification_record += Memory[address + i];
                }

                int modification_record_value = stoi(modification_record, NULL, 16);

                int modification_req = ESTAB[ext_ref];

                if (record[9] == '+')
                {
                    modification_record_value += modification_req;
                }
                else if (record[9] == '-')
                {
                    modification_record_value -= modification_req;
                }
                else
                {
                    cout << "Error in Modification record" << endl;
                }

                int mask = 0;
                for (int i = 0; i < (length + length % 2) * 4; i++)
                {
                    mask |= 1;
                    mask = mask << 1;
                }
                mask = mask >> 1;
                modification_record_value = modification_record_value & mask;

                string final_record = hexadecimal(modification_record_value, length + (length % 2));

                for (int i = 0; i < length; i += 2)
                {
                    Memory[address + i / 2] = final_record.substr(i, 2);
                }

                if (length % 2)
                {
                    Memory[address][0] = left;
                }
            }
            else
            {
                cout << "External Symbol not found : " << ext_ref << endl;
            }
        }
        else if (record[0] == 'E')
        {
            if (record != "E")
            {
                int FIRST = stoi(record.substr(1, 6), NULL, 16);
                EXECADDR = CSADDR + FIRST;
            }
            CSADDR = CSADDR + CSLTH;
        }
    }

    cout << "Starting Program Execution at: " << hexadecimal(EXECADDR, 4) << endl;
}