
#include "../MythonDebugger.h"
#include "MainDebuggerWindow.h"
#include "ConfigDialog.h"
#include "EditModulePropsDialog.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iterator>

#include "../PugiXML/pugixml.hpp" // Подключаем "ПугиХаЭмЭль"

using namespace std;
namespace fs = std::filesystem;

wxDEFINE_EVENT(DEBUG_EVENT_TYPE, wxCommandEvent);

MainDebuggerWindowImpl::MainDebuggerWindowImpl(wxWindow* parent, DebuggerProject& debugger_project) :
    MainDebuggerWindow(parent),
    debug_controller_(this, debugger_project)
{
    msgbox_msg_title = _("Сообщение");
    msgbox_err_title = _("Ошибка");
    msgbox_warning_title = _("Предупреждение");
    msgbox_open_err_title = _("Ошибка при открытии проекта");
    msgbox_save_err_title = _("Ошибка при записи");
    msg_not_found = _("не найден");
    msg_unknown = _("неизвестно");
    msg_none = wxT("None");

    static int widths_field[] = {-4, -1};

    main_debugger_window_name_ = _("Отладчик МуфлоЖук");
    MainWindowStatusBar->SetStatusWidths(2, widths_field);
    SetWindowLabel();
    Bind(DEBUG_EVENT_TYPE, &MainDebuggerWindowImpl::DebugEventHandler, this, wxID_MAIN_WINDOW);
    SourceViewer->MarkerDefine(MARKER_CURRENT_POINT, wxSTC_MARK_CIRCLE, *wxRED, *wxRED);
    SourceViewer->MarkerDefine(MARKER_STACK_ENTRY_BEGIN, wxSTC_MARK_ARROW, *wxYELLOW, *wxYELLOW);
    SourceViewer->SetMarginMask(0, 0xFFFFFFFF);

    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    if (this_app->options_data.option_filename.size())
    {
        pair<bool, wxString> load_result = DoLoadProject(this_app->options_data.option_filename[0]);
        if (!load_result.first)
            wxMessageBox(load_result.second, msgbox_open_err_title);
    }
}

bool MainDebuggerWindowImpl::IsDebugStopped()
{
    DebugController::ControllerStatus current_debug_status = debug_controller_.GetStatus();
    return debug_thread_.joinable() &&
           current_debug_status.run_status == DebugController::ControllerRunStatus::CONTROL_STATUS_STOPPED;
}

wxString MainDebuggerWindowImpl::FormatSymbolsListBox(const runtime::Closure* closure_ptr, const string& symbol_name)
{
    const wxString zpt = wxT(" : ");
    wxString result = symbol_name + zpt;
    if (!closure_ptr || !(closure_ptr->count(symbol_name)))
    {
        return result + msg_unknown + zpt + msg_not_found;
    }

    const runtime::ObjectHolder& object_holder = closure_ptr->at(symbol_name);
    if (!object_holder)
    { // Вместилище объекта пусто - это значение None
        return result + msg_none + zpt + msg_none;
    }
    else if (runtime::Number* number_ptr = object_holder.TryAs<runtime::Number>())
    {
        if (number_ptr->IsDouble())
            return result + wxT("Double") + zpt + to_string(number_ptr->GetDoubleValue());
        else
            return result + wxT("Integer") + zpt + to_string(number_ptr->GetIntValue());
    }
    else if (runtime::String* string_ptr = object_holder.TryAs<runtime::String>())
    {
        return result + wxT("String") + zpt + string_ptr->GetValue();
    }
    else if (runtime::Bool* bool_ptr = object_holder.TryAs<runtime::Bool>())
    {
        if (bool_ptr->GetValue())
            return result + wxT("Bool") + zpt + wxT("True");
        else
            return result + wxT("Bool") + zpt + wxT("False");
    }
    else if (runtime::Class* class_ptr = object_holder.TryAs<runtime::Class>())
    {
        return result + wxT("Class") + zpt + class_ptr->GetName();
    }
    else if (runtime::ClassInstance* class_instance_ptr = object_holder.TryAs<runtime::ClassInstance>())
    {
        return result + wxT("ClassInst") + zpt + class_instance_ptr->GetClassName();
    }
    else
    {
        return result + wxT("Other") + zpt + msg_not_found;
    }
}

void MainDebuggerWindowImpl::FillSymbolListBox(const runtime::Closure* closure_ptr)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    auto& watches_container = this_app->debugger_project.GetWatchesContainer();
    unordered_set<string> save_field_name;

    current_closure_ptr_ = closure_ptr;
    if (!IsDebugStopped())
        current_closure_ptr_ = nullptr;

    SymbolsList->Clear();
    // Сначала выведем в начало списка поля, за которыми установлен надзор
    for (const WatchesContainer::WatchDescType& watch_desc : watches_container)
    {
        if (watch_desc.is_active)
        {
            SymbolsList->Append(FormatSymbolsListBox(current_closure_ptr_, watch_desc.watch_symbol_name),
                new SymbolDescClientData(&watch_desc));
            save_field_name.insert(watch_desc.watch_symbol_name);
        }
    }
    if (!current_closure_ptr_)
        return;

    // А теперь дополняем список SymbolsList остальными записями из таблицы символов
    for (const auto& closure_rec_pair : *current_closure_ptr_)
    {
        if (!save_field_name.count(closure_rec_pair.first))
            SymbolsList->Append(FormatSymbolsListBox(current_closure_ptr_, closure_rec_pair.first),
                new SymbolDescClientData(closure_rec_pair.first));
    }
}

wxString MainDebuggerWindowImpl::FormatStackListBox(const runtime::CallStackEntry& stack_entry)
{
    return stack_entry.info_data + ':' +
           to_string(stack_entry.call_command.module_id) +'(' +
           to_string(stack_entry.call_command.module_string_number + 1) + ')' +
           " : " + to_string(stack_entry.first_command.module_id) + '(' +
           to_string(stack_entry.first_command.module_string_number + 1) + ')';
}

