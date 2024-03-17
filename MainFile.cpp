#include <bits/stdc++.h>
using namespace std;
#include "linking_loader_Pass2.cpp"

int main()
{
    ios::sync_with_stdio(0);
    cin.tie(0);

    // Running pass1 assembler
    pass1("input.txt");

    // Running pass2 assembler
    pass2("intermediate.txt");

    // 'START' not found
    if (!Start_Found)
    {
        cout<<"START instruction not found!"<<endl;
    }

    // 'END' not found
    if (!End_Found)
    {
        cout<<"END instruction not found!"<<endl;
    }

    // Running Linking Loader Pass 1
    Linking_Loader_Pass_1();

    // Running Linking Loader Pass 2
    Linking_Loader_Pass_2();

    // Printing the memory execution
    Print_Memory_Execution();
}