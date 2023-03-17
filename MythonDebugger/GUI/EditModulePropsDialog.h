#ifndef __EditModulePropsDialog__
#define __EditModulePropsDialog__

#include "MythonDebuggerGui.h"
#include "../MythonDebugger.h"

class EditModulePropsDialogImpl : public EditModulePropsDialog
{
protected:
	// Обработчики событий диалога EditModulePropsDialog.
	void EditModulePropsDialogOnInitDialog(wxInitDialogEvent& event);
    void EditModulePropsDialogOnClose(wxCloseEvent& event);
	void SaveNewModulePropsOnButtonClick(wxCommandEvent& event);
	void CancelEditModulePropsOnButtonClick(wxCommandEvent& event);
	void ResetModulePropsOnButtonClick(wxCommandEvent& event);
	void DeleteModuleOnButtonClick(wxCommandEvent& event);

public:
    inline static const int EDIT_MODULE_PROPS_CANCEL = 0;
    inline static const int EDIT_MODULE_PROPS_OK = 1;
    inline static const int EDIT_MODULE_PROPS_DELETE = 2;

    // Конструктор диалога
	EditModulePropsDialogImpl(wxWindow* parent, const LexerInputExImpl::ModuleDescType& module_desc);
    const LexerInputExImpl::ModuleDescType& GetNewModuleDesc()
    {
        return new_module_desc_;
    }

private:
    const LexerInputExImpl::ModuleDescType& module_desc_;
    LexerInputExImpl::ModuleDescType new_module_desc_;
};

#endif // __EditModulePropsDialog__
