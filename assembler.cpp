#include <bits/stdc++.h>
#include "helper_functions.cpp"
using namespace std;

void pass1(string input_file)
{
	fstream input, intermediate;

	input.open(input_file, ios::in);
	intermediate.open("intermediate.txt", ios::out);

	int LOCCTR = 0; // Location Counter
	int STADDR = 0; // Starting Address
	int length = 0;
	string control_sect = ""; // Control Section

	while (input.good())
	{
		string line;
		getline(input, line);

		if (line.empty())
		{
			break;
		}

		instruction instr;
		processing_line(line, instr);

		if (instr.is_comment())
		{
			// intermediate << line << endl;
		}
		else
		{
			// start of a control section
			if (instr.opcode == "START" || instr.opcode == "CSECT")
			{
				if(instr.opcode == "START") Start_Found = true;

				// previous control section
				if (control_sect != "")
				{
					length = LOCCTR - STADDR;
					SYMTAB.insert({symbol(control_sect, control_sect), value(STADDR, length)});
				}

				if (instr.has_operands())
				{
					STADDR = stoi(instr.operands, NULL, 16);
				}
				else
				{
					STADDR = 0;
				}

				LOCCTR = STADDR;
				control_sect = instr.label;
				preprocess_symbols(control_sect);
				intermediate << format_string(hexadecimal(LOCCTR, 4), LENGTH) << line << endl;
			}
			else if (instr.opcode == "BASE" || instr.opcode == "EXTDEF" || instr.opcode == "EXTREF")
			{
				intermediate << format_string("", LENGTH) << line << endl;
			}
			else if (instr.opcode == "EQU")
			{
				if (instr.has_label())
				{
					if (SYMTAB.find(symbol(control_sect, instr.label)) != SYMTAB.end())
					{
						cout << "Duplicate symbol error" << endl;
						exit(1);
					}
					else
					{
						if (instr.operands == "*")
						{
							SYMTAB.insert({symbol(control_sect, instr.label), value(LOCCTR)});
							intermediate << format_string(hexadecimal(LOCCTR, 4), LENGTH) << line << endl;
						}
						else if (is_number(instr.operands))
						{
							SYMTAB.insert({symbol(control_sect, instr.label), value(stoi(instr.opcode), 3, 0)});
							intermediate << format_string(hexadecimal(stoi(instr.operands), 4), LENGTH) << line << endl;
						}
						else
						{
							expression_evaluator e(control_sect);
							int val = e.evaluate(instr.operands).second;
							int type = e.evaluate(instr.operands).first;
							if (!(type == 0 || type == 1))
							{
								cout << "Invalid Expression in EQU" << endl;
								exit(1);
							}
							mask(val, 16);
							intermediate << format_string(hexadecimal(val, 4), LENGTH) << line << endl;
						}
					}
				}
			}
			else if (instr.opcode == "END" || instr.opcode == "LTORG")
			{
				if(instr.opcode == "END") End_Found = true;
				intermediate << format_string("", LENGTH) << line << endl;

				for (auto &x : LITTAB)
				{
					if (x.second.value_sym == -1)
					{
						intermediate << format_string(hexadecimal(LOCCTR, 4), LENGTH) << format_string("*", LENGTH) << "=" << format_string(x.first.symbol_name, LENGTH) << " " << format_string("", OPERAND_LENGTH) << endl;
						x.second.value_sym = LOCCTR;

						if (x.first.symbol_name.front() == 'C')
							LOCCTR += (x.first.symbol_name.length() - 3);
						else if (x.first.symbol_name.front() == 'X')
							LOCCTR += ((x.first.symbol_name.length() - 3) / 2);
						else
							LOCCTR += 3;
					}
				}

				if (instr.opcode == "END")
				{
					length = LOCCTR - STADDR;
					SYMTAB.insert({symbol(control_sect, control_sect), value(STADDR, length)});
				}
			}
			else
			{
				intermediate << format_string(hexadecimal(LOCCTR, 4), LENGTH) << line << endl;

				if (instr.literal)
				{
					if (LITTAB.find(symbol(control_sect, instr.operands)) == LITTAB.end())
					{
						LITTAB.insert({symbol(control_sect, instr.operands), value(-1)});
					}
				}

				if (instr.has_label())
				{
					if (SYMTAB.find(symbol(control_sect, instr.label)) == SYMTAB.end())
					{
						SYMTAB.insert({symbol(control_sect, instr.label), value(LOCCTR)});
					}
					else
					{
						cout << "Duplicate symbol" << endl;
						exit(1);
					}
				}

				if (OPTAB(instr.opcode) != make_pair(-1, -1))
				{
					pair<int, int> format = OPTAB(instr.opcode);
					if (instr.e)
					{
						format.first++;
					}
					instr.length = format.first;
					LOCCTR += instr.length;
				}
				else if (instr.opcode == "WORD")
					LOCCTR += 3;
				else if (instr.opcode == "RESW")
					LOCCTR += 3 * stoi(instr.operands);
				else if (instr.opcode == "RESB")
					LOCCTR += stoi(instr.operands);
				else if (instr.opcode == "BYTE")
				{
					if (instr.operands.front() == 'C')
						LOCCTR += (instr.operands.length() - 3);
					if (instr.operands.front() == 'X')
						LOCCTR += ((instr.operands.length() - 3) / 2);
				}
				else
				{
					cout << "Invalid Operation Code " << instr.opcode << endl;
					exit(1);
				}
			}
		}
	}

	input.close();
	intermediate.close();
}

