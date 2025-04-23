#pragma once

typedef struct IUnknown IUnknown;

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
#include <stdint.h>
#include "packets.h"


/******************************
             Types
******************************/

typedef enum {
    CLSTATUS_IDLE = 0,
    CLSTATUS_UPLOADING,
    CLSTATUS_DEAD,
} ClientDeviceStatus;


/*********************************
             Classes
*********************************/

// Prototypes
class ClientWindow;
class DeviceThread;
class ServerConnectionThread;
class UploadThread;

// Client window for N64 <-> Server communication
class ClientWindow : public wxFrame
{
    private:
        wxString m_ServerAddress;
        int m_ServerPort;
        wxGridBagSizer* m_Sizer_Input;
        wxRichTextCtrl* m_RichText_Console;
        wxTextCtrl* m_TextCtrl_Input;
        wxButton* m_Button_Send;
        wxGauge* m_Gauge_Upload;
        wxStatusBar* m_StatusBar_ClientStatus;
        wxString m_ROMPath;
        DeviceThread* m_DeviceThread;
        ClientDeviceStatus m_DeviceStatus;
        ServerConnectionThread* m_ServerThread;

        void ThreadEvent(wxThreadEvent& event);
        void StartThread_Device();
        void StartThread_Server();
        void StopThread_Device();
        void StopThread_Server();
        void SetClientDeviceStatus(ClientDeviceStatus status);

    protected:

    public:
        ClientWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 640,480 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
        ~ClientWindow();
        void m_Button_Send_OnButtonClick(wxCommandEvent& event);
        void m_TextCtrl_Input_OnText(wxCommandEvent& event);

        void BeginWorking();
        void LoadROM();
        void LoadData();
        void SetROM(wxString rom);
        void SetAddress(wxString addr);
        void SetPortNumber(int port);
        wxString GetAddress();
        int GetPort();
};

// Thread for handling USB communication
class DeviceThread : public wxThread
{
    private:
        ClientWindow* m_Window;
        UploadThread* m_UploadThread;
        bool m_FirstPrint;

        bool HandleMainInput(wxString* rompath);
        void ParseUSB_TextPacket(uint8_t* buff, uint32_t size);
        void ParseUSB_NetLibPacket(uint8_t* buff);
        void ParseUSB_HeartbeatPacket(uint8_t* buff, uint32_t size);
        void ClearConsole();
        void WriteConsole(wxString str);
        void WriteConsoleError(wxString str);
        void SetClientDeviceStatus(ClientDeviceStatus status);
        void SetUploadProgress(int progress);
        void UploadROM(wxString path);

    protected:

    public:
        DeviceThread(ClientWindow* win);
        ~DeviceThread();

        virtual void* Entry() wxOVERRIDE;
};

// Thread for handling ROM upload
class UploadThread : public wxThread
{
    private:
        ClientWindow* m_Window;
        wxString m_FilePath;

        void WriteConsole(wxString str);
        void WriteConsoleError(wxString str);

    protected:

    public:
        UploadThread(ClientWindow* win, wxString path);
        ~UploadThread();

        virtual void* Entry() wxOVERRIDE;
};

// Thread for handling server communication
class ServerConnectionThread : public wxThread
{
    private:
        udp::socket* m_Socket;
        ClientWindow* m_Window;

        void HandleMainInput();
        void TransferPacket(NetLibPacket* pkt);
        void WriteConsole(wxString str);
        void WriteConsoleError(wxString str);

    protected:

    public:
        ServerConnectionThread(ClientWindow* win);
        ~ServerConnectionThread();

        virtual void* Entry() wxOVERRIDE;
};