
#include "parse.h"
#include "lexer.h"
#include "statement.h"
#include "error_classes.h"
#include "throw_messages.h"
#include "plugin_caller.h"

using namespace std;
using runtime::ThrowMessages;

namespace ITokenType = parse::token_type;

namespace
{
    pair<string, string> GetStemExt(const string& filename)
    {
        int rev_slash_pos = static_cast<int>(filename.find_last_of('\\'));
        if (rev_slash_pos == string::npos)
            rev_slash_pos = -1;
        int slash_pos = static_cast<int>(filename.find_last_of('/'));
        if (slash_pos == string::npos)
            slash_pos = -1;
        int semi_colon_pos = static_cast<int>(filename.find_last_of(':'));
        if (semi_colon_pos == string::npos)
            semi_colon_pos = -1;
        int path_margin = max(rev_slash_pos, slash_pos);
        path_margin = max(path_margin, semi_colon_pos);

        int point_pos = static_cast<int>(filename.find_last_of('.'));
        if (point_pos == string::npos || point_pos <= path_margin)
            point_pos = static_cast<int>(filename.size());

        string stem = filename.substr(path_margin + 1, point_pos - path_margin - 1);
        string ext;
        if (point_pos < static_cast<int>(filename.size()))
            ext = filename.substr(point_pos);
        return {stem, ext};
    }

    bool operator==(const parse::Token& token, char c)
    {
        const auto* p = token.TryAs<ITokenType::Char>();
        return p != nullptr && p->value == c;
    }

    bool operator!=(const parse::Token& token, char c)
    {
        return !(token == c);
    }

    class StatementFactory
    {
    public:
        StatementFactory(const parse::Lexer& lexer) : lexer_(lexer)
        {}

        template <typename T>
        [[nodiscard]] unique_ptr<T> Create(T&& object)
        {
            auto result = make_unique<T>(forward<T>(object));
            result->SetCommandDesc(lexer_.GetCurrentCommandDesc());
            if constexpr (std::is_same_v<T, ast::ClassDefinition>)
            { // Для узлов определителей классов нужно дополнительно добавить указатель на каждый такой определитель во вспомогательный словарь
              // характеризатора runtime::TypeTraitsInstance.
                runtime::TypeTraitsInstance::AppendDeclaredClassDef(result->GetClassName(), result.get());
            }

            return result;
        }

        template <typename T>
        [[nodiscard]] unique_ptr<T> Create(T&& object, runtime::ProgramCommandDescriptor targ_command_desc)
        {
            auto result = make_unique<T>(forward<T>(object));
            result->SetCommandDesc(targ_command_desc);
            return result;
        }

        template <typename T>
        T CreateTemp(T object)
        {
            object.SetCommandDesc(lexer_.GetCurrentCommandDesc());
            return object;
        }

        void AddCommandDesc(ast::Statement* statement_ptr)
        {
            statement_ptr->SetCommandDesc(lexer_.GetCurrentCommandDesc());
        }

        [[noreturn]] void ThrowParseError(const string& except_text)
        {
            string command_desc = to_string(lexer_.GetCurrentCommandDesc().module_id) + "("s +
                                  to_string(lexer_.GetCurrentCommandDesc().module_string_number) + "):"s;
            throw ParseError(command_desc + except_text);
        }

        [[noreturn]] void ThrowParseError(ThrowMessageNumber msg_num)
        {
            string command_desc = to_string(lexer_.GetCurrentCommandDesc().module_id) + "("s +
                to_string(lexer_.GetCurrentCommandDesc().module_string_number) + "):"s;
            throw ParseError(command_desc + runtime::ThrowMessages::GetThrowText(msg_num));
        }

        runtime::Method* CurrentMethod() const
        {
            return current_method_;
        }

        void SetCurrentMethod(runtime::Method* current_method = nullptr)
        {
            current_method_ = current_method;
        }

    private:
        const parse::Lexer& lexer_;
        runtime::Method* current_method_ = nullptr;
    };

    class Parser
    {
    public:
        explicit Parser(parse::Lexer& lexer, parse::ParseContext& parse_context) :
            lexer_(lexer), exec_factory_(lexer_), parse_context_(parse_context)
        {
            runtime::TypeTraitsInstance::ClearInternalClassIds();
            runtime::TypeTraitsInstance::ClearDeclaredClassDefs();
            // Регистрируем внутренние встроенные "завершённые" классы - с фиксированным набором методов, реализуемых непосредственно
            // в коде данной исполняющей среды и без возможности наследования от них и их дальнейшей модификации.
            internal_classes_[ARRAY_CLASS_NAME] = {.creator = ast::CreateArray};
            runtime::TypeTraitsInstance::AppendInternalClassId(ARRAY_CLASS_NAME, internal_classes_[ARRAY_CLASS_NAME].my_id);

            internal_classes_[MAP_CLASS_NAME] = {.creator = ast::CreateMap};
            runtime::TypeTraitsInstance::AppendInternalClassId(MAP_CLASS_NAME, internal_classes_[MAP_CLASS_NAME].my_id);

            internal_classes_[MATH_CLASS_NAME] = {.creator = ast::CreateMath};
            runtime::TypeTraitsInstance::AppendInternalClassId(MATH_CLASS_NAME, internal_classes_[MATH_CLASS_NAME].my_id);

            // Набор записей с указаниями на производящие функции экземпляров классов ошибок.
            internal_classes_[COMMON_ERROR_CLASS_NAME] = {.creator = ast::CreateCommonError};
            runtime::TypeTraitsInstance::AppendInternalClassId(COMMON_ERROR_CLASS_NAME, internal_classes_[COMMON_ERROR_CLASS_NAME].my_id);

            internal_classes_[ERROR_DIVISION_BY_ZERO_CLASS_NAME] = {.creator = ast::CreateErrorDivisionByZero};
            runtime::TypeTraitsInstance::AppendInternalClassId
                (ERROR_DIVISION_BY_ZERO_CLASS_NAME, internal_classes_[ERROR_DIVISION_BY_ZERO_CLASS_NAME].my_id);

            internal_classes_[OVERFLOW_ERROR_CLASS_NAME] = {.creator = ast::CreateOverflowError};
            runtime::TypeTraitsInstance::AppendInternalClassId(OVERFLOW_ERROR_CLASS_NAME, internal_classes_[OVERFLOW_ERROR_CLASS_NAME].my_id);

            internal_classes_[DOMAIN_ERROR_CLASS_NAME] = {.creator = ast::CreateDomainError};
            runtime::TypeTraitsInstance::AppendInternalClassId(DOMAIN_ERROR_CLASS_NAME, internal_classes_[DOMAIN_ERROR_CLASS_NAME].my_id);

            internal_classes_[ERROR_PARAMS_INCONSISTENCY_CLASS_NAME] = {.creator = ast::CreateErrorParamsInconsistency};
            runtime::TypeTraitsInstance::AppendInternalClassId
                (ERROR_PARAMS_INCONSISTENCY_CLASS_NAME, internal_classes_[ERROR_PARAMS_INCONSISTENCY_CLASS_NAME].my_id);

            internal_classes_[SYNTAX_ERROR_CLASS_NAME] = {.creator = ast::CreateSyntaxError};
            runtime::TypeTraitsInstance::AppendInternalClassId(SYNTAX_ERROR_CLASS_NAME, internal_classes_[SYNTAX_ERROR_CLASS_NAME].my_id);

            internal_classes_[MODULE_ERROR_CLASS_NAME] = {.creator = ast::CreateModuleError};
            runtime::TypeTraitsInstance::AppendInternalClassId(MODULE_ERROR_CLASS_NAME, internal_classes_[MODULE_ERROR_CLASS_NAME].my_id);

            internal_classes_[LOGIC_ERROR_CLASS_NAME] = {.creator = ast::CreateLogicError};
            runtime::TypeTraitsInstance::AppendInternalClassId(LOGIC_ERROR_CLASS_NAME, internal_classes_[LOGIC_ERROR_CLASS_NAME].my_id);

            internal_classes_[REFERENCE_ERROR_CLASS_NAME] = {.creator = ast::CreateReferenceError};
            runtime::TypeTraitsInstance::AppendInternalClassId(REFERENCE_ERROR_CLASS_NAME, internal_classes_[REFERENCE_ERROR_CLASS_NAME].my_id);

            // Регистрируем класс типовых характеристик - TypeTraits.
            internal_classes_[TYPE_TRAITS_CLASS_NAME] = {.creator = ast::CreateTypeTraits};
            runtime::TypeTraitsInstance::AppendInternalClassId(TYPE_TRAITS_CLASS_NAME, internal_classes_[TYPE_TRAITS_CLASS_NAME].my_id);

            // Создаём предопределённые "прототипы" - встроенные классы с возможностью дальнейшего наследования и модификации.
            // Класс Awaitable - "ждун". По умолчанию оба его метода просто возвращают None.
            std::vector<runtime::Method> methods;
            methods.push_back
            (
                {.name = AWAITABLE_SUSPEND_METHOD, .formal_params = {"coro_instance"s},
                 .body = std::make_unique<runtime::PsevdoExecutable>(runtime::PsevdoExecutable{})
                }
            );
            methods.push_back
            (
                {.name = AWAITABLE_RESUME_METHOD, .formal_params = {"coro_instance"s, "suspend_value"s},
                 .body = std::make_unique<runtime::PsevdoExecutable>(runtime::PsevdoExecutable{})
                }
            );
            declared_classes_[AWAITABLE_CLASS_NAME] = runtime::ObjectHolder::Own(runtime::Class(AWAITABLE_CLASS_NAME, std::move(methods), nullptr));
        }

