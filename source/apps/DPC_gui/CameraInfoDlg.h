#pragma once
#include "afxdialogex.h"


// CameraInfoDlg ダイアログ

class CameraInfoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CameraInfoDlg)

public:
	CameraInfoDlg(CWnd* pParent = nullptr);   // 標準コンストラクター
	virtual ~CameraInfoDlg();

// ダイアログ データ
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG1 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);


	struct CameraParameter {
		TCHAR serial_number[128];
		TCHAR fpga_version[128];

		double base_length;
		double bf;
		double dinf;
	};
	CameraParameter camera_parameter_;
	void SetCameraParameter(const CameraParameter* camera_parameter);

};
