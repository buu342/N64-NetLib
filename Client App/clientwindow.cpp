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

static wxMessageQueue<NetLibPacket*> global_msgqueue_usbthread;
static wxMessageQueue<NetLibPacket*> global_msgqueue_serverthread;

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
    this->m_RichText_Console->Clear();

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
    if (this->m_DeviceThread->Run() != wxTHREAD_NO_ERROR)
    {
        delete this->m_DeviceThread;
        this->m_DeviceThread = NULL;
    }

    if (this->GetAddress() != "")
    {
        this->m_ServerThread = new ServerConnectionThread(this);
        if (this->m_ServerThread->Run() != wxTHREAD_NO_ERROR)
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

void ClientWindow::SetSocket(wxDatagramSocket* socket)
{
    this->m_Socket = socket;
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

wxDatagramSocket* ClientWindow::GetSocket()
{
    return this->m_Socket;
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
    this->m_FirstPrint = TRUE;
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
    this->WriteConsole("Searching for a valid flashcart\n");
    if (deverr != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format("Error finding flashcart. Returned error %d.\n", deverr));
        this->NotifyDeath();
        return NULL;
    }
    this->WriteConsole("Found ");
    switch (device_getcart())
    {
        case CART_64DRIVE1: this->WriteConsole("64Drive HW1\n"); break;
        case CART_64DRIVE2: this->WriteConsole("64Drive HW2\n"); break;
        case CART_EVERDRIVE: this->WriteConsole("EverDrive\n"); break;
        case CART_SC64: this->WriteConsole("SummerCart64\n"); break;
        case CART_NONE: 
            this->WriteConsole("Unknown\n");
            this->WriteConsoleError("USB Disconnected.\n");
            this->NotifyDeath();
            return NULL;
    }

    // Open the cart
    this->WriteConsole("Opening device\n");
    if (device_open() != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format("Error opening flashcart. Returned error %d.\n", deverr));
        this->NotifyDeath();
        return NULL;
    }

    // Test that debug mode is possible
    if (device_testdebug() != DEVICEERR_OK)
    {
        this->WriteConsoleError("Please upgrade to firmware 2.05 or higher to access full USB functionality.\n");
        this->NotifyDeath();
        return NULL;
    }

    // Load the ROM
    if (rompath != "" && wxFileExists(rompath))
    {
        this->WriteConsole("Loading '" + rompath + "' via USB\n");
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
        this->WriteConsole("Finished uploading.\n");
        if (device_getcart() != CART_EVERDRIVE)
            this->WriteConsole("You may now boot the console.\n");
    }
    else
    {
        // If no ROM was uploaded, assume async, and switch to latest protocol
        device_setprotocol(USBPROTOCOL_LATEST);
    }

    // Now just read from USB in a loop
    this->WriteConsole("USB polling begun\n");
    while (!TestDestroy() && device_isopen())
    {
        uint8_t* outbuff = NULL;
        uint32_t dataheader = 0;
        if (device_receivedata(&dataheader, &outbuff) != DEVICEERR_OK)
        {
            this->WriteConsoleError("\nError receiving data from the flashcart.\n");
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
                    this->WriteConsoleError(wxString::Format("\nError: Received unknown datatype '%02X' from the flashcart.\n", command));
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
                uint8_t* pktasbytes = pkt->GetAsBytes();
                device_senddata(DATATYPE_NETPACKET, (byte*)pkt->GetAsBytes(), (uint32_t)pkt->GetAsBytes_Size());
                free(pktasbytes);
                delete pkt;
            }
            else
                wxMilliSleep(10);
        }
    }
    this->WriteConsoleError("\nUSB Disconnected.\n");
    this->NotifyDeath();
    return NULL;
}

void DeviceThread::ParseUSB_TextPacket(uint8_t* buff, uint32_t size)
{
    char* text;
    text = (char*)malloc(size+1);
    if (text == NULL)
    {
        this->WriteConsoleError("\nError: Unable to allocate memory for incoming string.\n");
        return;
    }
    memset(text, 0, size+1);
    strncpy(text, (char*)buff, size);
    if (this->m_FirstPrint)
    {
        this->m_FirstPrint = false;
        this->ClearConsole();
    }
    this->WriteConsole(wxString(text));
    free(text);
}

void DeviceThread::ParseUSB_NetLibPacket(uint8_t* buff)
{
    NetLibPacket* pkt = NetLibPacket::FromBytes((uint8_t*)buff);
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    if (pkt == NULL)
    {
        this->WriteConsoleError("\nGot a bad NetLib Packet\n");
        return;
    }
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
        this->WriteConsoleError("\nError: Malformed heartbeat received.\n");
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
        this->WriteConsoleError(wxString::Format("\nError: USB protocol %d unsupported. Your NetLib Client is probably out of date.\n", protocol_version));
        return;
    }
    device_setprotocol((ProtocolVer)protocol_version);

    // Handle the heartbeat by reading more stuff based on the version
    // Currently, nothing here.
    if (heartbeat_version != 0x01)
    {
        this->WriteConsoleError(wxString::Format("\nError: Heartbeat version %d unsupported. Your NetLib Client is probably out of date.\n", heartbeat_version));
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
    if (dev->Run() != wxTHREAD_NO_ERROR)
        delete dev;
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
        this->WriteConsoleError(wxString::Format("\nError sending ROM. Returned error %d.\n", deverr));
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
    this->NotifyDeath();
}

void* ServerConnectionThread::Entry()
{
    uint8_t* buff = (uint8_t*)malloc(4096);
    wxDatagramSocket* socket = this->m_Window->GetSocket();
    UDPHandler* handler = new UDPHandler(socket, this->m_Window->GetAddress(), this->m_Window->GetPort());

    // Handle packets
    this->WriteConsole("Establishing connection to server once ROM is ready.\n");
    while (!TestDestroy())
    {
        try
        {
            NetLibPacket* pkt = NULL;

            // Check for messages from the main thread (which are relayed from USB)
            while (global_msgqueue_serverthread.ReceiveTimeout(0, pkt) == wxMSGQUEUE_NO_ERROR)
                handler->SendPacket(pkt);

            // Check for packets from the server and upload it to USB
            socket->Read(buff, 4096);
            while (socket->LastReadCount() > 0)
            {
                pkt = handler->ReadNetLibPacket(buff);
                if (pkt != NULL)
                    this->TransferPacket(pkt);
                socket->Read(buff, 4096);
            }
            wxMilliSleep(10);
        }
        catch (ClientTimeoutException& e)
        {
            (void)e;
            this->WriteConsoleError("\nServer timed out. Disconnected.\n");
            break;
        }
    }
    delete handler;
    free(buff);
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
    evt.SetPayload<NetLibPacket*>(NetLibPacket::FromBytes(pkt->GetAsBytes()));
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