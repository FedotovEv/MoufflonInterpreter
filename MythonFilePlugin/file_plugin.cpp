
// Двоичное дополнение - "втыкало" - для интерпретатора МУФЛОН. После подключения Обеспечивает
// простую работу с файловой системой средствами библиотеки ввода-вывода в стиле C.

#include "pch.h"
#include "file_plugin.h"

#include <optional>
#include <fstream>
#include <cstdio>

using namespace std;
using namespace runtime;

static char incorrect_arguments_quantity[] = "Конструктор объекта может иметь не более двух аргументов";
static char filename_not_string[] = "Имя файла должно быть строкой";
static char filemode_not_string[] = "Режим доступа к файлу должен быть строкой";
static char file_not_opened[] = "Файл не открыт";

namespace ast
{
    NewPlugin::NewPlugin(std::vector<std::unique_ptr<Statement>>&& args) : args_(move(args))
    {
        if (args_.size() > 2)
            throw ParseError(string(incorrect_arguments_quantity));
    }

    runtime::ObjectHolder NewPlugin::Execute(runtime::Closure& closure, runtime::Context& context)
    {
        PrepareExecute(this, closure, context);
        string filename, filemode;
        
        if (args_.size() >= 1)
        {
            ObjectHolder filename_object = args_[0]->Execute(closure, context);
            runtime::String* filename_ptr = filename_object.TryAs<runtime::String>();
            if (filename_ptr)
                filename = filename_ptr->GetValue();
            else
                ThrowRuntimeError(this, string(filename_not_string));
        }

        if (args_.size() >= 2)
        {
            ObjectHolder filemode_object = args_[1]->Execute(closure, context);
            runtime::String* filemode_ptr = filemode_object.TryAs<runtime::String>();
            if (filemode_ptr)
                filemode = filemode_ptr->GetValue();
            else
                ThrowRuntimeError(this, string(filemode_not_string));
        }

        return ObjectHolder::Own(runtime::PluginInstance(filename, filemode));
    }
} // namespace ast

static PluginListType plugin_list = {{"CreatePlugin"s, "file"s}};

extern "C"
{
    unique_ptr<ast::Statement> CreatePlugin(vector<unique_ptr<ast::Statement>> args)
    {
        return make_unique<ast::NewPlugin>(ast::NewPlugin(move(args)));
    }

    PluginListType* LoadPluginList()
    {
        return &plugin_list;
    }
}

namespace runtime
{
    const unordered_map<string_view, PluginInstance::PluginCallMethod> PluginInstance::plugin_method_table_
    {
        {"open"sv, &PluginInstance::MethodFileOpen},
        {"Open"sv, &PluginInstance::MethodFileOpen},
        {"close"sv, &PluginInstance::MethodFileClose},
        {"Close"sv, &PluginInstance::MethodFileClose},
        {"read"sv, &PluginInstance::MethodFileRead},
        {"Read"sv, &PluginInstance::MethodFileRead},
        {"write"sv, &PluginInstance::MethodFileWrite},
        {"Write"sv, &PluginInstance::MethodFileWrite},
        {"seek"sv, &PluginInstance::MethodFileSeek},
        {"Seek"sv, &PluginInstance::MethodFileSeek},
        {"tell"sv, &PluginInstance::MethodFileTell},
        {"Tell"sv, &PluginInstance::MethodFileTell},
        {"rewind"sv, &PluginInstance::MethodFileRewind},
        {"Rewind"sv, &PluginInstance::MethodFileRewind},
        {"is_open"sv, &PluginInstance::MethodFileIsOpen},
        {"IsOpen"sv, &PluginInstance::MethodFileIsOpen},
        {"remove"sv, &PluginInstance::MethodFileRemove},
        {"Remove"sv, &PluginInstance::MethodFileRemove},
        {"rename"sv, &PluginInstance::MethodFileRename},
        {"Rename"sv, &PluginInstance::MethodFileRename},
        {"status"sv, &PluginInstance::MethodFileStatus},
        {"Status"sv, &PluginInstance::MethodFileStatus},
        {"eof"sv, &PluginInstance::MethodFileEof},
        {"Eof"sv, &PluginInstance::MethodFileEof},
        {"error"sv, &PluginInstance::MethodFileError},
        {"Error"sv, &PluginInstance::MethodFileError}
    };

