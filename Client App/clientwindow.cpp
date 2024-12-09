/***************************************************************
                        clientwindow.cpp

The client window that handles USB and server communication
***************************************************************/

#include <wx/event.h>
#include <wx/msgqueue.h>
#include "clientwindow.h"
#include "Include/device.h"


/******************************
            Macros
******************************/

// Max supported protocol versions
#define USBPROTOCOL_VERSION PROTOCOL_VERSION2
#define HEARTBEAT_VERSION   1

#define DATATYPE_NETPACKET  (USBDataType)0x27


/******************************
             Types
******************************/

typedef enum {
    TEVENT_CLEARCONSOLE,
    TEVENT_WRITECONSOLE,
    TEVENT_WRITECONSOLEERROR,
    TEVENT_SETSTATUS,
    TEVENT_UPLOADPROGRESS,
    TEVENT_START_DEVICE,
    TEVENT_START_SERVER,
    TEVENT_DEATH_DEVICE,
    TEVENT_DEATH_SERVER,
    TEVENT_NETPACKET_USB_TO_SERVER,
    TEVENT_NETPACKET_SERVER_TO_USB,

    // User Input events
    TEVENT_PROGEND,
    TEVENT_LOADFAIL,
    TEVENT_CANCELLOAD,
} ThreadEventType;


/******************************
            Globals
******************************/

static wxMessageQueue<NetLibPacket*> global_msgqueue_usbthread_pkt;
static wxMessageQueue<NetLibPacket*> global_msgqueue_serverthread_pkt;
static wxMessageQueue<ThreadEventType> global_msgqueue_usbthread_input;
static wxMessageQueue<ThreadEventType> global_msgqueue_serverthread_input;


/*=============================================================
                         Client Window
=============================================================*/

/*==============================
    ClientWindow (Constructor)
    Initializes the class
    @param The parent window
    @param The window ID to use
    @param The title of the window
    @param The position of the window on the screen
    @param The size of the window
    @param Style flags
==============================*/

ClientWindow::ClientWindow(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
    this->m_ServerThread = NULL;
    this->m_DeviceThread = NULL;
    this->SetSizeHints(wxDefaultSize, wxDefaultSize);

    // Initialize the main window sizer
    wxFlexGridSizer* m_Sizer_Main;
    m_Sizer_Main = new wxFlexGridSizer(2, 0, 0, 0);
    m_Sizer_Main->AddGrowableCol(0);
    m_Sizer_Main->AddGrowableRow(0);
    m_Sizer_Main->SetFlexibleDirection(wxBOTH);
    m_Sizer_Main->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Rich console for text output
    this->m_RichText_Console = new wxRichTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxVSCROLL|wxHSCROLL|wxNO_BORDER|wxWANTS_CHARS);
    m_Sizer_Main->Add(this->m_RichText_Console, 1, wxEXPAND | wxALL, 5);
    this->m_RichText_Console->Clear();

    // Sizer for the bottom items
    this->m_Sizer_Input = new wxGridBagSizer(0, 0);
    this->m_Sizer_Input->SetFlexibleDirection(wxBOTH);
    this->m_Sizer_Input->SetNonFlexibleGrowMode(wxFLEX_GROWMODE_SPECIFIED);

    // Text input for USB sending (currently unused)
    this->m_TextCtrl_Input = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    this->m_TextCtrl_Input->Disable();
    this->m_Sizer_Input->Add(this->m_TextCtrl_Input, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL|wxEXPAND, 5);

    // USB upload progress bar
    this->m_Gauge_Upload = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_SMOOTH|wxGA_PROGRESS);
    this->m_Gauge_Upload->SetValue(0);
    this->m_Gauge_Upload->Disable();
    this->m_Gauge_Upload->Hide();

    // Reupload/Send text button
    this->m_Button_Send = new wxButton(this, wxID_ANY, wxT("Reupload"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Sizer_Input->Add(this->m_Button_Send, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 5);

    // Reconnect to server button
    this->m_Button_Reconnect = new wxButton(this, wxID_ANY, wxT("Reconnect"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Sizer_Input->Add(this->m_Button_Reconnect, wxGBPosition(0, 2), wxGBSpan(1, 1), wxALL, 5);

    // Configure the sizers
    this->m_Sizer_Input->AddGrowableCol(0);
    m_Sizer_Main->Add(this->m_Sizer_Input, 1, wxEXPAND, 5);

    // Layout the items
    this->SetSizer(m_Sizer_Main);
    this->Layout();

    // Status bar (currently unused)
    //m_StatusBar_ClientStatus = this->CreateStatusBar(1, wxSTB_SIZEGRIP, wxID_ANY);

    // Finalize positioning
    this->Centre(wxBOTH);
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ClientWindow::ThreadEvent));

    // Connect events
    this->m_Button_Reconnect->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ClientWindow::m_Button_Reconnect_OnButtonClick), NULL, this);
    this->m_Button_Send->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ClientWindow::m_Button_Send_OnButtonClick), NULL, this);
}


