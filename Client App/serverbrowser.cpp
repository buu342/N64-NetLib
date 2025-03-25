/***************************************************************
                       serverbrowser.cpp

The server browser window
***************************************************************/

#include "serverbrowser.h"
#include "clientwindow.h"
#include "packets.h"
#include "helper.h"
#include "sha256.h"
#include <stdint.h>
#include <wx/config.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>
#include <wx/tokenzr.h>
#include <wx/app.h>
#include "Resources/resources.h"


/******************************
             Types
******************************/

typedef enum {
    TEVENT_ADDSERVER,

    // User Input events
    TEVENT_DOLIST,
    TEVENT_DODL,
    TEVENT_CANCELDL,
    TEVENT_FILENAME,
} ThreadEventType;

typedef enum {
    COLUMN_PING = 0,
    COLUMN_PLAYERCOUNT = 1,
    COLUMN_ADDRESS = 2,
    COLUMN_ROM = 3,
    COLUMN_SERVERNAME = 4,
    COLUMN_ROMHASH = 5,
    COLUMN_ROMONMASTER = 6
} ServerListColumn;

typedef struct {
    ThreadEventType type;
    char* data;
} InputMessage;


/******************************
            Globals
******************************/

static wxMessageQueue<InputMessage*> global_msgqueue_serverthread_input;


/*=============================================================
                       Helper Functions
=============================================================*/

/*==============================
    address_fromstr
    Takes an address:port string and splits it into its components
    @param The address:port string
    @param A pointer to the wxString to fill with the address
    @param A pointer to an int to fill with the port
==============================*/

void address_fromstr(wxString str, wxString* addr, int* port)
{
    wxStringTokenizer tokenizer(str, ":");
    *addr = tokenizer.GetNextToken();
    if (tokenizer.HasMoreTokens())
        tokenizer.GetNextToken().ToInt(port);
}


/*==============================
    get_sanitizedromname
    Cleans up markup text from a ROM name
    @param  The ROM name to cleanup
    @return The sanitised ROM name
==============================*/

wxString get_sanitizedromname(wxString romname)
{
    if (romname.Right(wxString("</span>").Length()) == "</span>")
    {
        romname.Replace("</span>", "");
        return romname.AfterFirst('>');
    }
    else
        return romname;
}


/*==============================
    get_rompath
    Appends the ROM folder path to a given ROM name
    @param  The ROM name
    @return The full path to that ROM in the ROMs folder
==============================*/