        // Program -> eps
        //          | Statement \n Program
        unique_ptr<ast::Statement> ParseProgram()
        {
            auto result = exec_factory_.Create(ast::ProgramCompound());
            // Первому исполняемому узлу программы назначим специальный атрибут - CMD_GENUS_INITIALIZE.
            result->SetCommandGenus(runtime::CommandGenus::CMD_GENUS_INITIALIZE);
            // Далее создаём особый узел класса ClassDefinition для каждого из предопределённых классов-прототипов, находящихся к данному моменту
            // в словаре declared_classes_. Каждый такой класс будет доступен с самого начала исполнения программы для всего её последующего кода.
            for (const auto& declared_classes_pair : declared_classes_)
                result->AddStatement(std::make_unique<ast::ClassDefinition>(ast::ClassDefinition(declared_classes_pair.second)));
            while (!lexer_.CurrentToken().Is<ITokenType::Eof>())
                result->AddStatement(ParseStatement());

            return result;
        }

    private:
        // Suite -> NEWLINE INDENT (Statement)+ DEDENT
        unique_ptr<ast::Statement> ParseSuite()
        {
            lexer_.Expect<ITokenType::Newline>();
            lexer_.ExpectNext<ITokenType::Indent>();

            lexer_.NextToken();

            auto result = exec_factory_.Create(ast::Compound());
            while (!lexer_.CurrentToken().Is<ITokenType::Dedent>())
                result->AddStatement(ParseStatement());

            lexer_.Expect<ITokenType::Dedent>();
            lexer_.NextToken();

            return result;
        }

        // Methods -> [def id(Params) : Suite]*
        vector<runtime::Method> ParseMethods()
        {
            vector<runtime::Method> result;

            while (lexer_.CurrentToken().Is<ITokenType::Def>())
            {
                // Запомним истинное положение в исходном тексте строки с заголовком метода (строки, содержащей def)
                runtime::ProgramCommandDescriptor def_desc = lexer_.GetCurrentCommandDesc();
                runtime::Method m;
                exec_factory_.SetCurrentMethod(&m);

                m.name = lexer_.ExpectNext<ITokenType::Id>().value;
                lexer_.ExpectNext<ITokenType::Char>('(');

                if (lexer_.NextToken().Is<ITokenType::Id>())
                {
                    m.formal_params.push_back(lexer_.Expect<ITokenType::Id>().value);
                    while (lexer_.NextToken() == ',')
                        m.formal_params.push_back(lexer_.ExpectNext<ITokenType::Id>().value);
                }

                lexer_.Expect<ITokenType::Char>(')');
                lexer_.ExpectNext<ITokenType::Char>(':');
                lexer_.NextToken();

                m.body = exec_factory_.Create(ast::MethodBody(ParseSuite()), def_desc);

                exec_factory_.SetCurrentMethod();
                result.push_back(std::move(m));
            }
            return result;
        }

        // ClassDefinition -> Id ['(' Id ')'] : new_line indent MethodList dedent
        unique_ptr<ast::Statement> ParseClassDefinition()
        {
            string class_name = lexer_.Expect<ITokenType::Id>().value;

            lexer_.NextToken();

            const runtime::Class* base_class = nullptr;
            if (lexer_.CurrentToken() == '(')
            {
                auto name = lexer_.ExpectNext<ITokenType::Id>().value;
                lexer_.ExpectNext<ITokenType::Char>(')');
                lexer_.NextToken();

                auto it = declared_classes_.find(name);
                if (it == declared_classes_.end())
                    exec_factory_.ThrowParseError(
                        ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_BASE_CLASS) +
                        name + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_NOT_FOUND_FOR_CLASS)
                        + class_name);

                base_class = static_cast<const runtime::Class*>(it->second.Get());
            }

            lexer_.Expect<ITokenType::Char>(':');
            lexer_.ExpectNext<ITokenType::Newline>();
            lexer_.ExpectNext<ITokenType::Indent>();
            lexer_.ExpectNext<ITokenType::Def>();
            vector<runtime::Method> methods = ParseMethods();

            lexer_.Expect<ITokenType::Dedent>();
            lexer_.NextToken();

            auto [it, inserted] = declared_classes_.insert({
                class_name,
                runtime::ObjectHolder::Own(runtime::Class(class_name, std::move(methods), base_class)),
            });

