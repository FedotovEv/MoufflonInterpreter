#pragma once

#include "../lexer.h"
#include "../parse.h"
#include "../runtime.h"
#include "../statement.h"
#include "../debug_context.h"

#include <iostream>
#include <string>
#include <variant>
#include <unordered_map>
#include <cstdio>
#include <stdexcept>
#include <iterator>
#include <filesystem>

using namespace std::literals;

class LexerInputExImpl : public parse::LexerInputEx
{ // Класс диспетчера исходных модулей, хранящихся в виде строковых переменных
public:
    class iterator;
    friend class iterator;

    struct ModuleDescType
    {
        int module_id;
        std::string module_name;
        std::filesystem::path module_path;        
        std::string module_body;
    };

    struct StackType
    {
        std::string module_name;
        int module_position;
        int module_string_number;
    };

    void InitStreamStatus();
    void InitStatus();

    LexerInputExImpl() = default;
    LexerInputExImpl(std::initializer_list<std::pair<std::string, std::string>> module_list);
    LexerInputExImpl(const LexerInputExImpl& other) :
        include_map_(other.include_map_), main_module_name_(other.main_module_name_),
        search_modules_path_(other.search_modules_path_),
        is_auto_scan_include_modules_(other.is_auto_scan_include_modules_),
        is_include_map_changed_(other.is_include_map_changed_)
    {}

    LexerInputExImpl(LexerInputExImpl&& other) :
        include_map_(std::move(other.include_map_)),
        main_module_name_(std::move(other.main_module_name_)),
        search_modules_path_(std::move(other.search_modules_path_)),
        is_auto_scan_include_modules_(other.is_auto_scan_include_modules_),
        is_include_map_changed_(other.is_include_map_changed_)
    {
        other.InitStatus();
    }

    ~LexerInputExImpl() = default;

    LexerInputExImpl& operator=(const LexerInputExImpl& other);
    LexerInputExImpl& operator=(LexerInputExImpl&& other);

    void AddIncludeModule(const std::string& module_name, const std::string& module_body,
                          const std::filesystem::path& module_path = {});

    const ModuleDescType* GetModuleDescriptor(const std::string& module_name) const;
    const ModuleDescType* GetModuleDescriptor(size_t module_number) const;
    bool SetModuleDescriptor(ModuleDescType module_desc);

    LexerInputExImpl& SetMainModuleName(const std::string& main_module_name)
    {
        main_module_name_ = main_module_name;
        return *this;
    }

    std::string GetMainModuleName()
    {
        return main_module_name_;
    }

    LexerInputExImpl& SetSearchModulesPath(const std::filesystem::path& search_modules_path)
    {
        search_modules_path_ = search_modules_path;
        return *this;
    }

    std::filesystem::path GetSearchModulesPath()
    {
        return search_modules_path_;
    }

    bool IsIncludeMapChanged() const
    {
        return is_include_map_changed_;
    }

    void ClearChangeFlag()
    {
        is_include_map_changed_ = false;
    }

    LexerInputExImpl& SetAutoScanMode(bool is_auto_scan_include_modules)
    {
        is_auto_scan_include_modules_ = is_auto_scan_include_modules;
        return *this;
    }

    void SetCommandDescPtr(runtime::ProgramCommandDescriptor* command_desc_ptr) override
    {
        command_desc_ptr_ = command_desc_ptr;
    }

    void IncludeSwitchTo(std::string include_arg) override;

    int get() override;
    int peek() override;
    LexerInputExImpl& unget() override;

