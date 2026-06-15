
#include "DebuggerDataStruct.h"

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <variant>
#include <unordered_map>
#include <cstdio>
#include <stdexcept>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

void LexerInputExImpl::InitStreamStatus()
{
    eof_bit_ = false;
    last_read_symb_ = char_traits<char>::eof();
    unget_symb_ = char_traits<char>::eof();
    current_position_ = 0;
    current_module_desc_ptr_ = nullptr;
    current_module_name_.clear();
    include_stack_.clear();
}

void LexerInputExImpl::InitStatus()
{
    InitStreamStatus();
    include_map_.clear();
    command_desc_ptr_ = nullptr;
    main_module_name_.clear();
    search_modules_path_.clear();
    is_auto_scan_include_modules_ = false;
    is_include_map_changed_ = false;
}

LexerInputExImpl::LexerInputExImpl(initializer_list<pair<string, string>> module_list)
{ // Первый элемент пары - имя модуля, второй элемент - его тело.
    for (const auto& current_module_pair : module_list)
    {
        if (current_module_pair.first.find_first_not_of(SPACES_SYMBS_STR) == string::npos)
            throw runtime_error(EMPTY_MODULE_NAME_ERR);
        if (main_module_name_.empty())
            main_module_name_ = current_module_pair.first;

        ModuleDescType module_desc;
        module_desc.module_id = ++last_module_id_;
        module_desc.module_name = current_module_pair.first;
        module_desc.module_path.clear();
        module_desc.module_body = current_module_pair.second;
        module_desc.module_is_active = true;
        module_desc.module_is_main = (current_module_pair.first == main_module_name_);

        include_map_[current_module_pair.first] = move(module_desc);
    }
    is_include_map_changed_ = true;
}

LexerInputExImpl& LexerInputExImpl::operator=(const LexerInputExImpl& other)
{
    if (this != &other)
    {
        InitStatus(); // Устанавливаем наш (целевой объект присваивания) объект в начальное состояние
        // Копируем общие (управляемые) параметры конфигурации в поля нашего объекта
        include_map_ = other.include_map_;
        main_module_name_ = other.main_module_name_;
        search_modules_path_ = other.search_modules_path_;
        is_auto_scan_include_modules_ = other.is_auto_scan_include_modules_;
        is_include_map_changed_ = other.is_include_map_changed_;
    }

    return *this;
}

LexerInputExImpl& LexerInputExImpl::operator=(LexerInputExImpl&& other)
{
    if (this != &other)
    {
        InitStatus(); // Устанавливаем наш (целевой объект присваивания) объект в начальное состояние
        // Переносим общие (управляемые) параметры конфигурации в поля нашего объекта
        include_map_ = move(other.include_map_);
        main_module_name_ = move(other.main_module_name_);
        search_modules_path_ = move(other.search_modules_path_);
        is_auto_scan_include_modules_ = other.is_auto_scan_include_modules_;
        is_include_map_changed_ = other.is_include_map_changed_;
        other.InitStatus(); // Сбрасываем в начальное положение объект-источник
    }

    return *this;
}

void LexerInputExImpl::AddIncludeModule(const string& module_name, const string& module_body,
                                        const path& module_path)
{
    if (module_name.find_first_not_of(SPACES_SYMBS_STR) == string::npos)
        throw runtime_error(EMPTY_MODULE_NAME_ERR);
    if (main_module_name_.empty())
        main_module_name_ = module_name;

    ModuleDescType module_desc;
    module_desc.module_id = ++last_module_id_;
    module_desc.module_name = module_name;
    module_desc.module_path = module_path / path(module_name);
    module_desc.module_body = module_body;
    module_desc.module_is_active = true;
    module_desc.module_is_main = (module_name == main_module_name_);

    include_map_[module_name] = move(module_desc);
    is_include_map_changed_ = true;
}

bool LexerInputExImpl::EraseIncludeModule(const std::string& module_name)
{
    if (!include_map_.count(module_name))
        return false;
    if (main_module_name_ == module_name)
        main_module_name_.clear();
    return include_map_.erase(module_name);
}

bool LexerInputExImpl::RenameIncludeModule(const std::string& old_module_name, const std::string& new_module_name)
{
    if (!include_map_.count(old_module_name))
        return false;

    ModuleDescType& old_module_desc = include_map_[old_module_name];
    ModuleDescType new_module_desc;

    new_module_desc.module_id = old_module_desc.module_id;
    new_module_desc.module_name = new_module_name;
    new_module_desc.module_path = move(old_module_desc.module_path);
    new_module_desc.module_is_active = old_module_desc.module_is_active;
    new_module_desc.module_is_main = old_module_desc.module_is_main;
    new_module_desc.module_body = move(old_module_desc.module_body);
    if (main_module_name_ == old_module_name)
        main_module_name_ = new_module_name;
    include_map_.erase(old_module_name);
    include_map_[new_module_name] = move(new_module_desc);
    return true;
}

