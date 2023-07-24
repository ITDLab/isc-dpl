// DPParameterDlg.cpp : 実装ファイル
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

#include "DPParameterDlg.h"


// DPParameterDlg ダイアログ
constexpr int kMax_PARAMETER_ITEM_COUNT = 36;

static int idc_id_list[kMax_PARAMETER_ITEM_COUNT][3] = {
	{IDC_STATIC2, IDC_EDIT1, IDC_STATIC28},
	{IDC_STATIC3, IDC_EDIT2, IDC_STATIC29},
	{IDC_STATIC4, IDC_EDIT3, IDC_STATIC30},
	{IDC_STATIC5, IDC_EDIT4, IDC_STATIC31},
	{IDC_STATIC6, IDC_EDIT5, IDC_STATIC32},
	{IDC_STATIC7, IDC_EDIT6, IDC_STATIC33},
	{IDC_STATIC8, IDC_EDIT7, IDC_STATIC34},
	{IDC_STATIC9, IDC_EDIT8, IDC_STATIC35},
	{IDC_STATIC10, IDC_EDIT9, IDC_STATIC36},
	{IDC_STATIC11, IDC_EDIT10, IDC_STATIC37},
	{IDC_STATIC12, IDC_EDIT11, IDC_STATIC38},
	{IDC_STATIC13, IDC_EDIT12, IDC_STATIC39},
	{IDC_STATIC14, IDC_EDIT13, IDC_STATIC40},
	{IDC_STATIC15, IDC_EDIT14, IDC_STATIC41},
	{IDC_STATIC16, IDC_EDIT15, IDC_STATIC42},
	{IDC_STATIC17, IDC_EDIT16, IDC_STATIC43},
	{IDC_STATIC18, IDC_EDIT17, IDC_STATIC44},
	{IDC_STATIC19, IDC_EDIT18, IDC_STATIC45},
	{IDC_STATIC20, IDC_EDIT19, IDC_STATIC46},
	{IDC_STATIC21, IDC_EDIT20, IDC_STATIC47},
	{IDC_STATIC22, IDC_EDIT21, IDC_STATIC48},
	{IDC_STATIC23, IDC_EDIT22, IDC_STATIC49},
	{IDC_STATIC24, IDC_EDIT23, IDC_STATIC50},
	{IDC_STATIC25, IDC_EDIT24, IDC_STATIC51},
	{IDC_STATIC26, IDC_EDIT25, IDC_STATIC52},
	{IDC_STATIC53, IDC_EDIT26, IDC_STATIC64},
	{IDC_STATIC54, IDC_EDIT27, IDC_STATIC65},
	{IDC_STATIC55, IDC_EDIT28, IDC_STATIC66},
	{IDC_STATIC56, IDC_EDIT29, IDC_STATIC67},
	{IDC_STATIC57, IDC_EDIT30, IDC_STATIC68},
	{IDC_STATIC58, IDC_EDIT31, IDC_STATIC69},
	{IDC_STATIC59, IDC_EDIT32, IDC_STATIC70},
	{IDC_STATIC60, IDC_EDIT33, IDC_STATIC71},
	{IDC_STATIC61, IDC_EDIT34, IDC_STATIC72},
	{IDC_STATIC62, IDC_EDIT35, IDC_STATIC73},
	{IDC_STATIC63, IDC_EDIT36, IDC_STATIC74},

};


IMPLEMENT_DYNAMIC(DPParameterDlg, CDialogEx)

DPParameterDlg::DPParameterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG4, pParent)
{

	max_dpc_module_count_ = 8;
	dpl_gui_configuration_ = nullptr;
	isc_dpl_ = nullptr;

	loaded_successfully_ = false;
	module_index_ = -1;
	memset(&data_proc_module_parameter_, 0, sizeof(IscDataProcModuleParameter));
	memset(&original_data_proc_module_parameter_, 0, sizeof(IscDataProcModuleParameter));
	
}