wxString get_rompath(wxString romname)
{
    const wxString romfolder = wxString("roms");
    if (!wxDirExists(romfolder))
        wxDir::Make(romfolder, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    return romfolder + wxFileName::GetPathSeparator() + romname;
}


/*==============================
    is_samerom
    Checks if a given ROM matches a given hash string
    @param  The ROM name to fetch from the ROMs folder
    @param  The hash string to compare it to
    @return Whether the given ROM matches the hash
==============================*/

bool is_samerom(wxString romname, wxString romhash)
{
    uint8_t hash[32];
    uint8_t *filedata;
    uint32_t filesize;
    wxString rompath = get_rompath(romname);
    wxString hashstr = "";
    SHA256_CTX ctx;
    FILE* fp;
    if (!wxFileExists(rompath))
        return false;

    fp = fopen(rompath, "rb");
    if (fp == NULL)
        return false;

    // Get the file size and malloc space for the file data
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    filedata = (uint8_t*)malloc(filesize);
    if (filedata == NULL)
    {
        fclose(fp);
        return false;
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
    return hashstr == romhash;
}


/*=============================================================
                        Server Browser
=============================================================*/

/*==============================
    ServerBrowser (Constructor)
    Initializes the class
    @param The parent window
    @param The window ID to use
    @param The title of the window
    @param The position of the window on the screen
    @param The size of the window
    @param Style flags
==============================*/

ServerBrowser::ServerBrowser(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
    wxString maddr;
    this->m_FinderThread = NULL;
    this->m_DownloadWindow = NULL;

    // Initialize the master address from the config file
    maddr = wxConfigBase::Get()->Read("MasterAddress", wxString::Format("%s:%d", DEFAULT_MASTERSERVER_ADDRESS, DEFAULT_MASTERSERVER_PORT));
    address_fromstr(maddr, &this->m_MasterAddress, &this->m_MasterPort);

    // Begin the window
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    // Initialize menu bar
    this->m_MenuBar = new wxMenuBar(0);
    this->m_Menu_File = new wxMenu();
    wxMenuItem* m_MenuItem_File_Connect;

    // Add the File->Manual Connect option
    m_MenuItem_File_Connect = new wxMenuItem(this->m_Menu_File, wxID_ANY, wxString(wxT("Manual Connect")) + wxT('\t') + wxT("ALT+C"), wxEmptyString, wxITEM_NORMAL);
    this->m_Menu_File->Append(m_MenuItem_File_Connect);
    this->m_Menu_File->AppendSeparator();

    // Add the File->Quit option
    wxMenuItem* m_MenuItem_File_Quit;
    m_MenuItem_File_Quit = new wxMenuItem(this->m_Menu_File, wxID_ANY, wxString(wxT("Quit")) + wxT('\t') + wxT("ALT+F4"), wxEmptyString, wxITEM_NORMAL);
    this->m_Menu_File->Append(m_MenuItem_File_Quit);
    this->m_MenuBar->Append(this->m_Menu_File, wxT("File"));

    // Finish the menu bar
    this->SetMenuBar(this->m_MenuBar);

    // Create the toolbar
    this->m_ToolBar = this->CreateToolBar(wxTB_HORIZONTAL, wxID_ANY);
    this->m_ToolBar->SetToolPacking(10);

    // Add the refresh button
    this->m_Tool_Refresh = this->m_ToolBar->AddTool(wxID_ANY, wxT("Refresh"), icon_refresh, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL);

    // Add the master address input bar
    this->m_TextCtrl_MasterServerAddress = new wxTextCtrl(this->m_ToolBar, wxID_ANY, maddr, wxDefaultPosition, wxSize(200, -1), 0);
    this->m_ToolBar->AddControl(this->m_TextCtrl_MasterServerAddress);
    this->m_ToolBar->AddSeparator();
    this->m_ToolBar->Realize();

    // Add the main sizer
    wxGridBagSizer* m_Sizer_Main;
    m_Sizer_Main = new wxGridBagSizer(0, 0);
    m_Sizer_Main->SetFlexibleDirection(wxBOTH);
    m_Sizer_Main->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Create the server list
    this->m_DataViewListCtrl_Servers = new CustomDataView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_ROW_LINES);
    this->m_DataViewListColumn_Ping = this->m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Ping"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_Players = this->m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Players"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_Address = this->m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Address"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_ROM = this->m_DataViewListCtrl_Servers->AppendCustomTextColumn(wxT("ROM"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_ServerName = this->m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Server Name"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_RESIZABLE|wxDATAVIEW_COL_SORTABLE);
    this->m_DataViewListColumn_Hash = this->m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Hash"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_HIDDEN);
    this->m_DataViewListColumn_FileExistsOnMaster = this->m_DataViewListCtrl_Servers->AppendTextColumn(wxT("File Exists on Master"), wxDATAVIEW_CELL_INERT, -1, static_cast<wxAlignment>(wxALIGN_LEFT), wxDATAVIEW_COL_HIDDEN);
    m_Sizer_Main->Add(this->m_DataViewListCtrl_Servers, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL|wxEXPAND, 5);

    // Finalize the layout
    m_Sizer_Main->AddGrowableCol(0);
    m_Sizer_Main->AddGrowableRow(0);
    this->SetSizer(m_Sizer_Main);
    this->Layout();
    this->Centre(wxBOTH);

    // Connect Events
    this->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(ServerBrowser::m_Event_OnClose));
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));
    this->m_Menu_File->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ServerBrowser::m_MenuItem_File_Connect_OnMenuSelection), this, m_MenuItem_File_Connect->GetId());
    this->m_Menu_File->Bind(wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(ServerBrowser::m_MenuItem_File_Quit_OnMenuSelection), this, m_MenuItem_File_Quit->GetId());
    this->Connect(this->m_Tool_Refresh->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(ServerBrowser::m_Tool_Refresh_OnToolClicked));
    this->m_TextCtrl_MasterServerAddress->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(ServerBrowser::m_TextCtrl_MasterServerAddress_OnText), NULL, this);
    this->m_DataViewListCtrl_Servers->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(ServerBrowser::m_DataViewListCtrl_Servers_OnDataViewListCtrlItemActivated), NULL, this);
    this->m_DataViewListCtrl_Servers->Connect(wxEVT_MOTION, wxMouseEventHandler(ServerBrowser::m_DataViewListCtrl_Servers_OnMotion), NULL, this);

    // Connect to the master server
    this->ConnectMaster();
}


/*==============================
    ServerBrowser (Destructor)
    Cleans up the class before deletion
==============================*/

ServerBrowser::~ServerBrowser()
{
    // Kill the server finder thread
    this->StopThread_Finder();

    // Disconnect events
    this->Disconnect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(ServerBrowser::m_Event_OnClose));
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));
    this->Disconnect(this->m_Tool_Refresh->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(ServerBrowser::m_Tool_Refresh_OnToolClicked));
    this->m_DataViewListCtrl_Servers->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(ServerBrowser::m_DataViewListCtrl_Servers_OnDataViewListCtrlItemActivated), NULL, this);
}


