///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/listbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stc/stc.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/frame.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/stattext.h>

#include "../redefine_.h"

///////////////////////////////////////////////////////////////////////////

#define wxID_MAIN_WINDOW 1000
#define wxID_MODULE_LIST 1001
#define wxID_SOURCE_VIEWER 1002
#define wxID_BREAKPOINTS_LIST 1003
#define wxID_SYMBOLS_LIST 1004
#define wxID_STACK_LIST 1005
#define wxID_MAIN_STATUSBAR 1006
#define wxID_MAIN_MENUBAR 1007
#define wxID_CREATE_NEW_PROJECT 1008
#define wxID_OPEN_PROJECT 1009
#define wxID_SAVE_PROJECT 1010
#define wxID_SAVE_PROJECT_AS 1011
#define wxID_LOAD_ONE_FILE 1012
#define wxID_EXIT_PROGRAM 1013
#define wxID_VIEW_OUTPUT_WINDOW 1014
#define wxID_VIEW_CONFIG 1015
#define wxID_RUN_DEBUG_PROGRAM 1016
#define wxID_RUN_PROGRAM 1017
#define wxID_TRACE_IN 1018
#define wxID_TRACE_OUT 1019
#define wxID_GOTO_TO_CURSOR 1020
#define wxID_EXIT_METHOD 1021
#define wxID_STOP_PROGRAM 1022
#define wxID_FINISH_PROGRAM 1023
#define wxID_CREATE_BREAK 1024
#define wxID_CREATE_COND_BREAK 1025
#define wxID_DELETE_BREAK 1026
#define wxID_DELETE_ALL_BREAK 1027
#define wxID_ENABLE_BREAK 1028
#define wxID_ENABLE_ALL_BREAK 1029
#define wxID_DISABLE_ALL_BREAK 1030
#define wxID_GOTO_NEXT_BREAK 1031
#define wxID_CREATE_WATCH 1032
#define wxID_DELETE_WATCH 1033
#define wxID_SAVE_SYMBOL_INFO 1034
#define wxID_HELP_ABOUT 1035
#define wxID_MAIN_TOOLBAR 1036
#define wxID_TOOL_CREATE_PROJECT 1037
#define wxID_TOOL_OPEN_PROJECT 1038
#define wxID_TOOL_SAVE_PROJECT 1039
#define wxID_TOOL_SAVE_PROJECT_AS 1040
#define wxID_TOOL_ADD_ONE_FILE 1041
#define wxID_TOOL_EXIT_PROGRAM 1042
#define wxID_TOOL_RUN_DEBUG 1043
#define wxID_TOOL_RUN_FREE 1044
#define wxID_TOOL_TRACE_IN 1045
#define wxID_TOOL_TRACE_OUT 1046
#define wxID_TRACE_TO_CURSOR 1047
#define wxID_TOOL_EXIT_FROM_METHOD 1048
#define wxID_TOOL_STOP_PROGRAM 1049
#define wxID_TOOL_FINISH_PROGRAM 1050
#define wxID_TOOL_CREATE_BREAK 1051
#define wxID_TOOL_DELETE_BREAK 1052
#define wxID_TOOL_TOGGLE_BREAK 1053
#define wxID_TOOL_BREAK_ON 1054
#define wxID_BREAK_OFF 1055
#define wxID_TOOL_CREATE_WATCH 1056
#define wxID_TOOL_DELETE_WATCH 1057
#define wxID_MENU_MODULE_NAME 1058
#define wxID_MENU_MODULE_ACTIVE 1059
#define wxID_MENU_MODULE_MAIN 1060
#define wxID_MENU_EDIT_MODULE 1061
#define wxID_DEBUGGER_OUTPUT_WINDOW 1062
#define wxID_DEBUGGER_STATUSBAR 1063
#define wxID_DEBUGGER_MENUBAR 1064
#define wxID_MENU_DEBUG_SAVE_OUTPUT 1065
#define wxID_MENU_DEBUG_SAVE_OUTPUT_AS 1066
#define wxID_MENU_DEBUG_CLEAR_WINDOW 1067
#define wxID_MENU_DEBUG_CLOSE_WINDOW 1068
#define wxID_DEBUGGER_TOOLBAR 1069
#define wxID_SAVE_DEBUG_OUTPUT 1070
#define wxID_SAVE_AS_DEBUG_OUTPUT 1071
#define wxID_DEBUG_CLEAR_WINDOW 1072
#define wxID_DEBUG_CLOSE_WINDOW 1073
#define wxID_DEBUG_OUTPUT_TEXT 1074
#define wxID_CONFIG_DIALOG 1075
#define wxID_CONFIG_SOURCE_CODE 1076
#define wxID_SOURCE_UTF8 1077
#define wxID_SOURCE_SAVE_IN_PROJECT 1078
#define wxID_EDIT_MODULE_PROPS_DIALOG 1079
#define wxID_MODULE_ID_STAT 1080
#define wxID_MODULE_PATH_STAT 1081
#define wxID_MODULE_NAME_EDIT 1082
#define wxID_MODULE_ACTIVE_FLAG 1083
#define wxID_MODULE_MAIN_FLAG 1084

