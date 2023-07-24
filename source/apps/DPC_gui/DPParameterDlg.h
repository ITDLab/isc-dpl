#pragma once
#include "afxdialogex.h"


// DPParameterDlg ダイアログ

class DPParameterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(DPParameterDlg)

public:
	DPParameterDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~DPParameterDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG4 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	int max_dpc_module_count_;
	DplGuiConfiguration* dpl_gui_configuration_;
	ns_isc_dpl::IscDpl* isc_dpl_;

	bool loaded_successfully_;
	int module_index_;
	IscDataProcModuleParameter data_proc_module_parameter_, original_data_proc_module_parameter_;

	void SetObject(DplGuiConfiguration* dpl_gui_configuration, ns_isc_dpl::IscDpl* isc_dpl);
	void LoadParameter(int module_index);

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnEnKillfocusEdit1();
	afx_msg void OnEnKillfocusEdit2();
	afx_msg void OnEnKillfocusEdit3();
	afx_msg void OnEnKillfocusEdit4();
	afx_msg void OnEnKillfocusEdit5();
	afx_msg void OnEnKillfocusEdit6();
	afx_msg void OnEnKillfocusEdit7();
	afx_msg void OnEnKillfocusEdit8();
	afx_msg void OnEnKillfocusEdit9();
	afx_msg void OnEnKillfocusEdit10();
	afx_msg void OnEnKillfocusEdit11();
	afx_msg void OnEnKillfocusEdit12();
	afx_msg void OnEnKillfocusEdit13();
	afx_msg void OnEnKillfocusEdit14();
	afx_msg void OnEnKillfocusEdit15();
	afx_msg void OnEnKillfocusEdit16();
	afx_msg void OnEnKillfocusEdit17();
	afx_msg void OnEnKillfocusEdit18();
	afx_msg void OnEnKillfocusEdit19();
	afx_msg void OnEnKillfocusEdit20();
	afx_msg void OnEnKillfocusEdit21();
	afx_msg void OnEnKillfocusEdit22();
	afx_msg void OnEnKillfocusEdit23();
	afx_msg void OnEnKillfocusEdit24();
	afx_msg void OnEnKillfocusEdit25();
	afx_msg void OnEnKillfocusEdit26();
	afx_msg void OnEnKillfocusEdit27();
	afx_msg void OnEnKillfocusEdit28();
	afx_msg void OnEnKillfocusEdit29();
	afx_msg void OnEnKillfocusEdit30();
	afx_msg void OnEnKillfocusEdit31();
	afx_msg void OnEnKillfocusEdit32();
	afx_msg void OnEnKillfocusEdit33();
	afx_msg void OnEnKillfocusEdit34();
	afx_msg void OnEnKillfocusEdit35();
	afx_msg void OnEnKillfocusEdit36();
};