            if (!inserted)
                exec_factory_.ThrowParseError(
                    ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_CLASS) + class_name
                    + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_ALREADY_EXISTS));

            return exec_factory_.Create(ast::ClassDefinition(it->second));
        }

        // Функция выделения "именного терма" - комбинации (последовательности) имён, разделённых точками.
        vector<string> ParseDottedIds()
        {
            vector<string> result(1, lexer_.Expect<ITokenType::Id>().value);

            while (lexer_.NextToken() == '.')
                result.push_back(lexer_.ExpectNext<ITokenType::Id>().value);

            return result;
        }

        // Функция разбора конструкций присваивания различного типа (прямого и косвенного), а также вызовов методов.
        //  AssgnOrCall -> DottedIds = Expr
        //               | DottedIds '(' ExprList ')'
        //               | DottedIds '(' ExprList ')' = Expr
        unique_ptr<ast::Statement> ParseAssignmentOrCall()
        {
            lexer_.Expect<ITokenType::Id>();

            vector<string> id_list = ParseDottedIds();
            string last_name = id_list.back();
            id_list.pop_back();

            if (lexer_.CurrentToken() == '=')
            {
                lexer_.NextToken();
                if (id_list.empty()) // Это присваивание свободной переменной (не полю объекта) по имени last_name.
                    return exec_factory_.Create(ast::Assignment(std::move(last_name), ParseTest()));

                return exec_factory_.Create(ast::FieldAssignment    // Это присваивание полю last_name объекта с (составным) именем id_list.
                    (exec_factory_.CreateTemp(ast::VariableValue{std::move(id_list)}),
                     std::move(last_name), ParseTest()));
            }
            lexer_.Expect<ITokenType::Char>('(');
            lexer_.NextToken();

            if (id_list.empty())
                exec_factory_.ThrowParseError(ThrowMessages::GetThrowText
                    (ThrowMessageNumber::THRM_NOT_SUPPORT_FREE_FUNCTION) + last_name);

            // Выявим возможное наличие имени класса-уточнителя в терме, указывающем на вызываемый метод.
            string parent_class_name;
            // Последний компонент id_list будем считать таким уточнителем, если он является именем какого-либо из известных
            // в данный момент классов.
            if (declared_classes_.count(id_list.back()))
            {
                parent_class_name = id_list.back();
                id_list.pop_back();
            }

            vector<unique_ptr<ast::Statement>> args;
            if (lexer_.CurrentToken() != ')')
                args = ParseTestList();

            lexer_.Expect<ITokenType::Char>(')');
            lexer_.NextToken();

            unique_ptr<ast::Statement> method_call =
                exec_factory_.Create(ast::MethodCall(exec_factory_.Create(ast::VariableValue(std::move(id_list))),
                                     std::move(last_name), std::move(args), parent_class_name));
            // Далее разбираются два варианта - вызов метода либо косвенное присваивание
            // (присваивание указателю, содержащемуся в возвращенном методом результате).
            if (lexer_.CurrentToken() == '=')
            { // После вызова метода следует лексема '=' - это косвенное присваивание.
                lexer_.NextToken();
                return exec_factory_.Create(ast::IndirectAssignment(std::move(method_call), ParseTest(), parent_class_name));
            }
            else
            { // Вызов метода последняя лексема строки - имеем дело с простым вызовом метода.
                return method_call;
            }
        }

        // Далее следуют функции, образующие синтаксический анализатор арифметических выражений
        // Expr -> BitwiseOperand ['&'/'|'/'^' BitwiseOperand]*
        unique_ptr<ast::Statement> ParseExpression()
        { // Эта функция разбирает арифметическое выражение на операции самого низкого приоритета -
          // побитово-логические бинарные выражения - побитовое И (&), побитовое ИЛИ (|) и побитовое
          // "ИСКЛЮЧАЮЩЕЕ ИЛИ" (xor, ^). Результат же её работы - исполняемый узел, вычисляющий данное
          // арифметическое выражение целиком. Отсюда и название  - ParseExpression().
            unique_ptr<ast::Statement> result = ParseBitwiseOperand();
            while (lexer_.CurrentToken() == '&' || lexer_.CurrentToken() == '|' ||
                   lexer_.CurrentToken() == '^')
            {
                char op = lexer_.CurrentToken().As<ITokenType::Char>().value;
                lexer_.NextToken();

                if (op == '&')
                    result = exec_factory_.Create(ast::BitwiseAnd(std::move(result), ParseBitwiseOperand()));
                else if (op == '|')
                    result = exec_factory_.Create(ast::BitwiseOr(std::move(result), ParseBitwiseOperand()));
                else // op == '^'
                    result = exec_factory_.Create(ast::BitwiseXor(std::move(result), ParseBitwiseOperand()));
            }
            return result;
        }

        // BitwiseOperand -> ShiftOperand ["<<"/">>" ShiftOperand]*
        unique_ptr<ast::Statement> ParseBitwiseOperand()
        { // Вот эта функция уже разбирает арифметическое выражение на операции более высокого приоритета -
          // побитовые сдвиги влево и вправо. Результат же её работы - исполняемый узел, могущий служить
          // операндом для низкоприоритетных побитово-логических операций. Отсюда и название -
          //  - ParseBitwiseOperand().
            unique_ptr<ast::Statement> result = ParseShiftOperand();
            while (lexer_.CurrentToken().Is<parse::token_type::ShiftLeft>() ||
                   lexer_.CurrentToken().Is<parse::token_type::ShiftRight>())
            {
                bool is_op_shift_left = lexer_.CurrentToken().Is<parse::token_type::ShiftLeft>();
                lexer_.NextToken();

                if (is_op_shift_left)
                    result = exec_factory_.Create(ast::ShiftLeft(std::move(result), ParseShiftOperand()));
                else
                    result = exec_factory_.Create(ast::ShiftRight(std::move(result), ParseShiftOperand()));
            }
            return result;
        }

        // ShiftOperand -> Adderr ['+'/'-' Adder]*
        unique_ptr<ast::Statement> ParseShiftOperand()
        { // Данная функция разлагает арифметическое выражение на операции ещё более высокого приоритета -
          // сложение и вычитание. Результат же её работы - исполняемый узел, могущий служить операндом для
          // битовых сдвигов. Отсюда и название  - ParseShiftOperand().
            unique_ptr<ast::Statement> result = ParseAdder();
            while (lexer_.CurrentToken() == '+' || lexer_.CurrentToken() == '-')
            {
                char op = lexer_.CurrentToken().As<ITokenType::Char>().value;
                lexer_.NextToken();

                if (op == '+')
                    result = exec_factory_.Create(ast::Add(std::move(result), ParseAdder()));
                else
                    result = exec_factory_.Create(ast::Sub(std::move(result), ParseAdder()));
            }
            return result;
        }

        // Adder -> Mult ['*'/'/'/'%' Mult]*
        unique_ptr<ast::Statement> ParseAdder()
        { // Объектом обработки данной функции являются операции следующей ступени приоритета - умножение,
          // деление и модульное деление (получение остатка от деления). В целом же формируемый ей узел может
          // являться операндом для операций более низкого приоритета (сложения и вычитания), то есть быть
          // слагаемым. Из этого следует и название - ParseAdder().
            unique_ptr<ast::Statement> result = ParseMult();
            while (lexer_.CurrentToken() == '*' || lexer_.CurrentToken() == '/' ||
                   lexer_.CurrentToken() == '%')
            {
                char op = lexer_.CurrentToken().As<ITokenType::Char>().value;
                lexer_.NextToken();

                if (op == '*')
                {
                    result = exec_factory_.Create(ast::Mult(std::move(result), ParseMult()));
                }
                else if (op == '/')
                {
                    result = exec_factory_.Create(ast::Div(std::move(result), ParseMult()));
                }
                else
                {
                    result = exec_factory_.Create(ast::ModuloDiv(std::move(result), ParseMult()));
                }
            }
            return result;
        }

        // Mult -> '(' Expr ')'
        //       | NUMBER
        //       | '-' Mult
        //       | '~' Mult
        //       | STRING
        //       | NONE
        //       | TRUE
        //       | FALSE
        //       | DottedIds '(' ExprList ')'
        //       | DottedIds
        unique_ptr<ast::Statement> ParseMult()
        { // Данная же функция выделяет операции самого высокого приоритета - унарные (негация и побитовая инверсия),
          // атомы, вызовы методов. Образуемые же ей узлы могут быть операндами для менее приоритетных операций -
          // умножения, деления, и. т. д., то есть множителями. Поэтому название - ParseMult().
            if (lexer_.CurrentToken() == '(')
            {
                lexer_.NextToken();
                auto result = ParseTest();
                lexer_.Expect<ITokenType::Char>(')');
                lexer_.NextToken();
                return result;
            }

            if (lexer_.CurrentToken() == '-')
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Mult(ParseMult(), exec_factory_.Create(ast::NumericConst(-1))));
            }

            if (lexer_.CurrentToken() == '~')
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Complement(ParseMult()));
            }

            const auto* int_num_ptr = lexer_.CurrentToken().TryAs<ITokenType::NumberInt>();
            const auto* double_num_ptr = lexer_.CurrentToken().TryAs<ITokenType::NumberDouble>();
            if (int_num_ptr || double_num_ptr)
            {
                if (int_num_ptr)
                {
                    int result = int_num_ptr->value;
                    lexer_.NextToken();
                    return exec_factory_.Create(ast::NumericConst(result));
                }
                else if (double_num_ptr)
                {
                    double result = double_num_ptr->value;
                    lexer_.NextToken();
                    return exec_factory_.Create(ast::NumericConst(result));
                }
            }

            if (const auto* str = lexer_.CurrentToken().TryAs<ITokenType::String>())
            {
                string result = str->value;
                lexer_.NextToken();
                return exec_factory_.Create(ast::StringConst(std::move(result)));
            }

            if (lexer_.CurrentToken().Is<ITokenType::True>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::BoolConst(runtime::Bool(true)));
            }

            if (lexer_.CurrentToken().Is<ITokenType::False>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::BoolConst(runtime::Bool(false)));
            }

            if (lexer_.CurrentToken().Is<ITokenType::None>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::None());
            }

            return ParseDottedIdsInMultExpr();
        }

        std::unique_ptr<ast::Statement> ParseDottedIdsInMultExpr()
        {
            vector<string> names = ParseDottedIds();

            if (lexer_.CurrentToken() == '(')
            {
                // Различные вызовы методов или свободных функций.
                auto method_name = names.back();
                names.pop_back();

                if (names.empty() && (method_name == "is_visible"sv || method_name == "IsVisible"sv))
                { // Вызов встроенной свободной функции IsVisible() (или, в другом написании имени, is_visible()). Единственный аргумент такой
                  // функции - не выражение, а имя простой переменной либо поля объекта. Поэтому она рассматривается особым порядком в первую очередь.
                    lexer_.NextToken();
                    vector<string> test_var_names = ParseDottedIds();   // Аргумент функции - имя проверяемой переменной или поля объекта.
                    lexer_.Expect<ITokenType::Char>(')');
                    lexer_.NextToken(); // Проверяем наличие закрывающей скобки списка аргументов (он у нас дрлжен быть только один), а затем пропускаем её.
                    
                    auto last_var_name = test_var_names.back();
                    test_var_names.pop_back();
                    
                    if (test_var_names.empty())
                        // Аргумент функции - имя простой (глобальной или локальной) переменной.
                        return exec_factory_.Create(ast::IsVisibleVariable(std::move(last_var_name)));
                    else
                        // Аргумент функции - имя поля объекта класса.
                        return exec_factory_.Create(ast::IsVisibleField    // Это проверка видимости поля last_var_name объекта с (составным) именем test_var_names.
                            (exec_factory_.CreateTemp(ast::VariableValue{std::move(test_var_names)}), std::move(last_var_name)));
                }

                vector<unique_ptr<ast::Statement>> args;
                if (lexer_.NextToken() != ')')
                    args = ParseTestList();

                lexer_.Expect<ITokenType::Char>(')');
                lexer_.NextToken();

                if (!names.empty())
                { // Именной терм содержит несколько разделённых точками компонент. Следовательно, это вызов метода с именем, равным его
                  // последней компоненте. А объект, метод которого вызывается, определяется всеми прочими компонентами терма, кроме последнего.
                    return exec_factory_.Create(ast::MethodCall
                        (exec_factory_.Create(ast::VariableValue(std::move(names))), std::move(method_name),
                         std::move(args)));
                }

                // Далее анализируются конструкции, эквивалентные вызову именно свободных функций (выглядящие как таковые). Именной терм таких
                // функций однокомпонентный (не содержит внутри точек).
                try
                {
                    if (auto int_class_it = internal_classes_.find(method_name); int_class_it != internal_classes_.end())
                    { // Проверка совпадения имени вызываемой свободной функции к одному из имён внутренних классов. В этом случае
                      // имеет место операция создания такого класса (вызов его производящей функции).
                        unique_ptr<ast::Statement> internal_class_ptr = (int_class_it->second.creator)(std::move(args));
                        exec_factory_.AddCommandDesc(internal_class_ptr.get());
                        return internal_class_ptr;
                    }

                    if (auto plug_it = parse_context_.GetPlugines().find(method_name); plug_it != parse_context_.GetPlugines().end())
                    { // А тут выполним проверку имени вызываемой свободной функции на принадлежность к множеству имён классов втыкал,
                      // подключённых к данному моменту к интерпретатору. Если имя принадлежит этому множеству, то происходит операция
                      // создания класса соответствующей втыкалы.
                        unique_ptr<ast::Statement> plugin_class_ptr = ast::CreateNewPluginInstance(method_name, plug_it->second, std::move(args));
                        exec_factory_.AddCommandDesc(plugin_class_ptr.get());
                        return plugin_class_ptr;
                    }
                }
                catch (ParseError& parse_error)
                { // Данное исключение выбрасывается производящими функциями встроенных либо расширительных классов (классов втыкал), если в их
                  // конструкторы переданы недопустимые аргументы (по количеству или типам).
                    exec_factory_.ThrowParseError(parse_error.what());
                }

                if (auto it = declared_classes_.find(method_name); it != declared_classes_.end())
                { // Обработка случая, при котором имя функции совпадает с одним из ранее объявленных программно определённых классов, хранящихся
                  // в declared_classes_. В этом случае выполняемая операция - это создание (инстанцирование) такого класса.
                    return exec_factory_.Create(ast::NewInstance
                        (static_cast<const runtime::Class&>(*it->second), std::move(args)));
                }
            
                if (method_name == "str"sv)
                { // Случай вызова встроенной свободной функции str().
                    if (args.size() != 1)
                        exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_STR_HAS_ONE_PARAM);
                
                    return exec_factory_.Create(ast::Stringify(std::move(args.front())));
                }

                if (method_name == "is_same_target"sv || method_name == "IsSameTarget"sv)
                { // Наконец, случай вызова встроенной свободной функции is_same_target(). Эта функция позволяет выяснить, указывают ли два её аргумента
                  // на один и тот же объект в памяти. Такое происходит, например, при присваивании какой-либо переменной значения другой переменной,
                  // так как семантика присвоения в языке предполагает именно перенацеливание принимающей переменной в левой части оператора присваивания
                  // на объект, вычисленный в правой части этого оператора.
                    if (args.size() != 2)
                        exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_IS_SAME_TARGET_HAS_TWO_PARAMS);

                    return exec_factory_.Create(ast::IsSameTarget(std::move(args[0]), std::move(args[1])));
                }

                exec_factory_.ThrowParseError(ThrowMessages::GetThrowText
                    (ThrowMessageNumber::THRM_UNKNOWN_METHOD_CALL) + method_name + "()"s);
            }
            return exec_factory_.Create(ast::VariableValue(std::move(names)));
        }

        vector<unique_ptr<ast::Statement>> ParseTestList()
        {
            vector<unique_ptr<ast::Statement>> result;
            result.push_back(ParseTest());

            while (lexer_.CurrentToken() == ',')
            {
                lexer_.NextToken();
                result.push_back(ParseTest());
            }
            return result;
        }

        // Метод обработки грамматической продукции Condition -> if LogicalExpr: Suite [else: Suite]
        unique_ptr<ast::Statement> ParseCondition()
        {
            lexer_.Expect<ITokenType::If>();
            // Запомним истинное положение в исходном тексте оператора if.
            runtime::ProgramCommandDescriptor if_desc = lexer_.GetCurrentCommandDesc();
            lexer_.NextToken();

            auto condition = ParseTest();

            lexer_.Expect<ITokenType::Char>(':');
            lexer_.NextToken();

            auto if_body = ParseSuite();

            unique_ptr<ast::Statement> else_body;
            if (lexer_.CurrentToken().Is<ITokenType::Else>())
            {
                lexer_.ExpectNext<ITokenType::Char>(':');
                lexer_.NextToken();
                else_body = ParseSuite();
            }

            return exec_factory_.Create(ast::IfElse(std::move(condition), std::move(if_body),
                                            std::move(else_body)), if_desc);
        }

        // Обработка продукции Condition -> while LogicalExpr: Suite
        unique_ptr<ast::Statement> ParseWhileCondition()
        {
            lexer_.Expect<ITokenType::While>();
            // Запомним истинное положение в исходном тексте оператора while.
            runtime::ProgramCommandDescriptor while_desc = lexer_.GetCurrentCommandDesc();
            lexer_.NextToken();

            auto condition = ParseTest();

            lexer_.Expect<ITokenType::Char>(':');
            lexer_.NextToken();

            auto while_body = ParseSuite();

            return exec_factory_.Create(ast::While(std::move(condition), std::move(while_body)), while_desc);
        }

        // Метод разбора списка классов и соответствующих им переменных, требуемых для очередного except-предложения.
        // ExceptBlockList -> [(Class as VariableName)*].
        ast::TryExcept::ClassVarPairList ParseExceptClassVarList()
        {
            ast::TryExcept::ClassVarPairList class_var_list;
            if (lexer_.CurrentToken() == ':')
                return {}; // Терм except пустой, соответствует всеядному неизберательному перехватчику.

            while (true)
            { // Цикл по всему списку, имеющему формат "имя_класса [as имя_переменной][,]".
                string class_name = lexer_.Expect<ITokenType::Id>().value;
                lexer_.NextToken();
                string var_name;
                if (lexer_.CurrentToken() == ITokenType::As{})
                {
                    lexer_.NextToken();
                    var_name = lexer_.Expect<ITokenType::Id>().value;
                    lexer_.NextToken();
                }
                class_var_list.push_back({move(class_name), move(var_name)});

                if (lexer_.CurrentToken() == ':')
                    // Следующая лексема - символ ':', список требуемых пар "класс as переменная" полагается
                    // законченным, сворачиваемся и выходим.
                    break;                             
                lexer_.Expect<ITokenType::Char>(','); // Если список не закончился, далее должен быть разделитель-запятая.
                lexer_.NextToken(); // Пропуск запятой и переход к следующему элементу сканируемого списка "класс as переменная".
            }

            return class_var_list;
        }

        // Разбор продукции контролируемого блока структурной обработки исключений:
        // TryBlock -> try: Suite [(except  ExceptBlockList: Suite)*] [except: Suite] [else: Suite] [finally: Suite]
        unique_ptr<ast::Statement> ParseTrySuite()
        {
            lexer_.Expect<ITokenType::Try>();
            // Запомним истинное положение в исходном тексте оператора try
            runtime::ProgramCommandDescriptor while_desc = lexer_.GetCurrentCommandDesc();
            lexer_.ExpectNext<ITokenType::Char>(':');
            lexer_.NextToken();

            unique_ptr<ast::Statement> try_body = ParseSuite(); // Тело контролируемого блока try.

            ast::TryExcept::ExceptBlockList except_blocks;
            while (lexer_.CurrentToken().Is<ITokenType::Except>())
            { // Цикл разбора except-термов.
                lexer_.NextToken(); // Пропуск текущей лексемы "expect" и переход к ее списку класс->переменная.

                ast::TryExcept::ExceptBlockDesc new_except_block;
                new_except_block.class_var_pairs = ParseExceptClassVarList();
                lexer_.Expect<ITokenType::Char>(':');
                lexer_.NextToken();
                new_except_block.except_body = ParseSuite();
                // Очередной (не)селективный блок-перехватчик исключений разобран и подготовлен.
                except_blocks.push_back(move(new_except_block));
            }

            // Разбор else-терма, если он присутствует.
            unique_ptr<ast::Statement> else_body;
            if (lexer_.CurrentToken().Is<ITokenType::Else>())
            {
                lexer_.ExpectNext<ITokenType::Char>(':');
                lexer_.NextToken();
                else_body = ParseSuite();
            }
            // Разбор finally-терма, если он есть.
            unique_ptr<ast::Statement> finally_body;
            if (lexer_.CurrentToken().Is<ITokenType::Finally>())
            {
                lexer_.ExpectNext<ITokenType::Char>(':');
                lexer_.NextToken();
                finally_body = ParseSuite();
            }

            return exec_factory_.Create(ast::TryExcept(move(try_body), move(except_blocks), move(else_body), move(finally_body)));
        }

        // LogicalExpr -> XorTest [OR XorTest]
        // XorTest -> AndTest [XOR AndTest]
        // AndTest -> NotTest [AND NotTest]
        // NotTest -> [NOT] NotTest
        //          | Comparison
        unique_ptr<ast::Statement> ParseTest()
        {
            auto result = ParseXorTest();
            while (lexer_.CurrentToken().Is<ITokenType::Or>())
            {
                lexer_.NextToken();
                result = exec_factory_.Create(ast::Or(std::move(result), ParseXorTest()));
            }
            return result;
        }

        unique_ptr<ast::Statement> ParseXorTest()
        {
            auto result = ParseAndTest();
            while (lexer_.CurrentToken().Is<ITokenType::Xor>())
            {
                lexer_.NextToken();
                result = exec_factory_.Create(ast::Xor(std::move(result), ParseAndTest()));
            }
            return result;
        }

        unique_ptr<ast::Statement> ParseAndTest()
        {
            auto result = ParseNotTest();
            while (lexer_.CurrentToken().Is<ITokenType::And>())
            {
                lexer_.NextToken();
                result = exec_factory_.Create(ast::And(std::move(result), ParseNotTest()));
            }
            return result;
        }

        unique_ptr<ast::Statement> ParseNotTest()
        {
            if (lexer_.CurrentToken().Is<ITokenType::Not>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Not(ParseNotTest()));
            }
            return ParseComparison();
        }

        // Comparison -> Expr [COMP_OP Expr]
        unique_ptr<ast::Statement> ParseComparison()
        {
            auto result = ParseExpression();

            const auto tok = lexer_.CurrentToken();

            if (tok == '<')
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::Less, std::move(result),
                                            ParseExpression()));
            }
            if (tok == '>')
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::Greater, std::move(result),
                                            ParseExpression()));
            }
            if (tok.Is<ITokenType::Eq>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::Equal, std::move(result),
                                            ParseExpression()));
            }
            if (tok.Is<ITokenType::NotEq>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::NotEqual, std::move(result),
                                            ParseExpression()));
            }
            if (tok.Is<ITokenType::LessOrEq>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::LessOrEqual, std::move(result),
                                            ParseExpression()));
            }
            if (tok.Is<ITokenType::GreaterOrEq>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::GreaterOrEqual, std::move(result),
                                            ParseExpression()));
            }
            return result;
        }

        // Statement -> SimpleStatement Newline
        //           | class ClassDefinition
        //           | if Condition
        //           | while Condition
        // В данном разборщике также размещена обработка специальной инструкции try начала контролируемого блока кода
        // обработчика исключений периода исполнения.
        //           | try
        unique_ptr<ast::Statement> ParseStatement()
        {
            while (true)
            {
                const auto& tok = lexer_.CurrentToken();

                if (tok.Is<ITokenType::Class>())
                {
                    lexer_.NextToken();
                    return ParseClassDefinition();
                }
                else if (tok.Is<ITokenType::If>())
                {
                    return ParseCondition();
                }
                else if (tok.Is<ITokenType::While>())
                {
                    return ParseWhileCondition();
                }
                else if (tok.Is<ITokenType::Try>())
                {
                    return ParseTrySuite();
                }

                auto result = ParseSimpleStatement();
                lexer_.Expect<ITokenType::Newline>();
                lexer_.NextToken();
                if (result.has_value())
                    return move(result.value());
            }
        }

        // 
        // Головная функция, которуя обязана экспортировать динамическая библиотека, содержащей коллекцию втыкал, - это функция с
        // именем, заданным символической константой GET_PLUGINS_INFO_FUNCTION. Она определена следующим образом:
        // 
        // const char* GetPluginsInfoFunction(uint32_t load_level);
        // 
        // Анализ и поиск втыкал после загрузки такой библиотеки начинается именно с её импорта и вызова. Данная функция должна
        // возвращать указатель на коллекцию строк, заверщающихся нулём, по одной строке для каждой втыкалы, которую предоставляет
        // данная динамическая библиотека. Вся коллекция должна завершаться пустой строкой, т. е. содержащий только один нуль. Эта
        // строка - имя информирующей функции (условно будем далее называть её GetPluginInfo) втыкалы. Аргумент load_level может
        // использоваться кодом втыкалы для выбора различных их вариантов, возвращаемых по тому или иному запросу. По умолчанию
        // (для запроса некоего "общего", типового варианта втыкал) это поле должно быть равно 0.
        // 
        // Втыкала, подключаемая из памяти, всегда индивидуальна (представляет собой единственный класс), поэтому такой функции-перечислителя
        // не содержит. Её обработка начинается непосредственно с обращения за данными к её информирующей функции.
        // 
        // Функция-информатор предоставляет всю информацию, необходимую для последующей работы со втыкалой, а её сигнатура выглядит так:
        //      int32_t GetPluginInfo(uint32_t request_type, void* source_area, int32_t source_length, void* target_area, int32_t target_length);
        // Здесь request_type - тип запроса (один из членов перечисления PluginInfoRequest),
        //       source_area - указатель на область-источник, в которой хранятся входные параметры запроса.
        //       source_length - длина этой источниковой области, содержащей полезные данные.
        //       target_area - указатель на целевую область, куда будет сохранена информация по запросу request_type,
        //       target_length - предельная её длина.
        // Возвращаемое значение: при нормальном завершении запроса - длина скопированных в приёмное поле данных (значение более или
        //                        равно 0).
        //                        при ошибке - один из кодов ошибки из состава перечисления PluginErrorCode (всегда меньше нуля).
        // Все типы запросов, которые необходимо поддержать в обязательном порядке, указаны в перечислении PluginInfoRequest.
        //
        //      PLUG_REQUEST_PLUGIN_NAME - получение имени втыкалы.
        // В ответ на этот запрос в область получателя будет сохранена заканчивающаяся нулём строка с именем данной втыкалы.
        //
        //    PLUG_REQUEST_CALL_FUNCTION_NAME - получение имени функции, выполняющей вызов методов данной втыкалы (поддерживается только
        //                                      для втыкалы, размещённой в DLL).
        // Результатом выполнения этого запроса будет заполнение целевой области именем функции в виде C-строки, которая экспортируется
        // динамической библиотекой втыкалы и служит для обращения к её методам (условно будем называть эту функцию CallPluginMethod).
        // Для втыкал из памяти (ОЗУ) запрос должен завершиться ошибкой.
        // 
        //    PLUG_REQUEST_CALL_FUNCTION_ADDR - получение адреса функции, выполняющей вызов методов данной втыкалы (поддерживается только для
        //                                      втыкалы, сформированной и загружаемой из памяти).
        // В ходе этого запроса в целевую область сохраняется адрес (указатель на) "вызывной" функции соответствующей втыкалы. Для втыкал,
        // оформленных как динамические библиотеки, запрос должен оканчиваться возникновением ошибки.
        // 
        // В обоих случаях сигнатура этой вызывающей функции должна быть следующей:
        // void CallPluginMethod(const char* method_name, uintptr_t plugin_method_call_id);
        //      method_name - имя вызываемого метода (завершается нулём).
        //      plugin_method_call_id - идентификатор вызова, который далее может использоваться кодом втыкалы для обращения к прочим
        //                              вспомогательным функциям, экспортируемым ядром интерпретатора.
        // 
        //    PLUG_REQUEST_METHOD_LIST - получение списка имён методов класса втыкалы, доступных для вызова.
        // В приёмную область записывается коллекция C-строк, завершающихся нулём. Каждая такая строка указывает на один доступный для вызова
        // метод класса. Коллекция завершается пустой строкой.
        //
        //    PLUG_REQUEST_METHOD_PARAMS - предоставление описания входных параметров некоторого метода, предоставляемого втыкалой для обращения.
        // Область источника source_area содержит структуру типа RequestMethodParams. Первое её поле method_name указывает на C-строку с именем метода,
        // к которому относится запрос. Второе поле - method_ordinal - ординал (порядковый номер) метода. Это номер интересующего нас метода в списке,
        // возвращаемом по запросу PLUG_REQUEST_METHOD_LIST. Данное поле служит для обеспечения возможности перегрузки методов, то есть существования
        // методов с одинаковыми именами, но разными требованиями на фактические аргументы. Если данный метод не перегружен, то это поле должно
        // игнорироваться и может содержать любое значение.
        // В ответ целевая область заполняется данными структуры PluginMethodDefiner. Сразу после её завершения в приёмную область записывается также
        // содержимое массива param_types, количество членов в котором определяется полем param_types_count структуры PluginMethodDefiner. Каждый из
        // них яляется переменной типа uint32_t (эквивалентной какому-либо члену перечисления MethodParamType) и указывает требуемый тип очередного
        // фактического параметра метода.
        //
        //    PLUG_REQUEST_HELPER_FUNCTIONS - передача втыкале указателей на сервисные функции ядра исполнительской системы. При запросе буфер
        // source_area будет содержать структуру PluginHelperFunctions с адресами всех предоставляемых ядром служебных функций. Ответа со стороны
        // втыкалы не ожидается, поэтому целевая область при данном запросе не указывается (target_area = nullptr и target_length = 0) и обращение
        // к ней со стороны кода втыкалы не допускается.

        // Сначала создадим подпрограмму, содержащую общую часть функциональности, необходимой для загрузки втыкал как из разделяемых библиотек,
        // так и из памяти. К моменту вызова этой функции модуль втыкалы должен быть загружен в память, а также должны быть выяснены адреса его
        // экспортируемых функций - информирующей и вызывной.
        void LoadCommonLibrary(const std::string& plugin_name, PluginGetInfoFunc plugin_info_func,
                               PluginCallMethodFunc plugins_call_func, const std::string& library_alias)
        {
            #define IN_BUFFER_SIZE 256
            char in_buffer[IN_BUFFER_SIZE];
            #define OUT_BUFFER_SIZE 2048
            char out_buffer[OUT_BUFFER_SIZE];

            // Получаем информацию о существующих методах класса втыкалы.
            if (plugin_info_func(PluginInfoRequest::PLUG_REQUEST_METHOD_LIST, nullptr, 0, out_buffer, OUT_BUFFER_SIZE) == 0)
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Получен пустой ответ вместо списка методов.
            const char* plugin_methods_scan_ptr = out_buffer;
            std::vector<std::string> plugin_methods_names;
            while (true)
            {
                // Проверка очередного полученного имени публичного метода втыкалы на соблюдение правила предельной длины.
                if (!CheckStringMaxLength(out_buffer, MAX_PLUGIN_NAMES_LEN))
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Имя метода превышает допустимую длину.

                std::string plugin_method_name(plugin_methods_scan_ptr); // Строка, простирающаяся от plugin_methods_scan_ptr до ближайшего нуля.
                if (plugin_method_name.empty())
                    break;  // Список методов, принадлежащих классу втыкалы, закончен.

                plugin_methods_scan_ptr += (plugin_method_name.size() + 1);  // Перенацеливаем указатель на начало следующей строки коллекции.
                plugin_methods_names.push_back(std::move(plugin_method_name));
            }

            // Теперь предстоит получить у информатора описание каждого из методов класса, то есть сведения о допустимых его формальных и
            // фактических аргументах и предварительных условиях, которые на них накладываются.
            std::unordered_multimap<std::string, ast::MethodDefiner> plugin_method_definers;
            uint32_t method_index = 0;
            for (const std::string& method_name : plugin_methods_names)
            {
                RequestMethodParams* request_params_ptr = reinterpret_cast<RequestMethodParams*>(in_buffer);
                request_params_ptr->method_name = method_name.c_str();
                request_params_ptr->method_ordinal = method_index++;
                if (plugin_info_func(PluginInfoRequest::PLUG_REQUEST_METHOD_PARAMS, in_buffer, sizeof(RequestMethodParams), out_buffer, OUT_BUFFER_SIZE) <
                    static_cast<int32_t>(sizeof(PluginMethodDefiner)))
                    // Ответ на запрос о характеристиках фактических параметров метода должен как минимум содержать запись типа PluginMethodDefiner.
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);

                // Структура PluginMethodDefiner упакованная, поэтому типовой указатель на неё может ссылаться на любой адрес, без учёта выравнивания.
                PluginMethodDefiner* ext_method_definer = reinterpret_cast<PluginMethodDefiner*>(out_buffer);
                ast::MethodDefiner new_method;
                new_method.name = method_name;                                      // Имя метода.
                new_method.arg_count_min = ext_method_definer->arg_count_min;       // Минимально допустимое количество его параметров.
                new_method.arg_count_max = ext_method_definer->arg_count_max;       // Максимально допустимое количество его параметров.
                // Режим проверки допустимости фактических параметров метода.
                new_method.check_mode = static_cast<MethodParamCheckMode>(ext_method_definer->check_mode);
                // Заполним массив типовых требований на фактические параметры.
                char* param_types_ptr = reinterpret_cast<char*>(out_buffer) + sizeof(PluginMethodDefiner);
                for (uint32_t param_type_index = 0; param_type_index < ext_method_definer->param_types_count;
                    ++param_type_index, param_types_ptr += sizeof(uint32_t))
                {
                    uint32_t new_method_param_type; // Выровненное значение типа uint32_t.
                    memcpy(&new_method_param_type, param_types_ptr, sizeof(uint32_t));
                    new_method.param_types.push_back(static_cast<MethodParamType>(new_method_param_type));
                }
                plugin_method_definers.emplace(method_name, std::move(new_method));
            }
            // Последний акт пьесы. Передаём втыкале сведения о сервисных функциях ядра, которые она будет использовать для передачи и получения
            // информации от/к него/нему.
            PluginHelperFunctions helper_funcs_info;
            // Готовим запись с информацией о данных функциях.
            helper_funcs_info.get_instance_func = &PluginGetInstanceId;
            helper_funcs_info.set_runtime_error_func = &PluginSetRuntimeError;
            helper_funcs_info.set_result_value_func = &PluginSetResultValue;
            helper_funcs_info.params_count_func = &PluginParamsCount;
            helper_funcs_info.param_type_func = &PluginParamType;
            helper_funcs_info.param_get_value_func = &PluginParamGetValue;
            helper_funcs_info.param_string_size_func = &PluginParamStringSize;
            helper_funcs_info.print_to_context_func = &PluginPrintToContext;
            // Всё готово, выполняем информирующий запрос.
            plugin_info_func(PluginInfoRequest::PLUG_REQUEST_HELPER_FUNCTIONS, &helper_funcs_info, sizeof(PluginHelperFunctions), nullptr, 0);
            // Итак, подытожим результаты проделанной работы. Вся необходимая информация о втыкале получена, нужные её настройки также проделаны.
            // Осталось только сохранить её в очередной элемент накопителя внутри parse_context_.
            ast::PluginDescData new_plugin_desc;
            new_plugin_desc.info_func = plugin_info_func;
            new_plugin_desc.call_func = plugins_call_func;
            new_plugin_desc.methods = std::move(plugin_method_definers);
            // Описатель класса, предоставляемого текущей обрабатываемой втыкалой, полностью сформирован.
            parse_context_.GetPlugines().emplace(library_alias + "_"s + plugin_name, new_plugin_desc);
            // Обеспечим также возможность обращения к первому классу втыкала без суффикса имени класса.
            if (parse_context_.GetPlugines().count(library_alias) == 0 && internal_classes_.count(library_alias) == 0)
                parse_context_.GetPlugines().emplace(library_alias, new_plugin_desc);
        }

        // Пробуем загружать втыкало из разделяемой (динамической) библиотеки.
        // Внутри этой функции реализована процедура загрузки динамической библиотеки для разных ОС.
        void LoadImportLibrary(const string& library_filename, const string& library_alias)
        {
            #define WCHAR_FILENAME_SIZE 2048
            wchar_t wchar_buffer[WCHAR_FILENAME_SIZE];

            #if defined (_WIN64) || defined(_WIN32)
                // Вариант загрузки динамической библиотеки для Виндоус.
                if (mbstowcs(wchar_buffer, library_filename.c_str(), WCHAR_FILENAME_SIZE - 1) ==
                    static_cast<size_t>(-1))
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_IMPORT_FILENAME);
                HMODULE hAddonDll = LoadLibraryW(wchar_buffer);
                if (!hAddonDll || hAddonDll == INVALID_HANDLE_VALUE)
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_DYNAMIC_LIBRARY_NOT_LOADED);
                // Оборудуем небольшой RAII-нный фантик вокруг обработчика загруженной динамической библиотеки.
                std::unique_ptr<void, decltype([](void* dll_handle)
                    {
                        if (dll_handle)
                            FreeLibrary((HMODULE)dll_handle);
                    })> AddonDllDeleter((void*)hAddonDll);
                FuncGetPluginInfoNames get_plugins_info_func_name =
                    reinterpret_cast<FuncGetPluginInfoNames>(GetProcAddress(hAddonDll, GET_PLUGINS_INFO_FUNCTION));
            #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
                // Здесь реализована загрузка .SO линукса/юникса.
                void* hAddonDll = dlopen(library_filename.c_str(), RTLD_NOW | RTLD_GLOBAL);
                if (!hAddonDll)
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_DYNAMIC_LIBRARY_NOT_LOADED);
                // RAII-нная обертка для обработчика динамической библиотеки будет выглядеть здесь так.
                std::unique_ptr<void, decltype([](void* dll_handle)
                    {
                        if (dll_handle)
                            dlclose(hAddonDll);
                    }) > AddonDllDeleter(hAddonDll);
                FuncGetPluginInfoNames get_plugins_info_func_name =
                    reinterpret_cast<FuncGetPluginInfoNames>(dlsym(hAddonDll, GET_PLUGINS_INFO_FUNCTION));
            #else
                // Какие-то другие, неподдерживаемые варианты платформ.
                return;
            #endif
            if (!get_plugins_info_func_name) // Корневая функция-перечислитель динамической библиотеки не найдена.
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_LOAD_PLUGIN_LIST_NOT_FOUND);

            // Получаем от функции-перечислителя список доступных втыкал, предоставляемых загруженной библиотекой.
            // Для каждой из них нам возвращают имя информирующей функции, через которую мы получим всю прочую необходимую нам информацию.
            const char* plugin_names_scan_ptr = get_plugins_info_func_name(0);
            std::vector<std::string> inform_func_names;
            while (true)
            {
                // Проверим очередную строку коллекции (предполагаемое имя функции-информатора) на соблюдение ей правила предельной длины.
                if (!CheckStringMaxLength(plugin_names_scan_ptr, MAX_PLUGIN_NAMES_LEN))
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_LOAD_PLUGIN_LIST); // Имя функции слишком длинное.

                std::string inform_func_name(plugin_names_scan_ptr); // Строка, простирающаяся от plugin_names_scan_ptr до ближайшего нуля.
                if (inform_func_name.empty())
                    break;  // Список доступных втыкал (точнее, их информирующих функций) закончен.

                plugin_names_scan_ptr += (inform_func_name.size() + 1);  // Перенацеливаем указатель на начало следующей строки коллекции.
                inform_func_names.push_back(std::move(inform_func_name));
            }

            // Перебираем все полученные имена функций-информаторы, получаем от них все необходимые сведения о втыкалах и производим прочие
            // действия, необходимые для подключения их к исполнительной системе.
            for (const std::string inform_func_name : inform_func_names)
            {
                // Получаем адрес функции-информатора.
                #if defined (_WIN64) || defined(_WIN32)
                    // Вариант для Виндоус.
                    PluginGetInfoFunc plugin_info_func =
                        reinterpret_cast<PluginGetInfoFunc>(GetProcAddress(hAddonDll, inform_func_name.c_str()));
                #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
                    // Линукс и прочие POSIX-совместимые.
                    PluginGetInfoFunc plugin_info_func =
                        reinterpret_cast<PluginGetInfoFunc>(dlsym(hAddonDll, inform_func_name.c_str()));
                #endif
                if (!plugin_info_func) // Функция-информатор втыкалы не найдена среди экспорта динамической библиотеки.
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);

                // Запрашиваем у неё имя обслуживаемой ей втыкалы.
                if (plugin_info_func(PluginInfoRequest::PLUG_REQUEST_PLUGIN_NAME, nullptr, 0,
                                     wchar_buffer, WCHAR_FILENAME_SIZE * sizeof(wchar_t)) == 0)
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Получено пустое имя втыкалы.
                // Проверим возвращённое нам имя втыкалы на ограничение максимальной длины.
                if (!CheckStringMaxLength(reinterpret_cast<char*>(wchar_buffer), MAX_PLUGIN_NAMES_LEN))
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Имя втыкалы слишком длинное.
                std::string plugin_name(reinterpret_cast<char*>(wchar_buffer));

                // Далее выясним сначала имя "вызывной" функции, а затем её адрес.
                if (plugin_info_func(PluginInfoRequest::PLUG_REQUEST_CALL_FUNCTION_NAME, nullptr, 0,
                                     wchar_buffer, WCHAR_FILENAME_SIZE * sizeof(wchar_t)) == 0)
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Пустой ответ на запрос имени вызывной функции.

                // Проверим полученное нами имя вызывающей функции на непревышение им предельной длины.
                if (!CheckStringMaxLength(reinterpret_cast<char*>(wchar_buffer), MAX_PLUGIN_NAMES_LEN))
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Ошибка - имя функции превышает макс. длину.
                std::string plugin_call_func_name(reinterpret_cast<char*>(wchar_buffer));
                if (plugin_name.empty() || plugin_call_func_name.empty())   // Имя втыкалы или имя его вызываюшей функции некорректное.
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);

                // Разрешение адреса вызывающей функции класса втыкалы в различных вариантах для разных ОС.
                #if defined (_WIN64) || defined(_WIN32)
                    // Вариант для Виндоус.
                    PluginCallMethodFunc plugins_call_func =
                        reinterpret_cast<PluginCallMethodFunc>(GetProcAddress(hAddonDll, plugin_call_func_name.c_str()));
                #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
                    // Линукс и прочие POSIX-совместимые.
                    PluginCallMethodFunc plugins_call_func =
                        reinterpret_cast<PluginCallMethodFunc>(dlsym(hAddonDll, plugin_call_func_name.c_str()));
                #endif
                if (!plugins_call_func) // Не удалось установить адрес вызывающей функции.
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);

                // Выполняем операции по дальнейшей обработке втыкалы, оформленные как отдельная процедура.
                LoadCommonLibrary(plugin_name, plugin_info_func, plugins_call_func, library_alias);
            }

            AddonDllDeleter.release();
            parse_context_.AddDLLEntry(hAddonDll);
        }

        // Подключение втыкалы, модуль которой находится в ОЗУ.
        void LoadRAMLibrary(PluginGetInfoFunc plugin_inform_func, const string& library_alias)
        {
            #define OUT_BUFFER_SIZE 2048
            char out_buffer[OUT_BUFFER_SIZE];

            // В качествен аргумента нам передан указатель на информирующую функцию втыкалы, которая уже подготовлена к работе. Её нужно
            // только опросить для получения всех необходимых сведений о данной втыкале и произвести дальнейшие действия по её включению
            // в исполнительский комплекс.
            // Сначала запрашиваем у неё имя обслуживаемой ей втыкалы.
            if (plugin_inform_func(PluginInfoRequest::PLUG_REQUEST_PLUGIN_NAME, nullptr, 0, out_buffer, OUT_BUFFER_SIZE) == 0)
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Получено пустое имя втыкалы.            
            // Проверим возвращённую нам информацию (имя втыкалы) в out_buffer на предельную длину.
            if (!CheckStringMaxLength(out_buffer, MAX_PLUGIN_NAMES_LEN))
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Имя втыкалы превышает допустимую длину.

            std::string plugin_name(out_buffer);
            if (plugin_name.empty())   // Имя втыкалы некорректное.
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);

            // Далее выясним адрес "вызывной" функции (получим указатель на неё).
            if (plugin_inform_func(PluginInfoRequest::PLUG_REQUEST_CALL_FUNCTION_ADDR, nullptr, 0, out_buffer, OUT_BUFFER_SIZE) < sizeof(PluginCallMethodFunc))
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);    // Пустой ответ на запрос адреса вызывной функции.
            PluginCallMethodFunc plugins_call_func;
            memcpy(&plugins_call_func, out_buffer, sizeof(PluginCallMethodFunc));
            if (!plugins_call_func) // Не удалось установить адрес вызывающей функции.
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_PLUGIN_DATA);

            // Выполняем операции по дальнейшей обработке втыкалы, оформленные как отдельная процедура.
            LoadCommonLibrary(plugin_name, plugin_inform_func, plugins_call_func, library_alias);
        }

        void ProcessImportLibrary(std::vector<parse::Token> args, const parse::ParseContext& parse_context)
        {
            std::string library_filename, library_alias;
            if (args.size() != 1 && args.size() != 2)
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INCORRECT_TOKEN_LIST);
            for (parse::Token& current_token : args)
            {
                if (!current_token.Is<ITokenType::String>())
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INCORRECT_TOKEN_LIST);
            }

            library_filename = args[0].As<ITokenType::String>().value;
            if (args.size() == 2)
                library_alias = args[1].As<ITokenType::String>().value;
            // Пробуем загрузить разделяемую библиотеку по имени library_filename.
            LoadLibraryDefine lib_desc = parse_context.GetLoadLibraryDesc(library_filename);
            if (holds_alternative<monostate>(lib_desc))
                return;
            if (holds_alternative<PluginGetInfoFunc>(lib_desc))
            { // Подсоединение втыкала, уже существующего в памяти.
                if (library_alias.empty())
                    library_alias = library_filename;
                LoadRAMLibrary(get<PluginGetInfoFunc>(lib_desc), library_alias);
                return;
            }
            // Далее будем пытыться загрузить втыкало из разделяемой библиотеки
            if (library_alias.empty())
                library_alias = GetStemExt(library_filename).first;
            LoadImportLibrary(get<std::string>(lib_desc), library_alias);
        }

        // Функция делает текущий метод сопрограммой, выполняя при этом проверку допустимости этой операции.
        void MakeCurrentMethodCoroutine()
        {
            if (runtime::Method* current_method = exec_factory_.CurrentMethod())
            { 
                if (current_method->name.substr(0, 2) == "__")
                    // Специальные методы (точки настройки типа __init__, __add__, __eq__, и.т.д.), имена которых
                    // начинаются с "__", не могут быть сопрограммами.
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_SPECIAL_METHOD_CANT_COROUTINE);
                current_method->is_coroutine = true;
            }
        }

        // Кроме команд периода исполнения (print, break, и. т. д.), здесь также
        // обрабатываются директивы времени трансляции (например, import).
        // StatementBody -> return Expression
        //               | co_yield Expression
        //               | return_ref VariableValue
        //               | return_ref MethodCall
        //               | co_yield_ref VariableValue
        //               | co_yield_ref MethodCall
        //               | raise Expression
        //               | print ExpressionList
        //               | break
        //               | continue
        //               | pass
        //               | del DottedIds
        //               | AssignmentOrCall
        std::optional<unique_ptr<ast::Statement>> ParseSimpleStatement()
        {
            const auto& tok = lexer_.CurrentToken();

            if (tok.Is<ITokenType::Import>())
            {
                lexer_.NextToken();
                vector<parse::Token> args;
                if (!lexer_.CurrentToken().Is<ITokenType::Newline>())
                    args = ParseTokenList();
                ProcessImportLibrary(move(args), parse_context_);
                return nullopt;
            }

            if (tok.Is<ITokenType::Include>())
            {
                lexer_.NextToken();
                vector<parse::Token> args;
                if (!lexer_.CurrentToken().Is<ITokenType::Newline>())
                    args = ParseTokenList();
                if (args.size() != 1 || !args[0].Is<ITokenType::String>())
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INCLUDE_INVALID_PARAMS);
                lexer_.IncludeSwitchTo(args[0].As<ITokenType::String>().value);
                return nullopt;
            }

            if (tok.Is<ITokenType::Return>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Return(ParseTest()));
            }

            if (tok.Is<ITokenType::CoYield>())
            {
                MakeCurrentMethodCoroutine();   // Наличие оператора co_yield делает метод сопрограммой.
                lexer_.NextToken();
                return exec_factory_.Create(ast::CoYield(ParseTest()));
            }

            if (tok.Is<ITokenType::CoAwait>())
            {
                MakeCurrentMethodCoroutine();   // Наличие оператора co_await делает метод сопрограммой.
                // Аргументом инструкции co_await должен быть обыкновенный оператор присваивания (возможно и нечто иное, но оно должно
                // быть результатом анализа, выполненного функцией ParseAssignmentOrCall()).
                lexer_.NextToken();
                return exec_factory_.Create(ast::CoAwait(ParseAssignmentOrCall()));
            }

            if (tok.Is<ITokenType::Raise>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Raise(ParseTest()));
            }

            if (tok.Is<ITokenType::ReturnRef>() || tok.Is<ITokenType::CoYieldRef>())
            {
                bool is_co_yield_ref = tok.Is<ITokenType::CoYieldRef>();
                if (is_co_yield_ref)
                    MakeCurrentMethodCoroutine();   // Наличие оператора co_yield_ref делает метод сопрограммой.

                lexer_.NextToken();
                auto test_result = ParseTest();
                ast::VariableValue* variable_value_ptr = dynamic_cast<ast::VariableValue*>(test_result.get());
                ast::MethodCall* method_call_ptr = dynamic_cast<ast::MethodCall*>(test_result.get());
                if (variable_value_ptr)
                { // Вариант return_ref с именем поля данного объекта в качестве аргумента.
                    vector<string> dotted_ids = variable_value_ptr->GetDottedIds();
                    if (dotted_ids.size() && dotted_ids[0] == "self"sv)
                        return exec_factory_.Create(ast::ReturnRef(move(dotted_ids), is_co_yield_ref));
                }
                else if (method_call_ptr)
                { // Вариант return_ref с вызовом метода в качестве аргумента. Метод обязательно должен принадлежать тому
                  // же объекту, внутри которого выполняется данный return_ref (то есть содержать префикс self).
                    if (variable_value_ptr = method_call_ptr->GetCallObject())
                    {
                        vector<string> dotted_ids = variable_value_ptr->GetDottedIds();
                        if (dotted_ids.size() && dotted_ids[0] == "self"sv)
                            // Аргумент return_ref является вызовом именно некоторого метода данного класса.
                            return exec_factory_.Create(ast::ReturnRef(move(test_result), is_co_yield_ref));
                    }
                }
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_POINTER_RET_TO_VAL_DENIED);
            }

            if (tok.Is<ITokenType::Print>())
            {
                lexer_.NextToken();
                vector<unique_ptr<ast::Statement>> args;
                if (!lexer_.CurrentToken().Is<ITokenType::Newline>())
                    args = ParseTestList();

                return exec_factory_.Create(ast::Print(std::move(args)));
            }

            if (tok.Is<ITokenType::Delete>())
            { // Разбор оператора del - удаление объекта из области видимости (таблицы символов).
                lexer_.NextToken();
                vector<string> id_list = ParseDottedIds();
                string last_name = id_list.back();
                id_list.pop_back();

                if (id_list.empty()) // Это удаление свободной переменной (не поля объекта) по имени last_name.
                    return exec_factory_.Create(ast::DeleteVariable(std::move(last_name)));

                return exec_factory_.Create(ast::DeleteField    // Это удаление поля last_name объекта с (составным) именем id_list.
                    (exec_factory_.CreateTemp(ast::VariableValue{std::move(id_list)}), std::move(last_name)));
            }

            if (tok.Is<ITokenType::Break>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Break());
            }        

            if (tok.Is<ITokenType::Continue>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Continue());
            }
        
            if (tok.Is<ITokenType::Pass>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Pass());
            }

            return ParseAssignmentOrCall();
        }

        vector<parse::Token> ParseTokenList()
        {
            vector<parse::Token> result;
            parse::Token previous_token = parse::token_type::None{}, current_token;

            while (true)
            {
                current_token = lexer_.CurrentToken();
                if (current_token.Is<ITokenType::Newline>())
                {
                    result.push_back(previous_token);
                    break;
                }
                else if (current_token == ',')
                {
                    result.push_back(previous_token);
                    previous_token = parse::token_type::None{};
                }
                else
                {
                    if (previous_token.Is<ITokenType::None>() &&
                        (current_token.Is<ITokenType::NumberInt>() ||
                            current_token.Is<ITokenType::NumberDouble>() ||
                            current_token.Is<ITokenType::String>() ||
                            current_token.Is<ITokenType::True>() ||
                            current_token.Is<ITokenType::False>()))
                        previous_token = current_token;
                    else
                        exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INCORRECT_TOKEN_LIST);
                }

                lexer_.NextToken();
            }
            return result;
        }

        parse::Lexer& lexer_;
        StatementFactory exec_factory_;
        runtime::Closure declared_classes_;
        std::unordered_map<string, InternalObjectCreator> internal_classes_;
        parse::ParseContext& parse_context_;
    }; // class Parser
}  // namespace

