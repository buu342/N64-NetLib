#include "romdownloader.h"
#include "helper.h"
#include "packets.h"


/******************************
             Types
******************************/

typedef enum {
    TEVENT_SETPROGRESS,
} ThreadEventType;


/*==============================
    ROMDownloadWindow (Constructor)
    Initializes the class
    @param The parent window
    @param The window ID to use
    @param The title of the window
    @param The position of the window on the screen
    @param The size of the window
    @param Style flags
==============================*/

ROMDownloadWindow::ROMDownloadWindow(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
    this->SetReturnCode(0);
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    // Initialize the main window sizer
    this->m_Sizer_Main = new wxFlexGridSizer(0, 1, 0, 0);
    this->m_Sizer_Main->AddGrowableCol(0);
    this->m_Sizer_Main->AddGrowableRow(0);
    this->m_Sizer_Main->SetFlexibleDirection(wxBOTH);
    this->m_Sizer_Main->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Put some text in the center so that the user knows what's up
    this->m_Text_Download = new wxStaticText(this, wxID_ANY, wxT("Downloading ROM from Master Server..."), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Text_Download->Wrap(-1);
    this->m_Sizer_Main->Add(this->m_Text_Download, 0, wxALIGN_CENTER|wxALL, 5);

    // Place a gauge that shows the download progress
    this->m_Gauge_Download = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(-1,-1), wxGA_HORIZONTAL);
    this->m_Gauge_Download->SetValue(0);
    this->m_Gauge_Download->SetMinSize(wxSize(-1, 20));
    this->m_Sizer_Main->Add(this->m_Gauge_Download, 0, wxALL|wxEXPAND, 5);

    // Button for the user to cancel the download
    this->m_Button_Cancel = new wxButton(this, wxID_ANY, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Sizer_Main->Add(this->m_Button_Cancel, 0, wxALIGN_RIGHT|wxALL, 5);

    // Finalize the layout
    this->SetSizer(this->m_Sizer_Main);
    this->Layout();
    this->Centre(wxBOTH);

    // Connect events
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ROMDownloadWindow::ThreadEvent));
    this->m_Button_Cancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ROMDownloadWindow::m_Button_CancelOnButtonClick), NULL, this);
}


/*==============================
    ROMDownloadWindow (Destructor)
    Cleans up the class before deletion
==============================*/

ROMDownloadWindow::~ROMDownloadWindow()
{
    // Disconnect events
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ROMDownloadWindow::ThreadEvent));
    this->m_Button_Cancel->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ROMDownloadWindow::m_Button_CancelOnButtonClick), NULL, this);
}


/*==============================
    ROMDownloadWindow::m_Button_CancelOnButtonClick
    Event handler for cancel button clicking
    @param The command event
==============================*/

void ROMDownloadWindow::m_Button_CancelOnButtonClick(wxCommandEvent& event)
{
    (void)event;
    this->EndModal(0);
}


/*==============================
    ROMDownloadWindow::ThreadEvent
    Handles messages from the various threads
    @param The thread event
==============================*/

void ROMDownloadWindow::ThreadEvent(wxThreadEvent& event)
{
    switch ((ThreadEventType)event.GetInt())
    {
        case TEVENT_SETPROGRESS:
        {
            int prog = event.GetPayload<int>();
            this->m_Gauge_Download->SetValue(prog);
            this->Layout();
            if (prog == 100)
                this->EndModal(1);
            break;
        }
    }
}


/*==============================
    ROMDownloadWindow::UpdateDownloadProgress
    Sends a upload progress event to the main thread for drawing the gauge
    @param The upload progress (from 0 to 100) 
==============================*/

void ROMDownloadWindow::UpdateDownloadProgress(int progress)
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_SETPROGRESS);
    evt.SetPayload<int>(progress);
    wxQueueEvent(this, evt.Clone());
}