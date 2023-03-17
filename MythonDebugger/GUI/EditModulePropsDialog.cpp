
#include "EditModulePropsDialog.h"

using namespace std;

EditModulePropsDialogImpl::EditModulePropsDialogImpl(wxWindow* parent,
                      const LexerInputExImpl::ModuleDescType& module_desc) :
    EditModulePropsDialog(parent),
    module_desc_(module_desc)
{}

void EditModulePropsDialogImpl::EditModulePropsDialogOnInitDialog(wxInitDialogEvent& event)
{
    ModuleId->SetLabelText(to_string(module_desc_.module_id));
    ModulePath->SetLabelText(module_desc_.module_path.string());
    ModuleName->SetValue(module_desc_.module_name);
    ModuleIsActive->SetValue(module_desc_.module_is_active);
    ModuleIsMain->SetValue(module_desc_.module_is_main);
}

void EditModulePropsDialogImpl::EditModulePropsDialogOnClose(wxCloseEvent& event)
{
    EndModal(EDIT_MODULE_PROPS_CANCEL);
}

void EditModulePropsDialogImpl::SaveNewModulePropsOnButtonClick(wxCommandEvent& event)
{ // Сохраним в поле new_module_desc_ новое состояние модуля из полей ввода нашего диалога
    new_module_desc_.module_id = module_desc_.module_id;
    new_module_desc_.module_path = module_desc_.module_path;
    new_module_desc_.module_name = ModuleName->GetValue().ToStdString();
    new_module_desc_.module_is_active = ModuleIsActive->GetValue();
    new_module_desc_.module_is_main = ModuleIsMain->GetValue();
    EndModal(EDIT_MODULE_PROPS_OK);
}

void EditModulePropsDialogImpl::CancelEditModulePropsOnButtonClick(wxCommandEvent& event)
{
    Close();
}

void EditModulePropsDialogImpl::ResetModulePropsOnButtonClick(wxCommandEvent& event)
{
    wxInitDialogEvent init_event;
    EditModulePropsDialogOnInitDialog(init_event);
}

void EditModulePropsDialogImpl::DeleteModuleOnButtonClick(wxCommandEvent& event)
{
    EndModal(EDIT_MODULE_PROPS_DELETE);
}
