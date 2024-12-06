#pragma once

typedef struct IUnknown IUnknown;

#include <wx/dataview.h>

class CustomDataView : public wxDataViewListCtrl
{
    public:
        CustomDataView(wxWindow *parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize, long style=wxDV_ROW_LINES, const wxValidator &validator=wxDefaultValidator) : wxDataViewListCtrl(parent, id, pos, size, style, validator) {};
        wxDataViewColumn* AppendCustomTextColumn(const wxString &label, wxDataViewCellMode mode, int width, wxAlignment align, int flags);
};