
#include "parse.h"
#include "lexer.h"
#include "statement.h"
#include "throw_messages.h"

using namespace std;
using runtime::ThrowMessageNumber;
using runtime::ThrowMessages;

namespace ITokenType = parse::token_type;

namespace
{
    pair<string, string> GetStemExt(const string& filename)
    {
        int rev_slash_pos = filename.find_last_of('\\');
        if (rev_slash_pos == string::npos)
            rev_slash_pos = -1;
        int slash_pos = filename.find_last_of('/');
        if (slash_pos == string::npos)
            slash_pos = -1;
        int semi_colon_pos = filename.find_last_of(':');
        if (semi_colon_pos == string::npos)
            semi_colon_pos = -1;
        int path_margin = max(rev_slash_pos, slash_pos);
        path_margin = max(path_margin, semi_colon_pos);

        int point_pos = filename.find_last_of('.');
        if (point_pos == string::npos || point_pos <= path_margin)
            point_pos = filename.size();

        string stem = filename.substr(path_margin + 1, point_pos - path_margin - 1);
        string ext;
        if (point_pos < filename.size())
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
            return result;
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

        [[noreturn]] void ThrowParseError(runtime::ThrowMessageNumber msg_num)
        {
            string command_desc = to_string(lexer_.GetCurrentCommandDesc().module_id) + "("s +
                to_string(lexer_.GetCurrentCommandDesc().module_string_number) + "):"s;
            throw ParseError(command_desc + runtime::ThrowMessages::GetThrowText(msg_num));
        }

    private:
        const parse::Lexer& lexer_;
    };

    class Parser
    {
    public:
        explicit Parser(parse::Lexer& lexer, parse::ParseContext& parse_context) :
            lexer_(lexer), exec_factory_(lexer_), parse_context_(parse_context)
        {
            internal_classes_["array"s] = ast::CreateArray;
            internal_classes_["map"s] = ast::CreateMap;
            internal_classes_["math"s] = ast::CreateMath;
        }

        // Program -> eps
        //          | Statement \n Program
        unique_ptr<ast::Statement> ParseProgram()
        {
            auto result = exec_factory_.Create(ast::RootCompound());
            root_compound_ptr_ = result.get();
            while (!lexer_.CurrentToken().Is<ITokenType::Eof>())
                result->AddStatement(ParseStatement());

            return result;
        }

    private:
        // Suite -> NEWLINE INDENT (Statement)+ DEDENT
        unique_ptr<ast::Statement> ParseSuite()  // NOLINT
        {
            lexer_.Expect<ITokenType::Newline>();
            lexer_.ExpectNext<ITokenType::Indent>();

            lexer_.NextToken();

            auto result = exec_factory_.Create(ast::Compound());
            while (!lexer_.CurrentToken().Is<ITokenType::Dedent>())
                result->AddStatement(ParseStatement());  // NOLINT

            lexer_.Expect<ITokenType::Dedent>();
            lexer_.NextToken();

            return result;
        }

        // Methods -> [def id(Params) : Suite]*
        vector<runtime::Method> ParseMethods()  // NOLINT
        {
            vector<runtime::Method> result;

            while (lexer_.CurrentToken().Is<ITokenType::Def>())
            {
                runtime::Method m;

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

                m.body = exec_factory_.Create(ast::MethodBody(ParseSuite()));  // NOLINT

                result.push_back(std::move(m));
            }
            return result;
        }

        // ClassDefinition -> Id ['(' Id ')'] : new_line indent MethodList dedent
        unique_ptr<ast::Statement> ParseClassDefinition()  // NOLINT
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

                base_class = static_cast<const runtime::Class*>(it->second.Get());  // NOLINT
            }

            lexer_.Expect<ITokenType::Char>(':');
            lexer_.ExpectNext<ITokenType::Newline>();
            lexer_.ExpectNext<ITokenType::Indent>();
            lexer_.ExpectNext<ITokenType::Def>();
            vector<runtime::Method> methods = ParseMethods();  // NOLINT

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

        vector<string> ParseDottedIds()
        {
            vector<string> result(1, lexer_.Expect<ITokenType::Id>().value);

            while (lexer_.NextToken() == '.')
                result.push_back(lexer_.ExpectNext<ITokenType::Id>().value);

            return result;
        }

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
                if (id_list.empty())
                    return exec_factory_.Create(ast::Assignment(std::move(last_name), ParseTest()));

