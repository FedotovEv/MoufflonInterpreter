#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse
{
    static const string special_symb = "<>=!"s;

    static const unordered_map<std::string, Token> keyword_tokens
        {{"class"s, token_type::Class{}},
         {"return"s, token_type::Return{}},
         {"return_ref"s, token_type::ReturnRef{}},
         {"if"s, token_type::If{}},
         {"else"s, token_type::Else{}},
         {"while"s, token_type::While{}},
         {"break"s, token_type::Break{}},
         {"continue"s, token_type::Continue{}},
         {"def"s, token_type::Def{}},
         {"print"s, token_type::Print{}},
         {"import"s, token_type::Import{}},
         {"include"s, token_type::Include{}},
         {"and"s, token_type::And{}},
         {"or"s, token_type::Or{}},
         {"not"s, token_type::Not{}},
         {"None"s, token_type::None{}},
         {"True"s, token_type::True{}},
         {"False"s, token_type::False{}}
        };

    static const unordered_map<std::string, Token> special_tokens
        {
         {"<"s, token_type::Char{'<'}},
         {">"s, token_type::Char{'>'}},
         {"="s, token_type::Char{'='}},
         {"!"s, token_type::Char{'!'}},
         {"=="s, token_type::Eq{}},
         {"!="s, token_type::NotEq{}},
         {"<="s, token_type::LessOrEq{}},
         {">="s, token_type::GreaterOrEq{}}
        };

    enum class TokenTypeId
    {
        TOKEN_UNDEFINED = 0,
        TOKEN_STRING,
        TOKEN_ID,
        TOKEN_NUMBER_INT,
        TOKEN_CHAR,
        TOKEN_NEWLINE,
        TOKEN_EOF,
        TOKEN_NUMBER_DOUBLE
    };
    
    void SkipToEndLine(LexerInputEx& input)
    {
        while (input)
            if (input.get() == '\n')
            {
                input.unget();
                break;
            }
    }
    
    string GetStringValue(LexerInputEx& input)
    {
        string result;
        char termin_symb = input.get();
        while (input)
        {
            char ch = input.get();
            if (!input || ch == termin_symb)
            {
                break;
            }
            else if (ch == '\n')
            {
                input.unget();
                break;
            }
            else if (ch == '\\')
            { // Экранирующая комбинация
                    char ch1 = input.get();
                    switch (ch1)
                    {
                        case 'r':
                            result += '\r';
                            break;
                        case 'n':
                            result += '\n';
                            break;
                        case 't':
                            result += '\t';
                            break;
                        case '\'':
                            result += '\'';
                            break;
                        case '"':
                            result += '"';
                            break;
                        default:
                            result += '\\';
                            input.unget();
                            break;
                    }
            }
            else
            {
                result += ch;
            }
        }
        return result;
    }

    string GetIdentString(LexerInputEx& input)
    {
        string result;
        while (input)
        {
            char ch = input.get();
            if (input)
            {
                if (!isalnum(ch) && ch != '_')
                {
                    input.unget();
                    break;
                }
                result += ch;
            }
        }
        return result;
    }

    pair<string, TokenTypeId> GetNumberString(LexerInputEx& input)
    {
        string result_str;
        TokenTypeId result_token_id = TokenTypeId::TOKEN_NUMBER_INT;
        bool is_continue_loop = true, is_exponent = false;

        while (input && is_continue_loop)
        {
            char ch = input.get();
            if (input)
            {
                if (!isdigit(ch))
                {
                    switch (ch)
                    {
                    case '.':
                        if (result_token_id == TokenTypeId::TOKEN_NUMBER_INT && !is_exponent)
                            result_token_id = TokenTypeId::TOKEN_NUMBER_DOUBLE;
                        else
                            is_continue_loop = false;
                        break;
                    case 'E':
                    case 'e':
                        if (result_token_id == TokenTypeId::TOKEN_NUMBER_INT)
                        {
                            result_token_id = TokenTypeId::TOKEN_NUMBER_DOUBLE;
                            is_exponent = true;
                        }
                        else
                        {
                            if (!is_exponent)
                                is_exponent = true;
                            else
                                is_continue_loop = false;
                        }
                        break;
                    default:
                        is_continue_loop = false;
                        break;
                    }
                }

                if (is_continue_loop)
                    result_str += ch;
                else
                    input.unget();
            }
        }
        return {result_str, result_token_id};
    }
    
    string GetChardSequence(LexerInputEx& input)
    {
        string result;
        while (input)
        {
            char ch = input.get();
            if (input)
            {
                if (special_symb.find(ch) == string::npos)
                {
                    input.unget();
                    break;
                }
                result += ch;
                if (!special_tokens.count(result))
                {
                    input.unget();
                    break;
                }
            }
        }
        return result;
    }

    int GoToTokenBegin(LexerInputEx& input)
    {
        int space_counter = 0;
        while (input.good())
        {
            char ch = input.get();
            if (input.good())
            {
                if (ch == '#')
                {
                    SkipToEndLine(input);
                }
                else if (ch == ' ')
                {
                    ++space_counter;                
                }
                else
                {
                    input.unget();
                    break;                
                }
            }
        }
        return space_counter;
    }
    
    pair<string, TokenTypeId> GetNextTokenPair(LexerInputEx& input)
    {
        char ch = input.get(); // Первый символ лексемы
        if (!input)
            return {""s, TokenTypeId::TOKEN_EOF};

        if (ch == '\n')
            return {""s, TokenTypeId::TOKEN_NEWLINE};

        if (ch == '\r')
        {
            input.get(); // После \r должна следовать \n - её нужно тоже удалить из потока
            return {""s, TokenTypeId::TOKEN_NEWLINE};
        }

        input.unget();

        if (ch == '\'' || ch == '"')
            return {GetStringValue(input), TokenTypeId::TOKEN_STRING};  // Это строка
        
        if (isalpha(ch) || ch =='_')
            return {GetIdentString(input), TokenTypeId::TOKEN_ID};  // Это идентификатор
 
        if (isdigit(ch))
            return GetNumberString(input);  // Это число, целое или с плавающей точкой
    
        if (special_symb.find(ch) != string::npos)
        {  // Это специальная символьная группа
            string tst_token = GetChardSequence(input);
            if (tst_token.size())
                return {tst_token, TokenTypeId::TOKEN_CHAR};
        }

        return {string(1, input.get()), TokenTypeId::TOKEN_UNDEFINED};
    }

    bool operator==(const Token& lhs, const Token& rhs)
    {
        using namespace token_type;

        if (lhs.index() != rhs.index())
            return false;    
        if (lhs.Is<Char>())
            return lhs.As<Char>().value == rhs.As<Char>().value;
        if (lhs.Is<NumberInt>())
            return lhs.As<NumberInt>().value == rhs.As<NumberInt>().value;
        if (lhs.Is<NumberDouble>())
            return abs(lhs.As<NumberDouble>().value - rhs.As<NumberDouble>().value) < ZERO_TOLERANCE;
        if (lhs.Is<String>())
            return lhs.As<String>().value == rhs.As<String>().value;
        if (lhs.Is<Id>())
            return lhs.As<Id>().value == rhs.As<Id>().value;
        return true;
    }

    bool operator!=(const Token& lhs, const Token& rhs)
    {
        return !(lhs == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const Token& rhs)
    {
        using namespace token_type;

    #define VALUED_OUTPUT(type) \
        if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(NumberInt);
        VALUED_OUTPUT(NumberDouble);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

    #undef VALUED_OUTPUT

    #define UNVALUED_OUTPUT(type) \
        if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(While);
        UNVALUED_OUTPUT(Break);
        UNVALUED_OUTPUT(Continue);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Import);
        UNVALUED_OUTPUT(Include);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

    #undef UNVALUED_OUTPUT

        return os << "Unknown token :("sv;
    }

    Lexer::Lexer(LexerInputEx& input) : input_(input),
                                        indent_amount_(0),
                                        indent_sent_(0),
                                        current_token_(token_type::Newline{})
    {
        is_input_need_delete_ = false;
        input_.SetCommandDescPtr(&current_command_desc_);
        input_.IncludeSwitchTo("");
        NextToken();
    }

    Lexer::Lexer(istream& input) : input_(*new SimpleLexerInputEx(input)),
                                   indent_amount_(0),
                                   indent_sent_(0),
                                   current_token_(token_type::Newline{})
    {
        is_input_need_delete_ = true;
        input_.SetCommandDescPtr(&current_command_desc_);
        input_.IncludeSwitchTo("");
        NextToken();
    }

    Lexer::~Lexer()
    {
        if (is_input_need_delete_)
            delete &input_;
    }

    const Token& Lexer::CurrentToken() const
    {
        return current_token_;
    }

    Token Lexer::NextToken()
    {
        if (indent_sent_ == indent_amount_)
        {
            int space_cnt = GoToTokenBegin(input_);
            if (current_token_ == token_type::Newline{})
            { // Первая лексема на новой строке
                while (true)
                { // Пропускаем все пустые строки, не содержащие никаких лексем, кроме NewLine
                    ++current_command_desc_.module_string_number;
                    char peek_chr = input_.peek();
                    if (!input_)
                    {
                        indent_amount_ = 0;
                        break;
                    }
                    else if (peek_chr == '\n')
                    {
                        input_.get();
                        space_cnt = GoToTokenBegin(input_);
                    }
                    else if (peek_chr == '\r')
                    {
                        input_.get();
                        input_.get(); // После \r должна следовать \n - её нужно тоже удалить из потока
                        space_cnt = GoToTokenBegin(input_);
                    }
                    else
                    {   // Если строка не пуста, измеряем её отступ для последующей посылки событий
                        indent_amount_ = space_cnt / SPACES_PER_INDENT_STEP;
                        break;
                    }
                }
            }
        }

        if (indent_sent_ < indent_amount_)
        {
            ++indent_sent_;
            current_token_ = token_type::Indent{};
            return current_token_;        
        }
        else if (indent_sent_ > indent_amount_)
        {
            --indent_sent_;
            current_token_ = token_type::Dedent{};
            return current_token_;
        }
     
        auto next_token = GetNextTokenPair(input_);
    
        switch (next_token.second)
        {
            case TokenTypeId::TOKEN_STRING: // Лексема-строка
                current_token_ = token_type::String{next_token.first};
                break;
            case TokenTypeId::TOKEN_ID: // Идентификатор или ключевое слово
                if (keyword_tokens.count(next_token.first))
                    // Лексема - ключевое слово
                    current_token_ = keyword_tokens.at(next_token.first);
                else
                    // Лексема - идентификатор (имя переменной)
                    current_token_ = token_type::Id{next_token.first};
                break;
            case TokenTypeId::TOKEN_NUMBER_INT: // Лексема-целое число
                current_token_ = token_type::NumberInt{stoi(next_token.first)};
                break;
            case TokenTypeId::TOKEN_NUMBER_DOUBLE:
                current_token_ = token_type::NumberDouble{stod(next_token.first)};
                break;
            case TokenTypeId::TOKEN_CHAR: //Лексема - специальная символьная группа
                if (special_tokens.count(next_token.first))
                    // Лексема - распознанный набор спецсимволов
                    current_token_ = special_tokens.at(next_token.first);
                else
                    // Лексема - нераспознанная комбинация из спецсимволов
                    current_token_ = token_type::Char{next_token.first[0]};
                break;        
            case TokenTypeId::TOKEN_NEWLINE: //Лексема завершения строки
                current_token_ = token_type::Newline{};
                break;
            case TokenTypeId::TOKEN_EOF: // Поток данных исчерпан. Возвращаем завершающую лексему Eof.
                if (current_token_ != token_type::Eof{} &&
                    current_token_ != token_type::Newline{} &&
                    current_token_ != token_type::Dedent{})
                    current_token_ = token_type::Newline{};
                else
                    current_token_ = token_type::Eof{};
                break;
            default:
                current_token_ = token_type::Char{next_token.first[0]};
        }    
        return current_token_;
    }
}  // namespace parse