/*==============================
    ServerBrowser::StartThread_Server
    Starts the server finder thread
==============================*/

void ServerBrowser::StartThread_Finder()
{
    if (this->m_FinderThread == NULL)
    {
        this->m_FinderThread = new ServerFinderThread(this);
        if (this->m_FinderThread->Run() != wxTHREAD_NO_ERROR)
        {
            delete this->m_FinderThread;
            this->m_FinderThread = NULL;
        }
    }
}


/*==============================
    ServerBrowser::StopThread_Finder
    Stops the server finder thread
==============================*/

void ServerBrowser::StopThread_Finder()
{
    if (this->m_FinderThread != NULL)
    {
        this->m_FinderThread->Delete();
        this->m_FinderThread->Wait(wxTHREAD_WAIT_BLOCK);
        delete this->m_FinderThread;
        this->m_FinderThread = NULL;
    }
}


/*==============================
    ServerBrowser::m_Event_OnClose
    Event handler for window closing
    @param The command event
==============================*/

void ServerBrowser::m_Event_OnClose(wxCloseEvent& event)
{
    event.Skip();
}


/*==============================
    ServerBrowser::m_Button_Send_OnButtonClick
    Event handler for refresh button clicking
    @param The command event
==============================*/

void ServerBrowser::m_Tool_Refresh_OnToolClicked(wxCommandEvent& event)
{
    (void)event;
    this->ConnectMaster();
}


/*==============================
    ServerBrowser::m_Button_Send_OnButtonClick
    Event handler for refresh button clicking
    @param The command event
==============================*/

void ServerBrowser::m_DataViewListCtrl_Servers_OnDataViewListCtrlItemActivated(wxDataViewEvent& event)
{
    wxString serveraddr = this->m_DataViewListCtrl_Servers->GetTextValue(this->m_DataViewListCtrl_Servers->GetSelectedRow(), COLUMN_ADDRESS);
    wxString romname = get_sanitizedromname(this->m_DataViewListCtrl_Servers->GetTextValue(this->m_DataViewListCtrl_Servers->GetSelectedRow(), COLUMN_ROM));
    wxString romhash = this->m_DataViewListCtrl_Servers->GetTextValue(this->m_DataViewListCtrl_Servers->GetSelectedRow(), COLUMN_ROMHASH);
    wxString rompath = get_rompath(romname);
    bool romavailable = this->m_DataViewListCtrl_Servers->GetTextValue(this->m_DataViewListCtrl_Servers->GetSelectedRow(), COLUMN_ROMONMASTER) == "1";

    // Check the ROM exists in our ROMs folder
    if (wxFileExists(rompath))
    {
        // It does, so validate the ROM by comparing hashes
        if (!is_samerom(romname, romhash))
        {
            wxMessageDialog dialog(this, wxString("This server requires the ROM '") + romname + wxString("', which you have in your ROM folder, however your version differs from the one the server needs.\n\nPlease remove or rename the ROM in order to download the correct one."), wxString("Bad ROM found"), wxICON_EXCLAMATION);
            dialog.ShowModal();
        }
        else
            this->CreateClient(rompath, serveraddr);
    }
    else if (romavailable) // If the ROM is available for download from the Master Server
    {
        wxMessageDialog dialog(this, wxString("In order to join this server, the ROM '") + romname + wxString("' must be downloaded. This ROM is available to download from the Master Server.\n\nContinue?"), wxString("Download ROM?"), wxICON_QUESTION | wxYES_NO);
        if (dialog.ShowModal() == wxID_YES)
        {
            // Create the ROM download dialog    
            this->m_DownloadWindow = new ROMDownloadWindow(this);
            this->RequestDownload(romhash, rompath);
            if (this->m_DownloadWindow->ShowModal() != 1)
            {
                wxMessageDialog dialog(this, wxString("ROM download was unsuccessful."), wxString("ROM Download Failed"), wxICON_ERROR);
                dialog.ShowModal();
                global_msgqueue_serverthread_input.Post(new InputMessage{TEVENT_CANCELDL, NULL}); // Notify the server thread in case it was mid-download
            }
            else
                this->CreateClient(rompath, serveraddr);
        }
    }
    else // ROM is not available from the master server
    {
        wxMessageDialog dialog(this, wxString("This server requires the ROM '") + romname + wxString("', which is not available for download from the master server.\n\nIn order to join this server, you must manually find and download the ROM, and place it in your ROMs folder."), wxString("ROM Unavailable"), wxICON_ERROR);
        dialog.ShowModal();
    }

    // Unused parameter
    (void)event;
}


/*==============================
    ServerBrowser::m_TextCtrl_MasterServerAddress_OnText
    Event handler for changing the master server input box
    @param The command event
==============================*/