    const std::unordered_map<string_view, pair<size_t, size_t>> PluginInstance::plugin_method_argument_count_
    {
        {"open"sv, {1, 2}},
        {"Open"sv, {1, 2}},
        {"close"sv, {0, 0}},
        {"Close"sv, {0, 0}},
        {"read"sv, {1, 1}},
        {"Read"sv, {1, 1}},
        {"write"sv, {1, 1}},
        {"Write"sv, {1, 1}},
        {"seek"sv, {1, 2}},
        {"Seek"sv, {1, 2}},
        {"tell"sv, {0, 0}},
        {"Tell"sv, {0, 0}},
        {"rewind"sv, {0, 0}},
        {"Rewind"sv, {0, 0}},
        {"is_open"sv, {0, 0}},
        {"IsOpen"sv, {0, 0}},
        {"remove"sv, {1, 1}},
        {"Remove"sv, {1, 1}},
        {"rename"sv, {2, 2}},
        {"Rename"sv, {2, 2}},
        {"status"sv, {0, 0}},
        {"Status"sv, {0, 0}},
        {"eof"sv, {0, 0}},
        {"Eof"sv, {0, 0}},
        {"error"sv, {0, 0}},
        {"Error"sv, {0, 0}}
    };

    PluginInstance::PluginInstance(const std::string& filename, const std::string& filemode) :
                             filename_(filename), filemode_(filemode), file_handle_ptr_(nullptr), file_error_(0)
    {
        if (filename_.size())
        { // Конструктор вызван с именем файла, открываем его немедленно
            if (filemode_.empty())
                filemode_ = "r"s;

            errno = 0;
            file_handle_ptr_ = fopen(filename_.c_str(), filemode_.c_str());
            file_error_ = errno;
        }
    }

    PluginInstance::~PluginInstance()
    {
        if (file_handle_ptr_)
        {
            fclose(file_handle_ptr_);
            file_handle_ptr_ = nullptr;
        }
    }

    void PluginInstance::Print(ostream& os, Context& context)
    {
        os << "FileOpPlugin:";
    }

    ObjectHolder PluginInstance::MethodFileOpen(const string& method, const vector<ObjectHolder>& actual_args,
                                                Context& context)
    {
        CheckMethodParams(context, "Open"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_STRING, 1, actual_args);
        CheckMethodParams(context, "Open"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_LESS_EQ,
                          MethodParamType::PARAM_TYPE_STRING, 2, actual_args);

        file_error_ = 0;
        if (file_handle_ptr_)
        {
            fclose(file_handle_ptr_);
            file_handle_ptr_ = nullptr;
        }

        filename_ = actual_args[0].TryAs<String>()->GetValue();
        if (actual_args.size() == 2)
            filemode_ = actual_args[1].TryAs<String>()->GetValue();
        else
            filemode_ = "r"s;

        errno = 0;
        file_handle_ptr_ = fopen(filename_.c_str(), filemode_.c_str());
        file_error_ = errno;
        return ObjectHolder::Own(Number(file_error_));
    }

