///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "MythonDebuggerGui.h"

///////////////////////////////////////////////////////////////////////////

MainDebuggerWindow::MainDebuggerWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );

	ModuleList = new wxListBox( this, wxID_MODULE_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0|wxALWAYS_SHOW_SB|wxHSCROLL|wxVSCROLL );
	bSizer1->Add( ModuleList, 1, wxEXPAND, 5 );

	SourceViewer = new wxStyledTextCtrl( this, wxID_SOURCE_VIEWER, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	SourceViewer->SetUseTabs( true );
	SourceViewer->SetTabWidth( 4 );
	SourceViewer->SetIndent( 4 );
	SourceViewer->SetTabIndents( true );
	SourceViewer->SetBackSpaceUnIndents( true );
	SourceViewer->SetViewEOL( false );
	SourceViewer->SetViewWhiteSpace( false );
	SourceViewer->SetMarginWidth( 2, 0 );
	SourceViewer->SetIndentationGuides( true );
	SourceViewer->SetReadOnly( true );
	SourceViewer->SetMarginWidth( 1, 0 );
	SourceViewer->SetMarginType( 0, wxSTC_MARGIN_NUMBER );
	SourceViewer->SetMarginWidth( 0, SourceViewer->TextWidth( wxSTC_STYLE_LINENUMBER, wxT("_99999") ) );
	SourceViewer->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	SourceViewer->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	SourceViewer->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	SourceViewer->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	SourceViewer->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	SourceViewer->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	SourceViewer->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	SourceViewer->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	SourceViewer->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	SourceViewer->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	SourceViewer->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	SourceViewer->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	SourceViewer->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	SourceViewer->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	SourceViewer->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	SourceViewer->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	SourceViewer->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	bSizer1->Add( SourceViewer, 2, wxEXPAND, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );

	BreakPointsList = new wxListBox( this, wxID_BREAKPOINTS_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0|wxALWAYS_SHOW_SB|wxHSCROLL|wxVSCROLL );
	bSizer2->Add( BreakPointsList, 1, wxEXPAND, 5 );

	SymbolsList = new wxListBox( this, wxID_SYMBOLS_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0|wxALWAYS_SHOW_SB|wxHSCROLL|wxVSCROLL );
	bSizer2->Add( SymbolsList, 1, wxEXPAND, 5 );

	StackList = new wxListBox( this, wxID_STACK_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, 0|wxALWAYS_SHOW_SB|wxHSCROLL|wxVSCROLL );
	bSizer2->Add( StackList, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer2, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	MainWindowStatusBar = this->CreateStatusBar( 2, wxSTB_SIZEGRIP, wxID_MAIN_STATUSBAR );
	MainWindowMenuBar = new wxMenuBar( 0 );
	file_menu = new wxMenu();
	wxMenuItem* FileCreateProject;
	FileCreateProject = new wxMenuItem( file_menu, wxID_CREATE_NEW_PROJECT, wxString( _("Создать новый проект") ) , _("Создаёт новый отладочный проект"), wxITEM_NORMAL );
	file_menu->Append( FileCreateProject );

	wxMenuItem* FileOpenProject;
	FileOpenProject = new wxMenuItem( file_menu, wxID_OPEN_PROJECT, wxString( _("Открыть проект") ) + wxT('\t') + wxT("F3"), _("Открывает существующий проект"), wxITEM_NORMAL );
	file_menu->Append( FileOpenProject );

	wxMenuItem* FileSaveProject;
	FileSaveProject = new wxMenuItem( file_menu, wxID_SAVE_PROJECT, wxString( _("Сохранить проект") ) + wxT('\t') + wxT("F2"), _("Сохранить отладочный проект в файл"), wxITEM_NORMAL );
	file_menu->Append( FileSaveProject );

	wxMenuItem* FileSaveAsProject;
	FileSaveAsProject = new wxMenuItem( file_menu, wxID_SAVE_PROJECT_AS, wxString( _("Сохранить как") ) + wxT('\t') + wxT("Alt-F2"), _("Сохранить как"), wxITEM_NORMAL );
	file_menu->Append( FileSaveAsProject );

	file_menu->AppendSeparator();

	wxMenuItem* FileAddOneFile;
	FileAddOneFile = new wxMenuItem( file_menu, wxID_LOAD_ONE_FILE, wxString( _("Добавить модуль к проекту") ) + wxT('\t') + wxT("Ctrl+Alt+F3"), _("Добавляет существующий модуль к открытому проекту"), wxITEM_NORMAL );
	file_menu->Append( FileAddOneFile );

	file_menu->AppendSeparator();

	wxMenuItem* FileExitProgram;
	FileExitProgram = new wxMenuItem( file_menu, wxID_EXIT_PROGRAM, wxString( _("Выйти из программы") ) + wxT('\t') + wxT("Alt-F4"), _("Завершает работу отладчика"), wxITEM_NORMAL );
	file_menu->Append( FileExitProgram );

	MainWindowMenuBar->Append( file_menu, _("Файл") );

	view_menu = new wxMenu();
	wxMenuItem* ViewOutputWindow;
	ViewOutputWindow = new wxMenuItem( view_menu, wxID_VIEW_OUTPUT_WINDOW, wxString( _("Показать окно консоли") ) + wxT('\t') + wxT("Shift-F3"), _("Делает видимым окно консоли отлаживаемой программы"), wxITEM_NORMAL );
	view_menu->Append( ViewOutputWindow );

	wxMenuItem* ViewConfig;
	ViewConfig = new wxMenuItem( view_menu, wxID_VIEW_CONFIG, wxString( _("Настройка") ) , _("Настрока программы"), wxITEM_NORMAL );
	view_menu->Append( ViewConfig );

	ViewAvailLanguages = new wxMenu();
	wxMenuItem* ViewAvailLanguagesItem = new wxMenuItem( view_menu, wxID_ANY, _("Доступные языки"), wxEmptyString, wxITEM_NORMAL, ViewAvailLanguages );
	view_menu->Append( ViewAvailLanguagesItem );

	MainWindowMenuBar->Append( view_menu, _("Вид") );

	trace_menu = new wxMenu();
	wxMenuItem* TraceRunWithDebug;
	TraceRunWithDebug = new wxMenuItem( trace_menu, wxID_RUN_DEBUG_PROGRAM, wxString( _("Начать/Продолжить отладку программы ") ) + wxT('\t') + wxT("F5"), _("Запускает или продолжает работу программы под отладчиком"), wxITEM_NORMAL );
	trace_menu->Append( TraceRunWithDebug );

	wxMenuItem* TraceRunFree;
	TraceRunFree = new wxMenuItem( trace_menu, wxID_RUN_PROGRAM, wxString( _("Запустить программу без отладчика") ) + wxT('\t') + wxT("Ctrl-F5"), _("Запускает программу без отладки, в свободном исполнении"), wxITEM_NORMAL );
	trace_menu->Append( TraceRunFree );

	trace_menu->AppendSeparator();

	wxMenuItem* TraceIn;
	TraceIn = new wxMenuItem( trace_menu, wxID_TRACE_IN, wxString( _("Шаг с заходом") ) + wxT('\t') + wxT("F11"), _("Шаг трассировки с заходом в методы"), wxITEM_NORMAL );
	trace_menu->Append( TraceIn );

	wxMenuItem* TraceOut;
	TraceOut = new wxMenuItem( trace_menu, wxID_TRACE_OUT, wxString( _("Шаг с обходом") ) + wxT('\t') + wxT("F10"), _("Шаг трассировки, не заходя в тела методов"), wxITEM_NORMAL );
	trace_menu->Append( TraceOut );

	wxMenuItem* TraceToCursor;
	TraceToCursor = new wxMenuItem( trace_menu, wxID_GOTO_TO_CURSOR, wxString( _("Исполнить до курсора") ) + wxT('\t') + wxT("Ctrl-F10"), _("Исполнить программу до точки, на которой расположен курсор"), wxITEM_NORMAL );
	trace_menu->Append( TraceToCursor );

	wxMenuItem* TraceExitFromMethod;
	TraceExitFromMethod = new wxMenuItem( trace_menu, wxID_EXIT_METHOD, wxString( _("Выход из метода") ) + wxT('\t') + wxT("Shift-F11"), _("Исполняет программу до оператора выхода из метода"), wxITEM_NORMAL );
	trace_menu->Append( TraceExitFromMethod );

	trace_menu->AppendSeparator();

	wxMenuItem* TraceStopProgram;
	TraceStopProgram = new wxMenuItem( trace_menu, wxID_STOP_PROGRAM, wxString( _("Приостановка исполнения программмы") ) + wxT('\t') + wxT("Shift-F5"), _("Приостанавливает исполнение отлаживаемой программы"), wxITEM_NORMAL );
	trace_menu->Append( TraceStopProgram );

	wxMenuItem* TraceFinishProgram;
	TraceFinishProgram = new wxMenuItem( trace_menu, wxID_FINISH_PROGRAM, wxString( _("Прекратить исполнение программы") ) + wxT('\t') + wxT("Alt-Shift-F5"), _("Немедленно завершает работу отлаживаемой программой"), wxITEM_NORMAL );
	trace_menu->Append( TraceFinishProgram );

	MainWindowMenuBar->Append( trace_menu, _("Трассировка") );

	breakpoint_menu = new wxMenu();
	wxMenuItem* BreakpointCreate;
	BreakpointCreate = new wxMenuItem( breakpoint_menu, wxID_CREATE_BREAK, wxString( _("Создать точку останова") ) + wxT('\t') + wxT("Ctrl-B"), _("Создаёт новую точку останова"), wxITEM_NORMAL );
	#ifdef __WXMSW__
	BreakpointCreate->SetBitmaps( wxNullBitmap );
	#elif (defined( __WXGTK__ ) || defined( __WXOSX__ ))
	BreakpointCreate->SetBitmap( wxNullBitmap );
	#endif
	breakpoint_menu->Append( BreakpointCreate );

	wxMenuItem* BreakpointCreateConditional;
	BreakpointCreateConditional = new wxMenuItem( breakpoint_menu, wxID_CREATE_COND_BREAK, wxString( _("Создать условную точку останова") ) + wxT('\t') + wxT("Shift-Ctrl-B"), _("Создаёт условную точку останова"), wxITEM_NORMAL );
	breakpoint_menu->Append( BreakpointCreateConditional );

	wxMenuItem* BreakpointDelete;
	BreakpointDelete = new wxMenuItem( breakpoint_menu, wxID_DELETE_BREAK, wxString( _("Удалить точку останова") ) + wxT('\t') + wxT("Alt-Ctrl-B"), _("Удаляет точку останова под курсором"), wxITEM_NORMAL );
	breakpoint_menu->Append( BreakpointDelete );

	wxMenuItem* BreakpointDeleteAll;
	BreakpointDeleteAll = new wxMenuItem( breakpoint_menu, wxID_DELETE_ALL_BREAK, wxString( _("Удалить все точки останова") ) , _("Удаляет все существующие точки останова"), wxITEM_NORMAL );
	breakpoint_menu->Append( BreakpointDeleteAll );

	breakpoint_menu->AppendSeparator();

	wxMenuItem* BreakpointOn;
	BreakpointOn = new wxMenuItem( breakpoint_menu, wxID_ENABLE_BREAK, wxString( _("Включить точку останова") ) + wxT('\t') + wxT("Alt-F9"), _("Активизирует точку останова под курсором"), wxITEM_NORMAL );
	breakpoint_menu->Append( BreakpointOn );

	wxMenuItem* BreakpointOff;
	BreakpointOff = new wxMenuItem( breakpoint_menu, wxID_ANY, wxString( _("Выключить точку останова") ) + wxT('\t') + wxT("Ctrl-F9"), _("Отключает точку останова под курсором"), wxITEM_NORMAL );
	breakpoint_menu->Append( BreakpointOff );

	wxMenuItem* BreakpointEnableAll;
	BreakpointEnableAll = new wxMenuItem( breakpoint_menu, wxID_ENABLE_ALL_BREAK, wxString( _("Включить все точки останова") ) , _("Активрует все существующие точки останова"), wxITEM_NORMAL );
	breakpoint_menu->Append( BreakpointEnableAll );

	wxMenuItem* BreakpointDisableAll;
	BreakpointDisableAll = new wxMenuItem( breakpoint_menu, wxID_DISABLE_ALL_BREAK, wxString( _("Отключить все точки останова") ) , _("Отключает все существующие точки останова"), wxITEM_NORMAL );
	breakpoint_menu->Append( BreakpointDisableAll );

	breakpoint_menu->AppendSeparator();

	wxMenuItem* BreakpointToggle;
	BreakpointToggle = new wxMenuItem( breakpoint_menu, wxID_GOTO_NEXT_BREAK, wxString( _("Переключить точку останова") ) + wxT('\t') + wxT("F9"), _("Включает/выключает точку останова в текущей строке исходника"), wxITEM_NORMAL );
	breakpoint_menu->Append( BreakpointToggle );

	MainWindowMenuBar->Append( breakpoint_menu, _("Точки останова") );

	symbol_menu = new wxMenu();
	wxMenuItem* WatchCreate;
	WatchCreate = new wxMenuItem( symbol_menu, wxID_CREATE_WATCH, wxString( _("Добавить слежение") ) + wxT('\t') + wxT("Ctrl-F7"), _("Создаёт новый элемент слежения за символом"), wxITEM_NORMAL );
	symbol_menu->Append( WatchCreate );

	wxMenuItem* WatchDelete;
	WatchDelete = new wxMenuItem( symbol_menu, wxID_DELETE_WATCH, wxString( _("Удалить слежение") ) + wxT('\t') + wxT("Alt-F7"), _("Удаляет существующий элемент слежения"), wxITEM_NORMAL );
	symbol_menu->Append( WatchDelete );

	symbol_menu->AppendSeparator();

	wxMenuItem* SymbolsSave;
	SymbolsSave = new wxMenuItem( symbol_menu, wxID_SAVE_SYMBOL_INFO, wxString( _("Сохранить сведения о символах") ) , _("Сохраняет всю символьную информацию в текстовый файл"), wxITEM_NORMAL );
	symbol_menu->Append( SymbolsSave );

	MainWindowMenuBar->Append( symbol_menu, _("Символы") );

	help_menu = new wxMenu();
	wxMenuItem* HelpIndex;
	HelpIndex = new wxMenuItem( help_menu, wxID_HELP_INDEX, wxString( _("Индекс помощи") ) + wxT('\t') + wxT("F1"), _("Вызывает систему помощи программы"), wxITEM_NORMAL );
	help_menu->Append( HelpIndex );

	wxMenuItem* HelpAbout;
	HelpAbout = new wxMenuItem( help_menu, wxID_HELP_ABOUT, wxString( _("О программе") ) , _("Вызывает дилог с краткими сведениями о программе"), wxITEM_NORMAL );
	help_menu->Append( HelpAbout );

	MainWindowMenuBar->Append( help_menu, _("Помощь") );

	this->SetMenuBar( MainWindowMenuBar );

	MainWindowToolBar = this->CreateToolBar( wxTB_HORIZONTAL, wxID_MAIN_TOOLBAR );
	ToolCreateProject = MainWindowToolBar->AddTool( wxID_TOOL_CREATE_PROJECT, wxEmptyString, wxArtProvider::GetBitmap( wxART_NEW, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Создать новый проект"), _("Создание нового отладочного проекта"), NULL );

	ToolOpenProject = MainWindowToolBar->AddTool( wxID_TOOL_OPEN_PROJECT, wxEmptyString, wxArtProvider::GetBitmap( wxART_FILE_OPEN, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Открыть файл проекта"), _("Открывает существующий файл отладочного проекта"), NULL );

	ToolSaveProject = MainWindowToolBar->AddTool( wxID_TOOL_SAVE_PROJECT, wxEmptyString, wxArtProvider::GetBitmap( wxART_FILE_SAVE, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Сохранить проект"), _("Сохранить проект под текущим именем"), NULL );

	ToolSaveAsProject = MainWindowToolBar->AddTool( wxID_TOOL_SAVE_PROJECT_AS, wxEmptyString, wxArtProvider::GetBitmap( wxART_FILE_SAVE_AS, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Сохранить проект под другим именем"), _("Изменить имя текущего проекта"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolAddOneFile = MainWindowToolBar->AddTool( wxID_TOOL_ADD_ONE_FILE, wxEmptyString, wxArtProvider::GetBitmap( wxART_PLUS, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Добавить исходный файл к проекту"), _("Добавить один исходный модуль к проекту со всеми внутренне подключаемыми модулями"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolExitProgram = MainWindowToolBar->AddTool( wxID_TOOL_EXIT_PROGRAM, wxEmptyString, wxArtProvider::GetBitmap( wxART_QUIT, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Выход из программы"), _("Завершить работу данной программы"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolRunWithDebug = MainWindowToolBar->AddTool( wxID_TOOL_RUN_DEBUG, wxEmptyString, wxArtProvider::GetBitmap( wxART_GOTO_LAST, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Запуск под отладкой"), _("Начать отладку - запустить программу под контролем отладчика"), NULL );

	ToolRunFree = MainWindowToolBar->AddTool( wxID_TOOL_RUN_FREE, wxEmptyString, wxArtProvider::GetBitmap( wxART_GO_FORWARD, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Запуск без отладки"), _("Запустить программу без контроля отладчика"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolTraceIn = MainWindowToolBar->AddTool( wxID_TOOL_TRACE_IN, wxEmptyString, wxArtProvider::GetBitmap( wxART_GO_TO_PARENT, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Выполнить с заходом"), _("Сдклать шаг исполнения отлаживаемой программы, заходя в вызовы функций"), NULL );

	ToolTraceOut = MainWindowToolBar->AddTool( wxID_TOOL_TRACE_OUT, wxEmptyString, wxArtProvider::GetBitmap( wxART_REDO, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Выполнить с обходом"), _("Сделать шаг выполнения отлаживаемой программы, не заходя  в вызовы функций"), NULL );

	ToolTraceToCursor = MainWindowToolBar->AddTool( wxID_TRACE_TO_CURSOR, wxEmptyString, wxArtProvider::GetBitmap( wxART_GO_UP, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Исполнять до курсора"), _("Выполнить отлаживаемую программу до текущей строки под курсором"), NULL );

	ToolExitFromMethod = MainWindowToolBar->AddTool( wxID_TOOL_EXIT_FROM_METHOD, wxEmptyString, wxArtProvider::GetBitmap( wxART_UNDO, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Выйти из функции"), _("Исполнить программу до операторов возврата из функции"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolStopProgram = MainWindowToolBar->AddTool( wxID_TOOL_STOP_PROGRAM, wxEmptyString, wxArtProvider::GetBitmap( wxART_CROSS_MARK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Приостановка программы"), _("Приостановить (вернуть в состояние останова) текущую отлаживаемую программу"), NULL );

	ToolFinishProgram = MainWindowToolBar->AddTool( wxID_TOOL_FINISH_PROGRAM, wxEmptyString, wxArtProvider::GetBitmap( wxART_DELETE, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Завершить программу"), _("Прекратить исполнение работающей программы"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolCreateBreakpoint = MainWindowToolBar->AddTool( wxID_TOOL_CREATE_BREAK, wxEmptyString, wxArtProvider::GetBitmap( wxART_ADD_BOOKMARK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Создание точки останова"), _("Создаёт новую точку останова, указывающую на текущую строку исходника"), NULL );

	ToolDeleteBreakpoint = MainWindowToolBar->AddTool( wxID_TOOL_DELETE_BREAK, wxEmptyString, wxArtProvider::GetBitmap( wxART_DEL_BOOKMARK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Удаление текущей точки останова"), _("Удаление текущей (под курсором) точки останова"), NULL );

	ToolToggleBreakpoint = MainWindowToolBar->AddTool( wxID_TOOL_TOGGLE_BREAK, wxEmptyString, wxArtProvider::GetBitmap( wxART_GO_TO_PARENT, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Переброс точки останова"), _("Создать/удалить точку останова в текущем положении"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolBreakpointOn = MainWindowToolBar->AddTool( wxID_TOOL_BREAK_ON, wxEmptyString, wxArtProvider::GetBitmap( wxART_TICK_MARK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Включение точки останова"), _("Включение (активация) точки останова, на которую указвает курсор"), NULL );

	ToolBreakpointOff = MainWindowToolBar->AddTool( wxID_BREAK_OFF, wxEmptyString, wxArtProvider::GetBitmap( wxART_CROSS_MARK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Выключение точки останова"), _("Выключает (деактивирует) точку останова, на которую указывает курсор"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolCreateWatch = MainWindowToolBar->AddTool( wxID_TOOL_CREATE_WATCH, wxEmptyString, wxArtProvider::GetBitmap( wxART_ADD_BOOKMARK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Установить наблюдение"), _("Создать элемент наблюдения над состоянием символа"), NULL );

	ToolDeleteWatch = MainWindowToolBar->AddTool( wxID_TOOL_DELETE_WATCH, wxEmptyString, wxArtProvider::GetBitmap( wxART_DEL_BOOKMARK, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Снять наблюдение с символа"), _("Удаляет существующий элемент наблюдения над символом"), NULL );

	MainWindowToolBar->AddSeparator();

	ToolHelp = MainWindowToolBar->AddTool( wxID_HELP, wxEmptyString, wxArtProvider::GetBitmap( wxART_HELP, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, _("Помощь по работе с программой"), _("Вызывает систему помощи по работе с программой"), NULL );

	MainWindowToolBar->Realize();

	EditModuleMenu = new wxMenu();
	MenuModuleName = new wxMenuItem( EditModuleMenu, wxID_MENU_MODULE_NAME, wxString( _("Anonymous") ) , _("Имя текущего модуля"), wxITEM_NORMAL );
	EditModuleMenu->Append( MenuModuleName );

	EditModuleMenu->AppendSeparator();

	MenuModuleIsActive = new wxMenuItem( EditModuleMenu, wxID_MENU_MODULE_ACTIVE, wxString( _("Активен") ) , _("Модуль активен"), wxITEM_CHECK );
	EditModuleMenu->Append( MenuModuleIsActive );

	MenuModuleIsMain = new wxMenuItem( EditModuleMenu, wxID_MENU_MODULE_MAIN, wxString( _("Главный") ) , _("Модуль главный (стартовый)"), wxITEM_CHECK );
	EditModuleMenu->Append( MenuModuleIsMain );

	EditModuleMenu->AppendSeparator();

	wxMenuItem* EditModule;
	EditModule = new wxMenuItem( EditModuleMenu, wxID_MENU_EDIT_MODULE, wxString( _("Редактировать") ) , _("Вызов диалога редактирования"), wxITEM_NORMAL );
	EditModuleMenu->Append( EditModule );

	this->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDebuggerWindow::MainDebuggerWindowOnContextMenu ), NULL, this );


	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDebuggerWindow::MainDebuggerWindowOnClose ) );
	ModuleList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDebuggerWindow::ModuleListOnListBox ), NULL, this );
	ModuleList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDebuggerWindow::ModuleListOnListBoxDClick ), NULL, this );
	ModuleList->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDebuggerWindow::ModuleListOnRightDown ), NULL, this );
	SourceViewer->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( MainDebuggerWindow::SourceViewerOnKeyDown ), NULL, this );
	SourceViewer->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( MainDebuggerWindow::SourceViewerOnLeftDown ), NULL, this );
	SourceViewer->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDebuggerWindow::SourceViewerOnRightDown ), NULL, this );
	BreakPointsList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakPointsListOnListBox ), NULL, this );
	BreakPointsList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDebuggerWindow::BreakPointsListOnListBoxDClick ), NULL, this );
	SymbolsList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDebuggerWindow::SymbolsListOnListBox ), NULL, this );
	SymbolsList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDebuggerWindow::SymbolsListOnListBoxDClick ), NULL, this );
	StackList->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDebuggerWindow::StackListOnListBox ), NULL, this );
	StackList->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDebuggerWindow::StackListOnListBoxDClick ), NULL, this );
	file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::CreateProjectOnMenuSelection ), this, FileCreateProject->GetId());
	file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::FileLoadProjectOnMenuSelection ), this, FileOpenProject->GetId());
	file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::FileSaveProjectOnMenuSelection ), this, FileSaveProject->GetId());
	file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::FileSaveAsProjectOnMenuSelection ), this, FileSaveAsProject->GetId());
	file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::FileAddOneFileOnMenuSelection ), this, FileAddOneFile->GetId());
	file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::FileExitProgramOnMenuSelection ), this, FileExitProgram->GetId());
	view_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::ViewOutputWindowOnMenuSelection ), this, ViewOutputWindow->GetId());
	view_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::ViewConfigOnMenuSelection ), this, ViewConfig->GetId());
	trace_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::TraceRunWithDebugOnMenuSelection ), this, TraceRunWithDebug->GetId());
	trace_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::TraceRunFreeOnMenuSelection ), this, TraceRunFree->GetId());
	trace_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::TraceInOnMenuSelection ), this, TraceIn->GetId());
	trace_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::TraceOutOnMenuSelection ), this, TraceOut->GetId());
	trace_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::TraceToCursorOnMenuSelection ), this, TraceToCursor->GetId());
	trace_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::TraceExitFromMethodOnMenuSelection ), this, TraceExitFromMethod->GetId());
	trace_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::TraceStopProgramOnMenuSelection ), this, TraceStopProgram->GetId());
	trace_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::TraceFinishProgramOnMenuSelection ), this, TraceFinishProgram->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointCreateOnMenuSelection ), this, BreakpointCreate->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointCreateConditionalOnMenuSelection ), this, BreakpointCreateConditional->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointDeleteOnMenuSelection ), this, BreakpointDelete->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointDeleteAllOnMenuSelection ), this, BreakpointDeleteAll->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointOnOnMenuSelection ), this, BreakpointOn->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointOffOnMenuSelection ), this, BreakpointOff->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointEnableAllOnMenuSelection ), this, BreakpointEnableAll->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointDisableAllOnMenuSelection ), this, BreakpointDisableAll->GetId());
	breakpoint_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakpointToggleOnMenuSelection ), this, BreakpointToggle->GetId());
	symbol_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::WatchCreateOnMenuSelection ), this, WatchCreate->GetId());
	symbol_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::WatchDeleteOnMenuSelection ), this, WatchDelete->GetId());
	symbol_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::SymbolsSaveOnMenuSelection ), this, SymbolsSave->GetId());
	help_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::HelpIndexOnMenuSelection ), this, HelpIndex->GetId());
	help_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::HelpAboutOnMenuSelection ), this, HelpAbout->GetId());
	this->Connect( ToolCreateProject->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolCreateProjectOnToolClicked ) );
	this->Connect( ToolOpenProject->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolOpenProjectOnToolClicked ) );
	this->Connect( ToolSaveProject->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolSaveProjectOnToolClicked ) );
	this->Connect( ToolSaveAsProject->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolSaveAsProjectOnToolClicked ) );
	this->Connect( ToolAddOneFile->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolAddOneFileOnToolClicked ) );
	this->Connect( ToolExitProgram->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolExitProgramOnToolClicked ) );
	this->Connect( ToolRunWithDebug->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolRunWithDebugOnToolClicked ) );
	this->Connect( ToolRunFree->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolRunFreeOnToolClicked ) );
	this->Connect( ToolTraceIn->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolTraceInOnToolClicked ) );
	this->Connect( ToolTraceOut->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolTraceOutOnToolClicked ) );
	this->Connect( ToolTraceToCursor->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolTraceToCursorOnToolClicked ) );
	this->Connect( ToolExitFromMethod->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolExitFromMethodOnToolClicked ) );
	this->Connect( ToolStopProgram->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolStopProgramOnToolClicked ) );
	this->Connect( ToolFinishProgram->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolFinishProgramOnToolClicked ) );
	this->Connect( ToolCreateBreakpoint->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolCreateBreakpointOnToolClicked ) );
	this->Connect( ToolDeleteBreakpoint->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolDeleteBreakpointOnToolClicked ) );
	this->Connect( ToolToggleBreakpoint->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolToggleBreakpointOnToolClicked ) );
	this->Connect( ToolBreakpointOn->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolBreakpointOnOnToolClicked ) );
	this->Connect( ToolBreakpointOff->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolBreakpointOffOnToolClicked ) );
	this->Connect( ToolHelp->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolHelpOnToolClicked ) );
	EditModuleMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::ModuleIsActiveOnMenuSelection ), this, MenuModuleIsActive->GetId());
	EditModuleMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::ModuleIsMainOnMenuSelection ), this, MenuModuleIsMain->GetId());
	EditModuleMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( MainDebuggerWindow::EditModuleOnMenuSelection ), this, EditModule->GetId());
}

