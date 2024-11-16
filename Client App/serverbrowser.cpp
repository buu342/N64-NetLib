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
#include <wx/msgqueue.h>
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
    //this->ConnectMaster();
}

ServerBrowser::~ServerBrowser()
{
    // Deallocate the thread. Use a CriticalSection to access it
    {
        wxCriticalSectionLocker enter(this->m_FinderThreadCS);

        if (this->m_FinderThread != NULL)
            this->m_FinderThread->Delete();
    }

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
    else
    {
        if (romavailable)
        {
            wxMessageDialog dialog(this, wxString("In order to join this server, the ROM '") + romname + wxString("' must be downloaded. This ROM is available to download from the Master Server.\n\nContinue?"), wxString("Download ROM?"), wxICON_QUESTION | wxYES_NO);
            if (dialog.ShowModal() == wxID_YES)
            {
                // Stop the server retrieval thread if it is running
                {
                    wxCriticalSectionLocker enter(this->m_FinderThreadCS);

                    if (this->m_FinderThread != NULL)
                        this->m_FinderThread->Delete();
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
}

void ServerBrowser::m_TextCtrl_MasterServerAddress_OnText(wxCommandEvent& event)
{
    int port;
    wxString str = event.GetString();
    wxStringTokenizer tokenizer(str, ":");
    this->m_MasterAddress = tokenizer.GetNextToken();
    if (tokenizer.HasMoreTokens())
    {
        tokenizer.GetNextToken().ToInt(&port);
        this->m_MasterPort = port;
    }
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
    cw->SetROM(rom);
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

    // Open a UDP socket
    if (this->m_Socket == NULL)
    {
        wxIPV4address localaddr;
        localaddr.AnyAddress();
        localaddr.Service(0);
        this->m_Socket = new wxDatagramSocket(localaddr , wxSOCKET_NOWAIT);
    }

    // Create the finder thread
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
            data.push_back(wxVariant("?"));
            data.push_back(wxVariant(wxString::Format("%d/%d", server->playercount, server->maxplayers)));
            data.push_back(wxVariant(server->name));
            data.push_back(wxVariant(server->address));
            data.push_back(wxVariant(server->rom));
            data.push_back(wxVariant(server->hash));
            data.push_back(wxVariant(wxString::Format("%d", server->romonmaster)));
            // TODO: Poke the server to get its ping and player count
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
    //m_TextCtrl_Server->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ManualConnectWindow::m_TextCtrl_Server_OnText ), NULL, this );
    //m_FilePicker_ROM->Connect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( ManualConnectWindow::m_FilePicker_ROM_OnFileChanged ), NULL, this );
    m_Button_Connect->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualConnectWindow::m_Button_Connect_OnButtonClick ), NULL, this );
}

ManualConnectWindow::~ManualConnectWindow()
{
    // Disconnect Events
    //m_TextCtrl_Server->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( ManualConnectWindow::m_TextCtrl_Server_OnText ), NULL, this );
    //m_FilePicker_ROM->Disconnect( wxEVT_COMMAND_FILEPICKER_CHANGED, wxFileDirPickerEventHandler( ManualConnectWindow::m_FilePicker_ROM_OnFileChanged ), NULL, this );
    m_Button_Connect->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ManualConnectWindow::m_Button_Connect_OnButtonClick ), NULL, this );
}

/*void ManualConnectWindow::m_TextCtrl_Server_OnText(wxCommandEvent& event)
{

}

void ManualConnectWindow::m_FilePicker_ROM_OnFileChanged(wxFileDirPickerEvent& event)
{

}*/

void ManualConnectWindow::m_Button_Connect_OnButtonClick(wxCommandEvent& event)
{
    (void)event;
    wxString path = this->m_FilePicker_ROM->GetPath();
    wxString address = this->m_TextCtrl_Server->GetValue();
    ServerBrowser* parent = (ServerBrowser*)this->GetParent();
    this->Destroy(); // Must be done first or the parent window will steal attention from the client
    parent->CreateClient(path, address);
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
    while (!TestDestroy())
    {
        S64Packet* pkt;

        // Check for messages from the main thread
        global_msgqueue_serverthread.ReceiveTimeout(0, pkt);
        while (pkt != NULL)
        {
            wxIPV4address addr;
            addr.Hostname(this->m_Window->GetAddress());
            addr.Service(this->m_Window->GetPort());
            pkt->SendPacket(this->m_Window->GetSocket(), addr);

            pkt = NULL;
            global_msgqueue_serverthread.ReceiveTimeout(0, pkt);
        }
    }
    return NULL;
    /*
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
        printf("Master Server socket failed to connect.\n");
        return NULL;
    }

    printf("Connected to master server successfully!\n");
    pkt = new S64Packet("LIST", 0, NULL);
    pkt->SendPacket(this->m_Socket);
    printf("Requested server list\n");
    while (!TestDestroy())
    {
        if (this->m_Socket->IsData())
        {
            pkt = S64Packet::ReadPacket(this->m_Socket);

            // Parse the packet
            if (pkt != NULL)
            {
                if (!strncmp(pkt->GetType(), "SERVER", 6))
                    ParsePacket_Server(pkt->GetData());
                else
                    printf("Unexpected packet type received\n");
                delete pkt;
                continue;
            }
            else if (this->m_Socket->LastCount() == 0)
            {
                wxMilliSleep(100);
            }
        }
    }
    */
}

void ServerFinderThread::OnSocketEvent(wxSocketEvent& event)
{
    (void)event;
}

void ServerFinderThread::ParsePacket_Server(char* buf)
{
    uint32_t i;
    uint32_t strsize;
    int buffoffset = 0;
    uint8_t hash[32];
    FoundServer server;

    // Read the server name
    memcpy(&strsize, buf + buffoffset, sizeof(int));
    strsize = swap_endian32(strsize);
    buffoffset += sizeof(int);
    server.name = wxString(buf + buffoffset, (size_t)strsize);
    buffoffset += strsize;

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

    // Read the rom hash
    memcpy(&strsize, buf + buffoffset, sizeof(int));
    strsize = swap_endian32(strsize);
    buffoffset += sizeof(int);
    memcpy(hash, buf + buffoffset, (size_t)strsize);
    buffoffset += strsize;

    // Convert the hash
    server.hash = "";
    for (i=0; i<strsize; i++)
        server.hash += wxString::Format("%02X", hash[i]);

    // Read if the ROM is on the master
    memcpy(hash, buf + buffoffset, sizeof(char));
    buffoffset += sizeof(char);
    server.romonmaster = hash[0];

    // Read the playercount
    memcpy(hash, buf + buffoffset, sizeof(char));
    buffoffset += sizeof(char);
    server.playercount = hash[0];

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
    server_copy->hash = server->hash;
    server_copy->romonmaster = server->romonmaster;

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