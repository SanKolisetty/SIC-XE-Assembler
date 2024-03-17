ASSIGNMENT-2: SIC/XE 2 PASS ASSEMBLER AND LINKER LOADER
Sanjana Siri Kolisetty 210101093

ASSUMPTIONS:
--> Spacing is 10 for label and opcode and 30 for the operands. 
--> Extra column is provided for '+', '=' and '#'.
--> The program name is assumed to be atmost 6 characters long.
--> Make sure there are no spaces in the expressions in the input file.

ABOUT THE CODE:
--> strip_whitespaces(): removes trailing spaces
--> hexadecimal() and Format_String(): for pretty printing the code
--> is_number(): to check if a string is a number or not
--> mask(): so that printing can be done in two's complement properly (only for negative numbers)
--> OPTAB: returns opcode along with instruction length
--> symbol: for storing symbol name along with their CSECT
--> value: to store the address value and type of the SYMBOL
--> map<symbol, value> SYMTAB: symbol table
--> map<symbol, value> LLITTAB: literal table
--> set<string> EXTREF: the set of external references
--> expression_evaluator: It evaluates the expression consisting of (+, -, *, / ()) and returns the value
--> instruction: this structure stores the label, operation code, operand and i, n, p, b, e, x flags for each instruction.
--> vector<string> Memory: the simulated memory for the program
--> map<string, int> ESTAB: symbol table for the external references and csect names
--> processing_line: takes the line from input/intermediate file and tokenize to create instructions
--> preprocess: stores the register names and values in the SYMTAB
--> pass1(): Reads the input file line by line and produces the intermediate file with appropriate LOCCTR values for each line.
--> pass2(): Reads the intermediate file line by line and produces output file and listing file.
--> vector<pair<string, int>> Get_Define_Records(): tokenize the symbols in the define record.
--> Linking_Loader_Pass_1(): Reads the input record file and stores all the external definitions and csect names. It has been properly commented
--> Linking_Loader_Pass_2(): Reads the input record file and loads the text records in the simulated memory and then loads the modifications. It has been properly commented
--> Print_Memory_Execution(): prints in the format as given in memory.txt

HOW TO RUN:
--> Compile and Run 'MainFile.cpp'
--> Give the Program Address when prompted in the terminal
--> intermediate, listing, output, memory files are created