MainDebuggerWindow::~MainDebuggerWindow()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( MainDebuggerWindow::MainDebuggerWindowOnClose ) );
	ModuleList->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDebuggerWindow::ModuleListOnListBox ), NULL, this );
	ModuleList->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDebuggerWindow::ModuleListOnListBoxDClick ), NULL, this );
	ModuleList->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDebuggerWindow::ModuleListOnRightDown ), NULL, this );
	SourceViewer->Disconnect( wxEVT_KEY_DOWN, wxKeyEventHandler( MainDebuggerWindow::SourceViewerOnKeyDown ), NULL, this );
	SourceViewer->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( MainDebuggerWindow::SourceViewerOnLeftDown ), NULL, this );
	SourceViewer->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( MainDebuggerWindow::SourceViewerOnRightDown ), NULL, this );
	BreakPointsList->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDebuggerWindow::BreakPointsListOnListBox ), NULL, this );
	BreakPointsList->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDebuggerWindow::BreakPointsListOnListBoxDClick ), NULL, this );
	SymbolsList->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDebuggerWindow::SymbolsListOnListBox ), NULL, this );
	SymbolsList->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDebuggerWindow::SymbolsListOnListBoxDClick ), NULL, this );
	StackList->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( MainDebuggerWindow::StackListOnListBox ), NULL, this );
	StackList->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxCommandEventHandler( MainDebuggerWindow::StackListOnListBoxDClick ), NULL, this );
	this->Disconnect( ToolCreateProject->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolCreateProjectOnToolClicked ) );
	this->Disconnect( ToolOpenProject->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolOpenProjectOnToolClicked ) );
	this->Disconnect( ToolSaveProject->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolSaveProjectOnToolClicked ) );
	this->Disconnect( ToolSaveAsProject->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolSaveAsProjectOnToolClicked ) );
	this->Disconnect( ToolAddOneFile->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolAddOneFileOnToolClicked ) );
	this->Disconnect( ToolExitProgram->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolExitProgramOnToolClicked ) );
	this->Disconnect( ToolRunWithDebug->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolRunWithDebugOnToolClicked ) );
	this->Disconnect( ToolRunFree->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolRunFreeOnToolClicked ) );
	this->Disconnect( ToolTraceIn->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolTraceInOnToolClicked ) );
	this->Disconnect( ToolTraceOut->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolTraceOutOnToolClicked ) );
	this->Disconnect( ToolTraceToCursor->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolTraceToCursorOnToolClicked ) );
	this->Disconnect( ToolExitFromMethod->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolExitFromMethodOnToolClicked ) );
	this->Disconnect( ToolStopProgram->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolStopProgramOnToolClicked ) );
	this->Disconnect( ToolFinishProgram->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolFinishProgramOnToolClicked ) );
	this->Disconnect( ToolCreateBreakpoint->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolCreateBreakpointOnToolClicked ) );
	this->Disconnect( ToolDeleteBreakpoint->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolDeleteBreakpointOnToolClicked ) );
	this->Disconnect( ToolToggleBreakpoint->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolToggleBreakpointOnToolClicked ) );
	this->Disconnect( ToolBreakpointOn->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolBreakpointOnOnToolClicked ) );
	this->Disconnect( ToolBreakpointOff->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolBreakpointOffOnToolClicked ) );
	this->Disconnect( ToolHelp->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( MainDebuggerWindow::ToolHelpOnToolClicked ) );

	delete EditModuleMenu;
}

