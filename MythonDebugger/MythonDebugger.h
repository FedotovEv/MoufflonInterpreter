#pragma once

#include "resource.h"
#include "DebuggerDataStruct.h"
#include "DebuggerExecutor.h"

#include <vector>
#include <filesystem>

#include <wx/wx.h>
#include <wx/app.h>
#include <wx/cshelp.h>
#include <wx/html/helpctrl.h>
#include <wx/cmdline.h>
#include <wx/print.h>
#include <wx/filesys.h>
#include <wx/fs_zip.h>
#include <wx/intl.h>
#include <wx/fontdata.h>
#include <wx/event.h>

wxDECLARE_EVENT(DEBUG_EVENT_TYPE, wxCommandEvent);

#define FULLVERSION_STRING "0.0.3.40"

class ModuleDescClientData : public wxClientData
{
public:
    ModuleDescClientData(const LexerInputExImpl::ModuleDescType& module_desc) :
        module_desc_(module_desc)
    {}

    const LexerInputExImpl::ModuleDescType& GetModuleDesc() const
    {
        return module_desc_;
    }

private:
    const LexerInputExImpl::ModuleDescType& module_desc_;
};

class BreakpointDescClientData : public wxClientData
{
public:
    BreakpointDescClientData(const BreakpointsContainer::BreakpointDescType& breakpoint_desc) :
        breakpoint_desc_(breakpoint_desc)
    {}

    const BreakpointsContainer::BreakpointDescType& GetBreakpointDesc() const
    {
        return breakpoint_desc_;
    }

private:
    const BreakpointsContainer::BreakpointDescType& breakpoint_desc_;
};

class WatchDescClientData : public wxClientData
{
public:
    WatchDescClientData(const WatchesContainer::WatchDescType& watch_desc) :
        watch_desc_(watch_desc)
    {}

    const WatchesContainer::WatchDescType& GetWatchDesc() const
    {
        return watch_desc_;
    }

private:
    const WatchesContainer::WatchDescType& watch_desc_;
};

struct LanguageDescriptType
{
    wxLanguage language_identifier;
    wxString language_name;
    std::filesystem::path language_path;
    std::filesystem::path mo_file_path;
    long menu_item_id;
    wxMenuItem* menu_item_ptr = nullptr;
};

struct OptionsData
{
    // Имена файлов, которые могут быть либо исходными модулями, либо именем проекта.
    // Имя проекта может быть только одно, а исходников можно перечислить несколько.
    std::vector<wxString> option_filename;
    bool is_save_module_body = false;
    bool is_source_utf8 = true;
};

class MythonDebuggerApp : public wxApp
{
public:
    // Глобальные данные программы, описывающие её состояние
    int return_code = 0;
    wxFontData font_data;
    // Значения параметров настройки программы (которые можно указать в диалоге настройки)
    OptionsData options_data;
    // Массив поддерживаемых (русский плюс найденные при сканировании) программой языков
    std::vector<LanguageDescriptType> languages_descs;
    wxHtmlHelpController HtmlHelp;
    // Указатель на используемую нами локаль
    wxLocale* m_locale = nullptr;
    // Текущий открытый отладочный проект
    DebuggerProject debugger_project;

    MythonDebuggerApp();
    virtual ~MythonDebuggerApp();

    void RecreateGUI();
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);    
    virtual bool OnInit();
};

DECLARE_APP(MythonDebuggerApp)
