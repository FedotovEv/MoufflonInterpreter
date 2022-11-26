
// Демонстрация взаимодействия с интерпретатором Муфлона с использованием пользовательского выходного потока.

#include "..\declares.h"
#include "..\throw_messages.h"
#include "..\lexer.h"
#include "..\parse.h"
#include "..\runtime.h"
#include "..\statement.h"

#include <iostream>
#include <streambuf>
#include <string>
#include <variant>
#include <map>

using namespace std;

class my_streambuf : public streambuf
{
    int_type overflow(int_type c) override
    {
        if (c != '\n' && c >= 0 && c <= 255)
        {
            command_accumulator += c;
        }
        else
        {
            int command_end_pos = command_accumulator.find(' ');
            if (command_end_pos == string::npos)
                command_end_pos = command_accumulator.size();
            string command_word = command_accumulator.substr(0, command_end_pos);
            string command_args;
            if (command_end_pos  < static_cast<int>(command_accumulator.size()) - 1)
                command_args = command_accumulator.substr(command_end_pos + 1);
            cout << command_word << " : " << command_args << endl;
            command_accumulator.clear();
        }
        return 1;
    }

private:
    string command_accumulator;
};

void RunMythonProgram(istream & input, ostream & output)
{
    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::SimpleContext context(output);
    runtime::Closure closure;
    program->Execute(closure, context);
}

int main()
{
    istringstream input(R"--(
print "close window"
print "open door"
print
)--");

    my_streambuf my_buf;
    ostream special_stream(&my_buf);

    RunMythonProgram(input, special_stream);
    return 0;
}
