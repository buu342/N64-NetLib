#include <stdint.h>
#include "serverbrowser.h"
#include "clientwindow.h"

#define MASTERSERVERPACKET_HEADER "N64PKT"

typedef enum {
    TEVENT_ADDSERVER,
    TEVENT_THREADENDED,
} ThreadEventType;

uint32_t swap_endian32(uint32_t val)
{
    return ((val << 24)) | ((val << 8) & 0x00FF0000) | ((val >> 8) & 0x0000FF00) | ((val >> 24));
}

ServerBrowser::ServerBrowser(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
    this->m_MasterAddress = DEFAULT_MASTERSERVER_ADDRESS;
    this->m_MasterPort = DEFAULT_MASTERSERVER_PORT;
    this->m_FinderThread = NULL;

    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    this->m_MenuBar = new wxMenuBar(0);
    this->m_Menu_File = new wxMenu();
    wxMenuItem* m_MenuItem_File_Quit;
    m_MenuItem_File_Quit = new wxMenuItem(m_Menu_File, wxID_ANY, wxString(wxT("Quit")) + wxT('\t') + wxT("ALT+F4"), wxEmptyString, wxITEM_NORMAL);
    this->m_Menu_File->Append(m_MenuItem_File_Quit);

    this->m_MenuBar->Append(m_Menu_File, wxT("File"));

    this->SetMenuBar(m_MenuBar);

    this->m_ToolBar = this->CreateToolBar(wxTB_HORIZONTAL, wxID_ANY);
    this->m_Tool_Refresh = m_ToolBar->AddTool(wxID_ANY, wxT("Refresh"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL);

    this->m_TextCtrl_MasterServerAddress = new wxTextCtrl(this->m_ToolBar, wxID_ANY,wxString::Format("%s:%d", this->m_MasterAddress, this->m_MasterPort), wxDefaultPosition, wxDefaultSize, 0);
    this->m_ToolBar->AddControl(this->m_TextCtrl_MasterServerAddress);
    this->m_ToolBar->AddSeparator();

    this->m_ToolBar->Realize();

    wxGridBagSizer* m_Sizer_Main;
    m_Sizer_Main = new wxGridBagSizer(0, 0);
    m_Sizer_Main->SetFlexibleDirection(wxBOTH);
    m_Sizer_Main->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    this->m_DataViewListCtrl_Servers = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES);
    this->m_DataViewListColumn_Ping = m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Ping"), wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_Players = m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Players"), wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_ServerName = m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Server Name"), wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_Address = m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Address"), wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_ROM = m_DataViewListCtrl_Servers->AppendTextColumn(wxT("ROM"), wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE);
    m_Sizer_Main->Add(m_DataViewListCtrl_Servers, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL|wxEXPAND, 5);

    m_Sizer_Main->AddGrowableCol(0);
    m_Sizer_Main->AddGrowableRow(0);

    this->SetSizer(m_Sizer_Main);
    this->Layout();

    this->Centre(wxBOTH);
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));
    this->Connect(this->m_Tool_Refresh->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(ServerBrowser::m_Tool_RefreshOnToolClicked));

    //this->CreateClient();
    this->ConnectMaster();
}

ServerBrowser::~ServerBrowser()
{
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));
    this->Disconnect(this->m_Tool_Refresh->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(ServerBrowser::m_Tool_RefreshOnToolClicked));
    
    // Deallocate the thread. Use a CriticalSection to access it
    {
        wxCriticalSectionLocker enter(this->m_FinderThreadCS);

        if (this->m_FinderThread != NULL)
            this->m_FinderThread->Delete();
    }
}