void pass2(string input)
{
	fstream intermediate, listing, output;
	intermediate.open(input, ios::in);
	listing.open("listing.txt", ios::out);
	output.open("output.txt", ios::out);

	int LOCCTR = 0, BASE = 0, PC = 0, START = 0;
	string CSECT = "", PROGNAME = "", text = "";

	// for record storing
	map<string, vector<string>> text_list, modification_list;
	map<string, string> header_list, end_list, define_list, refer_list;
	vector<string> CSECTS;

	// CSECTS -> to store the names of all control sections
	// define_list -> to store each control section's (control_section, define string (symbol, address))
	// refer_list -> to store each control section's (control_section, refer string(symbols))
	// header_list -> to store the header of each control section
	// text_list -> to store the text of each control section, here for every control section, we ll have each line stored as a string, therefore
	// we have a vector of strings

	// WHAT HAPPENS FOR EACH OPCODE
	// 1. EXTDEF/EXTREF -> We just make the string and put it in the define_list/refer_list
	// 2. BASE -> we check if the operand is in symbol table, if yes, we put it's value in BASE
	// 3. LTORG -> Nothing
	// 4. START -> get the CSECT, STADDR, LENGTH, push CSECT in the CSECTS list, create the header and push it in header_list
	// 5. CSECT -> write the leftover previous into text_list and do the same thing as START for this new control section
	// 6. END -> write the e in end_list
	// 7. EQU -> Nothing
	// 8. Else ->

	// reading intermediate file
	while (intermediate.good())
	{
		string line;
		getline(intermediate, line);
		// end of file
		if (line.empty())
		{
			break;
		}
		
		// processing line into tokens
		instruction instr;
		LOCCTR = processing_line(line, instr, false);

		if (!instr.is_comment())
		{
			if (instr.opcode == "START")
			{
				// write listing for the instruction
				listing << line << '\n';

				// csection name
				CSECT = instr.label;
				PROGNAME = CSECT;
				CSECTS.push_back(CSECT);

				// starting address for the csection
				int STADDR = 0;
				if (instr.has_operands())
					STADDR = stoi(instr.operands, NULL, 16);
				else
					STADDR = 0;

				// length of the csection
				int length = 0;
				if (SYMTAB.find(symbol(CSECT, CSECT)) != SYMTAB.end())
					length = SYMTAB[symbol(CSECT, CSECT)].length;

				// write the output machine code
				stringstream header;
				header << "H";
				header << format_string(CSECT, 6);
				header << hexadecimal(STADDR, 6);
				header << hexadecimal(length, 6);
				header_list.insert({CSECT, header.str()});
			}
			else if (instr.opcode == "END")
			{
				// write listing for the instruction
				listing << line << '\n';

				// write the output machine code
				stringstream end;
				int first = SYMTAB[symbol(CSECT, instr.operands)].value_sym;
				end << "E";
				end_list.insert({CSECT, end.str()});

				end << hexadecimal(first, 6);
				end_list[PROGNAME] = end.str();
			}
			else if (instr.opcode == "CSECT")
			{
				// write listing for the instruction
				listing << line << '\n';

				// write the left over output machine code of the previous CSECT
				if (text.length())
				{
					int length = (text.length()) / 2;
					stringstream text_record;
					text_record << "T";
					text_record << hexadecimal(START, 6);
					text_record << hexadecimal(length, 2);
					text_record << text;

					text_list[CSECT].push_back(text_record.str());

					text = "";
					START = 0;
				}

				stringstream end;
				end << "E";
				end_list.insert({CSECT, end.str()});

				// starting address, length of the new CSECT
				int STADDR = 0;
				CSECT = instr.label;
				CSECTS.push_back(CSECT);

				int length = 0;
				if (SYMTAB.find(symbol(CSECT, CSECT)) != SYMTAB.end())
					length = SYMTAB[symbol(CSECT, CSECT)].length;

				// write the output machine code
				stringstream header;
				header << "H";
				header << format_string(CSECT, 6);
				header << hexadecimal(STADDR, 6);
				header << hexadecimal(length, 6);
				header_list.insert({CSECT, header.str()});
			}
			else if (instr.opcode == "EXTDEF" || instr.opcode == "EXTREF")
			{
				// write listing for the instruction
				listing << line << '\n';

				// write the output machine code
				vector<string> operands = instr.get_all_operands();
				if (instr.opcode == "EXTDEF")
				{
					stringstream def;
					def << "D";
					for (string operand : operands)
					{
						def << format_string(operand, 6);
						int address = 0;
						if (SYMTAB.find(symbol(CSECT, operand)) != SYMTAB.end())
							address = SYMTAB[symbol(CSECT, operand)].value_sym;
						def << hexadecimal(address, 6);
					}
					define_list.insert({CSECT, def.str()});
				}
				else
				{
					// clear EXTREF list and update according to new CSECT
					EXTREF.clear();
					for (auto x : operands)
						EXTREF.insert(x);

					stringstream refer;
					refer << "R";
					for (string operand : operands)
						refer << format_string(operand, 6);
					refer_list.insert({CSECT, refer.str()});
				}
			}
			else if (instr.opcode == "BASE")
			{
				// write listing for the instruction
				listing << line << '\n';

				// set BASE register location for base relative addressing
				if (SYMTAB.find(symbol(CSECT, instr.operands)) != SYMTAB.end())
					BASE = SYMTAB[symbol(CSECT, instr.operands)].value_sym;
			}
			else if (instr.opcode == "LTORG")
			{
				// write listing for the instruction
				listing << line << '\n';
			}
			else if (instr.opcode == "EQU")
			{
				// write listing for the instruction
				listing << line << '\n';
			}
			else
			{
				string obcode = "";

				if (OPTAB(instr.opcode) != make_pair(-1, -1))
				{
					// generate op
					pair<int, int> format = OPTAB(instr.opcode);

					// extended operation instruction
					if (instr.e)
						format.first++;
					instr.length = format.first;

					int op = format.second;
					instr.format_opcode(op);

					// update program counter
					PC = LOCCTR + instr.length;

					// generate operandcode
					int operandcode = 0;
					if (instr.has_operands())
					{
						// format operands
						if (instr.has_multiple_operands())
						{
							vector<string> m_operands = instr.get_all_operands();
							// register-register instructions
							if (instr.length == 2)
							{
								for (string op : m_operands)
								{
									if (SYMTAB.find(symbol(CSECT, op)) != SYMTAB.end())
									{
										int x = SYMTAB[symbol(CSECT, op)].value_sym;
										operandcode = operandcode << 4;
										operandcode += x;
									}
									else
									{
										perror("register not found");
										exit(1);
									}
								}
							}
							// instructions of type BUFFER,X
							else
							{
								int x = 0;
								if (SYMTAB.find(symbol(CSECT, m_operands.front())) != SYMTAB.end())
								{
									x = SYMTAB[symbol(CSECT, m_operands.front())].value_sym;
									operandcode += x;
									if (instr.p)
									{
										// decide between PC relative and base relative
										if (operandcode - PC >= -2048 && operandcode - PC <= 2047)
											operandcode -= PC;
										else if (operandcode - BASE >= 0 && operandcode - BASE <= 4095)
										{
											operandcode -= BASE;
											instr.p = 0;
											instr.b = 1;
										}
										else
										{
											perror("can't fit PC or BASE");
											exit(1);
										}
									}

									if (instr.e)
									{
										stringstream modification;
										modification << "M";
										modification << hexadecimal(LOCCTR + 1, 6);
										modification << "05+";
										modification << format_string(CSECT, 6);
										modification_list[CSECT].push_back(modification.str());
									}
								}
								else
								{
									operandcode += x;
									if (EXTREF.find(m_operands.front()) != EXTREF.end())
									{
										stringstream modification;
										modification << "M";
										modification << hexadecimal(LOCCTR + 1, 6);
										modification << "05+";
										modification << format_string(m_operands.front(), 6);
										modification_list[CSECT].push_back(modification.str());
									}
								}
								instr.x = 1;
							}
						}
						else
						{
							// if instruction has literal value then get its address
							if (instr.literal)
							{
								if (LITTAB.find(symbol(CSECT, instr.operands)) != LITTAB.end())
								{
									operandcode = LITTAB[symbol(CSECT, instr.operands)].value_sym;
									// decide between PC relative and BASE relative
									if (instr.p)
									{
										if (operandcode - PC >= -2048 && operandcode - PC <= 2047)
											operandcode -= PC;
										else if (operandcode - BASE >= 0 && operandcode - BASE <= 4095)
										{
											operandcode -= BASE;
											instr.p = 0;
											instr.b = 1;
										}
										else
										{
											perror("can't fit PC or BASE");
											exit(1);
										}
									}
								}
								else
								{
									// symbol not found in literal table -> that should not happen
									perror("my error");
									exit(1);
								}
							}
							// if instruction has operand symbol then get its address
							else if (SYMTAB.find(symbol(CSECT, instr.operands)) != SYMTAB.end())
							{
								operandcode = SYMTAB[symbol(CSECT, instr.operands)].value_sym;
								if (instr.length == 2)
								{
									operandcode = operandcode << 4;
								}
								else
								{
									if (instr.p)
									{
										// decide between PC relative and BASE relative
										if (operandcode - PC >= -2048 && operandcode - PC <= 2047)
											operandcode -= PC;
										else if (operandcode - BASE >= 0 && operandcode - BASE <= 4095)
										{
											operandcode -= BASE;
											instr.p = 0;
											instr.b = 1;
										}
										else
										{
											perror("can't fit PC or BASE");
											exit(1);
										}
									}

									if (instr.e)
									{
										stringstream modification;
										modification << "M";
										modification << hexadecimal(LOCCTR + 1, 6);
										modification << "05+";
										modification << format_string(CSECT, 6);
										modification_list[CSECT].push_back(modification.str());
									}
								}
							}
							// if operand is immediate or external reference
							else
							{
								if (EXTREF.find(instr.operands) != EXTREF.end())
								{
									stringstream modification;
									modification << "M";
									modification << hexadecimal(LOCCTR + 1, 6);
									modification << "05+";
									modification << format_string(instr.operands, 6);
									modification_list[CSECT].push_back(modification.str());
								}
								if (instr.i)
								{
									if (is_number(instr.operands))
										operandcode = stoi(instr.operands);
									else
										;
								}
							}
						}

						instr.format_operand(operandcode);
					}

					// writing listing for the instruction
					obcode = hexadecimal(op, 2) + hexadecimal(operandcode, 2 * (instr.length - 1));
					listing << line << format_string(obcode, 10) << '\n';
				}
				else
				{
					// process constants
					if (instr.label == "*")
					{
						string constant = instr.opcode.substr(2, instr.opcode.length() - 3);
						if (instr.opcode.front() == 'X')
						{
							int x = stoi(constant, nullptr, 16);
							obcode = hexadecimal(x, constant.length());
						}
						else if (instr.opcode.front() == 'C')
						{
							for (char ch : constant)
							{
								int x = ch;
								obcode += hexadecimal(x, 2, false);
							}
						}
						else
						{
							int word = stoi(instr.operands);
							obcode = hexadecimal(word, 6, false);
						}
					}

					if (instr.opcode == "WORD")
					{
						if (is_number(instr.operands))
						{
							int word = stoi(instr.operands);
							obcode = hexadecimal(word, 6);
						}
						else
						{
							expression_evaluator e(CSECT);
							int word = e.evaluate(instr.operands).second;
							mask(word, 24);
							obcode = hexadecimal(word, 6);

							vector<string> m_operands = e.tokenize(instr.operands);
							for (int i = 0; i < (int)(m_operands.size()); i++)
							{
								if (EXTREF.find(m_operands[i]) != EXTREF.end())
								{
									stringstream modification;
									modification << "M";
									modification << hexadecimal(LOCCTR, 6);
									modification << "06";
									if (i > 0 && m_operands[i - 1] == "-")
										modification << "-";
									else
										modification << "+";
									modification << format_string(m_operands[i], 6);
									modification_list[CSECT].push_back(modification.str());
								}
							}
						}
					}

					if (instr.opcode == "BYTE")
					{
						string constant = instr.operands.substr(2, instr.operands.length() - 3);
						if (instr.operands.front() == 'X')
						{
							int x = stoi(constant, nullptr, 16);
							obcode = hexadecimal(x, constant.length());
						}

						if (instr.operands.front() == 'C')
						{
							for (char ch : constant)
							{
								int x = ch;
								obcode += hexadecimal(x, 2, false);
							}
						}
					}

					// writing listing for the instruction
					listing << line << format_string(obcode, 10) << '\n';
				}

				// write the output machine code
				if (text.length() == 0)
					START = LOCCTR;

				if (text.length() + obcode.length() <= 60 && instr.opcode != "RESW" && instr.opcode != "RESB")
					text += obcode;
				else
				{
					if (text.length())
					{
						int length = (text.length()) / 2;
						stringstream text_record;
						text_record << "T";
						text_record << hexadecimal(START, 6);
						text_record << hexadecimal(length, 2);
						text_record << text;

						text_list[CSECT].push_back(text_record.str());

						text = obcode;
						START = LOCCTR;
					}
				}
			}
		}
		else
			listing << line << '\n';
	}

	if (text.length())
	{
		int length = (text.length()) / 2;
		stringstream text_record;
		text_record << "T";
		text_record << hexadecimal(START, 6);
		text_record << hexadecimal(length, 2);
		text_record << text;

		text_list[CSECT].push_back(text_record.str());

		text = "";
		START = 0;
	}

	for (string CSECT : CSECTS)
	{
		output << header_list[CSECT] << '\n';
		if (!define_list[CSECT].empty())
			output << define_list[CSECT] << '\n';
		if (!refer_list[CSECT].empty())
			output << refer_list[CSECT] << '\n';
		for (string text_record : text_list[CSECT])
			output << text_record << '\n';
		for (string modification_record : modification_list[CSECT])
			output << modification_record << '\n';
		output << end_list[CSECT] << '\n';
	}

	// closing files
	intermediate.close();
	output.close();
	listing.close();
}

