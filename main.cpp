
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

namespace parse
{
    void RunOpenLexerTests(TestRunner& tr);
}  // namespace parse

namespace ast
{
    void RunUnitTests(TestRunner& tr);
}

namespace runtime
{
    void RunObjectHolderTests(TestRunner& tr);
    void RunObjectsTests(TestRunner& tr);
}  // namespace runtime

void TestParseProgram(TestRunner& tr);

class LexerInputExImpl : public parse::LexerInputEx
{ // Класс диспетчера исходных модулей, хранящихся в виде строковых переменных
public:
    struct ModuleDescType
    {
        int part_number;
        string part_body;
    };

    struct StackType
    {
        string part_name;
        int part_position;
        int module_string_number;
    };

    LexerInputExImpl() = default;
    LexerInputExImpl(initializer_list<pair<string, string>> part_list)
    {
        for (const auto& current_part_pair : part_list)
        {
            if (!include_map_.size())
                main_module_name_ = current_part_pair.first;
            include_map_[current_part_pair.first] = { ++last_part_number_, current_part_pair.second };
        }
    }

    ~LexerInputExImpl() = default;
    void AddIncludePart(const string& part_name, const string& part_body)
    {
        if (!include_map_.size())
            main_module_name_ = part_name;
        include_map_[part_name] = {++last_part_number_, part_body};
    }

    void SetCommandDescPtr(runtime::ProgramCommandDescriptor* command_desc_ptr) override
    {
        command_desc_ptr_ = command_desc_ptr;
    }

    void IncludeSwitchTo(std::string include_arg) override
    {
        if (!include_arg.size())
        { // Инициализирующий вызов IncludeSwitchTo()
            eof_bit_ = false;
            last_read_symb_ = std::char_traits<char>::eof();
            unget_symb_ = std::char_traits<char>::eof();
            current_position_ = 0;
            current_module_desc_ptr_ = nullptr;
            current_part_name_.clear();
            include_stack_.clear();
            include_arg = main_module_name_;
        }
        
        if (!include_map_.count(include_arg))
            throw ParseError("Включаемая часть "s + include_arg + " не найдена"s);
        if (current_part_name_.size())
            include_stack_.push_back({current_part_name_, current_position_, command_desc_ptr_->module_string_number});
        current_part_name_ = include_arg;
        current_module_desc_ptr_ = &include_map_[current_part_name_];
        current_position_ = 0;
        command_desc_ptr_->module_id = current_module_desc_ptr_->part_number;
        command_desc_ptr_->module_string_number = 0;
    }

    int get() override
    {
        if (!good())
        {
            last_read_symb_ = char_traits<char>::eof();
            return last_read_symb_;
        }

        if (unget_symb_ != char_traits<char>::eof())
        {
            last_read_symb_ = unget_symb_;
            unget_symb_ = char_traits<char>::eof();
            return last_read_symb_;
        }

        while (true)
        {
            if (current_position_ < static_cast<int>(current_module_desc_ptr_->part_body.size()))
            {
                last_read_symb_ = current_module_desc_ptr_->part_body[current_position_];
                ++current_position_;
                break;
            }
            else
            {
                if (include_stack_.size())
                {
                    StackType stack_rec = include_stack_.back();
                    include_stack_.pop_back();
                    current_part_name_ = stack_rec.part_name;
                    current_position_ = stack_rec.part_position;
                    current_module_desc_ptr_ = &include_map_[current_part_name_];
                    command_desc_ptr_->module_id = current_module_desc_ptr_->part_number;
                    command_desc_ptr_->module_string_number = stack_rec.module_string_number;
                }
                else
                {
                    last_read_symb_ = char_traits<char>::eof();
                    eof_bit_ = true;
                    break;
                }
            }
        }

        return last_read_symb_;
    }

    int peek() override
    {
        if (!good())
        {
            return char_traits<char>::eof();
        }

        if (unget_symb_ != char_traits<char>::eof())
        {
            return unget_symb_;
        }

        get();
        if (good())
        {
            --current_position_;
            return last_read_symb_;
        }
        else
        {
            return char_traits<char>::eof();
        }
    }

    LexerInputExImpl& unget() override
    {
        if (last_read_symb_ != char_traits<char>::eof())
        {
            unget_symb_ = last_read_symb_;
            eof_bit_ = false;
        }
        return *this;
    }

    bool good() override
    {
        return !eof_bit_;
    }

    operator bool() override
    {
        return good();
    }

