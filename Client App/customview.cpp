/***************************************************************
                         customview.cpp

A custom wxDataViewListCtrl that supports markup in text entries
***************************************************************/

#include "customview.h"


/*==============================
    CustomDataView::AppendCustomTextColumn
    Initializes the class
    @param  The label to add
    @param  The cell mode
    @param  The width of the column
    @param  The alignment of the text
    @param  Style flags
    @return The column that text was added to
==============================*/

wxDataViewColumn* CustomDataView::AppendCustomTextColumn(const wxString &label, wxDataViewCellMode mode, int width, wxAlignment align, int flags)
{
    wxDataViewTextRenderer* renderer = new wxDataViewTextRenderer(wxT("string"), mode);
    renderer->EnableMarkup();
    GetStore()->AppendColumn(wxT("string"));
    wxDataViewColumn *ret = new wxDataViewColumn(label, renderer, GetColumnCount(), width, align, flags);
    wxDataViewCtrl::AppendColumn(ret);
    return ret;
}