    void Clear()
    {
        InitStatus();
        is_include_map_changed_ = false;
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
    // Поля для обслуживания ostream-функций
    bool eof_bit_ = false;
    int last_read_symb_ = std::char_traits<char>::eof();
    int unget_symb_ = std::char_traits<char>::eof();
    // Поля хранения текущего состояния диспетчера
    int current_position_ = 0;
    ModuleDescType* current_module_desc_ptr_ = nullptr;
    std::string current_module_name_;
    std::vector<StackType> include_stack_;
    // Управляемые параметры конфигурации диспетчера
    std::unordered_map<std::string, ModuleDescType> include_map_;
    runtime::ProgramCommandDescriptor* command_desc_ptr_ = nullptr;
    std::string main_module_name_;
    std::filesystem::path search_modules_path_;
    bool is_auto_scan_include_modules_ = false;
    bool is_include_map_changed_ = false;

    inline static int last_module_id_ = 0;

public:
    class iterator: public std::iterator<std::forward_iterator_tag, ModuleDescType>
    {
    public:
        friend class LexerInputExImpl;

        iterator(const LexerInputExImpl& src_object_ref, bool is_end_iterator) :
            src_object_ref_(src_object_ref)
        {
            if (is_end_iterator)
                loc_iter_ = src_object_ref_.include_map_.cend();
            else
                loc_iter_ = src_object_ref_.include_map_.cbegin();
        }

        iterator(const iterator& other) = default;
        iterator& operator=(const iterator& other) = default;

        const ModuleDescType& operator*() const
        {
            return loc_iter_->second;
        }

        const ModuleDescType* operator->() const
        {
            return &loc_iter_->second;
        }

        iterator& operator++()
        {
            ++loc_iter_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old_value(*this);
            ++(*this);
            return old_value;
        }

        bool operator==(const iterator& other) const
        {
            return loc_iter_ == other.loc_iter_;
        }

        bool operator!=(const iterator& other) const
        {
            return loc_iter_ != other.loc_iter_;
        }

    private:
        const LexerInputExImpl& src_object_ref_;
        decltype(LexerInputExImpl::include_map_)::const_iterator loc_iter_;
    };

    size_t size() const
    {
        return include_map_.size();
    }

    iterator begin() const
    {
        return iterator(*this, false);
    }

    iterator end() const
    {
        return iterator(*this, true);
    }

    bool erase(iterator& lexer_iterator)
    {
        include_map_.erase(lexer_iterator.loc_iter_);
        InitStreamStatus();
        is_include_map_changed_ = true;
    }
};

class BreakpointsContainer
{
public:
    class iterator;
    friend class iterator;

    struct BreakpointDescType
    {
        int breakpoint_id;
        runtime::ProgramCommandDescriptor breakpoint_point; // Положение точки останова в программе
        std::string conditional_expression; // Текстовая запись условия в инфиксной форме
        int start_counter = 0; // Значение счётчика срабатываний, после которого должен произойти останов
        int current_counter = 0; // Текущее значение счётчика срабатываний. Поля обслуживания счётчика
                                 // имеют смысл только для "счётной" точки останова при is_counting == false.
        bool is_active = false; // Флаг активности точки останова
        bool is_conditional = false; // Признак условной точки останова
        bool is_onetime = false; // Признак однократной точки останова
        bool is_counting = false; // Флаг "счётной" точки
    };

    const BreakpointDescType* GetBreakpointDescriptor(int breakpoint_id) const;
    const BreakpointDescType* GetBreakpointDescriptor(size_t breakpoint_number) const;
    bool SetBreakpointDescriptor(BreakpointDescType breakpoint_desc);

    bool IsBreakListChanged() const
    {
        return is_break_list_changed_;
    }

    void ClearChangeFlag()
    {
        is_break_list_changed_ = false;
    }

    void Clear()
    {
        breakpoint_descs_.clear();
        is_break_list_changed_ = false;
    }

private:
    std::unordered_map<int, BreakpointDescType> breakpoint_descs_;
    bool is_break_list_changed_ = false;

public:
    class iterator: public std::iterator<std::forward_iterator_tag, BreakpointDescType>
    {
    public:
        friend class BreakpointsContainer;

        iterator(const BreakpointsContainer& src_object_ref, bool is_end_iterator) :
            src_object_ref_(src_object_ref)
        {
            if (is_end_iterator)
                break_iter_ = src_object_ref_.breakpoint_descs_.cend();
            else
                break_iter_ = src_object_ref_.breakpoint_descs_.cbegin();
        }

        iterator(const iterator& other) = default;
        iterator& operator=(const iterator& other) = default;

        const BreakpointDescType& operator*() const
        {
            return break_iter_->second;
        }

        const BreakpointDescType* operator->() const
        {
            return &break_iter_->second;
        }

        iterator& operator++()
        {
            ++break_iter_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old_value(*this);
            ++(*this);
            return old_value;
        }

        bool operator==(const iterator& other) const
        {
            return break_iter_ == other.break_iter_;
        }

        bool operator!=(const iterator& other) const
        {
            return break_iter_ != other.break_iter_;
        }