///////////////////////////////////////////////////////////////////////////////
/// Class MainDebuggerWindow
///////////////////////////////////////////////////////////////////////////////
class MainDebuggerWindow : public wxFrame
{
	private:

	protected:
		wxListBox* ModuleList;
		wxStyledTextCtrl* SourceViewer;
		wxListBox* BreakPointsList;
		wxListBox* SymbolsList;
		wxListBox* StackList;
		wxStatusBar* MainWindowStatusBar;
		wxMenuBar* MainWindowMenuBar;
		wxMenu* view_menu;
		wxMenu* ViewAvailLanguages;
		wxMenu* breakpoint_menu;
		wxToolBar* MainWindowToolBar;
		wxToolBarToolBase* ToolCreateProject;
		wxToolBarToolBase* ToolOpenProject;
		wxToolBarToolBase* ToolSaveProject;
		wxToolBarToolBase* ToolSaveAsProject;
		wxToolBarToolBase* ToolAddOneFile;
		wxToolBarToolBase* ToolExitProgram;
		wxToolBarToolBase* ToolRunWithDebug;
		wxToolBarToolBase* ToolRunFree;
		wxToolBarToolBase* ToolTraceIn;
		wxToolBarToolBase* ToolTraceOut;
		wxToolBarToolBase* ToolTraceToCursor;
		wxToolBarToolBase* ToolExitFromMethod;
		wxToolBarToolBase* ToolStopProgram;
		wxToolBarToolBase* ToolFinishProgram;
		wxToolBarToolBase* ToolCreateBreakpoint;
		wxToolBarToolBase* ToolDeleteBreakpoint;
		wxToolBarToolBase* ToolToggleBreakpoint;
		wxToolBarToolBase* ToolBreakpointOn;
		wxToolBarToolBase* ToolBreakpointOff;
		wxToolBarToolBase* ToolCreateWatch;
		wxToolBarToolBase* ToolDeleteWatch;
		wxToolBarToolBase* ToolHelp;
		wxMenu* EditModuleMenu;
		wxMenuItem* MenuModuleName;
		wxMenuItem* MenuModuleIsActive;
		wxMenuItem* MenuModuleIsMain;

		// Virtual event handlers, override them in your derived class
		virtual void MainDebuggerWindowOnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void ModuleListOnListBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void ModuleListOnListBoxDClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void ModuleListOnRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void SourceViewerOnKeyDown( wxKeyEvent& event ) { event.Skip(); }
		virtual void SourceViewerOnLeftDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void SourceViewerOnRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void BreakPointsListOnListBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakPointsListOnListBoxDClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void SymbolsListOnListBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void SymbolsListOnListBoxDClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void StackListOnListBox( wxCommandEvent& event ) { event.Skip(); }
		virtual void StackListOnListBoxDClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void CreateProjectOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void FileLoadProjectOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void FileSaveProjectOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void FileSaveAsProjectOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void FileAddOneFileOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void FileExitProgramOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void ViewOutputWindowOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void ViewConfigOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void TraceRunWithDebugOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void TraceRunFreeOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void TraceInOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void TraceOutOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void TraceToCursorOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void TraceExitFromMethodOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void TraceStopProgramOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void TraceFinishProgramOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointCreateOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointCreateConditionalOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointDeleteOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointDeleteAllOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointOnOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointOffOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointEnableAllOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointDisableAllOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void BreakpointToggleOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void WatchCreateOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void WatchDeleteOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void SymbolsSaveOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void HelpIndexOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void HelpAboutOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolCreateProjectOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolOpenProjectOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolSaveProjectOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolSaveAsProjectOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolAddOneFileOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolExitProgramOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolRunWithDebugOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolRunFreeOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolTraceInOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolTraceOutOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolTraceToCursorOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolExitFromMethodOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolStopProgramOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolFinishProgramOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolCreateBreakpointOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolDeleteBreakpointOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolToggleBreakpointOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolBreakpointOnOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolBreakpointOffOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolHelpOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ModuleIsActiveOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void ModuleIsMainOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void EditModuleOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxMenu* file_menu;
		wxMenu* trace_menu;
		wxMenu* symbol_menu;
		wxMenu* help_menu;

