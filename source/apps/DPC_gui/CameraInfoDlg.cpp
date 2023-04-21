// CameraInfoDlg.cpp : 実装ファイル
//

#include "pch.h"
#include "DPC_gui.h"
#include "afxdialogex.h"
#include "CameraInfoDlg.h"


// CameraInfoDlg ダイアログ

IMPLEMENT_DYNAMIC(CameraInfoDlg, CDialogEx)

CameraInfoDlg::CameraInfoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG1, pParent)
{
	memset(&camera_parameter_, 0, sizeof(camera_parameter_));
}

CameraInfoDlg::~CameraInfoDlg()
{
}

void CameraInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CameraInfoDlg, CDialogEx)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CameraInfoDlg メッセージ ハンドラー


void CameraInfoDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	SetWindowText(_T("Camera Parameters"));

	((CEdit*)GetDlgItem(IDC_EDIT1))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(IDC_EDIT2))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(IDC_EDIT3))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(IDC_EDIT4))->EnableWindow(FALSE);
	((CEdit*)GetDlgItem(IDC_EDIT5))->EnableWindow(FALSE);

	((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(camera_parameter_.serial_number);
	((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(camera_parameter_.fpga_version);

	TCHAR tcBuff[64] = {};

	_stprintf_s(tcBuff, _T("%.02f"), camera_parameter_.base_length);
	((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tcBuff);

	_stprintf_s(tcBuff, _T("%.04f"), camera_parameter_.bf);
	((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tcBuff);

	_stprintf_s(tcBuff, _T("%.04f"), camera_parameter_.dinf);
	((CEdit*)GetDlgItem(IDC_EDIT5))->SetWindowText(tcBuff);

}

void CameraInfoDlg::SetCameraParameter(const CameraParameter* camera_parameter)
{
	_stprintf_s(camera_parameter_.serial_number, _T("%s"), camera_parameter->serial_number);
	_stprintf_s(camera_parameter_.fpga_version, _T("%s"), camera_parameter->fpga_version);

	camera_parameter_.base_length = camera_parameter->base_length;
	camera_parameter_.bf = camera_parameter->bf;
	camera_parameter_.dinf = camera_parameter->dinf;

	return;
}