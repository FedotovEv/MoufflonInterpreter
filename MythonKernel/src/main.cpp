
#include "lexer.h"
#include "parse.h"
#include "runtime.h"
#include "statement.h"
#include "test_runner_p.h"
#include "error_classes.h"
#include "debug_context.h"

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
} // namespace ast

namespace runtime
{
    struct DebugEventDesc
    {
        DebugCallbackReason event_reason;
        CommandGenus genus;
        ProgramCommandDescriptor command;
    };

    void RunObjectHolderTests(TestRunner& tr);
    void RunObjectsTests(TestRunner& tr);
}  // namespace runtime

void TestParseProgram(TestRunner& tr);

string ConvertLinkageToString(const runtime::LinkageValue& link_val)
{
    if (holds_alternative<string>(link_val))
    {
        return get<string>(link_val);
    }
    else if (holds_alternative<bool>(link_val))
    {
        if (get<bool>(link_val))
            return "True";
        else
            return "False";
    }
    else if (holds_alternative<int>(link_val))
    {
        return to_string(get<int>(link_val));
    }
    else if (holds_alternative<double>(link_val))
    {
        return to_string(get<double>(link_val));
    }
    else
    {
        return {};
    }
}

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
        parse::TrivialParseContext parse_context;
        runtime::SimpleContext context(output, link_function);
        runtime::Closure closure;

        parse::Lexer lexer(input);
        auto program = ParseProgram(lexer, parse_context);
        program->Execute(closure, context);
    }

    void RunMythonProgramEx(parse::LexerInputEx& input, ostream& output, const runtime::LinkageFunction& link_function = {})
    {
        parse::TrivialParseContext parse_context;
        runtime::SimpleContext context(output, link_function);
        runtime::Closure closure;

        parse::Lexer lexer(input);
        auto program = ParseProgram(lexer, parse_context);
        program->Execute(closure, context);
    }

    /**
    * \brief    Функция запуска Муфлон-программы под отладчиком.
    * \param    input - исходный текст Муфлон-программы.
    * \param    what_return - режимы исполнения программы, которые будут возвращаться из звонковой функции.
    * \param    breaks - список желаемых точек останова, которые будут установлены в отладочный контекст после его создания.
    * \param    link_function - функция внешней связи исполняемой программы. Задаётся только при необходимости.
    * \return   Первый член возвращаемого кортежа - строка отладочного потока, второй член - суммарная строка выходного потока,
                третий член - коллекция событий отладчика.
    */
    using DebugExecutionModeV = std::vector<runtime::DebugExecutionMode>;
    // Возможные варианты способа задания точки останова. Первый - прямая позиция в коде, второй - полное описание бряка,
    // третий - параметры свободной функции, четвёртый - характеристики метода.
    struct FreeFunctionCharm
    {
        std::string name;
        size_t arg_count = 0;
    };

    struct MethodCharm
    {
        std::string name;
        std::string class_name;
        size_t arg_count = 0;
    };

    using OneBreakpointDef = std::variant<runtime::ProgramCommandDescriptor, runtime::BreakpointDesc, FreeFunctionCharm, MethodCharm>;
    using BreakpointsList = std::vector<OneBreakpointDef>;

    std::tuple<std::string, std::string, std::vector<runtime::DebugEventDesc>> DebugMythonProgram
        (istream& input, std::variant<runtime::DebugExecutionMode, DebugExecutionModeV> what_return, const BreakpointsList& breaks = {},
         const runtime::DebugCallback& cond_break_req = {}, const runtime::LinkageFunction& link_function = {})
    {
        using namespace runtime;
        auto debug_event_out = [](std::ostream& ostr, DebugCallbackReason call_reason, Executable* exec_op, Closure& closure, Context& context)
            {
                static constexpr size_t MAX_TYPIID_LENGTH = 30;

                std::string typid_name = typeid(*exec_op).name();
                if (typid_name.size() > MAX_TYPIID_LENGTH)
                    typid_name = typid_name.substr(0, MAX_TYPIID_LENGTH);
                else
                    typid_name += std::string(MAX_TYPIID_LENGTH - typid_name.size(), ' ');

                ostr << "Reason : " << call_reason << " Genus : " << exec_op->GetCommandGenus() << " Type : "
                     << typid_name << ' ' << exec_op->GetCommandDesc();
            };

        ostringstream debug_ostr;
        std::vector<DebugEventDesc> debug_event_collector;
        auto debug_event_handler = [&](DebugCallbackReason call_reason, Executable* exec_op, Closure& closure, Context& context) -> DebugExecutionMode
            {
                static size_t what_return_index = 0;

                debug_event_out(debug_ostr, call_reason, exec_op, closure, context);
                DebugEventDesc rec_event{.event_reason = call_reason, .genus = exec_op->GetCommandGenus(), .command = exec_op->GetCommandDesc()};
                debug_ostr << std::endl;
                debug_event_collector.push_back(rec_event);

                if (call_reason == DebugCallbackReason::DEBUG_CALLBACK_CHECK_CONDITION && cond_break_req)
                    // Запрошена проверка условия, присоединённого к точке останова, и для таких запросов назначен обработчик. Вызовем его.
                    return cond_break_req(call_reason, exec_op, closure, context);

                if (std::holds_alternative<DebugExecutionMode>(what_return))
                { // Требуется отвечать на отладочные звонки одним и тем же значением.
                    return std::get<DebugExecutionMode>(what_return);
                }
                else if (std::holds_alternative<DebugExecutionModeV>(what_return))
                { // Ответы на отладочные звонки перечислены в массиве.
                    const DebugExecutionModeV& what_return_vec = std::get<DebugExecutionModeV>(what_return);
                    if (call_reason == DebugCallbackReason::DEBUG_CALLBACK_INIT)
                        what_return_index = 0;
                    if (what_return_vec.empty())
                        return DebugExecutionMode::DEBUG_NO_DEBUG;

                    if (what_return_index < what_return_vec.size())
                        return what_return_vec[what_return_index++];
                    else
                        return what_return_vec.back();
                }
                else
                {
                    return DebugExecutionMode::DEBUG_NO_DEBUG;
                }
            };

        parse::TrivialParseContext parse_context;
        runtime::Closure closure;

        parse::Lexer lexer(input);
        auto program = ParseProgram(lexer, parse_context);

        ostringstream out_ostr;
        DebugContext debug_context(out_ostr, debug_event_handler, link_function);
        if (!breaks.empty())
        {
            for (const OneBreakpointDef& one_break : breaks)
            { // Установка единичного бряка различных допустимых типов.
                if (std::holds_alternative<runtime::ProgramCommandDescriptor>(one_break))
                {
                    debug_context.AddBreakpoint(std::get<runtime::ProgramCommandDescriptor>(one_break));
                }
                else if (std::holds_alternative<runtime::BreakpointDesc>(one_break))
                {
                    debug_context.AddBreakpoint(std::get<runtime::BreakpointDesc>(one_break));
                }
                else if (std::holds_alternative<FreeFunctionCharm>(one_break))
                {
                    const FreeFunctionCharm& break_charm = std::get<FreeFunctionCharm>(one_break);
                    debug_context.AddBreakAtFreeFunction(break_charm.name, break_charm.arg_count);
                }
                else if (std::holds_alternative<MethodCharm>(one_break))
                {
                    const MethodCharm& break_charm = std::get<MethodCharm>(one_break);
                    debug_context.AddBreakAtMethod(break_charm.name, break_charm.arg_count, break_charm.class_name);
                }
            }
        }

        if (std::holds_alternative<DebugExecutionMode>(what_return))
        {
            debug_context.SetDebugMode(std::get<DebugExecutionMode>(what_return));
        }
        else if (std::holds_alternative<DebugExecutionModeV>(what_return))
        {
            const DebugExecutionModeV& what_return_vec = std::get<DebugExecutionModeV>(what_return);
            if (!what_return_vec.empty())
                debug_context.SetDebugMode(what_return_vec.front());
            else
                debug_context.SetDebugMode(DebugExecutionMode::DEBUG_NO_DEBUG);
        }
        else
        {
            debug_context.SetDebugMode(DebugExecutionMode::DEBUG_NO_DEBUG);
        }
        program->Execute(closure, debug_context);

        return {debug_ostr.str(), out_ostr.str(), move(debug_event_collector)};
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
                                   const vector<runtime::LinkageValue>& argument_value) -> runtime::LinkageValue
                            {
                                switch (what_reason)
                                {
                                    case runtime::LinkCallReason::CALL_REASON_WRITE_FIELD:
                                        ostr << field_name << " = "
                                             << ConvertLinkageToString(argument_value[0]) << endl;
                                        return {};
                                    case runtime::LinkCallReason::CALL_REASON_READ_FIELD:
                                        ostr << "Reading " << field_name << endl;
                                        return "empty"s;
                                    case runtime::LinkCallReason::CALL_REASON_CALL_METHOD:
                                        ostr << "Calling " << field_name << endl;
                                        return "executed"s;
                                    default:
                                        return {};
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
while not map_var.is_cursor_end(map_iter):
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
    
    void TestStringOperations()
    { // Функция опробования функций класса операций над строками string_ops.
        { // Сцепление и поиск.
            istringstream input(R"--(
str_ops = string_ops()
str_var_1 = "ABCDEFGH"
str_var_2 = "IJKLMNOPQ"
str_var_3 = "RSTUVWXYZ"
alphabet_var = str_ops.concat(str_var_1, str_var_2, str_var_3)
print alphabet_var, str_ops.size(alphabet_var) # Вывод в консоль полного латинского алфавита, а также его длины в 26 символов.
print str_ops.size(str_var_1), str_ops.size(str_var_2), str_ops.size(str_var_3) # Вывод длин исходных компонент 8, 9 и 9 символов.
# Прямой поиск
alphabet_var = alphabet_var + str_ops.reverse(alphabet_var)
# Следующие поисковые операции выведут в консоль следующие найденные положения - 0, 8, 18 и 10.
print str_ops.find(alphabet_var, "A"), str_ops.find(alphabet_var, "I"), str_ops.find(alphabet_var, "S"), str_ops.find(alphabet_var, "KLM")
# Обратный поиск - должны быть найдены концевые буквы из второй половины сцепки alphabet_var.
# Правильные результаты поиска - 51, 43, 33, 39.
print str_ops.rfind(alphabet_var, "A"), str_ops.rfind(alphabet_var, "I"), str_ops.rfind(alphabet_var, "S"), str_ops.find(alphabet_var, "MLK")
print str_ops.find(alphabet_var, "0") == str_ops.not_found() # Сравнение возвращает True, так как "0" не содержится в alphabet_var.
)--");
            ostringstream ostr;
            RunMythonProgram(input, ostr);
            // cout << ostr.str() << endl;
            ASSERT_EQUAL(ostr.str(), "ABCDEFGHIJKLMNOPQRSTUVWXYZ 26\n8 9 9\n0 8 18 10\n51 43 33 39\nTrue\n"s);
        }
        { // Подстроки.
            istringstream input(R"--(
str_ops = string_ops()
str_var = "SubString_1:SubString_2:SubString_3"
#
sub_1_start = str_ops.find(str_var, "Sub")
sub_1_end = str_ops.find(str_var, ":")
#
sub_2_start = str_ops.find(str_var, "Sub", sub_1_end)
sub_2_end = str_ops.find(str_var, ":", sub_2_start)
#
sub_3_start = str_ops.find(str_var, "Sub", sub_2_end)
sub_3_end = str_ops.find(str_var, ":", sub_3_start)
#
sub_1_str = str_ops.substr(str_var, sub_1_start, sub_1_end - sub_1_start)
sub_2_str = str_ops.substr(str_var, sub_2_start, sub_2_end - sub_2_start)
sub_3_str = str_ops.substr(str_var, sub_3_start)
#
print sub_1_start, sub_1_end, sub_1_str # Выделена подстрока "SubString_1" с позиции 0 до позиции 11.
print sub_2_start, sub_2_end, sub_2_str # Выделена подстрока "SubString_2" с позиции 12 до позиции 23.
print sub_3_start, sub_3_end, sub_3_str # Выделена подстрока "SubString_3" с позиции 24 до конца строки.
)--");
            ostringstream ostr;
            RunMythonProgram(input, ostr);
            // cout << ostr.str() << endl;
            ASSERT_EQUAL(ostr.str(), "0 11 SubString_1\n12 23 SubString_2\n24 -1 SubString_3\n"s);
        }
        { // Преобразование чисел в строки и обратно.
            istringstream input(R"--(
str_ops = string_ops()
# Простые примеры преобразований в числа.
ival_1 = str_ops.to_number("123")
dval_2 = str_ops.to_number("123.321")
# Преобразование в числа из подстрок.
ival_3 = str_ops.to_number("sss123 sss", 3) # результат - "123", задействовано 3 символа.
len_3 = str_ops.to_number_length()
dval_4 = str_ops.to_number("jklu123.321kkk", 4) # результат - "123.321", задействовано 7 символов.
len_4 = str_ops.to_number_length()
# Вывод в консоль результатов всех расположенных выше конверсий.
print ival_1, dval_2, ival_3, dval_4
print len_3, len_4 # Использовано 3 и 7 символов соответственно.

# Разные базы систем счисления для целочисленных значений.
ival_base_2 = str_ops.to_number("10110011", 0, 2) # = 179
ival_base_8 = str_ops.to_number("1237654", 0, 8) # = 343980
ival_base_10 = str_ops.to_number("123987", 0, 10) # = 123987
ival_base_16 = str_ops.to_number("123AB8EF", 0, 16) # = 305838319
ival_base_32 = str_ops.to_number("12IJK", 0, 32) # = 1133172
print ival_base_2, ival_base_8, ival_base_10, ival_base_16, ival_base_32

# Далее опробуем обратное превращение - строк в числа.
# Целое число в разных системах счисления.
test_int_val = 123456
str_int_0 = str_ops.to_string(test_int_val) # = "123456"
str_int_2 = str_ops.to_string(test_int_val, 2) # = "11110001001000000"
str_int_8 = str_ops.to_string(test_int_val, 8) # = "361100"
str_int_10 = str_ops.to_string(test_int_val, 10) # = "123456"
str_int_16 = str_ops.to_string(test_int_val, 16) # = "1e240"
str_int_32 = str_ops.to_string(test_int_val, 32) # = "3oi0"
# Вывод результатов целочисленных превращений.
print str_int_0, str_int_2, str_int_8, str_int_10, str_int_16, str_int_32

# Различные форматы представления дробных чисел.
#------------
CHARS_FORMAT_FIXED_MASK = 1
CHARS_FORMAT_SCIENTIFIC_MASK = 2
CHARS_FORMAT_HEX_MASK = 4
CHARS_FORMAT_GEN_MASK = CHARS_FORMAT_FIXED_MASK |  CHARS_FORMAT_SCIENTIFIC_MASK
#------------
test_double_val = 123456.87654321
str_double_typ = str_ops.to_string(test_double_val) # = "123456.87654321"
str_double_gen = str_ops.to_string(test_double_val, CHARS_FORMAT_GEN_MASK) # = "123456.87654321"
str_double_fix = str_ops.to_string(test_double_val, CHARS_FORMAT_FIXED_MASK) # = "123456.87654321"
str_double_sci = str_ops.to_string(test_double_val, CHARS_FORMAT_SCIENTIFIC_MASK) # = "1.2345687654321e+05"
str_double_hex = str_ops.to_string(test_double_val, CHARS_FORMAT_HEX_MASK) # = "1.e240e06522c48p+16"
str_double_fix_2 = str_ops.to_string(test_double_val, CHARS_FORMAT_FIXED_MASK, 2) # = "123456.88"
str_double_fix_3 = str_ops.to_string(test_double_val, CHARS_FORMAT_FIXED_MASK, 3) # = "123456.877"
str_double_sci_4 = str_ops.to_string(test_double_val, CHARS_FORMAT_SCIENTIFIC_MASK, 4) # = "1.2346e+05"
# Вывод результатов дробных превращений.
print str_double_typ, str_double_gen, str_double_fix, str_double_sci, str_double_hex
print str_double_fix_2, str_double_fix_3, str_double_sci_4
)--");
            ostringstream ostr;
            RunMythonProgram(input, ostr);
            // cout << ostr.str() << endl;
            ASSERT_EQUAL(ostr.str(),
                "123 123.321 123 123.321\n"s + 
                "3 7\n"s +
                "179 343980 123987 305838319 1133172\n"s +
                "123456 11110001001000000 361100 123456 1e240 3oi0\n"s +
                "123456.87654321 123456.87654321 123456.87654321 1.2345687654321e+05 1.e240e06522c48p+16\n"s +
                "123456.88 123456.877 1.2346e+05\n"s);
        }
        { // Различные изменяющие операции над строками.
            istringstream input(R"--(
str_ops = string_ops()
arg_str = "ABCDEFGHIJ1234567890"
arg_str_rev = str_ops.reverse(arg_str) # Обращение строки-аргумента arg_str. Результат - "0987654321JIHGFEDCBA".

# Вставка строки в строку arg_str перед символом "1". Результат - "ABCDEFGHIJZZZ1234567890".
arg_str_ins = str_ops.insert(arg_str, str_ops.find(arg_str, "1"), "ZZZ")

# Удаление 2 символов из строки arg_str_ins, начиная с положения первого символа "Z. Результат = "ABCDEFGHIJZ1234567890".
arg_str_erase = str_ops.erase(arg_str_ins, str_ops.find(arg_str_ins, "Z"), 2)

# Замена 4 символов строки arg_str, начиная с положения символа "1", на 3 символа строки "ZXCVBNM", взятых с позиции 2 в ней.
# Результат - "ABCDEFGHIJCVB567890".
arg_str_repl = str_ops.replace(arg_str, str_ops.find(arg_str, "1"), 4, "ZXCVBNM", 2, 3)

# Конструирование строки из 3 копий строки "AFZ". Результат - "AFZAFZAFZ".
res_str_replicate = str_ops.replicate("AFZ", 3)

# Вывод в консоль результатов операций.
print arg_str_rev
print arg_str_ins
print arg_str_erase
print arg_str_repl
print res_str_replicate
)--");
            ostringstream ostr;
            RunMythonProgram(input, ostr);
            // cout << ostr.str() << endl;
            ASSERT_EQUAL(ostr.str(),
                "0987654321JIHGFEDCBA\n"s +
                "ABCDEFGHIJZZZ1234567890\n"s +
                "ABCDEFGHIJZ1234567890\n" +
                "ABCDEFGHIJCVB567890\n"s +
                "AFZAFZAFZ\n"s);
        }
    }

    void TestFreeFunction()
    { // Проверка нормального функционирования механизма свободных функций.
        { // Обыкновенная свободная функция.
            istringstream input(R"--(
def free_func_add(arg_a, arg_b):
  return arg_a + arg_b

def free_func_sub(arg_a, arg_b):
  return arg_a - arg_b

def free_func_sqr(arg_a):
  return arg_a * arg_a

add_result = free_func_add(2, 3)
sub_result = free_func_sub(3, 2)
sqr_result_2 = free_func_sqr(2)
sqr_result_3 = free_func_sqr(3)
# Протокол результатов.
print add_result, sub_result, sqr_result_2, sqr_result_3
)--");
            ostringstream ostr;
            RunMythonProgram(input, ostr);
            ASSERT_EQUAL(ostr.str(), "5 1 4 9\n"s);
        }
        
        { // Свободная функция как сопрограмма.
            istringstream input(R"--(
def free_func_coro(arg_a):
  summator = arg_a
  while (True):
    co_yield summator
    summator = summator + arg_a

summ_coro = free_func_coro(2)
# Несколько возобновлений приостановленной функции-сопрограммы.
summ_result_1 = summ_coro.resume() # получаем 2 (исходный аргумент сопрограммы).
summ_result_2 = summ_coro.resume() # получаем 4
summ_result_3 = summ_coro.resume() # получаем 6
summ_result_4 = summ_coro.resume() # получаем 8
# Протокол результатов.
print summ_result_1, summ_result_2, summ_result_3, summ_result_4
)--");
            ostringstream ostr;
            RunMythonProgram(input, ostr);
            ASSERT_EQUAL(ostr.str(), "2 4 6 8\n"s);
        }
    }

    void TestImportBinaryModule()
    {
        {
            istringstream input(R"--(
import "MythonTestPlugin"
# Если префикс класса втыкалы не задан явно (вторым параметром директивы import), то таким префиксом будет являться
# имя библиотеки (в нашем случае MythonTestPlugin).
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
# Имя единственного класса втыкалы, которая содержится в библиотеке MythonTestPlugin - TestPlugin.
# Поэтому после такой директивы import доступен он будет либо как plug_TestPlugin, либо как просто plug.
tst = plug_TestPlugin()
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

    void TestIsSameTarget()
    {
        { // Проверка верности работы функции is_same_target() (или, что то же, IsSameTarget()).
            istringstream istr(R"--(
x = 254
y = x
print is_same_target(x, y)
y = y + 1
print is_same_target(x, y)
z = "ABC"
z2 = z
print is_same_target(z, z2)
z2 = x
print IsSameTarget(z2, x)
print is_same_target(x, z2)
z2 = None
print IsSameTarget(x, z2)
)--");
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "True\nFalse\nTrue\nTrue\nTrue\nFalse\n");
        }
    }

    void TestMethodsOverload()
    {
        { // Проверка обработки перегруженных методов класса.
            istringstream istr(R"--(
class WithOverloadedMethod:
  def overloaded_method(gw):
    print "1"
    return gw

  def overloaded_method(gw, gh):
    print "2"
    return gw + gh

  def overloaded_method(gw, gh, gz):
    print "3"
    return gw + gh + gz

over_class = WithOverloadedMethod()
# Вызов разных ипостасей перегруженного метода WithOverloadedMethod::overloaded_method(...)
over_class.overloaded_method(1)
over_class.overloaded_method(1, 2)
over_class.overloaded_method(1, 2, 3)
over_class.overloaded_method(1, 2)
over_class.overloaded_method(1)
)--");
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "1\n2\n3\n2\n1\n");
        }
    }

    void TestTryExceptions()
    {
        { // Проверка работоспособности аппарата перехвата и обработки исключений, возникающих при работе программы.
            istringstream istr(R"--(
class ErrorClass:
  def __init__(err_code):
    self.code = err_code

i = 10
while i > 0:
  i = i - 1
  try:
    if i < 5:
      err_var = ErrorClass(i)
      raise err_var
  except ErrorClass as ex_err:
    print ex_err.code
)--");
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "4\n3\n2\n1\n0\n");
        }
    }

    void TestSimpleCoroutine()
    { // Испытания работоспособности и различных способов применения существующего в языке механизма сопрограмм.
        std::string test_class_example(R"--(
class TestClass:
  def __init__(err_code):
    self.code = err_code

  def simple_method(x):
    return 2 * x

  def coroutine_method(x):
    z = x
    while (z < 20):
      z = z + 1
      co_yield z
      z = z + 2

test_instance = TestClass(0)
)--");
        { // Проверка работоспособности аппарата сопрограмм в его простейшем виде.
        std::string simple_coro_example(R"--(
ordinary_value = test_instance.simple_method(2)

coro_instance = test_instance.coroutine_method(2)
coro_value_1 = coro_instance.resume()
coro_value_2 = coro_instance.resume()
coro_value_3 = coro_instance.resume()

print ordinary_value
print coro_value_1, coro_value_2, coro_value_3
)--");

            istringstream istr(test_class_example + simple_coro_example);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "4\n3 6 9\n");
        }

        { // Чуть более сложный пример сопрограммы, где она выступает как генератор
          // (теоретически бесконечной) последовательности.
            std::string gener_coro_example(R"--(
ordinary_value = test_instance.simple_method(2)

coro_instance = test_instance.coroutine_method(2)
i = 0
while coro_instance.IsAwaiting():
  next_coro_value = coro_instance.resume()
  print i, next_coro_value
  i = i + 1

print "Всего", i
)--");

            istringstream istr(test_class_example + gener_coro_example);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "0 3\n1 6\n2 9\n3 12\n4 15\n5 18\n6 None\nВсего 7\n");
        }
    }

    void TestAwaitables()
    { // Проверка работоспособности механизма ждунов (ожидоспособных объектов), а также их функционирования в составе сопрограмм.
        { // Наличие встроенного класса Awaitable, возможности его инстанцирования.
            std::string simple_awaitable_example(R"--(
dummy_awaitable = Awaitable()  # Ждун по умолчанию.

dummy_suspend_result = dummy_awaitable.AwaitSuspend(None)
dummy_resume_result = dummy_awaitable.AwaitResume(None, 0)

# В умолчательном варианте объекта-ждуна обе его функции возвращают None.
print dummy_suspend_result
print dummy_resume_result
)--");

            istringstream istr(simple_awaitable_example);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "None\nNone\n");
        }

        { // Возможность наследования от него и корректность порождённого производного класса.
            std::string inherit_from_awaitable(R"--(
class MyAwaitable(Awaitable):
  def AwaitSuspend(coro_instance):
    return 1

  def AwaitResume(coro_instance, suspend_value):
    return 2

my_awaitable = MyAwaitable()  # Производный (унаследованный) ждун.

my_suspend_result = my_awaitable.AwaitSuspend(None)
my_resume_result = my_awaitable.AwaitResume(None, 0)

# Наш ждун-наследник через свои стандартные методы возвращает 1 и 2.
print my_suspend_result
print my_resume_result
)--");

            istringstream istr(inherit_from_awaitable);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "1\n2\n");
        }

        { // А теперь следует проверка основного сценария использования ждуна - его применения для условной приостановки
          // сопрограммы в составе оператора co_await.
            std::string coro_awaitable_suspend(R"--(
class MyAwaitable(Awaitable):
  def AwaitSuspend(coro_instance):
    return 1 # Возвращаемое значение, воспринимаемое как True. Поэтому с таким ждуном сопрограмма всегда приостанавливается.

  def AwaitResume(coro_instance, suspend_result):
    return suspend_result + 2

class TestClass:
  def __init__(use_await_p):
    self.use_awaitable = use_await_p

  def coroutine_method(x):
    z = x
    while (True):
      print z
      co_await z = z + 1 # Фиктивный co_await, ни в каком случае не производящий фактической приостановки.
      print z
      co_await self.await_result = self.use_awaitable  # Остановка сопрограммы всегда происходит здесь.
      # print self.await_result
      z = z + 2

await_instance = MyAwaitable()
test_instance = TestClass(await_instance)
coro_instance = test_instance.coroutine_method(2)
# Несколько раз возобновляем сопрограмму.
# Приостановка всегда произойдёт на "co_await self.await_result = self.use_awaitable"
# --------
# Первое возобновление (эквивалентное запуску) - вывод "2\n3\n" в консоль - начального аргумента сопрограммы Zstr == 2 и его же,
# увеличенного на 1.
coro_instance.resume()
# --------
# Далее в консоль будет выведено "5\n6\n" - (Zstr + (1 + 2) == 5) и (Zstr + (1 + 2) + 1 == 6).
coro_instance.resume()
# --------
# Вывод в консоль "8\n9\n" - (Zstr + (1 + 2 + 1 + 2) == 8) и (Zstr + (1 + 2 + 1 + 2) + 1 == 9).
coro_instance.resume()
# --------
# Наконец, последняя аналогичная операция.
# Вывод в консоль "11\n12\n" - (Zstr + (1 + 2 + 1 + 2 + 1 + 2) == 11) и (Zstr + (1 + 2 + 1 + 2 + 1 + 2) + 1 == 12) и приостановка
# на "co_await self.await_result = self.use_awaitable".
coro_instance.resume()
)--");

            istringstream istr(coro_awaitable_suspend);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            ASSERT_EQUAL(ostr.str(), "2\n3\n5\n6\n8\n9\n11\n12\n");
        }

    }

    void TestTypeTraits()
    { // Получение и иссследование типовых отпечатков (характериситического класса) для различных выражений языка.
        std::string classes_definitions(R"--(
# Переменные классовых типов программно-определяемых классов.
class OneClass:
  def OneClassMethod_1():
    self.x = 0
    self.y = "y"

  # Перегрузки метода OneClassMethod_2() с разным количеством формальных параметров.
  def OneClassMethod_2(arg_1):
    self.x = arg_1
    self.z = "z"

  def OneClassMethod_2(arg_1, arg_2):
    self.x = arg_1
    self.z = arg_2

  def OneClassMethod_3(arg_1, arg_2):
    self.x = arg_1
    self.y = arg_1
    self.z = arg_2

class TwoClass(OneClass):
  def TwoClassMethod_1():
    self.x = 2
    self.yy = "yy"

  # Переопределение метода OneClassMethod_1(), ранее определённого в родительском классе OneClass.
  def OneClassMethod_1():
    self.x = 10
    self.yyy = "yyy"

  # Новая перегрузка метода OneClassMethod_1(), на этот раз с тремя параметрами.
  def OneClassMethod_1(arg_1, arg_2, arg_3):
    self.x = 10
    self.yyyy = "yyyy"

)--");

        { // Простейшая типовая характеристика для переменной базового класса.
            std::string simple_type_traits(R"--(
# Переменные базовых классов.
# Целое число
x = 5
traits_x = TypeTraits(x)
print traits_x.IsBool(), traits_x.IsNumeric(), traits_x.IsString(), traits_x.Id(), traits_x.Name()

# Строка
y = "y"
traits_y = TypeTraits(y)
print traits_y.IsBool(), traits_y.IsNumeric(), traits_y.IsString(), traits_y.Id(), traits_y.Name()

# Пустое значение None
traits_none = TypeTraits(None)
print traits_none.IsBool(), traits_none.IsNumeric(), traits_none.IsString(), traits_none.Id(), traits_none.Name()
)--");
            istringstream istr(simple_type_traits);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            // std::cout << ostr.str() << std::endl;
            std::string etalon_string =
                "False True False " + std::to_string(NUMERIC_IDENT) + " Number\n" +
                "False False True " + std::to_string(STRING_IDENT) + " String\n" +
                "False False False " + std::to_string(NONE_IDENT) + " None\n";
            ASSERT_EQUAL(ostr.str(), etalon_string);
        }

        { // Более сложный случай взаимоотношей по родству общих программно-определяемых классов.
            std::string complex_classes_type_traits(R"--(
# Тривиальный случай соотношения наследственности между одним и тем же классом.
var_one_class = OneClass()
traits_one_class = TypeTraits(var_one_class)
print traits_one_class.IsBool(), traits_one_class.IsNumeric(), traits_one_class.IsString(), traits_one_class.Name()
print traits_one_class.IsSuccessorOf(var_one_class), traits_one_class.IsSuccessorOfName("OneClass"), traits_one_class.IsPredecessorOf(var_one_class), traits_one_class.IsPredecessorOfName("OneClass")

var_two_class = TwoClass()
traits_two_class = TypeTraits(var_two_class)
print traits_two_class.IsBool(), traits_two_class.IsNumeric(), traits_two_class.IsString(), traits_two_class.Name()
print traits_two_class.IsSuccessorOf(var_two_class), traits_two_class.IsSuccessorOfName("TwoClass"), traits_two_class.IsPredecessorOf(var_two_class), traits_two_class.IsPredecessorOfName("TwoClass")

# Перекрёстные соотношения родства классов.
print traits_one_class.IsSuccessorOf(var_two_class), traits_one_class.IsPredecessorOf(var_two_class), traits_one_class.IsSuccessorOfName("TwoClass"), traits_one_class.IsPredecessorOfName("TwoClass")
print traits_two_class.IsSuccessorOf(var_one_class), traits_two_class.IsPredecessorOf(var_one_class), traits_two_class.IsSuccessorOfName("OneClass"), traits_two_class.IsPredecessorOfName("OneClass")
)--");
            istringstream istr(classes_definitions + complex_classes_type_traits);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            // std::cout << ostr.str() << std::endl;
            std::string etalon_string = "False False False OneClass\nTrue True True True\nFalse False False TwoClass\nTrue True True True\nFalse True False True\nTrue False True False\n";
            ASSERT_EQUAL(ostr.str(), etalon_string);
        }

        { // Проверка наличия методов в объекте класса.
            std::string classes_check_metods(R"--(
var_one_class = OneClass()
traits_one_class = TypeTraits(var_one_class)
var_two_class = TwoClass()
traits_two_class = TypeTraits(var_two_class)

# С помощью экземпляра отпечатка traits_one_class проверим наличие в классе OneClass некоторых методов.
print traits_one_class.HasMethod("OneClassMethod_1", 0), traits_one_class.HasMethod("OneClassMethod_1", 1), traits_one_class.HasMethod("OneClassMethod_1", 2), traits_one_class.HasMethod("OneClassMethod_1", 3)
print traits_one_class.HasMethod("OneClassMethod_2", 0), traits_one_class.HasMethod("OneClassMethod_2", 1), traits_one_class.HasMethod("OneClassMethod_2", 2), traits_one_class.HasMethod("OneClassMethod_2", 3)
print traits_one_class.HasMethod("OneClassMethod_3", 0), traits_one_class.HasMethod("OneClassMethod_3", 1), traits_one_class.HasMethod("OneClassMethod_3", 2), traits_one_class.HasMethod("OneClassMethod_3", 3)
print traits_one_class.HasMethod("TwoClassMethod_1", 0), traits_one_class.HasMethod("TwoClassMethod_1", 1), traits_one_class.HasMethod("TwoClassMethod_1", 2), traits_one_class.HasMethod("TwoClassMethod_1", 3)

# А теперь посредством экземпляра отпечатка traits_two_class проверим наличие некоторых методов уже в классе TwoClass.
print traits_two_class.HasMethod("OneClassMethod_1", 0), traits_two_class.HasMethod("OneClassMethod_1", 1), traits_two_class.HasMethod("OneClassMethod_1", 2), traits_two_class.HasMethod("OneClassMethod_1", 3)
print traits_two_class.HasMethod("OneClassMethod_2", 0), traits_two_class.HasMethod("OneClassMethod_2", 1), traits_two_class.HasMethod("OneClassMethod_2", 2), traits_two_class.HasMethod("OneClassMethod_2", 3)
print traits_two_class.HasMethod("OneClassMethod_3", 0), traits_two_class.HasMethod("OneClassMethod_3", 1), traits_two_class.HasMethod("OneClassMethod_3", 2), traits_two_class.HasMethod("OneClassMethod_3", 3)
print traits_two_class.HasMethod("TwoClassMethod_1", 0), traits_two_class.HasMethod("TwoClassMethod_1", 1), traits_two_class.HasMethod("TwoClassMethod_1", 2), traits_two_class.HasMethod("TwoClassMethod_1", 3)
)--");
            istringstream istr(classes_definitions + classes_check_metods);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            // std::cout << ostr.str() << std::endl;
            std::string etalon_string =
                // Методы класса OneClass
                "True False False False\n"s +   // Есть метод OneClassMethod_1(), но нет методов OneClassMethod_1(a), OneClassMethod_1(a, a) и OneClassMethod_1(a, a, a).
                "False True True False\n"s +    // Есть методы OneClassMethod_2(a) и OneClassMethod_2(a, a), но нет методов OneClassMethod_2() и OneClassMethod_2(a, a, a).
                "False False True False\n"s +   // Есть метод OneClassMethod_3(a, a), но нет методов OneClassMethod_3(), OneClassMethod_3(a) и OneClassMethod_3(a, a, a).
                "False False False False\n"s +  // Нет никаких методов TwoClassMethod_1(...), определенных в наследующем классе TwoClass.
                // Методы класса TwoClass
                "True False False True\n"s +    // Есть методы OneClassMethod_1() и OneClassMethod_1(a, a, a) (определенный именно в TwoClass), но по-прежнему нет
                                                // методов OneClassMethod_1(a) и OneClassMethod_1(a, a).
                "False True True False\n"s +    // Есть метод OneClassMethod_2(a) и OneClassMethod_2(a, a), но нет методов OneClassMethod_2() и OneClassMethod_2(a, a, a).
                "False False True False\n"s +   // Есть метод OneClassMethod_3(a, a), но нет методов OneClassMethod_3(), OneClassMethod_3(a) и OneClassMethod_3(a, a, a).
                "True False False False\n"s;    // Есть метод TwoClassMethod_1(), определённый в TwoClass, но нет методов TwoClassMethod_1(a), TwoClassMethod_1(a, a) и
                                                // TwoClassMethod_1(a, a, a).
            ASSERT_EQUAL(ostr.str(), etalon_string);
        }

        { // Проверка наличия полей в объекте класса.
            std::string classes_check_fields(R"--(
var_two_class = TwoClass()
traits_two_class = TypeTraits(var_two_class)

# С помощью экземпляра отпечатка traits_two_class выполняется анализ наличия различных полей в объекте var_two_class класса TwoClass.
# Начальное состояние - никаких полей нет.
print traits_two_class.HasField("x"), traits_two_class.HasField("y"), traits_two_class.HasField("z")
print traits_two_class.HasField("yy"), traits_two_class.HasField("yyy"), traits_two_class.HasField("yyyy")

# При вызове методов класса в объекте постепенно появляются поля, задействованные в этих методах.
var_two_class.OneClassMethod_1() # После вызова метода OneClassMethod_1() должны появиться поля x и yyy.
print traits_two_class.HasField("x"), traits_two_class.HasField("y"), traits_two_class.HasField("z")
print traits_two_class.HasField("yy"), traits_two_class.HasField("yyy"), traits_two_class.HasField("yyyy")

var_two_class.TwoClassMethod_1() # После вызова метода TwoClassMethod_1() к ним добавляется поле yy. 
print traits_two_class.HasField("x"), traits_two_class.HasField("y"), traits_two_class.HasField("z")
print traits_two_class.HasField("yy"), traits_two_class.HasField("yyy"), traits_two_class.HasField("yyyy")

# После вызова метода OneClassMethod_1(arg_1, arg_2, arg_3) (перегрузка с тремя аргументами) возникает поле yyyy.
var_two_class.OneClassMethod_1(1, 2, 3)
print traits_two_class.HasField("x"), traits_two_class.HasField("y"), traits_two_class.HasField("z")
print traits_two_class.HasField("yy"), traits_two_class.HasField("yyy"), traits_two_class.HasField("yyyy")

var_two_class.OneClassMethod_2(1) # После вызова метода OneClassMethod_2(arg_1) добавляется видимое поле z.
print traits_two_class.HasField("x"), traits_two_class.HasField("y"), traits_two_class.HasField("z")
print traits_two_class.HasField("yy"), traits_two_class.HasField("yyy"), traits_two_class.HasField("yyyy")

var_two_class.OneClassMethod_2(1, 2) # После вызова метода OneClassMethod_2(arg_1, arg_2) наличие полей не изменяется.
print traits_two_class.HasField("x"), traits_two_class.HasField("y"), traits_two_class.HasField("z")
print traits_two_class.HasField("yy"), traits_two_class.HasField("yyy"), traits_two_class.HasField("yyyy")

var_two_class.OneClassMethod_3(1, 2) # После вызова метода OneClassMethod_3(arg_1, arg_2) проявляется последнее поле - y.
print traits_two_class.HasField("x"), traits_two_class.HasField("y"), traits_two_class.HasField("z")
print traits_two_class.HasField("yy"), traits_two_class.HasField("yyy"), traits_two_class.HasField("yyyy")
)--");
            istringstream istr(classes_definitions + classes_check_fields);
            ostringstream ostr;
            RunMythonProgram(istr, ostr);
            // std::cout << ostr.str() << std::endl;
            std::string etalon_string =
                // Исходное состояние объекта - полей нет.
                "False False False\n"s +
                "False False False\n" +
                // Вызов OneClassMethod_1(). Видимые поля x и yyy.
                "True False False\n" +
                "False True False\n" +
                // Вызов TwoClassMethod_1(). Видимые поля x, yy, yyy.
                "True False False\n" +
                "True True False\n" +
                // Вызов OneClassMethod_1(arg_1, arg_2, arg_3). Видимые поля x, yy, yyy, yyyy.
                "True False False\n" +
                "True True True\n" +
                // Вызов OneClassMethod_2(arg_1). Видимые поля x, z, yy, yyy, yyyy.
                "True False True\n" +
                "True True True\n" +
                // Вызов OneClassMethod_2(arg_1, arg_2). Видимые поля те же - x, z, yy, yyy, yyyy.
                "True False True\n" +
                "True True True\n" +
                // Вызов OneClassMethod_3(arg_1, arg_2). Видимы все существующие поля - x, y, z, yy, yyy, yyyy.
                "True True True\n" +
                "True True True\n"s;

            ASSERT_EQUAL(ostr.str(), etalon_string);
        }

    }

    void TestDelOperator()
    {  // Проверка работы инструкции del над простыми переменными и полями объектов.
        { // Применение del для простой переменной.
            istringstream simple_del_using(R"--(
x = 1
print x  # Эта инструкция должна выполниться нормально.
del x
print x  # А вот эта уже должна привести к выбрасыванию исключения ("отсутствие переменной").
)--");
            ostringstream ostr;
            ThrowMessageNumber err_msg_num = ThrowMessageNumber::THRM_UNKNOWN;
            try
            {
                RunMythonProgram(simple_del_using, ostr);
            }
            catch (runtime::RuntimeError& runtime_error)
            { // Здесь перехватывается общая ошибка типа runtime::RuntimeError, генерируемая интерпретатором МУФЛОНА всякий раз,
              // если внутренняя ошибка МУФЛОН-программы не обрабатывается внутри неё самой и выходит за её пределы. Тип ретранслируемой
              // "наружу" ошибки всегда в этом случае будет именно таким - runtime::RuntimeError. Частный же тип возникшей ошибки
              // можно выяснить путём анализа поля error_object_ этого класса, содержащего уже более конкретный экземпляр ошибки,
              // принадлежащей к одному из классов, определенных в заголовке error_classes.h.
                if (runtime_error)
                {
                    runtime::SyntaxError* syntax_error_ptr = runtime_error.error_object_.TryAs<runtime::SyntaxError>();
                    if (syntax_error_ptr)
                        err_msg_num = syntax_error_ptr->GetMsgNum();
                }
            }
            ASSERT_EQUAL(ostr.str(), "1\n");
            ASSERT_EQUAL(err_msg_num, ThrowMessageNumber::THRM_VARIABLE_NOT_FOUND);
        }

        { // Применение del к полю объекта - экземпляра класса.
            istringstream field_del_using(R"--(
class OneClass:
  def ClassMethod(arg_1):
    self.x = arg_1

one_class_var = OneClass()
one_class_var.ClassMethod(1)
print one_class_var.x # Эта инструкция должна выполниться нормально и вывести к контекст строку "1\n".
del one_class_var.x
print one_class_var.x # А вот эта уже должна привести к выбрасыванию исключения ("отсутствие переменной").
)--");
            ostringstream ostr;
            ThrowMessageNumber err_msg_num = ThrowMessageNumber::THRM_UNKNOWN;
            try
            {
                RunMythonProgram(field_del_using, ostr);
            }
            catch (runtime::RuntimeError& runtime_error)
            {
                if (runtime_error)
                {
                    runtime::SyntaxError* syntax_error_ptr = runtime_error.error_object_.TryAs<runtime::SyntaxError>();
                    if (syntax_error_ptr)
                        err_msg_num = syntax_error_ptr->GetMsgNum();
                }
            }
            ASSERT_EQUAL(ostr.str(), "1\n");
            ASSERT_EQUAL(err_msg_num, ThrowMessageNumber::THRM_VARIABLE_NOT_FOUND);
        }

        { // Обращение к оператору del с контролем результата с помощью функции IsVisible().
            istringstream using_del_is_visible(R"--(
x = 1
print IsVisible(x)
del x
print IsVisible(x)
x = 2
print IsVisible(x)
del x
print IsVisible(x)

class OneClass:
  def ClassMethod(arg_1):
    self.x = arg_1

one_class_var = OneClass()
one_class_var.ClassMethod(1)
print IsVisible(one_class_var.x)
del one_class_var.x
print IsVisible(one_class_var.x)
one_class_var.ClassMethod(1)
print IsVisible(one_class_var.x)
del one_class_var.x
print IsVisible(one_class_var.x)
)--");
            ostringstream ostr;
            RunMythonProgram(using_del_is_visible, ostr);
            ASSERT_EQUAL(ostr.str(), "True\nFalse\nTrue\nFalse\nTrue\nFalse\nTrue\nFalse\n");
        }

        { // Контроль за работой del с помощью объекта типового отпечатка TypeTraits.
            istringstream using_del_type_traits(R"--(
class OneClass:
  def ClassMethod_1(arg_1):
    self.x = arg_1
  def ClassMethod_2(arg_1):
    self.y = arg_1

# Создаем экземпляр класса и его характеристику.
one_class_var = OneClass()
one_class_var_traits = TypeTraits(one_class_var)

# Начинаем производить действия над объектом one_class_var, наблюдая за изменениями в one_class_var_traits.
one_class_var.ClassMethod_1(1)
print one_class_var_traits.HasField("x")
del one_class_var.x
print one_class_var_traits.HasField("x")

one_class_var.ClassMethod_2(1)
print one_class_var_traits.HasField("y")
del one_class_var.y
print one_class_var_traits.HasField("y")

one_class_var.ClassMethod_1(2)
one_class_var.ClassMethod_2(3)
print one_class_var_traits.HasField("x")
print one_class_var_traits.HasField("y")
del one_class_var.y
print one_class_var_traits.HasField("x")
print one_class_var_traits.HasField("y")
del one_class_var.x
print one_class_var_traits.HasField("x")
print one_class_var_traits.HasField("y")
)--");
            ostringstream ostr;
            RunMythonProgram(using_del_type_traits, ostr);
            ASSERT_EQUAL(ostr.str(), "True\nFalse\nTrue\nFalse\nTrue\nTrue\nTrue\nFalse\nFalse\nFalse\n");
        }
    }

    void TestClassDestructor()
    {
        { // Проверка вызова деструктора класса при уничтожении последней ссылки на него.
            istringstream class_with_dtor(R"--(
class OneClass:
  def __init__(a):
    self.var = a

  def __destroy__():
    print "Destructor :", self.var

z1_1 = OneClass(1)
z1_2 = z1_1
z1_3 = z1_1
# Удаляем ссылки на объект по одной.
del z1_3
print "del:z1_3"
del z1_1
print "del:z1_1"
del z1_2    # Тут должен быть вызван деструктор класса.
print "del:z1_2"

print "assign:create_z1_1"
z1_1 = OneClass(2) # Повторно создаём переменную z1_1 с новым экземпляром класса OneClass.
z1_1 = 5 # Здесь созданный в вышерасположенной команде объект должен быть уничтожен с вызовом его деструктора.
print "assign:z1_1"

# Наконец, проверим наличие обращений к деструкторам в процессе уничтожения объектов при завершении программы.
print "end:create_z1_2"
z1_2 = OneClass(3)
print "end:create_z1_3"
z1_3 = OneClass(4)
# При окончании программы будут разрушены объекты в z1_2 и z1_3.
)--");
            ostringstream ostr;
            RunMythonProgram(class_with_dtor, ostr);
            // std::cout << ostr.str() << std::endl;
            std::string etalon_string =
                "del:z1_3\ndel:z1_1\nDestructor : 1\ndel:z1_2\n"s +                     // Операции первой группы - удаление объектов по del.
                "assign:create_z1_1\nDestructor : 2\nassign:z1_1\n"s +                  // Операции второй группы - удаление объектов при присваивании.
                "end:create_z1_2\nend:create_z1_3\nDestructor : 3\nDestructor : 4\n"s;  // Операции третьей группы  - уничтожение объектов по завершении программы.

            ASSERT_EQUAL(ostr.str(), etalon_string);
        }
    }

    void TestFunctorClassFunction()
    { // Вызов функциональных методов различными способами.
        istringstream functor_and_calls(R"--(
class FunctorClass:
  def __init__(a):
    self.var = a

  def __run__(b):
    return self.var + b

class FunctorContainClass:
  def __init__(func_obj):
    self.func_field = func_obj

# Прямой вызов функтора (по типу свободной функции).
functor_var = FunctorClass(1)
print functor_var(10)

# Вызов функтора из поля другого объекта (по типу метода класса).
functor_contain_var = FunctorContainClass(FunctorClass(2))
print functor_contain_var.func_field(20)
)--");
        ostringstream ostr;
        RunMythonProgram(functor_and_calls, ostr);
        ASSERT_EQUAL(ostr.str(), "11\n22\n");
    }

    void TestDebugExecution()
    { // Тест исполнения программы под отладчиком (различных видов операций встроенного отладчика).
        // Лямбда-фунция сравнения двух последовательностей отладочных звонков.
        using namespace runtime;
        struct DebugEventSimp
        {
            DebugCallbackReason event_reason;
            int module_string_number;
        };
        // Функция сравнения последовательности отладочных событий, полученных при исполнении тестовой программы, с эталонной последовательностью.
        auto debug_event_sequence_check = []
            (const std::vector<DebugEventDesc>& sequence_fact, const std::vector<DebugEventSimp>& sequence_etalon) -> bool
            {
                if (sequence_fact.size() != sequence_etalon.size())
                    return false;

                for (size_t i = 0; i < sequence_fact.size(); ++i)
                {
                    const DebugEventDesc& event_fact = sequence_fact[i];
                    const DebugEventSimp& event_etalon = sequence_etalon[i];
                    if (event_fact.event_reason != event_etalon.event_reason ||
                        event_fact.command.module_string_number != event_etalon.module_string_number)
                        return false;
                }
                return true;
            };
        // Определения методов и свободных функций, которые будут использоваться в дальнейших тестах.
        std::string methods_def(R"--( 
class TestClass:                # Строка 1
  def __init__(a):              # Строка 2
    self.aa = a                 # Строка 3
                                # Строка 4
  def ClassMethod_1(a):         # Строка 5
    b = a * 2 + self.aa         # Строка 6
    print a, b                  # Строка 7
    c = b + 3                   # Строка 8
    return b + c                # Строка 9
                                # Строка 10
  def ClassMethod_2(b):         # Строка 11
    d = b * 3 / 11 + self.aa    # Строка 12
    e = d + 9.9                 # Строка 13
    print d, e                  # Строка 14
    return d + e                # Строка 15
                                # Строка 16
def FreeFunction_1(g, h):       # Строка 17
  print g, h                    # Строка 18
  gh = g * h + g - h            # Строка 19
  print gh                      # Строка 20
  return gh + (gh / 2.0)        # Строка 21
                                # Строка 22
def FreeFunction_2(j, k):       # Строка 23
  jk = j / k - j + k            # Строка 24
  print j, k                    # Строка 25
  print jk                      # Строка 26
  return jk - (jk / 3.0)        # Строка 27
)--");

        { // Пошаговое исполнение простой последовательности инструкций.
            istringstream input(R"--( # Строка 0
a = 1               # Строка 1
b = 2               # Строка 2
c = 3               # Строка 3
# Комментарий_1     # Строка 4
d = "d"             # Строка 5
ab = a + b          # Строка 6
bc = b + c          # Строка 7
# Комментарий_2     # Строка 8
abc = a + b + c     # Строка 9
# Комментарий_3     # Строка 10
print a, b, c       # Строка 11
print ab, bc, abc   # Строка 12
# Комментарий_4     # Строка 13
print d             # Строка 14
)--");
            std::tuple<std::string, std::string, std::vector<runtime::DebugEventDesc>> result_tuple =
                DebugMythonProgram(input, DebugExecutionMode::DEBUG_STEP_IN);
            // std::cout << "1. Debug -->>\n" << std::get<0>(result_tuple) << std::endl << "Out -->>\n" << std::get<1>(result_tuple) << std::endl;
            ASSERT(debug_event_sequence_check(std::get<2>(result_tuple),
                {
                    {DebugCallbackReason::DEBUG_CALLBACK_INIT, -1},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 1},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 2},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 3},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 5},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 6},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 7},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 9},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 11},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 12},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 14}
                }));
        }

        { // Пошаговый проход через составные и блочные инструкции.
            istringstream input(R"--(
a = 1                                           # Строка 1
if a == 1:                                      # Строка 2
  # Заходим сюда.                               # Строка 3
  a = a + 2                                     # Строка 4
# В этой точке a будет равно 3.                 # Строка 5
if a == 10:                                     # Строка 6
  a = a * 2                                     # Строка 7
elif a == 3:                                    # Строка 8
  # Исполняется эта ветвь условного оператора.  # Строка 9
  a = a * 3                                     # Строка 10
  a = a + 2                                     # Строка 11
else:                                           # Строка 12
  a = 1                                         # Строка 13
                                                # Строка 14
# Здесь a окажется равным 11.                   # Строка 15
while a < 33:                                   # Строка 16
  a = a + 11                                    # Строка 17
  print a                                       # Строка 18
                                                # Строка 19
print a * 2, a * 3                              # Строка 20
)--");
            std::tuple<std::string, std::string, std::vector<runtime::DebugEventDesc>> result_tuple =
                DebugMythonProgram(input, DebugExecutionMode::DEBUG_STEP_IN);
            // std::cout << "2. Debug -->>\n" << std::get<0>(result_tuple) << std::endl << "Out -->>\n" << std::get<1>(result_tuple) << std::endl;
            ASSERT(debug_event_sequence_check(std::get<2>(result_tuple),
                {
                    {DebugCallbackReason::DEBUG_CALLBACK_INIT, -1},             // Инициализация.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 1},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 2},           // Оператор if - проверка условия главной ветви.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 4},           // Условие выполнено, находимся внутри главной ветви инструкции if.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 6},           // Оператор if - проверка условия главной ветви.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 8},           // Условие главной ветви не выполняется, проверяем альтернативу.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 10},          // Условие первой альтернативы истинно, выбирается именно она.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 11},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 16},          // Первая итерация цикла while() - условие цикла удовлетворяется.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 17},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 18},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 16},          // Вторая итерация цикла while() - условие цикла удовлетворяется.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 17},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 18},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 16},          // Третья итерация цикла while() - условие цикла нарушено, цикл заканчивается.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 20}
                }));
        }

        // Тело испытательной программы, которое будет применяться в трёх следующих тестах.
        std::string main_body(R"--(
lc_1 = 5                                            # Строка 29
lc_2 = 11                                           # Строка 30
print lc_1, lc_2                                    # Строка 31
# Вызов свободной функции FreeFunction_1.           # Строка 32
ff_1 = FreeFunction_1(1, 2)                         # Строка 33
lc_2 = lc_2 + ff_1 + 2 * ff_1                       # Строка 34
print lc_2                                          # Строка 35
# Создание объекта класса и вызов его методов.      # Строка 36
tst_class = TestClass(6)                            # Строка 37
print tst_class.aa                                  # Строка 38
cls_1_ret = tst_class.ClassMethod_1(11)             # Строка 39
print cls_1_ret                                     # Строка 40
cls_2_ret = tst_class.ClassMethod_2(lc_1 + lc_2)    # Строка 41
print cls_2_ret                                     # Строка 42
cls_3 = cls_1_ret + cls_2_ret                       # Строка 43
print cls_3                                         # Строка 44
cls_3 = cls_3 * 2                                   # Строка 45
cls_3 = cls_3 * 3                                   # Строка 46
print cls_3                                         # Строка 47
)--");

        { // Пошаговое исполнение с заходом и обходом вызовов методов.
            istringstream input;
            input.str(methods_def + main_body);

            DebugExecutionModeV what_return_arr
            {
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов в процессе инициализации программы.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 29 - первая исполняемая строка программы.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 30.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 31.
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов перед исполнением строки 33 - обход тела функции FreeFunction_1().
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов перед исполнением строки 34.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 35.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 37 - вход в тело конструктора класса TestClass.
                // ----------
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 3 (внутри конструктора __init__() класса TestClass).
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед выходом из конструктора __init__() класса TestClass.
                // ----------
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 38.
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов перед исполнением строки 39 (обход вызова метода ClassMethod_1() без входа в его тело).
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов перед исполнением строки 40.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 41 - вход в тело метода ClassMethod_2().
                // ----------
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов перед исполнением строки 12 (в теле метода ClassMethod_2()).
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов перед исполнением строки 13 (в теле метода ClassMethod_2()).
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 14 (в теле метода ClassMethod_2()).
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 15 (в теле метода ClassMethod_2()) - явный возврат из метода.
                // ----------
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов перед исполнением строки 42. Сюда возвращается управление после завершения ClassMethod_2().
                DebugExecutionMode::DEBUG_NO_DEBUG      // Вызов перед исполнением строки 43. Дальнейшая работа программы происходит без трассировки.
            };
            std::tuple<std::string, std::string, std::vector<runtime::DebugEventDesc>> result_tuple = DebugMythonProgram(input, what_return_arr);
            // Вывод в консоль трассы исполнения тестовой программы.
            // std::cout << "3. Debug -->>\n" << std::get<0>(result_tuple) << std::endl << "Out -->>\n" << std::get<1>(result_tuple) << std::endl;
            ASSERT(debug_event_sequence_check(std::get<2>(result_tuple),
                {
                    {DebugCallbackReason::DEBUG_CALLBACK_INIT, -1},             // Инициализация.
                    // Далее следуют события последовательного исполнения от первой исполяемой строки программы 29 до строки 37 (вход в тело конструктора
                    // __init__() класса TestClass).
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 29},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 30},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 31},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 33},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 34},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 35},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 37},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 3},  // Заход в тело конструктора __init__() класса TestClass.
                    // Нижеследующий звонок - имитация оператора "выхода из метода" при его "естественном" завершении (исполнения до конца без явных инструкций выхода).
                    {DebugCallbackReason::DEBUG_CALLBACK_EXIT_METHOD, 4},
                    // После возврата из конструктора продолжение последовательного исполнения команд от строки 38 до строки 41.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 38},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 39},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 40},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 41},
                    // Вход в тело метода ClassMethod_2() и его нормальная работа.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 12},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 13},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 14},
                    {DebugCallbackReason::DEBUG_CALLBACK_EXIT_METHOD, 15},  // Выход из метода ClassMethod_2() с возвратом значения по return.
                    // Вновь возврат на модульный уровень.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 42},         // После выхода из метода возврат управления в строку 42.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 43},         // Звонок перед выполнением строки 43. На этом пошаговое исполнение заканчивается.
                }));
        }

        { // Заход и ускоренный выход из методов и свободных функций.
            istringstream input;
            input.str(methods_def + main_body);

            DebugExecutionModeV what_return_arr
            {
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов в процессе инициализации программы.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 29.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 30.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 31.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 33 - вход в тело функции FreeFunction_1().
                // ----------
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 18 (в теле функции FreeFunction_1()).
                DebugExecutionMode::DEBUG_EXIT_METHOD,  // Вызов перед исполнением строки 19 (в теле функции FreeFunction_1()).
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов в строке 21 перед возвратом из функции FreeFunction_1() по return.
                // ----------
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 34.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 35.
                DebugExecutionMode::DEBUG_STEP_OUT,     // Вызов перед исполнением строки 37 (обход функции-конструктора класса TestClass, в его тело не входим).
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 38.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 39 (заходим в тело метода ClassMethod_1()).
                // ----------
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 6 (в теле метода ClassMethod_1()).
                DebugExecutionMode::DEBUG_EXIT_METHOD,  // Вызов перед исполнением строки 7 (в теле метода ClassMethod_1()).
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед возвратом из метода ClassMethod_1().
                // ----------
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 40.
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 41 (вход в тело метода ClassMethod_2()).
                // ----------
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 12 (в теле метода ClassMethod_2()).
                DebugExecutionMode::DEBUG_EXIT_METHOD,  // Вызов перед исполнением строки 13 (в теле метода ClassMethod_2()).
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов в строке 15 перед возвратом из метода ClassMethod_2().
                // ----------
                DebugExecutionMode::DEBUG_STEP_IN,      // Вызов перед исполнением строки 42.
                DebugExecutionMode::DEBUG_NO_DEBUG      // Вызов перед исполнением строки 43. Дальнейшая работа программы происходит без трассировки.
            };
            std::tuple<std::string, std::string, std::vector<runtime::DebugEventDesc>> result_tuple = DebugMythonProgram(input, what_return_arr);
            // Очередную порцию трасс исполнения - в консоль.
            // std::cout << "4. Debug -->>\n" << std::get<0>(result_tuple) << std::endl << "Out -->>\n" << std::get<1>(result_tuple) << std::endl;
            ASSERT(debug_event_sequence_check(std::get<2>(result_tuple),
                {
                    {DebugCallbackReason::DEBUG_CALLBACK_INIT, -1},             // Инициализация.
                    // Ординарное исполнение строк от первой исполнимой с номером 29 до строки 33.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 29},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 30},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 31},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 33},
                    // Работа внутри функции FreeFunction_1().
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 18},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 19},
                    // Строка 20 пропускается, так как после 19-ой строки выполнен шаг типа DEBUG_EXIT_METHOD.
                    {DebugCallbackReason::DEBUG_CALLBACK_EXIT_METHOD, 21},  // Явный выход из свободной функции FreeFunction_1() с возвратом значения по return.
                    // Возврат управления на модульный уровень программы в строку 34, затем последовательное исполнение до строки 39. В строке 37 заход в тело
                    // конструктора __init__() класса TestClass не совершается, так как применён шаг типа DEBUG_STEP_OUT.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 34},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 35},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 37},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 38},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 39},     // Заход в тело метода ClassMethod_1().
                    // Исполнение тела метода ClassMethod_1(). Строка 8 пропускается (не прозванивается) вследствие совершения шага типа DEBUG_EXIT_METHOD
                    // после строки 7.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 6},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 7},
                    {DebugCallbackReason::DEBUG_CALLBACK_EXIT_METHOD, 9},   // Выход из метода ClassMethod_1() по return.
                    // Вновь работа на верхнем уровне до вызова метода ClassMethod_2() в строке 41.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 40},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 41},
                    // Обработка тела метода ClassMethod_2(). Строка 14 также опускается из-за шага типа DEBUG_EXIT_METHOD в строке 13.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 12},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 13},         // Тут делается шаг типа DEBUG_EXIT_METHOD.
                    {DebugCallbackReason::DEBUG_CALLBACK_EXIT_METHOD, 15},  // Выход из метода ClassMethod_2() по return.
                    // Возобновление работы на модульном уровне.
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 42},
                    {DebugCallbackReason::DEBUG_CALLBACK_STEP, 43}      // Шаг типа DEBUG_NO_DEBUG - прекращение трассировки.
                }));
        }

        { // Простые и условные точки останова.
            istringstream input;
            input.str(methods_def + main_body);
            // Составляем список точек останова, которые будут наложены на исполняемую программу.
            BreakpointsList break_list =
            {
                runtime::ProgramCommandDescriptor{.module_string_number = 29},    // Ординарная точка останова на строку 29 (первая действительно исполняемая строка).
                runtime::ProgramCommandDescriptor{.module_string_number = 31},    // Ординарная точка останова на строку 31.
                runtime::ProgramCommandDescriptor{.module_string_number = 31},    // Дублирующая ординарная точка останова на строку 31.
                runtime::ProgramCommandDescriptor{.module_string_number = 19},    // Ординарная точка останова на строку 19 (внутри свободной функции FreeFunction_1).
                runtime::ProgramCommandDescriptor{.module_string_number = 37},    // Ординарная точка останова на строку 37.
                MethodCharm{.name = "ClassMethod_2", .class_name = "TestClass", .arg_count =  1}, // Точка останова на вызов метода TestClass::ClassMethod_2().
                runtime::BreakpointDesc{.position{.module_string_number = 7},     // Отключённый останов на строку 7 (внутри метода TestClass::ClassMethod_1). Срабатывать не должен.
                                        .is_enabled = false},
                runtime::BreakpointDesc{.position{.module_string_number = 8}},    // Обычный останов на строку 8 (внутри метода TestClass::ClassMethod_1), но созданный с помощью BreakpointDesc{}.
                runtime::BreakpointDesc{.position{.module_string_number = 40},    // Условный бряк на строку 40.
                                        .is_conditional = true},
                runtime::BreakpointDesc{.position{.module_string_number = 14},    // Условный бряк на строку 14 (внутри метода TestClass::ClassMethod_2).
                                        .is_conditional = true},
                runtime::ProgramCommandDescriptor{.module_string_number = 44}     // Обыкновенная точка останова на строку 44.
            };

            std::tuple<std::string, std::string, std::vector<runtime::DebugEventDesc>> result_tuple =
                DebugMythonProgram(input, DebugExecutionMode::DEBUG_SIMPLE_RUN, break_list);
            // Выведем в консоль результаты трассировки.
            // std::cout << "5. Debug -->>\n" << std::get<0>(result_tuple) << std::endl << "Out -->>\n" << std::get<1>(result_tuple) << std::endl;
            ASSERT(debug_event_sequence_check(std::get<2>(result_tuple),
                {
                    {DebugCallbackReason::DEBUG_CALLBACK_INIT, -1},             // Инициализация.
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 29},
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 31},       // Однократное срабатывание сдвоенного бряка в строке 31.
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 19},
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 37},
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 8},
                    {DebugCallbackReason::DEBUG_CALLBACK_CHECK_CONDITION, 40},  // Проверка условия для условного бряка на строке 40.
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 40},
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 11},       // Срабатывание точки останова на вызов метода TestClass::ClassMethod_2()
                                                                                // при обращении к нему в строке 41 программы.
                    {DebugCallbackReason::DEBUG_CALLBACK_CHECK_CONDITION, 14},  // Проверка условия для условного бряка на строке 14.
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 14},
                    {DebugCallbackReason::DEBUG_CALLBACK_BREAKPOINT, 44}
                }));
        }

        // Далее находится группа тестов, осуществляющих работу трассировщика внутри сопрограмм.
        // Испытательная программа, содержащая сопрограмму - свободную функцию.
        std::string coro_program_free_func(R"--(
def CoroFunction_1(g, h) :                              # Строка 1
  print g, h                                            # Строка 2
  gh = g * h + g - h                                    # Строка 3
  print gh                                              # Строка 4
  while True:                                           # Строка 5
    co_yield gh + (gh / 2.0)                            # Строка 6
  print gh                                              # Строка 7
                                                        # Строка 8
# Создаём объект сопрограммы. Здесь он не выполняется.    Строка 9
coro_variable = CoroFunction_1(1, 2)                    # Строка 10
print 2 + 3                                             # Строка 11
resume_result_1 = coro_variable.resume()                # Строка 12
print resume_result_1                                   # Строка 13
resume_result_2 = coro_variable.resume()                # Строка 14
print resume_result_2                                   # Строка 15
resume_result_3 = coro_variable.resume()                # Строка 16
print resume_result_3                                   # Строка 17
# Завершаем выполнение программы.                         Строка 18
print resume_result_1, resume_result_2, resume_result_3 # Строка 19
)--");

        { // Точки останова внутри сопрограмм.
            istringstream input(coro_program_free_func);
            BreakpointsList break_list =
            {
                FreeFunctionCharm{.name = "CoroFunction_1", .arg_count = 2},    // Точка останова на вызов сопрограммы-функции CoroFunction_1(g, h).
                runtime::ProgramCommandDescriptor{.module_string_number = 6}    // Ординарная точка останова на строку 6.
            };

            std::tuple<std::string, std::string, std::vector<runtime::DebugEventDesc>> result_tuple =
                DebugMythonProgram(input, DebugExecutionMode::DEBUG_SIMPLE_RUN, break_list);
            // Выведем в консоль результаты трассировки.
            std::cout << "6. Debug -->>\n" << std::get<0>(result_tuple) << std::endl << "Out -->>\n" << std::get<1>(result_tuple) << std::endl;

        }
    }

    void TestAll()
    {
        cout << "Запуск тестов"s << endl;
        cout << endl << "Категория тестов элементарных операций интерпретатора,\nграмматического разбора и синтаксического анализа программ"s << endl;
        TestRunner tr;

        RUN_TEST(tr, TestDebugExecution);
        return;

        parse::RunOpenLexerTests(tr);
        runtime::RunObjectHolderTests(tr);
        runtime::RunObjectsTests(tr);
        ast::RunUnitTests(tr);
        TestParseProgram(tr);

        cout << endl << "Категория тестов исполнения полных примерных программ"s << endl;

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
        RUN_TEST(tr, TestStringOperations);
        RUN_TEST(tr, TestImportBinaryModule);
        RUN_TEST(tr, TestIncludes);
        RUN_TEST(tr, TestBitwiseOps);
        RUN_TEST(tr, TestShiftOps);
        RUN_TEST(tr, TestIsSameTarget);
        RUN_TEST(tr, TestMethodsOverload);
        RUN_TEST(tr, TestTryExceptions);
        RUN_TEST(tr, TestSimpleCoroutine);
        RUN_TEST(tr, TestAwaitables);
        RUN_TEST(tr, TestTypeTraits);
        RUN_TEST(tr, TestDelOperator);
        RUN_TEST(tr, TestClassDestructor);
        RUN_TEST(tr, TestFreeFunction);
        RUN_TEST(tr, TestFunctorClassFunction);
        RUN_TEST(tr, TestDebugExecution);
    }
}  // namespace

bool ScanArgvForString(int argc, char* argv[], const char* scan_row)
{
    for (int param_index = 1; param_index < argc; ++param_index)
        if (strcmp(argv[param_index], scan_row) == 0)
            return true;

    return false;
}

int main(int argc, char* argv[])
{
    // Переключим отображение текста в консоль в UTF-8 режим. Ну, по крайней мере, попытаемся...
    setlocale(LC_CTYPE, "ru_RU.UTF-8");
    try
    {
        TestAll();
        if (ScanArgvForString(argc, argv, "--console"))
        { // Если в командной строке есть параметр "--console", переходим к консольному режиму - исполнению программы,
          // вводимой пользователем со стандартного входа cin.
            RunMythonProgram(cin, cout);
            string white_line;
            getline(cin, white_line);
        }
    }
    catch (const exception& e)
    {
        cerr << e.what() << endl;
		return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
