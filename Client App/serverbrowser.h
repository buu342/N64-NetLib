#pragma once

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

class ServerBrowser : public wxFrame
{
    private:

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

    public:
        ServerBrowser(wxWindow* parent = NULL, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 800,600 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
        ~ServerBrowser();

};