#pragma once
#include "afxdialogex.h"


// PlayControlDlg ダイアログ

class PlayControlDlg : public CDialogEx
{
	DECLARE_DYNAMIC(PlayControlDlg)

public:
	PlayControlDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~PlayControlDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG3 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	
	int operation_status;	// 0:stop 1:playing
	bool pause_request_, resume_request_, stop_request_, restart_request_;

	void Initialize();
	void ClearRequests();
	void SetCurrentStatus(const bool is_playing);
	void GetRequest(bool* is_pause_request, bool* is_resume_request, bool* is_stop_request, bool* is_restart_request);

};
