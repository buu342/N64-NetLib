#include <wx/event.h>
#include <wx/msgqueue.h>
#include "clientwindow.h"
#include "Include/device.h"

// Max supported protocol versions
#define USBPROTOCOL_VERSION PROTOCOL_VERSION2
#define HEARTBEAT_VERSION   1

#define DATATYPE_NETPACKET  (USBDataType)0x27 

typedef enum {
    TEVENT_CLEARCONSOLE,
    TEVENT_WRITECONSOLE,
    TEVENT_WRITECONSOLEERROR,
    TEVENT_SETSTATUS,
    TEVENT_UPLOADPROGRESS,
    TEVENT_DEATH_DEVICE,
    TEVENT_DEATH_SERVER,
    TEVENT_NETPACKET_USB_TO_SERVER,
    TEVENT_NETPACKET_SERVER_TO_USB,
} ThreadEventType;

wxMessageQueue<NetLibPacket*> global_msgqueue_usbthread;
wxMessageQueue<NetLibPacket*> global_msgqueue_serverthread;

ClientWindow::ClientWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
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

    this->Centre(wxBOTH);
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ClientWindow::ThreadEvent));
}

ClientWindow::~ClientWindow()
{
    {
        wxCriticalSectionLocker enter(this->m_DeviceThreadCS);
        if (this->m_DeviceThread != NULL)
        {
            this->m_DeviceThread->SetMainWindow(NULL);
            this->m_DeviceThread->Delete();
        }
    }
    {
        wxCriticalSectionLocker enter(this->m_ServerThreadCS);
        if (this->m_ServerThread != NULL)
        {
            this->m_ServerThread->SetMainWindow(NULL);
            this->m_ServerThread->Delete();
        }
    }
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ClientWindow::ThreadEvent));
}

void ClientWindow::BeginWorking()
{
    this->m_DeviceThread = new DeviceThread(this);
    if (this->m_DeviceThread->Create() != wxTHREAD_NO_ERROR)
    {
        delete this->m_DeviceThread;
        this->m_DeviceThread = NULL;
    }
    else if (this->m_DeviceThread->Run() != wxTHREAD_NO_ERROR)
    {
        delete this->m_DeviceThread;
        this->m_DeviceThread = NULL;
    }

    if (this->GetAddress() != "")
    {
        this->m_ServerThread = new ServerConnectionThread(this);
        if (this->m_ServerThread->Create() != wxTHREAD_NO_ERROR)
        {
            delete this->m_ServerThread;
            this->m_ServerThread = NULL;
        }
        else if (this->m_ServerThread->Run() != wxTHREAD_NO_ERROR)
        {
            delete this->m_ServerThread;
            this->m_ServerThread = NULL;
        }
    }
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
        default:
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
            this->m_RichText_Console->ShowPosition(this->m_RichText_Console->GetLastPosition());
            break;
        case TEVENT_WRITECONSOLEERROR:
            this->m_RichText_Console->BeginTextColour(wxColour(255, 0, 0));
            this->m_RichText_Console->WriteText(event.GetString());
            this->m_RichText_Console->EndTextColour();
            this->m_RichText_Console->ShowPosition(this->m_RichText_Console->GetLastPosition());
            break;
        case TEVENT_SETSTATUS:
            this->SetClientDeviceStatus((ClientDeviceStatus)event.GetExtraLong());
            break;
        case TEVENT_UPLOADPROGRESS:
            {
                int prog = event.GetExtraLong();
                this->m_Gauge_Upload->SetValue(prog);
            }
            break;
        case TEVENT_DEATH_DEVICE:
            {
                wxCriticalSectionLocker enter(this->m_DeviceThreadCS);
                this->m_DeviceThread = NULL;
            }
            break;
        case TEVENT_DEATH_SERVER:
            {
                wxCriticalSectionLocker enter(this->m_ServerThreadCS);
                this->m_ServerThread = NULL;
            }
            break;
        case TEVENT_NETPACKET_USB_TO_SERVER:
            {
                NetLibPacket* pkt = event.GetPayload<NetLibPacket*>();
                if (this->m_ServerThread == NULL)
                    delete pkt;
                else
                    global_msgqueue_serverthread.Post(pkt);
            }
            break;
        case TEVENT_NETPACKET_SERVER_TO_USB:
            {
                NetLibPacket* pkt = event.GetPayload<NetLibPacket*>();
                if (this->m_DeviceThread == NULL)
                    delete pkt;
                else
                    global_msgqueue_usbthread.Post(pkt);
            }
            break;
    }
}