DPParameterDlg::~DPParameterDlg()
{
}

void DPParameterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(DPParameterDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDOK, &DPParameterDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &DPParameterDlg::OnBnClickedCancel)
	ON_EN_KILLFOCUS(IDC_EDIT1, &DPParameterDlg::OnEnKillfocusEdit1)
	ON_EN_KILLFOCUS(IDC_EDIT2, &DPParameterDlg::OnEnKillfocusEdit2)
	ON_EN_KILLFOCUS(IDC_EDIT3, &DPParameterDlg::OnEnKillfocusEdit3)
	ON_EN_KILLFOCUS(IDC_EDIT4, &DPParameterDlg::OnEnKillfocusEdit4)
	ON_EN_KILLFOCUS(IDC_EDIT5, &DPParameterDlg::OnEnKillfocusEdit5)
	ON_EN_KILLFOCUS(IDC_EDIT6, &DPParameterDlg::OnEnKillfocusEdit6)
	ON_EN_KILLFOCUS(IDC_EDIT7, &DPParameterDlg::OnEnKillfocusEdit7)
	ON_EN_KILLFOCUS(IDC_EDIT8, &DPParameterDlg::OnEnKillfocusEdit8)
	ON_EN_KILLFOCUS(IDC_EDIT9, &DPParameterDlg::OnEnKillfocusEdit9)
	ON_EN_KILLFOCUS(IDC_EDIT10, &DPParameterDlg::OnEnKillfocusEdit10)
	ON_EN_KILLFOCUS(IDC_EDIT11, &DPParameterDlg::OnEnKillfocusEdit11)
	ON_EN_KILLFOCUS(IDC_EDIT12, &DPParameterDlg::OnEnKillfocusEdit12)
	ON_EN_KILLFOCUS(IDC_EDIT13, &DPParameterDlg::OnEnKillfocusEdit13)
	ON_EN_KILLFOCUS(IDC_EDIT14, &DPParameterDlg::OnEnKillfocusEdit14)
	ON_EN_KILLFOCUS(IDC_EDIT15, &DPParameterDlg::OnEnKillfocusEdit15)
	ON_EN_KILLFOCUS(IDC_EDIT16, &DPParameterDlg::OnEnKillfocusEdit16)
	ON_EN_KILLFOCUS(IDC_EDIT17, &DPParameterDlg::OnEnKillfocusEdit17)
	ON_EN_KILLFOCUS(IDC_EDIT18, &DPParameterDlg::OnEnKillfocusEdit18)
	ON_EN_KILLFOCUS(IDC_EDIT19, &DPParameterDlg::OnEnKillfocusEdit19)
	ON_EN_KILLFOCUS(IDC_EDIT20, &DPParameterDlg::OnEnKillfocusEdit20)
	ON_EN_KILLFOCUS(IDC_EDIT21, &DPParameterDlg::OnEnKillfocusEdit21)
	ON_EN_KILLFOCUS(IDC_EDIT22, &DPParameterDlg::OnEnKillfocusEdit22)
	ON_EN_KILLFOCUS(IDC_EDIT23, &DPParameterDlg::OnEnKillfocusEdit23)
	ON_EN_KILLFOCUS(IDC_EDIT24, &DPParameterDlg::OnEnKillfocusEdit24)
	ON_EN_KILLFOCUS(IDC_EDIT25, &DPParameterDlg::OnEnKillfocusEdit25)
	ON_EN_KILLFOCUS(IDC_EDIT26, &DPParameterDlg::OnEnKillfocusEdit26)
	ON_EN_KILLFOCUS(IDC_EDIT27, &DPParameterDlg::OnEnKillfocusEdit27)
	ON_EN_KILLFOCUS(IDC_EDIT28, &DPParameterDlg::OnEnKillfocusEdit28)
	ON_EN_KILLFOCUS(IDC_EDIT29, &DPParameterDlg::OnEnKillfocusEdit29)
	ON_EN_KILLFOCUS(IDC_EDIT30, &DPParameterDlg::OnEnKillfocusEdit30)
	ON_EN_KILLFOCUS(IDC_EDIT31, &DPParameterDlg::OnEnKillfocusEdit31)
	ON_EN_KILLFOCUS(IDC_EDIT32, &DPParameterDlg::OnEnKillfocusEdit32)
	ON_EN_KILLFOCUS(IDC_EDIT33, &DPParameterDlg::OnEnKillfocusEdit33)
	ON_EN_KILLFOCUS(IDC_EDIT34, &DPParameterDlg::OnEnKillfocusEdit34)
	ON_EN_KILLFOCUS(IDC_EDIT35, &DPParameterDlg::OnEnKillfocusEdit35)
	ON_EN_KILLFOCUS(IDC_EDIT36, &DPParameterDlg::OnEnKillfocusEdit36)