int parse::ParseContext::current_type_id = CLASS_AREA_IDENTS;

ParseError::ParseError(ThrowMessageNumber throw_message_number) :
    runtime_error(ThrowMessages::GetThrowText(throw_message_number))
{}

parse::ParseContext::~ParseContext()
{
    if (is_auto_deallocate_)
        DeallocateGlobalResources();
}

void parse::ParseContext::DeallocateGlobalResources()
{
    #if defined (_WIN64) || defined(_WIN32)
        for (HMODULE hmodule : dll_list_)
            FreeLibrary(hmodule);
    #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
        for (void* hmodule : dll_list_)
            dlclose(hmodule);
    #else

    #endif
    dll_list_.clear();
}

LoadLibraryDefine parse::TrivialParseContext::GetLoadLibraryDesc(const string& library_name) const
{
    #if defined (_WIN64) || defined(_WIN32)
        string standart_lib_extension = ".dll"s;
    #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
        string standart_lib_extension = ".so"s;
    #else
        string standart_lib_extension = ".dxe"s;
    #endif

    if (!library_name.size())
        return {};

    if (GetStemExt(library_name).second.size())
        return library_name;
    else
        return library_name + standart_lib_extension;
}

CplxParsedProgram::CplxParsedProgram() :
    parse_context(std::make_unique<parse::TrivialParseContext>()), closure(std::make_unique<runtime::Closure>())
{}