void LexerInputExImpl::IncludeSwitchTo(string include_arg)
{
    if (!include_arg.size())
    { // Инициализирующий вызов IncludeSwitchTo()
        eof_bit_ = false;
        last_read_symb_ = std::char_traits<char>::eof();
        unget_symb_ = std::char_traits<char>::eof();
        current_position_ = 0;
        current_module_name_.clear();
        include_stack_.clear();
        include_arg = main_module_name_;
    }

    if (!include_arg.size())
        throw ParseError("Отсутствует стартовый модуль"s);

    if (!include_map_.count(include_arg))
    {  // Обрабатываем случай, если целевой модуль include_arg пока не существует
        path test_module_path = search_modules_path_ / path(include_arg);
        if (!is_auto_scan_include_modules_ || !exists(test_module_path))
            // Если автопоиск модулей запрещён или не удался, выбрасываем исключение
            throw ParseError("Включаемая часть "s + include_arg + " не найдена"s);

        // Файл модуля обнаружен, пробуем прочитать его
        ifstream ifile(test_module_path);
        if (!ifile)
            throw ParseError("Ошибка при открытии файла включаемой части "s + include_arg);

        string module_data{std::istreambuf_iterator<char>(ifile), std::istreambuf_iterator<char>()};

        if (!ifile.good() && !ifile.eof())
            throw ParseError("Ошибка при чтении файла включаемой части "s + include_arg);
        // Модуль успешно найден и считан. добавляем его в систему хранения.
        ModuleDescType module_desc;
        module_desc.module_id = ++last_module_id_;
        module_desc.module_name = include_arg;
        module_desc.module_path = move(test_module_path);
        module_desc.module_body = move(module_data);
        module_desc.module_is_active = true;
        module_desc.module_is_main = false;

        include_map_[include_arg] = move(module_desc);
        is_include_map_changed_ = true;
    }

    if (!include_map_[include_arg].module_is_active)
        throw ParseError("Модуль "s + include_arg + " не активен");
    // В том случае, если устанавливаемый модуль не является стартовым, сохраним состояние выбывающего модуля в стеке
    if (current_module_name_.size())
        include_stack_.push_back({current_module_name_, current_position_, command_desc_ptr_->module_string_number});

    current_module_name_ = include_arg;
    current_module_desc_ptr_ = &include_map_[current_module_name_];
    current_position_ = 0;
    command_desc_ptr_->module_id = current_module_desc_ptr_->module_id;
    command_desc_ptr_->module_string_number = 0;
}

const LexerInputExImpl::ModuleDescType* LexerInputExImpl::GetModuleDescriptor(const std::string& module_name) const
{
    if (include_map_.count(module_name))
        return &include_map_.at(module_name);
    else
        return nullptr;
}

const LexerInputExImpl::ModuleDescType* LexerInputExImpl::GetModuleDescriptor(size_t module_number) const
{
    if (module_number >= 0 && module_number < include_map_.size())
    {
        auto include_map_iter = include_map_.cbegin();
        advance(include_map_iter, module_number);
        return &include_map_iter->second;
    }
    else
    {
        return nullptr;
    }
}

void LexerInputExImpl::UnmainCurrentMainModule()
{ // Снимаем существующий главный модуль с его поста. Чиновника - в отставку!
    if (!main_module_name_.empty() && include_map_.count(main_module_name_))
    {
        include_map_.at(main_module_name_).module_is_main = false;
        main_module_name_.clear();
    }
}

bool LexerInputExImpl::SetModuleWithBody(LexerInputExImpl::ModuleDescType module_desc)
{ // Функция-член добавляет (или заменяет существующий) модуль вместе с его телом
    string include_map_key = module_desc.module_name;
    if (module_desc.module_is_main)
    { // Если устанавливается новый стартовый (главный) модуль - удаляем это свойство у уже существующего.
        UnmainCurrentMainModule();
        main_module_name_ = module_desc.module_name;
    }

    bool is_insert = include_map_.insert_or_assign(include_map_key, move(module_desc)).second;
    InitStreamStatus();
    is_include_map_changed_ = true;
    return is_insert;
}

bool LexerInputExImpl::FixModuleDesc(const LexerInputExImpl::ModuleDescType& new_module_desc)
{ // Функция член исправляет поля описателя существующего модуля, кроме полей тела и маршрута
  // (тело модуля здесь не меняется, поэтому маршрут к его файлу тоже оставим в неприкосновенности).
    if (!include_map_.count(new_module_desc.module_name))
        return false;
    ModuleDescType& old_module_desc = include_map_[new_module_desc.module_name];

    if (old_module_desc.module_is_main && !new_module_desc.module_is_main)
    { // Снимаем главенство с модуля
        UnmainCurrentMainModule();
        old_module_desc.module_is_main = false;
    }
    else if (!old_module_desc.module_is_main && new_module_desc.module_is_main)
    { // Делаем модуль главным
        UnmainCurrentMainModule();
        main_module_name_ = new_module_desc.module_name;
        old_module_desc.module_is_main = true;
    }

    old_module_desc.module_id = new_module_desc.module_id;
    old_module_desc.module_is_active = new_module_desc.module_is_active;
    return true;
}

