#pragma once

typedef struct IUnknown IUnknown;

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/toolbar.h>
#include <wx/gbsizer.h>
#include <wx/frame.h>
#include <wx/socket.h>
#include <wx/artprov.h>
#include <wx/stattext.h>
#include <wx/filepicker.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/dialog.h>
#include <unordered_map>
#include <list>
#include "customview.h"
#include "packets.h"
#include "romdownloader.h"

#define DEFAULT_MASTERSERVER_ADDRESS "localhost"
#define DEFAULT_MASTERSERVER_PORT    6464

typedef struct
{
    UDPHandler* handler;
    wxString fulladdress;
    wxString name;
    int playercount;
    int maxplayers;
    wxLongLong ping;
    wxString romname;
    wxString romhash;
    bool romdownloadable;
} FoundServer;

typedef struct
{
    wxString filepath;
    uint8_t* filedata;
    uint32_t filesize;
    uint32_t chunksize;
    std::list<std::pair<uint32_t, wxLongLong>> chunksleft;
} FileDownload;

class ServerFinderThread;

class ServerBrowser : public wxFrame
{
    private:
        wxString    m_MasterAddress;
        int         m_MasterPort;
        UDPHandler* m_MasterConnectionHandler;
        wxDatagramSocket* m_Socket;
        ServerFinderThread* m_FinderThread;
        wxCriticalSection m_FinderThreadCS;

    protected:
        wxMenuBar* m_MenuBar;
        wxMenu* m_Menu_File;
        wxToolBar* m_ToolBar;
        wxToolBarToolBase* m_Tool_Refresh;
        wxTextCtrl* m_TextCtrl_MasterServerAddress;
        CustomDataView*   m_DataViewListCtrl_Servers;
        wxDataViewColumn* m_DataViewListColumn_Ping;
        wxDataViewColumn* m_DataViewListColumn_Players;
        wxDataViewColumn* m_DataViewListColumn_ServerName;
        wxDataViewColumn* m_DataViewListColumn_Address;
        wxDataViewColumn* m_DataViewListColumn_ROM;
        wxDataViewColumn* m_DataViewListColumn_Hash;
        wxDataViewColumn* m_DataViewListColumn_FileExistsOnMaster;

        void m_MenuItem_File_Connect_OnMenuSelection(wxCommandEvent& event);
        void m_MenuItem_File_Quit_OnMenuSelection(wxCommandEvent& event);
        void m_Tool_Refresh_OnToolClicked(wxCommandEvent& event);
        void m_TextCtrl_MasterServerAddress_OnText(wxCommandEvent& event);
        void m_DataViewListCtrl_Servers_OnDataViewListCtrlItemActivated(wxDataViewEvent& event);
        void m_DataViewListCtrl_Servers_OnMotion(wxMouseEvent& event);

    public:
        ROMDownloadWindow* m_DownloadWindow;
        ServerBrowser(wxWindow* parent = NULL, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 800,600 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
        ~ServerBrowser();

        void CreateClient(wxString rom, wxString address);
        void ConnectMaster();
        void ClearServers();
        void ThreadEvent(wxThreadEvent& event);
        void RequestDownload(wxString hash, wxString filepath);
        wxString          GetAddress();
        int               GetPort();
        wxDatagramSocket* GetSocket();
};

class ServerFinderThread : public wxThread
{
    private:
        ServerBrowser* m_Window;

    protected:

    public:
        ServerFinderThread(ServerBrowser* win);
        ~ServerFinderThread();

        virtual void* Entry() wxOVERRIDE;
        FoundServer   ParsePacket_Server(wxDatagramSocket* socket, S64Packet* pkt);
        void          DiscoveredServer(std::unordered_map<wxString, std::pair<FoundServer, wxLongLong>>* serverlist, S64Packet* pkt);
        FileDownload* BeginFileDownload(S64Packet* pkt, wxString filepath);
        void          HandleFileData(S64Packet* pkt, FileDownload** filedlp);
        void          NotifyMainOfDeath();
        void          UpdateDLProgress(int progress);
};

class ManualConnectWindow : public wxDialog
{
    private:

    protected:
        wxTextCtrl* m_TextCtrl_Server;
        wxFilePickerCtrl* m_FilePicker_ROM;
        wxButton* m_Button_Connect;

        void m_Button_Connect_OnButtonClick(wxCommandEvent& event);

    public:
        ManualConnectWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Manual Server Connect"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 300,125 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
        ~ManualConnectWindow();
};