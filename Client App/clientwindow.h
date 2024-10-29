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

class ClientWindow : public wxFrame
{
    private:

    protected:
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
        wxCriticalSection m_DeviceThreadCS;
        ServerConnectionThread* m_ServerThread;
        wxCriticalSection m_ServerThreadCS;

        void ThreadEvent(wxThreadEvent& event);

    public:
        ClientWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 640,480 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL|wxSTAY_ON_TOP );
        ~ClientWindow();

        void ConnectServer();
        void SetClientDeviceStatus(ClientDeviceStatus status);
        void SetROM(wxString rom);
        void SetAddress(wxString addr);
        void SetPort(int port);
        wxString GetROM();
        wxString GetAddress();
        int GetPort();
};

class DeviceThread : public wxThread
{
    private:
        ClientWindow* m_Window;

    protected:

    public:
        DeviceThread(ClientWindow* win);
        ~DeviceThread();

        virtual void* Entry() wxOVERRIDE;
        void ParseUSB_TextPacket(uint8_t* buff, uint32_t size);
        void ParseUSB_HeartbeatPacket(uint8_t* buff, uint32_t size);
        void ClearConsole();
        void WriteConsole(wxString str);
        void WriteConsoleError(wxString str);
        void SetClientDeviceStatus(ClientDeviceStatus status);
        void SetUploadProgress(int progress);
        void SetMainWindow(ClientWindow* win);
        void UploadROM(FILE* fp);
        void NotifyDeath();
};

class UploadThread : public wxThread
{
    private:
        ClientWindow* m_Window;
        FILE* m_File;

    protected:

    public:
        UploadThread(ClientWindow* win, FILE* fp);
        ~UploadThread();

        virtual void* Entry() wxOVERRIDE;
        void WriteConsole(wxString str);
        void WriteConsoleError(wxString str);
};

class ServerConnectionThread : public wxThread
{
    private:
        wxSocketClient* m_Socket;
        ClientWindow* m_Window;

    protected:

    public:
        ServerConnectionThread(ClientWindow* win);
        ~ServerConnectionThread();

        virtual void* Entry() wxOVERRIDE;
        void SetMainWindow(ClientWindow* win);
        void WriteConsole(wxString str);
        void WriteConsoleError(wxString str);
        void NotifyDeath();
};