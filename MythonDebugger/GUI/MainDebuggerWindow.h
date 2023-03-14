#ifndef __MainDebuggerWindow__
#define __MainDebuggerWindow__

#include "MythonDebuggerGui.h"
#include "../redefine_.h""

class MainDebuggerWindowImpl : public MainDebuggerWindow
{
protected:
	// Handlers for MainDebuggerWindow events.
	void ModuleListOnListBox( wxCommandEvent& event );
	void ModuleListOnListBoxDClick( wxCommandEvent& event );
	void SourceViewerOnKeyDown( wxKeyEvent& event );
	void SourceViewerOnLeftDown( wxMouseEvent& event );
	void SourceViewerOnRightDown( wxMouseEvent& event );
	void BreakPointsListOnListBox( wxCommandEvent& event );
	void BreakPointsListOnListBoxDClick( wxCommandEvent& event );
	void SymbolsListOnListBox( wxCommandEvent& event );
	void SymbolsListOnListBoxDClick( wxCommandEvent& event );
	void StackListOnListBox( wxCommandEvent& event );
	void StackListOnListBoxDClick( wxCommandEvent& event );
	void CreateProjectOnMenuSelection( wxCommandEvent& event );
	void FileLoadProjectOnMenuSelection( wxCommandEvent& event );
    void FileSaveProjectOnMenuSelection(wxCommandEvent& event);
    void FileSaveAsProjectOnMenuSelection(wxCommandEvent& event);
	void FileAddOneFileOnMenuSelection( wxCommandEvent& event );
	void FileExitProgramOnMenuSelection( wxCommandEvent& event );
    void ViewOutputWindowOnMenuSelection(wxCommandEvent& event);
	void ViewConfigOnMenuSelection( wxCommandEvent& event );
	void TraceRunWithDebugOnMenuSelection( wxCommandEvent& event );
	void TraceRunFreeOnMenuSelection( wxCommandEvent& event );
	void TraceInOnMenuSelection( wxCommandEvent& event );
	void TraceOutOnMenuSelection( wxCommandEvent& event );
	void TraceToCursorOnMenuSelection( wxCommandEvent& event );
	void TraceExitFromMethodOnMenuSelection( wxCommandEvent& event );
	void TraceStopProgramOnMenuSelection( wxCommandEvent& event );
	void TraceFinishProgramOnMenuSelection( wxCommandEvent& event );
	void BreakpointCreateOnMenuSelection( wxCommandEvent& event );
	void BreakpointCreateConditionalOnMenuSelection( wxCommandEvent& event );
	void BreakpointDeleteOnMenuSelection( wxCommandEvent& event );
	void BreakpointDeleteAllOnMenuSelection( wxCommandEvent& event );
	void BreakpointOnOnMenuSelection( wxCommandEvent& event );
	void BreakpointOffOnMenuSelection( wxCommandEvent& event );
	void BreakpointEnableAllOnMenuSelection( wxCommandEvent& event );
	void BreakpointDisableAllOnMenuSelection( wxCommandEvent& event );
	void BreakpointToggleOnMenuSelection( wxCommandEvent& event );
	void WatchCreateOnMenuSelection( wxCommandEvent& event );
	void WatchDeleteOnMenuSelection( wxCommandEvent& event );
	void SymbolsSaveOnMenuSelection( wxCommandEvent& event );
	void HelpIndexOnMenuSelection( wxCommandEvent& event );
	void HelpAboutOnMenuSelection( wxCommandEvent& event );
	void ToolCreateProjectOnToolClicked( wxCommandEvent& event );
	void ToolOpenProjectOnToolClicked( wxCommandEvent& event );
    void ToolSaveProjectOnToolClicked(wxCommandEvent& event);
    void ToolSaveAsProjectOnToolClicked(wxCommandEvent& event);
	void ToolAddOneFileOnToolClicked( wxCommandEvent& event );
	void ToolExitProgramOnToolClicked( wxCommandEvent& event );
	void ToolRunWithDebugOnToolClicked( wxCommandEvent& event );
	void ToolRunFreeOnToolClicked(wxCommandEvent& event);
	void ToolTraceInOnToolClicked(wxCommandEvent& event);
	void ToolTraceOutOnToolClicked(wxCommandEvent& event);
	void ToolTraceToCursorOnToolClicked(wxCommandEvent& event);
	void ToolExitFromMethodOnToolClicked(wxCommandEvent& event);
	void ToolStopProgramOnToolClicked(wxCommandEvent& event);
	void ToolFinishProgramOnToolClicked(wxCommandEvent& event);
	void ToolCreateBreakpointOnToolClicked(wxCommandEvent& event);
	void ToolDeleteBreakpointOnToolClicked(wxCommandEvent& event);
	void ToolToggleBreakpointOnToolClicked(wxCommandEvent& event);
	void ToolBreakpointOnOnToolClicked(wxCommandEvent& event);
	void ToolBreakpointOffOnToolClicked(wxCommandEvent& event);
	void ToolHelpOnToolClicked(wxCommandEvent& event);
    void DebugEventHandler(wxCommandEvent& event);
    void MainDebuggerWindowOnClose(wxCloseEvent& event);

public:
	MainDebuggerWindowImpl(wxWindow* parent, DebuggerProject& debugger_project);

private:
    static const int MARKER_CURRENT_POINT = 5; // Номер маркёра для обозначения текущей исполняемой команды
    static const int MARKER_SIMPLE_BREAK_POINT = 6; // Маркёр обычной точки останова
    static const int MARKER_COND_BREAK_POINT = 7; // Маркёр условной точки останова
    static const int MARKER_BOOKMARK = 8; // Маркёр для обозначения закладок

    wxString main_debugger_window_name_;
    // Контроллер отладки, который мы будем использовать для работы
    DebugController debug_controller_;
    std::thread debug_thread_;
    int current_module_id_ = wxNOT_FOUND; // id текущего модуля, загруженного в просмотрщик
    runtime::ProgramCommandDescriptor current_point_; // Текущая исполняемая строка (обновляется при останове
                                                      // отладчика по любой причине).

    std::string FormatListBoxItem(const LexerInputExImpl::ModuleDescType& module_desc);
    void FillProjectListBox();
    bool CheckProjectModifyStatus();
    bool CheckDebugInProcess();
    void SetViewerModuleText(int new_selection);
    void SetWindowLabel();
    void IndicateControllerResult(DebugController::ControllerResult& current_debug_result);
    int ScanSelectionByModuleId(int module_id);
    bool IsNextStepPossible();
    std::pair<bool, wxString> DoLoadProject(const wxString& project_filename);
};

#endif // __MainDebuggerWindow__