void ServerBrowser::m_TextCtrl_MasterServerAddress_OnText(wxCommandEvent& event)
{
    wxString str = event.GetString();
    address_fromstr(str, &this->m_MasterAddress, &this->m_MasterPort);
    wxConfigBase::Get()->Write("MasterAddress", str);
    wxConfigBase::Get()->Flush();
}


/*==============================
    ServerBrowser::m_DataViewListCtrl_Servers_OnMotion
    Event handler for moving the mouse in the server list
    @param The command event
==============================*/

void ServerBrowser::m_DataViewListCtrl_Servers_OnMotion(wxMouseEvent& event)
{
    int row;
    wxDataViewItem item;
    wxDataViewColumn* col;
    static int lastrow = -1;

    // Check where the mouse is colliding
    this->m_DataViewListCtrl_Servers->HitTest(this->m_DataViewListCtrl_Servers->ScreenToClient(wxGetMousePosition()), item, col);
    row = this->m_DataViewListCtrl_Servers->ItemToRow(item);

    // If we hit a valid item in the third column (the ROMs column)
    if (item != NULL && col->GetModelColumn() == COLUMN_ROM)
    {
        if (lastrow != row) // Only do this when the mouse changes row
        {
            wxString romname = get_sanitizedromname(this->m_DataViewListCtrl_Servers->GetTextValue(row, COLUMN_ROM));
            wxString romhash = this->m_DataViewListCtrl_Servers->GetTextValue(row, COLUMN_ROMHASH);
            wxString rompath = get_rompath(romname);
            bool romavailable = this->m_DataViewListCtrl_Servers->GetTextValue(row, COLUMN_ROMONMASTER) == "1";

            // Set the tooltip based on the state of the ROM
            if (wxFileExists(rompath))
            {
                if (is_samerom(romname, romhash))
                    this->m_DataViewListCtrl_Servers->SetToolTip("You have this ROM!\n"+romhash);
                else
                    this->m_DataViewListCtrl_Servers->SetToolTip("Your ROM differs from the server.\n" + romhash);
            }
            else if (!romavailable)
                this->m_DataViewListCtrl_Servers->SetToolTip("This ROM is not available for download.\n" + romhash);
            else
                this->m_DataViewListCtrl_Servers->SetToolTip("This ROM is available for download from the master server.\n" + romhash);
            
        }
    }
    else if (lastrow != row)
        this->m_DataViewListCtrl_Servers->SetToolTip("");

    // Update the lastrow for next time this function is called
    lastrow = this->m_DataViewListCtrl_Servers->ItemToRow(item);

    // Unused parameter
    (void)event;
}


/*==============================
    ServerBrowser::m_MenuItem_File_Connect_OnMenuSelection
    Event handler for Manual Connect menu item selection
    @param The command event
==============================*/

void ServerBrowser::m_MenuItem_File_Connect_OnMenuSelection(wxCommandEvent& event)
{
    (void)event;
    ManualConnectWindow* win = new ManualConnectWindow(this);
    win->ShowModal();
}


/*==============================
    ServerBrowser::m_MenuItem_File_Connect_OnMenuSelection
    Event handler for Quit menu item selection
    @param The command event
==============================*/

void ServerBrowser::m_MenuItem_File_Quit_OnMenuSelection(wxCommandEvent& event)
{
    (void)event;
    this->Destroy();
}


/*==============================
    ServerBrowser::CreateClient
    Create the Client window for USB + Internet connectivity
    @param The ROM to upload via USB
    @param The server address:port to connect to
==============================*/

void ServerBrowser::CreateClient(wxString rom, wxString addressport)
{
    int port;
    wxString address;
    ClientWindow* cw = new ClientWindow(this);

    // Initialize the client window
    address_fromstr(addressport, &address, &port);
    cw->SetROM(rom);
    cw->SetAddress(address);
    cw->SetPortNumber(port);
    this->Lower();
    cw->BeginWorking();
    cw->Raise();
    cw->Show();
}


/*==============================
    ServerBrowser::ConnectMaster
    Create the master server connection thread
    @param Whether to start the thread right now (if possible)
==============================*/

void ServerBrowser::ConnectMaster()
{
    // Clear the server list
    this->ClearServers();

    // (Re)start the finder thread
    if (this->m_FinderThread != NULL)
        this->StopThread_Finder();
    this->StartThread_Finder();

    // Send a message to the finder thread that we wanna connect to the master server
    printf("Requesting server list from Master Server\n");
    global_msgqueue_serverthread_input.Post(new InputMessage{TEVENT_DOLIST, NULL});
}


/*==============================
    ServerBrowser::RequestDownload
    Request a file download from the master server
    @param The hash of the file to download
    @param The destination filepath of the ROM
==============================*/

