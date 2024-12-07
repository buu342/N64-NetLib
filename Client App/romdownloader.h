#pragma once

typedef struct IUnknown IUnknown;

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


/******************************
             Types
******************************/

class ROMDownloadThread;


/*********************************
             Classes
*********************************/

class ROMDownloadWindow : public wxDialog
{
    private:
        wxFlexGridSizer* m_Sizer_Main;
        wxStaticText* m_Text_Download;
        wxGauge* m_Gauge_Download;
        wxButton* m_Button_Cancel;

        void ThreadEvent(wxThreadEvent& event);

    protected:

    public:
        ROMDownloadWindow(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE);
        ~ROMDownloadWindow();
        void m_Button_CancelOnButtonClick(wxCommandEvent& event);
        void UpdateDownloadProgress(int progress);
};