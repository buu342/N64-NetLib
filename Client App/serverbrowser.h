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
#include <wx/dataview.h>
#include <wx/gbsizer.h>
#include <wx/frame.h>
#include <wx/socket.h>

#define DEFAULT_MASTERSERVER_ADDRESS "localhost"
#define DEFAULT_MASTERSERVER_PORT    6464

typedef struct
{
    wxString name;
    wxString address;
    int playercount;
    int maxplayers;
    int ping;
    wxString rom;
    wxString hash;
    bool romonmaster;
} FoundServer;

class ServerFinderThread;

class ServerBrowser : public wxFrame
{
    private:
        wxString m_MasterAddress;
        int      m_MasterPort;
        ServerFinderThread* m_FinderThread;
        wxCriticalSection m_FinderThreadCS;

    protected:
        wxMenuBar* m_MenuBar;
        wxMenu* m_Menu_File;
        wxToolBar* m_ToolBar;
        wxToolBarToolBase* m_Tool_Refresh;
        wxTextCtrl* m_TextCtrl_MasterServerAddress;
        wxDataViewListCtrl* m_DataViewListCtrl_Servers;
        wxDataViewColumn* m_DataViewListColumn_Ping;
        wxDataViewColumn* m_DataViewListColumn_Players;
        wxDataViewColumn* m_DataViewListColumn_ServerName;
        wxDataViewColumn* m_DataViewListColumn_Address;
        wxDataViewColumn* m_DataViewListColumn_ROM;
        wxDataViewColumn* m_DataViewListColumn_Hash;
        wxDataViewColumn* m_DataViewListColumn_FileExistsOnMaster;

        void m_Tool_RefreshOnToolClicked(wxCommandEvent& event);
        void m_DataViewListCtrl_ServersOnDataViewListCtrlItemActivated(wxDataViewEvent& event);

    public:
        ServerBrowser(wxWindow* parent = NULL, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 800,600 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
        ~ServerBrowser();

        void CreateClient(wxString rom);
        void ConnectMaster();
        void ClearServers();
        void ThreadEvent(wxThreadEvent& event);
        wxString GetAddress();
        int GetPort();
};

class ServerFinderThread : public wxThread
{
    private:
        wxSocketClient* m_Socket;
        ServerBrowser* m_Window;

    protected:

    public:
        ServerFinderThread(ServerBrowser* win);
        ~ServerFinderThread();

        virtual void* Entry() wxOVERRIDE;
        void OnSocketEvent(wxSocketEvent& event);
        void ParsePacket_Server(char* buf);
        void AddServer(FoundServer* server);
        void NotifyMainOfDeath();
};