void MainDebuggerWindowImpl::FillStackListBox()
{
    StackList->Clear();
    for (const runtime::CallStackEntry& stack_entry : debug_controller_.GetContext().GetCallStack())
        StackList->Append(FormatStackListBox(stack_entry), new StackEntryClientData(stack_entry));
    SelectStackListLine(INT_MAX, false);
}

void MainDebuggerWindowImpl::SelectStackListLine(int new_stack_list_line, bool is_switch_module)
{ // new_stack_list_line == wxNOT_FOUND - устанавливаем состояние без выбранного стекового кадра.
  // new_stack_list_line == INT_MAX - выбираем последний элемент списка - текущий стековый кадр.
    StackEntryClientData* stack_client_data_ptr;

    SourceViewer->MarkerDeleteAll(MARKER_STACK_ENTRY_BEGIN);
    if (new_stack_list_line == wxNOT_FOUND || !StackList->GetCount())
    {
        StackList->SetSelection(wxNOT_FOUND);
        select_stack_entry_point_.module_id = -1;
        select_stack_entry_point_.module_string_number = -1;
        SymbolsList->Clear();
        return;
    }

    if (new_stack_list_line == INT_MAX)
        new_stack_list_line = static_cast<int>(StackList->GetCount()) - 1;

    StackList->SetSelection(new_stack_list_line);
    stack_client_data_ptr = static_cast<StackEntryClientData*>(StackList->GetClientObject(new_stack_list_line));
    select_stack_entry_point_ = stack_client_data_ptr->GetStackEntry().first_command;
    FillSymbolListBox(stack_client_data_ptr->GetStackEntry().closure_ptr);

    if (current_module_id_ == select_stack_entry_point_.module_id)
    {
        if (select_stack_entry_point_.module_string_number >= 0)
            SourceViewer->MarkerAdd(select_stack_entry_point_.module_string_number, MARKER_STACK_ENTRY_BEGIN);
    }
    else
    {
        if (is_switch_module)
            SetViewerModuleText(select_stack_entry_point_.module_id);
    }
}

void MainDebuggerWindowImpl::SetWindowLabel()
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    SetLabel(main_debugger_window_name_ + wxT(" - ") +
        wxString(this_app->debugger_project.GetProjectPath().filename().string()));
}

int MainDebuggerWindowImpl::ScanSelectionByModuleId(int module_id)
{
    for (unsigned int i = 0; i < ModuleList->GetCount(); ++i)
    {
        ModuleDescClientData* module_client_data_ptr =
            static_cast<ModuleDescClientData*>(ModuleList->GetClientObject(i));    
        if (module_client_data_ptr->GetModuleDesc().module_id == module_id)
            return i;
    }

    return wxNOT_FOUND;
}

void MainDebuggerWindowImpl::SetViewerModuleText(int new_selection)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);

    if (new_selection < 0)
        new_selection = wxNOT_FOUND;
    if (new_selection >= static_cast<int>(ModuleList->GetCount()))
    {
        if (ModuleList->GetCount())
            new_selection = ModuleList->GetCount() - 1;
        else
            new_selection = wxNOT_FOUND;
    }

    ModuleDescClientData* module_client_data_ptr = nullptr;
    int new_module_id_ = wxNOT_FOUND;
    if (new_selection != wxNOT_FOUND)
    {
        module_client_data_ptr = static_cast<ModuleDescClientData*>(ModuleList->GetClientObject(new_selection));
        new_module_id_ = module_client_data_ptr->GetModuleDesc().module_id;
    }
    if (new_module_id_ == current_module_id_)
        return;

    current_module_id_ = new_module_id_;
    SourceViewer->MarkerDeleteAll(MARKER_CURRENT_POINT);
    SourceViewer->MarkerDeleteAll(MARKER_STACK_ENTRY_BEGIN);
    SourceViewer->SetReadOnly(false);
    SourceViewer->ClearAll();
    SourceViewer->SetReadOnly(true);
    if (new_selection != ModuleList->GetSelection())
        ModuleList->SetSelection(new_selection);
    if (new_selection == wxNOT_FOUND || new_module_id_ == wxNOT_FOUND || !module_client_data_ptr)
    {
        MainWindowStatusBar->SetStatusText(_("Нет"), 0);
        return;
    }

    const LexerInputExImpl::ModuleDescType& module_desc = module_client_data_ptr->GetModuleDesc();
    wxString set_status_text = module_desc.module_name + ':' + to_string(current_module_id_);
    if (module_desc.module_is_active)
        set_status_text += _(":Активный");
    if (module_desc.module_is_main)
        set_status_text += _(":Главный");
    MainWindowStatusBar->SetStatusText(set_status_text, 0);

    SourceViewer->SetReadOnly(false);
    if (this_app->options_data.is_source_utf8)
        SourceViewer->SetText(wxString::FromUTF8(module_desc.module_body.c_str()));
    else
        SourceViewer->SetText(module_desc.module_body);
    SourceViewer->SetReadOnly(true);

    if (current_point_.module_id == current_module_id_ && current_point_.module_string_number >= 0)
        SourceViewer->MarkerAdd(current_point_.module_string_number, MARKER_CURRENT_POINT);

    if (select_stack_entry_point_.module_id == current_module_id_ &&
        select_stack_entry_point_.module_string_number >= 0)
        SourceViewer->MarkerAdd(select_stack_entry_point_.module_string_number, MARKER_STACK_ENTRY_BEGIN);
}

bool MainDebuggerWindowImpl::CheckProjectModifyStatus()
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);

    if (!this_app->debugger_project.IsProjectChanged())
        return true;

    if (wxMessageBox(_("В проект вносились изменения.\nПри продолжении операции они будут утеряны.\nПродолжать?"),
                     msgbox_warning_title, wxYES_NO) == wxYES)
    {
        this_app->debugger_project.ClearChangeFlag();
        return true;
    }
    else
    {
        return false;
    }
}

bool MainDebuggerWindowImpl::CheckDebugInProcess()
{ // Если отладочный поток уже запущен, перед выходом из программы его нужно обязательно остановить.
    if (!debug_thread_.joinable())
        return true;
    if (wxMessageBox(_("В данный момент выполняется отладка.\nОстановить?"),
                     msgbox_warning_title, wxYES_NO) != wxYES)
        return false;
    debug_controller_.CommitCommand(DebugController::ControllerCommand::CONTROL_TERMINATE_PROGRAM);
    debug_thread_.join();
    return true;
}

