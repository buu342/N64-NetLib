#pragma once

typedef struct IUnknown IUnknown;

#include <wx/wx.h>
#include "serverbrowser.h"


/*********************************
             Classes
*********************************/

class App : public wxApp
{
	private:
		ServerBrowser* m_Frame = nullptr;
        
	protected:
    
	public:
		App();
		~App();
		virtual bool OnInit();
};