void ClientWindow::SetROM(wxString rom)
{
    this->m_ROMPath = rom;
}


void ClientWindow::SetAddress(wxString addr)
{
    this->m_ServerAddress = addr;
}

void ClientWindow::SetPortNumber(int port)
{
    this->m_ServerPort = port;
}

wxString ClientWindow::GetROM()
{
    return this->m_ROMPath;
}

wxString ClientWindow::GetAddress()
{
    return this->m_ServerAddress;
}

int ClientWindow::GetPort()
{
    return this->m_ServerPort;
}


/*=============================================================

=============================================================*/

DeviceThread::DeviceThread(ClientWindow* win)
{
    this->m_Window = win;
    global_msgqueue_usbthread.Clear();
    device_initialize();
}

DeviceThread::~DeviceThread()
{
    if (device_isopen())
        device_close();
    this->NotifyDeath();
}

void* DeviceThread::Entry()
{
    float oldprogress = 0;
    DeviceError deverr = device_find();
    wxString rompath = this->m_Window->GetROM();

    // Check which flashcart we found
    this->ClearConsole();
    this->WriteConsole(wxT("Searching for a valid flashcart"));
    if (deverr != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format(wxT("\nError finding flashcart. Returned error %d."), deverr));
        this->NotifyDeath();
        return NULL;
    }
    this->WriteConsole(wxT("\nFound "));
    switch (device_getcart())
    {
        case CART_64DRIVE1: this->WriteConsole(wxT("64Drive HW1")); break;
        case CART_64DRIVE2: this->WriteConsole(wxT("64Drive HW2")); break;
        case CART_EVERDRIVE: this->WriteConsole(wxT("EverDrive")); break;
        case CART_SC64: this->WriteConsole(wxT("SummerCart64")); break;
        case CART_NONE: 
            this->WriteConsole(wxT("Unknown"));
            this->WriteConsoleError("USB Disconnected.\n");
            this->NotifyDeath();
            return NULL;
    }

    // Open the cart
    this->WriteConsole("\nOpening device");
    if (device_open() != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format(wxT("\nError opening flashcart. Returned error %d."), deverr));
        this->NotifyDeath();
        return NULL;
    }

    // Test that debug mode is possible
    if (device_testdebug() != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxT("\nPlease upgrade to firmware 2.05 or higher to access full USB functionality."));
        this->NotifyDeath();
        return NULL;
    }

    // Load the ROM
    if (rompath != "" && wxFileExists(rompath))
    {
        this->WriteConsole("\nLoading '" + rompath + "' via USB");
        this->SetClientDeviceStatus(CLSTATUS_UPLOADING);

        // Upload the ROM
        this->UploadROM(rompath);
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

        // TODO: Handle TestDestroy() properly

        // Finished uploading
        this->WriteConsole("\nFinished uploading.");
        if (device_getcart() != CART_EVERDRIVE)
            this->WriteConsole("\nYou may now boot the console.");
    }
    else
    {
        // If no ROM was uploaded, assume async, and switch to latest protocol
        device_setprotocol(USBPROTOCOL_LATEST);
    }

    // Now just read from USB in a loop
    this->WriteConsole("\nUSB polling begun\n");
    while (!TestDestroy() && device_isopen())
    {
        uint8_t* outbuff = NULL;
        uint32_t dataheader = 0;
        if (device_receivedata(&dataheader, &outbuff) != DEVICEERR_OK)
        {
            this->WriteConsoleError("\nError receiving data from the flashcart.");
            break;
        }
        if (dataheader != 0 && outbuff != NULL)
        {
            uint32_t size = dataheader & 0xFFFFFF;
            USBDataType command = (USBDataType)((dataheader >> 24) & 0xFF);

            // Decide what to do with the data based off the command type
            switch ((int)command)
            {
                case DATATYPE_TEXT: this->ParseUSB_TextPacket(outbuff, size); break;
                case DATATYPE_NETPACKET: this->ParseUSB_NetLibPacket(outbuff); break;
                case DATATYPE_HEARTBEAT: this->ParseUSB_HeartbeatPacket(outbuff, size); break;
                default:
                    this->WriteConsoleError(wxString::Format("\nError: Received unknown datatype '%02X' from the flashcart.", command));
                    break;
            }

            // Cleanup
            free(outbuff);
            outbuff = NULL;
        }
        else // No incoming USB data, that means we can send data safely
        {
            NetLibPacket* pkt;
            if (global_msgqueue_usbthread.ReceiveTimeout(0, pkt) == wxMSGQUEUE_NO_ERROR)
            {
                char* pktasbytes = pkt->GetAsBytes();
                device_senddata(DATATYPE_NETPACKET, (byte*)pkt->GetAsBytes(), (uint32_t)pkt->GetAsBytes_Size());
                free(pktasbytes);
                delete pkt;
            }
        }
    }
    this->WriteConsoleError("USB Disconnected.\n");
    this->NotifyDeath();
    return NULL;
}