string MainDebuggerWindowImpl::FormatProjectListBoxItem(const LexerInputExImpl::ModuleDescType& module_desc)
{
    const string zpt = " : ";
    string list_string_result = to_string(module_desc.module_id) + zpt + module_desc.module_name +
           zpt + module_desc.module_path.string() + ':';
    if (module_desc.module_is_active)
        list_string_result += 'V';
    else
        list_string_result += 'U';
    if (module_desc.module_is_main)
        list_string_result += ":M"s;

    return list_string_result;
}

void MainDebuggerWindowImpl::FillProjectListBox()
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);

    auto& lexer_input = this_app->debugger_project.GetLexerInputStream();

    ModuleList->Clear();
    for (const LexerInputExImpl::ModuleDescType& module_desc : lexer_input)
        ModuleList->Append(FormatProjectListBoxItem(module_desc), new ModuleDescClientData(module_desc));

    SetViewerModuleText(0);
}

pair<bool, wxString> MainDebuggerWindowImpl::DoLoadProject(const wxString& project_filename)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(project_filename.ToStdString().c_str());

    if (!result)
        return {false, _("Ошибка при разборе xml-файла:") + wxString(result.description(), wxConvUTF8)};

    bool is_signature_correct = false;
    pugi::xml_node signature_node = doc.child("moufflon-project");
    if (signature_node)
    {
        pugi::xml_attribute signature_attrrib = signature_node.attribute("signature");
        if (signature_attrrib && string(signature_attrrib.as_string()) == "MoufflonBug"s)
            is_signature_correct = true;
    }

    if (!is_signature_correct)
        return {false, _("Ошибка в формате файла")};

    this_app->debugger_project.Clear();
    pugi::xml_node modules_node = doc.child("modules");
    if (modules_node)
    {
        for (pugi::xml_node current_module_node : modules_node.children())
        {
            LexerInputExImpl::ModuleDescType current_module_desc;

            current_module_desc.module_id = current_module_node.attribute("module_id").as_int();
            current_module_desc.module_name = current_module_node.child("module_name").text().as_string();
            current_module_desc.module_path = current_module_node.child("module_path").text().as_string();
            pugi::xml_node module_body_node = current_module_node.child("module_body");
            current_module_desc.module_is_active = current_module_node.child("module_is_active");
            current_module_desc.module_is_main = current_module_node.child("module_is_main");
            if (module_body_node)
            { // Тело модуля хранится прямо в проекте. Получим его оттуда.
                current_module_desc.module_body = module_body_node.text().as_string();
            }
            else
            { // В файле проекта исходника нет. Пробуем считать его из файла current_module_desc.module_path.
                ifstream ifile(current_module_desc.module_path);
                if (ifile)
                {
                    string module_data{ std::istreambuf_iterator<char>(ifile), std::istreambuf_iterator<char>() };
                    current_module_desc.module_body = move(module_data);
                }
            }
            // Добавляем к проекту новый сформированный модуль
            this_app->debugger_project.GetLexerInputStream()
                .SetModuleWithBody(move(current_module_desc));
        }
    }

    pugi::xml_node breakpoints_node = doc.child("breakpoints");
    if (breakpoints_node)
    {
        for (pugi::xml_node current_break_node : breakpoints_node.children())
        {
            BreakpointsContainer::BreakpointDescType current_breakpoint_desc;

            current_breakpoint_desc.breakpoint_id = current_break_node.attribute("breakpoint_id").as_int();
            current_breakpoint_desc.breakpoint_point.module_id =
                current_break_node.attribute("module_id").as_int();
            current_breakpoint_desc.breakpoint_point.module_string_number =
                current_break_node.attribute("module_string_number").as_int();
            current_breakpoint_desc.is_active = current_break_node.attribute("is_active").as_bool();

            pugi::xml_attribute start_counter_attr = current_break_node.attribute("start_counter");
            if (start_counter_attr)
            {
                current_breakpoint_desc.is_counting = true;
                current_breakpoint_desc.start_counter = start_counter_attr.as_int();
                current_breakpoint_desc.current_counter = 0;
            }

            pugi::xml_node current_cond_expr_node = current_break_node.child("conditional_expression");
            if (current_cond_expr_node)
            {
                current_breakpoint_desc.is_conditional = true;
                current_breakpoint_desc.conditional_expression = current_cond_expr_node.text().as_string();
            }
            // Добавляем к проекту новую полностью описанную точку останова
            this_app->debugger_project.GetBreakpointsContainer()
                .SetBreakpointDescriptor(move(current_breakpoint_desc));
        }
    }

    pugi::xml_node watches_node = doc.child("watches");
    if (watches_node)
    {
        for (pugi::xml_node current_watch_node : watches_node.children())
        {
            WatchesContainer::WatchDescType current_watch_desc;

            current_watch_desc.watch_id = current_watch_node.attribute("watch_id").as_int();;
            current_watch_desc.is_active = current_watch_node.attribute("is_active").as_bool();
            current_watch_desc.watch_symbol_name = current_watch_node.child("symbol_name").text().as_string();
            // Добавляем к проекту новую полностью описанную надзорную запись
            this_app->debugger_project.GetWatchesContainer()
                .SetWatchDescriptor(move(current_watch_desc));
        }
    }

    this_app->debugger_project.SetProjectPath(project_filename.ToStdString());
    SetWindowLabel();
    FillProjectListBox();
    this_app->debugger_project.ClearChangeFlag();
    return {true, {}};
}

void MainDebuggerWindowImpl::ModuleListOnListBox(wxCommandEvent& event)
{
    SetViewerModuleText(ModuleList->GetSelection());
}

