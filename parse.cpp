#include "parse.h"

#include "lexer.h"
#include "statement.h"
#include "throw_messages.h"

using namespace std;
using runtime::ThrowMessageNumber;
using runtime::ThrowMessages;

namespace TokenType = parse::token_type;

namespace
{
    bool operator==(const parse::Token& token, char c)
    {
        const auto* p = token.TryAs<TokenType::Char>();
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
        using InternalObjectCreator = function<unique_ptr<ast::Statement>
                                (std::vector<std::unique_ptr<ast::Statement>> args)>;

        explicit Parser(parse::Lexer& lexer) : lexer_(lexer), exec_factory_(lexer_)
        {
            internal_classes_["array"s] = ast::CreateArray;
            internal_classes_["map"s] = ast::CreateMap;
            internal_classes_["math"s] = ast::CreateMath;
        }

        // Program -> eps
        //          | Statement \n Program
        unique_ptr<ast::Statement> ParseProgram()
        {
            auto result = exec_factory_.Create(ast::Compound());
            while (!lexer_.CurrentToken().Is<TokenType::Eof>())
                result->AddStatement(ParseStatement());

            return result;
        }

    private:
        // Suite -> NEWLINE INDENT (Statement)+ DEDENT
        unique_ptr<ast::Statement> ParseSuite()  // NOLINT
        {
            lexer_.Expect<TokenType::Newline>();
            lexer_.ExpectNext<TokenType::Indent>();

            lexer_.NextToken();

            auto result = exec_factory_.Create(ast::Compound());
            while (!lexer_.CurrentToken().Is<TokenType::Dedent>())
                result->AddStatement(ParseStatement());  // NOLINT

            lexer_.Expect<TokenType::Dedent>();
            lexer_.NextToken();

            return result;
        }

        // Methods -> [def id(Params) : Suite]*
        vector<runtime::Method> ParseMethods()  // NOLINT
        {
            vector<runtime::Method> result;

            while (lexer_.CurrentToken().Is<TokenType::Def>())
            {
                runtime::Method m;

                m.name = lexer_.ExpectNext<TokenType::Id>().value;
                lexer_.ExpectNext<TokenType::Char>('(');

                if (lexer_.NextToken().Is<TokenType::Id>())
                {
                    m.formal_params.push_back(lexer_.Expect<TokenType::Id>().value);
                    while (lexer_.NextToken() == ',')
                        m.formal_params.push_back(lexer_.ExpectNext<TokenType::Id>().value);
                }

                lexer_.Expect<TokenType::Char>(')');
                lexer_.ExpectNext<TokenType::Char>(':');
                lexer_.NextToken();

                m.body = exec_factory_.Create(ast::MethodBody(ParseSuite()));  // NOLINT

                result.push_back(std::move(m));
            }
            return result;
        }

        // ClassDefinition -> Id ['(' Id ')'] : new_line indent MethodList dedent
        unique_ptr<ast::Statement> ParseClassDefinition()  // NOLINT
        {
            string class_name = lexer_.Expect<TokenType::Id>().value;

            lexer_.NextToken();

            const runtime::Class* base_class = nullptr;
            if (lexer_.CurrentToken() == '(')
            {
                auto name = lexer_.ExpectNext<TokenType::Id>().value;
                lexer_.ExpectNext<TokenType::Char>(')');
                lexer_.NextToken();

                auto it = declared_classes_.find(name);
                if (it == declared_classes_.end())
                    exec_factory_.ThrowParseError(
                        ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_BASE_CLASS) +
                        name + ThrowMessages::GetThrowText(ThrowMessageNumber::THRM_NOT_FOUND_FOR_CLASS)
                        + class_name);

                base_class = static_cast<const runtime::Class*>(it->second.Get());  // NOLINT
            }

            lexer_.Expect<TokenType::Char>(':');
            lexer_.ExpectNext<TokenType::Newline>();
            lexer_.ExpectNext<TokenType::Indent>();
            lexer_.ExpectNext<TokenType::Def>();
            vector<runtime::Method> methods = ParseMethods();  // NOLINT

            lexer_.Expect<TokenType::Dedent>();
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
            vector<string> result(1, lexer_.Expect<TokenType::Id>().value);

            while (lexer_.NextToken() == '.')
                result.push_back(lexer_.ExpectNext<TokenType::Id>().value);

            return result;
        }