void DeviceThread::ParseUSB_TextPacket(uint8_t* buff, uint32_t size)
{
    char* text;
    static bool firstprint = TRUE;
    text = (char*)malloc(size+1);
    if (text == NULL)
    {
        this->WriteConsoleError("\nError: Unable to allocate memory for incoming string.");
        return;
    }
    memset(text, 0, size+1);
    strncpy(text, (char*)buff, size);
    if (firstprint)
    {
        firstprint = false;
        this->ClearConsole();
    }
    this->WriteConsole(wxString(text));
    free(text);
}

void DeviceThread::ParseUSB_NetLibPacket(uint8_t* buff)
{
    NetLibPacket* pkt = NetLibPacket::FromBytes((char*)buff);
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_NETPACKET_USB_TO_SERVER);
    evt.SetPayload<NetLibPacket*>(pkt);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::ParseUSB_HeartbeatPacket(uint8_t* buff, uint32_t size)
{
    uint32_t header;
    uint16_t heartbeat_version;
    uint16_t protocol_version;
    if (size < 4)
    {
        this->WriteConsoleError("\nError: Malformed heartbeat received.");
        return;
    }

    // Read the heartbeat header
    header = (buff[3] << 24) | (buff[2] << 16) | (buff[1] << 8) | (buff[0]);
    header = swap_endian(header);
    heartbeat_version = (uint16_t)(header&0x0000FFFF);
    protocol_version = (header&0xFFFF0000)>>16;

    // Ensure we support this protocol version
    if (protocol_version > USBPROTOCOL_VERSION)
    {
        this->WriteConsoleError(wxString::Format("\nError: USB protocol %d unsupported. Your NetLib Client is probably out of date.", protocol_version));
        return;
    }
    device_setprotocol((ProtocolVer)protocol_version);

    // Handle the heartbeat by reading more stuff based on the version
    // Currently, nothing here.
    if (heartbeat_version != 0x01)
    {
        this->WriteConsoleError(wxString::Format("\nError: Heartbeat version %d unsupported. Your NetLib Client is probably out of date.", heartbeat_version));
        return;
    }
}

void DeviceThread::ClearConsole()
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_CLEARCONSOLE);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::WriteConsole(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLE);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::WriteConsoleError(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLEERROR);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::SetClientDeviceStatus(ClientDeviceStatus status)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_SETSTATUS);
    evt.SetExtraLong(status);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::SetUploadProgress(int progress)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_UPLOADPROGRESS);
    evt.SetExtraLong(progress);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void DeviceThread::SetMainWindow(ClientWindow* win)
{
    this->m_Window = win;
}