void ServerBrowser::RequestDownload(wxString hash, wxString filepath)
{
    InputMessage* usrinput;
    uint8_t romhash[32];

    // Get the hash as bytes
    hash = hash.Lower();
    for (size_t i=0; i<hash.length(); i+=2)
    {
        char first = hash[i];
        char second = hash[i+1];
        if (first >= 'a' && first <= 'f')
            first = 10 + first - 'a';
        else
            first = first - '0';
        if (second >= 'a' && second <= 'f')
            second = 10 + second - 'a';
        else
            second = second - '0';
        romhash[i/2] = (first << 4) | second;
    }

    // Create the finder thread if it doesn't exist yet
    if (this->m_FinderThread == NULL)
        this->StartThread_Finder();

    // Send a message to the finder thread that we wanna connect to download a ROM
    printf("Requesting ROM download from Master Server into '%s'\n", static_cast<const char*>(filepath.c_str()));
    usrinput = new InputMessage{TEVENT_FILENAME, NULL};
    usrinput->data = (char*)malloc(filepath.Length()+1);
    strcpy(usrinput->data, filepath.mb_str());
    global_msgqueue_serverthread_input.Post(usrinput);
    usrinput = new InputMessage{TEVENT_DODL, NULL};
    usrinput->data = (char*)malloc(32);
    memcpy(usrinput->data, romhash, 32);
    global_msgqueue_serverthread_input.Post(usrinput);
}


/*==============================
    ServerBrowser::ClearServers
    Clear the list of servers
==============================*/

void ServerBrowser::ClearServers()
{
    this->m_DataViewListCtrl_Servers->DeleteAllItems();
}


/*==============================
    ServerBrowser::ThreadEvent
    Handles messages from the connection thread
    @param The thread event
==============================*/

void ServerBrowser::ThreadEvent(wxThreadEvent& event)
{
    switch ((ThreadEventType)event.GetInt())
    {
        case TEVENT_ADDSERVER:
        {
            wxString rompath;
            FoundServer* server = event.GetPayload<FoundServer*>();
            wxVector<wxVariant> data;
            rompath = get_rompath(server->romname);
            data.push_back(wxVariant(wxString::Format("%ld", server->ping)));
            data.push_back(wxVariant(wxString::Format("%d/%d", server->playercount, server->maxplayers)));
            data.push_back(wxVariant(server->fulladdress));
            if (wxFileExists(rompath))
            {
                if (is_samerom(server->romname, server->romhash))
                    data.push_back(wxVariant(wxString::Format("<span color=\"#008000\">%s</span>", server->romname)));
                else
                    data.push_back(wxVariant(wxString::Format("<span color=\"#FF8000\">%s</span>", server->romname)));
            }
            else if (!server->romdownloadable)
                data.push_back(wxVariant(wxString::Format("<span color=\"#800000\">%s</span>", server->romname)));
            else
                data.push_back(wxVariant(server->romname));
            data.push_back(wxVariant(server->name));
            data.push_back(wxVariant(server->romhash));
            data.push_back(wxVariant(wxString::Format("%d", server->romdownloadable)));
            this->m_DataViewListCtrl_Servers->AppendItem(data);
            free(server);
            break;
        }
        default:
            break;
    }
}


/*==============================
    ServerBrowser::GetAddress
    Retreives the server address to connect to
    @return The server address to connect to
==============================*/

wxString ServerBrowser::GetAddress()
{
    return this->m_MasterAddress;
}


/*==============================
    ServerBrowser::GetPort
    Retreives the server port to use
    @return The server port to use
==============================*/

int ServerBrowser::GetPort()
{
    return this->m_MasterPort;
}


/*=============================================================
                     Server Finder Thread
=============================================================*/

/*==============================
    ServerFinderThread (Constructor)
    Initializes the class
    @param The parent window
==============================*/

ServerFinderThread::ServerFinderThread(ServerBrowser* win) : wxThread(wxTHREAD_JOINABLE)
{
    this->m_Window = win;
}


/*==============================
    ServerFinderThread (Destructor)
    Cleans up the class before deletion
==============================*/

ServerFinderThread::~ServerFinderThread()
{
    // Nothing here
}


/*==============================
    ServerFinderThread::Entry
    The entry function for the thread
    @return The exit code
==============================*/