/*==============================
    ClientWindow (Destructor)
    Cleans up the class before deletion
==============================*/

ClientWindow::~ClientWindow()
{
    // Disconnect events
    this->m_Button_Reconnect->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ClientWindow::m_Button_Reconnect_OnButtonClick), NULL, this);
    this->m_Button_Send->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ClientWindow::m_Button_Send_OnButtonClick), NULL, this);

    // Kill threads
    this->StopThread_Device(true);
    this->StopThread_Server(true);

    // Safe to disconnect the thread handler event
    this->Disconnect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ClientWindow::ThreadEvent));
}


/*==============================
    ClientWindow::BeginWorking
    Start the threads for USB and server communication.
    Should be called before ClientWindow->Show()
==============================*/

void ClientWindow::BeginWorking()
{
    this->StartThread_Device(true);
    this->StartThread_Server(true);
}


/*==============================
    ClientWindow::StartThread_Device
    Starts the USB device thread
    @param Whether to start the thread right now (if possible)
==============================*/

void ClientWindow::StartThread_Device(bool startnow)
{
    if (startnow && this->m_DeviceThread == NULL)
    {
        this->m_DeviceThread = new DeviceThread(this);
        if (this->m_DeviceThread->Run() != wxTHREAD_NO_ERROR)
        {
            delete this->m_DeviceThread;
            this->m_DeviceThread = NULL;
        }
    }
    else
    {
        wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
        evt.SetInt(TEVENT_START_DEVICE);
        if (this->m_DeviceThread != NULL)
            wxMilliSleep(100);
        wxQueueEvent(this, evt.Clone());
    }
}


/*==============================
    ClientWindow::StartThread_Server
    Starts the server socket thread
    @param Whether to start the thread right now (if possible)
==============================*/

void ClientWindow::StartThread_Server(bool startnow)
{
    if (startnow && this->m_ServerThread == NULL)
    {
        this->m_ServerThread = new ServerConnectionThread(this);
        if (this->m_ServerThread->Run() != wxTHREAD_NO_ERROR)
        {
            delete this->m_ServerThread;
            this->m_ServerThread = NULL;
        }
        else
            this->m_Button_Reconnect->Disable();
    }
    else
    {
        wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
        evt.SetInt(TEVENT_START_SERVER);
        if (this->m_ServerThread != NULL)
            wxMilliSleep(100);
        wxQueueEvent(this, evt.Clone());
    }
}


/*==============================
    ClientWindow::StopThread_Device
    Stops the USB device thread
    @param Whether to nullify the client window pointer
           Only necessary during this object's destruction
==============================*/

void ClientWindow::StopThread_Device(bool nullwindow)
{
    if (this->m_DeviceThread != NULL)
    {
        if (nullwindow)
            global_msgqueue_usbthread_input.Post(TEVENT_PROGEND);
        this->m_DeviceThread->Delete();
    }
}


/*==============================
    ClientWindow::StopThread_Server
    Stops the server socket thread
    @param Whether to nullify the client window pointer
           Only necessary during this object's destruction
==============================*/

void ClientWindow::StopThread_Server(bool nullwindow)
{
    if (this->m_ServerThread != NULL)
    {
        if (nullwindow)
            global_msgqueue_serverthread_input.Post(TEVENT_PROGEND);
        this->m_ServerThread->Delete();
    }
}


/*==============================
    ClientWindow::m_Button_Send_OnButtonClick
    Event handler for Send/Reupload button clicking
    @param The command event
==============================*/

void ClientWindow::m_Button_Send_OnButtonClick(wxCommandEvent& event)
{
    if (this->m_Button_Send->GetLabel() == "Reupload")
    {
        this->StopThread_Device(false);
        this->StartThread_Device(false);
    }
    else if (this->m_Button_Send->GetLabel() == "Cancel")
        global_msgqueue_usbthread_input.Post(TEVENT_CANCELLOAD);

    // Unused parameter
    (void)event;
}


