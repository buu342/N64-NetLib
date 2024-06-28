#include "serverbrowser.h"
#include "clientwindow.h"

typedef enum {
    TEVENT_THREADENDED,
} ThreadEventType;

typedef struct
{
    wxString name;
    wxString address;
    int playercount;
    int maxplayers;
    wxString rom;
} FoundServer;

ServerBrowser::ServerBrowser( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    printf("HEllo!\n");
    this->m_MasterAddress = DEFAULT_MASTERSERVER_ADDRESS;
    this->m_MasterPort = DEFAULT_MASTERSERVER_PORT;
    this->m_FinderThread = NULL;

    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    m_MenuBar = new wxMenuBar( 0 );
    m_Menu_File = new wxMenu();
    wxMenuItem* m_MenuItem_File_Quit;
    m_MenuItem_File_Quit = new wxMenuItem( m_Menu_File, wxID_ANY, wxString( wxT("Quit") ) + wxT('\t') + wxT("ALT+F4"), wxEmptyString, wxITEM_NORMAL );
    m_Menu_File->Append( m_MenuItem_File_Quit );

    m_MenuBar->Append( m_Menu_File, wxT("File") );

    this->SetMenuBar( m_MenuBar );

    m_ToolBar = this->CreateToolBar( wxTB_HORIZONTAL, wxID_ANY );
    //m_Tool_Refresh = m_ToolBar->AddTool( wxID_ANY, wxT("Refresh"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

    m_TextCtrl_MasterServerAddress = new wxTextCtrl( m_ToolBar, wxID_ANY,wxString::Format("%s:%d", this->m_MasterAddress, this->m_MasterPort), wxDefaultPosition, wxDefaultSize, 0 );
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
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));

    //this->CreateClient();
    this->ConnectMaster();
}

ServerBrowser::~ServerBrowser()
{
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));
    
    // Deallocate the thread. Use a CriticalSection to access it
    {
        wxCriticalSectionLocker enter(this->m_FinderThreadCS);

        if (this->m_FinderThread != NULL)
            this->m_FinderThread->Delete();
    }
}

void ServerBrowser::CreateClient()
{
    ClientWindow* cw = new ClientWindow(this);
    cw->SetFocus();
    cw->Raise();
    cw->Show();
    this->Lower();
}

void ServerBrowser::ConnectMaster()
{
    // If the thread is running, kill it
    {
        wxCriticalSectionLocker enter(this->m_FinderThreadCS);
        if (this->m_FinderThread != NULL)
        {
            this->m_FinderThread->Delete();
            this->m_FinderThread = NULL;
        }
    }

    // Clear the server list
    this->ClearServers();

    // Create the finder thread
    this->m_FinderThread = new ServerFinderThread(this);
    if (this->m_FinderThread->Run() != wxTHREAD_NO_ERROR)
    {
        delete this->m_FinderThread;
        this->m_FinderThread = NULL;
    }
}

void ServerBrowser::ClearServers()
{
    this->m_DataViewListCtrl_Servers->DeleteAllItems();
}

void ServerBrowser::ThreadEvent(wxThreadEvent& event)
{
    switch ((ThreadEventType)event.GetInt())
    {
        case TEVENT_THREADENDED:
            this->m_FinderThread = NULL;
            break;
    }
}

wxString ServerBrowser::GetAddress()
{
    return this->m_MasterAddress;
}

int ServerBrowser::GetPort()
{
    return this->m_MasterPort;
}


/*=============================================================

=============================================================*/

ServerFinderThread::ServerFinderThread(ServerBrowser* win)
{
    this->m_Window = win;
    this->m_Socket = NULL;
}

ServerFinderThread::~ServerFinderThread()
{
    if (this->m_Socket != NULL)
        delete this->m_Socket;
}

void* ServerFinderThread::Entry()
{
    int packetsize = -1;
    FoundServer serverdata;
    wxIPV4address addr;
    addr.Hostname(this->m_Window->GetAddress());
    addr.Service(this->m_Window->GetPort());

    // Attempt to connect the socket
    this->m_Socket = new wxSocketClient(wxSOCKET_BLOCK);
    this->m_Socket->SetTimeout(10);
    this->m_Socket->Connect(addr);
    if (!this->m_Socket->IsConnected())
    {
        this->m_Socket->Close();
        printf("Socket failed to connect.\n");
        this->NotifyMainOfDeath();
        return NULL;
    }

    printf("Socket connected successfully!\n");
    while (!TestDestroy())
    {
        if (this->m_Socket->IsData())
        {
            int readsize;
            const int readbuffsize = 512;
            char* buf = (char*)malloc(readbuffsize);

            // Perform a blocking read
            this->m_Socket->Read(buf, readbuffsize);
            if (this->m_Socket->LastError() != wxSOCKET_NOERROR)
            {
                printf("Socket threw error %d\n", this->m_Socket->LastError());
                free(buf);
                this->NotifyMainOfDeath();
                return NULL;
            }

            // Echo back the packet
            readsize = this->m_Socket->LastReadCount();
            printf("Reading %d bytes\n", this->m_Socket->LastReadCount());
            for (int i=0; i<readsize; i++)
                printf("%c", buf[i]);
            printf("\n");

            // Parse the packet
            if (packetsize == -1)
            {

            }

            free(buf);
        }
    }
    this->NotifyMainOfDeath();
    return 0;
}

void ServerFinderThread::OnSocketEvent(wxSocketEvent& event)
{
    
}

void ServerFinderThread::AddServer(wxString name, wxString players, wxString address, wxString ROM, wxString ping)
{

}

void ServerFinderThread::NotifyMainOfDeath()
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_THREADENDED);
    wxQueueEvent(this->m_Window, evt.Clone());
}