#include "clientwindow.h"
#include "device.h"

ClientWindow::ClientWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
    this->SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxFlexGridSizer* m_Sizer_Main;
    m_Sizer_Main = new wxFlexGridSizer( 2, 0, 0, 0 );
    m_Sizer_Main->AddGrowableCol( 0 );
    m_Sizer_Main->AddGrowableRow( 0 );
    m_Sizer_Main->SetFlexibleDirection( wxBOTH );
    m_Sizer_Main->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_RichText_Console = new wxRichTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0|wxVSCROLL|wxHSCROLL|wxNO_BORDER|wxWANTS_CHARS );
    m_Sizer_Main->Add( m_RichText_Console, 1, wxEXPAND | wxALL, 5 );

    wxGridBagSizer* m_Sizer_Input;
    m_Sizer_Input = new wxGridBagSizer( 0, 0 );
    m_Sizer_Input->SetFlexibleDirection( wxBOTH );
    m_Sizer_Input->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

    m_TextCtrl_Input = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
    m_Sizer_Input->Add( m_TextCtrl_Input, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALL|wxEXPAND, 5 );

    m_Button_Send = new wxButton( this, wxID_ANY, wxT("Send"), wxDefaultPosition, wxDefaultSize, 0 );
    m_Sizer_Input->Add( m_Button_Send, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALL, 5 );


    m_Sizer_Input->AddGrowableCol( 0 );

    m_Sizer_Main->Add( m_Sizer_Input, 1, wxEXPAND, 5 );


    this->SetSizer( m_Sizer_Main );
    this->Layout();
    m_StatusBar_ClientStatus = this->CreateStatusBar( 1, wxSTB_SIZEGRIP, wxID_ANY );

    this->Centre( wxBOTH );

    this->HandleFlashcart();
}

ClientWindow::~ClientWindow()
{

}

void ClientWindow::HandleFlashcart()
{
    DeviceError deverr;
    device_initialize();
    deverr = device_find();
    if (deverr != DEVICEERR_OK)
    {
        this->m_RichText_Console->WriteText(wxString::Format(wxT("Error finding flashcart. Returned error %d\n"), deverr));
        return;
    }
    this->m_RichText_Console->WriteText(wxT("Found "));
    switch (device_getcart())
    {
        case CART_64DRIVE1: this->m_RichText_Console->WriteText(wxT("64Drive HW1")); break;
        case CART_64DRIVE2: this->m_RichText_Console->WriteText(wxT("64Drive HW2")); break;
        case CART_EVERDRIVE: this->m_RichText_Console->WriteText(wxT("EverDrive")); break;
        case CART_SC64: this->m_RichText_Console->WriteText(wxT("SummerCart64")); break;
    }
    this->m_RichText_Console->Newline();
}