/*==============================
    ClientWindow::m_Button_Reconnect_OnButtonClick
    Event handler for reconnect button clicking
    @param The command event
==============================*/

void ClientWindow::m_Button_Reconnect_OnButtonClick(wxCommandEvent& event)
{
    this->StartThread_Server(false);

    // Unused parameter
    (void)event;
}


/*==============================
    ClientWindow::SetClientDeviceStatus
    Sets the device status so that the send button can do different things
    @param The client device status
==============================*/

void ClientWindow::SetClientDeviceStatus(ClientDeviceStatus status)
{
    this->m_DeviceStatus = status;
    switch(status)
    {
        case CLSTATUS_UPLOADING:
            if (this->m_TextCtrl_Input->IsShown())
            {
                this->m_TextCtrl_Input->Hide();
                this->m_TextCtrl_Input->Disable();
                this->m_Sizer_Input->Detach(this->m_TextCtrl_Input);
                this->m_Sizer_Input->Add(this->m_Gauge_Upload, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL|wxEXPAND, 5);
                this->m_Gauge_Upload->Enable();
                this->m_Gauge_Upload->Show();
            }
            this->m_Button_Send->SetLabel(wxT("Cancel"));
            //this->m_Button_Send->Enable();
            this->Layout();
            this->Refresh();
            break;
        case CLSTATUS_UPLOADDONE:
            this->m_Button_Send->SetLabel(wxT("Reupload"));
            //this->m_Button_Send->Enable();
            break;
        default:
            break;
    }
}


/*==============================
    ClientWindow::ThreadEvent
    Handles messages from the various threads
    @param The thread event
==============================*/

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
        case TEVENT_START_DEVICE:
            this->StartThread_Device(this->m_DeviceThread == NULL);
            break;
        case TEVENT_DEATH_DEVICE:
            this->m_DeviceThread = NULL;
            break;
        case TEVENT_START_SERVER:
            if (this->GetAddress() != "")
                this->StartThread_Server(this->m_DeviceThread == NULL);
            break;
        case TEVENT_DEATH_SERVER:
            this->m_ServerThread = NULL;
            this->m_Button_Reconnect->Enable();
            break;
        case TEVENT_NETPACKET_USB_TO_SERVER:
            {
                NetLibPacket* pkt = event.GetPayload<NetLibPacket*>();
                if (this->m_ServerThread == NULL)
                    delete pkt;
                else
                    global_msgqueue_serverthread_pkt.Post(pkt);
            }
            break;
        case TEVENT_NETPACKET_SERVER_TO_USB:
            {
                NetLibPacket* pkt = event.GetPayload<NetLibPacket*>();
                if (this->m_DeviceThread == NULL)
                    delete pkt;
                else
                    global_msgqueue_usbthread_pkt.Post(pkt);
            }
            break;
        default:
            break;
    }
}


/*==============================
    ClientWindow::SetROM
    Sets the path to the ROM to upload
    @param The path to the ROM
==============================*/

void ClientWindow::SetROM(wxString rom)
{
    this->m_ROMPath = rom;
}


/*==============================
    ClientWindow::SetSocket
    Sets the socket to use for networking
    @param The socket to use
==============================*/

void ClientWindow::SetSocket(wxDatagramSocket* socket)
{
    this->m_Socket = socket;
}


/*==============================
    ClientWindow::SetAddress
    Sets the server address to connect to
    @param The address to connect to
==============================*/

void ClientWindow::SetAddress(wxString addr)
{
    this->m_ServerAddress = addr;
}


/*==============================
    ClientWindow::SetPortNumber
    Sets the server port to use
    @param The address to use
==============================*/

void ClientWindow::SetPortNumber(int port)
{
    this->m_ServerPort = port;
}


/*==============================
    ClientWindow::GetROM
    Retreives the path to the ROM to upload
    @return The path to the ROM to upload
==============================*/

wxString ClientWindow::GetROM()
{
    return this->m_ROMPath;
}


/*==============================
    ClientWindow::GetSocket
    Retreives the socket to use for networking
    @return The socket to use
==============================*/

wxDatagramSocket* ClientWindow::GetSocket()
{
    return this->m_Socket;
}


/*==============================
    ClientWindow::GetAddress
    Retreives the server address to connect to
    @return The server address to connect to
==============================*/