OutputDebuggerWindow::OutputDebuggerWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	DebuggerWindowStatusBer = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_DEBUGGER_STATUSBAR );
	DebuggerWindowMenuBar = new wxMenuBar( 0 );
	debug_file_menu = new wxMenu();
	wxMenuItem* DebugOutputSave;
	DebugOutputSave = new wxMenuItem( debug_file_menu, wxID_MENU_DEBUG_SAVE_OUTPUT, wxString( _("Сохранить") ) + wxT('\t') + wxT("F2"), _("Сохраняет вывод программы в файл"), wxITEM_NORMAL );
	debug_file_menu->Append( DebugOutputSave );

	wxMenuItem* DebugOutputSaveAs;
	DebugOutputSaveAs = new wxMenuItem( debug_file_menu, wxID_MENU_DEBUG_SAVE_OUTPUT_AS, wxString( _("Сохранить как") ) + wxT('\t') + wxT("Ctrl-F2"), _("Сохраняет вывод отлаживаемой программы в файл под любым именем"), wxITEM_NORMAL );
	debug_file_menu->Append( DebugOutputSaveAs );

	wxMenuItem* DebugOutputClear;
	DebugOutputClear = new wxMenuItem( debug_file_menu, wxID_MENU_DEBUG_CLEAR_WINDOW, wxString( _("Очистить") ) + wxT('\t') + wxT("Alt-F7"), _("Очищает окно вывода программы"), wxITEM_NORMAL );
	debug_file_menu->Append( DebugOutputClear );

	debug_file_menu->AppendSeparator();

	wxMenuItem* DebugClose;
	DebugClose = new wxMenuItem( debug_file_menu, wxID_MENU_DEBUG_CLOSE_WINDOW, wxString( _("Закрыть окно") ) + wxT('\t') + wxT("Alt-F4"), _("Закрывает данное отладочное окно"), wxITEM_NORMAL );
	debug_file_menu->Append( DebugClose );

	DebuggerWindowMenuBar->Append( debug_file_menu, _("Файл") );

	this->SetMenuBar( DebuggerWindowMenuBar );

	DebuggerWindowToolBar = this->CreateToolBar( wxTB_HORIZONTAL, wxID_DEBUGGER_TOOLBAR );
	ToolDebugOutputSave = DebuggerWindowToolBar->AddTool( wxID_SAVE_DEBUG_OUTPUT, wxEmptyString, wxArtProvider::GetBitmap( wxART_FILE_SAVE, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	ToolDebugOutputSaveAs = DebuggerWindowToolBar->AddTool( wxID_SAVE_AS_DEBUG_OUTPUT, wxEmptyString, wxArtProvider::GetBitmap( wxART_FILE_SAVE_AS, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	ToolDebugClearWindow = DebuggerWindowToolBar->AddTool( wxID_DEBUG_CLEAR_WINDOW, wxEmptyString, wxArtProvider::GetBitmap( wxART_DELETE, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	DebuggerWindowToolBar->AddSeparator();

	ToolDebugCloseWindow = DebuggerWindowToolBar->AddTool( wxID_DEBUG_CLOSE_WINDOW, wxEmptyString, wxArtProvider::GetBitmap( wxART_QUIT, wxART_TOOLBAR ), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

	DebuggerWindowToolBar->Realize();

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	DebugOutputText = new wxTextCtrl( this, wxID_DEBUG_OUTPUT_TEXT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxTE_MULTILINE|wxTE_READONLY );
	bSizer3->Add( DebugOutputText, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer3 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( OutputDebuggerWindow::OutputDebuggerWindowOnClose ) );
	debug_file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OutputDebuggerWindow::DebugOutputSaveOnMenuSelection ), this, DebugOutputSave->GetId());
	debug_file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OutputDebuggerWindow::DebugOutputSaveAsOnMenuSelection ), this, DebugOutputSaveAs->GetId());
	debug_file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OutputDebuggerWindow::DebugOutputClearOnMenuSelection ), this, DebugOutputClear->GetId());
	debug_file_menu->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OutputDebuggerWindow::DebugCloseOnMenuSelection ), this, DebugClose->GetId());
	this->Connect( ToolDebugOutputSave->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OutputDebuggerWindow::ToolDebugOutputSaveOnToolClicked ) );
	this->Connect( ToolDebugOutputSaveAs->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OutputDebuggerWindow::ToolDebugOutputSaveAsOnToolClicked ) );
	this->Connect( ToolDebugClearWindow->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OutputDebuggerWindow::ToolDebugClearWindowOnToolClicked ) );
	this->Connect( ToolDebugCloseWindow->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OutputDebuggerWindow::ToolDebugCloseWindowOnToolClicked ) );
}