                return exec_factory_.Create(ast::FieldAssignment(ast::VariableValue{std::move(id_list)},
                                                                 std::move(last_name), ParseTest()));
            }
            lexer_.Expect<ITokenType::Char>('(');
            lexer_.NextToken();

            if (id_list.empty())
                exec_factory_.ThrowParseError(ThrowMessages::GetThrowText(
                    ThrowMessageNumber::THRM_NOT_SUPPORT_FREE_FUNCTION) + last_name);

            vector<unique_ptr<ast::Statement>> args;
            if (lexer_.CurrentToken() != ')')
                args = ParseTestList();

            lexer_.Expect<ITokenType::Char>(')');
            lexer_.NextToken();

            // Далее разбираются два варианта - вызов метода либо косвенное присваивание
            // (присваивание указателю, содержащемуся в возвращенном методом результате).
            if (lexer_.CurrentToken() == '=')
            { // После вызова метода следует лексема '=' - это косвенное присваивание
                lexer_.NextToken();
                return exec_factory_.Create(ast::IndirectAssignment(exec_factory_.Create(
                        ast::VariableValue(std::move(id_list))),
                        std::move(last_name), std::move(args), ParseTest()));
            }
            else
            { // Вызов метода последняя лексема строки - имеем дело с простым вызовом метода
                return exec_factory_.Create(ast::MethodCall(exec_factory_.Create(ast::VariableValue(std::move(id_list))),
                    std::move(last_name), std::move(args)));
            }
        }

        // Expr -> Adder ['+'/'-' Adder]*
        unique_ptr<ast::Statement> ParseExpression()  // NOLINT
        {
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

        // Adder -> Mult ['*'/'/' Mult]*
        unique_ptr<ast::Statement> ParseAdder()  // NOLINT
        {
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
        //       | STRING
        //       | NONE
        //       | TRUE
        //       | FALSE
        //       | DottedIds '(' ExprList ')'
        //       | DottedIds
        unique_ptr<ast::Statement> ParseMult()  // NOLINT
        {
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
                // various calls
                vector<unique_ptr<ast::Statement>> args;
                if (lexer_.NextToken() != ')')
                    args = ParseTestList();

                lexer_.Expect<ITokenType::Char>(')');
                lexer_.NextToken();

                auto method_name = names.back();
                names.pop_back();

                if (!names.empty())
                {
                    return exec_factory_.Create(ast::MethodCall(
                        exec_factory_.Create(ast::VariableValue(std::move(names))), std::move(method_name),
                        std::move(args)));
                }

                try
                {
                    if (auto it = internal_classes_.find(method_name); it != internal_classes_.end())
                    {
                        unique_ptr<ast::Statement> internal_class_ptr = (it->second)(std::move(args));
                        exec_factory_.AddCommandDesc(internal_class_ptr.get());
                        return internal_class_ptr;
                    }
                }
                catch (ParseError& parse_error)
                {
                    exec_factory_.ThrowParseError(parse_error.what());
                }

                if (auto it = declared_classes_.find(method_name); it != declared_classes_.end())
                {
                    return exec_factory_.Create(ast::NewInstance(
                        static_cast<const runtime::Class&>(*it->second), std::move(args)));
                }
            
                if (method_name == "str"sv)
                {
                    if (args.size() != 1)
                        exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_STR_HAS_ONE_PARAM);
                
                    return exec_factory_.Create(ast::Stringify(std::move(args.front())));
                }
                exec_factory_.ThrowParseError(ThrowMessages::GetThrowText(
                    ThrowMessageNumber::THRM_UNKNOWN_METHOD_CALL) + method_name + "()"s);
            }
            return exec_factory_.Create(ast::VariableValue(std::move(names)));
        }

        vector<unique_ptr<ast::Statement>> ParseTestList()  // NOLINT
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

        // Condition -> if LogicalExpr: Suite [else: Suite]
        unique_ptr<ast::Statement> ParseCondition()  // NOLINT
        {
            lexer_.Expect<ITokenType::If>();
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
                                            std::move(else_body)));
        }

        // Condition -> while LogicalExpr: Suite
        unique_ptr<ast::Statement> ParseWhileCondition()  // NOLINT
        {
            lexer_.Expect<ITokenType::While>();
            lexer_.NextToken();

            auto condition = ParseTest();

            lexer_.Expect<ITokenType::Char>(':');
            lexer_.NextToken();

            auto while_body = ParseSuite();

            return exec_factory_.Create(ast::While(std::move(condition), std::move(while_body)));
        }

        // LogicalExpr -> AndTest [OR AndTest]
        // AndTest -> NotTest [AND NotTest]
        // NotTest -> [NOT] NotTest
        //          | Comparison
        unique_ptr<ast::Statement> ParseTest()  // NOLINT
        {
            auto result = ParseAndTest();
            while (lexer_.CurrentToken().Is<ITokenType::Or>())
            {
                lexer_.NextToken();
                result = exec_factory_.Create(ast::Or(std::move(result), ParseAndTest()));
            }
            return result;
        }

        unique_ptr<ast::Statement> ParseAndTest()  // NOLINT
        {
            auto result = ParseNotTest();
            while (lexer_.CurrentToken().Is<ITokenType::And>())
            {
                lexer_.NextToken();
                result = exec_factory_.Create(ast::And(std::move(result), ParseNotTest()));
            }
            return result;
        }

        unique_ptr<ast::Statement> ParseNotTest()  // NOLINT
        {
            if (lexer_.CurrentToken().Is<ITokenType::Not>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Not(ParseNotTest()));  // NOLINT
            }
            return ParseComparison();
        }

        // Comparison -> Expr [COMP_OP Expr]
        unique_ptr<ast::Statement> ParseComparison()  // NOLINT
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
        unique_ptr<ast::Statement> ParseStatement()  // NOLINT
        {
            while (true)
            {
                const auto& tok = lexer_.CurrentToken();

                if (tok.Is<ITokenType::Class>())
                {
                    lexer_.NextToken();
                    return ParseClassDefinition();  // NOLINT
                }
                else if (tok.Is<ITokenType::If>())
                {
                    return ParseCondition();
                }
                else if (tok.Is<ITokenType::While>())
                {
                    return ParseWhileCondition();
                }

                auto result = ParseSimpleStatement();
                lexer_.Expect<ITokenType::Newline>();
                lexer_.NextToken();
                if (result.has_value())
                    return move(result.value());
            }
        }

        // Кроме команд периода исполнения (print, break, и. т. д.), здесь также
        // обрабатываются директивы времени трансляции (например, import).
        // StatementBody -> return Expression
        //               | print ExpressionList
        //               | break
        //               | continue
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

            if (tok.Is<ITokenType::ReturnPtr>())
            {
                lexer_.NextToken();
                auto test_result = ParseTest();
                if (typeid(*test_result) != typeid(ast::VariableValue))
                {
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_POINTER_RET_TO_VAL_DENIED);
                }
                else
                {
                    ast::VariableValue& result_ref = static_cast<ast::VariableValue&>(*test_result);
                    std::vector<std::string> dotted_ids = result_ref.GetDottedIds();
                    if (!dotted_ids.size() || dotted_ids[0] != "self"sv)
                        exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_POINTER_RET_TOL_LOCAL_VAR_DENIED);
                    else
                        return exec_factory_.Create(ast::ReturnPtr(move(dotted_ids)));
                }
            }

            if (tok.Is<ITokenType::Print>())
            {
                lexer_.NextToken();
                vector<unique_ptr<ast::Statement>> args;
                if (!lexer_.CurrentToken().Is<ITokenType::Newline>())
                    args = ParseTestList();

                return exec_factory_.Create(ast::Print(std::move(args)));
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
        
            return ParseAssignmentOrCall();
        }

        using PluginListType = vector<pair<string, string>>;
        using FuncGetPluginList = PluginListType*();
        static constexpr char LOAD_PLUGIN_LIST_NAME[] = "LoadPluginList";
        // Пробуем загружать втыкало из разделяемой библиотеки.
        #if defined (_WIN64) || defined(_WIN32)
            // Загрузка .DLL для винды
            void LoadImportLibrary(const string& library_filename, const string& library_alias)
            {
                #define WCHAR_FILENAME_SIZE 2048
                wchar_t wchar_buffer[WCHAR_FILENAME_SIZE];
                if (mbstowcs(wchar_buffer, library_filename.c_str(), WCHAR_FILENAME_SIZE - 1) ==
                    static_cast<size_t>(-1))
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INVALID_IMPORT_FILENAME);

                HMODULE hAddonDll = LoadLibraryW(wchar_buffer);
                if (!hAddonDll || hAddonDll == INVALID_HANDLE_VALUE)
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_DYNAMIC_LIBRARY_NOT_LOADED);

                FuncGetPluginList* load_plugin_list_proc = reinterpret_cast<FuncGetPluginList*>
                                                           (GetProcAddress(hAddonDll, LOAD_PLUGIN_LIST_NAME));
                if (!load_plugin_list_proc)
                {
                    FreeLibrary(hAddonDll);
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_LOAD_PLUGIN_LIST_NOT_FOUND);
                }

                for (const auto& current_plugin_pair : *load_plugin_list_proc())
                {
                    FuncInternalObjectCreator* create_plugin_proc = reinterpret_cast<FuncInternalObjectCreator*>
                                                    (GetProcAddress(hAddonDll, current_plugin_pair.first.c_str()));
                    if (!create_plugin_proc)
                    {
                        FreeLibrary(hAddonDll);
                        exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_CREATE_PLUGIN_NOT_FOUND);
                    }
                    internal_classes_[library_alias + "_"s + current_plugin_pair.second] = create_plugin_proc;
                    // Обеспечим также возможность обращения к первому классу втыкала без суффикса имени класса
                    if (!internal_classes_.count(library_alias))
                        internal_classes_[library_alias] = create_plugin_proc;
                }
                root_compound_ptr_->AddDLLEntry(hAddonDll);
                return;
            }
        #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
            // Здесь реализована загрузка .SO линукса/юникса
            void LoadImportLibrary(const string& library_filename, const string& library_alias)
            {
                return;
            }
        #else
            // Какие-то другие, неподдерживаемые варианты платформ
            void LoadImportLibrary(const string& library_filename, const string& library_alias)
            {
                return;
            }
        #endif

        void ProcessImportLibrary(vector<parse::Token> args, const parse::ParseContext& parse_context)
        {
            string library_filename, library_alias;
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
            // Пробуем загружить разделяемую библиотеку по имени library_filename.
            LoadLibraryDefine lib_desc = parse_context.GetLoadLibraryDesc(library_filename);
            if (holds_alternative<monostate>(lib_desc))
                return;
            if (holds_alternative<InternalObjectCreatorList>(lib_desc))
            { // Подсоединение втыкала, уже существующего в памяти.
                if (library_alias.empty())
                    library_alias = library_filename;

                for (auto& internal_object_creator_pair : get<InternalObjectCreatorList>(lib_desc))
                    internal_classes_[library_alias + "_"s + internal_object_creator_pair.first] =
                        internal_object_creator_pair.second;
                return;
            }
            // Далее будем пытыться загрузить втыкало из разделяемой библиотеки
            if (library_alias.empty())
                library_alias = GetStemExt(library_filename).first;
            LoadImportLibrary(get<string>(lib_desc), library_alias);
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
        unordered_map<string, InternalObjectCreator> internal_classes_;
        parse::ParseContext& parse_context_;
        ast::RootCompound* root_compound_ptr_ = nullptr;
    }; // class Parser
}  // namespace

ParseError::ParseError(ThrowMessageNumber throw_message_number) :
    runtime_error(ThrowMessages::GetThrowText(throw_message_number))
{}

class TrivialParseContext : public parse::ParseContext
{
public:
    LoadLibraryDefine GetLoadLibraryDesc(const string& library_name) const override
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
};

unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer)
{
    TrivialParseContext parse_context;
    return Parser(lexer, parse_context).ParseProgram();
}

parse::GlobalResourceInfo GetGlobalResourceList(const unique_ptr<runtime::Executable>& node)
{
    const ast::RootCompound* root_ptr = dynamic_cast<const ast::RootCompound*>(node.get());
    if (!root_ptr)
        throw ParseError(ThrowMessageNumber::THRM_NODE_NOT_ROOT);
    return root_ptr->GetGlobalResourceInfo();
}

void DeallocateGlobalResources(const parse::GlobalResourceInfo& global_resources)
{
    #if defined (_WIN64) || defined(_WIN32)
        for (HMODULE hmodule : global_resources.dll_list)
            FreeLibrary(hmodule);
    #elif defined(__unix__) || defined(__linux__) || defined(__USE_POSIX)
        for (void* hmodule : global_resources.dll_list)
            dlclose(hmodule);
    #else

    #endif    
}
