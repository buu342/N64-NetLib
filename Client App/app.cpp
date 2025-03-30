/***************************************************************
                            app.cpp

This file handles the wxWidgets initialization.
***************************************************************/

#include "app.h"
#include <wx/stdpaths.h>
#include <wx/config.h>
#include <wx/fileconf.h>
#include <wx/dir.h>

#include "Resources/resources.h"

wxIMPLEMENT_APP(App);


/******************************
            Globals
******************************/

wxIcon icon_program = wxNullIcon;
wxBitmap icon_refresh = wxNullBitmap;


/*==============================
    App (Constructor)
    Initializes the class
==============================*/

App::App()
{
    
}


/*==============================
    App (Destructor)
    Cleans up the class before deletion
==============================*/

App::~App()
{
    
}


/*==============================
    App::OnInit
    Called when the application is initialized
    @return Whether the application initialized correctly
==============================*/

bool App::OnInit()
{
    wxString cfgpath;
    wxFileConfig* cfgfile;
    const wxString cfgname = "config.cfg";

    // Ensure the window loaded properly
    if (!wxApp::OnInit())
        return false;

    // Initialize image handlers
    wxInitAllImageHandlers();

    // Initialize asio
    ASIOSocket::InitASIO();

    // Initialize config file
    wxStandardPaths::Get().SetFileLayout(wxStandardPaths::FileLayout_XDG);
    cfgpath = wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + "N64NetLibBrowser" + wxFileName::GetPathSeparator();
    if (!wxDirExists(cfgpath))
        wxDir::Make(cfgpath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    cfgfile = new wxFileConfig(wxEmptyString, wxEmptyString, cfgpath + cfgname);
    wxConfigBase::Set(cfgfile);

    // Create icons
    icon_refresh = wxBITMAP_PNG_FROM_DATA(icon_refresh);
    wxBitmap temp = wxBITMAP_PNG_FROM_DATA(icon_program);
    icon_program.CopyFromBitmap(temp);

    // Show the main window
    this->m_Frame = new ServerBrowser();
    this->m_Frame->SetIcon(icon_program);
    this->m_Frame->Show();
    SetTopWindow(this->m_Frame);
    return true;
}