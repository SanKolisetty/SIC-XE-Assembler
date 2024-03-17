#include <bits/stdc++.h>
using namespace std;

#define LENGTH 10 // length of all fields except operand
#define OPERAND_LENGTH 30 // length of operand filed

// removes the whitespaces
string strip_whitespaces(string s)
{
    s.erase(remove(s.begin(), s.end(), ' '), s.end());
    return s;
}

// returns the given integer in hexadecimal with padding
string hexadecimal(int number, int width, char padding = '0')
{
    stringstream temp;
    temp << hex << uppercase << setfill(padding) << setw(width) << number;
    return temp.str();
}

// returns the formatted string
string format_string(string s, int width, char padding = ' ')
{
    stringstream temp;
    temp << left << setfill(padding) << setw(width) << s;
    return temp.str();
}

// returns true if a number, otherwise false
bool is_number(string s)
{
    int n = s.size();
    for(int i = 0; i < n; i++)
    {
        if(!isdigit(s[i]))
        {
            return false;
        }
    }
    return true;
}

// taking 2's complement
void mask(int& value, int bits)
{
	if(value < 0)
	{
		int mask = 0;
		for(int i = 0; i < bits; i++)
		{
			mask |= 1;
			mask = mask << 1;
		}
		mask = mask >> 1;
		value = value & mask;
	}
}

// Op table
pair<int, int> OPTAB(string opcode)
{
	if(opcode == "LDA")	return make_pair(3,0x00);
	if(opcode == "LDX")	return make_pair(3,0x04);
	if(opcode == "LDL")	return make_pair(3,0x08);
	if(opcode == "LDB")	return make_pair(3,0x68);
	if(opcode == "LDT")	return make_pair(3,0x74);
	if(opcode == "STA")	return make_pair(3,0x0C);
	if(opcode == "STX")	return make_pair(3,0x10);
	if(opcode == "STL")	return make_pair(3,0x14);
	if(opcode == "LDCH")	return make_pair(3,0x50);
	if(opcode == "STCH")	return make_pair(3,0x54);
	if(opcode == "ADD")	return make_pair(3,0x18);
	if(opcode == "SUB")	return make_pair(3,0x1C);
	if(opcode == "MUL")	return make_pair(3,0x20);
	if(opcode == "DIV")	return make_pair(3,0x24);
	if(opcode == "COMP")	return make_pair(3,0x28);
	if(opcode == "COMPR")	return make_pair(2,0xA0);
	if(opcode == "CLEAR")	return make_pair(2,0xB4);
	if(opcode == "J")	return make_pair(3,0x3C);
	if(opcode == "JLT")	return make_pair(3,0x38);
	if(opcode == "JEQ")	return make_pair(3,0x30);
	if(opcode == "JGT")	return make_pair(3,0x34);
	if(opcode == "JSUB")	return make_pair(3,0x48);
	if(opcode == "RSUB")	return make_pair(3,0x4C);
	if(opcode == "TIX")	return make_pair(3,0x2C);
	if(opcode == "TIXR")	return make_pair(2,0xB8);
	if(opcode == "TD")	return make_pair(3,0xE0);
	if(opcode == "RD")	return make_pair(3,0xD8);
	if(opcode == "WD")	return make_pair(3,0xDC);
	
	// not found
	return make_pair(-1, -1);
}

// storing symbols in each control section
struct symbol
{
    string control_section;
    string symbol_name;

    symbol(string ctrl_sec, string sym)
    {
        this->control_section = ctrl_sec;
        this->symbol_name = sym; 
    }

	// Used for comparing in maps for storing in ascending order
	bool operator<(const symbol& s) const
	{
		return make_pair(this -> control_section, this -> symbol_name) < make_pair(s.control_section, s.symbol_name);
	}
};

// storing value of the symbol
struct value
{
    int value_sym;
    bool type; // 0 for absolute, 1 for relative; most useful in determining the validity of the expressions
    int length; // stores the length of the control section for a control section
    value(int value_sym = 0, int length = 3, bool type = 1)
	{
		this -> value_sym = value_sym;
		this -> length = length;
		this -> type = type;
	}
	
};

map <symbol, value> SYMTAB; //Stores symbols
map <symbol, value> LITTAB; //Stores Literals
set <string> EXTREF; //Stores the EXTREF symbols

// Adding symbols to a control section
void preprocess_symbols(string CSECT)
{
	SYMTAB.insert({symbol(CSECT,"A" ),value(0, 3, 0)});
	SYMTAB.insert({symbol(CSECT,"X" ),value(1, 3, 0)});
	SYMTAB.insert({symbol(CSECT,"L" ),value(2, 3, 0)});
	SYMTAB.insert({symbol(CSECT,"PC"),value(8, 3, 0)});
	SYMTAB.insert({symbol(CSECT,"SW"),value(9, 3, 0)});
	SYMTAB.insert({symbol(CSECT,"B" ),value(3, 3, 0)});
	SYMTAB.insert({symbol(CSECT,"S" ),value(4, 3, 0)});
	SYMTAB.insert({symbol(CSECT,"T" ),value(5, 3, 0)});
	SYMTAB.insert({symbol(CSECT,"F" ),value(6, 3, 0)});
}

