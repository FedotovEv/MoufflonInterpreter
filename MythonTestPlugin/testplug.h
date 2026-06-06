
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

#define MYTHLON_PLUGIN
#include "plugin_helpers.h"

class PluginInstance
{ // Экземпляр "двоичного дополнения МУФЛОНа" - специального загружаемого объекта с предопределенным набором методов.
public:       
    using PluginCallMethod = void(PluginInstance::*)(uintptr_t);

    PluginInstance() = default;
    void Call(const char* method_name, uintptr_t plugin_method_call_id);
    enum class CopyCharmResult
    {
        COPY_CHARM_OK = 0,
        COPY_CHARM_METHOD_NOT_FOUND,
        COPY_CHARM_BUFFER_TOO_SMALL
    };
    static std::pair<size_t, CopyCharmResult> CopyCharm(const std::string& req_method_name, void* target_area, size_t target_area_size);

    static void SetHelperFunctions(const PluginHelperFunctions& helper_funcs)
    {
        helper_funcs_ = helper_funcs;
    }

    static PluginGetInstanceIdFunc GetInstanceId()
    {
        return helper_funcs_.get_instance_func;
    }

private:
    static const std::unordered_map<std::string_view, PluginCallMethod> plugin_method_table_;
    static PluginHelperFunctions helper_funcs_;

    struct ParamsCharm
    {
        PluginMethodDefiner params_definer;          // Количественная характеристика списка фактических параметров.
        std::vector<MethodParamType> params_type;    // Типовое описание каждого фактического параметра, если оно используется.
    };
    static const std::unordered_map<std::string_view, ParamsCharm> plugin_method_params_charm_;

    // Обработчики методов класса двоичного дополнения МУФЛОНа
    void MethodInit(uintptr_t plugin_method_call_id);
    void MethodStringize(uintptr_t plugin_method_call_id);
    void MethodTestAddAll(uintptr_t plugin_method_call_id);
    void MethodTestFindZero(uintptr_t plugin_method_call_id);
    void MethodTestFindChar(uintptr_t plugin_method_call_id);
    void MethodTestSton(uintptr_t plugin_method_call_id);
    void MethodTestPrintHello(uintptr_t plugin_method_call_id);
};

extern "C"
{
    // Головная функция с предопределённым и фиксированным именем, предоставляющая список имён информирующих функций, по одной для
    // каждой втыкалы, существующей в составе данной библиотеки. Так как у нас втыкала в библиотеке всего одна, то и список имён будет состоять только из
    // одного элемента.
    MYTHLON_MODULE_EXPORT const char* GetPluginsInfoFunction(uint32_t load_level);
    // Информирующая функция (информатор) для данной втыкалы.
    MYTHLON_MODULE_EXPORT int32_t GetPluginInfo(uint32_t request_type, void* source_area, int32_t source_length, void* target_area, int32_t target_length);
    // Её вызывная функция, служащая для обращения к методам класса втыкалы.
    MYTHLON_MODULE_EXPORT void CallPluginMethod(const char* method_name, uintptr_t plugin_method_call_id);
}