void MainDebuggerWindowImpl::ModuleListOnListBoxDClick(wxCommandEvent& event)
{
    if (ModuleList->GetSelection() == wxNOT_FOUND)
        return;

    if (debug_thread_.joinable())
    {
        wxMessageBox(_("Выполняется отладка. Редактирование проекта невозможно."), msgbox_msg_title);
        return;
    }

    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    ModuleDescClientData* module_client_data_ptr =
        static_cast<ModuleDescClientData*>(ModuleList->GetClientObject(ModuleList->GetSelection()));
    string old_module_name = module_client_data_ptr->GetModuleDesc().module_name;
    EditModulePropsDialogImpl edit_module_dialog(this, module_client_data_ptr->GetModuleDesc());

    switch (edit_module_dialog.ShowModal())
    {
    case EditModulePropsDialogImpl::EDIT_MODULE_PROPS_OK:
        if (edit_module_dialog.GetNewModuleDesc().module_name != old_module_name)
            debug_controller_.GetInputLexer().RenameIncludeModule(old_module_name,
                               edit_module_dialog.GetNewModuleDesc().module_name);
        debug_controller_.GetInputLexer().FixModuleDesc(edit_module_dialog.GetNewModuleDesc());
        FillProjectListBox();
        break;
    case EditModulePropsDialogImpl::EDIT_MODULE_PROPS_DELETE:
        debug_controller_.GetInputLexer().EraseIncludeModule(old_module_name);
        FillProjectListBox();
        break;
    case EditModulePropsDialogImpl::EDIT_MODULE_PROPS_CANCEL:
        [[fallthrough]];
    default:
        break;
    }
    return;
}

void MainDebuggerWindowImpl::ModuleListOnRightDown(wxMouseEvent& event)
{
    if (ModuleList->GetSelection() == wxNOT_FOUND || debug_thread_.joinable())
        return;
    ModuleDescClientData* module_client_data_ptr =
        static_cast<ModuleDescClientData*>(ModuleList->GetClientObject(ModuleList->GetSelection()));
    if (!module_client_data_ptr)
        return;

    MenuModuleName->Enable(false);
    MenuModuleName->SetItemLabel(module_client_data_ptr->GetModuleDesc().module_name);
    MenuModuleIsActive->Check(module_client_data_ptr->GetModuleDesc().module_is_active);
    MenuModuleIsMain->Check(module_client_data_ptr->GetModuleDesc().module_is_main);
    PopupMenu(EditModuleMenu);
}

void MainDebuggerWindowImpl::SourceViewerOnKeyDown(wxKeyEvent& event)
{
// TODO: Implement SourceViewerOnKeyDown
    event.Skip();
}

void MainDebuggerWindowImpl::SourceViewerOnLeftDown(wxMouseEvent& event)
{
// TODO: Implement SourceViewerOnLeftDown
    event.Skip();
}

void MainDebuggerWindowImpl::SourceViewerOnRightDown(wxMouseEvent& event)
{
// TODO: Implement SourceViewerOnRightDown
    event.Skip();
}

void MainDebuggerWindowImpl::BreakPointsListOnListBox( wxCommandEvent& event )
{
// TODO: Implement BreakPointsListOnListBox
}

void MainDebuggerWindowImpl::BreakPointsListOnListBoxDClick( wxCommandEvent& event )
{
// TODO: Implement BreakPointsListOnListBoxDClick
}

void MainDebuggerWindowImpl::SymbolsListOnListBox( wxCommandEvent& event )
{
// TODO: Implement SymbolsListOnListBoxDClick
}

void MainDebuggerWindowImpl::SymbolsListOnListBoxDClick( wxCommandEvent& event )
{
// TODO: Implement SymbolsListOnListBoxDClick
}

void MainDebuggerWindowImpl::StackListOnListBox(wxCommandEvent& event)
{
    SelectStackListLine(StackList->GetSelection(), true);
}

void MainDebuggerWindowImpl::StackListOnListBoxDClick( wxCommandEvent& event )
{
// TODO: Implement StackListOnListBoxDClick
}

void MainDebuggerWindowImpl::CreateProjectOnMenuSelection(wxCommandEvent& event)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);

    if (!CheckProjectModifyStatus())
        return;

    debug_controller_.Clear();
    SetWindowLabel();
    FillProjectListBox();
    FillStackListBox();
}

