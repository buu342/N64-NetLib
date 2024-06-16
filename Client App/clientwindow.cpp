#include <wx/event.h>
#include "clientwindow.h"
#include "Include/device.h"

typedef enum {
    TEVENT_CLEARCONSOLE,
    TEVENT_WRITECONSOLE,
    TEVENT_SETSTATUS,
    TEVENT_UPLOADPROGRESS,
} ThreadEventType;

ClientWindow::ClientWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    DeviceThread* dev;
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxFlexGridSizer* m_Sizer_Main;
    m_Sizer_Main = new wxFlexGridSizer( 2, 0, 0, 0 );
    m_Sizer_Main->AddGrowableCol( 0 );
    m_Sizer_Main->AddGrowableRow( 0 );
    m_Sizer_Main->SetFlexibleDirection( wxBOTH );
    m_Sizer_Main->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_RichText_Console = new wxRichTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxVSCROLL|wxHSCROLL|wxNO_BORDER|wxWANTS_CHARS );

    m_Sizer_Main->Add( m_RichText_Console, 1, wxEXPAND | wxALL, 5 );

    m_Sizer_Input = new wxGridBagSizer( 0, 0 );
    m_Sizer_Input->SetFlexibleDirection( wxBOTH );
    m_Sizer_Input->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_TextCtrl_Input = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_TextCtrl_Input->Enable( false );
    m_Sizer_Input->Add( m_TextCtrl_Input, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

    m_Gauge_Upload = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_SMOOTH|wxGA_PROGRESS);
    m_Gauge_Upload->SetValue(0);
    m_Gauge_Upload->Disable();
    m_Gauge_Upload->Hide();

    m_Button_Send = new wxButton( this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0 );
    m_Button_Send->Enable( false );
    m_Sizer_Input->Add( m_Button_Send, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );

    m_Sizer_Input->AddGrowableCol( 0 );
    m_Sizer_Main->Add( m_Sizer_Input, 1, wxEXPAND, 5 );

    this->SetSizer( m_Sizer_Main );
    this->Layout();
    //m_StatusBar_ClientStatus = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

    this->Centre( wxBOTH );
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ClientWindow::ThreadEvent));

    dev = new DeviceThread(this);
    if (dev->Create() != wxTHREAD_NO_ERROR)
    {
        delete dev;
    }
    else if (dev->Run() != wxTHREAD_NO_ERROR)
    {
        delete dev;
    }
}

ClientWindow::~ClientWindow()
{
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ClientWindow::ThreadEvent));
}

void ClientWindow::SetClientDeviceStatus(ClientDeviceStatus status)
{
    this->m_DeviceStatus = status;
    switch(status)
    {
        case CLSTATUS_UPLOADING:
            this->m_TextCtrl_Input->Hide();
            this->m_TextCtrl_Input->Disable();
            this->m_Sizer_Input->Detach(this->m_TextCtrl_Input);
            this->m_Sizer_Input->Add(this->m_Gauge_Upload, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5);
            this->m_Gauge_Upload->Enable();
            this->m_Gauge_Upload->Show();
            this->m_Button_Send->SetLabel(wxT("Cancel"));
            this->m_Button_Send->Enable();
            this->Layout();
            this->Refresh();
            break;
        case CLSTATUS_UPLOADDONE:
            this->m_Button_Send->SetLabel(wxT("Reupload"));
            this->m_Button_Send->Enable();
            break;
    }
}

void ClientWindow::ThreadEvent(wxThreadEvent& event)
{
    switch ((ThreadEventType)event.GetInt())
    {
        case TEVENT_CLEARCONSOLE:
            this->m_RichText_Console->Clear();
            break;
        case TEVENT_WRITECONSOLE:
            this->m_RichText_Console->WriteText(event.GetString());
            break;
        case TEVENT_SETSTATUS:
            this->SetClientDeviceStatus((ClientDeviceStatus)event.GetExtraLong());
            break;
        case TEVENT_UPLOADPROGRESS:
            int prog = event.GetExtraLong();
            this->m_Gauge_Upload->SetValue(prog);
            break;
    }
}


/*=============================================================

=============================================================*/

