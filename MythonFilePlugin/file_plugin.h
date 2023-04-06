
#pragma once

// Двоичное дополнение - "втыкало" - для интерпретатора МУФЛОН. После подключения обеспечивает
// простую работу с файловой системой средствами библиотеки ввода-вывода в стиле C.

#define MYTHLON_INTERPRETER_MODULE

#include "../declares.h"
#include "../runtime.h"
#include "../statement.h"
#include "../parse.h"
#include "../throw_messages.h"

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <cstdio>

#if defined (_WIN64) || defined(_WIN32)
    #define MYTHLON_MODULE_EXPORT __declspec(dllexport)
#else
    #define MYTHLON_MODULE_EXPORT
#endif

namespace ast
{
    class NewPlugin : public Statement
    {
    public:    
        NewPlugin(const NewPlugin&) = delete;
        NewPlugin(NewPlugin&&) = default;
        NewPlugin(const std::vector<std::unique_ptr<Statement>>& args) = delete;
        NewPlugin& operator=(const NewPlugin&) = delete;
        NewPlugin& operator=(NewPlugin&&) = default;
        NewPlugin(std::vector<std::unique_ptr<Statement>>&& args);
        // Возвращает объект, содержащий значение типа PluginInstance,
        // представляющее собой созданный экземпляр специального объекта двоичного дополнения интерпретатора МУФЛОНа.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;
        
    private:
        std::vector<std::unique_ptr<Statement>> args_;
    };
} //namespace ast

using PluginListType = std::vector<std::pair<std::string, std::string>>;

extern "C"
{
    MYTHLON_MODULE_EXPORT std::unique_ptr<ast::Statement>
        CreatePlugin(std::vector<std::unique_ptr<ast::Statement>> args);
    MYTHLON_MODULE_EXPORT PluginListType* LoadPluginList();
}

namespace runtime
{
    class PluginInstance : public CommonClassInstance
    { // Экземпляр "двоичного дополнения МУФЛОНа" - специального загружаемого объекта с предопределенным
      // набором методов.
    public:
        using PluginCallMethod = ObjectHolder(PluginInstance::*)(const std::string&, const std::vector<ObjectHolder>&,
                                                                 Context&);
        PluginInstance(const std::string& filename, const std::string& filemode);
        ~PluginInstance();
        void Print(std::ostream& os, Context& context) override;

        ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                          Context& context) override;
        bool HasMethod(const std::string& method_name, size_t argument_count) const override;
    
    private:
        static const std::unordered_map<std::string_view, PluginCallMethod> plugin_method_table_;
        static const std::unordered_map<std::string_view,  std::pair<size_t, size_t>> plugin_method_argument_count_;

        // Обработчики методов класса двоичного дополнения МУФЛОНа
        ObjectHolder MethodFileOpen(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                    Context& context);
        ObjectHolder MethodFileClose(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                     Context& context);
        ObjectHolder MethodFileRead(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                    Context& context);
        ObjectHolder MethodFileWrite(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                     Context& context);
        ObjectHolder MethodFileSeek(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                    Context& context);
        ObjectHolder MethodFileTell(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                    Context& context);
        ObjectHolder MethodFileRewind(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                      Context& context);
        ObjectHolder MethodFileIsOpen(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                      Context& context);
        ObjectHolder MethodFileRemove(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                      Context& context);
        ObjectHolder MethodFileRename(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                      Context& context);
        ObjectHolder MethodFileStatus(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                      Context& context);
        ObjectHolder MethodFileEof(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                   Context& context);
        ObjectHolder MethodFileError(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                     Context& context);

        std::string filename_;
        std::string filemode_;
        FILE* file_handle_ptr_ = nullptr;
        int file_error_ = 0;
    };
} // namespace runtime