void MainDebuggerWindowImpl::FileLoadProjectOnMenuSelection(wxCommandEvent& event)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    if (!CheckProjectModifyStatus())
        return;

    wxFileDialog load_file_dialog(this, _("Открыть проект отладчика МуфлоЖук"), {}, {},
        _("Проекты отладчика МуфлоЖук (*.xml)|*.xml|Все файлы|*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (load_file_dialog.ShowModal() == wxID_CANCEL)
        return;

    pair<bool, wxString> load_result = DoLoadProject(load_file_dialog.GetPath());
    if (!load_result.first)
        wxMessageBox(load_result.second, msgbox_open_err_title);
}

void MainDebuggerWindowImpl::FileSaveProjectOnMenuSelection(wxCommandEvent& event)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);

    pugi::xml_document doc;
    doc.append_child("moufflon-project").append_attribute("signature").set_value("MoufflonBug");
    if (this_app->debugger_project.GetLexerInputStream().size())
    { // Сначала формируем раздел XML, хранящий информацию о модулях проекта
        pugi::xml_node modules_node = doc.append_child("modules");
        size_t i = 0;
        for (const LexerInputExImpl::ModuleDescType& current_module_desc :
             this_app->debugger_project.GetLexerInputStream())
        {
            // Коневой узел данного модуля имеет имя, сформированное из слова "module_"
            // и его порядкового номера в списке.
            string module_node_name = "module_"s + to_string(++i);
            pugi::xml_node current_module_node = modules_node.append_child(module_node_name.c_str());
            // module_id сохраняем в виде атрибута
            current_module_node.append_attribute("module_id") = current_module_desc.module_id;
            // Сначала присоединим к корню узел с именем модуля
            pugi::xml_node current_name_node = current_module_node.append_child("module_name");
            current_name_node.append_child(pugi::node_pcdata).set_value(current_module_desc.module_name.c_str());
            // Затем к корневому узлу стыкуется узел, содержащий маршрут файла модуля
            pugi::xml_node current_path_node = current_module_node.append_child("module_path");
            current_path_node.append_child(pugi::node_pcdata).set_value(current_module_desc.module_path.string().c_str());
            if (current_module_desc.module_is_active)
                current_module_node.append_child("module_is_active");
            if (current_module_desc.module_is_main)
                current_module_node.append_child("module_is_main");
            if (this_app->options_data.is_save_module_body)
            { // Если включён соответствующий режим, сохраняем также и тело модуля
                // К корневому узлу стыкуется узел, содержащий ранее считанное из файл тело модуля
                pugi::xml_node current_body_node = current_module_node.append_child("module_body");
                current_body_node.append_child(pugi::node_pcdata).set_value(current_module_desc.module_body.c_str());
            }
        }

    }
    
    if (this_app->debugger_project.GetBreakpointsContainer().size())
    {  // Далее создаём раздел XML, содержащий информацию о точках останова
        pugi::xml_node breakpoints_node = doc.append_child("breakpoints");
        size_t i = 0;
        for (const BreakpointsContainer::BreakpointDescType& current_breakpoint_desc :
             this_app->debugger_project.GetBreakpointsContainer())
        {
            if (current_breakpoint_desc.is_onetime) // Одноразовые служебные точки останова не сохраняем
                continue;
            // Коневой узел данной точки станова имеет имя, сформированное из слова "breakpoint_"
            // и порядкового номера точки.
            string breakpoint_name = "breakpoint_"s + to_string(++i);
            pugi::xml_node current_break_node = breakpoints_node.append_child(breakpoint_name.c_str());
            // Основную массу информации о точке останова сохраним в виде атрибутов узла current_break_node
            current_break_node.append_attribute("breakpoint_id") = current_breakpoint_desc.breakpoint_id;
            current_break_node.append_attribute("module_id") = current_breakpoint_desc.breakpoint_point.module_id;
            current_break_node.append_attribute("module_string_number") =
                current_breakpoint_desc.breakpoint_point.module_string_number;            
            current_break_node.append_attribute("is_active") = current_breakpoint_desc.is_active;
            // Для счётной точки сохраним её счётчик
            if (current_breakpoint_desc.is_counting)
                current_break_node.append_attribute("start_counter") = current_breakpoint_desc.start_counter;                
            
            if (current_breakpoint_desc.is_conditional && current_breakpoint_desc.conditional_expression.size())
            { // Если условное выражение определено, сохраним его тоже
                pugi::xml_node current_cond_expr_node = current_break_node.append_child("conditional_expression");
                current_cond_expr_node.append_child(pugi::node_pcdata)
                                      .set_value(current_breakpoint_desc.conditional_expression.c_str());
            }
        }
    }

    if (this_app->debugger_project.GetWatchesContainer().size())
    { // Наконец, формируем раздел XML с информацией о надзорных записях
        pugi::xml_node watches_node = doc.append_child("watches");
        size_t i = 0;
        for (const WatchesContainer::WatchDescType& current_watch_desc :
             this_app->debugger_project.GetWatchesContainer())
        {
            // Коневой узел данной надзорной записи имеет имя, сформированное из слова "watch_"
            // и порядкового номера записи.
            string watch_name = "watch_"s + to_string(++i);
            pugi::xml_node current_watch_node = watches_node.append_child(watch_name.c_str());
            current_watch_node.append_attribute("watch_id") = current_watch_desc.watch_id;
            current_watch_node.append_attribute("is_active") = current_watch_desc.is_active;
            pugi::xml_node current_watch_symbols_node = current_watch_node.append_child("symbol_name");
            current_watch_symbols_node.append_child(pugi::node_pcdata)
                                      .set_value(current_watch_desc.watch_symbol_name.c_str());
        }
    }
    
    string project_path = this_app->debugger_project.GetProjectPath().string();
    if (!doc.save_file(project_path.c_str()))
        wxMessageBox(_("Произошла ошибка при записи файла ") + project_path, msgbox_save_err_title);
    this_app->debugger_project.ClearChangeFlag();
}