END_MESSAGE_MAP()


// DPParameterDlg メッセージ ハンドラー


void DPParameterDlg::OnClose()
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	CDialogEx::OnClose();
}


void DPParameterDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: ここにメッセージ ハンドラー コードを追加します。

	// Initialize
	wchar_t wstring[64] = {};

	(GetDlgItem(IDC_STATIC27))->SetWindowText(_T("---"));

	for (int i = 0; i < kMax_PARAMETER_ITEM_COUNT; i++) {

		_stprintf_s(wstring, _T("---"));
		(GetDlgItem(idc_id_list[i][0]))->SetWindowText(wstring);

		_stprintf_s(wstring, _T(""));
		((CEdit*)GetDlgItem(idc_id_list[i][1]))->SetWindowText(wstring);
		((CEdit*)GetDlgItem(idc_id_list[i][1]))->SetReadOnly(TRUE);

		_stprintf_s(wstring, _T("---"));
		(GetDlgItem(idc_id_list[i][2]))->SetWindowText(wstring);
	}

	size_t size = 0;
	errno_t err = 0;

	if (loaded_successfully_) {
		// title
		swprintf_s(wstring, L"%s", data_proc_module_parameter_.module_name);
		(GetDlgItem(IDC_STATIC27))->SetWindowText(wstring);

		for (int i = 0; i < data_proc_module_parameter_.parameter_count; i++) {

			wchar_t label_string[64] = {};
			swprintf_s(label_string, L"[%s]%s",
				data_proc_module_parameter_.parameter_set[i].category,
				data_proc_module_parameter_.parameter_set[i].name);

			(GetDlgItem(idc_id_list[i][0]))->SetWindowText(label_string);

			wchar_t value_string[32] = {};
			if (data_proc_module_parameter_.parameter_set[i].value_type == 0){
				swprintf_s(value_string, L"%d", data_proc_module_parameter_.parameter_set[i].value_int);
			}
			else if (data_proc_module_parameter_.parameter_set[i].value_type == 1) {
				swprintf_s(value_string, L"%.3f", data_proc_module_parameter_.parameter_set[i].value_float);
			}
			else if (data_proc_module_parameter_.parameter_set[i].value_type == 2) {
				swprintf_s(value_string, L"%.3f", data_proc_module_parameter_.parameter_set[i].value_double);
			}

			((CEdit*)GetDlgItem(idc_id_list[i][1]))->SetWindowText(value_string);
			((CEdit*)GetDlgItem(idc_id_list[i][1]))->SetReadOnly(FALSE);

			swprintf_s(label_string, L"%s", data_proc_module_parameter_.parameter_set[i].description);
			(GetDlgItem(idc_id_list[i][2]))->SetWindowText(label_string);

		}
	}

}

void DPParameterDlg::SetObject(DplGuiConfiguration* dpl_gui_configuration, ns_isc_dpl::IscDpl* isc_dpl)
{
	dpl_gui_configuration_ = dpl_gui_configuration;
	isc_dpl_ = isc_dpl;

	return;
}