    private:
        const BreakpointsContainer& src_object_ref_;
        decltype(BreakpointsContainer::breakpoint_descs_)::const_iterator break_iter_;
    };

    size_t size()
    {
        return breakpoint_descs_.size();
    }

    iterator begin()
    {
        return iterator(*this, false);
    }

    iterator end()
    {
        return iterator(*this, true);
    }

    bool erase(iterator& break_iterator)
    {
        breakpoint_descs_.erase(break_iterator.break_iter_);
        is_break_list_changed_ = true;
    }
};

class WatchesContainer
{
public:
    class iterator;
    friend class iterator;

    struct WatchDescType
    {
        int watch_id; // Уникальный идентификатор надзорной записи
        std::string watch_symbol_name; // Имя символа, над которым установлен надзор
        bool is_active = false; // Флаг активности надзорной записи
    };

    const WatchDescType* GetWatchDescriptor(int watch_id) const;
    const WatchDescType* GetWatchDescriptor(size_t watch_number) const;
    bool SetWatchDescriptor(WatchDescType watch_desc);

    bool IsWatchListChanged() const
    {
        return is_watch_list_changed_;
    }

    void ClearChangeFlag()
    {
        is_watch_list_changed_ = false;
    }

    void Clear()
    {
        watch_descs_.clear();
        is_watch_list_changed_ = false;
    }

private:
    std::unordered_map<int, WatchDescType> watch_descs_;
    bool is_watch_list_changed_ = false;

public:
    class iterator: public std::iterator<std::forward_iterator_tag, WatchDescType>
    {
    public:
        friend class WatchesContainer;

        iterator(const WatchesContainer& src_object_ref, bool is_end_iterator) :
            src_object_ref_(src_object_ref)
        {
            if (is_end_iterator)
                watch_iter_ = src_object_ref_.watch_descs_.cend();
            else
                watch_iter_ = src_object_ref_.watch_descs_.cbegin();
        }

        iterator(const iterator& other) = default;
        iterator& operator=(const iterator& other) = default;

        const WatchDescType& operator*() const
        {
            return watch_iter_->second;
        }

        const WatchDescType* operator->() const
        {
            return &watch_iter_->second;
        }

        iterator& operator++()
        {
            ++watch_iter_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator old_value(*this);
            ++(*this);
            return old_value;
        }

        bool operator==(const iterator& other) const
        {
            return watch_iter_ == other.watch_iter_;
        }

        bool operator!=(const iterator& other) const
        {
            return watch_iter_ != other.watch_iter_;
        }

    private:
        const WatchesContainer& src_object_ref_;
        decltype(WatchesContainer::watch_descs_)::const_iterator watch_iter_;
    };

    size_t size()
    {
        return watch_descs_.size();
    }

    iterator begin()
    {
        return iterator(*this, false);
    }

    iterator end()
    {
        return iterator(*this, true);
    }

    bool erase(iterator& watch_iterator)
    {
        watch_descs_.erase(watch_iterator.watch_iter_);
        is_watch_list_changed_ = true;
    }
};

class DebuggerProject
{
public:
    DebuggerProject() : project_path_(noname_project_)
    {}

    LexerInputExImpl& GetLexerInputStream()
    {
        return module_dispather_;
    }

    BreakpointsContainer& GetBreakpointsContainer()
    {
        return break_container_;
    }

    WatchesContainer& GetWatchesContainer()
    {
        return watch_container_;
    }

    bool IsProjectChanged()
    {
        return module_dispather_.IsIncludeMapChanged() ||
               break_container_.IsBreakListChanged() ||
               watch_container_.IsWatchListChanged();
    }

    void ClearChangeFlag()
    {
        module_dispather_.ClearChangeFlag();
        break_container_.ClearChangeFlag();
        watch_container_.ClearChangeFlag();
    }

    void Clear()
    {
        module_dispather_.Clear();
        break_container_.Clear();
        watch_container_.Clear();
        project_path_ = noname_project_;
    }

    void SetProjectPath(const std::string& project_path)
    {
        project_path_ = project_path;
    }

    std::filesystem::path GetProjectPath()
    {
        return project_path_;
    }

private:
    LexerInputExImpl module_dispather_;
    BreakpointsContainer break_container_;
    WatchesContainer watch_container_;
    std::filesystem::path project_path_;    

    inline static const char* const noname_project_{"noname.xml"};
};
