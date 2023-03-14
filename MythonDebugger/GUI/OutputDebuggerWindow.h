#ifndef __OutputDebuggerWindow__
#define __OutputDebuggerWindow__

#include "MythonDebuggerGui.h"
#include "../redefine_.h"

class OutputDebuggerWindowImpl : public OutputDebuggerWindow
{
protected:
	// Handlers for OutputDebuggerWindow events.
	void DebugOutputSaveOnMenuSelection(wxCommandEvent& event);
	void DebugOutputSaveAsOnMenuSelection(wxCommandEvent& event);
	void DebugOutputClearOnMenuSelection(wxCommandEvent& event);
	void DebugCloseOnMenuSelection(wxCommandEvent& event);
	void ToolDebugOutputSaveOnToolClicked(wxCommandEvent& event);
	void ToolDebugOutputSaveAsOnToolClicked(wxCommandEvent& event);
	void ToolDebugClearWindowOnToolClicked(wxCommandEvent& event);
	void ToolDebugCloseWindowOnToolClicked(wxCommandEvent& event);
    void OutputDebuggerWindowOnClose(wxCloseEvent& event);

public:
    OutputDebuggerWindowImpl(wxWindow* parent);

    wxTextCtrl* GetOutputTextCtrl()
	{
		return DebugOutputText;
	}

private:
    wxString output_debugger_window_name_;
    wxString target_file_;

    void SetWindowLabel();
};

#endif // __OutputDebuggerWindow__