		MainDebuggerWindow( wxWindow* parent, wxWindowID id = wxID_MAIN_WINDOW, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 553,360 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MainDebuggerWindow();

		void MainDebuggerWindowOnContextMenu( wxMouseEvent &event )
		{
			this->PopupMenu( EditModuleMenu, event.GetPosition() );
		}

};

///////////////////////////////////////////////////////////////////////////////
/// Class OutputDebuggerWindow
///////////////////////////////////////////////////////////////////////////////
class OutputDebuggerWindow : public wxFrame
{
	private:

	protected:
		wxStatusBar* DebuggerWindowStatusBer;
		wxMenuBar* DebuggerWindowMenuBar;
		wxMenu* debug_file_menu;
		wxToolBar* DebuggerWindowToolBar;
		wxToolBarToolBase* ToolDebugOutputSave;
		wxToolBarToolBase* ToolDebugOutputSaveAs;
		wxToolBarToolBase* ToolDebugClearWindow;
		wxToolBarToolBase* ToolDebugCloseWindow;
		wxTextCtrl* DebugOutputText;

		// Virtual event handlers, override them in your derived class
		virtual void OutputDebuggerWindowOnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void DebugOutputSaveOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void DebugOutputSaveAsOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void DebugOutputClearOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void DebugCloseOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolDebugOutputSaveOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolDebugOutputSaveAsOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolDebugClearWindowOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void ToolDebugCloseWindowOnToolClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		OutputDebuggerWindow( wxWindow* parent, wxWindowID id = wxID_DEBUGGER_OUTPUT_WINDOW, const wxString& title = _("Вывод отлаживаемой программы"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~OutputDebuggerWindow();

};

///////////////////////////////////////////////////////////////////////////////
/// Class ConfigDialog
///////////////////////////////////////////////////////////////////////////////
class ConfigDialog : public wxDialog
{
	private:

	protected:
		wxCheckBox* SourceIsUtf8;
		wxCheckBox* SourceIsFullSave;
		wxButton* SaveConfig;
		wxButton* CancelConfig;
		wxButton* RestoreCurrentConfig;

		// Virtual event handlers, override them in your derived class
		virtual void ConfigDialogOnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void ConfigDialogOnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void SaveConfigOnButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void CancelConfigOnButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void RestoreCurrentConfigOnButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		ConfigDialog( wxWindow* parent, wxWindowID id = wxID_CONFIG_DIALOG, const wxString& title = _("Настройка программы"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

		~ConfigDialog();

};

///////////////////////////////////////////////////////////////////////////////
/// Class EditModulePropsDialog
///////////////////////////////////////////////////////////////////////////////
class EditModulePropsDialog : public wxDialog
{
	private:

	protected:
		wxStaticText* m_staticText3;
		wxStaticText* ModuleId;
		wxStaticText* m_staticText4;
		wxStaticText* ModulePath;
		wxStaticText* m_staticText1;
		wxTextCtrl* ModuleName;
		wxCheckBox* ModuleIsActive;
		wxCheckBox* ModuleIsMain;
		wxButton* SaveNewModuleProps;
		wxButton* CancelEditModuleProps;
		wxButton* ResetModuleProps;
		wxButton* DeleteModule;

		// Virtual event handlers, override them in your derived class
		virtual void EditModulePropsDialogOnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void EditModulePropsDialogOnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void SaveNewModulePropsOnButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void CancelEditModulePropsOnButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void ResetModulePropsOnButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void DeleteModuleOnButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		EditModulePropsDialog( wxWindow* parent, wxWindowID id = wxID_EDIT_MODULE_PROPS_DIALOG, const wxString& title = _("Свойства модуля"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxDEFAULT_DIALOG_STYLE );

		~EditModulePropsDialog();

};