    bool operator!() override
    {
        return !good();
    }

private:
    bool eof_bit_ = false;
    int last_read_symb_ = char_traits<char>::eof();
    int unget_symb_ = char_traits<char>::eof();
    int current_position_ = 0;
    ModuleDescType* current_module_desc_ptr_ = nullptr;
    string current_part_name_;
    unordered_map<string, ModuleDescType> include_map_;
    vector<StackType> include_stack_;
    runtime::ProgramCommandDescriptor* command_desc_ptr_ = nullptr;
    string main_module_name_;

    inline static int last_part_number_ = 0;
};

namespace
{
    void RunMythonProgram(istream& input, ostream& output, const runtime::LinkageFunction& link_function = {})
    {
        parse::TrivialParseContext parse_context(true);
        runtime::SimpleContext context(output, link_function);
        runtime::Closure closure;

        parse::Lexer lexer(input);
        auto program = ParseProgram(lexer, parse_context);
        program->Execute(closure, context);
    }

    void RunMythonProgramEx(parse::LexerInputEx& input, ostream& output, const runtime::LinkageFunction& link_function = {})
    {
        parse::TrivialParseContext parse_context(true);
        runtime::SimpleContext context(output, link_function);
        runtime::Closure closure;

        parse::Lexer lexer(input);
        auto program = ParseProgram(lexer, parse_context);
        program->Execute(closure, context);
    }

    void TestSimplePrints()
    {
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
    
    void TestAssignments()
    {
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
    
    void TestArithmetics()
    {
        istringstream input("print 1+2+3+4+5, 1*2*3*4*5, 1-2-3-4-5, 36/4/3, 2*5+10/2");
    
        ostringstream output;
        RunMythonProgram(input, output);    
        ASSERT_EQUAL(output.str(), "15 120 -13 3 15\n");
    }
    
    void TestVariablesArePointers()
    {
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
        parse::TrivialParseContext parse_context;
        auto program = ParseProgram(lexer, parse_context);
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

  def get_w_ref():
    return_ref self.w

  def get_h_ref():
    return_ref self.h

x_rect = Rect(10, 20)
print x_rect.w, x_rect.h # Эта команда выведет: 10 20
print x_rect.get_w(), x_rect.get_h() # Эта команда выведет: 10 20
print x_rect.get_w_ref(), x_rect.get_h_ref() # Эта команда также выведет: 10 20
x_rect.get_w_ref() = 100
x_rect.get_h_ref() = 200
print x_rect.w, x_rect.h # Эта команда выведет: 100 200
)--");

        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "10 20\n10 20\n10 20\n100 200\n"s);
    }
    
    void TestFloatPointEvaluation()
    {
        {
            istringstream input(R"--(
x = 3.1415925
print 2 * x, 2.5 * x
y = 4
print x * y
z = 6.2
print x * z + 6, z * 2 - x * 3 - 6
)--");

            ostringstream ostr;
            RunMythonProgram(input, ostr);
            ASSERT_EQUAL(ostr.str(), "6.28318 7.85398\n12.5664\n25.4779 -3.02478\n"s);
        }
    
        {
            istringstream input(R"--(
x = 3.1415925 / 2
m = math()
cos0 = m.cos(x)
print m.sin(x), cos0, m.round(cos0 + 2)
print m.sin(x / 2), m.cos(x / 4)
print m.atan(1), m.atan2(1, 1)
)--");

            ostringstream ostr;
            RunMythonProgram(input, ostr);
            ASSERT_EQUAL(ostr.str(), "1 7.67949e-08 2\n0.707107 0.92388\n0.785398 0.785398\n"s);
        }
    }
    
    void TestImportBinaryModule()
    {
        {
            istringstream input(R"--(
import "MythonTestPlugin"
tst = MythonTestPlugin()
print tst.print_hello()
print tst.add_all(5, 6, 7, 8)
print tst.add_all(5.5, 6.6, 7.7, 8.8)
zp = tst.find_zero(5, 6, 0, 9, 10)
zc = tst.find_char("ABCDabcd", "D")
print zp, zc, tst.ston("56"), tst.ston("5.6")
)--");

            ostringstream ostr;
            RunMythonProgram(input, ostr);
            ASSERT_EQUAL(ostr.str(), "HelloHello\n26\n28.6\n2 3 56 5.6\n"s);
        }

        {
            istringstream input(R"--(
import "MythonTestPlugin", "plug"
tst = plug_test()
s = tst.print_hello()
print tst.add_all(5, 6, 7.7, 8.8), s
zp = tst.find_zero(5, 6, 9, 5, 0, 9, 10)
zc = tst.find_char("ABCDabcd", "d")
print zp, zc
)--");

            ostringstream ostr;
            RunMythonProgram(input, ostr);
            ASSERT_EQUAL(ostr.str(), "Hello27.5 Hello\n4 7\n"s);
        }
    }

