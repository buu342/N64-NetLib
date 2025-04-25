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

#define STOPONERROR  0


/******************************
             Types
******************************/

typedef enum {
    TEVENT_CLEARCONSOLE,
    TEVENT_WRITECONSOLE,
    TEVENT_WRITECONSOLEERROR,
    TEVENT_SETSTATUS,
    TEVENT_UPLOADPROGRESS,

    // User Input events
    TEVENT_LOADROM,
    TEVENT_CANCELLOAD,
    TEVENT_LOADDATA,
} ThreadEventType;

typedef struct {
    ThreadEventType type;
    char* data;
} InputMessage;


/******************************
            Globals
******************************/

static wxMessageQueue<NetLibPacket*> global_msgqueue_usbthread_pkt;
static wxMessageQueue<NetLibPacket*> global_msgqueue_serverthread_pkt;
static wxMessageQueue<InputMessage*> global_msgqueue_usbthread_input;


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
    this->m_DeviceStatus = CLSTATUS_DEAD;
    this->m_Button_Send = new wxButton(this, wxID_ANY, wxT("Reupload"), wxDefaultPosition, wxDefaultSize, 0);
    this->m_Sizer_Input->Add(this->m_Button_Send, wxGBPosition(0, 1), wxGBSpan(1, 1), wxALL, 5);

    // Configure the sizers
    this->m_Sizer_Input->AddGrowableCol(0);
    m_Sizer_Main->Add(this->m_Sizer_Input, 1, wxEXPAND, 5);

    // Layout the items
    this->SetSizer(m_Sizer_Main);
    this->Layout();

    // Status bar (currently unused, would be nice to display statistics)
    //m_StatusBar_ClientStatus = this->CreateStatusBar(1, wxSTB_SIZEGRIP, wxID_ANY);

    // Finalize positioning
    this->Centre(wxBOTH);
    this->Connect(wxID_ANY, wxEVT_THREAD, wxThreadEventHandler(ClientWindow::ThreadEvent));

    // Connect events
    this->m_TextCtrl_Input->Connect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(ClientWindow::m_TextCtrl_Input_OnText), NULL, this);
    this->m_Button_Send->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ClientWindow::m_Button_Send_OnButtonClick), NULL, this);
}


/*==============================
    ClientWindow (Destructor)
    Cleans up the class before deletion
==============================*/

ClientWindow::~ClientWindow()
{
    // Kill threads
    this->StopThread_Server(); // Kill server thread first otherwise USB will keep receiving messages and sending them
    this->StopThread_Device();

    // Disconnect events
    this->m_Button_Send->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ClientWindow::m_Button_Send_OnButtonClick), NULL, this);
    this->m_TextCtrl_Input->Disconnect(wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(ClientWindow::m_TextCtrl_Input_OnText), NULL, this);

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
    this->StartThread_Device();
    this->StartThread_Server();
    this->LoadROM();
}


/*==============================
    ClientWindow::LoadROM
    Sends the to USB thread a ROM load message
==============================*/

void ClientWindow::LoadROM()
{
    InputMessage* usrinput = new InputMessage{TEVENT_LOADROM, NULL};
    usrinput->data = (char*)malloc(this->m_ROMPath.Length()+1);
    strcpy(usrinput->data, this->m_ROMPath.mb_str());
    global_msgqueue_usbthread_input.Post(usrinput);
}


/*==============================
    ClientWindow::LoadData
    Sends the to USB thread a data load message
==============================*/

void ClientWindow::LoadData()
{
    InputMessage* usrinput = new InputMessage{ TEVENT_LOADDATA, NULL };
    usrinput->data = (char*)malloc(this->m_TextCtrl_Input->GetValue().Length() + 1);
    strcpy(usrinput->data, this->m_TextCtrl_Input->GetValue().mb_str());
    global_msgqueue_usbthread_input.Post(usrinput);
    this->m_TextCtrl_Input->SetValue("");
}


/*==============================
    ClientWindow::StartThread_Device
    Starts the USB device thread
==============================*/

