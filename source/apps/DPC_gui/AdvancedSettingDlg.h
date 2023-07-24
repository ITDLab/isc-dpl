#pragma once
#include "afxdialogex.h"


// AdvancedSettingDlg ダイアログ

class AdvancedSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(AdvancedSettingDlg)

public:
	AdvancedSettingDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~AdvancedSettingDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG2 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();

	void SetObject(DplGuiConfiguration* dpl_gui_configuration, ns_isc_dpl::IscDpl* isc_dpl);
	void SetDpcParameterFileName(const int module_index, const TCHAR* module_name, const TCHAR* fileName);
	bool InvokeDpcParameterEditor(const int module_index, const TCHAR* parameter_file_name);

	int max_dpc_module_count_;
	TCHAR dpc_parameter_file_name_[8][_MAX_PATH];
	DplGuiConfiguration* dpl_gui_configuration_;
	ns_isc_dpl::IscDpl* isc_dpl_;

	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
};
