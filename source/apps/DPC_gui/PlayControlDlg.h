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
	
	enum class OperationStatus {
		idle,
		run,
		stop
	};

	OperationStatus operation_status_;	

	bool pause_request_, resume_request_, stop_request_, restart_request_, end_request_;
	int current_frame_number_;

	bool request_fo_designated_number_;
	int designated_number_;

	bool scb_thunb_tracking_;

	struct PlayDataInformation {
		wchar_t file_name_play[_MAX_PATH];

		__int64 total_frame_count;  /**< Number of frames */
		__int64	total_time_sec;     /**< Playback Time (sec) */
		int		frame_interval;     /**< Storage Interval */
		__int64 start_time;         /**< Start time */
		__int64 end_time;           /**< End time */
	};
	PlayDataInformation play_data_information_;

	void Initialize(PlayDataInformation* pdi);
	void ClearRequests();
	void SetCurrentStatus(const bool is_playing);
	void GetRequest(bool* is_pause_request, bool* is_resume_request, bool* is_stop_request, bool* is_restart_request, bool* is_end_request);
	void SetCurrentFrameNumber(const int frame_number);
	void GetPlayFromSpecifiedFrame(bool* is_request, int* specify_frame);
	void FormatTimeMsg(const __int64 time, wchar_t* time_msg, const int max_length);

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedButton4();
};
