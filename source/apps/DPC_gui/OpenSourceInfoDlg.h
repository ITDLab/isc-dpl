#pragma once
#include "afxdialogex.h"


// OpenSourceInfoDlg ダイアログ

class OpenSourceInfoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(OpenSourceInfoDlg)

public:
	OpenSourceInfoDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~OpenSourceInfoDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG5 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
};