void MainDebuggerWindowImpl::FileSaveAsProjectOnMenuSelection(wxCommandEvent& event)
{
    wxFileDialog save_as_file_dialog(this, _("Сохранить проект как"),
        wxEmptyString, wxEmptyString, _("Проекты отладчика МуфлоЖук (*.xml)|*.xml|Все файлы|*.*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (save_as_file_dialog.ShowModal() == wxID_CANCEL)
        return;

    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    this_app->debugger_project.SetProjectPath(save_as_file_dialog.GetPath().ToStdString());

    SetWindowLabel();    
    FileSaveProjectOnMenuSelection(event);
}

void MainDebuggerWindowImpl::FileAddOneFileOnMenuSelection(wxCommandEvent& event)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
 
    wxFileDialog load_file_dialog(this, _("Открыть исходный файл на языке Муфлон"), {}, {},
        _("Исходные файлы Муфлон (*.muf)|*.muf|Все файлы|*.*"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (load_file_dialog.ShowModal() == wxID_CANCEL)
        return;

    fs::path full_module_path = load_file_dialog.GetPath().ToStdString();
    string module_name = full_module_path.filename().string();
    fs::path search_module_path = full_module_path.parent_path();

    ifstream ifile(full_module_path, ios::binary);
    string module_body{istreambuf_iterator<char>(ifile), istreambuf_iterator<char>()};

    auto& lexer_input = this_app->debugger_project.GetLexerInputStream();
    this_app->debugger_project.GetLexerInputStream().AddIncludeModule(module_name, module_body,
                                                                      search_module_path);    
    lexer_input.SetMainModuleName(module_name).SetSearchModulesPath(search_module_path)
               .SetAutoScanMode(true);
    parse::Lexer lexer(lexer_input);
    try
    {
        auto program = ParseProgram(lexer);
    }
    catch (ParseError& parse_error)
    {
        wxMessageBox(parse_error.what(), _("Ошибка синтаксического анализа исходного файла"));
    }
    catch (parse::LexerError& lexer_error)
    {
        wxMessageBox(lexer_error.what(), _("Ошибка лексического разбора исходного файла"));
    }

    lexer_input.SetAutoScanMode(false);
    FillProjectListBox();
}

void MainDebuggerWindowImpl::FileExitProgramOnMenuSelection(wxCommandEvent& event)
{
    Close();
}

void MainDebuggerWindowImpl::MainDebuggerWindowOnClose(wxCloseEvent& event)
{
    if (!CheckDebugInProcess())
        return;
    if (!CheckProjectModifyStatus())
        return;
    event.Skip();
}

void MainDebuggerWindowImpl::ViewOutputWindowOnMenuSelection(wxCommandEvent& event)
{
    debug_controller_.GetDebugOutput().Show();
}

void MainDebuggerWindowImpl::ViewConfigOnMenuSelection(wxCommandEvent& event)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    ConfigDialogImpl config_dialog(this, this_app->options_data);
    config_dialog.ShowModal();
}

bool MainDebuggerWindowImpl::IsNextStepPossible()
{ // Следующий шаг возможен, только если отладка выполняется и программа приостановлена
    return debug_thread_.joinable() && debug_controller_.GetStatus().run_status ==
           DebugController::ControllerRunStatus::CONTROL_STATUS_STOPPED;
}

void MainDebuggerWindowImpl::TraceRunWithDebugOnMenuSelection(wxCommandEvent& event)
{
    if (debug_thread_.joinable())
    { // Отладка уже происходит
        if (IsNextStepPossible())
            debug_controller_.CommitCommand(DebugController::ControllerCommand::CONTROL_EXECUTE_PROGRAM);
    }
    else
    { // Сеанс работы ещё не начат либо завершился, запустим его вновь
        debug_controller_.GetDebugOutput().Show();
        debug_thread_ = thread(ref(debug_controller_), true);
    }
}

void MainDebuggerWindowImpl::TraceRunFreeOnMenuSelection(wxCommandEvent& event)
{
    if (debug_thread_.joinable())
        return; // Отладка уже идёт, ничего сделать нельзя, выходим

    debug_controller_.GetDebugOutput().Show();
    debug_thread_ = thread(ref(debug_controller_), false);
}

void MainDebuggerWindowImpl::TraceInOnMenuSelection(wxCommandEvent& event)
{
    if (IsNextStepPossible())
        debug_controller_.CommitCommand(DebugController::ControllerCommand::CONTROL_STEP_IN);
}

void MainDebuggerWindowImpl::TraceOutOnMenuSelection(wxCommandEvent& event)
{
    if (IsNextStepPossible())
        debug_controller_.CommitCommand(DebugController::ControllerCommand::CONTROL_STEP_OUT);
}

void MainDebuggerWindowImpl::TraceToCursorOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement TraceToCursorOnMenuSelection
}

void MainDebuggerWindowImpl::TraceExitFromMethodOnMenuSelection(wxCommandEvent& event)
{
    if (IsNextStepPossible())
        debug_controller_.CommitCommand(DebugController::ControllerCommand::CONTROL_EXIT_METHOD);
}

void MainDebuggerWindowImpl::TraceStopProgramOnMenuSelection(wxCommandEvent& event)
{ // Данный обработчик пытается приостановить работающую программу, переведя её в состояние
  // останова на очередной строке.
    if (!debug_thread_.joinable())
        return; // Отладка не выполняется - выходим.
    if (debug_controller_.GetStatus().run_status !=
        DebugController::ControllerRunStatus::CONTROL_STATUS_RUNNING)
        return; // Программа уже на останове, также просто выходим

    debug_controller_.GetContext().SetDebugMode(runtime::DebugExecutionMode::DEBUG_STEP_IN);
}

void MainDebuggerWindowImpl::TraceFinishProgramOnMenuSelection(wxCommandEvent& event)
{  // Этот обработчик выполняет немедленное завершение исполняющейся Муфлон-программы
    if (!debug_thread_.joinable())
        return; // Отладка не выполняется - выходим.

    debug_controller_.CommitCommand(DebugController::ControllerCommand::CONTROL_TERMINATE_PROGRAM);
}

void MainDebuggerWindowImpl::BreakpointCreateOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointCreateOnMenuSelection
}

void MainDebuggerWindowImpl::BreakpointCreateConditionalOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointCreateConditionalOnMenuSelection
}

void MainDebuggerWindowImpl::BreakpointDeleteOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointDeleteOnMenuSelection
}

void MainDebuggerWindowImpl::BreakpointDeleteAllOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointDeleteAllOnMenuSelection
}

void MainDebuggerWindowImpl::BreakpointOnOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointOnOnMenuSelection
}

void MainDebuggerWindowImpl::BreakpointOffOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointOffOnMenuSelection
}

void MainDebuggerWindowImpl::BreakpointEnableAllOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointEnableAllOnMenuSelection
}

void MainDebuggerWindowImpl::BreakpointDisableAllOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointDisableAllOnMenuSelection
}

void MainDebuggerWindowImpl::BreakpointToggleOnMenuSelection( wxCommandEvent& event )
{
// TODO: Implement BreakpointToggleOnMenuSelection
}

void MainDebuggerWindowImpl::WatchCreateOnMenuSelection(wxCommandEvent& event)
{
    wxTextEntryDialog text_entry_dialog(this, _("Введите отслеживаемое выражение"), _("Надзорное выражение"));
    if (text_entry_dialog.ShowModal() != wxID_OK)
        return;

    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    auto& watches_container = this_app->debugger_project.GetWatchesContainer();

    WatchesContainer::WatchDescType watch_desc;
    watch_desc.is_active = true;
    watch_desc.watch_symbol_name = text_entry_dialog.GetValue();
    watches_container.AddWatchDescriptor(move(watch_desc));
    FillSymbolListBox(current_closure_ptr_);
}

void MainDebuggerWindowImpl::WatchDeleteOnMenuSelection(wxCommandEvent& event)
{
    int watch_num = SymbolsList->GetSelection();
    if (watch_num == wxNOT_FOUND)
        return;

    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    auto& watches_container = this_app->debugger_project.GetWatchesContainer();
    if (watch_num >= static_cast<int>(watches_container.size()))
        return;
    auto watch_iter = watches_container.begin();
    advance(watch_iter, watch_num);
    watches_container.erase(watch_iter);
    FillSymbolListBox(current_closure_ptr_);
}