OutputDebuggerWindow::~OutputDebuggerWindow()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( OutputDebuggerWindow::OutputDebuggerWindowOnClose ) );
	this->Disconnect( ToolDebugOutputSave->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OutputDebuggerWindow::ToolDebugOutputSaveOnToolClicked ) );
	this->Disconnect( ToolDebugOutputSaveAs->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OutputDebuggerWindow::ToolDebugOutputSaveAsOnToolClicked ) );
	this->Disconnect( ToolDebugClearWindow->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OutputDebuggerWindow::ToolDebugClearWindowOnToolClicked ) );
	this->Disconnect( ToolDebugCloseWindow->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OutputDebuggerWindow::ToolDebugCloseWindowOnToolClicked ) );

}

ConfigDialog::ConfigDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxVERTICAL );

	wxStaticBoxSizer* ConfigSourceCode;
	ConfigSourceCode = new wxStaticBoxSizer( new wxStaticBox( this, wxID_CONFIG_SOURCE_CODE, _("Исходники") ), wxVERTICAL );

	SourceIsUtf8 = new wxCheckBox( ConfigSourceCode->GetStaticBox(), wxID_SOURCE_UTF8, _("В UTF-8"), wxDefaultPosition, wxDefaultSize, 0 );
	ConfigSourceCode->Add( SourceIsUtf8, 0, wxALL, 5 );

	SourceIsFullSave = new wxCheckBox( ConfigSourceCode->GetStaticBox(), wxID_SOURCE_SAVE_IN_PROJECT, _("Сохранять в проекте"), wxDefaultPosition, wxDefaultSize, 0 );
	ConfigSourceCode->Add( SourceIsFullSave, 0, wxALL, 5 );


	bSizer7->Add( ConfigSourceCode, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	SaveConfig = new wxButton( this, wxID_ANY, _("Сохранить"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( SaveConfig, 0, wxALL, 5 );

	CancelConfig = new wxButton( this, wxID_ANY, _("Отменить"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( CancelConfig, 0, wxALL, 5 );

	RestoreCurrentConfig = new wxButton( this, wxID_ANY, _("Текущая"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( RestoreCurrentConfig, 0, wxALL, 5 );


	bSizer7->Add( bSizer8, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer7 );
	this->Layout();
	bSizer7->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ConfigDialog::ConfigDialogOnClose ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( ConfigDialog::ConfigDialogOnInitDialog ) );
	SaveConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigDialog::SaveConfigOnButtonClick ), NULL, this );
	CancelConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigDialog::CancelConfigOnButtonClick ), NULL, this );
	RestoreCurrentConfig->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigDialog::RestoreCurrentConfigOnButtonClick ), NULL, this );
}

