#include "romdownloader.h"
#include "helper.h"
#include "packets.h"

#define BUFFER_SIZE  8192

typedef enum {
    TEVENT_SETPROGRESS,
    TEVENT_THREADENDED,
} ThreadEventType;

ROMDownloadWindow::ROMDownloadWindow(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxDialog(parent, id, title, pos, size, style)
{
    this->m_DownloadThread = NULL;
    this->SetReturnCode(0);
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    this->m_Sizer_Main = new wxFlexGridSizer(0, 1, 0, 0);
    this->m_Sizer_Main->AddGrowableCol(0);
    this->m_Sizer_Main->AddGrowableRow(0);
    this->m_Sizer_Main->SetFlexibleDirection(wxBOTH);
    this->m_Sizer_Main->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    this->m_Text_Download = new wxStaticText(this, wxID_ANY, wxT("Downloading ROM from Master Server..."), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Text_Download->Wrap(-1);
    this->m_Sizer_Main->Add(this->m_Text_Download, 0, wxALIGN_CENTER|wxALL, 5);

    this->m_Gauge_Download = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(-1,-1), wxGA_HORIZONTAL);
    this->m_Gauge_Download->SetValue(0);
    this->m_Gauge_Download->SetMinSize(wxSize(-1, 20));
    this->m_Sizer_Main->Add(this->m_Gauge_Download, 0, wxALL|wxEXPAND, 5);

    this->m_Button_Cancel = new wxButton(this, wxID_ANY, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Sizer_Main->Add(this->m_Button_Cancel, 0, wxALIGN_RIGHT|wxALL, 5);

    this->SetSizer(this->m_Sizer_Main);
    this->Layout();

    this->Centre(wxBOTH);

    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ROMDownloadWindow::ThreadEvent));
    this->m_Button_Cancel->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ROMDownloadWindow::m_Button_CancelOnButtonClick), NULL, this);
}

ROMDownloadWindow::~ROMDownloadWindow()
{
    // Deallocate the thread. Use a CriticalSection to access it
    {
        wxCriticalSectionLocker enter(this->m_DownloadThreadCS);

        if (this->m_DownloadThread != NULL)
            this->m_DownloadThread->Delete();
    }

    // Disconnect events
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ROMDownloadWindow::ThreadEvent));
    this->m_Button_Cancel->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ROMDownloadWindow::m_Button_CancelOnButtonClick), NULL, this);
}

void ROMDownloadWindow::ThreadEvent(wxThreadEvent& event)
{
    switch ((ThreadEventType)event.GetInt())
    {
        case TEVENT_THREADENDED:
        {
            this->EndModal(event.GetPayload<bool>());
            this->m_DownloadThread = NULL;
            break;
        }
        case TEVENT_SETPROGRESS:
            this->m_Gauge_Download->SetValue(event.GetPayload<int>());
            this->Layout();
            break;
    }
}

void ROMDownloadWindow::m_Button_CancelOnButtonClick(wxCommandEvent& event)
{
    (void)event;
    // Stop the thread if it is running
    {
        wxCriticalSectionLocker enter(this->m_DownloadThreadCS);

        if (this->m_DownloadThread != NULL)
            this->m_DownloadThread->Delete();
    }
}

void ROMDownloadWindow::SetROMPath(wxString path)
{
    int w, h;
    this->m_ROMPath = path;
    this->m_Text_Download->SetLabel(wxString("Downloading '") + path + wxString("' from master server..."));
    this->GetSize(&w, &h);
    this->m_Text_Download->Wrap(w);
    this->Layout();
}

void ROMDownloadWindow::SetROMHash(wxString hash)
{
    size_t i;
    hash = hash.Lower();
    for (i=0; i<hash.length(); i+=2)
    {
        char first = hash[i];
        char second = hash[i+1];
        if (first >= 'a' && first <= 'f')
            first = 10 + first - 'a';
        else
            first = first - '0';
        if (second >= 'a' && second <= 'f')
            second = 10 + second - 'a';
        else
            second = second - '0';
        this->m_ROMHash[i/2] = (first << 4) | second;
    }
}

void ROMDownloadWindow::SetAddress(wxString address)
{
    this->m_MasterAddress = address;
}

void ROMDownloadWindow::SetPort(int port)
{
    this->m_MasterPort = port;
}

wxString ROMDownloadWindow::GetROMPath()
{
    return this->m_ROMPath;
}