void MainDebuggerWindowImpl::WatchFromSymbolListOnMenuSelection(wxCommandEvent& event)
{
    int watch_num = SymbolsList->GetSelection();
    if (watch_num == wxNOT_FOUND)
        return;

    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    auto& watches_container = this_app->debugger_project.GetWatchesContainer();
    SymbolDescClientData* symbol_client_desc_ptr =
        static_cast<SymbolDescClientData*>(SymbolsList->GetClientObject(watch_num));
    if (symbol_client_desc_ptr->is_watch())
        return;

    WatchesContainer::WatchDescType watch_desc;
    watch_desc.is_active = true;
    watch_desc.watch_symbol_name = symbol_client_desc_ptr->GetSymbolName();
    watches_container.AddWatchDescriptor(move(watch_desc));
    FillSymbolListBox(current_closure_ptr_);
}

void MainDebuggerWindowImpl::ChewSymbToOut(string prefix, string symbol_name, const runtime::Closure* closure_ptr, ostream& out)
{
    out << prefix << FormatSymbolsListBox(closure_ptr, symbol_name) << endl;
    const runtime::ObjectHolder& object_holder = closure_ptr->at(symbol_name);
    if (runtime::Class* class_ptr = object_holder.TryAs<runtime::Class>())
    {
        for (auto& methods_desc_pair : class_ptr->GetMethodsDesc())
            out << prefix << " :M " << methods_desc_pair.first << " <- " << methods_desc_pair.second << endl;
    }
    else if (runtime::ClassInstance* class_instance_ptr = object_holder.TryAs<runtime::ClassInstance>())
    {
        string new_prefix = prefix + "  :i<> " + class_instance_ptr->GetClassName() + '.';
        for (const auto& closure_rec_pair : class_instance_ptr->Fields())
            ChewSymbToOut(new_prefix, closure_rec_pair.first, &class_instance_ptr->Fields(), out);
    }
}

