/***************************************************************
                            app.cpp

This file handles the wxWidgets initialization.
***************************************************************/

#include <wx/socket.h>
#include "app.h"

#include "Resources/resources.h"

wxIMPLEMENT_APP(App);

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
    @returns Whether the application initialized correctly
==============================*/

bool App::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    // Initialize image handlers
    wxInitAllImageHandlers();
    wxSocketBase::Initialize();

    // Create icons
    icon_refresh = wxBITMAP_PNG_FROM_DATA(icon_refresh);

    // Show the main window
    this->m_Frame = new ServerBrowser();
    this->m_Frame->Show();
    SetTopWindow(this->m_Frame);
    return true;
}