void ServerBrowser::m_Tool_RefreshOnToolClicked(wxCommandEvent& event)
{
    this->ClearServers();
    this->ConnectMaster();
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
        case TEVENT_ADDSERVER:
            FoundServer* server = event.GetPayload<FoundServer*>();
            wxVector<wxVariant> data;
            data.push_back(wxVariant("?"));
            data.push_back(wxVariant(wxString::Format("%d/%d", server->playercount, server->maxplayers)));
            data.push_back(wxVariant(server->name));
            data.push_back(wxVariant(server->address));
            data.push_back(wxVariant(server->rom));
            this->m_DataViewListCtrl_Servers->AppendItem(data);
            free(server);
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
    this->NotifyMainOfDeath();
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
    this->m_Socket = new wxSocketClient(wxSOCKET_BLOCK | wxSOCKET_WAITALL);
    this->m_Socket->SetTimeout(10);
    this->m_Socket->Connect(addr);
    if (!this->m_Socket->IsConnected())
    {
        this->m_Socket->Close();
        printf("Socket failed to connect.\n");
        return NULL;
    }

    printf("Connected to master server successfully!\n");
    while (!TestDestroy())
    {
        if (this->m_Socket->IsData())
        {
            int readsize;
            char* buf;
            char headerbuf[6];

            // Read the packet header
            this->m_Socket->Read(headerbuf, 6);
            if (this->m_Socket->LastError() != wxSOCKET_NOERROR)
            {
                printf("Socket threw error %d\n", this->m_Socket->LastError());
                return NULL;
            }

            // Validate the packet header
            if (strncmp(headerbuf, MASTERSERVERPACKET_HEADER, 6) != 0)
            {
                printf("Received bad packet header %.6s\n", headerbuf);
                continue;
            }

            // Read the packet size
            this->m_Socket->Read(&readsize, 4);
            readsize = swap_endian32(readsize);
            if (this->m_Socket->LastError() != wxSOCKET_NOERROR)
            {
                printf("Socket threw error %d\n", this->m_Socket->LastError());
                return NULL;
            }

            // Malloc a buffer for the packet data
            buf = (char*)malloc(readsize);
            if (buf == NULL)
            {
                printf("Unable to allocate buffer of %d bytes for server data\n", readsize);
                continue;
            }

            // Now read the incoming data
            this->m_Socket->Read(buf, readsize);
            if (this->m_Socket->LastError() != wxSOCKET_NOERROR)
            {
                free(buf);
                printf("Socket threw error %d\n", this->m_Socket->LastError());
                return NULL;
            }

            // Parse the packet
            if (!strncmp(buf, "SERVER", 6))
                ParsePacket_Server(buf);

            // Cleanup
            free(buf);
        }
    }
    return NULL;
}

void ServerFinderThread::OnSocketEvent(wxSocketEvent& event)
{
    
}

void ServerFinderThread::ParsePacket_Server(char* buf)
{
    int strsize;
    int buffoffset = 6;
    FoundServer server;

    // Read the server name
    memcpy(&strsize, buf + buffoffset, sizeof(int));
    strsize = swap_endian32(strsize);
    buffoffset += sizeof(int);
    server.name = wxString(buf + buffoffset, (size_t)strsize);
    buffoffset += strsize;

    // Read the player count
    memcpy(&strsize, buf + buffoffset, sizeof(int));
    strsize = swap_endian32(strsize);
    buffoffset += sizeof(int);
    server.playercount = strsize;

    // Read the max players
    memcpy(&strsize, buf + buffoffset, sizeof(int));
    strsize = swap_endian32(strsize);
    buffoffset += sizeof(int);
    server.maxplayers = strsize;

    // Read the server address
    memcpy(&strsize, buf + buffoffset, sizeof(int));
    strsize = swap_endian32(strsize);
    buffoffset += sizeof(int);
    server.address = wxString(buf + buffoffset, (size_t)strsize);
    buffoffset += strsize;

    // Read the rom
    memcpy(&strsize, buf + buffoffset, sizeof(int));
    strsize = swap_endian32(strsize);
    buffoffset += sizeof(int);
    server.rom = wxString(buf + buffoffset, (size_t)strsize);
    buffoffset += strsize;

    // Send the server info to the main thread
    this->AddServer(&server);
}

void ServerFinderThread::AddServer(FoundServer* server)
{
    FoundServer* server_copy = new FoundServer();
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);

    // Copy the server data
    server_copy->name = server->name;
    server_copy->playercount = server->playercount;
    server_copy->maxplayers = server->maxplayers;
    server_copy->address = server->address;
    server_copy->rom = server->rom;

    // Send the event
    evt.SetInt(TEVENT_ADDSERVER);
    evt.SetPayload<FoundServer*>(server_copy);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void ServerFinderThread::NotifyMainOfDeath()
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_THREADENDED);
    wxQueueEvent(this->m_Window, evt.Clone());
}