        //  AssgnOrCall -> DottedIds = Expr
        //               | DottedIds '(' ExprList ')'
        //               | DottedIds '(' ExprList ')' = Expr
        unique_ptr<ast::Statement> ParseAssignmentOrCall()
        {
            lexer_.Expect<TokenType::Id>();

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
            lexer_.Expect<TokenType::Char>('(');
            lexer_.NextToken();

            if (id_list.empty())
                exec_factory_.ThrowParseError(ThrowMessages::GetThrowText(
                    ThrowMessageNumber::THRM_NOT_SUPPORT_FREE_FUNCTION) + last_name);

            vector<unique_ptr<ast::Statement>> args;
            if (lexer_.CurrentToken() != ')')
                args = ParseTestList();

            lexer_.Expect<TokenType::Char>(')');
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
                char op = lexer_.CurrentToken().As<TokenType::Char>().value;
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
                char op = lexer_.CurrentToken().As<TokenType::Char>().value;
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
                lexer_.Expect<TokenType::Char>(')');
                lexer_.NextToken();
                return result;
            }

            if (lexer_.CurrentToken() == '-')
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Mult(ParseMult(), exec_factory_.Create(ast::NumericConst(-1))));
            }

            const auto* int_num_ptr = lexer_.CurrentToken().TryAs<TokenType::NumberInt>();
            const auto* double_num_ptr = lexer_.CurrentToken().TryAs<TokenType::NumberDouble>();
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

            if (const auto* str = lexer_.CurrentToken().TryAs<TokenType::String>())
            {
                string result = str->value;
                lexer_.NextToken();
                return exec_factory_.Create(ast::StringConst(std::move(result)));
            }

            if (lexer_.CurrentToken().Is<TokenType::True>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::BoolConst(runtime::Bool(true)));
            }

            if (lexer_.CurrentToken().Is<TokenType::False>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::BoolConst(runtime::Bool(false)));
            }

            if (lexer_.CurrentToken().Is<TokenType::None>())
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

                lexer_.Expect<TokenType::Char>(')');
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
            lexer_.Expect<TokenType::If>();
            lexer_.NextToken();

            auto condition = ParseTest();

            lexer_.Expect<TokenType::Char>(':');
            lexer_.NextToken();

            auto if_body = ParseSuite();

            unique_ptr<ast::Statement> else_body;
            if (lexer_.CurrentToken().Is<TokenType::Else>())
            {
                lexer_.ExpectNext<TokenType::Char>(':');
                lexer_.NextToken();
                else_body = ParseSuite();
            }

            return exec_factory_.Create(ast::IfElse(std::move(condition), std::move(if_body),
                                            std::move(else_body)));
        }

        // Condition -> while LogicalExpr: Suite
        unique_ptr<ast::Statement> ParseWhileCondition()  // NOLINT
        {
            lexer_.Expect<TokenType::While>();
            lexer_.NextToken();

            auto condition = ParseTest();

            lexer_.Expect<TokenType::Char>(':');
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
            while (lexer_.CurrentToken().Is<TokenType::Or>())
            {
                lexer_.NextToken();
                result = exec_factory_.Create(ast::Or(std::move(result), ParseAndTest()));
            }
            return result;
        }

        unique_ptr<ast::Statement> ParseAndTest()  // NOLINT
        {
            auto result = ParseNotTest();
            while (lexer_.CurrentToken().Is<TokenType::And>())
            {
                lexer_.NextToken();
                result = exec_factory_.Create(ast::And(std::move(result), ParseNotTest()));
            }
            return result;
        }

        unique_ptr<ast::Statement> ParseNotTest()  // NOLINT
        {
            if (lexer_.CurrentToken().Is<TokenType::Not>())
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
            if (tok.Is<TokenType::Eq>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::Equal, std::move(result),
                                                    ParseExpression()));
            }
            if (tok.Is<TokenType::NotEq>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::NotEqual, std::move(result),
                                                    ParseExpression()));
            }
            if (tok.Is<TokenType::LessOrEq>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Comparison(runtime::LessOrEqual, std::move(result),
                                                    ParseExpression()));
            }
            if (tok.Is<TokenType::GreaterOrEq>())
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

                if (tok.Is<TokenType::Class>())
                {
                    lexer_.NextToken();
                    return ParseClassDefinition();  // NOLINT
                }
                else if (tok.Is<TokenType::If>())
                {
                    return ParseCondition();
                }
                else if (tok.Is<TokenType::While>())
                {
                    return ParseWhileCondition();
                }

                auto result = ParseSimpleStatement();
                lexer_.Expect<TokenType::Newline>();
                lexer_.NextToken();
                if (result.has_value())
                    return move(result.value());
            }
        }

        // StatementBody -> return Expression
        //               | print ExpressionList
        //               | break
        //               | continue
        //               | AssignmentOrCall
        std::optional<unique_ptr<ast::Statement>> ParseSimpleStatement()
        {
            const auto& tok = lexer_.CurrentToken();

            if (tok.Is<TokenType::Import>())
            {
                lexer_.NextToken();
                vector<parse::Token> args;
                if (!lexer_.CurrentToken().Is<TokenType::Newline>())
                    args = ParseTokenList();
                ProcessImportLibrary(move(args));
                return nullopt;
            }

            if (tok.Is<TokenType::Return>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Return(ParseTest()));
            }

            if (tok.Is<TokenType::ReturnPtr>())
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

            if (tok.Is<TokenType::Print>())
            {
                lexer_.NextToken();
                vector<unique_ptr<ast::Statement>> args;
                if (!lexer_.CurrentToken().Is<TokenType::Newline>())
                    args = ParseTestList();

                return exec_factory_.Create(ast::Print(std::move(args)));
            }


            if (tok.Is<TokenType::Break>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Break());
            }        
        

            if (tok.Is<TokenType::Continue>())
            {
                lexer_.NextToken();
                return exec_factory_.Create(ast::Continue());
            }
        
            return ParseAssignmentOrCall();
        }

        void ProcessImportLibrary(vector<parse::Token> args)
        {
            string library_filename, library_alias;
            if (args.size() != 1 && args.size() != 2)
                exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INCORRECT_TOKEN_LIST);
            for (parse::Token& current_token : args)
            {
                if (!current_token.Is<TokenType::String>())
                    exec_factory_.ThrowParseError(ThrowMessageNumber::THRM_INCORRECT_TOKEN_LIST);
            }

            library_filename = args[0].As<TokenType::String>().value;
            if (args.size() == 2)
                library_alias = args[1].As<TokenType::String>().value;
        }

        vector<parse::Token> ParseTokenList()
        {
            vector<parse::Token> result;
            parse::Token previous_token = parse::token_type::None{}, current_token;

            while (true)
            {
                current_token = lexer_.CurrentToken();
                if (current_token.Is<TokenType::Newline>())
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
                    if (previous_token.Is<TokenType::None>() &&
                        (current_token.Is<TokenType::NumberInt>() ||
                            current_token.Is<TokenType::NumberDouble>() ||
                            current_token.Is<TokenType::String>() ||
                            current_token.Is<TokenType::True>() ||
                            current_token.Is<TokenType::False>()))
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
    }; // class Parser
}  // namespace

ParseError::ParseError(ThrowMessageNumber throw_message_number) :
    runtime_error(ThrowMessages::GetThrowText(throw_message_number))
{}

unique_ptr<runtime::Executable> ParseProgram(parse::Lexer& lexer)
{
    return Parser{lexer}.ParseProgram();
}
