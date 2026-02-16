
#pragma once

// Двоичное тестовое дополнение - "втыкало" - для интерпретатора МУФЛОН.  Служит для работы модульных тестов интерпретатора,
// проверяющих работоспособность механизма втыкал.

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
        PluginInstance() = default;
        void Print(std::ostream& os, Context& context) override;

        ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                          Context& context, const std::string& parent_name = {}) override;
        bool HasMethod(const std::string& method_name, size_t argument_count) const override;

    private:
        static const std::unordered_map<std::string_view, PluginCallMethod> plugin_method_table_;
        static const std::unordered_map<std::string_view, std::pair<size_t, size_t>> plugin_method_argument_count_;

        // Обработчики методов класса двоичного дополнения МУФЛОНа
        ObjectHolder MethodTestAddAll(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                      Context& context);
        ObjectHolder MethodTestFindZero(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                        Context& context);
        ObjectHolder MethodTestFindChar(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                        Context& context);
        ObjectHolder MethodTestSton(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                    Context& context);
        ObjectHolder MethodTestPrintHello(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                                          Context& context);
    };
} // namespace runtime