void* ServerFinderThread::Entry()
{
    std::unordered_map<wxString, std::pair<FoundServer, wxLongLong>> serversleft; 
    wxIPV4address localaddr;
    localaddr.AnyAddress();
    localaddr.Service(0);
    uint8_t* buff = (uint8_t*)malloc(4096);
    wxDatagramSocket* sock = new wxDatagramSocket(localaddr , wxSOCKET_NOWAIT);
    UDPHandler* handler = new UDPHandler(sock, this->m_Window->GetAddress(), this->m_Window->GetPort());
    FileDownload* filedl = NULL;
    wxString filedl_path = "";

    // Run in a loop until the main thread wants to kill us
    while (!TestDestroy() && this->m_Window != NULL)
    {
        try 
        {
            S64Packet* pkt = NULL;

            // Check for packets from the master server / servers we pinged
            sock->Read(buff, 4096);
            while (sock->LastReadCount() > 0)
            {
                pkt = handler->ReadS64Packet(buff);

                // Handle various packets that we received from a server
                if (pkt != NULL)
                {
                    try
                    {
                        if (!strncmp(pkt->GetType(), "SERVER", 6))
                        {
                            FoundServer server = this->ParsePacket_Server(handler->GetSocket(), pkt);
                            serversleft[server.fulladdress] = std::make_pair(server, wxGetLocalTimeMillis());
                        }
                        else if (!strncmp(pkt->GetType(), "DONELISTING", 11))
                            printf("Master server finished sending server list\n");
                        else if (!strncmp(pkt->GetType(), "DOWNLOAD", 8))
                            filedl = this->BeginFileDownload(pkt, filedl_path);
                        else if (!strncmp(pkt->GetType(), "FILEDATA", 8))
                            this->HandleFileData(pkt, &filedl);
                        else if (!strncmp(pkt->GetType(), "DISCOVER", 8))
                            this->DiscoveredServer(&serversleft, pkt);
                        else
                            printf("Unexpected packet type received '%s'\n", static_cast<const char*>(pkt->GetType().c_str()));
                    }
                    catch (BadPacketVersionException& e)
                    {
                        printf("Got unsupported packet version %d\n", e.what());
                        break;
                    }
                }

                // Check for more packets
                sock->Read(buff, 4096);
            }
            handler->ResendMissingPackets();

            // If we have a file download in progress, request some chunks
            if (filedl != NULL)
            {
                std::list<std::pair<uint32_t, wxLongLong>>::iterator it;
                for (it = filedl->chunksleft.begin(); it != filedl->chunksleft.end(); ++it)
                {
                    std::pair<uint32_t, wxLongLong> itpair = *it;
                    if (itpair.second == 0 || wxGetLocalTimeMillis() - itpair.second > 1000)
                    {
                        uint32_t chunk = swap_endian32(itpair.first);
                        handler->SendPacket(new S64Packet("FILEDATA", sizeof(uint32_t), (uint8_t*)&chunk, FLAG_UNRELIABLE));
                        itpair.second = wxGetLocalTimeMillis();
                        wxMilliSleep(50); // TODO: Write a congestion avoidance algorithm so that we can send packets faster than this
                        break;
                    }
                }
            }
            else
                wxMilliSleep(10);

            // Check for messages from the main thread
            this->HandleMainInput(handler, &filedl, &filedl_path);
        }
        catch (ClientTimeoutException& e)
        {
            wxString fulladdr = wxString::Format("%s:%d", handler->GetAddress(), handler->GetPort());

            // Log error messages based on what server timed out
            if (e.what() == fulladdr)
            {
                printf("Master server timed out\n");
                if (filedl != NULL)
                {
                    filedl_path = "";
                    free(filedl->filedata);
                    delete filedl;
                    filedl = NULL;
                }
                break;
            }
            else
            {
                if (serversleft.find(fulladdr) != serversleft.end())
                {
                    std::pair<FoundServer, wxLongLong> found = serversleft[fulladdr];
                    printf("Server %s timed out\n", static_cast<const char*>(fulladdr.c_str()));
                    delete found.first.handler;
                    serversleft.erase(fulladdr);
                }
            }
        }
    }

    // Cleanup
    for (std::pair<wxString, std::pair<FoundServer, wxLongLong>> it : serversleft)
        delete it.second.first.handler;
    delete handler;
    free(buff);
    sock->Destroy();
    return NULL;
}


/*==============================
    ServerFinderThread::HandleMainInput
    Handle input from the main thread
    @param A pointer to the UDP handler
    @param A pointer to the pointer of the file download struct
    @param A pointer to the string with the file download path
==============================*/

void ServerFinderThread::HandleMainInput(UDPHandler* handler, FileDownload** filedl, wxString* filedl_path)
{
    InputMessage* usrinput = NULL;
    while (global_msgqueue_serverthread_input.ReceiveTimeout(0, usrinput) == wxMSGQUEUE_NO_ERROR)
    {
        switch (usrinput->type)
        {
            case TEVENT_DOLIST:
                handler->SendPacket(new S64Packet("LIST", 0, NULL, FLAG_UNRELIABLE));
                break;
            case TEVENT_DODL:
                handler->SendPacket(new S64Packet("DOWNLOAD", 32, (uint8_t*)usrinput->data, FLAG_UNRELIABLE));
                break;
            case TEVENT_CANCELDL:
                if (*filedl != NULL)
                {
                    *filedl_path = "";
                    free((*filedl)->filedata);
                    delete *filedl;
                    *filedl = NULL;
                }
                break;
            case TEVENT_FILENAME:
                *filedl_path = wxString(usrinput->data);
                break;
            default:
                break;
        }
        free(usrinput->data);
        delete usrinput;
    }
}