DeviceThread::DeviceThread(ClientWindow* win)
{
    device_initialize();
    device_setrom((char*)"unflexa4.z64");
    this->m_Window = win;
}

DeviceThread::~DeviceThread()
{
    if (device_isopen())
        device_close();
}

void* DeviceThread::Entry()
{
    FILE* fp;
    float oldprogress = 0;
    DeviceError deverr = device_find();

    // Check which flashcart we found
    this->ClearConsole();
    this->WriteConsole(wxT("Searching for a valid flashcart"));
    if (deverr != DEVICEERR_OK)
    {
        this->WriteConsole(wxString::Format(wxT("\nError finding flashcart. Returned error %d."), deverr));
        return NULL;
    }
    this->WriteConsole(wxT("\nFound "));
    switch (device_getcart())
    {
        case CART_64DRIVE1: this->WriteConsole(wxT("64Drive HW1")); break;
        case CART_64DRIVE2: this->WriteConsole(wxT("64Drive HW2")); break;
        case CART_EVERDRIVE: this->WriteConsole(wxT("EverDrive")); break;
        case CART_SC64: this->WriteConsole(wxT("SummerCart64")); break;
    }

    // Open the cart
    this->WriteConsole("\nOpening device");
    if (device_open() != DEVICEERR_OK)
    {
        this->WriteConsole(wxString::Format(wxT("\nError opening flashcart. Returned error %d."), deverr));
        return NULL;
    }

    // Test that debug mode is possible
    if (device_testdebug() != DEVICEERR_OK)
    {
        this->WriteConsole(wxT("\nPlease upgrade to firmware 2.05 or higher to access full USB functionality."));
        return NULL;
    }

    // Load the ROM
    this->WriteConsole("\nLoading UNFLoader Example 4 via USB");
    this->SetClientDeviceStatus(CLSTATUS_UPLOADING);
    fp = fopen(device_getrom(), "r");
    if (fp == NULL)
    {
        this->WriteConsole(wxT("\nFailed to open ROM file."));
        return NULL;
    }

    // Set the CIC for the ROM to be bootable
    device_explicitcic();

    // Upload the ROM
    this->UploadROM(fp);
    while (oldprogress < 100.0f)
    {
        float curprog = device_getuploadprogress();
        if (curprog != oldprogress)
        {
            oldprogress = curprog;
            this->SetUploadProgress(device_getuploadprogress());
        }
    }
    this->SetClientDeviceStatus(CLSTATUS_UPLOADDONE);

    // Finish
    fclose(fp);
    this->WriteConsole("\nFinished uploading.");
    if (device_getcart() != CART_EVERDRIVE)
        this->WriteConsole("\nYou may now boot the console.");
    return NULL;
}

void DeviceThread::ClearConsole()
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_CLEARCONSOLE);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::WriteConsole(wxString str)
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLE);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::SetClientDeviceStatus(ClientDeviceStatus status)
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_SETSTATUS);
    evt.SetExtraLong(status);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::SetUploadProgress(int progress)
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_UPLOADPROGRESS);
    evt.SetExtraLong(progress);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::UploadROM(FILE* fp)
{
    UploadThread* dev = new UploadThread(this->m_Window, fp);
    if (dev->Create() != wxTHREAD_NO_ERROR)
    {
        delete dev;
    }
    else if (dev->Run() != wxTHREAD_NO_ERROR)
    {
        delete dev;
    }
}


/*=============================================================

=============================================================*/

UploadThread::UploadThread(ClientWindow* win, FILE* fp)
{
    this->m_Window = win;
    this->m_File = fp;
}

void* UploadThread::Entry()
{
    int filesize;
    DeviceError deverr;

    // Get the file size
    fseek(this->m_File, 0L, SEEK_END);
    filesize = ftell(this->m_File);
    rewind(this->m_File);

    // Upload the ROM
    deverr = device_sendrom(this->m_File, filesize);
    if (deverr != DEVICEERR_OK)
    {
        this->WriteConsole(wxString::Format(wxT("\nError sending ROM. Returned error %d."), deverr));
        return NULL;
    }
    return NULL;
}

void UploadThread::WriteConsole(wxString str)
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLE);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}