
#pragma once

// Двоичное тестовое дополнение - "втыкало" - для интерпретатора МУФЛОН.  Служит для работы модульных тестов интерпретатора,
// проверяющих работоспособность механизма втыкал.

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

namespace runtime
{
    class PluginInstance
    { // Экземпляр "двоичного дополнения МУФЛОНа" - специального загружаемого объекта с предопределенным
      // набором методов.
    public:

        //using PluginCallMethod = ObjectHolder(PluginInstance::*)(const std::string&, const std::vector<ObjectHolder>&, Context&);
        
        using PluginCallMethod = intptr_t(PluginInstance::*)(intptr_t, intptr_t, intptr_t);

        PluginInstance() = default;
        void Print(intptr_t ostream, intptr_t context);

        intptr_t Call(intptr_t method, intptr_t actual_args, intptr_t context, intptr_t parent_name = 0);
        bool HasMethod(intptr_t method_name, size_t argument_count) const;

    private:
        static const std::unordered_map<std::string_view, PluginCallMethod> plugin_method_table_;
        static const std::unordered_map<std::string_view, std::pair<size_t, size_t>> plugin_method_argument_count_;

        // Обработчики методов класса двоичного дополнения МУФЛОНа
        intptr_t MethodTestAddAll(const std::string& method, const std::vector<intptr_t>& actual_args,
                                  intptr_t context);
        intptr_t MethodTestFindZero(const std::string& method, const std::vector<intptr_t>& actual_args,
                                    intptr_t context);
        intptr_t MethodTestFindChar(const std::string& method, const std::vector<intptr_t>& actual_args,
                                    intptr_t context);
        intptr_t MethodTestSton(const std::string& method, const std::vector<intptr_t>& actual_args,
                                intptr_t context);
        intptr_t MethodTestPrintHello(const std::string& method, const std::vector<intptr_t>& actual_args,
                                      intptr_t context);
    };
} // namespace runtime

namespace ast
{
    // Класс NewPlugin - фабричный класс, создающий рабочий объект данной "втыкалы" - экземпляр основного ее класса PluginInstance.
    class NewPlugin
    {
    public:
        NewPlugin(const NewPlugin&) = delete;
        NewPlugin(NewPlugin&&) = default;
        NewPlugin(const std::vector<intptr_t>& args);
        NewPlugin& operator=(const NewPlugin&) = delete;
        NewPlugin& operator=(NewPlugin&&) = default;
        // Возвращает объект, содержащий значение типа PluginInstance,
        // представляющее собой созданный экземпляр специального объекта двоичного дополнения интерпретатора МУФЛОНа.
        intptr_t Execute(intptr_t closure, intptr_t context);

    private:
        std::vector<intptr_t> args_;
    };

    std::unordered_map<intptr_t, runtime::PluginInstance> plugin_instance_storage;
    intptr_t CreateNewPlugin(intptr_t args_list_handle);

} //namespace ast

extern "C"
{
    MYTHLON_MODULE_EXPORT intptr_t CreatePlugin(intptr_t args);
    MYTHLON_MODULE_EXPORT intptr_t LoadPluginList();
}
