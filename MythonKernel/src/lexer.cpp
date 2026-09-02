#include "lexer.h"
#include "encodings.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>

using namespace std;

namespace parse
{
    static const string special_symb = "<>=!"s;

    static const unordered_map<std::string, Token> keyword_tokens
        {
            {"class"s, token_type::Class{}},
            // Лексемы операторов завершения или приостановки методов.
            {"return"s, token_type::Return{}},
            {"co_yield"s, token_type::CoYield{}},
            {"return_ref"s, token_type::ReturnRef{}},
            {"co_yield_ref"s, token_type::CoYieldRef{}},
            {"co_await"s, token_type::CoAwait{}},
            // Лексемы, связанные с обработкой условных операторов.
            {"if"s, token_type::If{}},
            {"elif"s, token_type::Elif{}},
            {"else"s, token_type::Else{}},
            {"while"s, token_type::While{}},
            {"break"s, token_type::Break{}},
            {"continue"s, token_type::Continue{}},
            //
            {"pass"s, token_type::Pass{}},
            {"del"s, token_type::Del{}},
            // Специальные лексемы обработки исключений.
            {"try"s, token_type::Try{}},
            {"except"s, token_type::Except{}},
            {"finally"s, token_type::Finally{}},
            {"as"s, token_type::As{}},
            {"raise"s, token_type::Raise{}},
            // Прочие негруппированные лексемы.
            {"def"s, token_type::Def{}},
            {"print"s, token_type::Print{}},
            {"import"s, token_type::Import{}},
            {"include"s, token_type::Include{}},
            {"and"s, token_type::And{}},
            {"or"s, token_type::Or{}},
            {"not"s, token_type::Not{}},
            {"xor"s, token_type::Xor{}},
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
            {">="s, token_type::GreaterOrEq{}},
            {"<<"s, token_type::ShiftLeft{}},
            {">>"s, token_type::ShiftRight{}}
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

    // Извлечение из потока input широкого символа, кодированного symb_count-значным (symb_count / 2)-байтовым шестнадцатеричным кодом.
    uint32_t HexSetToCharCode(Lexer* lexer, LexerInputEx& input, uint32_t symb_count)
    {
        static constexpr uint32_t BITS_PER_HEX_DIGIT = 4;

        uint32_t result = 0;
        uint32_t shift_factor = symb_count * BITS_PER_HEX_DIGIT;
        while (shift_factor > 0)
        {
            shift_factor -= BITS_PER_HEX_DIGIT;
            input.peek();
            if (!input.good())
                throw LexerError(CommandDescToString(lexer->GetCurrentCommandDesc()) + Lexer::LEXEM_PREMATURE_TERMINATED);

            char next_symb = input.get();
            if (isdigit(next_symb))
                result += (next_symb - 0x30) << shift_factor;
            else if (isxdigit(next_symb))
                result += (toupper(next_symb) - 'A' + 10) << shift_factor;
            else
                throw LexerError(CommandDescToString(lexer->GetCurrentCommandDesc()) + Lexer::HEX_DIGIT_AWAIT);
        }
        return result;
    }

    string GetStringValue(LexerInputEx& input, bool is_utf8_string, Lexer* lexer)
    {
        string result;
        char termin_symb = input.get();
        while (input)
        {
            char ch = input.get();
            if (!input || ch == termin_symb)
            { // Конец строковго литерала.
                break;
            }
            else if (ch == '\n')
            { // Также завершение тела литерала по окончанию текущей строки исходника.
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
                        case 'x':   // Символ, определённый двухзначным шестнадцатеричным кодом.
                            result += static_cast<unsigned char>(HexSetToCharCode(lexer, input, 2));
                            break;
                        case 'u':   // Широкий символ, кодированный четырёхзначным двухбайтовым шестнадцатеричным кодом.
                            if (is_utf8_string)
                                result += ConvSymbToUTF8(HexSetToCharCode(lexer, input, 4));
                            else
                                throw LexerError(CommandDescToString(lexer->GetCurrentCommandDesc()) + Lexer::WIDE_CHAR_IN_NARROW_STRING);
                            break;
                        case 'U':   // Широкий символ, кодированный восьмизначным четырёхбайтовым шестнадцатеричным кодом.
                            if (is_utf8_string)
                                result += ConvSymbToUTF8(HexSetToCharCode(lexer, input, 8));
                            else
                                throw LexerError(CommandDescToString(lexer->GetCurrentCommandDesc()) + Lexer::WIDE_CHAR_IN_NARROW_STRING);
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

    struct TokenDesc
    {
        string body;
        TokenTypeId id = TokenTypeId::TOKEN_UNDEFINED;
        intptr_t int_param = 0;
    };

    TokenDesc GetNumberString(LexerInputEx& input)
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
        return {.body = move(result_str), .id = result_token_id};
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

    // Функция ищет начало следующего жетона (пропуская все посторонние незначащие символы), одновременно подсчитывая
    // количество пробелов, находящихся перед таким жетоном и началом строки, в котором он расположен.
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

    TokenDesc GetNextTokenPair(LexerInputEx& input, Lexer* lexer)
    {
        char ch = input.get(); // Первый символ лексемы.
        if (!input)
            return {.id = TokenTypeId::TOKEN_EOF};

        if (ch == '\n')
            return {.id = TokenTypeId::TOKEN_NEWLINE};

        if (ch == '\r')
        {
            input.get(); // После \r должна следовать \n - её нужно тоже удалить из потока.
            return {.id = TokenTypeId::TOKEN_NEWLINE};
        }

        input.unget();

        if (ch == '\'' || ch == '"')
        { // Это строка (закавыченный строковый литерал).
            const SingleByteEncodingDesc* real_encoding = lexer->GetSourceEncoding();
            if (string string_prefix = lexer->GetStringPrefix(); !string_prefix.empty())
            {
                if (string_prefix == UTF_8_STRING_LITERAL)
                { // Префикс явно указывает на UTF-8-кодированную строку.
                    real_encoding = UTF_8_ENCODING;
                }
                else if (string_prefix == NARROW_STRING_LITERAL)
                { // Префикс узкой строки с неопределённой кодировкой.
                    real_encoding = NO_ENCODING;
                }
                else if (string_prefix.starts_with(NAMED_ENC_STRING_LITERAL))
                { // Префикс строки с определённой кодировкой, заданной хвостовой частью префикса.
                    string encoding_name = string_prefix.substr(NAMED_ENC_STRING_LITERAL.size());
                    if (int set_enc_id = FindEncoding(encoding_name); set_enc_id != NON_INDEXED_ENCODING_ID)
                        real_encoding = GetEncoding(set_enc_id);
                    else
                        throw LexerError(CommandDescToString(lexer->GetCurrentCommandDesc()) + Lexer::BAD_ENC_NAME);
                }
                else
                {
                    throw LexerError(CommandDescToString(lexer->GetCurrentCommandDesc()) + Lexer::BAD_PREFIX_VALUE);
                }
                lexer->ClearStringPrefix();
            }
            
            return {.body = GetStringValue(input, real_encoding == UTF_8_ENCODING, lexer), .id = TokenTypeId::TOKEN_STRING,
                    .int_param = reinterpret_cast<intptr_t>(real_encoding)};
        }
        
        if (isalpha(ch) || ch == '_')
        {  // Вероятно, это идентификатор. Но может быть и префиксом строкового литерала.
            string id_name = GetIdentString(input);
            char next_ch = input.peek();
            if (next_ch == '\'' || next_ch == '"')
            { // Это именно строковый префикс.
                lexer->SetStringPrefix(move(id_name));
                return GetNextTokenPair(input, lexer);  // Этот вызов вернёт нам строковый литерал, обработанный согласно указанному префиксу.
            }
            // Да, это идентификатор.
            return {.body = move(id_name), .id = TokenTypeId::TOKEN_ID};
        }
 
        if (isdigit(ch))
            return GetNumberString(input);  // Это число, целое или с плавающей точкой.
    
        if (special_symb.find(ch) != string::npos)
        {  // Это специальная символьная группа.
            string tst_token = GetChardSequence(input);
            if (tst_token.size())
                return {.body = move(tst_token), .id = TokenTypeId::TOKEN_CHAR};
        }

        return {.body = string(1, input.get()), .id = TokenTypeId::TOKEN_UNDEFINED};
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
        {
            const String& lhs_string_token = lhs.As<String>();
            const String& rhs_string_token = rhs.As<String>();
            if (lhs_string_token.encoding == rhs_string_token.encoding)
            { // Кодировки сравниваемых строк совпадают.
                if (lhs_string_token.encoding == UTF_8_ENCODING)
                { // Для Юникодных строк сравним только значащую часть их контейнеров.
                    size_t important_len = lhs_string_token.utf8_map.BytePosAfterEnd();
                    return important_len == rhs_string_token.utf8_map.BytePosAfterEnd() &&
                           lhs_string_token.value.substr(0, important_len) == rhs_string_token.value.substr(0, important_len);
                }
                else
                { // Для однобайтовых кодировок лексикографически сравниваем полные строки без учёта возможных отождествлений разных символов.
                    return lhs_string_token.value == rhs_string_token.value;
                }
            }
            else
            { // Кодировки сравниваемых строк различаются. Перед сравнением транскодируем оба аргумента в Юникод.
                if (lhs_string_token.encoding == NO_ENCODING || rhs_string_token.encoding == NO_ENCODING)
                    return false;   // В таком случае преобразовать оба аргумента к Юникоду не получится, сравнить их невозможно, поэтому возвратим "ЛОЖЬ".
                const string *lhs_transcoded, *rhs_transcoded;
                const UTF8Map *lhs_utf8_map, *rhs_utf8_map;
                tuple<string, UTF8Map, UTF8Error> lhs_transcode_tuple, rhs_transcode_tuple;
                // Если первая из сравниваемых строк пока не Юникод, конвертируем её туда.
                if (lhs_string_token.encoding != UTF_8_ENCODING)
                {
                    lhs_transcode_tuple = TranscodeToUTF8Ex(lhs_string_token.value, lhs_string_token.encoding->to_utf8);
                    lhs_transcoded = &get<string>(lhs_transcode_tuple);
                    lhs_utf8_map = &get<UTF8Map>(lhs_transcode_tuple);
                }
                else
                {
                    lhs_transcoded = &lhs_string_token.value;
                    lhs_utf8_map = &lhs_string_token.utf8_map;
                }
                // Если вторая из сравниваемых строк не находится в Юникоде, также конвертируем её туда.
                if (rhs_string_token.encoding != UTF_8_ENCODING)
                {
                    rhs_transcode_tuple = TranscodeToUTF8Ex(rhs_string_token.value, rhs_string_token.encoding->to_utf8);
                    rhs_transcoded = &get<string>(rhs_transcode_tuple);
                    rhs_utf8_map = &get<UTF8Map>(rhs_transcode_tuple);
                }
                else
                {
                    rhs_transcoded = &rhs_string_token.value;
                    rhs_utf8_map = &rhs_string_token.utf8_map;
                }
                // При сравнении Юникодных строк учитываем только значащую часть их контейнера.
                size_t important_len = lhs_utf8_map->BytePosAfterEnd();
                return important_len == rhs_utf8_map->BytePosAfterEnd() &&
                       lhs_transcoded->substr(0, important_len) == rhs_transcoded->substr(0, important_len);
            }
        }
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
        UNVALUED_OUTPUT(CoYield);
        UNVALUED_OUTPUT(ReturnRef);
        UNVALUED_OUTPUT(CoYieldRef);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Elif);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(While);
        UNVALUED_OUTPUT(Break);
        UNVALUED_OUTPUT(Continue);
        UNVALUED_OUTPUT(Pass);
        UNVALUED_OUTPUT(Del);
        UNVALUED_OUTPUT(Try);
        UNVALUED_OUTPUT(Except);
        UNVALUED_OUTPUT(Finally);
        UNVALUED_OUTPUT(As);
        UNVALUED_OUTPUT(Raise);
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
        UNVALUED_OUTPUT(Xor);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(ShiftLeft);
        UNVALUED_OUTPUT(ShiftRight);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

    #undef UNVALUED_OUTPUT

        return os << "Unknown token :("sv;
    }

    std::string TokenTypeToString(const Token& rhs)
    {
        using namespace token_type;

        #define STRINGIZE_TOKEN_TYPE(type) \
            if (auto p = rhs.TryAs<type>()) return '{' + std::string(#type) + '}';

        STRINGIZE_TOKEN_TYPE(NumberInt);
        STRINGIZE_TOKEN_TYPE(NumberDouble);
        STRINGIZE_TOKEN_TYPE(Id);
        STRINGIZE_TOKEN_TYPE(String);
        STRINGIZE_TOKEN_TYPE(Char);
        STRINGIZE_TOKEN_TYPE(Class);
        STRINGIZE_TOKEN_TYPE(Return);
        STRINGIZE_TOKEN_TYPE(CoYield);
        STRINGIZE_TOKEN_TYPE(ReturnRef);
        STRINGIZE_TOKEN_TYPE(CoYieldRef);
        STRINGIZE_TOKEN_TYPE(If);
        STRINGIZE_TOKEN_TYPE(Elif);
        STRINGIZE_TOKEN_TYPE(Else);
        STRINGIZE_TOKEN_TYPE(While);
        STRINGIZE_TOKEN_TYPE(Break);
        STRINGIZE_TOKEN_TYPE(Continue);
        STRINGIZE_TOKEN_TYPE(Pass);
        STRINGIZE_TOKEN_TYPE(Del);
        STRINGIZE_TOKEN_TYPE(Try);
        STRINGIZE_TOKEN_TYPE(Except);
        STRINGIZE_TOKEN_TYPE(Finally);
        STRINGIZE_TOKEN_TYPE(As);
        STRINGIZE_TOKEN_TYPE(Raise);
        STRINGIZE_TOKEN_TYPE(Def);
        STRINGIZE_TOKEN_TYPE(Newline);
        STRINGIZE_TOKEN_TYPE(Print);
        STRINGIZE_TOKEN_TYPE(Import);
        STRINGIZE_TOKEN_TYPE(Include);
        STRINGIZE_TOKEN_TYPE(Indent);
        STRINGIZE_TOKEN_TYPE(Dedent);
        STRINGIZE_TOKEN_TYPE(And);
        STRINGIZE_TOKEN_TYPE(Or);
        STRINGIZE_TOKEN_TYPE(Not);
        STRINGIZE_TOKEN_TYPE(Xor);
        STRINGIZE_TOKEN_TYPE(Eq);
        STRINGIZE_TOKEN_TYPE(NotEq);
        STRINGIZE_TOKEN_TYPE(LessOrEq);
        STRINGIZE_TOKEN_TYPE(GreaterOrEq);
        STRINGIZE_TOKEN_TYPE(ShiftLeft);
        STRINGIZE_TOKEN_TYPE(ShiftRight);
        STRINGIZE_TOKEN_TYPE(None);
        STRINGIZE_TOKEN_TYPE(True);
        STRINGIZE_TOKEN_TYPE(False);
        STRINGIZE_TOKEN_TYPE(Eof);

        #undef STRINGIZE_TOKEN_TYPE

        return "{Unknown token type}";
    }

    Lexer::Lexer(LexerInputEx& input) : input_(input),
                                        indent_amount_(0),
                                        indent_sent_(0),
                                        current_token_(token_type::Newline{})
    {
        is_input_need_delete_ = false;
        input_.SetCommandDescPtr(&current_command_desc_);
        input_.IncludeSwitchTo("");
        --current_command_desc_.module_string_number;
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
        --current_command_desc_.module_string_number;
        NextToken();
    }

    Lexer::Lexer(const Lexer& other) :
        input_(other.input_), indent_amount_(other.indent_amount_), indent_sent_(other.indent_sent_), current_token_(other.current_token_),
        current_command_desc_(other.current_command_desc_), is_input_need_delete_(other.is_input_need_delete_)
    {
        const_cast<Lexer&>(other).is_input_need_delete_ = false;
    }
    
    Lexer::Lexer(Lexer&& other) noexcept :
        input_(other.input_), indent_amount_(other.indent_amount_), indent_sent_(other.indent_sent_), current_token_(other.current_token_),
        current_command_desc_(other.current_command_desc_), is_input_need_delete_(other.is_input_need_delete_)
    {
        other.current_command_desc_ = runtime::ProgramCommandDescriptor{};
        other.is_input_need_delete_ = false;
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
     
        TokenDesc next_token = GetNextTokenPair(input_, this);
    
        switch (next_token.id)
        {
            case TokenTypeId::TOKEN_STRING: // Лексема-строка
            {
                token_type::String string_token
                {
                    .value = next_token.body,
                    .encoding = reinterpret_cast<const SingleByteEncodingDesc*>(next_token.int_param)
                };
                // Для Юникодных строк сразу составим карту размещения символов в строке.
                if (string_token.encoding == UTF_8_ENCODING)
                    string_token.utf8_map = BuildUTF8Map(string_token.value).first;
                else
                    string_token.utf8_map.Clear();

                current_token_ = move(string_token);
                break;
            }
            case TokenTypeId::TOKEN_ID: // Идентификатор или ключевое слово
                if (keyword_tokens.count(next_token.body))
                    // Лексема - ключевое слово
                    current_token_ = keyword_tokens.at(next_token.body);
                else
                    // Лексема - идентификатор (имя переменной)
                    current_token_ = token_type::Id{next_token.body};
                break;
            case TokenTypeId::TOKEN_NUMBER_INT: // Лексема-целое число
                current_token_ = token_type::NumberInt{stoi(next_token.body)};
                break;
            case TokenTypeId::TOKEN_NUMBER_DOUBLE:
                current_token_ = token_type::NumberDouble{stod(next_token.body)};
                break;
            case TokenTypeId::TOKEN_CHAR: // Лексема - специальная символьная группа
                if (special_tokens.count(next_token.body))
                    // Лексема - распознанный набор спецсимволов
                    current_token_ = special_tokens.at(next_token.body);
                else
                    // Лексема - нераспознанная комбинация из спецсимволов
                    current_token_ = token_type::Char{next_token.body[0]};
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
                current_token_ = token_type::Char{next_token.body[0]};
        }    
        return current_token_;
    }
}  // namespace parse