/*==============================
    ServerFinderThread::ParsePacket_Server
    Parse a server info packet from the master server
    @param The socket to write the ping to
    @param The packet with the server info
==============================*/

FoundServer ServerFinderThread::ParsePacket_Server(wxDatagramSocket* socket, S64Packet* pkt)
{
    uint8_t hash[32];
    uint32_t read32;
    uint8_t* buf = pkt->GetData();
    uint32_t buffoffset = 0;
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
    server.handler->SendPacket(new S64Packet("DISCOVER", serveraddr.length(), (uint8_t*)serveraddr.data(), FLAG_UNRELIABLE));
    
    // Done
    return server;
}


/*==============================
    ServerFinderThread::DiscoveredServer
    Parse a response packet from a server we pinged
    @param The map to place the known server in
    @param The packet with the server info
==============================*/

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
        printf("Discovered %s in %lldms\n", static_cast<const char*>(fulladdress.c_str()), server->ping.GetValue());

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


/*==============================
    ServerFinderThread::BeginFileDownload
    Prepare a file for download from the master server
    @param The packet with the file info
    @param The filepath to store the file in
==============================*/

FileDownload* ServerFinderThread::BeginFileDownload(S64Packet* pkt, wxString filepath)
{
    uint32_t chunkcount;
    FileDownload* ret = NULL;
    if (filepath == "" || pkt->GetSize() == 0)
        return NULL;
    ret = new FileDownload();

    // Assign file info
    ret->filepath = filepath;
    memcpy(&ret->filesize, &pkt->GetData()[0], sizeof(uint32_t));
    ret->filesize = swap_endian32(ret->filesize);
    ret->filedata = (uint8_t*)malloc(ret->filesize);
    if (ret->filedata == NULL)
    {
        delete ret;
        return NULL;
    }

    // Get chunk data
    memcpy(&ret->chunksize, &pkt->GetData()[4], sizeof(uint32_t));
    ret->chunksize = swap_endian32(ret->chunksize);
    chunkcount = ceilf(((float)ret->filesize)/ret->chunksize);
    for (uint32_t i=0; i<chunkcount; i++)
        ret->chunksleft.push_back(std::make_pair(i, 0));

    // Done
    return ret;
}


/*==============================
    ServerFinderThread::HandleFileData
    Handle a file chunk from the server
    @param The packet with the file chunk
    @param A pointer to the file download struct
==============================*/

void ServerFinderThread::HandleFileData(S64Packet* pkt, FileDownload** filedlp)
{
    FileDownload* filedl = *filedlp;
    uint32_t chunknum, readsize;
    std::list<std::pair<uint32_t, wxLongLong>>::iterator it;

    // Ensure we have a valid packet
    if (filedl == NULL || pkt->GetSize() == 0)
        return;

    // Read data from the packet
    memcpy(&chunknum, &pkt->GetData()[0], sizeof(uint32_t));
    chunknum = swap_endian32(chunknum);
    memcpy(&readsize, &pkt->GetData()[4], sizeof(uint32_t));
    readsize = swap_endian32(readsize);
    memcpy(&filedl->filedata[chunknum*filedl->chunksize], &pkt->GetData()[8], readsize);

    // Remove this chunk from the list
    for (it = filedl->chunksleft.begin(); it != filedl->chunksleft.end(); ++it)
    {
        std::pair<uint32_t, wxLongLong> itpair = *it;
        if (itpair.first == chunknum)
        {
            filedl->chunksleft.erase(it);
            break;
        }
    }

    // If no chunks are left, dump this ROM to a file
    if (filedl->chunksleft.size() == 0)
    {
        printf("Download done, saving file\n");
        FILE* fp = fopen(filedl->filepath, "wb+");
        fwrite(filedl->filedata, 1, filedl->filesize, fp);
        fclose(fp);
        this->m_Window->m_DownloadWindow->UpdateDownloadProgress(100);
        free(filedl->filedata);
        delete filedl;
        filedl = NULL;
        return;
    }
    this->m_Window->m_DownloadWindow->UpdateDownloadProgress((1-((float)filedl->chunksize*filedl->chunksleft.size())/filedl->filesize)*100);
}



/*=============================================================
                Manual Server Connection Window
=============================================================*/

/*==============================
    ManualConnectWindow (Constructor)
    Initializes the class
    @param The parent window
    @param The window ID to use
    @param The title of the window
    @param The position of the window on the screen
    @param The size of the window
    @param Style flags
==============================*/