void ClientWindow::StartThread_Device()
{
    // If the thread is stopped but not null, then free it before creating another
    if (this->m_DeviceThread != NULL && !this->m_DeviceThread->IsAlive())
    {
        this->m_DeviceThread->Delete();
        this->m_DeviceThread = NULL;
    }

    // Spawn another device thread if there is none
    if (this->m_DeviceThread == NULL)
    {
        this->m_DeviceThread = new DeviceThread(this);
        if (this->m_DeviceThread->Run() != wxTHREAD_NO_ERROR)
        {
            delete this->m_DeviceThread;
            this->m_DeviceThread = NULL;
        }
    }
}


/*==============================
    ClientWindow::StartThread_Server
    Starts the server socket thread
==============================*/

void ClientWindow::StartThread_Server()
{
    if (this->m_ServerThread == NULL && this->GetAddress() != "")
    {
        this->m_ServerThread = new ServerConnectionThread(this);
        if (this->m_ServerThread->Run() != wxTHREAD_NO_ERROR)
        {
            delete this->m_ServerThread;
            this->m_ServerThread = NULL;
        }
    }
}


/*==============================
    ClientWindow::StopThread_Device
    Stops the USB device thread
==============================*/

void ClientWindow::StopThread_Device()
{
    if (this->m_DeviceThread != NULL)
    {
        this->m_DeviceThread->Delete();
        delete this->m_DeviceThread;
        this->m_DeviceThread = NULL;
    }
}


/*==============================
    ClientWindow::StopThread_Server
    Stops the server socket thread
==============================*/

void ClientWindow::StopThread_Server()
{
    if (this->m_ServerThread != NULL)
    {
        this->m_ServerThread->Delete();
        delete this->m_ServerThread;
        this->m_ServerThread = NULL;
    }
}


/*==============================
    ClientWindow::m_Button_Send_OnButtonClick
    Event handler for Send/Reupload button clicking
    @param The command event
==============================*/

void ClientWindow::m_Button_Send_OnButtonClick(wxCommandEvent& event)
{
    // Start the device thread if it's dead for some reason
    this->StartThread_Device();

    // Handle the input
    switch (this->m_DeviceStatus)
    {
        case CLSTATUS_UPLOADING:
            global_msgqueue_usbthread_input.Post(new InputMessage{TEVENT_CANCELLOAD, NULL});
            break;
        case CLSTATUS_DEAD:
            this->LoadROM();
            break;
        case CLSTATUS_IDLE:
            if (this->m_TextCtrl_Input->GetValue() == "")
                this->LoadROM();
            else
                this->LoadData();
            break;
        default: break;
    }

    // Unused parameter
    (void)event;
}


/*==============================
    ClientWindow::m_TextCtrl_Input_OnText
    Event handler for TextCtrl text changing
    @param The command event
==============================*/

void ClientWindow::m_TextCtrl_Input_OnText(wxCommandEvent& event)
{
    if (this->m_TextCtrl_Input->GetValue() == "")
        this->m_Button_Send->SetLabel(wxT("Reupload"));
    else
        this->m_Button_Send->SetLabel(wxT("Send"));
}


/*==============================
    ClientWindow::SetClientDeviceStatus
    Sets the device status so that the send button can do different things
    @param The client device status
==============================*/

