#pragma once

#include "declares.h"
#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>

namespace parse
{
    namespace token_type
    {
        struct NumberInt
        {  // Лексема «целое число»
            int value;   // число
        };

        struct NumberDouble
        {  // Лексема «число с плавающей точкой»
            double value;   // число
        };

        struct Id
        {  // Лексема «идентификатор»
            std::string value;  // Имя идентификатора
        };

        struct Char
        {    // Лексема «символ»
            char value;  // код символа
        };

        struct String
        {  // Лексема «строковая константа»
            std::string value;
        };

        struct Class {};     // Лексема «class»
        struct Return {};    // Лексема «return»
        struct ReturnRef {}; // Лексема «return_ref»
        struct If {};        // Лексема «if»
        struct Else {};      // Лексема «else»
        struct While {};     // Лексема "while"
        struct Break {};     // Лексема "break"
        struct Continue {};  // Лексема "continue"
        struct Def {};       // Лексема «def»
        struct Newline {};   // Лексема «конец строки»
        struct Print {};     // Лексема «print»
        struct Import {};    // Лексема «import»
        struct Include {};   // Лексема «include»
        struct Indent {};    // Лексема «увеличение отступа», соответствует двум пробелам
        struct Dedent {};    // Лексема «уменьшение отступа»
        struct Eof {};       // Лексема «конец файла»
        struct And {};       // Лексема «and»
        struct Or {};        // Лексема «or»
        struct Not {};       // Лексема «not»
        struct Eq {};        // Лексема «==»
        struct NotEq {};     // Лексема «!=»
        struct LessOrEq {};  // Лексема «<=»
        struct GreaterOrEq {};  // Лексема «>=»
        struct None {};         // Лексема «None»
        struct True {};         // Лексема «True»
        struct False {};        // Лексема «False»
    }  // namespace token_type

    using TokenBase
        = std::variant<token_type::NumberInt, token_type::NumberDouble, token_type::Id, token_type::Char,
                       token_type::String, token_type::Class,
                       token_type::Return, token_type::ReturnRef, token_type::If, token_type::Else,
                       token_type::While, token_type::Break, token_type::Continue,
                       token_type::Def, token_type::Newline,
                       token_type::Print, token_type::Import, token_type::Include,
                       token_type::Indent, token_type::Dedent, token_type::And, token_type::Or,
                       token_type::Not, token_type::Eq, token_type::NotEq, token_type::LessOrEq,
                       token_type::GreaterOrEq, token_type::None, token_type::True,
                       token_type::False, token_type::Eof>;

    struct Token : TokenBase
    {
        using TokenBase::TokenBase;

        template <typename T>
        [[nodiscard]] bool Is() const
        {
            return std::holds_alternative<T>(*this);
        }

        template <typename T>
        [[nodiscard]] const T& As() const
        {
            return std::get<T>(*this);
        }

        template <typename T>
        [[nodiscard]] const T* TryAs() const
        {
            return std::get_if<T>(this);
        }
    };

    bool operator==(const Token& lhs, const Token& rhs);
    bool operator!=(const Token& lhs, const Token& rhs);

    std::ostream& operator<<(std::ostream& os, const Token& rhs);

    class LexerError : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class LexerInputEx
    {
    public:
        virtual ~LexerInputEx() = default;
        // Описанная ниже функция вызывается из конструктора лексического разборщика Муфлона один раз
        // и передаёт нашему классу указатель на внутреннее поле разборщика, хранящего описатель текущей
        // строки исходника в формате runtime::ProgramCommandDescriptor. Наследники этого класса должны
        // самостоятельно изменять содержимое этого поля при любом переключении (как прямом, по директиве
        // include, так и обратном - когда текущий модуль завершается) исходных модулей.
        virtual void SetCommandDescPtr(runtime::ProgramCommandDescriptor* command_desc_ptr) = 0;
        // Функция ниже вызывается при обработке директивы include. include_arg - параметр этой директивы.
        virtual void IncludeSwitchTo(std::string include_arg) = 0;
        virtual int get() = 0;
        virtual int peek() = 0;
        virtual LexerInputEx& unget() = 0;
        virtual operator bool() = 0;
        virtual bool operator!() = 0;
        virtual bool good() = 0;
    };
    
    class SimpleLexerInputEx : public LexerInputEx
    {
    public:
        SimpleLexerInputEx(std::istream& input_stream) : input_stream_(input_stream)
        {}

        void IncludeSwitchTo(std::string include_arg) override
        {}

        void SetCommandDescPtr(runtime::ProgramCommandDescriptor* command_desc_ptr) override
        {}

        int get() override
        {
            return input_stream_.get();
        }
        
        int peek() override
        {
            return input_stream_.peek();
        }
        
        SimpleLexerInputEx& unget() override
        {
            input_stream_.unget();
            return *this;
        }
                
        bool good() override
        {
            return input_stream_.good();
        }

        operator bool() override
        {
            return bool(input_stream_);
        }

        bool operator!() override
        {
            return !input_stream_;
        }

    private:
        std::istream& input_stream_;
    };
    
    class Lexer
    {
    public:
        explicit Lexer(LexerInputEx& input);
        explicit Lexer(std::istream& input);
        ~Lexer();

        // Возвращает ссылку на текущий токен или token_type::Eof, если поток токенов закончился
        [[nodiscard]] const Token& CurrentToken() const;

        // Возвращает следующий токен, либо token_type::Eof, если поток токенов закончился
        Token NextToken();

        // Если текущий токен имеет тип T, метод возвращает ссылку на него.
        // В противном случае метод выбрасывает исключение LexerError
        template <typename T>
        const T& Expect() const
        {
            using namespace std::literals;
            if (current_token_.Is<T>())
            {
                return current_token_.As<T>();
            }
            else
            {
                std::string command_desc = std::to_string(current_command_desc_.module_id) + "("s +
                    std::to_string(current_command_desc_.module_string_number) + "):"s;
                throw LexerError(command_desc + "Bad token type"s);
            }
        }

        // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит значение value.
        // В противном случае метод выбрасывает исключение LexerError
        template <typename T, typename U>
        void Expect(const U& value) const
        {
            using namespace std::literals;
            Expect<T>();
            if (current_token_ != T{value})
            {
                std::string command_desc = std::to_string(current_command_desc_.module_id) + "("s +
                    std::to_string(current_command_desc_.module_string_number) + "):"s;
                throw LexerError(command_desc + "Bad token value"s);
            }
        }

        // Если следующий токен имеет тип T, метод возвращает ссылку на него.
        // В противном случае метод выбрасывает исключение LexerError
        template <typename T>
        const T& ExpectNext()
        {
            using namespace std::literals;
            NextToken();
            return Expect<T>();
        }

        // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит значение value.
        // В противном случае метод выбрасывает исключение LexerError
        template <typename T, typename U>
        void ExpectNext(const U& value)
        {
            using namespace std::literals;
            NextToken();
            Expect<T>(value);
        }

        runtime::ProgramCommandDescriptor GetCurrentCommandDesc() const
        {
            return current_command_desc_;
        }

        void IncludeSwitchTo(std::string include_arg)
        {
            input_.IncludeSwitchTo(include_arg);
        }

    private:

        static constexpr int SPACES_PER_INDENT_STEP = 2;

        LexerInputEx& input_;
        int indent_amount_;
        int indent_sent_;
        Token current_token_;
        runtime::ProgramCommandDescriptor current_command_desc_;
        bool is_input_need_delete_;
    };
}  // namespace parse
