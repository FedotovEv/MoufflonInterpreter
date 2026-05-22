#pragma once

// Заголовок модуль (единицы трансляции), в котором реализованы оберточные классы над C-интерфейсом втыкалы, обеспечивающие ее работу как
// стандартного внутреннего объекта исполнительской среды МУФЛОНа.
// Кроме того, тут находятся некоторые вспомогательные функции и прочий синтаксический сахар, облегчающий использование втыкал другими
// частями комплекса.

#include "declares.h"
#include "throw_messages.h"
#include "runtime.h"
#include "statement.h"
#include "parse.h"

// Функции возврата целочисленного обработчика, соответствующего объектам классов runtime::ObjectHolder и runtime::Context.
intptr_t ObjectHolderHandler(runtime::ObjectHolder& object_holder);
intptr_t ContextHanlder(runtime::Context& context);

namespace ast
{
    struct MethodDefiner
    { // Структура описания некоторого метода, предоставляемого классом-втыкалой.
        std::string name;               // Имя метода.
        size_t arg_count_min = 0;       // Минимально допустимое количество его параметров.
        size_t arg_count_max = 0;       // Максимально допустимое количество его параметров.
        // Если метод имеет фиксированное и однозначно определённое количество параметров, можно выполнить контроль соответствия их
        // фактического типа требуемому.
        // Режим проверки допустимости данного параметра.
        runtime::MethodParamCheckMode check_mode = runtime::MethodParamCheckMode::PARAM_CHECK_NONE;
        // Список допустимых типов очередного параметра.
        std::vector<runtime::MethodParamType> param_types;
    };

    using PluginMethodList = std::unordered_map<std::string, MethodDefiner>;

    // Класс NewPlugin - обобщённый фабричный класс-переходник, создающий рабочий объект любой "втыкалы" - экземпляр основного ее класса PluginInstance.
    class NewPluginInstance : public Statement
    {
    public:
        NewPluginInstance
            (std::string&& class_name, std::vector<std::unique_ptr<Statement>>&& expression_args, PluginMethodList&& methods_def_list);
        NewPluginInstance(const NewPluginInstance&) = delete;
        NewPluginInstance(NewPluginInstance&&) = default;
        NewPluginInstance& operator=(const NewPluginInstance&) = delete;
        NewPluginInstance& operator=(NewPluginInstance&&) = default;
        // При исполнении (вызове его метода Execute) узла синтаксического дерева, содержащего этот объект, он возвращает объект, содержащий
        // значение типа PluginInstance, представляющее собой созданный экземпляр специального объекта двоичного дополнения интерпретатора
        // МУФЛОНа.
        runtime::ObjectHolder Execute(runtime::Closure& closure, runtime::Context& context) override;

    private:
        std::string class_name_;
        PluginMethodList plugin_methods_list_;
        std::vector<std::unique_ptr<Statement>> expression_args_;
    };
} //namespace ast

namespace runtime
{
    // Функция поиска метода method_name, принимающего arg_count параметров, в списке plugin_methods_list.
    const ast::MethodDefiner* GetMethod(const ast::PluginMethodList& plugin_methods_list, const std::string& method_name, size_t arg_count);
    // Проверка допустимости по типам и количеству списка фактических параметров actual_args для вызова метода method_def.
    void CheckActualParamLegality(Context& context, const ast::MethodDefiner& method_def, const std::vector<ObjectHolder>& actual_args);

    class PluginInstance : public CommonClassInstance
    { // Экземпляр "двоичного дополнения МУФЛОНа" - специального загружаемого объекта с предопределенным набором методов.
    public:

        PluginInstance(const std::string& class_name, const ast::PluginMethodList& methods_def_list);
        void Print(std::ostream& os, Context& context) override;

        ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                          Context& context, const std::string& parent_name = {}) override;
        bool HasMethod(const std::string& method_name, size_t argument_count) const override;
        std::string GetClassName() const override
        {
            return class_name_;
        }

    private:
        std::string class_name_;
        // Словарь plugin_methods_list_ содержит описания методов, предоставляемых классом втыкалы для вызова из программы на Муфлоне.
        // Также предусмотрена возможная процедура предварительной проверки соответствия фактических параметров метода
        // (при его вызове) требованиям к составу его формальных аргументов. При нарушении этих требований вызов метода не
        // выполняется, а сразу выбрасывается соответствующее исключение.
        ast::PluginMethodList plugin_methods_list_;
    };
} // namespace runtime
