#include "lexer.h"
#include "parse.h"
#include "statement.h"

#include "test_runner_p.h"

using namespace std;

namespace parse
{
    CplxParsedProgram ParseProgramFromString(const string& program)
    {
        istringstream is(program);
        CplxParsedProgram cplx_program;
        cplx_program.SetLexer(parse::Lexer(is));
        ParseProgram(cplx_program);

        return cplx_program;
    }

    void TestSimpleProgram()
    {
        const string program = R"(
x = 4
y = 5
z = "hello, "
n = "world"
print x + y, z + n
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);

        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(), "9 hello, world\n"s);
    }

    void TestProgramWithClasses()
    {
        const string program = R"(
program_name = "Classes test"

class Empty:
  def __init__():
    x = 0

class Point:
  def __init__(x, y):
    self.x = x
    self.y = y

  def SetX(value):
    self.x = value
  def SetY(value):
    self.y = value

  def __str__():
    return '(' + str(self.x) + '; ' + str(self.y) + ')'

origin = Empty()
origin = Point(0, 0)

far_far_away = Point(10000, 50000)

print program_name, origin, far_far_away, origin.SetX(1)
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);

        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(), "Classes test (0; 0) (10000; 50000) None\n"s);
    }

    void TestProgramWithIf()
    {
        const string program = R"(
x = 4
y = 5
if x > y:
  print "x > y"
else:
  print "x <= y"
if x > 0:
  if y < 0:
    print "y < 0"
  else:
    print "y >= 0"
else:
  print 'x <= 0'
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);

        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(), "x <= y\ny >= 0\n"s);
    }

    void TestReturnFromIf()
    {
        const string program = R"(
class Abs:
  def calc(n):
    if n > 0:
      return n
    else:
      return -n

x = Abs()
print x.calc(2)
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);
        
        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(), "2\n"s);
    }

    // Проверка работы многовариантного оператора if ... elif ... else ... .
    void TestProgramWithComplexIf()
    {
        const string program = R"(
class ValClassify:
  def calc(n):
    if n > 1000:
      return 4
    elif n > 100:
      return 3
    elif n > 10:
      return 2
    elif n > 0:
      return 1
    elif n == 0:
      return 0
    elif n >= -10:
      return -1
    elif n >= -100:
      return -2
    elif n >= -1000:
      return -3
    else:
      return -4

x = ValClassify()
print x.calc(0), x.calc(1), x.calc(11), x.calc(111), x.calc(1111), x.calc(500), x.calc(50), x.calc(5), x.calc(0)
print x.calc(0), x.calc(-1), x.calc(-11), x.calc(-111), x.calc(-1111), x.calc(-500), x.calc(-50), x.calc(-5), x.calc(0)
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);

        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(), "0 1 2 3 4 3 2 1 0\n0 -1 -2 -3 -4 -3 -2 -1 0\n"s);
    }

    void TestRecursion()
    { // Рекурсивный вызов метода с накоплением результата в поле класса.
        const string program = R"(
class ArithmeticProgression:
  def calc(n):
    self.result = 0
    self.calc_impl(n)

  def calc_impl(n):
    value = n
    if value > 0:
      self.result = self.result + value
      self.calc_impl(value - 1)

x = ArithmeticProgression()
x.calc(10)
print x.result
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);

        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(), "55\n"s);
    }

    void TestRecursion2()
    { // Классическая рекурсия с возвращением результата через возвращаемое значение рекурсивного метода.
        const string program = R"(
class GCD:
  def __init__():
    self.call_count = 0

  def calc(a, b):
    self.call_count = self.call_count + 1
    if a < b:
      return self.calc(b, a)
    if b == 0:
      return a
    return self.calc(a - b, b)

x = GCD()
print x.calc(510510, 18629977)
print x.calc(22, 17)
print x.call_count
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);

        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(), "17\n1\n115\n"s);
    }

    void TestComplexLogicalExpression()
    { // Вычисление сложных логических выражений.
        const string program = R"(
a = 1
b = 2
c = 3
ok = a + b > c and a + c > b and b + c > a
print ok
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);

        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(), "False\n"s);
    }

    void TestClassicalPolymorphism()
    {
        const string program = R"(
class Shape:
  def __str__():
    return "Shape"

class Rect(Shape):
  def __init__(w, h):
    self.w = w
    self.h = h

  def __str__():
    return "Rect(" + str(self.w) + 'x' + str(self.h) + ')'

class Circle(Shape):
  def __init__(r):
    self.r = r

  def __str__():
    return 'Circle(' + str(self.r) + ')'

class Triangle(Shape):
  def __init__(a, b, c):
    self.ok = a + b > c and a + c > b and b + c > a
    if (self.ok):
      self.a = a
      self.b = b
      self.c = c

  def __str__():
    if self.ok:
      return 'Triangle(' + str(self.a) + ', ' + str(self.b) + ', ' + str(self.c) + ')'
    else:
      return 'Wrong triangle'

r = Rect(10, 20)
c = Circle(52)
t1 = Triangle(3, 4, 5)
t2 = Triangle(125, 1, 2)

print r, c, t1, t2
)"s;
        CplxParsedProgram cplx_program = ParseProgramFromString(program);
        cplx_program.SetContext(runtime::DummyContext());
        ExecuteProgram(cplx_program);

        runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
        ASSERT_EQUAL(context->output.str(),
                     "Rect(10x20) Circle(52) Triangle(3, 4, 5) Wrong triangle\n"s);
    }

    void TestAnchestorCalls()
    {
        { // Проверка вызовов "скрытых" (переопределённых) методов унаследованных классов.
            const string program = R"--(
class Shape:
  def __init__(shape_name):
    self.shape_name = shape_name
    self.width = 0
    self.height = 0
    print "Shape_Init - Shape_Name =", self.shape_name

class Rect(Shape):
  def __init__(shape_name, width, height):
    self.Shape.__init__(shape_name)
    self.width = width
    self.height = height
    print "Rect_Init_3"

  def __init__(width, height):
    self.__init__("Rect", width, height)
    print "Rect_Init_2"

class Square(Rect):
  def __init__(shape_name, size):
    self.Rect.__init__(shape_name, size, size)
    print "Square_Init_2"

  def __init__(size):
    self.Rect.__init__("Square", size, size)
    print "Square_Init_1"

  def __init__():
    self.Shape.__init__("Square")
    print "Square_Init_0"

shape_instance = Shape("Unknown_Shape")
rect_instance = Rect(10, 20)
square_instance_0 = Square()
square_instance_1 = Square(50)
square_instance_0 = Square("Super_Square", 100)
)--"s;
            CplxParsedProgram cplx_program = ParseProgramFromString(program);
            cplx_program.SetContext(runtime::DummyContext());
            ExecuteProgram(cplx_program);

            runtime::DummyContext* context = dynamic_cast<runtime::DummyContext*>(cplx_program.context.get());
            std::string etalon_string =
                // Исполняется shape_instance = Shape("Unknown_Shape") - происходит прямой вызов единственного конструктора
                // Shape.__init__(shape_name) класса Shape.
                "Shape_Init - Shape_Name = Unknown_Shape\n"s +      // Печать из Shape.__init__(shape_name).
                // Исполняется rect_instance = Rect(10, 20) - сначала вызывается конструктор Rect.__init__(width, height),
                // оттуда происходит вызов другого конструктора того же класса Rect.__init__(shape_name, width, height),
                // который, в свою очередь, обращается к вложенному методу Shape.__init__(shape_name).
                // Порядок вывода строк: Shape.__init__(shape_name), выводящий в контекст первую строку, затем печать второй
                // строки из тела Rect.__init__(shape_name, width, height), и, наконец, третья строка, выводимая из Rect.__init__(width, height).
                "Shape_Init - Shape_Name = Rect\n"s +               // Печать из Shape.__init__(shape_name).
                "Rect_Init_3\n"s +                                  // Печать из Rect.__init__(shape_name, width, height).
                "Rect_Init_2\n"s +                                  // Печать из Rect.__init__(width, height).
                // Исполняется square_instance_0 = Square() - вызов конструктора Square.__init__(), при котором сначала производится вызов
                // Shape.__init__(shape_name), а затем печать строки из тела самого Square.__init__().
                "Shape_Init - Shape_Name = Square\n" +              // Печать из Shape.__init__(shape_name).
                "Square_Init_0\n" +                                 // Печать из Square.__init__().
                // Исполняется square_instance_1 = Square(50) - вызов конструктора Square.__init__(size), оттуда запрашивается Rect.__init__(shape_name, width, height),
                // а из него затем уже метод Shape.__init__(shape_name).
                "Shape_Init - Shape_Name = Square\n" +              // Печать из Shape.__init__(shape_name).
                "Rect_Init_3\n" +                                   // Печать из Rect.__init__(shape_name, width, height).
                "Square_Init_1\n" +                                 // Печать из Square.__init__(size).
                // Исполняется square_instance_0 = Square("Super_Square", 100) - исполнение конструктора Square.__init__(shape_name, size), откуда первой очередью
                // вызвавается Rect.__init__(shape_name, width, height), а из него, в свою очередь, Shape.__init__(shape_name).:
                "Shape_Init - Shape_Name = Super_Square\n" +        // Печать из Shape.__init__(shape_name).
                "Rect_Init_3\n" +                                   // Печать из Rect.__init__(shape_name, width, height).
                "Square_Init_2\n";                                  // Печать из Square.__init__(shape_name, size)
            ASSERT_EQUAL(context->output.str(), etalon_string);
        }
    }
}  // namespace parse

void TestParseProgram(TestRunner& tr)
{
    cout << endl << "Тесты построения абстрактного синтаксического дерева программы"s << endl;

    RUN_TEST(tr, parse::TestSimpleProgram);
    RUN_TEST(tr, parse::TestProgramWithClasses);
    RUN_TEST(tr, parse::TestProgramWithIf);
    RUN_TEST(tr, parse::TestProgramWithComplexIf);
    RUN_TEST(tr, parse::TestReturnFromIf);
    RUN_TEST(tr, parse::TestRecursion);
    RUN_TEST(tr, parse::TestRecursion2);
    RUN_TEST(tr, parse::TestComplexLogicalExpression);
    RUN_TEST(tr, parse::TestClassicalPolymorphism);
    RUN_TEST(tr, parse::TestAnchestorCalls);
}