// Used for evaluating expressions
struct expression_evaluator
{
	string CSECT;

	expression_evaluator(string CSECT)
	{
		this -> CSECT = CSECT;
	}
	
	int precedence(string op)
	{
		if(op == "+" || op == "-")
			return 1;
		if(op == "*" || op == "/")
			return 2;
		return 0;
	}

    // direct application between two operators
	pair<int, int> operation(pair<int, int> a, pair<int, int> b, string op)
	{
		if(op == "+") return make_pair(a.first + b.first, a.second + b.second);
		if(op == "-") return make_pair(a.first - b.first, a.second - b.second);
		if(op == "*") return make_pair(a.first * b.first, a.second * b.second);
		if(op == "/") return make_pair(0, a.second / b.second);
		return make_pair(0, 0);
	}

    // tokenizing a given expression
	vector<string> tokenize(string exp)
	{
		int n = exp.length();
		vector<string> tokens;
		int i = 0;
		while(i < n)
		{
			if(exp[i] == '*' || exp[i] == '+' || exp[i] == '-' || exp[i] == '/' || exp[i] == ')' || exp[i] == '(')
			{
				tokens.push_back(exp.substr(i, 1));
				i++;
			}
			else
			{
				int j = i;
				while(j < n && !(exp[j] == '*' || exp[j] == '+' || exp[j] == '-' || exp[j] == '/' || exp[j] == '(' || exp[j] == ')'))
					j++;
				tokens.push_back(exp.substr(i, j - i));
				i = j;
			}
		}
		return tokens;
	}

	// if *, / and at least one of the operands has type 1 ->false
	// total_type = 0 or 1, that's it, otherwise -> false
	
    // evaluation an expression
	pair<int, int> evaluate(string exp)
	{
		vector<string> tokens = tokenize(exp);
		stack<pair<int, int>> operands;
		stack<string> operators;
		int n = tokens.size();
		
		for(int i = 0; i < n; i++)
		{
			if(tokens[i] == "(")
			{
				operators.push("(");
			}
			else if(is_number(tokens[i]))
			{
				operands.push(make_pair(0, stoi(tokens[i])));
			}
			else if(tokens[i] == ")")
			{
				while(!operators.empty() && operators.top() != "(")
				{
					pair<int, int> operand_2 = operands.top();
					operands.pop();

					pair<int, int> operand_1 = operands.top();
					operands.pop();

					string op = operators.top();
					operators.pop();
					
					if(op == "*" || op == "/")
					{
						if(operand_1.first == 1 || operand_2.first == 1)
						{
							exit(1);
						}
					}

					operands.push(operation(operand_1, operand_2, op));
				}
				if(!operators.empty())
				{
					operators.pop();
				}
			}
			else if(tokens[i] == "+" || tokens[i] == "-" || tokens[i] == "*" || tokens[i] == "/")
			{
				while(!operators.empty() && precedence(operators.top()) >= precedence(tokens[i]))
				{
					pair<int, int> operand_2 = operands.top();
					operands.pop();

					pair<int, int> operand_1 = operands.top();
					operands.pop();

					string op = operators.top();
					operators.pop();

					if(op == "*" || op == "/")
					{
						if(operand_1.first == 1 || operand_2.first == 1)
						{
							exit(1);
						}
					}
					operands.push(operation(operand_1, operand_2, op));
				}
				operators.push(tokens[i]);
			}	
			else
			{
				if(SYMTAB.find(symbol(CSECT, tokens[i])) != SYMTAB.end())
				{
					int value = SYMTAB[symbol(CSECT, tokens[i])].value_sym;
					int type = SYMTAB[symbol(CSECT, tokens[i])].type;
					operands.push(make_pair(type, value));
				}
				else
				{
					// ????
					if(EXTREF.find(tokens[i]) != EXTREF.end())
					{
					}
					operands.push(make_pair(0, 0));
				}
			}	
		}
		while(!operators.empty())
		{
			pair<int, int> operand_2 = operands.top();
			operands.pop();

			pair<int, int> operand_1 = operands.top();
			operands.pop();

			string op = operators.top();
			operators.pop();

			if(op == "*" || op == "/")
			{
				if(operand_1.first == 1 || operand_2.first == 1)
				{
					exit(1);
				}
			}

			operands.push(operation(operand_1, operand_2, op));
		}
		return operands.top();
	}
};

// Used for storing the details of an instruction
struct instruction
{
	string label;
	string opcode;
	string operands;
	int length;
	int n,i,b,p,e,x;
	bool literal;

