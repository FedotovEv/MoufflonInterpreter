
#include "MythonDebugger.h"
#include "gui/MainDebuggerWindow.h"

#include <wx/image.h>
#include <wx/cshelp.h>
#include <wx/html/helpctrl.h>
#include <wx/cmdline.h>
#include <wx/print.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/xrc/xmlres.h>

#include "wx/filesys.h"
#include "wx/fs_zip.h"

wxCmdLineEntryDesc const static cmd_line_desc[] =
{
    {wxCMD_LINE_SWITCH, "h", "help", "вывод помощи по формату командной строки программы",
     wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP},
    {wxCMD_LINE_SWITCH, "f", "full-save", "сохранение исходников в файле проекта"},
    {wxCMD_LINE_SWITCH, "u", "utf8", "исходники в кодировке utf-8"},
    {wxCMD_LINE_PARAM, NULL, NULL, "входной файл", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
    {wxCMD_LINE_NONE}
};

IMPLEMENT_APP(MythonDebuggerApp)

MythonDebuggerApp::MythonDebuggerApp()
{}

MythonDebuggerApp::~MythonDebuggerApp()
{}

void MythonDebuggerApp::RecreateGUI()
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    MainDebuggerWindowImpl* mainframe = new MainDebuggerWindowImpl((wxWindow*)NULL, this_app->debugger_project);
    mainframe->Centre();
    mainframe->Show();
    SetTopWindow(mainframe);
    setlocale(LC_NUMERIC, "C");
}

void MythonDebuggerApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc(cmd_line_desc);
    // Используем для предварения параметров символ '-', чтобы не путать его с маршрутами файлов
    parser.SetSwitchChars(wxT("-"));
}

bool MythonDebuggerApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    options_data.is_save_module_body = parser.Found(wxT("f"));
    options_data.is_source_utf8 = parser.Found(wxT("u"));

    for (size_t i = 0; i < parser.GetParamCount(); ++i)
        options_data.option_filename.push_back(parser.GetParam(i));

    return true;
}

bool MythonDebuggerApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    MainDebuggerWindowImpl* mainframe = new MainDebuggerWindowImpl((wxWindow*)NULL, this_app->debugger_project);
    mainframe->Show();
    SetTopWindow(mainframe);

    wxHelpControllerHelpProvider* provider = new wxHelpControllerHelpProvider;
    wxHelpProvider::Set(provider);
    provider->SetHelpController(&HtmlHelp);
    HtmlHelp.AddBook(wxFileName(wxT("MythonDebuggerHelp.zip")));
    setlocale(LC_NUMERIC, "C");

    return true;
}