void ClientWindow::SetClientDeviceStatus(ClientDeviceStatus status)
{
    this->m_DeviceStatus = status;
    switch (status)
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
            this->Layout();
            this->Refresh();
            break;
        case CLSTATUS_DEAD:
            if (!this->m_TextCtrl_Input->IsShown())
            {
                this->m_Gauge_Upload->SetValue(0);
                this->m_Gauge_Upload->Disable();
                this->m_Gauge_Upload->Hide();
                this->m_Sizer_Input->Detach(this->m_Gauge_Upload);
                this->m_Sizer_Input->Add(this->m_TextCtrl_Input, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxEXPAND, 5);
                this->m_TextCtrl_Input->Show();
            }
            this->m_TextCtrl_Input->Disable();
            this->m_TextCtrl_Input->SetValue("");
            this->m_Button_Send->SetLabel(wxT("Reupload"));
            this->Layout();
            this->Refresh();
            break;
        case CLSTATUS_IDLE:
            if (!this->m_TextCtrl_Input->IsShown())
            {
                this->m_Gauge_Upload->SetValue(0);
                this->m_Gauge_Upload->Disable();
                this->m_Gauge_Upload->Hide();
                this->m_Sizer_Input->Detach(this->m_Gauge_Upload);
                this->m_Sizer_Input->Add(this->m_TextCtrl_Input, wxGBPosition(0, 0), wxGBSpan(1, 1), wxALL | wxEXPAND, 5);
                this->m_TextCtrl_Input->Show();
                this->m_TextCtrl_Input->Enable();
            }
            this->m_Button_Send->SetLabel(wxT("Reupload"));
            this->Layout();
            this->Refresh();
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

DeviceThread::DeviceThread(ClientWindow* win) : wxThread(wxTHREAD_JOINABLE)
{
    this->m_Window = win;
    this->m_UploadThread = NULL;
    this->m_FirstPrint = true;
    global_msgqueue_usbthread_pkt.Clear();
    device_initialize();
    this->SetClientDeviceStatus(CLSTATUS_IDLE);
}


/*==============================
    DeviceThread (Destructor)
    Cleans up the class before deletion
==============================*/

DeviceThread::~DeviceThread()
{
    if (device_isopen())
        device_close();
}


/*==============================
    DeviceThread::Entry
    The entry function for the thread
    @return The exit code
==============================*/

void* DeviceThread::Entry()
{
    wxString rompath = "";
    DeviceError deverr = device_find();

    // Search for a cart
    this->WriteConsole("Searching for a valid flashcart\n");
    if (deverr != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format("Error finding flashcart. Returned error %d.\n", deverr));
        this->SetClientDeviceStatus(CLSTATUS_DEAD);
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
            this->SetClientDeviceStatus(CLSTATUS_DEAD);
            return NULL;
    }

    // Open the cart
    this->WriteConsole("Opening device\n");
    if (device_open() != DEVICEERR_OK)
    {
        this->WriteConsoleError(wxString::Format("Error opening flashcart. Returned error %d.\n", deverr));
        this->SetClientDeviceStatus(CLSTATUS_DEAD);
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
        this->SetClientDeviceStatus(CLSTATUS_DEAD);
        return NULL;
    }
    device_setprotocol(USBPROTOCOL_LATEST);

    // Handle USB communication in a loop
    while (!TestDestroy() && device_isopen())
    {
        uint8_t* outbuff = NULL;
        uint32_t dataheader = 0;

        // Handle ROM uploading
        if (rompath != "")
        {
            this->WriteConsole("Loading '" + rompath + "' via USB\n");
            this->SetClientDeviceStatus(CLSTATUS_UPLOADING);

            // Create upload thread and update the status bar as it's uploading
            this->UploadROM(rompath);
            if (this->m_UploadThread != NULL)
            {
                bool cancelled = false;
                float oldprogress = 0;
                while (oldprogress < 100.0f)
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
                    if (TestDestroy() || this->HandleMainInput(&rompath))
                    {
                        device_cancelupload();
                        this->WriteConsoleError("\nUpload interrupted.\n");
                        this->m_FirstPrint = true;
                        cancelled = true;
                        break;
                    }
                }
                rompath = "";
                this->m_UploadThread->Delete(); // TODO: 64Drive upload can get stuck, causing this thread join to freeze the server browser
                delete this->m_UploadThread;
                this->m_UploadThread = NULL;
                this->SetClientDeviceStatus(CLSTATUS_IDLE);

                // Finished uploading
                if (!cancelled)
                {
                    this->WriteConsole("Finished uploading.\n");
                    if (device_getcart() != CART_EVERDRIVE)
                        this->WriteConsole("You may now boot the console.\n");
                    rompath = "";
                    wxMilliSleep(100); // Prevent the next receivedata call from reading any extra data from the upload process
                }
            }
        }

        // Read from the N64's USB, and ensure there were no errors during the USB reading process
        if (device_receivedata(&dataheader, &outbuff) != DEVICEERR_OK)
        {
            this->WriteConsoleError("\nError receiving data from the flashcart.\n");
            #if STOPONERROR
            break;
            #endif
        }

        // If we have a valid USB data header
        if (dataheader != 0 && outbuff != NULL)
        {
            uint32_t size = dataheader & 0xFFFFFF;
            USBDataType command = (USBDataType)((dataheader >> 24) & 0xFF);

            // Decide what to do with the data based off the command type
            switch ((int)command)
            {
                case DATATYPE_TEXT:      this->ParseUSB_TextPacket(outbuff, size); break;
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
        else // No incoming USB data, that means we can send a data packet to the N64 safely
        {
            bool workdone = false;
            NetLibPacket* pkt;
            if (global_msgqueue_usbthread_pkt.ReceiveTimeout(0, pkt) == wxMSGQUEUE_NO_ERROR)
            {
                uint8_t* pktasbytes = pkt->GetAsBytes();
                device_senddata(DATATYPE_NETPACKET, (byte*)pktasbytes, (uint32_t)pkt->GetAsBytes_Size());
                workdone = true;
                free(pktasbytes);
            }
            
            // Rest if there was nothing done
            if (!workdone)
                wxMilliSleep(1);
        }

        // Check for messages from the main thread
        this->HandleMainInput(&rompath);
    }

    // Finished
    this->WriteConsoleError("\nUSB Disconnected.\n");
    this->SetClientDeviceStatus(CLSTATUS_DEAD);
    return NULL;
}


/*==============================
    DeviceThread::HandleMainInput
    Handle input from the main thread
    @param  A pointer to the ROM path string
    @return Whether to stop the thread
==============================*/

bool DeviceThread::HandleMainInput(wxString* rompath)
{
    bool kill = false;
    InputMessage* usrinput;
    while (global_msgqueue_usbthread_input.ReceiveTimeout(0, usrinput) == wxMSGQUEUE_NO_ERROR)
    {
        switch (usrinput->type)
        {
            case TEVENT_LOADROM:
                *rompath = wxString(usrinput->data);
                break;
            case TEVENT_CANCELLOAD:
                kill = true;
                break;
            case TEVENT_LOADDATA:
                // TODO
                break;
            default:
                break;
        }
        free(usrinput->data);
        delete usrinput;
    }
    return kill;
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

    // Send the packet to the networking thread
    global_msgqueue_serverthread_pkt.Post(pkt);
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
    {
        delete this->m_UploadThread;
        this->m_UploadThread = NULL;
    }
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

UploadThread::UploadThread(ClientWindow* win, wxString path) : wxThread(wxTHREAD_JOINABLE)
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

ServerConnectionThread::ServerConnectionThread(ClientWindow* win) : wxThread(wxTHREAD_JOINABLE)
{
    this->m_Window = win;
    global_msgqueue_serverthread_pkt.Clear(); // TODO: Manually empty the queue to prevent leaks?
}


/*==============================
    ServerConnectionThread (Destructor)
    Cleans up the class before deletion
==============================*/

ServerConnectionThread::~ServerConnectionThread()
{
    // Does nothing
}


/*==============================
    ServerConnectionThread::Entry
    The entry function for the thread
    @return The exit code
==============================*/

void* ServerConnectionThread::Entry()
{
    uint8_t* buff = (uint8_t*)malloc(4096);
    ASIOSocket* socket = new ASIOSocket(this->m_Window->GetAddress(), this->m_Window->GetPort());
    UDPHandler* handler = new UDPHandler(socket, this->m_Window->GetAddress(), this->m_Window->GetPort());

    // Handle packets coming from the server
    this->WriteConsole("Establishing connection to server once ROM is ready.\n");
    while (!TestDestroy())
    {
        try
        {
            bool workdone = false;
            NetLibPacket* pkt = NULL;

            // Check for messages from the USB thread to send to the server
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
                workdone = true;
            }
            if (!workdone)
                wxMilliSleep(1);
        }
        catch (ClientTimeoutException& e)
        {
            (void)e;
            this->WriteConsoleError("\nServer timed out. Disconnected.\n");
            break;
        }
    }

    // Cleanup
    delete handler;
    free(buff);
    delete socket;
    return NULL;
}


/*==============================
    ServerConnectionThread::TransferPacket
    Transfers a packet from the server to the USB thread
    @param The packet to transfer
==============================*/

void ServerConnectionThread::TransferPacket(NetLibPacket* pkt)
{
    global_msgqueue_usbthread_pkt.Post(NetLibPacket::FromBytes(pkt->GetAsBytes()));
}


/*==============================
    ServerConnectionThread::WriteConsole
    Send a text console write request to the main thread
    @param The text to write
==============================*/

void ServerConnectionThread::WriteConsole(wxString str)
{
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
    wxThreadEvent evt = wxThreadEvent(wxEVT_THREAD, wxID_ANY);
    evt.SetInt(TEVENT_WRITECONSOLEERROR);
    evt.SetString(str.c_str());
    wxQueueEvent(this->m_Window, evt.Clone());
}