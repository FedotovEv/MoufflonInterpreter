#pragma once

#include "declares.h"
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
        {  // Лексема «целое число».
            int value;   // Целое число.
        };

        struct NumberDouble
        {  // Лексема «число с плавающей точкой».
            double value;   // Дробное число.
        };

        struct Id
        {  // Лексема «идентификатор».
            std::string value;  // Имя идентификатора.
        };

        struct Char
        {    // Лексема «символ».
            char value;  // Код символа.
        };

        struct String
        {  // Лексема «строковая константа».
            std::string value;          // Тело строки.
            const SingleByteEncodingDesc* encoding = NO_ENCODING;   // Кодировка, в которой находится тело.
            UTF8Map utf8_map;           // Карта расположения символов в строке для многобайтовой кодировки UTF-8.
        };

        struct Class {};        // Лексема «class»
        struct Return {};       // Лексема «return»
        struct CoYield {};      // Лексема «co_yield»
        struct ReturnRef {};    // Лексема «return_ref»
        struct CoYieldRef {};   // Лексема «co_yield_ref»
        struct CoAwait {};      // Лексема «co_await»
        struct If {};           // Лексема «if»
        struct Elif {};         // Лексема «elif»
        struct Else {};         // Лексема «else»
        struct While {};        // Лексема "while"
        struct Break {};        // Лексема "break"
        struct Continue {};     // Лексема "continue"
        struct Pass {};         // Лексема "pass"
        struct Del {};          // Лексема "del"
        // Лексемы обслуживания системы обработки исключений.
        struct Try {};          // Лексема "try"
        struct Except {};       // Лексема "except"
        struct Finally {};      // Лексема "finally"
        struct As {};           // Лексема "as"
        struct Raise {};        // Лексема "raise"
        //
        struct Def {};          // Лексема «def»
        struct Newline {};      // Лексема «конец строки»
        struct Print {};        // Лексема «print»
        struct Import {};       // Лексема «import»
        struct Include {};      // Лексема «include»
        struct Indent {};       // Лексема «увеличение отступа», соответствует двум пробелам
        struct Dedent {};       // Лексема «уменьшение отступа»
        struct Eof {};          // Лексема «конец файла»
        struct And {};          // Лексема «and»
        struct Or {};           // Лексема «or»
        struct Not {};          // Лексема «not»
        struct Xor {};          // Лексема «xor»
        struct Eq {};           // Лексема «==»
        struct NotEq {};        // Лексема «!=»
        struct LessOrEq {};     // Лексема «<=»
        struct GreaterOrEq {};  // Лексема «>=»
        struct ShiftLeft {};    // Лексема «<<»
        struct ShiftRight {};   // Лексема «>>»
        struct None {};         // Лексема «None»
        struct True {};         // Лексема «True»
        struct False {};        // Лексема «False»
    }  // namespace token_type

    using TokenBase
        = std::variant<token_type::NumberInt, token_type::NumberDouble, token_type::Id, token_type::Char,
                       token_type::String, token_type::Class,
                       token_type::Return, token_type::CoYield, token_type::ReturnRef, token_type::CoYieldRef, token_type::CoAwait,
                       token_type::If, token_type::Elif, token_type::Else,
                       token_type::While, token_type::Break, token_type::Continue, token_type::Pass, token_type::Del,
                       token_type::Try, token_type::Except, token_type::Finally, token_type::As, token_type::Raise,
                       token_type::Def, token_type::Newline,
                       token_type::Print, token_type::Import, token_type::Include,
                       token_type::Indent, token_type::Dedent,
                       token_type::And, token_type::Or, token_type::Xor,
                       token_type::Not, token_type::Eq, token_type::NotEq, token_type::LessOrEq,
                       token_type::GreaterOrEq, token_type::ShiftLeft, token_type::ShiftRight,
                       token_type::None, token_type::True, token_type::False, token_type::Eof>;

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
    std::string TokenTypeToString(const Token& rhs);

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
        static constexpr char BAD_TOKEN_TYPE[] = "Недопустимый тип жетона";
        static constexpr char BAD_TOKEN_VALUE[] = "Недопустимое значение жетона";
        static constexpr char TOKEN_AWAITING[] = " - ожидается - ";
        static constexpr char BAD_PREFIX_VALUE[] = "Недопустимое значение префикса строки";
        static constexpr char BAD_ENC_NAME[] = "Неизвестное имя кодировки";
        static constexpr char WIDE_CHAR_IN_NARROW_STRING[] = "Определение широкого символа для узких строк";
        static constexpr char HEX_DIGIT_AWAIT[] = "Ожидается шестнадцатеричная цифра";
        static constexpr char LEXEM_PREMATURE_TERMINATED[] = "Преждевременный обрыв лексемы";
        
        explicit Lexer(LexerInputEx& input);
        explicit Lexer(std::istream& input);
        Lexer(const Lexer& other);
        Lexer(Lexer&& other) noexcept;
        ~Lexer();

        // Возвращает ссылку на текущий жетон или token_type::Eof, если поток жетонов на входе закончился.
        [[nodiscard]] const Token& CurrentToken() const;

        // Продвигает очередь входных жетонов вперёд на единицу и возвращает следующий жетон, либо token_type::Eof,
        // если поток жетонов на входе закончился.
        Token NextToken();

        // Если текущий токен имеет тип T, метод возвращает ссылку на него.
        // В противном случае метод выбрасывает исключение LexerError.
        template <typename T>
        const T& Expect() const
        {
            using namespace std::literals;
            if (current_token_.Is<T>())
                return current_token_.As<T>();
            else
                throw LexerError(CommandDescToString(current_command_desc_) + BAD_TOKEN_TYPE + TOKEN_AWAITING + TokenTypeToString(T{}));
        }

        // Метод проверяет, что текущий токен имеет тип T, а сам токен содержит значение value.
        // В противном случае метод выбрасывает исключение LexerError.
        template <typename T, typename U>
        void Expect(const U& value) const
        {
            using namespace std::literals;
            Expect<T>();
            if (current_token_ != T{value})
                throw LexerError(CommandDescToString(current_command_desc_) + BAD_TOKEN_TYPE + TOKEN_AWAITING + TokenTypeToString(T{value}));
        }

        // Если следующий токен имеет тип T, метод возвращает ссылку на него.
        // В противном случае метод выбрасывает исключение LexerError.
        template <typename T>
        const T& ExpectNext()
        {
            using namespace std::literals;
            NextToken();
            return Expect<T>();
        }

        // Метод проверяет, что следующий токен имеет тип T, а сам токен содержит значение value.
        // В противном случае метод выбрасывает исключение LexerError.
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
            --current_command_desc_.module_string_number;
        }

        void SetSourceEncoding(const SingleByteEncodingDesc* source_encoding)
        {
            source_encoding_ = source_encoding;
        }

        const SingleByteEncodingDesc* GetSourceEncoding() const
        {
            return source_encoding_;
        }

        bool IsSourceInUTF8() const
        {
            return source_encoding_ == UTF_8_ENCODING;
        }

        // Операции работы со строковым префиксом (префиксом строкового литерала).
        std::string GetStringPrefix() const
        {
            return string_prefix_;
        }

        void SetStringPrefix(std::string string_prefix)
        {
            string_prefix_ = std::move(string_prefix);
        }

        void ClearStringPrefix()
        {
            string_prefix_.clear();
        }

    private:
        static constexpr int SPACES_PER_INDENT_STEP = 2;

        LexerInputEx& input_;
        int indent_amount_;
        int indent_sent_;
        Token current_token_;
        runtime::ProgramCommandDescriptor current_command_desc_;
        bool is_input_need_delete_;
        // Информация о строковой кодировке исходных текстов разбираемой программы.
        const SingleByteEncodingDesc* source_encoding_ = NO_ENCODING;
        std::string string_prefix_;     // Строковый префикс следующего строкового литерала.
    };
}  // namespace parse
