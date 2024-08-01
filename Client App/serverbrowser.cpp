#include "serverbrowser.h"
#include "clientwindow.h"
#include "romdownloader.h"
#include "helper.h"
#include "sha256.h"
#include <stdint.h>
#include <wx/dir.h>
#include <wx/msgdlg.h>

#define MASTERSERVERPACKET_HEADER "N64PKT"

typedef enum {
    TEVENT_ADDSERVER,
    TEVENT_THREADENDED,
} ThreadEventType;

ServerBrowser::ServerBrowser(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
    this->m_MasterAddress = DEFAULT_MASTERSERVER_ADDRESS; // TODO: This has to actually be editable
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
    this->m_DataViewListColumn_Hash = m_DataViewListCtrl_Servers->AppendTextColumn(wxT("Hash"), wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_HIDDEN);
    this->m_DataViewListColumn_FileExistsOnMaster = m_DataViewListCtrl_Servers->AppendTextColumn(wxT("File Exists on Master"), wxDATAVIEW_CELL_INERT, -1, wxALIGN_LEFT, wxDATAVIEW_COL_HIDDEN);
    m_Sizer_Main->Add(m_DataViewListCtrl_Servers, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL|wxEXPAND, 5);

    m_Sizer_Main->AddGrowableCol(0);
    m_Sizer_Main->AddGrowableRow(0);

    this->SetSizer(m_Sizer_Main);
    this->Layout();

    this->Centre(wxBOTH);
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ServerBrowser::ThreadEvent));
    this->Connect(this->m_Tool_Refresh->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(ServerBrowser::m_Tool_RefreshOnToolClicked));
    this->m_DataViewListCtrl_Servers->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(ServerBrowser::m_DataViewListCtrl_ServersOnDataViewListCtrlItemActivated), NULL, this);

    this->ConnectMaster();
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
    this->Disconnect(this->m_Tool_Refresh->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler(ServerBrowser::m_Tool_RefreshOnToolClicked));
    this->m_DataViewListCtrl_Servers->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxDataViewEventHandler(ServerBrowser::m_DataViewListCtrl_ServersOnDataViewListCtrlItemActivated), NULL, this);
}

void ServerBrowser::m_Tool_RefreshOnToolClicked(wxCommandEvent& event)
{
    this->ClearServers();
    this->ConnectMaster();
}

void ServerBrowser::m_DataViewListCtrl_ServersOnDataViewListCtrlItemActivated(wxDataViewEvent& event)
{
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
        for (int i=0; i<(sizeof(hash)/sizeof(hash[0])); i++)
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
            this->CreateClient(rompath);
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
                rdw->SetPort(this->m_MasterPort);
                rdw->BeginConnection();
                if (rdw->ShowModal() == 0)
                {
                    wxMessageDialog dialog(this, wxString("ROM download was unsuccessful."), wxString("ROM Download Failed"), wxICON_ERROR);
                    dialog.ShowModal();
                }
                else
                    this->CreateClient(rompath);
            }
        }
        else
        {
            wxMessageDialog dialog(this, wxString("This server requires the ROM '") + romname + wxString("', which is not available for download from the master server.\n\nIn order to join this server, you must manually find and download the ROM, and place it in your ROMs folder."), wxString("ROM Unavailable"), wxICON_ERROR);
            dialog.ShowModal();
        }
    }
}

void ServerBrowser::CreateClient(wxString rom)
{
    ClientWindow* cw = new ClientWindow(this);
    cw->SetROM(rom);
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
            data.push_back(wxVariant(wxString::Format("?/%d", server->maxplayers)));
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
    char outtext[64];
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
    sprintf(outtext, "N64PKT");
    this->m_Socket->Write(outtext, strlen(outtext));
    sprintf(outtext, "LIST");
    packetsize = swap_endian32(strlen(outtext));
    this->m_Socket->Write(&packetsize, sizeof(int));
    this->m_Socket->Write(outtext, swap_endian32(packetsize));
    // TODO: Send the packet version
    printf("Requested server list\n");
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
    uint32_t i;
    uint32_t strsize;
    int buffoffset = 6;
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

    // Send the server info to the main thread
    this->AddServer(&server);
}

void ServerFinderThread::AddServer(FoundServer* server)
{
    FoundServer* server_copy = new FoundServer();
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);

    // Copy the server data
    server_copy->name = server->name;
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