    ObjectHolder PluginInstance::MethodFileClose(const string& method, const vector<ObjectHolder>& actual_args,
                                                 Context& context)
    {
        CheckMethodParams(context, "Close"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        file_error_ = 0;
        if (file_handle_ptr_)
        {
            fclose(file_handle_ptr_);
            file_handle_ptr_ = nullptr;
        }
        return ObjectHolder::None();
    }

    ObjectHolder PluginInstance::MethodFileRead(const string& method, const vector<ObjectHolder>& actual_args,
                                                Context& context)
    {
        CheckMethodParams(context, "Read"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);

        if (!file_handle_ptr_)
            ThrowRuntimeError(context, string(file_not_opened));

        int read_length = actual_args[0].TryAs<Number>()->GetIntValue();
        string read_buffer(read_length, 0);
        
        errno = 0;
        fread(read_buffer.data(), 1, read_length, file_handle_ptr_);
        file_error_ = errno;
        return ObjectHolder::Own(String(move(read_buffer)));
    }

    ObjectHolder PluginInstance::MethodFileWrite(const string& method, const vector<ObjectHolder>& actual_args,
                                                 Context& context)
    {
        CheckMethodParams(context, "Write"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_NUMERIC_STRING, 1, actual_args);

        if (!file_handle_ptr_)
            ThrowRuntimeError(context, string(file_not_opened));

        const void* buffer_ptr = actual_args[0].Get()->GetPtr();
        int write_length = actual_args[0].Get()->SizeOf();
        
        errno = 0;
        int result = fwrite(buffer_ptr, 1, write_length, file_handle_ptr_);
        file_error_ = errno;
        return ObjectHolder::Own(Number(result));
    }

    ObjectHolder PluginInstance::MethodFileSeek(const string& method, const vector<ObjectHolder>& actual_args,
                                                Context& context)
    {
        CheckMethodParams(context, "Seek"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_GREATER_EQ,
                          MethodParamType::PARAM_TYPE_NUMERIC, 1, actual_args);
        CheckMethodParams(context, "Seek"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_LESS_EQ,
                          MethodParamType::PARAM_TYPE_NUMERIC, 2, actual_args);

        if (!file_handle_ptr_)
            ThrowRuntimeError(context, string(file_not_opened));

        int target_point = actual_args[0].TryAs<Number>()->GetIntValue();
        int seek_mode = SEEK_SET;
        if (actual_args.size() == 2)
        {
            int test_target_point = actual_args[1].TryAs<Number>()->GetIntValue();
            if (!test_target_point)
                seek_mode = SEEK_CUR;
            else
                seek_mode = test_target_point > 0 ? SEEK_SET : SEEK_END;
        }

        errno = 0;
        int result = fseek(file_handle_ptr_, target_point, seek_mode);
        file_error_ = errno;
        return ObjectHolder::Own(Number(result));
    }

    ObjectHolder PluginInstance::MethodFileTell(const string& method, const vector<ObjectHolder>& actual_args,
                                                Context& context)
    {
        CheckMethodParams(context, "Tell"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        if (!file_handle_ptr_)
            ThrowRuntimeError(context, string(file_not_opened));

        errno = 0;
        int result = ftell(file_handle_ptr_);
        file_error_ = errno;
        return ObjectHolder::Own(Number(result));
    }

    ObjectHolder PluginInstance::MethodFileRewind(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                  Context& context)
    {
        CheckMethodParams(context, "Rewind"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        if (!file_handle_ptr_)
            ThrowRuntimeError(context, string(file_not_opened));

        errno = 0;
        rewind(file_handle_ptr_);
        file_error_ = errno;
        return ObjectHolder::Own(Number(file_error_));
    }

    ObjectHolder PluginInstance::MethodFileIsOpen(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                  Context& context)
    {
        CheckMethodParams(context, "IsOpen"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);
        return ObjectHolder::Own(Bool(file_handle_ptr_));
    }

    ObjectHolder PluginInstance::MethodFileRemove(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                  Context& context)
    {
        CheckMethodParams(context, "Remove"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_STRING, 1, actual_args);    

        string remove_file_name = actual_args[0].TryAs<String>()->GetValue();
        errno = 0;
        int result = remove(remove_file_name.c_str());
        file_error_ = errno;
        return ObjectHolder::Own(Number(result));
    }
    
    ObjectHolder PluginInstance::MethodFileRename(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                  Context& context)
    {
        CheckMethodParams(context, "Rename"s, MethodParamCheckMode::PARAM_CHECK_TYPE_QUANTITY_EQUAL,
            MethodParamType::PARAM_TYPE_STRING, 2, actual_args);    

        string old_file_name = actual_args[0].TryAs<String>()->GetValue();
        string new_file_name = actual_args[0].TryAs<String>()->GetValue();
        errno = 0;
        int result = rename(old_file_name.c_str(), new_file_name.c_str());
        file_error_ = errno;
        return ObjectHolder::Own(Number(result));
    }

    ObjectHolder PluginInstance::MethodFileStatus(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                  Context& context)
    {
        CheckMethodParams(context, "Status"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        return ObjectHolder::Own(Number(file_error_));
    }
    
    ObjectHolder PluginInstance::MethodFileEof(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                               Context& context)
    {
        CheckMethodParams(context, "Eof"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        if (file_handle_ptr_)
            return ObjectHolder::Own(Bool(feof(file_handle_ptr_)));
        else
            return ObjectHolder::Own(Bool(true));
    }
    
    ObjectHolder PluginInstance::MethodFileError(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                                 Context& context)
    {
        CheckMethodParams(context, "Error"s, MethodParamCheckMode::PARAM_CHECK_QUANTITY_EQUAL,
                          MethodParamType::PARAM_TYPE_ANY, 0, actual_args);

        if (file_handle_ptr_)
            return ObjectHolder::Own(Bool(ferror(file_handle_ptr_)));
        else
            return ObjectHolder::Own(Bool(true));
    }    

    ObjectHolder PluginInstance::Call(const string& method_name,
                                      const vector<ObjectHolder>& actual_args, Context& context)
    {
        if (plugin_method_table_.count(method_name))
            return (this->*plugin_method_table_.at(method_name))(method_name, actual_args, context);
        else
            ThrowRuntimeError(context, ThrowMessageNumber::THRM_METHOD_NOT_FOUND);
    }

    bool PluginInstance::HasMethod(const string& method_name, size_t argument_count) const
    {
        if (plugin_method_argument_count_.count(method_name))
        {
            auto argument_org_count = plugin_method_argument_count_.at(method_name);
            return argument_count >= argument_org_count.first &&
                argument_count <= argument_org_count.second;
        }
        else
        {
            return false;
        }
    }
} //namespace runtime