    void TestIncludes()
    {
        LexerInputExImpl input_ex;
        string main_program(R"--(
print "in main"
include "Include1"
print "already in main"
include "Include2"
print "again in main"
include "Include3"
print "finish in main"
)--");
        string include_1(R"--(
print "now in include_1"
)--");
        string include_2(R"--(
print "now in include_2"
include "Include21"
print "again in include_2"
include "Include22"
print "finish in include_2"
)--");
        string include_3(R"--(
print "now in include_3"
include "Include1"
print "again in include_3"
)--");
        string include_2_1(R"--(
print "now in include_2_1"
)--");
        string include_2_2(R"--(
print "now in include_2_2"
)--");

        input_ex.AddIncludePart("MainProgram", main_program);
        input_ex.AddIncludePart("Include1", include_1);
        input_ex.AddIncludePart("Include2", include_2);
        input_ex.AddIncludePart("Include3", include_3);
        input_ex.AddIncludePart("Include21", include_2_1);
        input_ex.AddIncludePart("Include22", include_2_2);
        ostringstream ostr;
        RunMythonProgramEx(input_ex, ostr);
        //cout << ostr.str() << endl;
        string proper_result = "in main\nnow in include_1\nalready in main\nnow in include_2\n"s;
        proper_result += "now in include_2_1\nagain in include_2\nnow in include_2_2\nfinish in include_2\n"s;
        proper_result += "again in main\nnow in include_3\nnow in include_1\nagain in include_3\nfinish in main\n"s;
        ASSERT_EQUAL(ostr.str(), proper_result);
    }

    void TestReturnRef()
    {
        // Сначала проверим работоспособность ссылок при разных способах их правильного применения
        istringstream input(R"--(
class WithArray:
  def __init__(w, h):
    self.test_arr = array(w, h)
    self.w = w
    self.h = h

  def get_arr_cell_ref(gw, gh):
    return_ref self.test_arr.get(gw, gh)

  def get_arr_cell(gw, gh):
    return self.test_arr.get(gw, gh)

  def get_arr_cell_ref_ind(gw, gh):
    return_ref self.get_arr_cell_ref(gw, gh)

  def get_w_ref():
    return_ref self.w

  def get_h_ref():
    return_ref self.h

  def get_w_():
    return self.w

  def get_h():
    return self.h

wa_object = WithArray(10, 20)
# Начальная инициализация используемых в тесте элементов массива wa_object.test_arr
wa_object.test_arr.get(3, 4) = -1
wa_object.test_arr.get(4, 3) = -1
wa_object.test_arr.get(3, 8) = -38
wa_object.test_arr.get(8, 3) = -83
# Простая переустановка некоторых ячеек массива с использованием ссылок на них
wa_object.get_arr_cell_ref(3, 4) = 34
wa_object.get_arr_cell_ref(4, 3) = 43
print wa_object.get_arr_cell(3, 4), wa_object.get_arr_cell_ref(4, 3) # Выводится 34 43
print wa_object.test_arr.get(3, 4) + wa_object.test_arr.get(4, 3) # Выводится 77
# Более сложная констукция косвенного присваивания с применением автоматического разыменования ссылок
wa_object.get_arr_cell_ref(3, 8) = -wa_object.get_arr_cell_ref(3, 8)
wa_object.get_arr_cell_ref(8, 3) = wa_object.get_arr_cell_ref(8, 3) * 2
print wa_object.get_arr_cell(3, 8), wa_object.get_arr_cell_ref(8, 3) # Здесь будет напечатано 38 -166
# Получим ссылку косвенно, добавив ещё один уровень вложенности
wa_object.get_arr_cell_ref_ind(3, 4) = -wa_object.get_arr_cell_ref_ind(3, 4) + 10
# Нижележащий оператор print должен вывести: -24 -24 -24
print wa_object.get_arr_cell(3, 4), wa_object.get_arr_cell_ref(3, 4), wa_object.get_arr_cell_ref_ind(3, 4)
# Ну и, наконец, проверка образования ссылок на простые скалярные поля
wa_object.get_w_ref() = 100
wa_object.get_h_ref() = 200
print wa_object.w, wa_object.h # Эта команда выведет: 100 200
# Ещё одна проба разыменования ссылок в правой части косвенного присваивания
wa_object.get_w_ref() = 2 * wa_object.get_h_ref()
print wa_object.w, wa_object.h # Эта команда выведет: 400 200
)--");

        ostringstream ostr;
        RunMythonProgram(input, ostr);
        ASSERT_EQUAL(ostr.str(), "34 43\n77\n38 -166\n-24 -24 -24\n100 200\n400 200\n"s);

        // Далее испытаем швырки исключений при ошибках формирования ссылок
        // (запрещённые ссылки на локальные переменные и временные значения).
        istringstream input2(R"--(
class ArrayWithInvalidRefs:
  def __init__(w, h):
    self.test_arr = array(w, h)

  def get_arr_cell_invalid_ref(gw, gh):
    # Аргументом расположенного ниже оператора return_ref является временное значение,
    # что недопустимо. Тут должна возникнуть ошибка при синтаксическом разборе
    return_ref self.test_arr.get(gw, gh) + 2

wa_object = ArrayWithInvalidRefs(10, 20)
r = wa_object.get_arr_cell_invalid_ref(3, 4)
)--");

        ASSERT_THROWS(RunMythonProgram(input2, ostr), ParseError);

        istringstream input3(R"--(
class ClassWithInvalidRefs:
  def __init__(w, h):
    self.w = w
    self.h = h

  def get_invalid_ref(gw, gh):
    # Аргументом расположенного ниже оператора return_ref является локальная переменная
    # метода, что недопустимо. Тут должна возникнуть ошибка при синтаксическом разборе
    local_var = 5
    return_ref local_var

wa_object = ClassWithInvalidRefs(10, 20)
r = wa_object.get_invalid_ref(3, 4)
)--");

        ASSERT_THROWS(RunMythonProgram(input3, ostr), ParseError);
    }