void DeviceThread::UploadROM(wxString path)
{
    UploadThread* dev = new UploadThread(this->m_Window, path);
    if (dev->Create() != wxTHREAD_NO_ERROR)
    {
        delete dev;
    }
    else if (dev->Run() != wxTHREAD_NO_ERROR)
    {
        delete dev;
    }
}

void DeviceThread::NotifyDeath()
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_DEATH_DEVICE);
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*=============================================================

=============================================================*/

UploadThread::UploadThread(ClientWindow* win, wxString path)
{
    this->m_Window = win;
    this->m_FilePath = path;
}

UploadThread::~UploadThread()
{

}

void* UploadThread::Entry()
{
    FILE* fp;
    int filesize;
    DeviceError deverr;
    char* romstr = (char*)malloc(this->m_FilePath.Length() + 1);
    strcpy(romstr, this->m_FilePath.mb_str());

    // Open the file
    device_setrom(romstr);
    fp = fopen(romstr, "rb");
    if (fp == NULL)
    {
        free(romstr);
        return NULL;
    }

    // Set the CIC for the ROM to be bootable
    device_explicitcic();

    // Get the file size
    fseek(fp, 0L, SEEK_END);
    filesize = ftell(fp);
    rewind(fp);

    // Upload the ROM
    deverr = device_sendrom(fp, filesize);
    if (deverr != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format(wxT("\nError sending ROM. Returned error %d."), deverr));
        fclose(fp);
        free(romstr);
        return NULL;
    }
    fclose(fp);
    free(romstr);
    return NULL;
}

void UploadThread::WriteConsole(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLE);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}

void UploadThread::WriteConsoleError(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLEERROR);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*=============================================================

=============================================================*/

ServerConnectionThread::ServerConnectionThread(ClientWindow* win)
{
    this->m_Window = win;
    global_msgqueue_serverthread.Clear();
}

ServerConnectionThread::~ServerConnectionThread()
{
    if (this->m_Socket->IsConnected())
        this->m_Socket->Close();
    this->NotifyDeath();
}

void* ServerConnectionThread::Entry()
{
    wxIPV4address addr;
    addr.Hostname(this->m_Window->GetAddress());
    addr.Service(this->m_Window->GetPort());

    // Attempt to connect the socket
    this->m_Socket = new wxSocketClient(wxSOCKET_BLOCK | wxSOCKET_WAITALL);
    this->m_Socket->SetTimeout(10);
    this->m_Socket->Connect(addr);
    if (!this->m_Socket->IsConnected())
    {
        this->m_Socket->Close();
        this->WriteConsoleError("Server socket failed to connect.\n");
        this->NotifyDeath();
        return NULL;
    }
    this->WriteConsole("Socket connected!\n");
    // TODO: Figure out why console writing isn't working here

    // Relay packets 
    while (!TestDestroy() && this->m_Socket->IsConnected())
    {
        NetLibPacket* pkt = NULL;
        if (global_msgqueue_serverthread.ReceiveTimeout(0, pkt) == wxMSGQUEUE_NO_ERROR)
        {
            pkt->SendPacket(this->m_Socket);
            delete pkt;
        }
        else
        {
            if (this->m_Socket->IsData())
                pkt = NetLibPacket::ReadPacket(this->m_Socket);
            if (pkt != NULL)
                this->TransferPacket(pkt);
            else if (this->m_Socket->LastCount() == 0)
                wxMilliSleep(10);
        }
    }
    this->WriteConsoleError("Server Disconnected.\n");
    this->NotifyDeath();
    return NULL;
}

void ServerConnectionThread::SetMainWindow(ClientWindow* win)
{
    this->m_Window = win;
}

void ServerConnectionThread::TransferPacket(NetLibPacket* pkt)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_NETPACKET_SERVER_TO_USB);
    evt.SetPayload<NetLibPacket*>(pkt);
    wxQueueEvent(this->m_Window, evt.Clone());
}

void ServerConnectionThread::WriteConsole(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLE);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}

void ServerConnectionThread::WriteConsoleError(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLEERROR);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}

void ServerConnectionThread::NotifyDeath()
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_DEATH_SERVER);
    wxQueueEvent(this->m_Window, evt.Clone());
}