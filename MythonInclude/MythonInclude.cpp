
#include "../lexer.h"
#include "../parse.h"
#include "../runtime.h"
#include "../statement.h"

#include <iostream>
#include <string>
#include <variant>
#include <unordered_map>
#include <cstdio>
#include <stdexcept>

using namespace std;

class LexerFileInputExImpl : public parse::LexerInputEx
{
public:
    struct StackType
    {
        string file_name;
        int file_position;
        int module_string_number;
    };    

    LexerFileInputExImpl(string start_file_name) : start_file_name_(move(start_file_name))
    {}

    ~LexerFileInputExImpl()
    {
        CloseFile();
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
            if (current_file_)
            {
                fclose(current_file_);
                current_file_ = nullptr;
            }
            current_file_name_.clear();
            current_file_size_ = 0;
            current_file_position_ = 0;
            include_stack_.clear();
            // Устанавливаем start_file_name_ в качестве стартового модуля проекта.
            include_arg = start_file_name_;
        }

        if (current_file_name_.size())
        {
            include_stack_.push_back({current_file_name_, current_file_position_,
                                      command_desc_ptr_->module_string_number});
            CloseFile();
        }

        OpenFile(include_arg);
        if (!filename_to_module_id_.count(current_file_name_))
            filename_to_module_id_[current_file_name_] = ++last_file_number_;
        command_desc_ptr_->module_id = filename_to_module_id_[current_file_name_];
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
            if (current_file_position_ < current_file_size_)
            {
                errno = 0;
                last_read_symb_ = fgetc(current_file_);
                if (errno || last_read_symb_ == EOF)
                    ThrowReadException(current_file_name_);
                ++current_file_position_;
                break;
            }
            else
            {
                if (include_stack_.size())
                {
                    StackType stack_rec = include_stack_.back();
                    include_stack_.pop_back();
                    CloseFile();
                    OpenFile(stack_rec.file_name);
                    errno = 0;
                    if (fseek(current_file_, stack_rec.file_position, SEEK_SET) || errno)
                        ThrowReadException(current_file_name_);                    
                    current_file_position_ = stack_rec.file_position;
                    command_desc_ptr_->module_id = filename_to_module_id_[current_file_name_];
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
            ungetc(last_read_symb_, current_file_);
            --current_file_position_;
            return last_read_symb_;
        }
        else
        {
            return char_traits<char>::eof();
        }
    }

    LexerFileInputExImpl& unget() override
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

    [[noreturn]] static void ThrowOpenException(const string& filename)
    {
        throw ParseError("Файл "s + filename + " не найден или ошибка при открытии"s);
    }

    [[noreturn]] static void ThrowReadException(const string& filename)
    {
        throw ParseError("Ошибка при чтении Файла "s + filename);
    }

    void OpenFile(const string& filename)
    {
        errno = 0;
        current_file_ = fopen(filename.c_str(), "rb");
        if (!current_file_ || errno)
            ThrowOpenException(filename);
        fseek(current_file_, 0, SEEK_END);
        if (errno)
            ThrowOpenException(filename);        
        current_file_size_ = ftell(current_file_);
        rewind(current_file_);
        current_file_name_ = filename;
        current_file_position_ = 0;
    }

    void CloseFile()
    {
        if (current_file_)
        {
            fclose(current_file_);
            current_file_ = nullptr;
            current_file_position_ = 0;
            current_file_size_ = 0;
            current_file_name_.clear();
        }
    }

    bool eof_bit_ = false;
    int last_read_symb_ = char_traits<char>::eof();
    int unget_symb_ = char_traits<char>::eof();
    string current_file_name_;
    int current_file_size_ = 0;
    int current_file_position_ = 0;
    FILE* current_file_ = nullptr;
    vector<StackType> include_stack_;
    unordered_map<string, int> filename_to_module_id_;
    runtime::ProgramCommandDescriptor* command_desc_ptr_ = nullptr;
    string start_file_name_;

    inline static int last_file_number_ = 0;
};

void RunMythonProgramFromFile(const string& input_filename, ostream& output,
                              const runtime::LinkageFunction& link_function = {})
{
    parse::TrivialParseContext parse_context;
    LexerFileInputExImpl input_ex(input_filename);
    parse::Lexer lexer(input_ex);
    runtime::SimpleContext context(output, link_function);
    runtime::Closure closure;

    {
        auto program = ParseProgram(lexer, parse_context);
        program->Execute(closure, context);
    }
    parse_context.DeallocateGlobalResources();
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cout << "Формат команды: MythonInclude program_filename" << endl;
        return EXIT_FAILURE;
    }

    try
    {
        RunMythonProgramFromFile(argv[1], cout);
    }
    catch (const ParseError& parse_error)
    {
        cout << "Ошибка синтаксического разбора - " << parse_error.what() << endl;
        return EXIT_FAILURE;        
    }
    catch (const runtime_error& runtime)
    {
        cout << "Ошибка периода исполнения - " << runtime.what() << endl;
        return EXIT_FAILURE;        
    }
    catch (const exception& another_exception)
    {
        cout << "Прочие ошибки - " << another_exception.what() << endl;
        return EXIT_FAILURE;        
    }
    
    return EXIT_SUCCESS;
}