    void TestBitwiseOps()
    {
        { // Проверка побитовых операций над целыми числами
            istringstream istr(R"--(
x = 5
print x, ~x, 2 * ~x, ~(2 * x)
y = 3.1415925
print y, ~y, 2 * ~y, ~(2 * y)
z1 = 235
z2 = 12345
print z1 & z2, z1 | z2, z1 ^ z2, z2 & z1, z2 | z1, z2 ^ z1, z2 ^ z2
# В Муфлоне система приоритета побитовых операций отличается от C++
# Расположенное ниже выражение эквивалентно следующему на C/C++:
# int z3 = ((45 + 89 & 35 * 98 + 32) | (123 - 101)) & (101 ^ 123);
z3 = 45 + 89 & 35 * 98 + 32 | (123 - 101) & (101 ^ 123)
print z3
# Для получения подобия такого выражения на C/C++
# int z4 = 45 + 89 & 35 * 98 + 32 | (123 - 101) & (101 ^ 123);
# средствами Муфлона следует расставить скобки:
z4 = (45 + 89 & 35 * 98 + 32) | ((123 - 101) & (101 ^ 123))
print z4
)--");
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            string proper_result = "5 -6 -12 -11\n3.14159 -1.4292 -2.85841 -0.714602\n";
            proper_result += "41 12539 12498 41 12539 12498 0\n22\n150\n";
            ASSERT_EQUAL(ostr.str(), proper_result);
        }

        {  // Теперь испытаем побитовые операции над строками
            istringstream istr(R"--(
s1 = "\x01\x10\x23\x32\x00\xff"
print ~s1
s2 = "\xFF\xFE\x73\x37\x02\x88"
print s1 & s2
print s1 | s2
print s1 ^ s2
)--");
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            string proper_result = "\xfe\xef\xdc\xcd\xff\0\n\x01\x10\x23\x32\0\x88\n"s;
            proper_result += "\xff\xfe\x73\x37\x02\xff\n"s;
            proper_result += "\xfe\xee\x50\x05\x02\x77\n"s;
            ASSERT_EQUAL(ostr.str(), proper_result);
        }
    }

    void TestShiftOps()
    {
        { // Проверка сдвигов целых чисел
            istringstream istr(R"--(
x = 254
print x << 1, x << 2, x >> 1, x >> 2
y = 4
print x << y, x << y + 1, x >> y, x >> y - 1
)--");
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "508 1016 127 63\n4064 8128 15 31\n");
        }
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
        RUN_TEST(tr, TestReturnRef);
        RUN_TEST(tr, TestArrays);
        RUN_TEST(tr, TestMaps);
        RUN_TEST(tr, TestFloatPointEvaluation);
        RUN_TEST(tr, TestImportBinaryModule);
        RUN_TEST(tr, TestIncludes);
        RUN_TEST(tr, TestBitwiseOps);
        RUN_TEST(tr, TestShiftOps);
    }
}  // namespace

int main()
{
    try
    {
        TestAll();
        RunMythonProgram(cin, cout);
        string white_line; 
        getline(cin, white_line);
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
		return 1;
    }

    return 0;
}