wxString ClientWindow::GetAddress()
{
    return this->m_ServerAddress;
}


/*==============================
    ClientWindow::GetPort
    Retreives the server port to use
    @return The server port to use
==============================*/

int ClientWindow::GetPort()
{
    return this->m_ServerPort;
}


/*=============================================================
                      USB Handler Thread
=============================================================*/

/*==============================
    DeviceThread (Constructor)
    Initializes the class
    @param The parent window
==============================*/

DeviceThread::DeviceThread(ClientWindow* win)
{
    this->m_Window = win;
    this->m_FirstPrint = TRUE;
    this->m_UploadThread = NULL;
    global_msgqueue_usbthread_pkt.Clear();
    device_initialize();
}


/*==============================
    DeviceThread (Destructor)
    Cleans up the class before deletion
==============================*/

DeviceThread::~DeviceThread()
{
    if (device_isopen())
        device_close();
    this->NotifyDeath();
}


/*==============================
    DeviceThread::Entry
    The entry function for the thread
    @return The exit code
==============================*/

void* DeviceThread::Entry()
{
    float oldprogress = 0;
    DeviceError deverr = device_find();
    wxString rompath = this->m_Window->GetROM();

    // Search for a cart
    this->WriteConsole("Searching for a valid flashcart\n");
    if (deverr != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format("Error finding flashcart. Returned error %d.\n", deverr));
        return NULL;
    }

    // Return which cart was found
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
            return NULL;
    }

    // Open the cart
    this->WriteConsole("Opening device\n");
    if (device_open() != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format("Error opening flashcart. Returned error %d.\n", deverr));
        return NULL;
    }

    // Test that debug mode is possible
    if (device_testdebug() != DEVICEERR_OK)
    {
        switch (device_getcart())
        {
            case CART_64DRIVE1: 
            case CART_64DRIVE2: 
                this->WriteConsoleError("Please upgrade to firmware 2.05 or higher to access full USB functionality.\n");
                break;
            default:
                this->WriteConsoleError("Unable to debug on this flashcart.\n");
                break;
        }
        return NULL;
    }

    // Load the ROM if it exists
    if (rompath != "" && wxFileExists(rompath))
    {
        this->WriteConsole("Loading '" + rompath + "' via USB\n");
        this->SetClientDeviceStatus(CLSTATUS_UPLOADING);

        // Create upload thread and update the status bar as it's uploading
        this->UploadROM(rompath);
        while (oldprogress < 100.0f && this->m_Window != NULL)
        {
            float curprog = device_getuploadprogress();
            if (curprog != oldprogress)
            {
                oldprogress = curprog;
                this->SetUploadProgress(device_getuploadprogress());
            }

            // Take a small break;
            wxMilliSleep(10);

            // Check for a thread kill request
            if (TestDestroy() || this->HandleMainInput())
            {
                device_cancelupload();
                wxMilliSleep(500);
                this->SetClientDeviceStatus(CLSTATUS_UPLOADDONE);
                this->WriteConsoleError("\nUpload interrupted.\n");
                return NULL;
            }
        }
        this->SetClientDeviceStatus(CLSTATUS_UPLOADDONE);

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
    while (!TestDestroy() && device_isopen() && this->m_Window != NULL)
    {
        uint8_t* outbuff = NULL;
        uint32_t dataheader = 0;

        // Ensure there were no errors during the reading process
        if (device_receivedata(&dataheader, &outbuff) != DEVICEERR_OK)
        {
            this->WriteConsoleError("\nError receiving data from the flashcart.\n");
            break;
        }

        // If we have a valid USB data header
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
        else // No incoming USB data, that means we can send data (packets) to it safely
        {
            NetLibPacket* pkt;
            if (global_msgqueue_usbthread_pkt.ReceiveTimeout(0, pkt) == wxMSGQUEUE_NO_ERROR)
            {
                uint8_t* pktasbytes = pkt->GetAsBytes();
                device_senddata(DATATYPE_NETPACKET, (byte*)pkt->GetAsBytes(), (uint32_t)pkt->GetAsBytes_Size());
                free(pktasbytes);
                delete pkt;
            }
            else
                wxMilliSleep(10);
        }

        // Check for messages from the main thread
        this->HandleMainInput();
    }

    // Finished
    this->WriteConsoleError("\nUSB Disconnected.\n");
    return NULL;
}