void DPParameterDlg::LoadParameter(int module_index)
{

	// get parameter from module
	module_index_ = module_index;
	memset(&data_proc_module_parameter_, 0, sizeof(IscDataProcModuleParameter));

	int ret = isc_dpl_->GetDataProcModuleParameter(module_index_, &data_proc_module_parameter_);

	if (ret == DPC_E_OK) {

		// back up to 
		original_data_proc_module_parameter_.module_index = data_proc_module_parameter_.module_index;
		swprintf_s(original_data_proc_module_parameter_.module_name , L"%s", data_proc_module_parameter_.module_name);
		original_data_proc_module_parameter_.parameter_count = data_proc_module_parameter_.parameter_count;

		for (int i = 0; i < original_data_proc_module_parameter_.parameter_count; i++) {
			original_data_proc_module_parameter_.parameter_set[i].value_type = data_proc_module_parameter_.parameter_set[i].value_type;
			original_data_proc_module_parameter_.parameter_set[i].value_int = data_proc_module_parameter_.parameter_set[i].value_int;
			original_data_proc_module_parameter_.parameter_set[i].value_float = data_proc_module_parameter_.parameter_set[i].value_float;
			original_data_proc_module_parameter_.parameter_set[i].value_double = data_proc_module_parameter_.parameter_set[i].value_double;

			swprintf_s(original_data_proc_module_parameter_.parameter_set[i].category, L"%s", data_proc_module_parameter_.parameter_set[i].category);
			swprintf_s(original_data_proc_module_parameter_.parameter_set[i].name, L"%s", data_proc_module_parameter_.parameter_set[i].name);
			swprintf_s(original_data_proc_module_parameter_.parameter_set[i].description, L"%s", data_proc_module_parameter_.parameter_set[i].description);
		}

		loaded_successfully_ = true;
	}
	else {
		loaded_successfully_ = false;
	}

	return;
}


void DPParameterDlg::OnBnClickedOk()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// save request to file
	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, true);
	if (ret == DPC_E_OK) {
	}

	CDialogEx::OnOK();
}


void DPParameterDlg::OnBnClickedCancel()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// change back to default
	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &original_data_proc_module_parameter_, true);
	if (ret == DPC_E_OK) {
	}

	CDialogEx::OnCancel();
}


void DPParameterDlg::OnEnKillfocusEdit1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 0;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit2()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 1;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 2;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit4()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 3;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit5()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 4;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit6()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 5;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit7()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 6;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit8()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 7;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit9()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 8;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit10()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 9;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit11()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 10;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit12()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 11;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit13()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 12;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit14()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 13;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit15()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 14;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit16()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 15;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit17()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 16;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit18()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 17;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit19()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 18;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit20()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 19;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit21()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 20;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit22()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 21;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit23()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 22;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit24()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 23;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit25()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 24;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit26()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 25;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit27()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 26;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit28()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 27;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit29()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 28;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit30()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 29;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit31()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 30;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit32()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 31;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit33()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 32;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit34()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 33;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit35()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 34;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}


void DPParameterDlg::OnEnKillfocusEdit36()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	int index = 35;

	CString str;
	((CEdit*)GetDlgItem(idc_id_list[index][1]))->GetWindowText(str);

	if (data_proc_module_parameter_.parameter_set[index].value_type == 0) {
		data_proc_module_parameter_.parameter_set[index].value_int = _ttoi(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 1) {
		data_proc_module_parameter_.parameter_set[index].value_float = (float)_ttof(str);
	}
	else if (data_proc_module_parameter_.parameter_set[index].value_type == 2) {
		data_proc_module_parameter_.parameter_set[index].value_double = _ttof(str);
	}

	int ret = isc_dpl_->SetDataProcModuleParameter(module_index_, &data_proc_module_parameter_, false);
	if (ret == DPC_E_OK) {
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ SetDataProcModuleParameter() failure code=0X%08X"), ret);
		MessageBox(msg, _T("DPParameterDlg"), MB_ICONERROR);
	}

}
