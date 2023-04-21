// PlayControlDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "DPC_gui.h"
#include "afxdialogex.h"
#include "PlayControlDlg.h"


// PlayControlDlg ダイアログ

IMPLEMENT_DYNAMIC(PlayControlDlg, CDialogEx)

PlayControlDlg::PlayControlDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG3, pParent)
{
	operation_status = 0;
	pause_request_ = false;
	resume_request_ = false;
	stop_request_ = false;
	restart_request_ = false;
}

PlayControlDlg::~PlayControlDlg()
{
}

void PlayControlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(PlayControlDlg, CDialogEx)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_BUTTON1, &PlayControlDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &PlayControlDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &PlayControlDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// PlayControlDlg メッセージ ハンドラー


void PlayControlDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	(GetDlgItem(IDOK))->ShowWindow(SW_HIDE);

	HWND hwnd = this->m_hWnd;

	::SetWindowPos(
		hwnd, HWND_TOPMOST,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE
	);
}


void PlayControlDlg::OnBnClickedButton1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Pause/Resume
	if (operation_status == 0) {
		// under pause
		operation_status = 1;

		(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("||"));
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(TRUE);

		resume_request_ = true;
	}
	else {
		// under play
		operation_status = 0;
		(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("▶"));
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);

		pause_request_ = true;
	}
	
}


void PlayControlDlg::OnBnClickedButton2()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Stop
	operation_status = 0;
	(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("▶"));
	(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);

	stop_request_ = true;

}


void PlayControlDlg::OnBnClickedButton3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Restart
	operation_status = 0;
	(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("▶"));
	(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);

	restart_request_ = true;

}


void PlayControlDlg::Initialize()
{
	operation_status = 0;
	pause_request_ = false;
	resume_request_ = false;
	stop_request_ = false;
	restart_request_ = false;
}


void PlayControlDlg::ClearRequests()
{
	pause_request_ = false;
	resume_request_ = false;
	stop_request_ = false;
	restart_request_ = false;
}


void PlayControlDlg::SetCurrentStatus(const bool is_playing)
{
	if (is_playing) {
		operation_status = 1;

		(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("||"));
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(TRUE);
	}
	else {
		operation_status = 0;
		(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("▶"));
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);
	}
}


void PlayControlDlg::GetRequest(bool* is_pause_request, bool* is_resume_request, bool* is_stop_request, bool* is_restart_request)
{
	*is_pause_request = pause_request_;
	*is_resume_request = resume_request_;
	*is_stop_request = stop_request_;
	*is_restart_request = restart_request_;
}