wxString ROMDownloadWindow::GetAddress()
{
    return this->m_MasterAddress;
}

int ROMDownloadWindow::GetPort()
{
    return this->m_MasterPort;
}

uint8_t* ROMDownloadWindow::GetROMHash()
{
    return this->m_ROMHash;
}

void ROMDownloadWindow::BeginConnection()
{
    this->m_DownloadThread = new ROMDownloadThread(this);
    if (this->m_DownloadThread->Run() != wxTHREAD_NO_ERROR)
        delete this->m_DownloadThread;
}


/*=============================================================

=============================================================*/

ROMDownloadThread::ROMDownloadThread(ROMDownloadWindow* win)
{
    this->m_Window = win;
    this->m_Socket = NULL;
    this->m_File = NULL;
    this->m_Success = false;
}

ROMDownloadThread::~ROMDownloadThread()
{
    this->NotifyMainOfDeath();
    if (this->m_File != NULL)
        fclose(this->m_File);
    if (!this->m_Success && wxFileExists(this->m_Window->GetROMPath()))
        wxRemoveFile(this->m_Window->GetROMPath());
    if (this->m_Socket != NULL)
        delete this->m_Socket;
}

void* ROMDownloadThread::Entry()
{
    S64Packet* pkt;
    wxIPV4address addr;
    addr.Hostname(this->m_Window->GetAddress());
    addr.Service(this->m_Window->GetPort());
    int filesize, bytesleft;
    uint8_t* buff;
    char out[36];

    this->m_Socket = new wxSocketClient(wxSOCKET_BLOCK | wxSOCKET_WAITALL);
    this->m_Socket->SetTimeout(10);
    this->m_Socket->Connect(addr);
    if (!this->m_Socket->IsConnected())
    {
        this->m_Socket->Close();
        printf("Socket failed to connect.\n");
        return NULL;
    }

    // Send the header
    printf("Connected to master server successfully!\n");
    filesize = swap_endian32(32);
    memcpy(out, &filesize, 4);
    memcpy(out+4, this->m_Window->GetROMHash(), 32);
    pkt = new S64Packet("DOWNLOAD", 36, out);
    pkt->SendPacket(this->m_Socket);
    delete pkt;
    printf("Requested ROM download\n");

    // Read the file size
    this->m_Socket->Read(&filesize, 4);
    filesize = swap_endian32(filesize);
    if (this->m_Socket->LastError() != wxSOCKET_NOERROR)
    {
        printf("Socket threw error %d\n", this->m_Socket->LastError());
        return NULL;
    }
    if (filesize == 0)
    {
        printf("ROM unavailable from Master Server\n");
        return NULL;
    }

    // Open the file
    this->m_File = fopen(this->m_Window->GetROMPath(), "wb+");

    // Allocate memory for the buffer
    buff = (uint8_t*)malloc(BUFFER_SIZE);
    if (buff == NULL)
    {
        printf("Malloc error\n");
        return NULL;
    }

    // Download it in chunks
    bytesleft = filesize;
    while (!TestDestroy())
    {
        if (this->m_Socket->IsData())
        {
            int readcount = BUFFER_SIZE;
            if (readcount > bytesleft)
                readcount = bytesleft; 
            
            // Read bytes into the file
            this->m_Socket->Read(buff, readcount);
            if (this->m_Socket->LastError() != wxSOCKET_NOERROR)
            {
                printf("Socket threw error %d\n", this->m_Socket->LastError());
                free(buff);
                fclose(this->m_File);
                this->m_File = NULL;
                return NULL;
            }
            fwrite(buff, 1, readcount, this->m_File);

            // Decrease the number of bytes left, and break out of the loop if we're done
            bytesleft -= readcount;
            this->UpdateDownloadProgress(100 - ((float)bytesleft/filesize)*100);
            if (bytesleft <= 0)
                break;
        }
    }

    // Done downloading, now cleanup
    free(buff);
    fclose(this->m_File);
    this->m_File = NULL;
    if (bytesleft <= 0)
        this->m_Success = true;
    return NULL;
}

void ROMDownloadThread::UpdateDownloadProgress(int progress)
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_SETPROGRESS);
    evt.SetPayload<int>(progress);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void ROMDownloadThread::NotifyMainOfDeath()
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_THREADENDED);
    evt.SetPayload<bool>(this->m_Success);
    wxQueueEvent(this->m_Window, evt.Clone());
}