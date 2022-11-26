
// Макетная программа для демонстрации техники применения объекта __external

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
#include <algorithm>
#include <cctype>

using namespace std;

runtime::LinkageReturn DemoLinkFunction(runtime::LinkCallReason what_reason, const string& field_name,
    const vector<string>& argument_value)
{
    enum CommandCode
    {
        COMMAND_AS_IS = 1,
        COMMAND_REVERSE,
        COMMAND_UPCASE,
        COMMAND_LOWCASE
    };

    static int command_code = 0;
    static string command_argument;
    string new_string;

    switch (what_reason)
    {
    case runtime::LinkCallReason::CALL_REASON_WRITE_FIELD:
        if (field_name == "command_code")
            command_code = stoi(argument_value[0]);
        else if (field_name == "command_argument")
            command_argument = argument_value[0];
        return {};
    case runtime::LinkCallReason::CALL_REASON_READ_FIELD:
        if (field_name == "command_result")
            switch (command_code)
            {
            case COMMAND_AS_IS:
                return command_argument;
            case COMMAND_REVERSE:
                new_string = command_argument;
                reverse(new_string.begin(), new_string.end());
                return new_string;
            case COMMAND_UPCASE:
                new_string = command_argument;
                for_each(new_string.begin(), new_string.end(), [](char& c) {c = toupper(c); });
                return new_string;
            case COMMAND_LOWCASE:
                new_string = command_argument;
                for_each(new_string.begin(), new_string.end(), [](char& c) {c = tolower(c); });
                return new_string;
            default:
                return "bad command"s;
            }
        else if (field_name == "command_code")
            return command_code;
        else if (field_name == "command_argument")
            return command_argument;
        else
            return "bad field"s;
    case runtime::LinkCallReason::CALL_REASON_CALL_METHOD:
        if (field_name == "external_method")
            return "external return"s;
        else
            return "something"s;
    }
    return {};
}

void RunMythonProgram(istream& input, ostream& output, const runtime::LinkageFunction& link_function)
{
    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::SimpleContext context(output, link_function);
    runtime::Closure closure;
    program->Execute(closure, context);
}

int main()
{
    istringstream input(R"--(
class __external:
  def __init__():
    self.commad_code = 0
    self.command_argument = 0
    self.command_result = 0

  def external_method():
    print "In external method"

exts = __external()
exts.command_argument = "ArGuMeNt"
exts.command_code = 1
print exts.command_result
exts.command_code = 2
print exts.command_result
exts.command_code = 3
print exts.command_result
exts.command_code = 4
print exts.command_result
print exts.external_method()
)--");

    RunMythonProgram(input, cout, DemoLinkFunction);
    return 0;
}
