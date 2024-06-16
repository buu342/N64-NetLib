#pragma once

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

typedef enum {
    CLSTATUS_STARTED = 0,
    CLSTATUS_FOUNDCART,
    CLSTATUS_UPLOADING,
    CLSTATUS_UPLOADDONE,
    CLSTATUS_RUNNING,
    CLSTATUS_STOPPED,
} ClientDeviceStatus;

class ClientWindow : public wxFrame
{
    private:

    protected:
        wxGridBagSizer* m_Sizer_Input;
        wxRichTextCtrl* m_RichText_Console;
        wxTextCtrl* m_TextCtrl_Input;
        wxButton* m_Button_Send;
        wxGauge* m_Gauge_Upload;
        wxStatusBar* m_StatusBar_ClientStatus;
        ClientDeviceStatus m_DeviceStatus;

        void ThreadEvent(wxThreadEvent& event);

    public:
        ClientWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 640,480 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL|wxSTAY_ON_TOP );
        ~ClientWindow();

        void SetClientDeviceStatus(ClientDeviceStatus status);
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
        void ClearConsole();
        void WriteConsole(wxString str);
        void SetClientDeviceStatus(ClientDeviceStatus status);
        void SetUploadProgress(int progress);
};