
#include "../lexer.h"
#include "../parse.h"
#include "../runtime.h"
#include "../statement.h"
#include "../test_runner_p.h"

#include <iostream>
#include <string>
#include <variant>
#include <map>
#include <filesystem>

using namespace std;

void RunMythonProgram(istream& input, ostream& output)
{
    parse::TrivialParseContext parse_context(true);
    runtime::SimpleContext context(output);
    runtime::Closure closure;

    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer, parse_context);
    program->Execute(closure, context);
}

void TestCreateFile()
{
    istringstream create_file(R"(
import "MythonFilePlugin"
files_object = MythonFilePlugin()
print files_object.IsOpen()

files_object.open("testfile.txt", "w")
print files_object.IsOpen()

files_object.write("First String")
files_object.write(111)

s = "Second String"
files_object.write(s)
files_object.write(3.1415925)
files_object.write("Third String")

ps = 2.17
files_object.write(ps)

files_object.close()
print files_object.IsOpen()
)");

    ostringstream output_create;
    RunMythonProgram(create_file, output_create);
    ASSERT_EQUAL(output_create.str(), "False\nTrue\nFalse\n");
    ASSERT_EQUAL(filesystem::exists("testfile.txt"), true);

    istringstream read_file(R"(
import "MythonFilePlugin"
files_object = MythonFilePlugin()
files_object.open("testfile.txt", "r")
print files_object.read(12)
files_object.close()
)");

    ostringstream output_read;
    RunMythonProgram(read_file, output_read);
    ASSERT_EQUAL(output_read.str(), "First String\n");
    ASSERT_EQUAL(filesystem::exists("testfile.txt"), true);

    istringstream delete_file(R"(
import "MythonFilePlugin"
files_object = MythonFilePlugin()
files_object.remove("testfile.txt")
)");

    ostringstream output_delete;
    RunMythonProgram(delete_file, output_delete);
    ASSERT_EQUAL(!filesystem::exists("testfile.txt"), true);
}

int main()
{
    try
    {
        TestCreateFile();
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
		return 1;
    }

    return 0;
}
