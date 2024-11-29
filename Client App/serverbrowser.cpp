#include "serverbrowser.h"
#include "clientwindow.h"
#include "romdownloader.h"
#include "packets.h"
#include "helper.h"
#include "sha256.h"
#include <stdint.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <wx/app.h>
#include "Resources/resources.h"

typedef enum {
    TEVENT_ADDSERVER,
    TEVENT_THREADENDED,
} ThreadEventType;

static wxMessageQueue<S64Packet*> global_msgqueue_serverthread;

ServerBrowser::ServerBrowser(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
    wxString addrstr;
    this->m_MasterAddress = DEFAULT_MASTERSERVER_ADDRESS; // TODO: This has to actually be editable
    this->m_MasterPort = DEFAULT_MASTERSERVER_PORT;
    this->m_FinderThread = NULL;
    this->m_Socket = NULL;
    this->m_MasterConnectionHandler = NULL;
    addrstr = wxString::Format("%s:%d", this->m_MasterAddress, this->m_MasterPort);

    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    m_MenuBar = new wxMenuBar( 0 );
    m_Menu_File = new wxMenu();
    wxMenuItem* m_MenuItem_File_Connect;
    m_MenuItem_File_Connect = new wxMenuItem( m_Menu_File, wxID_ANY, wxString( wxT("Manual Connect") ) + wxT('\t') + wxT("ALT+C"), wxEmptyString, wxITEM_NORMAL );
    m_Menu_File->Append( m_MenuItem_File_Connect );

    m_Menu_File->AppendSeparator();

    wxMenuItem* m_MenuItem_File_Quit;
    m_MenuItem_File_Quit = new wxMenuItem( m_Menu_File, wxID_ANY, wxString( wxT("Quit") ) + wxT('\t') + wxT("ALT+F4"), wxEmptyString, wxITEM_NORMAL );
    m_Menu_File->Append( m_MenuItem_File_Quit );

    m_MenuBar->Append( m_Menu_File, wxT("File") );

    this->SetMenuBar( m_MenuBar );

    m_ToolBar = this->CreateToolBar( wxTB_HORIZONTAL, wxID_ANY );
    m_Tool_Refresh = m_ToolBar->AddTool( wxID_ANY, wxT("Refresh"), icon_refresh, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL );

    m_TextCtrl_MasterServerAddress = new wxTextCtrl( m_ToolBar, wxID_ANY, addrstr, wxDefaultPosition, wxDefaultSize, 0 );
    m_ToolBar->AddControl( m_TextCtrl_MasterServerAddress );
    m_ToolBar->AddSeparator();

    m_ToolBar->Realize();

    wxGridBagSizer* m_Sizer_Main;
    m_Sizer_Main = new wxGridBagSizer( 0, 0 );
    m_Sizer_Main->SetFlexibleDirection( wxBOTH );
    m_Sizer_Main->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_DataViewListCtrl_Servers = new wxDataViewListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES );
    m_DataViewListColumn_Ping = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Ping"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE );
    m_DataViewListColumn_Players = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Players"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE );
    m_DataViewListColumn_ServerName = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Server Name"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE );
    m_DataViewListColumn_Address = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Address"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE );
    m_DataViewListColumn_ROM = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("ROM"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE );
    m_DataViewListColumn_Hash = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("Hash"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_HIDDEN );
    m_DataViewListColumn_FileExistsOnMaster = m_DataViewListCtrl_Servers->AppendTextColumn( wxT("File Exists on Master"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_HIDDEN );
    m_Sizer_Main->Add( m_DataViewListCtrl_Servers, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

    m_Sizer_Main->AddGrowableCol( 0 );
    m_Sizer_Main->AddGrowableRow( 0 );

    this->SetSizer( m_Sizer_Main );
    this->Layout();

    this->Centre(wxBOTH);

    // Connect Events
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));
    m_Menu_File->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( ServerBrowser::m_MenuItem_File_Connect_OnMenuSelection ), this, m_MenuItem_File_Connect->GetId());
    m_Menu_File->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( ServerBrowser::m_MenuItem_File_Quit_OnMenuSelection ), this, m_MenuItem_File_Quit->GetId());
    this->Connect( m_Tool_Refresh->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( ServerBrowser::m_Tool_Refresh_OnToolClicked ) );
    m_TextCtrl_MasterServerAddress->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ServerBrowser::m_TextCtrl_MasterServerAddress_OnText ), NULL, this );
    m_DataViewListCtrl_Servers->Connect( wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler( ServerBrowser::m_DataViewListCtrl_Servers_OnDataViewListCtrlItemActivated ), NULL, this );

    // Connect to the master server
    this->ConnectMaster();
}

