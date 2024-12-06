#include "customview.h"

wxDataViewColumn* CustomDataView::AppendCustomTextColumn(const wxString &label, wxDataViewCellMode mode, int width, wxAlignment align, int flags)
{
    wxDataViewTextRenderer* renderer = new wxDataViewTextRenderer(wxT("string"), mode);
    renderer->EnableMarkup();
    GetStore()->AppendColumn(wxT("string"));
    wxDataViewColumn *ret = new wxDataViewColumn(label, renderer, GetColumnCount(), width, align, flags);
    wxDataViewCtrl::AppendColumn(ret);
    return ret;
}