CplxParsedProgram::~CplxParsedProgram()
{
    // Порядок уничтожения активов (компонент) программы имеет значение. Поэтому данный деструктор освободит их
    // в безопасном порядке.
    // Сначала уничтожаем само дерево программы. На момент его уничтожения все контексты (разборочный и
    // исполнительский) должны ещё существовать.
    program.reset();
    // Затем можно разрушить лексический разборщик.
    lexer.reset();
    // Затем - таблицу символов.
    closure.reset();
    // После - исполнительский контекст.
    context.reset();
    // И, наконец, разборочный контекст. Он уничтожается последним, так как содержит метаданные, который могут
    // использовать все иные активы комплекса программы.
    try
    {
        if (parse_context)
            parse_context->DeallocateGlobalResources();
    }
    catch (...)
    {}
    parse_context.reset();
}

CplxParsedProgram& CplxParsedProgram::SetLexer(parse::Lexer&& p_lexer)
{
    lexer = std::make_unique<parse::Lexer>(std::move(p_lexer));
    return *this;
}

CplxParsedProgram& CplxParsedProgram::SetClosure(runtime::Closure&& p_closure)
{
    closure = std::make_unique<runtime::Closure>(std::move(p_closure));
    return *this;
}

// Определение функции синтаксического анализа исходного текста МУФЛОН-программы.
void ParseProgram(CplxParsedProgram& cplx_program)
{
    cplx_program.program = Parser(*cplx_program.lexer, *cplx_program.parse_context).ParseProgram();
}

// Определения функции исполнения 
runtime::ObjectHolder ExecuteProgram(CplxParsedProgram& cplx_program)
{
    // Аргумент program->parse_context ДОЛЖЕН совпадать с тем, который использовался при разборе исполняемой программы
    // cplx_program.program какой-либо функцией ParseProgram().
    if (!cplx_program.IsParsed())
        return runtime::ObjectHolder::None();
    return cplx_program.program->Execute(*cplx_program.closure, *cplx_program.context);
}