ServerBrowser::~ServerBrowser()
{
    // Deallocate the thread. Use a CriticalSection to access it
    {
        wxCriticalSectionLocker enter(this->m_FinderThreadCS);

        if (this->m_FinderThread != NULL)
            this->m_FinderThread->Delete();
        this->m_FinderThread = NULL;
    }

    // Close the socket
    if (this->m_Socket != NULL)
        this->m_Socket->Destroy();

    // Disconnect events
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));
    this->Disconnect(this->m_Tool_Refresh->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(ServerBrowser::m_Tool_Refresh_OnToolClicked));
    this->m_DataViewListCtrl_Servers->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(ServerBrowser::m_DataViewListCtrl_Servers_OnDataViewListCtrlItemActivated), NULL, this);
}

void ServerBrowser::m_Tool_Refresh_OnToolClicked(wxCommandEvent& event)
{
    (void)event;
    this->ConnectMaster();
}

void ServerBrowser::m_DataViewListCtrl_Servers_OnDataViewListCtrlItemActivated(wxDataViewEvent& event)
{
    (void)event;
    wxString serveraddr = this->m_DataViewListCtrl_Servers->GetTextValue(this->m_DataViewListCtrl_Servers->GetSelectedRow(), 3);
    wxString romname = this->m_DataViewListCtrl_Servers->GetTextValue(this->m_DataViewListCtrl_Servers->GetSelectedRow(), 4);
    wxString romhash = this->m_DataViewListCtrl_Servers->GetTextValue(this->m_DataViewListCtrl_Servers->GetSelectedRow(), 5);
    wxString rompath = wxString("roms/") + romname;
    bool romavailable = this->m_DataViewListCtrl_Servers->GetTextValue(this->m_DataViewListCtrl_Servers->GetSelectedRow(), 6) == "1";

    if (!wxDirExists("roms"))
        wxDir::Make("roms", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    if (wxFileExists(rompath))
    {
        uint8_t hash[32];
        uint8_t *filedata;
        uint32_t filesize;
        wxString hashstr = "";
        SHA256_CTX ctx;
        FILE* fp = fopen(rompath, "rb");
        if (fp == NULL)
        {
            return;
        }

        // Get the file size and malloc space for the file data
        fseek(fp, 0L, SEEK_END);
        filesize = ftell(fp);
        filedata = (uint8_t*)malloc(filesize);
        if (filedata == NULL)
        {
            fclose(fp);
            return;
        }

        // Read the file data, and then calculate the hash
        fseek(fp, 0L, SEEK_SET);
        fread(filedata, 1, filesize, fp);
        sha256_init(&ctx);
        sha256_update(&ctx, filedata, filesize);
        sha256_final(&ctx, hash);

        // Convert it to a string
        hashstr = "";
        for (uint32_t i=0; i<(sizeof(hash)/sizeof(hash[0])); i++)
            hashstr += wxString::Format("%02X", hash[i]);

        // Cleanup
        fclose(fp);
        free(filedata);

        // Now validate the ROM
        if (hashstr != romhash)
        {
            wxMessageDialog dialog(this, wxString("This server requires the ROM '") + romname + wxString("', which you have in your ROM folder, however your version differs from the one the server needs.\n\nPlease remove or rename the ROM in order to download the correct one."), wxString("Bad ROM found"), wxICON_EXCLAMATION);
            dialog.ShowModal();
        }
        else
            this->CreateClient(rompath, serveraddr);
    }
    else if (romavailable)
    {
        wxMessageDialog dialog(this, wxString("In order to join this server, the ROM '") + romname + wxString("' must be downloaded. This ROM is available to download from the Master Server.\n\nContinue?"), wxString("Download ROM?"), wxICON_QUESTION | wxYES_NO);
        if (dialog.ShowModal() == wxID_YES)
        {
            // Stop the server retrieval thread if it is running
            {
                wxCriticalSectionLocker enter(this->m_FinderThreadCS);

                if (this->m_FinderThread != NULL)
                    this->m_FinderThread->Delete();
                this->m_FinderThread = NULL;
            }

            // Create the ROM download dialog
            ROMDownloadWindow* rdw = new ROMDownloadWindow(this);
            rdw->SetROMPath(rompath);
            rdw->SetROMHash(romhash);
            rdw->SetAddress(this->m_MasterAddress);
            rdw->SetPortNumber(this->m_MasterPort);
            rdw->BeginConnection();
            if (rdw->ShowModal() == 0)
            {
                wxMessageDialog dialog(this, wxString("ROM download was unsuccessful."), wxString("ROM Download Failed"), wxICON_ERROR);
                dialog.ShowModal();
            }
            else
                this->CreateClient(rompath, serveraddr);
        }
    }
    else
    {
        wxMessageDialog dialog(this, wxString("This server requires the ROM '") + romname + wxString("', which is not available for download from the master server.\n\nIn order to join this server, you must manually find and download the ROM, and place it in your ROMs folder."), wxString("ROM Unavailable"), wxICON_ERROR);
        dialog.ShowModal();
    }
}

void ServerBrowser::m_TextCtrl_MasterServerAddress_OnText(wxCommandEvent& event)
{
    wxString str = event.GetString();
    wxStringTokenizer tokenizer(str, ":");
    this->m_MasterAddress = tokenizer.GetNextToken();
    if (tokenizer.HasMoreTokens())
        tokenizer.GetNextToken().ToInt(&this->m_MasterPort);
}

void ServerBrowser::m_MenuItem_File_Connect_OnMenuSelection(wxCommandEvent& event)
{
    (void)event;
    ManualConnectWindow* win = new ManualConnectWindow(this);
    win->ShowModal();
}

void ServerBrowser::m_MenuItem_File_Quit_OnMenuSelection(wxCommandEvent& event)
{
    (void)event;
    this->Destroy();
}

void ServerBrowser::CreateClient(wxString rom, wxString address)
{
    int port;
    wxStringTokenizer tokenizer(address, ":");
    ClientWindow* cw = new ClientWindow(this);

    // Kill the server finder thread so it doesn't steal packets from the client window
    {
        wxCriticalSectionLocker enter(this->m_FinderThreadCS);

        if (this->m_FinderThread != NULL)
            this->m_FinderThread->Delete();
        this->m_FinderThread = NULL;
    }

    // Open a UDP socket if it's not open yet
    if (this->m_Socket == NULL)
    {
        wxIPV4address localaddr;
        localaddr.AnyAddress();
        localaddr.Service(0);
        this->m_Socket = new wxDatagramSocket(localaddr , wxSOCKET_NOWAIT);
    }

    // Initialize the client window
    cw->SetROM(rom);
    cw->SetSocket(this->m_Socket);
    cw->SetAddress(tokenizer.GetNextToken());
    tokenizer.GetNextToken().ToInt(&port);
    this->Lower();
    cw->SetPortNumber(port);
    cw->BeginWorking();
    cw->Raise();
    cw->Show();
}

void ServerBrowser::ConnectMaster()
{
    // Clear the server list
    this->ClearServers();

    // Open a UDP socket if it's not open yet
    if (this->m_Socket == NULL)
    {
        wxIPV4address localaddr;
        localaddr.AnyAddress();
        localaddr.Service(0);
        this->m_Socket = new wxDatagramSocket(localaddr , wxSOCKET_NOWAIT);
    }

    // Create the finder thread if it doesn't exist yet
    if (this->m_FinderThread == NULL)
    {
        this->m_FinderThread = new ServerFinderThread(this);
        if (this->m_FinderThread->Run() != wxTHREAD_NO_ERROR)
        {
            delete this->m_FinderThread;
            this->m_FinderThread = NULL;
        }
    }

    // Send a message to the finder thread that we wanna connect to the master server
    global_msgqueue_serverthread.Post(new S64Packet("LIST", 0, NULL));
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
            data.push_back(wxVariant(wxString::Format("%ld", server->ping)));
            data.push_back(wxVariant(wxString::Format("%d/%d", server->playercount, server->maxplayers)));
            data.push_back(wxVariant(server->name));
            data.push_back(wxVariant(server->fulladdress));
            data.push_back(wxVariant(server->romname));
            data.push_back(wxVariant(server->romhash));
            data.push_back(wxVariant(wxString::Format("%d", server->romdownloadable)));
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

wxDatagramSocket* ServerBrowser::GetSocket()
{
    return this->m_Socket;
}


/*=============================================================

=============================================================*/

ServerFinderThread::ServerFinderThread(ServerBrowser* win)
{
    this->m_Window = win;
}

ServerFinderThread::~ServerFinderThread()
{
    this->NotifyMainOfDeath();
}

void* ServerFinderThread::Entry()
{
    std::unordered_map<wxString, std::pair<FoundServer, wxLongLong>> serversleft; 
    uint8_t* buff = (uint8_t*)malloc(4096);
    wxDatagramSocket* sock = this->m_Window->GetSocket();
    UDPHandler* handler = new UDPHandler(sock, this->m_Window->GetAddress(), this->m_Window->GetPort());
    while (!TestDestroy())
    {
        S64Packet* pkt = NULL;

        // Check for messages from the main thread
        while (global_msgqueue_serverthread.ReceiveTimeout(0, pkt) == wxMSGQUEUE_NO_ERROR)
        {
            if (pkt->GetType() == "LIST")
                handler->SendPacket(pkt);
            else
                printf("Client sent packet of unknown type\n");
        }

        // Check for packets from the master server / servers we pinged
        sock->Read(buff, 4096);
        while (sock->LastReadCount() > 0)
        {
            pkt = handler->ReadS64Packet(buff);
            if (pkt != NULL)
            {
                if (!strncmp(pkt->GetType(), "SERVER", 6))
                {
                    FoundServer server = this->ParsePacket_Server(handler->GetSocket(), pkt);
                    serversleft[server.fulladdress] = std::make_pair(server, wxGetLocalTimeMillis());
                }
                else if (!strncmp(pkt->GetType(), "DONELISTING", 11))
                    printf("Master server finished sending server list\n");
                else if (!strncmp(pkt->GetType(), "DISCOVER", 8))
                    this->DiscoveredServer(&serversleft, pkt);
                else
                    printf("Unexpected packet type received\n");
            }
            sock->Read(buff, 4096);
        }
        wxMilliSleep(10);
    }
    free(buff);
    return NULL;
}

FoundServer ServerFinderThread::ParsePacket_Server(wxDatagramSocket* socket, S64Packet* pkt)
{
    uint8_t hash[32];
    uint32_t read32;
    uint8_t* buf = pkt->GetData();
    uint32_t buffoffset = 0;
    S64Packet* ping;
    FoundServer server;
    wxCharBuffer serveraddr;

    // Read the full server address
    memcpy(&read32, buf + buffoffset, sizeof(uint32_t));
    read32 = swap_endian32(read32);
    buffoffset += sizeof(uint32_t);
    server.fulladdress = wxString(buf + buffoffset, (size_t)read32);
    buffoffset += read32;

    // Read the ROM name
    memcpy(&read32, buf + buffoffset, sizeof(uint32_t));
    read32 = swap_endian32(read32);
    buffoffset += sizeof(uint32_t);
    server.romname = wxString(buf + buffoffset, (size_t)read32);
    buffoffset += read32;

    // Read the rom hash
    memcpy(&read32, buf + buffoffset, sizeof(uint32_t));
    read32 = swap_endian32(read32);
    buffoffset += sizeof(uint32_t);
    memcpy(hash, buf + buffoffset, read32);
    buffoffset += read32;

    // Convert the hash
    server.romhash = "";
    for (uint32_t i=0; i<read32; i++)
        server.romhash += wxString::Format("%02X", hash[i]);

    // Read if the ROM is on the master
    memcpy(hash, buf + buffoffset, sizeof(char));
    buffoffset += sizeof(char);
    server.romdownloadable = hash[0];

    // Create the UDP handler and send the ping packet
    server.handler = new UDPHandler(socket, server.fulladdress);
    serveraddr = server.fulladdress.ToUTF8();
    server.handler->SendPacket(new S64Packet("DISCOVER", serveraddr.length(), (uint8_t*)serveraddr.data()));

    // Done
    return server;
}

void ServerFinderThread::DiscoveredServer(std::unordered_map<wxString, std::pair<FoundServer, wxLongLong>>* serverlist, S64Packet* pkt)
{
    uint32_t read32;
    wxString fulladdress;
    uint32_t buffoffset = 0;
    uint8_t* buf = pkt->GetData();

    // Read the full server address
    memcpy(&read32, buf + buffoffset, sizeof(uint32_t));
    read32 = swap_endian32(read32);
    buffoffset += sizeof(uint32_t);
    fulladdress = wxString(buf + buffoffset, (size_t)read32);
    buffoffset += read32;

    // Ensure the server that pinged us back is actually in the server list
    if (serverlist->find(fulladdress) != serverlist->end())
    {
        std::pair<FoundServer, wxLongLong> found = (*serverlist)[fulladdress];
        FoundServer* server_copy = new FoundServer();
        wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
        FoundServer* server = &found.first;

        // Read the server name
        memcpy(&read32, buf + buffoffset, sizeof(uint32_t));
        read32 = swap_endian32(read32);
        buffoffset += sizeof(uint32_t);
        server->name = wxString(buf + buffoffset, (size_t)read32);
        buffoffset += read32;

        // Read if the player count and max player count
        memcpy(&read32, buf + buffoffset, sizeof(uint32_t));
        read32 = swap_endian32(read32);
        buffoffset += sizeof(uint32_t);
        server->playercount = read32;
        memcpy(&read32, buf + buffoffset, sizeof(uint32_t));
        read32 = swap_endian32(read32);
        buffoffset += sizeof(uint32_t);
        server->maxplayers = read32;

        // Calculate the ping
        server->ping = wxGetLocalTimeMillis() - found.second;

        // Copy the server data
        server_copy->fulladdress = server->fulladdress;
        server_copy->name = server->name;
        server_copy->playercount = server->playercount;
        server_copy->maxplayers = server->maxplayers;
        server_copy->ping = server->ping;
        server_copy->romname = server->romname;
        server_copy->romhash = server->romhash;
        server_copy->romdownloadable = server->romdownloadable;

        // Cleanup
        delete server->handler;
        serverlist->erase(fulladdress);

        // Send the event to the main thread
        evt.SetInt(TEVENT_ADDSERVER);
        evt.SetPayload<FoundServer*>(server_copy);
        wxQueueEvent(this->m_Window, evt.Clone());
    }
}

void ServerFinderThread::NotifyMainOfDeath()
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_THREADENDED);
    wxQueueEvent(this->m_Window, evt.Clone());
}