/*==============================
    DeviceThread::HandleMainInput
    Handle input from the main thread
    @return Whether to stop the thread
==============================*/

bool DeviceThread::HandleMainInput()
{
    ThreadEventType usrinput;
    while (global_msgqueue_usbthread_input.ReceiveTimeout(0, usrinput) == wxMSGQUEUE_NO_ERROR)
    {
        switch (usrinput)
        {
            case TEVENT_PROGEND:
                this->m_Window = NULL;
                break;
            case TEVENT_CANCELLOAD:
                return true;
            default:
                break;
        }
    }
    return false;
}


/*==============================
    DeviceThread::ParseUSB_TextPacket
    Parses a USB text packet
    @param The raw buffer with text data
    @param The size of the data
==============================*/

void DeviceThread::ParseUSB_TextPacket(uint8_t* buff, uint32_t size)
{
    char* text = (char*)calloc(size+1, sizeof(char));

    // Ensure malloc succeeded
    if (text == NULL)
    {
        this->WriteConsoleError("\nError: Unable to allocate memory for incoming string.\n");
        return;
    }

    // Copy the string over
    memset(text, 0, size+1);
    strncpy(text, (char*)buff, size);

    // If this was the first print of this thread, clear the console of status text 
    if (this->m_FirstPrint)
    {
        this->m_FirstPrint = false;
        this->ClearConsole();
    }

    // Send the text packet to the main thread for displaying, and cleanup
    this->WriteConsole(wxString(text));
    free(text);
}


/*==============================
    DeviceThread::ParseUSB_NetLibPacket
    Parses a NetLib packet and sends it to the main thread (so it can be relayed to the server)
    @param The raw buffer with NetLib packet data
==============================*/

void DeviceThread::ParseUSB_NetLibPacket(uint8_t* buff)
{
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    NetLibPacket* pkt = NetLibPacket::FromBytes((uint8_t*)buff);

    // Ensure we had a valid packet
    if (pkt == NULL)
    {
        this->WriteConsoleError("\nGot a bad NetLib Packet\n");
        return;
    }

    // Send the packet to the main thread so that it can be sent to the networking thread
    evt.SetInt(TEVENT_NETPACKET_USB_TO_SERVER);
    evt.SetPayload<NetLibPacket*>(pkt);
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    DeviceThread::ParseUSB_HeartbeatPacket
    Parses a UNFLoader heartbeat packet
    @param The raw buffer with heartbeat data
    @param The size of the data
==============================*/

void DeviceThread::ParseUSB_HeartbeatPacket(uint8_t* buff, uint32_t size)
{
    uint32_t header;
    uint16_t heartbeat_version;
    uint16_t protocol_version;

    // Heartbeat packet must have at least 4 bytes
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


/*==============================
    DeviceThread::ClearConsole
    Send a text console clear request to the main thread
==============================*/

void DeviceThread::ClearConsole()
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_CLEARCONSOLE);
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    DeviceThread::WriteConsole
    Send a text console write request to the main thread
    @param The text to write
==============================*/

void DeviceThread::WriteConsole(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLE);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    DeviceThread::WriteConsoleError
    Send a text console error write request to the main thread
    @param The error text to write
==============================*/

void DeviceThread::WriteConsoleError(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLEERROR);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    DeviceThread::SetClientDeviceStatus
    Send a client device status update notification to the main thread
    @param The new client status
==============================*/

void DeviceThread::SetClientDeviceStatus(ClientDeviceStatus status)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_SETSTATUS);
    evt.SetExtraLong(status);
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    DeviceThread::SetUploadProgress
    Send a upload progress notification to the main thread so the gauge can be updated
    @param The current progress (from 0 to 100)
==============================*/

void DeviceThread::SetUploadProgress(int progress)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_UPLOADPROGRESS);
    evt.SetExtraLong(progress);
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    DeviceThread::UploadROM
    Create a ROM uploading thread
    @param The path to the ROM to upload
==============================*/

void DeviceThread::UploadROM(wxString path)
{
    this->m_UploadThread = new UploadThread(this->m_Window, path);
    if (this->m_UploadThread->Run() != wxTHREAD_NO_ERROR)
        delete this->m_UploadThread;
}


/*==============================
    DeviceThread::NotifyDeath
    Notify the main thread that this one died so that pointers can be nullified
==============================*/

void DeviceThread::NotifyDeath()
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_DEATH_DEVICE);
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*=============================================================
                       ROM Upload Thread
