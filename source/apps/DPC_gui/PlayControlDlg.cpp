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
	operation_status_ = OperationStatus::idle;
	pause_request_ = false;
	resume_request_ = false;
	stop_request_ = false;
	restart_request_ = false;
	end_request_ = false;

	current_frame_number_ = 0;
	request_fo_designated_number_ = false;
	designated_number_ = 0;

	scb_thunb_tracking_ = false;

	memset(&play_data_information_, 0, sizeof(play_data_information_));

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
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BUTTON4, &PlayControlDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// PlayControlDlg メッセージ ハンドラー


void PlayControlDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	(GetDlgItem(IDOK))->ShowWindow(SW_HIDE);
	(GetDlgItem(IDC_EDIT1))->EnableWindow(FALSE);

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
	if (operation_status_ == OperationStatus::stop) {
		// under pause
		operation_status_ = OperationStatus::run;

		(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("||"));
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(TRUE);

		resume_request_ = true;
	}
	else {
		// under play
		operation_status_ = OperationStatus::stop;
		(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("▶"));
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);

		pause_request_ = true;
	}
	
}


void PlayControlDlg::OnBnClickedButton2()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Stop
	operation_status_ = OperationStatus::stop;
	(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("▶"));
	(GetDlgItem(IDC_BUTTON1))->EnableWindow(FALSE);
	(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);

	stop_request_ = true;

}


void PlayControlDlg::OnBnClickedButton3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Restart
	operation_status_ = OperationStatus::stop;
	(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("▶"));
	(GetDlgItem(IDC_BUTTON1))->EnableWindow(TRUE);
	(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);

	restart_request_ = true;

}


void PlayControlDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	if (pScrollBar == (CScrollBar*)GetDlgItem(IDC_SLIDER1)) {
	
		int scroll_pos = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->GetPos();

		int min_value = 0;
		int max_value = 0;
		((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->GetRange(min_value, max_value);

		TCHAR tc_buff[64] = {};

		switch (nSBCode)
		{
		case SB_LEFT:
			// 左上にスクロールします。
			scroll_pos = min_value;
			((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_RIGHT:
			// 右下にスクロールします。
			scroll_pos = max_value;
			((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_PAGELEFT:
			// ウィンドウの幅だけ左にスクロールします。
			scroll_pos -= 10;
			((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_LINELEFT:
			// 1 単位ずつ左にスクロールします。
			scroll_pos = max(min_value, scroll_pos - 1);
			((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_PAGERIGHT:
			// ウィンドウの幅だけ右にスクロールします。
			scroll_pos += 10;
			((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_LINERIGHT:
			// 1 単位ずつ右にスクロールします。
			scroll_pos = min(max_value, scroll_pos + 1);
			((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_THUMBPOSITION:
			// ユーザーがスクロール ボックス (つまみ) をドラッグし、マウス ボタンを離しました。
			scb_thunb_tracking_ = false;
			break;
		case SB_THUMBTRACK:
			scb_thunb_tracking_ = true;
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_ENDSCROLL:
			// スクロールを終了します。
			scb_thunb_tracking_ = false;
			break;
		}

		if (nSBCode == SB_ENDSCROLL) {
			// update
			scroll_pos = ((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->GetPos();
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);

			// Frame番号を設定
			request_fo_designated_number_ = true;
			designated_number_ = scroll_pos;

			Sleep(10);
		}
	}

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}


void PlayControlDlg::OnBnClickedButton4()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Ended
	end_request_ = true;

}


void PlayControlDlg::Initialize(PlayDataInformation* pdi)
{
	operation_status_ = OperationStatus::idle;
	pause_request_ = false;
	resume_request_ = false;
	stop_request_ = false;
	restart_request_ = false;
	end_request_ = false;

	current_frame_number_ = 0;
	request_fo_designated_number_ = false;
	designated_number_ = 0;

	if (pdi != nullptr) {
		swprintf_s(play_data_information_.file_name_play, L"%s", pdi->file_name_play);
		play_data_information_.total_frame_count = pdi->total_frame_count;
		play_data_information_.total_time_sec = pdi->total_time_sec;
		play_data_information_.frame_interval = pdi->frame_interval;
		play_data_information_.start_time = pdi->start_time;
		play_data_information_.end_time = pdi->end_time;

		// File Name
		(GetDlgItem(IDC_STATIC_FILE))->SetWindowText(CString(play_data_information_.file_name_play));

		// Total Frame Count
		wchar_t total_frame_count[64] = {};
		swprintf_s(total_frame_count, L"%I64d", play_data_information_.total_frame_count);
		(GetDlgItem(IDC_STATIC_TFC))->SetWindowText(CString(total_frame_count));

		((CSliderCtrl*)(GetDlgItem(IDC_SLIDER1)))->SetRange(0, (int)(play_data_information_.total_frame_count - 1));

		// Total Time
		wchar_t total_time[64] = {};

		FormatTimeMsg(play_data_information_.total_time_sec, total_time, 64);
		(GetDlgItem(IDC_STATIC_TT))->SetWindowText(CString(total_time));

		// current frame
		((CSliderCtrl*)GetDlgItem(IDC_SLIDER1))->SetPos(0);
		(GetDlgItem(IDC_EDIT1))->SetWindowText(_T("0"));

		// Set Button
		(GetDlgItem(IDC_BUTTON1))->EnableWindow(TRUE);
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(TRUE);
		(GetDlgItem(IDC_BUTTON3))->EnableWindow(TRUE);

		operation_status_ = OperationStatus::stop;
	}
	else {
		operation_status_ = OperationStatus::idle;

		// File Name
		(GetDlgItem(IDC_STATIC_FILE))->SetWindowText(L"---");

		// Total Frame Count
		(GetDlgItem(IDC_STATIC_TFC))->SetWindowText(L"---");

		// Total Time
		(GetDlgItem(IDC_STATIC_TT))->SetWindowText(L"--:--:--");

		// current frame
		(GetDlgItem(IDC_EDIT1))->SetWindowText(_T("0"));

		memset(&play_data_information_, 0, sizeof(play_data_information_));
	}

	return;
}


void PlayControlDlg::ClearRequests()
{
	pause_request_ = false;
	resume_request_ = false;
	stop_request_ = false;
	restart_request_ = false;
	end_request_ = false;

	request_fo_designated_number_ = false;

	return;
}


void PlayControlDlg::SetCurrentStatus(const bool is_playing)
{
	if (is_playing) {
		operation_status_ = OperationStatus::run;

		(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("||"));
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(TRUE);
	}
	else {
		operation_status_ = OperationStatus::stop;
		(GetDlgItem(IDC_BUTTON1))->SetWindowText(_T("▶"));
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);
	}

	return;
}


void PlayControlDlg::GetRequest(bool* is_pause_request, bool* is_resume_request, bool* is_stop_request, bool* is_restart_request, bool* is_end_request)
{
	*is_pause_request = pause_request_;
	*is_resume_request = resume_request_;
	*is_stop_request = stop_request_;
	*is_restart_request = restart_request_;
	*is_end_request = end_request_;

	return;
}


void PlayControlDlg::SetCurrentFrameNumber(const int frame_number)
{
	if (!scb_thunb_tracking_) {

		current_frame_number_ = frame_number;
		((CSliderCtrl*)(GetDlgItem(IDC_SLIDER1)))->SetPos(current_frame_number_);

		wchar_t msg_frame_number[64] = {};
		swprintf_s(msg_frame_number, L"%d", current_frame_number_);
		(GetDlgItem(IDC_EDIT1))->SetWindowText(msg_frame_number);
	}

	return;
}


void PlayControlDlg::GetPlayFromSpecifiedFrame(bool* is_request, int* specify_frame)
{
	
	if (operation_status_ != OperationStatus::idle) {
		*is_request = request_fo_designated_number_;
		*specify_frame = designated_number_;
	}

	return;
}


void PlayControlDlg::FormatTimeMsg(const __int64 time, wchar_t* time_msg, const int max_length)
{
	
	int hour = 0;
	int min = 0;
	int sec = 0;

	__int64 hour_base = 60 * 60;
	__int64 min_base = 60;

	hour = (int)(time / hour_base);
	min = (int)((time - (hour * hour_base)) / min_base);
	sec = (int)(time - ((hour * hour_base) + (min * min_base)));

	wchar_t msg_hour[4] = {}, msg_min[4] = {}, msg_sec[4] = {};

	if (hour <= 0) {
		swprintf_s(msg_hour, L"--");
	}
	else {
		swprintf_s(msg_hour, L"%02d", hour);
	}

	if (min <= 0) {
		swprintf_s(msg_min, L"--");
	}
	else {
		swprintf_s(msg_min, L"%02d", min);
	}

	if (sec <= 0) {
		swprintf_s(msg_sec, L"--");
	}
	else {
		swprintf_s(msg_sec, L"%02d", sec);
	}

	swprintf_s(time_msg, max_length, L"%s : %s : %s", msg_hour, msg_min, msg_sec);

	return;
}
