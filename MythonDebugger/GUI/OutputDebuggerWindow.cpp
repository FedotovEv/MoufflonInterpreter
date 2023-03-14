
#include "../MythonDebugger.h"
#include "OutputDebuggerWindow.h"

OutputDebuggerWindowImpl::OutputDebuggerWindowImpl(wxWindow* parent) :
    OutputDebuggerWindow(parent)
{
    output_debugger_window_name_ = _("Отладчик МуфлоЖук - консоль отладки");
    target_file_ = "noname.txt";
    SetWindowLabel();
}

void OutputDebuggerWindowImpl::SetWindowLabel()
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    SetLabel(output_debugger_window_name_ + wxT(" - ") + target_file_);
}

void OutputDebuggerWindowImpl::DebugOutputSaveOnMenuSelection(wxCommandEvent& event)
{
    DebugOutputText->SaveFile(target_file_);
}

void OutputDebuggerWindowImpl::DebugOutputSaveAsOnMenuSelection(wxCommandEvent& event)
{
    wxFileDialog save_as_file_dialog(this, _("Сохранить содержимое консоли как"),
        wxEmptyString, wxEmptyString, _("Текстовые файлы (*.txt)|*.txt|Все файлы|*.*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (save_as_file_dialog.ShowModal() == wxID_CANCEL)
        return;

    target_file_ = save_as_file_dialog.GetPath();
    SetWindowLabel();
    DebugOutputSaveOnMenuSelection(event);
}

void OutputDebuggerWindowImpl::DebugOutputClearOnMenuSelection(wxCommandEvent& event)
{
    DebugOutputText->Clear();
}

void OutputDebuggerWindowImpl::DebugCloseOnMenuSelection(wxCommandEvent& event)
{
    Hide();
}

void OutputDebuggerWindowImpl::ToolDebugOutputSaveOnToolClicked(wxCommandEvent& event)
{
    DebugOutputSaveOnMenuSelection(event);
}

void OutputDebuggerWindowImpl::ToolDebugOutputSaveAsOnToolClicked(wxCommandEvent& event)
{
    DebugOutputSaveAsOnMenuSelection(event);
}

void OutputDebuggerWindowImpl::ToolDebugClearWindowOnToolClicked(wxCommandEvent& event)
{
    DebugOutputClearOnMenuSelection(event);
}

void OutputDebuggerWindowImpl::ToolDebugCloseWindowOnToolClicked(wxCommandEvent& event)
{
    DebugCloseOnMenuSelection(event);
}

void OutputDebuggerWindowImpl::OutputDebuggerWindowOnClose(wxCloseEvent& event)
{
     Hide();
}
