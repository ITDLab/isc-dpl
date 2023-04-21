// AdvancedSettingDlg.cpp : 実装ファイル
//

#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <DbgHelp.h>
#include <Shlwapi.h>

#include "DPC_gui.h"
#include "afxdialogex.h"

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "dpl_gui_configuration.h"
#include "isc_dpl.h"

#include "OpenSourceInfoDlg.h"

#include "AdvancedSettingDlg.h"


OpenSourceInfoDlg* opensourecinfo_dlg_ = nullptr;


int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
	case BFFM_INITIALIZED:
		SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
		break;
	case BFFM_SELCHANGED:
		break;
	}
	return 0;
}


BOOL SelectFolder(
	HWND hWnd,
	LPCTSTR lpDefFolder,
	LPTSTR lpSelectPath,
	UINT nFlag,
	LPCTSTR lpTitle)
{
	LPMALLOC pMalloc;
	BOOL bRet = FALSE;

	if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
		BROWSEINFO browsInfo;
		LPITEMIDLIST pIDlist;

		memset(&browsInfo, NULL, sizeof(browsInfo));

		browsInfo.hwndOwner = hWnd;
		browsInfo.pidlRoot = NULL;
		browsInfo.pszDisplayName = lpSelectPath;
		browsInfo.lpszTitle = lpTitle;
		browsInfo.ulFlags = nFlag;
		browsInfo.lpfn = &BrowseCallbackProc;
		browsInfo.lParam = (LPARAM)lpDefFolder;
		browsInfo.iImage = (int)NULL;

		pIDlist = SHBrowseForFolder(&browsInfo);
		if (NULL == pIDlist) {
			_stprintf_s(lpSelectPath, MAX_PATH, lpDefFolder);
		}
		else {
			SHGetPathFromIDList(pIDlist, lpSelectPath);
			bRet = TRUE;
			pMalloc->Free(pIDlist);
		}
		pMalloc->Release();
	}
	return bRet;
}


// AdvancedSettingDlg ダイアログ

IMPLEMENT_DYNAMIC(AdvancedSettingDlg, CDialogEx)

AdvancedSettingDlg::AdvancedSettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG2, pParent)
{
	max_dpc_module_count_ = 8;
	memset(dpc_parameter_file_name_, 0, sizeof(dpc_parameter_file_name_));
	dpl_gui_configuration_ = nullptr;
	isc_dpl_ = nullptr;
}

AdvancedSettingDlg::~AdvancedSettingDlg()
{
}

void AdvancedSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(AdvancedSettingDlg, CDialogEx)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_BUTTON1, &AdvancedSettingDlg::OnBnClickedButton1)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &AdvancedSettingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON2, &AdvancedSettingDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &AdvancedSettingDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &AdvancedSettingDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &AdvancedSettingDlg::OnBnClickedButton5)
END_MESSAGE_MAP()


// AdvancedSettingDlg メッセージ ハンドラー


void AdvancedSettingDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	if (dpl_gui_configuration_ != nullptr) {

		// Log
		int listCount = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCount();
		if (listCount == 0) {
			((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("NONE"));
			((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("FATAL"));
			((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("ERROR"));		
			((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("WARN"));	
			((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("INFO"));
			((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("DEBUG"));
			((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("TRACE"));
		}
		int log_level = min(6, dpl_gui_configuration_->GetLogLevel());
		((CComboBox*)GetDlgItem(IDC_COMBO1))->SetCurSel(log_level);

		TCHAR log_file_path[_MAX_PATH] = {};

		dpl_gui_configuration_->GetLogFilePath(log_file_path, _MAX_PATH);
		((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(log_file_path);

		// Camera
		bool enabled_camera = dpl_gui_configuration_->IsEnabledCamera();
		if (enabled_camera) {
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);
		}
		else {
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);
		}

		listCount = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCount();
		if (listCount == 0) {
			((CComboBox*)GetDlgItem(IDC_COMBO2))->InsertString(-1, _T("VM"));
			((CComboBox*)GetDlgItem(IDC_COMBO2))->InsertString(-1, _T("XC"));
		}
		const int camera_model = dpl_gui_configuration_->GetCameraModel();
		IscCameraModel isc_camera_model = IscCameraModel::kUnknown;
		switch (camera_model) {
		case 0:
			((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(0);
			break;
		case 1:
			((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(1);
			break;
		default:
			((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(0);
			break;
		}

		TCHAR camera_data_file_path[_MAX_PATH] = {};

		dpl_gui_configuration_->GetDataRecordPath(camera_data_file_path, _MAX_PATH);
		((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(camera_data_file_path);

		// Data proc module
		bool enabled_data_proc_module = dpl_gui_configuration_->IsEnabledDataProcLib();
		if (enabled_data_proc_module) {
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);
		}
		else {
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);
		}

		// Draw
		TCHAR info_msg[64] = {};
		int draw_min_distancel = (int)dpl_gui_configuration_->GetDrawMinDistance();
		_stprintf_s(info_msg, _T("%d"), draw_min_distancel);
		((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(info_msg);

		int draw_max_distancel = (int)dpl_gui_configuration_->GetDrawMaxDistance();
		_stprintf_s(info_msg, _T("%d"), draw_max_distancel);
		((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(info_msg);

		bool draw_outside_bounds = dpl_gui_configuration_->IsDrawOutsideBounds();
		if (draw_outside_bounds) {
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_CHECKED);
		}
		else {
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);
		}
	}

}


void AdvancedSettingDlg::OnClose()
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	CDialogEx::OnClose();
}


void AdvancedSettingDlg::OnBnClickedButton1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Data processing library parameter settings
	int module_index = 0;
	InvokeDpcParameterEditor(module_index, dpc_parameter_file_name_[module_index]);

}


void AdvancedSettingDlg::OnBnClickedButton4()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Data processing library parameter settings
	int module_index = 1;
	InvokeDpcParameterEditor(module_index, dpc_parameter_file_name_[module_index]);

}


void AdvancedSettingDlg::OnBnClickedOk()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Up Data Setting
	if (dpl_gui_configuration_ != nullptr) {

		// Log
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		dpl_gui_configuration_->SetLogLevel(index);

		CString str;
		((CEdit*)GetDlgItem(IDC_EDIT1))->GetWindowText(str);
		dpl_gui_configuration_->SetLogFilePath(str.GetBuffer());

		// Camera
		int checked = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck();
		bool enabled_camera = (checked == BST_CHECKED)? true:false;
		dpl_gui_configuration_->SetEnabledCamera(enabled_camera);

		index = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCurSel();
		dpl_gui_configuration_->SetCameraModel(index);

		((CEdit*)GetDlgItem(IDC_EDIT2))->GetWindowText(str);
		dpl_gui_configuration_->SetDataRecordPath(str.GetBuffer());

		// Data proc module
		checked = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck();
		bool enabled_data_proc_module = (checked == BST_CHECKED) ? true : false;
		dpl_gui_configuration_->SetEnabledDataProcLib(enabled_data_proc_module);

		// Draw
		((CEdit*)GetDlgItem(IDC_EDIT3))->GetWindowText(str);
		int draw_min_distancel = _ttoi(str);
		dpl_gui_configuration_->SetDrawMinDistance((double)draw_min_distancel);

		((CEdit*)GetDlgItem(IDC_EDIT4))->GetWindowText(str);
		int draw_max_distancel = _ttoi(str);
		dpl_gui_configuration_->SetDrawMaxDistance((double)draw_max_distancel);

		checked = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck();
		bool draw_outside_bounds = (checked == BST_CHECKED) ? true : false;
		dpl_gui_configuration_->SetDrawOutsideBounds(draw_outside_bounds);

		dpl_gui_configuration_->Save();
	}

	CDialogEx::OnOK();
}


void AdvancedSettingDlg::OnBnClickedButton2()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Set Log Path
	TCHAR selectedFilder[_MAX_PATH] = {};

	CString csString;
	((CEdit*)GetDlgItem(IDC_EDIT1))->GetWindowText(csString);
	TCHAR defultFolder[_MAX_PATH] = {};
	_stprintf_s(defultFolder, _T("%s"), csString.GetBuffer());


	BOOL bRes = SelectFolder(
		this->m_hWnd,
		defultFolder,	//NULL,
		selectedFilder,
		BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE,
		_T("Please select a folder")
	);

	if (bRes) {
		((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(selectedFilder);
	}

}


void AdvancedSettingDlg::OnBnClickedButton3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Set Camera Data Path
	TCHAR selectedFilder[_MAX_PATH] = {};

	CString csString;
	((CEdit*)GetDlgItem(IDC_EDIT2))->GetWindowText(csString);
	TCHAR defultFolder[_MAX_PATH] = {};
	_stprintf_s(defultFolder, _T("%s"), csString.GetBuffer());


	BOOL bRes = SelectFolder(
		this->m_hWnd,
		defultFolder,	//NULL,
		selectedFilder,
		BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE,
		_T("Please select a folder")
	);

	if (bRes) {
		((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(selectedFilder);
	}

}


void AdvancedSettingDlg::SetObject(DplGuiConfiguration* dpl_gui_configuration, ns_isc_dpl::IscDpl* isc_dpl)
{
	dpl_gui_configuration_ = dpl_gui_configuration;
	isc_dpl_ = isc_dpl;

	return;
}


void AdvancedSettingDlg::SetDpcParameterFileName(const int module_index, const TCHAR* module_name, const TCHAR* fileName)
{
	if (module_index >= 8) {
		return;
	}
	switch (module_index) {
	case 0:	GetDlgItem(IDC_STATIC_MODULE_NAME_1)->SetWindowText(module_name); break;
	case 1:	GetDlgItem(IDC_STATIC_MODULE_NAME_2)->SetWindowText(module_name); break;
	}
	
	_stprintf_s(dpc_parameter_file_name_[module_index], _T("%s"), fileName);

	return;
}

bool AdvancedSettingDlg::InvokeDpcParameterEditor(const int module_index, const TCHAR* parameter_file_name)
{
	DWORD dwExidCode = -1;

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = 0;

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));

	TCHAR cmdLine[_MAX_PATH + _MAX_PATH + 1] = {};
	_stprintf_s(cmdLine, _T("C:\\WINDOWS\\system32\\notepad.exe %s"), parameter_file_name);

	if (CreateProcess(NULL, cmdLine, &sa, &sa, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &dwExidCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		// update parameter
		isc_dpl_->ReloadParameterFromFile(module_index, parameter_file_name, true);
	}
	else {
		TCHAR msg[_MAX_PATH + 32] = {};
		_stprintf_s(msg, _T("[ERROR]Could not open file %s"), parameter_file_name);
		MessageBox(msg, _T("AdvancedSettingDlg::OnBnClickedButton1()"), MB_ICONERROR);
	}

	return true;
}





void AdvancedSettingDlg::OnBnClickedButton5()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	opensourecinfo_dlg_ = new OpenSourceInfoDlg(this);
	opensourecinfo_dlg_->Create(IDD_DIALOG5, this);

	opensourecinfo_dlg_->DoModal();
	//opensourecinfo_dlg_->ShowWindow(SW_SHOW);

	delete opensourecinfo_dlg_;
	opensourecinfo_dlg_ = nullptr;

}