ManualConnectWindow::ManualConnectWindow(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    // Initialize the main window sizer
    wxFlexGridSizer* m_Sizer_Main;
    m_Sizer_Main = new wxFlexGridSizer(2, 1, 0, 0);
    m_Sizer_Main->AddGrowableCol(0);
    m_Sizer_Main->AddGrowableRow(0);
    m_Sizer_Main->SetFlexibleDirection(wxBOTH);
    m_Sizer_Main->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Add the sizer for the input boxes
    wxFlexGridSizer* m_Sizer_Inputs;
    m_Sizer_Inputs = new wxFlexGridSizer(0, 2, 0, 0);
    m_Sizer_Inputs->AddGrowableCol(1);
    m_Sizer_Inputs->AddGrowableRow(1);
    m_Sizer_Inputs->SetFlexibleDirection(wxBOTH);
    m_Sizer_Inputs->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Server IP text
    wxStaticText* m_StaticText_Server;
    m_StaticText_Server = new wxStaticText(this, wxID_ANY, wxT("Server IP:"), wxDefaultPosition, wxDefaultSize, 0);
    m_StaticText_Server->Wrap(-1);
    m_Sizer_Inputs->Add(m_StaticText_Server, 0, wxALIGN_CENTER|wxALL, 5);

    // Server IP input
    this->m_TextCtrl_Server = new wxTextCtrl(this, wxID_ANY, wxConfigBase::Get()->Read("ManualAddress", wxEmptyString), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Inputs->Add(this->m_TextCtrl_Server, 0, wxALL|wxEXPAND, 5);

    // ROM path text
    wxStaticText* m_StaticText_ROM;
    m_StaticText_ROM = new wxStaticText(this, wxID_ANY, wxT("ROM Path:"), wxDefaultPosition, wxDefaultSize, 0);
    m_StaticText_ROM->Wrap(-1);
    m_Sizer_Inputs->Add(m_StaticText_ROM, 0, wxALIGN_CENTER|wxALL, 5);

    // ROM path input
    this->m_FilePicker_ROM = new wxFilePickerCtrl(this, wxID_ANY, wxConfigBase::Get()->Read("ManualROMPath", wxEmptyString), wxT("Load ROM"), wxT("*.*"), wxDefaultPosition, wxDefaultSize, wxFLP_DEFAULT_STYLE|wxFLP_FILE_MUST_EXIST|wxFLP_OPEN|wxFLP_SMALL|wxFLP_USE_TEXTCTRL);
    this->m_FilePicker_ROM->DragAcceptFiles(true);
    this->m_FilePicker_ROM->SetPath(wxConfigBase::Get()->Read("ManualROMPath", wxEmptyString));
    this->m_FilePicker_ROM->SetInitialDirectory(wxConfigBase::Get()->Read("ManualROMPath", wxEmptyString));
    m_Sizer_Inputs->Add(this->m_FilePicker_ROM, 0, wxALL|wxEXPAND, 5);
    m_Sizer_Main->Add(m_Sizer_Inputs, 1, wxEXPAND, 5);

    // Connect button
    this->m_Button_Connect = new wxButton(this, wxID_ANY, wxT("Connect"), wxDefaultPosition, wxDefaultSize, 0);
    m_Sizer_Main->Add(this->m_Button_Connect, 0, wxALIGN_CENTER|wxALL, 5);

    // Finalize the layour
    this->SetSizer(m_Sizer_Main);
    this->Layout();
    this->Centre(wxBOTH);

    // Connect Events
    this->m_Button_Connect->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ManualConnectWindow::m_Button_Connect_OnButtonClick), NULL, this);
}


/*==============================
    ManualConnectWindow (Destructor)
    Cleans up the class before deletion
==============================*/

ManualConnectWindow::~ManualConnectWindow()
{
    // Disconnect Events
    m_Button_Connect->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ManualConnectWindow::m_Button_Connect_OnButtonClick), NULL, this);
}


/*==============================
    ManualConnectWindow::m_Button_Connect_OnButtonClick
    Event handler for Connect button clicking
    @param The command event
==============================*/

void ManualConnectWindow::m_Button_Connect_OnButtonClick(wxCommandEvent& event)
{
    (void)event;
    wxString path = this->m_FilePicker_ROM->GetPath();
    wxString address = this->m_TextCtrl_Server->GetValue();
    ServerBrowser* parent = (ServerBrowser*)this->GetParent();
    wxConfigBase::Get()->Write("ManualAddress", address);
    wxConfigBase::Get()->Write("ManualROMPath", path);
    this->Destroy(); // Must be done first or the parent window will steal attention from the client
    parent->CreateClient(path, address);
}