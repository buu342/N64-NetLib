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
#include <wx/socket.h>
#include <stdint.h>
#include "packets.h"


/******************************
             Types
******************************/

typedef enum {
    CLSTATUS_STARTED = 0,
    CLSTATUS_FOUNDCART,
    CLSTATUS_UPLOADING,
    CLSTATUS_UPLOADDONE,
    CLSTATUS_RUNNING,
    CLSTATUS_STOPPED,
} ClientDeviceStatus;

class ClientWindow;
class DeviceThread;
class ServerConnectionThread;
class UploadThread;


/*********************************
             Classes
*********************************/

class ClientWindow : public wxFrame
{
    private:
        wxDatagramSocket* m_Socket;
        wxString m_ServerAddress;
        int m_ServerPort;
        wxGridBagSizer* m_Sizer_Input;
        wxRichTextCtrl* m_RichText_Console;
        wxTextCtrl* m_TextCtrl_Input;
        wxButton* m_Button_Send;
        wxButton* m_Button_Reconnect;
        wxGauge* m_Gauge_Upload;
        wxStatusBar* m_StatusBar_ClientStatus;
        wxString m_ROMPath;
        DeviceThread* m_DeviceThread;
        ClientDeviceStatus m_DeviceStatus;
        ServerConnectionThread* m_ServerThread;

        void ThreadEvent(wxThreadEvent& event);
        void StartThread_Device(bool startnow);
        void StartThread_Server(bool startnow);
        void StopThread_Device(bool nullwindow);
        void StopThread_Server(bool nullwindow);
        void SetClientDeviceStatus(ClientDeviceStatus status);

    protected:

    public:
        ClientWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 640,480 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
        ~ClientWindow();
        void m_Button_Send_OnButtonClick(wxCommandEvent& event);
        void m_Button_Reconnect_OnButtonClick(wxCommandEvent& event);

        void BeginWorking();
        void SetROM(wxString rom);
        void SetSocket(wxDatagramSocket* socket);
        void SetAddress(wxString addr);
        void SetPortNumber(int port);
        wxDatagramSocket* GetSocket();
        wxString GetROM();
        wxString GetAddress();
        int GetPort();
};

class DeviceThread : public wxThread
{
    private:
        ClientWindow* m_Window;
        UploadThread* m_UploadThread;
        bool m_FirstPrint;

        void HandleMainInput();
        void ParseUSB_TextPacket(uint8_t* buff, uint32_t size);
        void ParseUSB_NetLibPacket(uint8_t* buff);
        void ParseUSB_HeartbeatPacket(uint8_t* buff, uint32_t size);
        void ClearConsole();
        void WriteConsole(wxString str);
        void WriteConsoleError(wxString str);
        void SetClientDeviceStatus(ClientDeviceStatus status);
        void SetUploadProgress(int progress);
        void UploadROM(wxString path);
        void NotifyDeath();

    protected:

    public:
        DeviceThread(ClientWindow* win);
        ~DeviceThread();

        virtual void* Entry() wxOVERRIDE;
};

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

class ServerConnectionThread : public wxThread
{
    private:
        wxDatagramSocket* m_Socket;
        ClientWindow* m_Window;

        void HandleMainInput();
        void TransferPacket(NetLibPacket* pkt);
        void WriteConsole(wxString str);
        void WriteConsoleError(wxString str);
        void NotifyDeath();

    protected:

    public:
        ServerConnectionThread(ClientWindow* win);
        ~ServerConnectionThread();

        virtual void* Entry() wxOVERRIDE;
};