void MainDebuggerWindowImpl::SymbolsSaveOnMenuSelection(wxCommandEvent& event)
{
    if (!current_closure_ptr_ || !IsDebugStopped())
    {
        wxMessageBox(_("Операцию можно выполнить только в состоянии приостанова отладки"), _("Невыполнимо"));
        return;
    }

    wxFileDialog save_as_file_dialog(this, _("Сохранить символы как"),
        wxEmptyString, wxEmptyString, _("Текстовые файлы (*.txt)|*.txt|Все файлы|*.*"),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (save_as_file_dialog.ShowModal() == wxID_CANCEL)
        return;

    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    ofstream ofile(save_as_file_dialog.GetPath().ToStdString());
    for (size_t i = 0; i < SymbolsList->GetCount(); ++i)
    {
        SymbolDescClientData* symb_client_ptr = static_cast<SymbolDescClientData*>(SymbolsList->GetClientObject(i));
        if (symb_client_ptr->is_watch())
            ChewSymbToOut("", symb_client_ptr->GetWatchDesc()->watch_symbol_name, current_closure_ptr_, ofile);
        else
            ChewSymbToOut("", symb_client_ptr->GetSymbolName(), current_closure_ptr_, ofile);
    }
}

void MainDebuggerWindowImpl::HelpIndexOnMenuSelection(wxCommandEvent& event)
{
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    this_app->HtmlHelp.DisplayContents();
}

void MainDebuggerWindowImpl::HelpAboutOnMenuSelection( wxCommandEvent& event )
{
    wxString msg = _("МуфлоЖук - отладчик для программ на языке Муфлон.");
    msg += _("Обеспечивает стандартные техники отладки для программ на этом языке:");
    msg += _("запуск, пошаговое исполнение, точки останова, наблюдение за переменными,");
    msg += _("изучение эволюции символов, изучение стека вызовов, и. т. д.\n");
    msg += _("Федотов Евгений (fedotov_ev@rambler.ru)\n");
    msg += _("Версия ") + wxString(FULLVERSION_STRING, wxConvUTF8);
    msg += _(", сборка ") + wxString(wxT(__DATE__));
    msg += _("\nhttp://www.github.com/FedotovEv/MoufflonInterpreter");
    wxMessageBox(msg, _("Кто я??? Где я???"));
}

void MainDebuggerWindowImpl::ToolCreateProjectOnToolClicked( wxCommandEvent& event )
{
    CreateProjectOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolOpenProjectOnToolClicked(wxCommandEvent& event)
{
    FileLoadProjectOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolSaveProjectOnToolClicked(wxCommandEvent& event)
{
    FileSaveProjectOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolSaveAsProjectOnToolClicked(wxCommandEvent& event)
{
    FileSaveAsProjectOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolAddOneFileOnToolClicked(wxCommandEvent& event)
{
    FileAddOneFileOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolExitProgramOnToolClicked(wxCommandEvent& event)
{
    FileExitProgramOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolRunWithDebugOnToolClicked(wxCommandEvent& event)
{
    TraceRunWithDebugOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolRunFreeOnToolClicked(wxCommandEvent& event)
{
    TraceRunFreeOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolTraceInOnToolClicked(wxCommandEvent& event)
{
    TraceInOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolTraceOutOnToolClicked(wxCommandEvent& event)
{
    TraceOutOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolTraceToCursorOnToolClicked( wxCommandEvent& event )
{
// TODO: Implement ToolTraceToCursorOnToolClicked
}

void MainDebuggerWindowImpl::ToolExitFromMethodOnToolClicked(wxCommandEvent& event)
{
    TraceExitFromMethodOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolStopProgramOnToolClicked(wxCommandEvent& event)
{
    TraceStopProgramOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolFinishProgramOnToolClicked(wxCommandEvent& event)
{
    TraceFinishProgramOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolCreateBreakpointOnToolClicked( wxCommandEvent& event )
{
// TODO: Implement ToolCreateBreakpointOnToolClicked
}

void MainDebuggerWindowImpl::ToolDeleteBreakpointOnToolClicked( wxCommandEvent& event )
{
// TODO: Implement ToolDeleteBreakpointOnToolClicked
}

void MainDebuggerWindowImpl::ToolToggleBreakpointOnToolClicked( wxCommandEvent& event )
{
// TODO: Implement ToolToggleBreakpointOnToolClicked
}

void MainDebuggerWindowImpl::ToolBreakpointOnOnToolClicked( wxCommandEvent& event )
{
// TODO: Implement ToolBreakpointOnOnToolClicked
}

void MainDebuggerWindowImpl::ToolBreakpointOffOnToolClicked( wxCommandEvent& event )
{
// TODO: Implement ToolBreakpointOffOnToolClicked
}

void MainDebuggerWindowImpl::ToolCreateWatchOnToolClicked(wxCommandEvent& event)
{
    WatchCreateOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolWatchFromSymbolListOnToolClicked(wxCommandEvent& event)
{
    WatchFromSymbolListOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolDeleteWatchOnToolClicked(wxCommandEvent& event)
{
    WatchDeleteOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ToolHelpOnToolClicked(wxCommandEvent& event)
{
    HelpIndexOnMenuSelection(event);
}

void MainDebuggerWindowImpl::ModuleIsActiveOnMenuSelection(wxCommandEvent& event)
{
    if (debug_thread_.joinable())
        return;
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    ModuleDescClientData* module_client_data_ptr =
        static_cast<ModuleDescClientData*>(ModuleList->GetClientObject(ModuleList->GetSelection()));
    const LexerInputExImpl::ModuleDescType& old_module_desc = module_client_data_ptr->GetModuleDesc();
    int save_selection = ModuleList->GetSelection();

    LexerInputExImpl::ModuleDescType new_module_desc;
    new_module_desc.module_id = old_module_desc.module_id;
    new_module_desc.module_name = old_module_desc.module_name;
    new_module_desc.module_is_active = MenuModuleIsActive->IsChecked();
    new_module_desc.module_is_main = old_module_desc.module_is_main;
    debug_controller_.GetInputLexer().FixModuleDesc(new_module_desc);
    FillProjectListBox();
    SetViewerModuleText(save_selection);
}

void MainDebuggerWindowImpl::ModuleIsMainOnMenuSelection(wxCommandEvent& event)
{
    if (debug_thread_.joinable())
        return;
    MythonDebuggerApp* this_app = static_cast<MythonDebuggerApp*>(wxTheApp);
    ModuleDescClientData* module_client_data_ptr =
        static_cast<ModuleDescClientData*>(ModuleList->GetClientObject(ModuleList->GetSelection()));
    const LexerInputExImpl::ModuleDescType& old_module_desc = module_client_data_ptr->GetModuleDesc();
    int save_selection = ModuleList->GetSelection();

    LexerInputExImpl::ModuleDescType new_module_desc;
    new_module_desc.module_id = old_module_desc.module_id;
    new_module_desc.module_name = old_module_desc.module_name;
    new_module_desc.module_is_active = old_module_desc.module_is_active;
    new_module_desc.module_is_main = MenuModuleIsMain->IsChecked();
    debug_controller_.GetInputLexer().FixModuleDesc(new_module_desc);
    FillProjectListBox();
    SetViewerModuleText(save_selection);
}

void MainDebuggerWindowImpl::EditModuleOnMenuSelection(wxCommandEvent& event)
{
    if (debug_thread_.joinable())
        return;
    ModuleListOnListBoxDClick(event);
}

void MainDebuggerWindowImpl::IndicateControllerResult(DebugController::ControllerResult& current_debug_result)
{
    wxString result_text;

    if (holds_alternative<monostate>(current_debug_result))
        result_text = _("Нет результата");
    else if (holds_alternative<DebugController::ExecutionRetcode>(current_debug_result))
        result_text = _("Успешное завершение:") +
            to_string(static_cast<int>(get<DebugController::ExecutionRetcode>(current_debug_result)));
    else if (holds_alternative<ParseError>(current_debug_result))
        result_text = _("Синтаксическая ошибка разбора:") +
            wxString::FromUTF8(get<ParseError>(current_debug_result).what());
    else if (holds_alternative<runtime_error>(current_debug_result))
        result_text = _("Ошибка исполнения:") +
            wxString::FromUTF8(get<runtime_error>(current_debug_result).what());

    wxMessageBox(result_text, _("Завершение отладки"));
}

void MainDebuggerWindowImpl::DebugEventHandler(wxCommandEvent& event)
{
    DebugController::ControllerStatus current_debug_status = debug_controller_.GetStatus();
    DebugController::ControllerResult current_debug_result = debug_controller_.GetResult();

    switch (static_cast<DebugController::ControllerRunStatus>(event.GetInt()))
    {
    case DebugController::ControllerRunStatus::CONTROL_STATUS_RUNNING:
        MainWindowStatusBar->SetStatusText(_("Работа"), 1);
        break;
    case DebugController::ControllerRunStatus::CONTROL_STATUS_STOPPED:
        MainWindowStatusBar->SetStatusText(_("Останов"), 1);
        current_point_ = current_debug_status.stop_point;
        if (current_module_id_ != current_point_.module_id)
        {
            int new_module_list_selection = ScanSelectionByModuleId(current_debug_status.stop_point.module_id);
            SetViewerModuleText(new_module_list_selection);
        }
        else
        {
            SourceViewer->MarkerDeleteAll(MARKER_CURRENT_POINT);
            if (current_debug_status.stop_point.module_string_number >= 0)
                SourceViewer->MarkerAdd(current_debug_status.stop_point.module_string_number,
                                        MARKER_CURRENT_POINT);
        }
        
        FillStackListBox();
        break;
    case DebugController::ControllerRunStatus::CONTROL_STATUS_FINISHED:
        MainWindowStatusBar->SetStatusText(_("Конец"), 1);
        if (debug_thread_.joinable())
        {
            debug_thread_.join();
            FillStackListBox();
            IndicateControllerResult(current_debug_result);
        }
        break;
    case DebugController::ControllerRunStatus::CONTROL_STATUS_UNKNOWN:
    default:
        MainWindowStatusBar->SetStatusText(_("Неизвестно"), 1);
        break;
    }
}
