
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

  def inner_method(param1):
    self.arg1 = param1

exts = __external()
exts.arg1 = 2
print exts.arg2
exts.inner_method(4)
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
                                    ostr << "Calling " << field_name << endl;
                                    return "executed"s;
                            }
                        };

    RunMythonProgram(input, ostr, lambda_link);
    ASSERT_EQUAL(ostr.str(), "arg1 = 0\narg2 = 0\narg1 = 2\nReading arg2\nempty\narg1 = 4\nCalling inner_method\n"s);
}

void TestWhileLoop()
{
    { // Проверка цикла while без команд досрочного окончания
        istringstream input(R"--(
i = 5
while i > 0:
  print i
  i = i - 1

print "End"
)--");

        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "5\n4\n3\n2\n1\nEnd\n"s);
    }
    
    { // Проверка цикла while с досрочным завершением по break
        istringstream input(R"--(
i = 10
while i > 0:
  print i
  if i <= 5:
    break
  i = i - 1

print "End"
)--");

        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "10\n9\n8\n7\n6\n5\nEnd\n"s);
    }    

    { // Проверка рвботы while в связке с continue
        istringstream input(R"--(
i = 10
while i > 0:
  i = i - 1
  if i % 2 == 1:
    continue
  print i

print "End"
)--");

        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "8\n6\n4\n2\n0\nEnd\n"s);
    }    
}

void TestArrays()
{
    {
        istringstream input(R"--(
# сначала поработаем с одномерным массивом
x = 2
arr_1d = array(5)
arr_1d.get(2) = 1
arr_1d.get(3) = "str"
arr_1d.push_back(6)
arr_1d.push_back("rts")
print "arr_1d:", arr_1d.get_array_dimensions(), arr_1d.get_dimension_count(1)
# команда выше выводит такую строку: arr_1d: 1 7
print arr_1d.get(3), arr_1d.get(5), arr_1d.back()
# эта команда выведет следующее : str 6 rts
print arr_1d.get(2), arr_1d.get(2) * 2, arr_1d.get(2) * 3
# ну а тут будет получено такое : 1 2 3
)--");
        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "arr_1d: 1 7\nstr 6 rts\n1 2 3\n"s);
    }

    {
        istringstream input(R"--(
# далее некоторые эксперименты с многомерным (двумерным) массивом
x = 3
y = 2
arr_2d = array(6, 7)
arr_2d.get(x, y) = "STR"
arr_2d.get(2, 1) = 21
arr_2d.get(y, x) = "RTS"
print "arr_2d:", arr_2d.get_array_dimensions()
# команда выше выводит такую строку: arr_2d: 2
print arr_2d.get_dimension_count(1), arr_2d.get_dimension_count(2)
# здесь будет следующее: 6 7
print arr_2d.get(x, y), arr_2d.get(2, 3)
# эта команда выведет следующее : STR RTS
print arr_2d.get(2, 1), arr_2d.get(2, 1) * 2, arr_2d.get(2, 1) * 3
# ну а тут будет получено такое : 21 42 63
)--");
        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "arr_2d: 2\n6 7\nSTR RTS\n21 42 63\n"s);
    }
}

void TestMaps()
{
    {
        istringstream input(R"--(
map_var = map()
keyb = "ququ"
map_var.insert(keyb, 32)
if map_var.contains(keyb):
  print map_var.find(keyb)
  map_var.erase(keyb)
if not map_var.contains(keyb):
  print "OK"
else:
  print "Error"
)--");
        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "32\nOK\n"s);
    }

    {
        istringstream input(R"--(
map_var = map()
i = 0
while i < 10:        
  map_var.insert(i, 2 * i)
  i = i + 1
              
map_iter = map_var.begin()
while not map_var.is_iterator_end(map_iter):
  print map_var.key(map_iter), map_var.value(map_iter)
  map_var.next(map_iter)
map_var.release()
)--");
        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "0 0\n1 2\n2 4\n3 6\n4 8\n5 10\n6 12\n7 14\n8 16\n9 18\n"s);
    }
}

void TestIndirectAssignment()
{
    istringstream input(R"--(
class Rect:
  def __init__(w, h):
    self.w = w
    self.h = h

  def get_w():
    return self.w

  def get_h():
    return self.h

  def get_w_ptr():
    return_ptr self.w

  def get_h_ptr():
    return_ptr self.h

x_rect = Rect(10, 20)
print x_rect.w, x_rect.h # Эта команда выведет: 10 20
print x_rect.get_w(), x_rect.get_h() # Эта команда выведет: 10 20
print x_rect.get_w_ptr(), x_rect.get_h_ptr() # Эта команда также выведет: 10 20
x_rect.get_w_ptr() = 100
x_rect.get_h_ptr() = 200
print x_rect.w, x_rect.h # Эта команда выведет: 100 200
)--");

    ostringstream ostr;
    RunMythonProgram(input, ostr);
    ASSERT_EQUAL(ostr.str(), "10 20\n10 20\n10 20\n100 200\n"s);
}

void TestAll()
{
    cout << "Запуск тестов"s << endl;
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
    RUN_TEST(tr, TestWhileLoop);
    RUN_TEST(tr, TestIndirectAssignment);
    RUN_TEST(tr, TestArrays);
    RUN_TEST(tr, TestMaps);
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