	// Constructor
	instruction(string label="", string opcode="", string operands="", int length=3, int n=1, int i=1, int b=0, int p=1, int e=0, int x=0, bool literal =false)
	{
		this->label = label;
		this->opcode = opcode;
		this->operands = operands;
		this->length = length;
		this->n = n;
		this->i = i;
		this->b = b;
		this->p = p;
		this->e = e;
		this->x = x;
		this->literal = literal;
	}

	// get all operands
	vector<string> get_all_operands()
	{
		vector<string> all_operands;
		int i = 0;
		int l = operands.length();
		while(i < l)
		{
			int j = i + 1;
			while(j < l && operands[j] != ',')
				j++;
			all_operands.push_back(operands.substr(i, j - i));
			i = j + 1;
		}
		return all_operands;
	}

	// whether an instruction is comment or not
	bool is_comment()
	{
		return (label == ".");
	}

	// whether an instruction has operands or not
	bool has_operands()
	{
		return (operands != "");
	}

	// whether an instruction has multiple operands or not
	bool has_multiple_operands()
	{
		return (operands.find(",") != string::npos);
	}

	// whether an instruction has label or not
	bool has_label()
	{
		return (label != "");
	}

	// work in the flags for opcode
	void format_opcode(int& op)
	{
		if(length == 3 || length == 4)
		{
			op |= i;
			op |= (n << 1);
		}
	}

	// work in the flags for operands
	void format_operand(int& operand)
	{
		if(length == 3 || length == 4)
		{
			// apply mask for negative operand
			int mask = 0;
			for(int i = 0; i < ((length == 3) ? 12 : 20); i++)
			{
				mask |= 1;
				mask = mask << 1;
			}
			mask = mask >> 1;
			operand = operand & mask;


			if(!e)
			{
				operand |= (p << 13);
				operand |= (b << 14);
				operand |= (x << 15);
			}
			else
			{
				operand |= (e << 20);
				operand |= (p << 21);
				operand |= (b << 22);
				operand |= (x << 23);
			}
		}
	}
};

// Function to process a line in the input file
int processing_line(string line, instruction &instr, bool input = true)
{
	int offset = (input) ? 0: LENGTH;
	string LOCCTR = "";

	// Get the label
	instr.label = strip_whitespaces(line.substr(offset, LENGTH));

	// Check if extended instruction
	if(line[offset + LENGTH] == '+')
	{
		instr.e = 1;
		instr.n = 1;
		instr.i = 1;
		instr.p = 0;
		instr.b = 0;
	}

	// Get the opcode
	instr.opcode = strip_whitespaces(line.substr(offset + LENGTH + 1, LENGTH));

	// Get the operands
	instr.operands = strip_whitespaces(line.substr(offset + 2 * LENGTH + 2, OPERAND_LENGTH));

	// Check for multiple operands
	if(line[offset + 2 * LENGTH + 1] != ' ')
	{
		char x = line[offset + 2 * LENGTH + 1];
		// immediate
		if(x == '#')
		{
			instr.i = 1;
			instr.n = 0;
			if(is_number(instr.operands))
			{
				instr.p = 0;
				instr.b = 0;
			}
		}
		// indirect
		else if(x == '@')
		{
			instr.i = 0;
			instr.n = 1;
		}
		// literal
		else if(x == '=')
		{
			instr.literal = true;
		}
	}

	// When pass 2
	if(!input)
	{
		LOCCTR = strip_whitespaces(line.substr(0, LENGTH));
		if(!LOCCTR.empty())
			return stoi(LOCCTR, NULL, 16);
	}
	return -1;
}

int PROGADDR;
int LAST_ADDR;

map<string, int> ESTAB;
vector<string> Memory((1 << 20), "xx");

// tokenize the symbols in the define record.
vector<pair<string, int>> Get_Define_Records(string record)
{
    vector<pair<string, int>> symbols;

    for (int idx = 1; idx < record.length(); idx += 12)
    {
        string name = record.substr(idx, 6);
        string relative_addr = record.substr(idx + 6, 6);
        int Relative_Address = stoi(relative_addr, NULL, 16);
        symbols.push_back({name, Relative_Address});
    }
    return symbols;
}

// prints in the format as given in memory.txt
void Print_Memory_Execution()
{
    ofstream fout("memory.txt");

    int i = (PROGADDR / 16) * 16;
    int n = ((LAST_ADDR + 16) / 16) * 16;
    while (i < n)
    {
        fout << hexadecimal(i, 4) << " ";
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                fout << Memory[i++];
            }
            fout << " ";
        }
        fout << endl;
    }
}

void Print_All_Memory()
{
    ofstream fout("memory.txt");

    int i = 0;
    int n = Memory.size();
    while (i < n)
    {
        fout << hexadecimal(i, 4) << " ";
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                fout << Memory[i++];
            }
            fout << " ";
        }
        fout << endl;
    }
}

bool Start_Found = false; // Boolean for 'START'
bool End_Found = false; // Boolean for 'END'