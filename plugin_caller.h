#pragma once

// Заголовок модуль (единицы трансляции), в котором реализованы оберточные классы над C-интерфейсом втыкалы, обеспечивающие ее работу как
// стандартного внутреннего объекта исполнительской среды МУФЛОНа.

#include "declares.h"
#include "throw_messages.h"
#include "runtime.h"
#include "statement.h"
#include "parse.h"

#undef MYTHLON_PLUGIN
#include "plugin_helpers.h"

// Вспомогательная функция проверки удовлетворения C-строкой правила предельной длины.
bool CheckStringMaxLength(const char* test_string, size_t max_length);

namespace ast
{
    // Класс NewPlugin - обобщённый фабричный класс-переходник, создающий рабочий объект любой "втыкалы" - экземпляр основного ее класса PluginInstance.
    class NewPluginInstance : public Statement
    {
    public:
        NewPluginInstance
            (const std::string& class_name, std::vector<std::unique_ptr<Statement>>&& expression_args, const PluginDescData& plugin_desc);
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
        const PluginDescData& plugin_desc_;
        std::vector<std::unique_ptr<Statement>> expression_args_;
    };

    // Фабричная функция, создающая экземпляр класса NewPluginInstance. Фактически, является некоторой обёрткой вокруг его конструктора.
    std::unique_ptr<Statement> CreateNewPluginInstance
        (const std::string& class_name, const PluginDescData& plugin_desc, std::vector<std::unique_ptr<Statement>> args);
} //namespace ast

namespace runtime
{
    struct GetMethodResult
    {
        GetMethodResult() = default;
        GetMethodResult(const ast::MethodDefiner* p_method_definer) : method_definer(p_method_definer)
        {}
        GetMethodResult(ThrowMessageNumber p_err_num, std::string p_err_text) : err_num(p_err_num), err_text(std::move(p_err_text))
        {}

        const ast::MethodDefiner* method_definer = nullptr;
        // Поля, кодирующие ошибку поиска совместимой перегрузки метода. Используются при method_definer == nullptr.
        ThrowMessageNumber err_num = ThrowMessageNumber::THRM_UNKNOWN;
        std::string err_text;
    };

    // Проверка допустимости по типам и количеству списка фактических параметров actual_args для вызова метода method_def.
    std::pair<ThrowMessageNumber, std::string> CheckActualParamLegality(const ast::MethodDefiner& method_def, const std::vector<ObjectHolder>& actual_args);

    // Перегрузки функции поиска метода method_name, принимающего arg_count параметров (первая перегрузка) или параметры actual_args (вторая перегрузка),
    // в списке доступных методов втыкалы plugin_desc.
    GetMethodResult GetMethod(const ast::PluginDescData& plugin_desc, const std::string& method_name, size_t arg_count);
    GetMethodResult GetMethod(const ast::PluginDescData& plugin_desc, const std::string& method_name, const std::vector<ObjectHolder>& actual_args);

    class PluginInstance : public CommonClassInstance
    { // Экземпляр "двоичного дополнения МУФЛОНа" - специального загружаемого объекта с предопределенным набором методов.
    public:

        PluginInstance(const std::string& class_name, const ast::PluginDescData& plugin_desc, Context& context);
        // Класс некопируемый, но перемещаемый.
        PluginInstance(const PluginInstance& other) = delete;
        PluginInstance(PluginInstance&& other) noexcept;

        ~PluginInstance() override;

        void Print(std::ostream& os, Context& context) override;

        ObjectHolder Call(const std::string& method, const std::vector<ObjectHolder>& actual_args,
                          Context& context, const std::string& parent_name = {}) override;
        bool HasMethod(const std::string& method_name, size_t argument_count) const override;
        std::string GetClassName() const override
        {
            return class_name_;
        }

    private:
        // Имя класса втыкалы, которому соответствует данный объект.
        std::string class_name_;
        // Описатель втыкалы plugin_desc_ содержит всю информацию, необходимую для работы с ней - указатели на её сервисные функции (информирующую
        // и исполняющую), а также дескрипторы всех методов, предоставляемых классом втыкалы для вызова из программы на Муфлоне. Дескрипторы методов
        // содержат также данные, позволяющие организовать процедуру предварительной проверки соответствия фактических параметров метода
        // (при его вызове) требованиям к составу его формальных аргументов. При нарушении этих требований вызов метода не выполняется, а сразу
        // выбрасывается соответствующее исключение.
        const ast::PluginDescData& plugin_desc_;
        Context& context_;
    };
} // namespace runtime