=============================================================*/

/*==============================
    UploadThread (Constructor)
    Initializes the class
    @param The parent window
    @param The path to the ROM to upload
==============================*/

UploadThread::UploadThread(ClientWindow* win, wxString path)
{
    this->m_Window = win;
    this->m_FilePath = path;
}


/*==============================
    UploadThread (Destructor)
    Cleans up the class before deletion
==============================*/

UploadThread::~UploadThread()
{
    // Does nothing
}


/*==============================
    UploadThread::Entry
    The entry function for the thread
    @return The exit code
==============================*/

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
        if (deverr != DEVICEERR_UPLOADCANCELLED)
            this->WriteConsoleError(wxString::Format("\nError sending ROM. Returned error %d.\n", deverr));
        fclose(fp);
        free(romstr);
        return NULL;
    }

    // Cleanup
    fclose(fp);
    free(romstr);
    return NULL;
}


/*==============================
    UploadThread::WriteConsole
    Send a text console write request to the main thread
    @param The text to write
==============================*/

void UploadThread::WriteConsole(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLE);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    UploadThread::WriteConsoleError
    Send a text console error write request to the main thread
    @param The error text to write
==============================*/

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
                   Server Connection Thread
=============================================================*/

/*==============================
    ServerConnectionThread (Constructor)
    Initializes the class
    @param The parent window
    @param The path to the ROM to upload
==============================*/

ServerConnectionThread::ServerConnectionThread(ClientWindow* win)
{
    this->m_Window = win;
    global_msgqueue_serverthread_pkt.Clear();
}


/*==============================
    ServerConnectionThread (Destructor)
    Cleans up the class before deletion
==============================*/

ServerConnectionThread::~ServerConnectionThread()
{
    this->NotifyDeath();
}


/*==============================
    ServerConnectionThread::Entry
    The entry function for the thread
    @return The exit code
==============================*/

void* ServerConnectionThread::Entry()
{
    uint8_t* buff = (uint8_t*)malloc(4096);
    wxDatagramSocket* socket = this->m_Window->GetSocket();
    UDPHandler* handler = new UDPHandler(socket, this->m_Window->GetAddress(), this->m_Window->GetPort());

    // Handle packets
    this->WriteConsole("Establishing connection to server once ROM is ready.\n");
    while (!TestDestroy() && this->m_Window != NULL)
    {
        try
        {
            NetLibPacket* pkt = NULL;

            // Check for messages from the main thread (which are relayed from USB)
            while (global_msgqueue_serverthread_pkt.ReceiveTimeout(0, pkt) == wxMSGQUEUE_NO_ERROR)
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

        // Check for input messages from the main thread
        this->HandleMainInput();
    }

    // Cleanup
    delete handler;
    free(buff);
    return NULL;
}


/*==============================
    ServerConnectionThread::HandleMainInput
    Handle input from the main thread
==============================*/

void ServerConnectionThread::HandleMainInput()
{
    ThreadEventType usrinput;
    while (global_msgqueue_serverthread_input.ReceiveTimeout(0, usrinput) == wxMSGQUEUE_NO_ERROR)
    {
        switch (usrinput)
        {
            case TEVENT_PROGEND:
                this->m_Window = NULL;
                break;
            default:
                break;
        }
    }
}


/*==============================
    ServerConnectionThread::TransferPacket
    Transfers a packet from the server to the main thread (so it can be relayed to USB)
    @param The packet to transfer
==============================*/

void ServerConnectionThread::TransferPacket(NetLibPacket* pkt)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_NETPACKET_SERVER_TO_USB);
    evt.SetPayload<NetLibPacket*>(NetLibPacket::FromBytes(pkt->GetAsBytes()));
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    ServerConnectionThread::WriteConsole
    Send a text console write request to the main thread
    @param The text to write
==============================*/

void ServerConnectionThread::WriteConsole(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLE);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    ServerConnectionThread::WriteConsoleError
    Send a text console error write request to the main thread
    @param The error text to write
==============================*/

void ServerConnectionThread::WriteConsoleError(wxString str)
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLEERROR);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}


/*==============================
    ServerConnectionThread::NotifyDeath
    Notify the main thread that this one died so that pointers can be nullified
==============================*/

void ServerConnectionThread::NotifyDeath()
{
    if (this->m_Window == NULL)
        return;
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_DEATH_SERVER);
    wxQueueEvent(this->m_Window, evt.Clone());
}