
#include "lexer.h"
#include "parse.h"
#include "runtime.h"
#include "statement.h"
#include "test_runner_p.h"

#include <iostream>
#include <string>
#include <variant>
#include <map>

using namespace std;

namespace parse {
void RunOpenLexerTests(TestRunner& tr);
}  // namespace parse

namespace ast {
void RunUnitTests(TestRunner& tr);
}

namespace runtime {
void RunObjectHolderTests(TestRunner& tr);
void RunObjectsTests(TestRunner& tr);
}  // namespace runtime

void TestParseProgram(TestRunner& tr);

namespace {

void RunMythonProgram(istream& input, ostream& output, const runtime::LinkageFunction& link_function = {})
{
    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::SimpleContext context(output, link_function);
    runtime::Closure closure;
    program->Execute(closure, context);
}

void TestSimplePrints() {
    istringstream input(R"(
print 57
print 10, 24, -8
print 'hello'
print "world"
print True, False
print
print None
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "57\n10 24 -8\nhello\nworld\nTrue False\n\nNone\n");
}

void TestAssignments() {
    istringstream input(R"(
x = 57
print x
x = 'C++ black belt'
print x
y = False
x = y
print x
x = None
print x, y
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "57\nC++ black belt\nFalse\nNone False\n");
}

void TestArithmetics() {
    istringstream input("print 1+2+3+4+5, 1*2*3*4*5, 1-2-3-4-5, 36/4/3, 2*5+10/2");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "15 120 -13 3 15\n");
}

void TestVariablesArePointers() {
    istringstream input(R"(
class Counter:
  def __init__():
    self.value = 0

  def add():
    self.value = self.value + 1

class Dummy:
  def do_add(counter):
    counter.add()

x = Counter()
y = x

x.add()
y.add()

print x.value

d = Dummy()
d.do_add(x)

print y.value
)");

    ostringstream output;
    RunMythonProgram(input, output);

    ASSERT_EQUAL(output.str(), "2\n3\n");
}

void TestSelfInConstructor()
{
    istringstream input(R"--(
class X:
  def __init__(p):
    p.x = self

class XHolder:
  def __init__():
    dummy = 0

xh = XHolder()
x = X(xh)
)--");
    
    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::DummyContext context;
    runtime::Closure closure;
    program->Execute(closure, context);
    const auto* xh = closure.at("xh"s).TryAs<runtime::ClassInstance>();
    ASSERT(xh != nullptr);
    ASSERT_EQUAL(xh->Fields().at("x"s).Get(), closure.at("x"s).Get());
}

void TestExternalObject()
{
    istringstream input(R"--(
class __external:
  def __init__():
    self.arg1 = 0
    self.arg2 = 0

exts = __external()
exts.arg1 = 2
print exts.arg2
)--");

    ostringstream ostr;
    auto lambda_link = [&ostr](runtime::LinkCallReason what_reason, const string& field_name,
                               const vector<string>& argument_value) -> runtime::LinkageReturn
                        {
                            switch (what_reason)
                            {
                                case runtime::LinkCallReason::CALL_REASON_WRITE_FIELD:
                                    ostr << field_name << " = " << argument_value[0] << endl;
                                    return {};
                                case runtime::LinkCallReason::CALL_REASON_READ_FIELD:
                                    ostr << "Reading " << field_name << endl;
                                    return "empty"s;
                                case runtime::LinkCallReason::CALL_REASON_CALL_METHOD:
                                    ostr << "Reading " << field_name << endl;
                                    return "executed"s;
                            }
                        };

    RunMythonProgram(input, ostr, lambda_link);
    ASSERT_EQUAL(ostr.str(), "arg1 = 0\narg2 = 0\narg1 = 2\nReading arg2\nempty\n"s);
}

void TestAll()
{
    TestRunner tr;
    parse::RunOpenLexerTests(tr);
    runtime::RunObjectHolderTests(tr);
    runtime::RunObjectsTests(tr);
    ast::RunUnitTests(tr);
    TestParseProgram(tr);

    RUN_TEST(tr, TestSimplePrints);
    RUN_TEST(tr, TestAssignments);
    RUN_TEST(tr, TestArithmetics);
    RUN_TEST(tr, TestVariablesArePointers);
    RUN_TEST(tr, TestSelfInConstructor);
    RUN_TEST(tr, TestExternalObject);
}

}  // namespace

int main()
{
    try
    {
        TestAll();
        RunMythonProgram(cin, cout);
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
		return 1;
    }

    return 0;
}
