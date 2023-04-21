
// DPC_gui.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CDPCguiApp:
// See DPC_gui.cpp for the implementation of this class
//

class CDPCguiApp : public CWinApp
{
public:
	CDPCguiApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CDPCguiApp theApp;
