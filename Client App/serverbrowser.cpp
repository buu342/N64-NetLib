#include "serverbrowser.h"

ServerBrowser::ServerBrowser( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    m_MenuBar = new wxMenuBar( 0 );
    m_Menu_File = new wxMenu();
    wxMenuItem* m_MenuItem_File_Quit;
    m_MenuItem_File_Quit = new wxMenuItem( m_Menu_File, wxID_ANY, wxString( wxT("Quit") ) + wxT('\t') + wxT("ALT+F4"), wxEmptyString, wxITEM_NORMAL );
    m_Menu_File->Append( m_MenuItem_File_Quit );

    m_MenuBar->Append( m_Menu_File, wxT("File") );

    this->SetMenuBar( m_MenuBar );

    m_ToolBar = this->CreateToolBar( wxTB_HORIZONTAL, wxID_ANY );
    m_Tool_Refresh = m_ToolBar->AddTool( wxID_ANY, wxT("Refresh"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

    m_TextCtrl_MasterServerAddress = new wxTextCtrl( m_ToolBar, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_ToolBar->AddControl( m_TextCtrl_MasterServerAddress );
    m_ToolBar->AddSeparator();

    m_ToolBar->Realize();

    wxGridBagSizer* m_Sizer_Main;
    m_Sizer_Main = new wxGridBagSizer( 0, 0 );
    m_Sizer_Main->SetFlexibleDirection( wxBOTH );
    m_Sizer_Main->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_DataViewListCtrl_Servers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES );
    m_DataViewListColumn_Ping = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Ping"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
    m_DataViewListColumn_Players = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Players"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
    m_DataViewListColumn_ServerName = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Server Name"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
    m_DataViewListColumn_Address = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Address"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
    m_DataViewListColumn_ROM = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("ROM"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE );
    m_Sizer_Main->Add( m_DataViewListCtrl_Servers, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );


    m_Sizer_Main->AddGrowableCol( 0 );
    m_Sizer_Main->AddGrowableRow( 0 );

    this->SetSizer( m_Sizer_Main );
    this->Layout();

    this->Centre( wxBOTH );
}

ServerBrowser::~ServerBrowser()
{
    
}