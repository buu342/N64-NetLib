#pragma once

#include <stdint.h>
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/gauge.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/dialog.h>
#include <wx/socket.h>

class ROMDownloadThread;

class ROMDownloadWindow : public wxDialog
{
    private:

    protected:
        wxFlexGridSizer* m_Sizer_Main;
        wxStaticText* m_Text_Download;
        wxGauge* m_Gauge_Download;
        wxButton* m_Button_Cancel;
        wxString m_ROMPath;
        uint8_t m_ROMHash[32];
        wxString m_MasterAddress;
        int m_MasterPort;
        ROMDownloadThread* m_DownloadThread;
        wxCriticalSection m_DownloadThreadCS;

    public:
        ROMDownloadWindow(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE);
        ~ROMDownloadWindow();
        void m_Button_CancelOnButtonClick(wxCommandEvent& event);
        void ThreadEvent(wxThreadEvent& event);
        void SetROMPath(wxString path);
        void SetROMHash(wxString hash);
        void SetAddress(wxString address);
        void SetPort(int port);
        wxString GetROMPath();
        wxString GetAddress();
        uint8_t* GetROMHash();
        int GetPort();
        void BeginConnection();
};

class ROMDownloadThread : public wxThread
{
    private:
        wxSocketClient* m_Socket;
        ROMDownloadWindow* m_Window;
        FILE* m_File;
        bool m_Success;

    protected:

    public:
        ROMDownloadThread(ROMDownloadWindow* win);
        ~ROMDownloadThread();

        virtual void* Entry() wxOVERRIDE;
        void OnSocketEvent(wxSocketEvent& event);
        void UpdateDownloadProgress(int progress);
        void NotifyMainOfDeath();
};