ConfigDialog::~ConfigDialog()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ConfigDialog::ConfigDialogOnClose ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( ConfigDialog::ConfigDialogOnInitDialog ) );
	SaveConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigDialog::SaveConfigOnButtonClick ), NULL, this );
	CancelConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigDialog::CancelConfigOnButtonClick ), NULL, this );
	RestoreCurrentConfig->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ConfigDialog::RestoreCurrentConfigOnButtonClick ), NULL, this );

}

EditModulePropsDialog::EditModulePropsDialog( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText3 = new wxStaticText( this, wxID_ANY, _("Идентификатор:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3->Wrap( -1 );
	bSizer10->Add( m_staticText3, 0, wxALL, 5 );

	ModuleId = new wxStaticText( this, wxID_MODULE_ID_STAT, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	ModuleId->Wrap( -1 );
	bSizer10->Add( ModuleId, 1, wxALL, 5 );


	bSizer6->Add( bSizer10, 1, wxALIGN_CENTER_HORIZONTAL, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText4 = new wxStaticText( this, wxID_ANY, _("Маршрут:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText4->Wrap( -1 );
	bSizer11->Add( m_staticText4, 0, wxALL, 5 );

	ModulePath = new wxStaticText( this, wxID_MODULE_PATH_STAT, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	ModulePath->Wrap( -1 );
	bSizer11->Add( ModulePath, 0, wxALL, 5 );


	bSizer6->Add( bSizer11, 1, wxALIGN_CENTER_HORIZONTAL, 5 );

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, _("Имя модуля:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer7->Add( m_staticText1, 0, wxALL, 5 );

	ModuleName = new wxTextCtrl( this, wxID_MODULE_NAME_EDIT, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer7->Add( ModuleName, 0, wxALL, 5 );


	bSizer6->Add( bSizer7, 1, wxALIGN_CENTER_HORIZONTAL, 5 );

	ModuleIsActive = new wxCheckBox( this, wxID_MODULE_ACTIVE_FLAG, _("Активен"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( ModuleIsActive, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	ModuleIsMain = new wxCheckBox( this, wxID_MODULE_MAIN_FLAG, _("Главный (стартовый) модуль"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer6->Add( ModuleIsMain, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5 );

	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	SaveNewModuleProps = new wxButton( this, wxID_ANY, _("Принять"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( SaveNewModuleProps, 0, wxALL, 5 );

	CancelEditModuleProps = new wxButton( this, wxID_ANY, _("Отменить"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( CancelEditModuleProps, 0, wxALL, 5 );

	ResetModuleProps = new wxButton( this, wxID_ANY, _("Сбросить"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( ResetModuleProps, 0, wxALL, 5 );

	DeleteModule = new wxButton( this, wxID_ANY, _("Удалить"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer8->Add( DeleteModule, 0, wxALL, 5 );


	bSizer6->Add( bSizer8, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer6 );
	this->Layout();
	bSizer6->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EditModulePropsDialog::EditModulePropsDialogOnClose ) );
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( EditModulePropsDialog::EditModulePropsDialogOnInitDialog ) );
	SaveNewModuleProps->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EditModulePropsDialog::SaveNewModulePropsOnButtonClick ), NULL, this );
	CancelEditModuleProps->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EditModulePropsDialog::CancelEditModulePropsOnButtonClick ), NULL, this );
	ResetModuleProps->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EditModulePropsDialog::ResetModulePropsOnButtonClick ), NULL, this );
	DeleteModule->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EditModulePropsDialog::DeleteModuleOnButtonClick ), NULL, this );
}

EditModulePropsDialog::~EditModulePropsDialog()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EditModulePropsDialog::EditModulePropsDialogOnClose ) );
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( EditModulePropsDialog::EditModulePropsDialogOnInitDialog ) );
	SaveNewModuleProps->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EditModulePropsDialog::SaveNewModulePropsOnButtonClick ), NULL, this );
	CancelEditModuleProps->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EditModulePropsDialog::CancelEditModulePropsOnButtonClick ), NULL, this );
	ResetModuleProps->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EditModulePropsDialog::ResetModulePropsOnButtonClick ), NULL, this );
	DeleteModule->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( EditModulePropsDialog::DeleteModuleOnButtonClick ), NULL, this );

}
