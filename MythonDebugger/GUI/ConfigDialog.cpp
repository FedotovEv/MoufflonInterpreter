#include "ConfigDialog.h"

ConfigDialogImpl::ConfigDialogImpl(wxWindow* parent, OptionsData& options_data) :
    ConfigDialog(parent), options_data_(options_data)
{}

void ConfigDialogImpl::ConfigDialogOnClose( wxCloseEvent& event )
{
    EndModal(0);
}

void ConfigDialogImpl::ConfigDialogOnInitDialog( wxInitDialogEvent& event )
{
    SourceFullSave->SetValue(options_data_.is_save_module_body);
    SourceUtf8->SetValue(options_data_.is_source_utf8);
}

void ConfigDialogImpl::SaveConfigOnButtonClick( wxCommandEvent& event )
{
    options_data_.is_save_module_body = SourceFullSave->IsChecked();
    options_data_.is_source_utf8 = SourceUtf8->IsChecked();
    EndModal(wxID_OK);
}

void ConfigDialogImpl::CancelConfigOnButtonClick( wxCommandEvent& event )
{
    Close();
}

void ConfigDialogImpl::RestoreCurrentConfigOnButtonClick( wxCommandEvent& event )
{
    SourceFullSave->SetValue(options_data_.is_save_module_body);
    SourceUtf8->SetValue(options_data_.is_source_utf8);
}
