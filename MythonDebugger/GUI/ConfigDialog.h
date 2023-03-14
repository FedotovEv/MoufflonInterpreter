#ifndef __ConfigDialog__
#define __ConfigDialog__

#include "MythonDebuggerGui.h"
#include "../MythonDebugger.h"

class ConfigDialogImpl : public ConfigDialog
{
protected:
	// Handlers for ConfigDialog events.
	void ConfigDialogOnClose(wxCloseEvent& event);
	void ConfigDialogOnInitDialog(wxInitDialogEvent& event);
	void SaveConfigOnButtonClick(wxCommandEvent& event);
	void CancelConfigOnButtonClick(wxCommandEvent& event);
	void RestoreCurrentConfigOnButtonClick(wxCommandEvent& event);

public:
	ConfigDialogImpl(wxWindow* parent, OptionsData& options_data);

private:
    OptionsData& options_data_;
};

#endif // __ConfigDialog__
