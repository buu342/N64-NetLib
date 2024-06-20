/***************************************************************
                            app.cpp

This file handles the wxWidgets initialization.
***************************************************************/

#include <wx/socket.h>
#include "app.h"

wxIMPLEMENT_APP(App);


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

    // Show the main window
    this->m_Frame = new ServerBrowser();
    this->m_Frame->Show();
    SetTopWindow(this->m_Frame);
    return true;
}