LexerInputExImpl& LexerInputExImpl::SetMainModuleName(const std::string& main_module_name)
{    
    UnmainCurrentMainModule(); // Устанавливается новый стартовый (главный) модуль - сначала
                               // удаляем это свойство у уже существующего.
    if (include_map_.count(main_module_name))
    { // Если модуль с указанным именем существует - делаем его главным.
      // Иначе система останется без точки запуска.
        main_module_name_ = main_module_name;
        include_map_.at(main_module_name_).module_is_main = true;
    }

    return *this;
}

int LexerInputExImpl::get()
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
        if (current_position_ < static_cast<int>(current_module_desc_ptr_->module_body.size()))
        {
            last_read_symb_ = current_module_desc_ptr_->module_body[current_position_];
            ++current_position_;
            break;
        }
        else
        {
            if (include_stack_.size())
            {
                StackType stack_rec = include_stack_.back();
                include_stack_.pop_back();
                current_module_name_ = stack_rec.module_name;
                current_position_ = stack_rec.module_position;
                current_module_desc_ptr_ = &include_map_[current_module_name_];
                command_desc_ptr_->module_id = current_module_desc_ptr_->module_id;
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

int LexerInputExImpl::peek()
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
        --current_position_;
        return last_read_symb_;
    }
    else
    {
        return char_traits<char>::eof();
    }
}

LexerInputExImpl& LexerInputExImpl::unget()
{
    if (last_read_symb_ != char_traits<char>::eof())
    {
        unget_symb_ = last_read_symb_;
        eof_bit_ = false;
    }
    return *this;
}

const BreakpointsContainer::BreakpointDescType*
    BreakpointsContainer::GetBreakpointDescriptor(int breakpoint_id) const
{
    if (breakpoint_descs_.count(breakpoint_id))
        return &breakpoint_descs_.at(breakpoint_id);
    else
        return nullptr;
}

const BreakpointsContainer::BreakpointDescType*
    BreakpointsContainer::GetBreakpointDescriptor(size_t breakpoint_number) const
{
    if (breakpoint_number >= 0 && breakpoint_number < breakpoint_descs_.size())
    {
        auto breakpoint_descs_iter = breakpoint_descs_.cbegin();
        advance(breakpoint_descs_iter, breakpoint_number);
        return &breakpoint_descs_iter->second;
    }
    else
    {
        return nullptr;
    }
}

bool BreakpointsContainer::SetBreakpointDescriptor(BreakpointsContainer::BreakpointDescType breakpoint_desc)
{
    int breakpoint_desc_key = breakpoint_desc.breakpoint_id;
    bool is_insert = breakpoint_descs_.insert_or_assign(breakpoint_desc_key, move(breakpoint_desc)).second;
    is_break_list_changed_ = true;
    return is_insert;
}

const WatchesContainer::WatchDescType*
    WatchesContainer::GetWatchDescriptor(int watch_id) const
{
    if (watch_descs_.count(watch_id))
        return &watch_descs_.at(watch_id);
    else
        return nullptr;
}

const WatchesContainer::WatchDescType*
    WatchesContainer::GetWatchDescriptor(size_t watch_number) const
{
    if (watch_number >= 0 && watch_number < watch_descs_.size())
    {
        auto watch_descs_iter = watch_descs_.cbegin();
        advance(watch_descs_iter, watch_number);
        return &watch_descs_iter->second;
    }
    else
    {
        return nullptr;
    }
}

void WatchesContainer::Clear()
{
    free_ids_.clear();
    last_used_id_ = 0;
    watch_descs_.clear();
    is_watch_list_changed_ = false;
}

int WatchesContainer::AllocId()
{
    if (free_ids_.size())
    {
        int allocated_id = *free_ids_.begin();
        free_ids_.erase(allocated_id);
        return allocated_id;
    }
    else
    {
        return ++last_used_id_;
    }
}

bool WatchesContainer::SetWatchDescriptor(WatchesContainer::WatchDescType watch_desc)
{
    int watch_desc_key = watch_desc.watch_id;
    bool is_insert = watch_descs_.insert_or_assign(watch_desc_key, move(watch_desc)).second;
    is_watch_list_changed_ = true;
    return is_insert;
}

int WatchesContainer::AddWatchDescriptor(WatchesContainer::WatchDescType watch_desc)
{
    int allocated_id = AllocId();
    watch_desc.watch_id = allocated_id;
    watch_descs_.insert({allocated_id, move(watch_desc)});
    is_watch_list_changed_ = true;
    return allocated_id;
}

bool WatchesContainer::EraseWatchDescriptor(int watch_id)
{
    if (watch_descs_.erase(watch_id))
    {
        FreeId(watch_id);
        is_watch_list_changed_ = true;
        return true;
    }
    return false;
}
