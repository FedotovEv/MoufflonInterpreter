
// Двоичное дополнение - "втыкало" - для интерпретатора МУФЛОН. После подключения обеспечивает
// простую работу с файловой системой средствами библиотеки ввода-вывода в стиле C.
// Данный библиотечный модуль реализован на языке C, а не C++, поэтому также выступает как пример
// реализации расширений для среды МУФЛОНА на этом языке программирования.
#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#if defined (_WIN64) || defined(_WIN32)
    #define MYTHLON_MODULE_EXPORT __declspec(dllexport)
#else
    #define MYTHLON_MODULE_EXPORT
#endif

#define MYTHLON_PLUGIN
#include "plugin_helpers.h"

// Головная функция с предопределённым и фиксированным именем, предоставляющая список имён информирующих функций, по одной для
// каждой втыкалы, существующей в составе данной библиотеки. Так как у нас втыкала в библиотеке всего одна, то и список имён будет состоять только из
// одного элемента.
MYTHLON_MODULE_EXPORT const char* GetPluginsInfoFunction(uint32_t load_level);
// Информирующая функция (информатор) для данной втыкалы.
MYTHLON_MODULE_EXPORT int32_t GetPluginInfo(uint32_t request_type, void* source_area, int32_t source_length, void* target_area, int32_t target_length);
// Её вызывная функция, служащая для обращения к методам класса втыкалы.
MYTHLON_MODULE_EXPORT void CallPluginMethod(const char* method_name, uintptr_t plugin_method_call_id);

// Полная максимальная длина маршрута обрабатываемого файла.
#define FILENAME_LENGTH 512
#define FILEMODE_LENGTH 8
struct FilePluginStatus
{
    // Идентификатор (идент) "внешнего" (существующего в рамках исполнительной среды, к которой мы подключены) оболочечного
    // объекта втыкалы.
    uintptr_t external_object_id;
    // Внешние реквизиты обрабатываемого файла - его имя и режим открытия.
    char filename[FILENAME_LENGTH];
    char filemode[FILEMODE_LENGTH];
    // Внутренние идентификаторы состояния обрабатываемого файла.
    FILE* file_handle;
    int file_error;
    // Указатель на предыдущий элемент двусвязного списка состояний существующих в данный момент объектов втыкал.
    struct FilePluginStatus* prev_plugin_rec;
    // Указатель на следующий элемент этого двусвязного списка.
    struct FilePluginStatus* next_plugin_rec;
};

// Головная функция с предопределённым и фиксированным именем, предоставляющая список имён информирующих функций, по одной для
// каждой втыкалы, существующей в составе данной библиотеки. Так как у нас втыкала в библиотеке всего одна, то и список имён будет
// состоять только из одного элемента.
MYTHLON_MODULE_EXPORT const char* GetPluginsInfoFunction(uint32_t load_level);
// Информирующая функция (информатор) для данной втыкалы.
MYTHLON_MODULE_EXPORT int32_t GetPluginInfo(uint32_t request_type, void* source_area, int32_t source_length, void* target_area, int32_t target_length);
// Её вызывная функция, служащая для обращения к методам класса втыкалы.
MYTHLON_MODULE_EXPORT void CallPluginMethod(const char* method_name, uintptr_t plugin_method_call_id);

// Функция общей очистки всех существующих экземпляров втыкалы.
void ClearPluginStatuses();

// Функциональный тип, описывающий обработчик метода класса втыкалы.
typedef void(*MethodFunc)(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
// Обработчики методов класса двоичного дополнения МУФЛОНа
// Конструктор.
void MethodInit(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
// Строковщик - метод, возвращающий строку в соответствии с текущим состоянием класса.
void MethodStringize(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
// Деструктор.
void MethodDestroy(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
// Прочие методы класса втыкалы общего назначения (собственно, производящие работу с файлом).
void MethodFileOpen(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileClose(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileRead(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileWrite(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileSeek(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileTell(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileRewind(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileIsOpen(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileRemove(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileRename(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileStatus(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileEof(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
void MethodFileError(uintptr_t plugin_method_call_id, struct FilePluginStatus* plugin_status);