/*=============================================================

=============================================================*/

ManualConnectWindow::ManualConnectWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxFlexGridSizer* m_Sizer_Main;
    m_Sizer_Main = new wxFlexGridSizer( 2, 1, 0, 0 );
    m_Sizer_Main->AddGrowableCol( 0 );
    m_Sizer_Main->AddGrowableRow( 0 );
    m_Sizer_Main->SetFlexibleDirection( wxBOTH );
    m_Sizer_Main->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    wxFlexGridSizer* m_Sizer_Inputs;
    m_Sizer_Inputs = new wxFlexGridSizer( 0, 2, 0, 0 );
    m_Sizer_Inputs->AddGrowableCol( 1 );
    m_Sizer_Inputs->AddGrowableRow( 1 );
    m_Sizer_Inputs->SetFlexibleDirection( wxBOTH );
    m_Sizer_Inputs->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    wxStaticText* m_StaticText_Server;
    m_StaticText_Server = new wxStaticText( this, wxID_ANY, wxT("Server IP:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_StaticText_Server->Wrap( -1 );
    m_Sizer_Inputs->Add( m_StaticText_Server, 0, wxALL, 5 );

    m_TextCtrl_Server = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_Sizer_Inputs->Add( m_TextCtrl_Server, 0, wxALL|wxEXPAND, 5 );

    wxStaticText* m_StaticText_ROM;
    m_StaticText_ROM = new wxStaticText( this, wxID_ANY, wxT("ROM Path:"), wxDefaultPosition, wxDefaultSize, 0 );
    m_StaticText_ROM->Wrap( -1 );
    m_Sizer_Inputs->Add( m_StaticText_ROM, 0, wxALIGN_CENTER|wxALL, 5 );

    m_FilePicker_ROM = new wxFilePickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Load ROM"), wxT("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE|wxFLP_FILE_MUST_EXIST|wxFLP_OPEN|wxFLP_SMALL|wxFLP_USE_TEXTCTRL );
    m_FilePicker_ROM->DragAcceptFiles( true );

    m_Sizer_Inputs->Add( m_FilePicker_ROM, 0, wxALL|wxEXPAND, 5 );

    m_Sizer_Main->Add( m_Sizer_Inputs, 1, wxEXPAND, 5 );

    m_Button_Connect = new wxButton( this, wxID_ANY, wxT("Connect"), wxDefaultPosition, wxDefaultSize, 0 );
    m_Sizer_Main->Add( m_Button_Connect, 0, wxALIGN_CENTER|wxALL, 5 );

    this->SetSizer( m_Sizer_Main );
    this->Layout();

    this->Centre( wxBOTH );

    // Connect Events
    m_Button_Connect->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualConnectWindow::m_Button_Connect_OnButtonClick ), NULL, this );
}

ManualConnectWindow::~ManualConnectWindow()
{
    // Disconnect Events
    m_Button_Connect->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualConnectWindow::m_Button_Connect_OnButtonClick ), NULL, this );
}

void ManualConnectWindow::m_Button_Connect_OnButtonClick(wxCommandEvent& event)
{
    (void)event;
    wxString path = this->m_FilePicker_ROM->GetPath();
    wxString address = this->m_TextCtrl_Server->GetValue();
    ServerBrowser* parent = (ServerBrowser*)this->GetParent();
    this->Destroy(); // Must be done first or the parent window will steal attention from the client
    parent->CreateClient(path, address);
}