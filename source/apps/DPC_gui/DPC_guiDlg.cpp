
// DPC_guiDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <locale.h>
#include <stdint.h>
#include <process.h>
#include <numeric>

#include "DPC_gui.h"

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"

#include "dpl_gui_configuration.h"
#include "dpc_image_writer.h"
#include "dpc_draw_lib.h"

#include "gui_support.h"

#include "CameraInfoDlg.h"
#include "AdvancedSettingDlg.h"
#include "PlayControlDlg.h"
#include "DPParameterDlg.h"

#include "DPC_guiDlg.h"
#include "afxdialogex.h"

#pragma comment (lib, "DbgHelp")
#pragma comment (lib, "shlwapi")
#pragma comment (lib, "IscDpl")
#pragma comment (lib, "d2d1")
#pragma comment (lib, "Dwrite")


#ifdef _DEBUG
#pragma comment (lib,"opencv_world480d")
#else
#pragma comment (lib,"opencv_world480")
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// debug
LARGE_INTEGER performance_freq_;

// system
TCHAR app_path_[_MAX_PATH] = {};

// dialogs
CameraInfoDlg* camera_info_dlg_ = nullptr;
AdvancedSettingDlg* advanced_setting_dlg_ = nullptr;
PlayControlDlg* play_control_dlg_ = nullptr;
DPParameterDlg* dp_param_dlg0_ = nullptr;
DPParameterDlg* dp_param_dlg1_ = nullptr;

// dialog parameters
IscDplConfiguration isc_dpl_configuration_ = {};

// modules
DpcDrawLib* draw_data_lib_ = nullptr;
DplGuiConfiguration* dpl_gui_configuration_ = nullptr;
ns_isc_dpl::IscDpl* isc_dpl_ = nullptr;

// Timer ID
constexpr int kMainTimerID = 1;
const UINT kMainTimerElapsed = 10;
UINT_PTR main_timer_handle_ = NULL;
bool timer_processing_now = false;

// buffer for Images
DpcDrawLib::ImageDataSet image_data_set_[2] = { {}, {} };

// control for GUI
IscControl isc_control_;

// mouse cotrol 
struct MousePostionInformation {
	bool valid;
	POINT position_at_client;
	POINT position_at_image;
	POINT position_at_original_image;
	POINT position_at_depth_image;
	int currently_selected_index;

	void Clear(void)
	{
		valid = false;
		position_at_client.x = 0;
		position_at_client.y = 0;

		position_at_image.x = 0;
		position_at_image.y = 0;

		position_at_original_image.x = 0;
		position_at_original_image.y = 0;

		position_at_depth_image.x = 0;
		position_at_depth_image.y = 0;

		currently_selected_index = -1;
	}
};

struct MouseRectInformation {
	bool valid;
	RECT rect_at_client;
	RECT rect_at_image;
	RECT rect_at_original_image;
	RECT rect_at_depth_image;
	int currently_selected_index[2];

	void Clear(void)
	{
		valid = false;
		rect_at_client.top = 0;
		rect_at_client.left = 0;
		rect_at_client.bottom = 0;
		rect_at_client.right = 0;

		rect_at_image.top = 0;
		rect_at_image.left = 0;
		rect_at_image.bottom = 0;
		rect_at_image.right = 0;

		rect_at_original_image.top = 0;
		rect_at_original_image.left = 0;
		rect_at_original_image.bottom = 0;
		rect_at_original_image.right = 0;

		rect_at_depth_image.top = 0;
		rect_at_depth_image.left = 0;
		rect_at_depth_image.bottom = 0;
		rect_at_depth_image.right = 0;

		currently_selected_index[0] = -1;
		currently_selected_index[1] = -1;
	}
};

struct MouseOperationControl {
	// real time monitor
	MousePostionInformation mouse_position_real_time_monitor;

	// position pick
	bool is_pick_event_request;
	int pick_event_id;
	MousePostionInformation mouse_position_pick_information[4];

	// rect
	bool is_set_rect_event_request;
	int set_rect_event_state;
	int rect_pick_event_id;

	MouseRectInformation mouse_rect_information[4];

	void Clear(void)
	{
		mouse_position_real_time_monitor.Clear();
		is_pick_event_request = false;
		pick_event_id = 0;
		for (int i = 0; i < 4; i++) { mouse_position_pick_information[i].Clear(); }
		is_set_rect_event_request = false;
		set_rect_event_state = 0;
		rect_pick_event_id = 0;
		for (int i = 0; i < 4; i++) { mouse_rect_information[i].Clear(); }
	}
};
MouseOperationControl mouse_operation_control_;


PlayControlDlg::PlayDataInformation play_data_information_ = {};

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDPCguiDlg dialog



CDPCguiDlg::CDPCguiDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DPC_GUI_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDPCguiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDPCguiDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	ON_WM_TIMER()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_BUTTON1, &CDPCguiDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CDPCguiDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CDPCguiDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CDPCguiDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CDPCguiDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CDPCguiDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CDPCguiDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON8, &CDPCguiDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_BUTTON9, &CDPCguiDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_CHECK1, &CDPCguiDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK2, &CDPCguiDlg::OnBnClickedCheck2)
	ON_BN_CLICKED(IDC_CHECK3, &CDPCguiDlg::OnBnClickedCheck3)
	ON_BN_CLICKED(IDC_CHECK4, &CDPCguiDlg::OnBnClickedCheck4)
	ON_BN_CLICKED(IDC_CHECK5, &CDPCguiDlg::OnBnClickedCheck5)
	ON_BN_CLICKED(IDC_CHECK6, &CDPCguiDlg::OnBnClickedCheck6)
	ON_BN_CLICKED(IDC_CHECK7, &CDPCguiDlg::OnBnClickedCheck7)
	ON_BN_CLICKED(IDC_CHECK8, &CDPCguiDlg::OnBnClickedCheck8)
	ON_BN_CLICKED(IDC_CHECK9, &CDPCguiDlg::OnBnClickedCheck9)
	ON_BN_CLICKED(IDC_CHECK10, &CDPCguiDlg::OnBnClickedCheck10)
	ON_BN_CLICKED(IDC_CHECK11, &CDPCguiDlg::OnBnClickedCheck11)
	ON_BN_CLICKED(IDC_CHECK12, &CDPCguiDlg::OnBnClickedCheck12)
	ON_BN_CLICKED(IDC_CHECK13, &CDPCguiDlg::OnBnClickedCheck13)
	ON_BN_CLICKED(IDC_CHECK14, &CDPCguiDlg::OnBnClickedCheck14)
	ON_BN_CLICKED(IDC_CHECK15, &CDPCguiDlg::OnBnClickedCheck15)
	ON_BN_CLICKED(IDC_CHECK16, &CDPCguiDlg::OnBnClickedCheck16)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CDPCguiDlg::OnCbnSelchangeCombo1)
	ON_CBN_SELCHANGE(IDC_COMBO3, &CDPCguiDlg::OnCbnSelchangeCombo3)
	ON_BN_CLICKED(IDC_CHECK17, &CDPCguiDlg::OnBnClickedCheck17)
END_MESSAGE_MAP()


// CDPCguiDlg message handlers

BOOL CDPCguiDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	char* locRet = setlocale(LC_ALL, "ja-JP");

	CWnd* pCWnd = this;
	HWND hWnd = pCWnd->m_hWnd;

	// Lauout
	WINDOWPLACEMENT info = {};
	LONG            sx = 0;
	LONG            sy = 0;

	info.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(&info);
	sx = (info.rcNormalPosition.right - info.rcNormalPosition.left);
	sy = (info.rcNormalPosition.bottom - info.rcNormalPosition.top);

	// set window size
	info.rcNormalPosition.top = 0;
	info.rcNormalPosition.left = 0;
	sx = 1920;
	sy = 1050;
	info.length = sizeof(WINDOWPLACEMENT);
	info.flags = 0;
	info.showCmd = SW_SHOWNORMAL;
	info.rcNormalPosition.right = (info.rcNormalPosition.left + sx);
	info.rcNormalPosition.bottom = (info.rcNormalPosition.top + sy);
	BOOL ret = SetWindowPlacement(&info);

	// get window size
	CRect mainWinRect = {};
	SystemParametersInfo(SPI_GETWORKAREA, 0, &mainWinRect, 0);

	// initialize
	GetDlgItem(IDC_STATIC_IP0_XY)->SetWindowText(_T(""));
	GetDlgItem(IDC_STATIC_IP0_DISP)->SetWindowText(_T(""));
	GetDlgItem(IDC_STATIC_IP0_XYZ)->SetWindowText(_T(""));

	GetDlgItem(IDC_STATIC_IP1_XY)->SetWindowText(_T(""));
	GetDlgItem(IDC_STATIC_IP1_DISP)->SetWindowText(_T(""));
	GetDlgItem(IDC_STATIC_IP1_XYZ)->SetWindowText(_T(""));

	GetDlgItem(IDC_STATIC_IP2_XY)->SetWindowText(_T(""));
	GetDlgItem(IDC_STATIC_IP2_DISP)->SetWindowText(_T(""));
	GetDlgItem(IDC_STATIC_IP2_XYZ)->SetWindowText(_T(""));

	GetDlgItem(IDC_STATIC_IA0_XY)->SetWindowText(_T(""));
	GetDlgItem(IDC_STATIC_IA0_DISP)->SetWindowText(_T(""));
	GetDlgItem(IDC_STATIC_IA0_WHZ)->SetWindowText(_T(""));

	// ISC_PIC1
	RECT pic1_rect = {};
	GetDlgItem(IDC_PIC1)->GetWindowRect(&pic1_rect);
	POINT pic1_pt = { pic1_rect.left, pic1_rect.top };
	ScreenToClient(&pic1_pt);

	RECT pic1_rect_cl = {};
	GetDlgItem(IDC_PIC1)->GetClientRect(&pic1_rect_cl);
	RECT pic1_rect_new = {};
	pic1_rect_new.top		= pic1_pt.y;
	pic1_rect_new.left		= pic1_pt.x;
	pic1_rect_new.bottom	= pic1_rect_new.top + 640;
	pic1_rect_new.right		= pic1_rect_new.left + 1300;

	GetDlgItem(IDC_PIC1)->MoveWindow(&pic1_rect_new);
	GetDlgItem(IDC_PIC1)->GetClientRect(&pic1_rect_cl);

	// load configuration
	TCHAR appDir[_MAX_PATH] = {};
	TCHAR full[_MAX_PATH] = {};
	TCHAR drive[_MAX_PATH] = {};
	TCHAR dir[_MAX_PATH] = {};

	::GetModuleFileName(NULL, full, sizeof(full) / sizeof(TCHAR));
	_tsplitpath_s(full, drive, _MAX_PATH, dir, _MAX_PATH, NULL, 0, NULL, 0);
	_tmakepath_s(appDir, _MAX_PATH, drive, dir, NULL, NULL);
	_stprintf_s(app_path_, _T("%s"), appDir);
	PathRemoveFileSpec(app_path_);

	// initialize sequence
	QueryPerformanceFrequency(&performance_freq_);
	ClearIscControl(&isc_control_);
	mouse_operation_control_.Clear();

	dpl_gui_configuration_ = new DplGuiConfiguration;
	dpl_gui_configuration_->Load(app_path_);

	// check camera SDK dll
	{
		TCHAR dll_file_name[_MAX_PATH] = {};
		const int camera_model = dpl_gui_configuration_->GetCameraModel();

		if (camera_model == 0) {
			// 0:VM
			_stprintf_s(dll_file_name, _T("%s\\ISCSDKLibvm200.dll"), app_path_);
		}
		else if (camera_model == 1) {
			// 1:XC
			_stprintf_s(dll_file_name, _T("%s\\ISCSDKLibxc.dll"), app_path_);
		}
		else if (camera_model == 2) {
			// 2:4K
			_stprintf_s(dll_file_name, _T("%s\\ISCSDKLib4K.dll"), app_path_);
		}
		else if (camera_model == 3) {
			// 3:4KA
			_stprintf_s(dll_file_name, _T("%s\\ISCSDKLib4K.dll"), app_path_);
		}
		else if (camera_model == 4) {
			// 4:4KJ
			_stprintf_s(dll_file_name, _T("%s\\ISCSDKLib4KJ.dll"), app_path_);
		}
		else {
			_stprintf_s(dll_file_name, _T("%s\\ISCSDKLibvm200.dll"), app_path_);
		}

		bool is_exists = PathFileExists(dll_file_name) == TRUE ? true : false;
		if (!is_exists) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ Initialize() 指定されたカメラのDLLが見つかりません!　終了します!"));
			MessageBox(msg, _T("CDPCguiDlg::OnInitDialog()"), MB_ICONERROR);

			delete dpl_gui_configuration_;
			dpl_gui_configuration_ = nullptr;

			exit(0);
		}
	}

	// check memory size for 4k
	{
		const int camera_model = dpl_gui_configuration_->GetCameraModel();

		bool check_it = false;
		if (camera_model == 2) {
			// 2:4K
			check_it = true;
		}
		else if (camera_model == 3) {
			// 3:4KA
			check_it = true;
		}
		else if (camera_model == 4) {
			// 4:4KJ
			check_it = true;
		}

		if (check_it) {
			unsigned long long total_physical_memory_mb = 0;
			unsigned long long total_installed_physical_memory_mb = 0;
			bool ret = GetGlobalMemoryStatus(&total_physical_memory_mb, &total_installed_physical_memory_mb);

			if (ret) {
				constexpr unsigned long long size_for_limit_mb = (unsigned long long)16 * (unsigned long long)1024;
				if (total_installed_physical_memory_mb < size_for_limit_mb) {
					TCHAR msg[64] = {};
					_stprintf_s(msg, _T("[WARNING]isc_dpl_ Initialize() 4Kシリーズのカメラの使用には,16GB以上のメモリを推奨します"));
					MessageBox(msg, _T("CDPCguiDlg::OnInitDialog()"), MB_ICONWARNING);
				}
			}
		}
	}

	// open library
	isc_dpl_ = new ns_isc_dpl::IscDpl;

	swprintf_s(isc_dpl_configuration_.configuration_file_path, L"%s", app_path_);
	dpl_gui_configuration_->GetLogFilePath(isc_dpl_configuration_.log_file_path, _MAX_PATH);
	isc_dpl_configuration_.log_level = dpl_gui_configuration_->GetLogLevel();

	isc_dpl_configuration_.enabled_camera = dpl_gui_configuration_->IsEnabledCamera();

	const int camera_model = dpl_gui_configuration_->GetCameraModel();
	IscCameraModel isc_camera_model = IscCameraModel::kUnknown;
	switch (camera_model) {
	case 0:isc_camera_model = IscCameraModel::kVM; break;
	case 1:isc_camera_model = IscCameraModel::kXC; break;
	case 2:isc_camera_model = IscCameraModel::k4K; break;
	case 3:isc_camera_model = IscCameraModel::k4KA; break;
	case 4:isc_camera_model = IscCameraModel::k4KJ; break;
	}
	isc_dpl_configuration_.isc_camera_model = isc_camera_model;

	dpl_gui_configuration_->GetDataRecordPath(isc_dpl_configuration_.save_image_path, _MAX_PATH);
	dpl_gui_configuration_->GetDataRecordPath(isc_dpl_configuration_.load_image_path, _MAX_PATH);

	isc_dpl_configuration_.minimum_write_interval_time = dpl_gui_configuration_->GetCameraMinimumWriteInterval();

	isc_dpl_configuration_.enabled_data_proc_module = dpl_gui_configuration_->IsEnabledDataProcLib();

	// open camera for use it
	DPL_RESULT dpl_result = isc_dpl_->Initialize(&isc_dpl_configuration_);

	if (dpl_result == DPC_E_OK) {
		isc_dpl_->InitializeIscIamgeinfo(&isc_control_.isc_image_info);
		isc_dpl_->InitializeIscDataProcResultData(&isc_control_.isc_data_proc_result_data);

		isc_dpl_->DeviceGetOption(IscCameraInfo::kBaseLength, &isc_control_.camera_parameter.b);
		isc_dpl_->DeviceGetOption(IscCameraInfo::kBF, &isc_control_.camera_parameter.bf);
		isc_dpl_->DeviceGetOption(IscCameraInfo::kDINF, &isc_control_.camera_parameter.dinf);
		isc_control_.camera_parameter.setup_angle = 0;
	}
	else {
		isc_dpl_->InitializeIscIamgeinfo(&isc_control_.isc_image_info);
		isc_dpl_->InitializeIscDataProcResultData(&isc_control_.isc_data_proc_result_data);

		// set some default value
		isc_control_.camera_parameter.b = 0.1F;
		isc_control_.camera_parameter.bf = 60.0F;
		isc_control_.camera_parameter.dinf = 2.01F;
		isc_control_.camera_parameter.setup_angle = 0.0F;
	}

	if (dpl_result == DPC_E_OK) {
		// set up for run
		if (isc_dpl_configuration_.enabled_camera) {
			SetupDialogItemsInitial(false);
		}
		else {
			// disable operation
			SetupDialogItemsInitial(true);
			((CEdit*)GetDlgItem(IDC_STATIC_ISC_MODEL))->SetWindowText(_T("ISC MODEL:  --------"));
		}
	}
	else {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ Initialize() failure code=0X%08X"), dpl_result);
		MessageBox(msg, _T("CDPCguiDlg::OnInitDialog()"), MB_ICONERROR);

		// disable operation
		SetupDialogItemsInitial(true);
		((CEdit*)GetDlgItem(IDC_STATIC_ISC_MODEL))->SetWindowText(_T("ISC MODEL:  --------"));
	}

	// Gui default on/off
	SetupGuiControlDefault(isc_dpl_configuration_.enabled_camera && (dpl_result == DPC_E_OK));

	// Setop camera option parameters
	SetupCameraOptions(isc_dpl_configuration_.enabled_camera && (dpl_result == DPC_E_OK));

	// initialize Dialog for camera parameter
	camera_info_dlg_ = new CameraInfoDlg(this);
	camera_info_dlg_->Create(IDD_DIALOG1, this);

	CameraInfoDlg::CameraParameter info_dlg_camera_parameter = {};
	if (dpl_result == DPC_E_OK) {

		if (isc_dpl_configuration_.enabled_camera) {
			char camera_str[128] = {};
			isc_dpl_->DeviceGetOption(IscCameraInfo::kSerialNumber, &camera_str[0], (int)sizeof(camera_str));
			size_t size = 0;
			errno_t err = mbstowcs_s(&size, info_dlg_camera_parameter.serial_number, 128, camera_str, 127);

			uint64_t fpga_version = 0;
			isc_dpl_->DeviceGetOption(IscCameraInfo::kFpgaVersion, &fpga_version);
			_stprintf_s(info_dlg_camera_parameter.fpga_version, _T("0x%016I64X"), fpga_version);

			info_dlg_camera_parameter.base_length = isc_control_.camera_parameter.b;
			info_dlg_camera_parameter.bf = isc_control_.camera_parameter.bf;
			info_dlg_camera_parameter.dinf = isc_control_.camera_parameter.dinf;
		}
		else {
			_stprintf_s(info_dlg_camera_parameter.serial_number, _T("---"));
			_stprintf_s(info_dlg_camera_parameter.fpga_version, _T("---"));

			info_dlg_camera_parameter.base_length = 0;
			info_dlg_camera_parameter.bf = 0;
			info_dlg_camera_parameter.dinf = 0;
		}
	}
	else {
		_stprintf_s(info_dlg_camera_parameter.serial_number, _T("---"));
		_stprintf_s(info_dlg_camera_parameter.fpga_version, _T("---"));

		info_dlg_camera_parameter.base_length = 0;
		info_dlg_camera_parameter.bf = 0;
		info_dlg_camera_parameter.dinf = 0;
	}
	camera_info_dlg_->SetCameraParameter(&info_dlg_camera_parameter);

	// initialize Dialog for parameter setting
	advanced_setting_dlg_ = new AdvancedSettingDlg(this);
	advanced_setting_dlg_->Create(IDD_DIALOG2, this);
	advanced_setting_dlg_->SetObject(dpl_gui_configuration_, isc_dpl_);

	int total_module_count = 0;
	isc_dpl_->GetTotalModuleCount(&total_module_count);

	if (total_module_count != 0) {

		TCHAR dpc_module_name_[64] = {};
		TCHAR dpc_parameter_file_name_[_MAX_PATH] = {};

		for (int i = 0; i < total_module_count; i++) {
			isc_dpl_->GetModuleNameByIndex(i, dpc_module_name_, 64);
			isc_dpl_->GetParameterFileName(i, dpc_parameter_file_name_, _MAX_PATH);

			advanced_setting_dlg_->SetDpcParameterFileName(i, dpc_module_name_, dpc_parameter_file_name_);
		}
	}

	// initialize DpcDrawLib
	draw_data_lib_ = new DpcDrawLib;
	int draw_max_width = 0, draw_max_height = 0;
	if (isc_dpl_configuration_.enabled_camera && dpl_result == DPC_E_OK) {
		isc_dpl_->DeviceGetOption(IscCameraInfo::kWidthMax, &draw_max_width);
		isc_dpl_->DeviceGetOption(IscCameraInfo::kHeightMax, &draw_max_height);
	}
	else {
		// 暫定として指定カメラを確保する
		switch (isc_camera_model) {
		case IscCameraModel::kVM:
			draw_max_width = 720;
			draw_max_height = 480;
			break;
		case IscCameraModel::kXC:
			draw_max_width = 1280;
			draw_max_height = 720;
			break;
		case IscCameraModel::k4K:
			draw_max_width = 3840;
			draw_max_height = 1920;
			break;
		case IscCameraModel::k4KA:
			draw_max_width = 3840;
			draw_max_height = 1920;
			break;
		case IscCameraModel::k4KJ:
			draw_max_width = 3840;
			draw_max_height = 1920;
			break;
		case IscCameraModel::kUnknown:
			draw_max_width = 3840;
			draw_max_height = 1920;
			break;
		default:
			draw_max_width = 3840;
			draw_max_height = 1920;
			break;
		}
	}
	double draw_min_distancel = dpl_gui_configuration_->GetDrawMinDistance();
	double draw_max_distancel = dpl_gui_configuration_->GetDrawMaxDistance();
	double max_disparity = dpl_gui_configuration_->GetMaxDisparity();
	draw_data_lib_->Open(draw_max_width, draw_max_height, draw_min_distancel, draw_max_distancel, max_disparity, isc_dpl_configuration_.save_image_path);

	// initialize Dialog for play
	play_control_dlg_ = new PlayControlDlg(this);
	play_control_dlg_->Create(IDD_DIALOG3, this);

	// initialize dialog for parameter
	dp_param_dlg0_ = new DPParameterDlg(this);
	dp_param_dlg0_->Create(IDD_DIALOG4, this);

	dp_param_dlg1_ = new DPParameterDlg(this);
	dp_param_dlg1_->Create(IDD_DIALOG4, this);

	// 表示用バッファーの初期化
	draw_data_lib_->InitializeImageDataSet(&image_data_set_[0]);
	draw_data_lib_->InitializeImageDataSet(&image_data_set_[1]);

	// Timer開始
	main_timer_handle_ = SetTimer(kMainTimerID, kMainTimerElapsed, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDPCguiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDPCguiDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDPCguiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



int CDPCguiDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO: ここに特定な作成コードを追加してください。

	return 0;
}


void CDPCguiDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: ここにメッセージ ハンドラー コードを追加します。
	if (main_timer_handle_ != NULL) {
		KillTimer(main_timer_handle_);
		main_timer_handle_ = NULL;
	}

	if (draw_data_lib_ != nullptr) {
		draw_data_lib_->ReleaseImageDataSet(&image_data_set_[0]);
		draw_data_lib_->ReleaseImageDataSet(&image_data_set_[1]);

		draw_data_lib_->Close();
		delete draw_data_lib_;
		draw_data_lib_ = nullptr;
	}

	delete dp_param_dlg0_;
	dp_param_dlg0_ = nullptr;

	delete dp_param_dlg1_;
	dp_param_dlg1_ = nullptr;

	delete play_control_dlg_;
	play_control_dlg_ = nullptr;

	delete advanced_setting_dlg_;
	advanced_setting_dlg_ = nullptr;

	delete camera_info_dlg_;
	camera_info_dlg_ = nullptr;

	if (isc_dpl_ != nullptr) {
		isc_dpl_->ReleaeIscDataProcResultData(&isc_control_.isc_data_proc_result_data);
		isc_dpl_->ReleaeIscIamgeinfo(&isc_control_.isc_image_info);

		isc_dpl_->Terminate();
		delete isc_dpl_;
		isc_dpl_ = nullptr;
	}

	if (dpl_gui_configuration_ != nullptr) {
		delete dpl_gui_configuration_;
		dpl_gui_configuration_ = nullptr;
	}

}


void CDPCguiDlg::OnClose()
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	// run?
	if (isc_control_.main_state != MainStateState::kIdle) {
		isc_control_.stop_request = true;
		Sleep(100);
	}

	CDialogEx::OnClose();
}


void CDPCguiDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: ここにメッセージ ハンドラー コードを追加します。
}


void CDPCguiDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	LARGE_INTEGER start_time = {}, end_time = {};

	if (!timer_processing_now) {
		timer_processing_now = true;

		switch (isc_control_.main_state) {
		case MainStateState::kIdle:
			if (isc_control_.start_request) {
				isc_control_.start_request = false;
				if (isc_control_.main_state_mode == MainStateMode::kLiveStreaming) {
					isc_control_.main_state = MainStateState::kGrabStart;
				}
				else if (isc_control_.main_state_mode == MainStateMode::kPlay) {
					isc_control_.main_state = MainStateState::kPlayStart;
				}
				else {
					__debugbreak();
				}
			}
			break;

		case MainStateState::kGrabStart:
		{
			DPL_RESULT dpl_result = isc_dpl_->Start(&isc_control_.isc_start_mode);
			if (dpl_result == DPC_E_OK) {
				SetupDialogItems(true, &isc_control_);
				isc_control_.camera_status = CameraStatus::kStart;
				isc_control_.main_state = MainStateState::kGrabRun;
			}
			else {
				isc_control_.main_state = MainStateState::kGrabStop;
			}
		}
			break;


		case MainStateState::kGrabRun:
			if (isc_control_.stop_request) {
				isc_control_.stop_request = false;
				isc_control_.main_state = MainStateState::kGrabStop;
			}
			else {
				QueryPerformanceCounter(&start_time);
				bool ret = ImageCaptureProc();

				if (ret) {
					QueryPerformanceCounter(&end_time);
					double elapsed_time_1 = (double)((double)((end_time.QuadPart - start_time.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

					QueryPerformanceCounter(&start_time);
					ImageDrawProc();
					QueryPerformanceCounter(&end_time);
					double elapsed_time_2 = (double)((double)((end_time.QuadPart - start_time.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

					//TCHAR msg[128] = {};
					//_stprintf_s(msg, _T("[INFO]Capture & Draw time(ms) %.1f , %.1f\n"), elapsed_time_1, elapsed_time_2);
					//OutputDebugString(msg);
				}
			}
			break;

		case MainStateState::kGrabStop:
		{
			DPL_RESULT dpl_result = isc_dpl_->Stop();
			isc_control_.main_state = MainStateState::kGrabEnded;
		}
			break;

		case MainStateState::kGrabEnded:
			SetupDialogItems(false, &isc_control_);
			isc_control_.camera_status = CameraStatus::kStop;

			isc_control_.main_state = MainStateState::kIdle;
			break;

		case MainStateState::kPlayStart:
		{
			DPL_RESULT dpl_result = isc_dpl_->Start(&isc_control_.isc_start_mode);
			if (dpl_result == DPC_E_OK) {
				SetupDialogItems(true, &isc_control_);
				play_control_dlg_->SetCurrentStatus(true);
				isc_control_.play_frame_number = 0;
				isc_control_.camera_status = CameraStatus::kStart;
				isc_control_.main_state = MainStateState::kPlayReadyToRun;
				isc_control_.time_to_event = GetTickCount64();
			}
			else {
				isc_control_.main_state = MainStateState::kPlayEnded;
			}
		}
		break;

		case MainStateState::kPlayReadyToRun:
			if (isc_control_.stop_request) {
				isc_control_.stop_request = false;
				DPL_RESULT dpl_result = isc_dpl_->Stop();
				isc_control_.main_state = MainStateState::kPlayStop;
			}
			else {
				isc_control_.main_state = MainStateState::kPlayRun;
				isc_control_.time_to_event = GetTickCount64();
			}
			break;

		case MainStateState::kPlayRun:
			if (isc_control_.end_request) {
				isc_control_.end_request = false;
				DPL_RESULT dpl_result = isc_dpl_->Stop();
				isc_control_.main_state = MainStateState::kPlayEnded;
			}
			else if (isc_control_.stop_request) {
				isc_control_.stop_request = false;
				DPL_RESULT dpl_result = isc_dpl_->Stop();
				isc_control_.main_state = MainStateState::kPlayStop;
			}
			else if (isc_control_.pause_request) {
				isc_control_.pause_request = false;
				isc_control_.main_state = MainStateState::kPlayPause;
			}
			else if (isc_control_.restart_request) {
				isc_control_.restart_request = false;
				DPL_RESULT dpl_result = isc_dpl_->Stop();
				isc_control_.main_state = MainStateState::kPlayStart;
			}
			else {
				QueryPerformanceCounter(&start_time);
				bool ret = ImageCaptureProcForPlay();

				if (ret) {
					isc_control_.play_frame_number = isc_control_.isc_image_info.frame_data[0].data_index;
					play_control_dlg_->SetCurrentFrameNumber(isc_control_.play_frame_number);

					bool is_pause_request = false, is_resume_request = false, is_stop_request = false, is_restart_request = false, is_end_request = false;
					play_control_dlg_->GetRequest(&is_pause_request, &is_resume_request, &is_stop_request, &is_restart_request, &is_end_request);
					if (is_end_request) {
						play_control_dlg_->ClearRequests();
						isc_control_.end_request = true;
					}
					else if (is_stop_request) {
						play_control_dlg_->ClearRequests();
						isc_control_.stop_request = true;
					}
					else if (is_restart_request) {
						play_control_dlg_->ClearRequests();
						isc_control_.restart_request = true;
					}
					else if (is_pause_request) {
						play_control_dlg_->ClearRequests();
						isc_control_.pause_request = true;
					}
					else if (is_resume_request) {
						play_control_dlg_->ClearRequests();
						__debugbreak();
					}

					if (!is_end_request && !is_stop_request && !is_restart_request && !is_pause_request && !is_resume_request) {
						bool is_request = false;
						int specify_frame = 0;
						play_control_dlg_->GetPlayFromSpecifiedFrame(&is_request, &specify_frame);
						play_control_dlg_->ClearRequests();

						if (is_request) {
							DPL_RESULT dpl_result = isc_dpl_->SetReadFrameNumber(specify_frame);
						}
					}
				}

				if (ret) {
					QueryPerformanceCounter(&end_time);
					double elapsed_time_1 = (double)((double)((end_time.QuadPart - start_time.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

					QueryPerformanceCounter(&start_time);
					ImageDrawProc();
					QueryPerformanceCounter(&end_time);
					double elapsed_time_2 = (double)((double)((end_time.QuadPart - start_time.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);

					//TCHAR msg[128] = {};
					//_stprintf_s(msg, _T("[INFO]Capture & Draw time(ms) %.1f , %.1f\n"), elapsed_time_1, elapsed_time_2);
					//OutputDebugString(msg);

					isc_control_.time_to_event = GetTickCount64();
				}
				else {
					// Played to end of file
					if (isc_control_.play_frame_number >= (play_data_information_.total_frame_count - 1)) {
						// ended
						DPL_RESULT dpl_result = isc_dpl_->Stop();
						isc_control_.main_state = MainStateState::kPlayStop;
					}
					else {
						// Get Status
						__int64 current_frame_number = -1;
						IscFileReadStatus file_read_status = IscFileReadStatus::kNotReady;
						DPL_RESULT dpl_result = isc_dpl_->GetFileReadStatus(&current_frame_number, &file_read_status);
						if (dpl_result == DPC_E_OK) {
							if (file_read_status == IscFileReadStatus::kEnded) {
								// ended
								DPL_RESULT dpl_result = isc_dpl_->Stop();
								isc_control_.main_state = MainStateState::kPlayStop;
							}
						}
						else {
							// 時間待ちで終了させる
#ifdef _DEBUG
							// Debug時は時間待ちしない	
#else
							ULONGLONG time = GetTickCount64();
							const ULONGLONG play_time_out = (ULONGLONG)5000;
							if ((time - isc_control_.time_to_event) > play_time_out) {
								// time out
								DPL_RESULT dpl_result = isc_dpl_->Stop();
								isc_control_.main_state = MainStateState::kPlayStop;
							}
#endif

						}
					}
				}
			}
			break;

		case MainStateState::kPlayPause:
			if (isc_control_.end_request) {
				isc_control_.end_request = false;
				DPL_RESULT dpl_result = isc_dpl_->Stop();
				isc_control_.main_state = MainStateState::kPlayEnded;
			}
			else if (isc_control_.stop_request) {
				isc_control_.stop_request = false;
				DPL_RESULT dpl_result = isc_dpl_->Stop();
				isc_control_.main_state = MainStateState::kPlayStop;
			}
			else if (isc_control_.resume_request) {
				isc_control_.resume_request = false;
				isc_control_.main_state = MainStateState::kPlayRun;
			}
			else if (isc_control_.restart_request) {
				isc_control_.restart_request = false;
				DPL_RESULT dpl_result = isc_dpl_->Stop();
				isc_control_.main_state = MainStateState::kPlayStart;
			}
			else {
				bool is_pause_request = false, is_resume_request = false, is_stop_request = false, is_restart_request = false, is_end_request = false;
				play_control_dlg_->GetRequest(&is_pause_request, &is_resume_request, &is_stop_request, &is_restart_request, &is_end_request);
				if (is_end_request) {
					play_control_dlg_->ClearRequests();
					isc_control_.end_request = true;
				}
				else if (is_stop_request) {
					play_control_dlg_->ClearRequests();
					isc_control_.stop_request = true;
				}
				else if (is_restart_request) {
					play_control_dlg_->ClearRequests();
					isc_control_.restart_request = true;
				}
				else if (is_pause_request) {
					play_control_dlg_->ClearRequests();
					//isc_control_.pause_request = true;
				}
				else if (is_resume_request) {
					play_control_dlg_->ClearRequests();
					isc_control_.resume_request = true;
				}

				if (!is_end_request && !is_stop_request && !is_restart_request && !is_resume_request) {
					bool is_request = false;
					int specify_frame = 0;
					play_control_dlg_->GetPlayFromSpecifiedFrame(&is_request, &specify_frame);
					play_control_dlg_->ClearRequests();

					if (is_request) {
						DPL_RESULT dpl_result = isc_dpl_->SetReadFrameNumber(specify_frame);
					}
				}

				// for pickup information
				ImageDrawProc();
				isc_control_.time_to_event = GetTickCount64();
			}
			break;

		case MainStateState::kPlayStop:
			if (isc_control_.end_request) {
				isc_control_.end_request = false;
				isc_control_.main_state = MainStateState::kPlayEnded;
			}
			else if (isc_control_.stop_request) {
				isc_control_.stop_request = false;
			}
			else if (isc_control_.resume_request) {
				isc_control_.resume_request = false;
			}
			else if (isc_control_.restart_request) {
				isc_control_.restart_request = false;
				isc_control_.main_state = MainStateState::kPlayStart;
			}
			else {
				bool is_pause_request = false, is_resume_request = false, is_stop_request = false, is_restart_request = false, is_end_request = false;
				play_control_dlg_->GetRequest(&is_pause_request, &is_resume_request, &is_stop_request, &is_restart_request, &is_end_request);
				if (is_end_request) {
					play_control_dlg_->ClearRequests();
					isc_control_.end_request = true;
				}
				else if (is_restart_request) {
					play_control_dlg_->ClearRequests();
					isc_control_.restart_request = true;
				}
				else {
					play_control_dlg_->ClearRequests();
				}
			}
			break;

		case MainStateState::kPlayEnded:
			SetupDialogItems(false, &isc_control_);
			isc_control_.camera_status = CameraStatus::kStop;

			play_control_dlg_->Initialize(nullptr);
			play_control_dlg_->SetCurrentStatus(false);
			play_control_dlg_->ShowWindow(SW_HIDE);

			isc_control_.main_state = MainStateState::kIdle;
			break;

		default:
			break;
		}
	
		timer_processing_now = false;
	}


	CDialogEx::OnTimer(nIDEvent);
}


void CDPCguiDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	CDialogEx::OnLButtonDown(nFlags, point);
}


void CDPCguiDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	// Mouse Up
	if (mouse_operation_control_.is_pick_event_request) {
		mouse_operation_control_.is_pick_event_request = false;
		// マウス位置の取得
		RECT rect = {};
		GetDlgItem(IDC_PIC1)->GetWindowRect(&rect);
		ScreenToClient(&rect);

		POINT position_on_client = point;
		position_on_client.x = point.x - rect.left;
		position_on_client.y = point.y - rect.top;

		int index = mouse_operation_control_.pick_event_id;
		mouse_operation_control_.mouse_position_pick_information[index].valid = true;

		POINT current_screen_position = position_on_client;
		POINT original_screen_position = {};
		draw_data_lib_->GetOriginalMagnificationPosition(current_screen_position, &original_screen_position);

		mouse_operation_control_.mouse_position_pick_information[index].position_at_client.x = original_screen_position.x;
		mouse_operation_control_.mouse_position_pick_information[index].position_at_client.y = original_screen_position.y;

		POINT image_position = {};
		draw_data_lib_->ScreenPostionToDrawImagePosition(mouse_operation_control_.mouse_position_real_time_monitor.position_at_client, &image_position);
		mouse_operation_control_.mouse_position_pick_information[index].position_at_image = image_position;

		POINT image_position_on_original_image = {};
		int currently_selected_index = -1;
		draw_data_lib_->ScreenPostionToImagePosition(mouse_operation_control_.mouse_position_real_time_monitor.position_at_client, &image_position_on_original_image, &currently_selected_index);
		mouse_operation_control_.mouse_position_pick_information[index].position_at_original_image = image_position_on_original_image;

		POINT position_on_depth_image = {};
		draw_data_lib_->ScreenPostionToDepthImagePosition(mouse_operation_control_.mouse_position_real_time_monitor.position_at_client, &position_on_depth_image);
		mouse_operation_control_.mouse_position_pick_information[index].position_at_depth_image = position_on_depth_image;

		mouse_operation_control_.mouse_position_pick_information[index].currently_selected_index = currently_selected_index;

	}

	if (mouse_operation_control_.is_set_rect_event_request) {
		// マウス位置の取得
		RECT rect = {};
		GetDlgItem(IDC_PIC1)->GetWindowRect(&rect);
		ScreenToClient(&rect);

		POINT position_on_client = point;
		position_on_client.x = point.x - rect.left;
		position_on_client.y = point.y - rect.top;

		int index = mouse_operation_control_.rect_pick_event_id;

		if (mouse_operation_control_.set_rect_event_state == 0) {
			POINT current_screen_position = position_on_client;
			POINT original_screen_position = {};
			draw_data_lib_->GetOriginalMagnificationPosition(current_screen_position, &original_screen_position);

			mouse_operation_control_.mouse_rect_information[index].rect_at_client.top = original_screen_position.y;
			mouse_operation_control_.mouse_rect_information[index].rect_at_client.left = original_screen_position.x;

			mouse_operation_control_.set_rect_event_state = 1;
		}
		else if (mouse_operation_control_.set_rect_event_state == 1) {
			mouse_operation_control_.is_set_rect_event_request = false;

			POINT current_screen_position = position_on_client;
			POINT original_screen_position = {};
			draw_data_lib_->GetOriginalMagnificationPosition(current_screen_position, &original_screen_position);

			mouse_operation_control_.mouse_rect_information[index].valid = true;
			mouse_operation_control_.mouse_rect_information[index].rect_at_client.bottom = original_screen_position.y;
			mouse_operation_control_.mouse_rect_information[index].rect_at_client.right = original_screen_position.x;

			POINT image_position[2] = { {},{} };
			POINT rect_at_client_point[2] = { {},{} };
			rect_at_client_point[0].x = mouse_operation_control_.mouse_rect_information[0].rect_at_client.left;
			rect_at_client_point[0].y = mouse_operation_control_.mouse_rect_information[0].rect_at_client.top;
			draw_data_lib_->ScreenPostionToDrawImagePosition(rect_at_client_point[0], &image_position[0]);

			rect_at_client_point[1].x = mouse_operation_control_.mouse_rect_information[0].rect_at_client.right;
			rect_at_client_point[1].y = mouse_operation_control_.mouse_rect_information[0].rect_at_client.bottom;
			draw_data_lib_->ScreenPostionToDrawImagePosition(rect_at_client_point[1], &image_position[1]);

			mouse_operation_control_.mouse_rect_information[index].rect_at_image.top = image_position[0].y;
			mouse_operation_control_.mouse_rect_information[index].rect_at_image.left = image_position[0].x;
			mouse_operation_control_.mouse_rect_information[index].rect_at_image.bottom = image_position[1].y;
			mouse_operation_control_.mouse_rect_information[index].rect_at_image.right = image_position[1].x;

			POINT image_position_on_original_image[2] = { {}, {} };
			int currently_selected_index[2] = { -1, -1 };
			draw_data_lib_->ScreenPostionToImagePosition(rect_at_client_point[0], &image_position_on_original_image[0], &currently_selected_index[0]);
			draw_data_lib_->ScreenPostionToImagePosition(rect_at_client_point[1], &image_position_on_original_image[1], &currently_selected_index[1]);

			mouse_operation_control_.mouse_rect_information[index].rect_at_original_image.top		= image_position_on_original_image[0].y;
			mouse_operation_control_.mouse_rect_information[index].rect_at_original_image.left		= image_position_on_original_image[0].x;
			mouse_operation_control_.mouse_rect_information[index].rect_at_original_image.bottom	= image_position_on_original_image[1].y;
			mouse_operation_control_.mouse_rect_information[index].rect_at_original_image.right		= image_position_on_original_image[1].x;

			POINT position_on_depth_image[2] = { {}, {} };
			draw_data_lib_->ScreenPostionToDepthImagePosition(rect_at_client_point[0], &position_on_depth_image[0]);
			draw_data_lib_->ScreenPostionToDepthImagePosition(rect_at_client_point[1], &position_on_depth_image[1]);

			mouse_operation_control_.mouse_rect_information[index].rect_at_depth_image.top		= position_on_depth_image[0].y;
			mouse_operation_control_.mouse_rect_information[index].rect_at_depth_image.left		= position_on_depth_image[0].x;
			mouse_operation_control_.mouse_rect_information[index].rect_at_depth_image.bottom	= position_on_depth_image[1].y;
			mouse_operation_control_.mouse_rect_information[index].rect_at_depth_image.right	= position_on_depth_image[1].x;

			mouse_operation_control_.mouse_rect_information[index].currently_selected_index[0] = currently_selected_index[0];
			mouse_operation_control_.mouse_rect_information[index].currently_selected_index[1] = currently_selected_index[1];

			mouse_operation_control_.set_rect_event_state = 2;
		}
	}
		

	CDialogEx::OnLButtonUp(nFlags, point);
}


void CDPCguiDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	CDialogEx::OnRButtonDown(nFlags, point);
}


void CDPCguiDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	CDialogEx::OnRButtonUp(nFlags, point);
}


void CDPCguiDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	// マウス位置の取得
	RECT rect = {};
	GetDlgItem(IDC_PIC1)->GetWindowRect(&rect);
	ScreenToClient(&rect);
	
	POINT position_on_client = point;
	position_on_client.x = point.x - rect.left;
	position_on_client.y = point.y - rect.top;

	LONG rect_width = rect.right - rect.left;
	LONG rect_height = rect.bottom - rect.top;

	bool is_inside_rect = false;
	if (position_on_client.x >= 0 && position_on_client.y >= 0 && position_on_client.x < rect_width && position_on_client.y < rect_height) {
		is_inside_rect = true;
	}

	if (draw_data_lib_ == nullptr) {
		return;
	}

	POINT image_position = {};
	if (is_inside_rect) {
		mouse_operation_control_.mouse_position_real_time_monitor.valid = true;

		POINT current_screen_position = position_on_client;
		POINT original_screen_position = {};
		draw_data_lib_->GetOriginalMagnificationPosition(current_screen_position, &original_screen_position);

		mouse_operation_control_.mouse_position_real_time_monitor.position_at_client.x = original_screen_position.x;
		mouse_operation_control_.mouse_position_real_time_monitor.position_at_client.y = original_screen_position.y;
	}
	else {
		mouse_operation_control_.mouse_position_real_time_monitor.valid = false;
		mouse_operation_control_.mouse_position_real_time_monitor.position_at_client.x = 0;
		mouse_operation_control_.mouse_position_real_time_monitor.position_at_client.y = 0;
	}

	if (mouse_operation_control_.is_set_rect_event_request && (mouse_operation_control_.set_rect_event_state == 1)) {
		int index = mouse_operation_control_.rect_pick_event_id;

		POINT current_screen_position = position_on_client;
		POINT original_screen_position = {};
		draw_data_lib_->GetOriginalMagnificationPosition(current_screen_position, &original_screen_position);

		mouse_operation_control_.mouse_rect_information[index].rect_at_client.bottom = original_screen_position.y;
		mouse_operation_control_.mouse_rect_information[index].rect_at_client.right = original_screen_position.x;
	}


	CDialogEx::OnMouseMove(nFlags, point);
}


BOOL CDPCguiDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。
	RECT rect = {};
	GetDlgItem(IDC_PIC1)->GetWindowRect(&rect);
	ScreenToClient(&rect);

	POINT position_on_client = pt;
	position_on_client.x = pt.x - rect.left;
	position_on_client.y = pt.y - rect.top;

	if (zDelta > 0) {
		isc_control_.draw_settings.magnification += 0.5;
		if (isc_control_.draw_settings.magnification > 16) {
			isc_control_.draw_settings.magnification = 16;
		}
		isc_control_.draw_settings.magnification_center.x = position_on_client.x;
		isc_control_.draw_settings.magnification_center.y = position_on_client.y;


		//isc_control_.draw_settings.magnification = 1.0;
		//isc_control_.draw_settings.magnification_center.x = 0;
		//isc_control_.draw_settings.magnification_center.y = rect.top;

	}
	else {
		isc_control_.draw_settings.magnification -= 0.5;
		if (isc_control_.draw_settings.magnification < 1) {
			isc_control_.draw_settings.magnification = 1;
		}
		isc_control_.draw_settings.magnification_center.x = position_on_client.x;
		isc_control_.draw_settings.magnification_center.y = position_on_client.y;
	}

	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}


void CDPCguiDlg::OnBnClickedButton1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// it shows camera parameters
	// camera_info_dlg_->DoModal();
	camera_info_dlg_->ShowWindow(SW_SHOW);

	return;
}


void CDPCguiDlg::OnBnClickedButton2()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// save current settings
	SaveGuiControlDefault();

	// camera streaming start/stop
	if (isc_control_.camera_status == CameraStatus::kStop) {
		// rebuild the map if necessary
		double current_min_distance = 0, current_max_distance = 0;
		draw_data_lib_->GetMinMaxDistance(&current_min_distance, &current_max_distance);

		double draw_min_distancel = dpl_gui_configuration_->GetDrawMinDistance();
		double draw_max_distancel = dpl_gui_configuration_->GetDrawMaxDistance();

		if ((current_min_distance != draw_min_distancel) || (current_max_distance != draw_max_distancel)) {
			draw_data_lib_->RebuildDrawColorMap(draw_min_distancel, draw_max_distancel);
		}

		IscFeatureRequest isc_feature_request = {};

		// 0: double 1: single 2:overlap
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// 0:distance 1:disparity
		index = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCurSel();
		switch (index) {
		case 0:
			isc_feature_request.display_mode_depth = DisplayModeDepth::kDistance;
			break;
		case 1:
			isc_feature_request.display_mode_depth = DisplayModeDepth::kDisparity;
			break;
		}

		isc_feature_request.is_disparity				= ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s0_image			= ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s0_image_correct	= ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s1_image			= ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s1_image_correct	= ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_color_image				= ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_color_image_correct		= ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_dpl_stereo_matching		= ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_dpl_disparity_filter		= ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;
		if (isc_feature_request.is_dpl_stereo_matching || isc_feature_request.is_dpl_disparity_filter) {
			isc_feature_request.is_dpl_frame_decoder = true;
		}

		SetupIscControlToStart(true, false, false, nullptr, &isc_feature_request, &isc_control_);
	}
	else {

		IscFeatureRequest isc_feature_request = {};
		SetupIscControlToStart(false, false, false, nullptr, &isc_feature_request, &isc_control_);
	}
	
	return;
}


void CDPCguiDlg::OnBnClickedButton3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// free space check
	// Require 8GB or more
	unsigned __int64 requested_size = 8i64 * 1024i64 * 1024i64 * 1024i64;	
	bool ret = CheckDiskFreeSpace(isc_dpl_configuration_.save_image_path, requested_size);
	if (!ret) {
		// unable to start
		return;
	}

	// camera streaming start width data write/stop
	if (isc_control_.camera_status == CameraStatus::kStop) {
		// rebuild the map if necessary
		double current_min_distance = 0, current_max_distance = 0;
		draw_data_lib_->GetMinMaxDistance(&current_min_distance, &current_max_distance);

		double draw_min_distancel = dpl_gui_configuration_->GetDrawMinDistance();
		double draw_max_distancel = dpl_gui_configuration_->GetDrawMaxDistance();

		if ((current_min_distance != draw_min_distancel) || (current_max_distance != draw_max_distancel)) {
			draw_data_lib_->RebuildDrawColorMap(draw_min_distancel, draw_max_distancel);
		}

		IscFeatureRequest isc_feature_request = {};

		// 0: double 1: single 2:overlap
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// 0:distance 1:disparity
		index = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCurSel();
		switch (index) {
		case 0:
			isc_feature_request.display_mode_depth = DisplayModeDepth::kDistance;
			break;
		case 1:
			isc_feature_request.display_mode_depth = DisplayModeDepth::kDisparity;
			break;
		}

		isc_feature_request.is_disparity				= ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s0_image			= ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s0_image_correct	= ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s1_image			= ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s1_image_correct	= ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_color_image				= ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_color_image_correct		= ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_dpl_stereo_matching		= ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_dpl_disparity_filter		= ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;
		if (isc_feature_request.is_dpl_stereo_matching || isc_feature_request.is_dpl_disparity_filter) {
			isc_feature_request.is_dpl_frame_decoder = true;
		}

		SetupIscControlToStart(true, true, false, nullptr, &isc_feature_request, &isc_control_);
	}
	else {
		IscFeatureRequest isc_feature_request = {};
		SetupIscControlToStart(false, false, false, nullptr, &isc_feature_request, &isc_control_);
	}
	
	return;
}


void CDPCguiDlg::OnBnClickedButton4()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// run forcae correction calibration
	int	ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kManualCalibration, true);
	if (ret != DPC_E_OK) {
		TCHAR msg[64] = {};
		_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
		MessageBox(msg, _T("CDPCguiDlg::OnBnClickedButton4()"), MB_ICONERROR);
	}

	return;
}


void CDPCguiDlg::OnBnClickedButton5()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Advanced settings

	// advanced_setting_dlg_->DoModal();
	advanced_setting_dlg_->ShowWindow(SW_SHOW);

	return;
}


void CDPCguiDlg::OnBnClickedButton6()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Read from file and display
	if (isc_control_.camera_status == CameraStatus::kStop) {
		// play recoed data
		TCHAR defaultFolder[MAX_PATH] = {};
		dpl_gui_configuration_->GetDataRecordPath(defaultFolder, MAX_PATH);
	 
		TCHAR selFilter[] = _T("dat file(*.dat)|*.dat|all files(*.*)|*.*||");

		// ファイルを開くダイアログを表示します。 | OFN_NOCHANGEDIR
		CFileDialog dlg(TRUE, _T(".dat"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, selFilter, NULL, 0, TRUE);
		dlg.m_ofn.lpstrInitialDir = defaultFolder;

		CString selectFileName = _T("VM-20221212-223015011.dat");
		if (dlg.DoModal() == IDOK) {
			// 開くボタンを押下した場合
			selectFileName = dlg.GetPathName();
		}
		else {
			// キャンセルボタンを押下した場合
			return;
		}

		// rebuild the map if necessary
		double current_min_distance = 0, current_max_distance = 0;
		draw_data_lib_->GetMinMaxDistance(&current_min_distance, &current_max_distance);

		double draw_min_distancel = dpl_gui_configuration_->GetDrawMinDistance();
		double draw_max_distancel = dpl_gui_configuration_->GetDrawMaxDistance();

		if ((current_min_distance != draw_min_distancel) || (current_max_distance != draw_max_distancel)) {
			draw_data_lib_->RebuildDrawColorMap(draw_min_distancel, draw_max_distancel);
		}

		IscFeatureRequest isc_feature_request = {};

		// 0: double 1: single 2:overlap
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			isc_feature_request.display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// 0:distance 1:disparity
		index = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCurSel();
		switch (index) {
		case 0:
			isc_feature_request.display_mode_depth = DisplayModeDepth::kDistance;
			break;
		case 1:
			isc_feature_request.display_mode_depth = DisplayModeDepth::kDisparity;
			break;
		}

		// Get file information and apply it
		IscRawFileHeader raw_file_headaer = {};
		IscPlayFileInformation play_file_information = {};
		int ret = isc_dpl_->GetFileInformation(selectFileName.GetBuffer(), &raw_file_headaer, &play_file_information);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ GetFileInformation() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedButton6()"), MB_ICONERROR);
			return;
		}

		// IDC_CHECK1   Disparity
		// IDC_CHECK2   Base Image
		// IDC_CHECK3   Matching Image
		// IDC_CHECK4   Color Image
		// 
		// IDC_CHECK5   Base Image Correct
		// IDC_CHECK6   Matching Image Correct
		// IDC_CHECK7   Color Image Correct

		const bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
		const bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
		const bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
		const bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
		const bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
		const bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
		const bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
		const bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
		const bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

		bool is_header_valided = true;
		switch (raw_file_headaer.grab_mode) {
		case(1):
			// IscGrabMode::kParallax:
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);
			if (is_disparity && is_dpl_disparity_filter) {
				((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);
			}
			else if (!is_disparity && !is_dpl_disparity_filter) {
				((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);
			}

			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);

			if (raw_file_headaer.color_mode == 0) {
				((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);

				if (!is_mono_s0_image) {
					((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);
				}
			}
			else {
				//((CButton*)GetDlgItem(IDC_CHECK7))->SetCheck(BST_UNCHECKED);

				if (is_mono_s0_image && is_color_image) {
					((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);
				}
				else if (!is_mono_s0_image && !is_color_image) {
					((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_CHECKED);
				}
			}
			break;

		case(2):
			// IscGrabMode::kCorrect:
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);

			if (is_mono_s1_image && is_dpl_stereo_matching) {
				((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);
			}
			else if (!is_mono_s1_image && !is_dpl_stereo_matching) {
				((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_CHECKED);
			}

			if (raw_file_headaer.color_mode == 0) {
				((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);

				if (!is_mono_s0_image) {
					((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);
				}
			}
			else {
				//((CButton*)GetDlgItem(IDC_CHECK7))->SetCheck(BST_UNCHECKED);

				if (is_mono_s0_image && is_color_image) {
					((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);
				}
				else if (!is_mono_s0_image && !is_color_image) {
					((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_CHECKED);
				}
			}

			break;

		case(3):
			// IscGrabMode::kBeforeCorrect:
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);
			((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_UNCHECKED);
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_CHECKED);
			((CButton*)GetDlgItem(IDC_CHECK6))->SetCheck(BST_UNCHECKED);
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);

			break;

		case(4):
			// IscGrabMode::kBayerS0:
			is_header_valided = false;
			break;

		case(5):
			//  IscGrabMode::kBayerS1:
			is_header_valided = false;
			break;
		default:
			is_header_valided = false;
			break;
		}
		
		// shutter mode
		switch (raw_file_headaer.shutter_mode) {
		case 0:
			((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(0);
			break;
		case 1:
			((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(1);
			break;
		case 2:
			((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(2);
			break;
		case 3:
			((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(3);
			break;
		}
		OnCbnSelchangeCombo3();


		UpdateData(TRUE);

		if (raw_file_headaer.color_mode == 0) {
		}
		else if (raw_file_headaer.color_mode == 1) {
		}
		else {
			is_header_valided = false;
		}

		if (!is_header_valided) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]File header contents are invalid"));
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedButton6()"), MB_ICONERROR);
			return;
		}

		isc_control_.camera_parameter.b = raw_file_headaer.base_length;
		isc_control_.camera_parameter.bf = raw_file_headaer.bf;
		isc_control_.camera_parameter.dinf = raw_file_headaer.d_inf;
		isc_control_.camera_parameter.setup_angle = 0.0F;

		isc_feature_request.is_disparity				= ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s0_image			= ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s0_image_correct	= ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s1_image			= ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_mono_s1_image_correct	= ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_color_image				= ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_color_image_correct		= ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_dpl_stereo_matching		= ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
		isc_feature_request.is_dpl_disparity_filter		= ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;
		if (isc_feature_request.is_dpl_stereo_matching || isc_feature_request.is_dpl_disparity_filter) {
			isc_feature_request.is_dpl_frame_decoder = true;
		}

		SetupIscControlToStart(false, false, true, selectFileName.GetBuffer(), &isc_feature_request, &isc_control_);

		memset(&play_data_information_, 0, sizeof(play_data_information_));
		swprintf_s(play_data_information_.file_name_play, L"%s", selectFileName.GetBuffer());
		play_data_information_.total_frame_count = play_file_information.total_frame_count;
		play_data_information_.total_time_sec = play_file_information.total_time_sec;
		play_data_information_.frame_interval = play_file_information.frame_interval;
		play_data_information_.start_time = play_file_information.start_time;
		play_data_information_.end_time = play_file_information.end_time;
		
		play_control_dlg_->Initialize(&play_data_information_);
		play_control_dlg_->ShowWindow(SW_SHOW);
	}
	else {
		IscFeatureRequest isc_feature_request = {};
		SetupIscControlToStart(false, false, false, nullptr, &isc_feature_request, &isc_control_);
	}


}


void CDPCguiDlg::OnBnClickedButton7()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// Requests a single image save
	isc_control_.one_shot_save_request = true;

}


void CDPCguiDlg::OnBnClickedButton8()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// parameter for soft stereo matching
	dp_param_dlg0_->LoadParameter(0);
	dp_param_dlg0_->ShowWindow(SW_SHOW);

}


void CDPCguiDlg::OnBnClickedButton9()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// parameter for disparity
	dp_param_dlg1_->LoadParameter(2);
	dp_param_dlg1_->ShowWindow(SW_SHOW);

}


void CDPCguiDlg::OnCbnSelchangeCombo1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// it chnaged display mode, check
	DisplayModeDisplay display_mode_display = DisplayModeDisplay::kSingle;
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
	switch (index) {
	case 0:
		display_mode_display = DisplayModeDisplay::kSingle;
		break;
	case 1:
		display_mode_display = DisplayModeDisplay::kDual;
		break;
	case 2:
		display_mode_display = DisplayModeDisplay::kOverlapped;
		break;
	}

	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	// check
	if (display_mode_display == DisplayModeDisplay::kSingle) {
		if (is_disparity) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);	// mono s0 image
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
		}
		else if (is_mono_s0_image) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
		}
		else if (is_mono_s1_image) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
		}
		else if (is_color_image) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
		}
		else if (is_dpl_stereo_matching || is_dpl_disparity_filter) {
		}
		else {
			// 1つはOnとする
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);		// mono s0 image
		}
	}
	else if (display_mode_display == DisplayModeDisplay::kDual) {
		if (is_disparity) {
			if (is_mono_s0_image) {
				// 他はOffとする
				((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
				((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
				((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
				((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
			}
			else if (is_mono_s1_image) {
				// この設定はできない
				((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);		// mono s0 image
			}
			else if (is_color_image) {
				// 他はOffとする
				((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
				((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
			}
			else if (is_dpl_stereo_matching || is_dpl_disparity_filter) {
			}
			else {
				// Onとする
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);		// mono s0 image
			}
		}
		else if (is_mono_s0_image) {
			if (is_mono_s1_image) {
				((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
				((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
				((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
			}
			else if (is_color_image) {
				((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
				((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
			}
			else if (is_dpl_stereo_matching || is_dpl_disparity_filter) {
			}
			else {
				((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);		// disparity data
			}
		}
		else if (is_mono_s1_image) {
			if (is_color_image) {
				// この設定はできない
				((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
				((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
				((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);		// mono s0 image
			}
			else if (is_dpl_stereo_matching || is_dpl_disparity_filter) {
				// この設定はできない
				((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
				((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);		// mono s0 image
			}
			else {
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);		// mono s0 image
			}
		}
		else if (is_color_image) {
			if (is_dpl_stereo_matching || is_dpl_disparity_filter) {
			}
			else {
				((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);		// disparity data
			}
		}
		else if (is_dpl_stereo_matching || is_dpl_disparity_filter) {
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);			// mono s0 image
		}
		else {
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);			// disparity data
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);			// mono s0 image
		}
	}
	else if (display_mode_display == DisplayModeDisplay::kOverlapped) {
		((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);				// disparity data
		((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);				// mono s0 image
		((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_CHECKED);				// mono s0 image correct

		// 他はOffとする
		((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);			// mono s1 image
		((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);			// color image
		((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);			// block matching data
		((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);			// frame decoder data
	}

	// マウスの選択情報は一旦クリアする
	((CButton*)GetDlgItem(IDC_CHECK11))->SetCheck(BST_UNCHECKED);
	((CButton*)GetDlgItem(IDC_CHECK12))->SetCheck(BST_UNCHECKED);
	((CButton*)GetDlgItem(IDC_CHECK13))->SetCheck(BST_UNCHECKED);
	((CButton*)GetDlgItem(IDC_CHECK14))->SetCheck(BST_UNCHECKED);
	mouse_operation_control_.Clear();

}

void CDPCguiDlg::OnBnClickedCheck1()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click disparity
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	if (is_disparity) {
		// 0: double 1: single 2:overlap
		DisplayModeDisplay display_mode_display = DisplayModeDisplay::kSingle;
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// check
		if (display_mode_display == DisplayModeDisplay::kSingle) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);	// mono s0 image
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
		}
		else if (display_mode_display == DisplayModeDisplay::kDual) {
			// 以下は不許可
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data

			// どれかがOnであること
			if (is_mono_s0_image || is_color_image || is_dpl_disparity_filter) {
				if (is_mono_s0_image) {
					if (!is_mono_s0_image_correct) {
						// 自動でON
						((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_CHECKED);	// mono s0 image correct
					}
					// 他はOffとする
					((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
					((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
					((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
				}
				else if (is_color_image) {
					// 他はOffとする
					((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
					((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
				}
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);	// mono s0 image
				((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_CHECKED);	// mono s0 image correct
			}
		}
		else if (display_mode_display == DisplayModeDisplay::kOverlapped) {
			// 以下は不許可
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK6))->SetCheck(BST_UNCHECKED);	// mono s1 image correct
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data

			// Onであること
			if (is_mono_s0_image && is_mono_s0_image_correct) {
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);	// mono s0 image
				((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_CHECKED);	// mono s0 image correct
			}
		}
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck2()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click base image
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	if (is_mono_s0_image) {
		// 0: double 1: single 2:overlap
		DisplayModeDisplay display_mode_display = DisplayModeDisplay::kSingle;
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// check
		if (display_mode_display == DisplayModeDisplay::kSingle) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);	// disparity data
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
		}
		else if (display_mode_display == DisplayModeDisplay::kDual) {
			// 以下は不許可
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image

			// どれかがOnであること
			if (is_disparity || is_mono_s1_image || is_dpl_stereo_matching || is_dpl_disparity_filter) {
				if (is_disparity) {
					// 他はOffとする
					((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
					((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
					((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
				}
				else if (is_mono_s1_image) {
					// 他はOffとする
					((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
					((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
				}
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);	// disparity data
			}
		}
		else if (display_mode_display == DisplayModeDisplay::kOverlapped) {
			// 以下は不許可
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image

			// どれかがOnであること
			if (is_disparity || is_dpl_stereo_matching || is_dpl_disparity_filter) {
				if (is_disparity) {
					// 他はOffとする
					((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
					((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
					// 自動でON
					((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_CHECKED);		// mono s0 image correct
				}
				else if (is_dpl_stereo_matching || is_dpl_disparity_filter) {
				}
				else {
					// 自動でON
					((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);	// disparity data
					((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_CHECKED);	// mono s0 image correct
				}
			}
		}
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck5()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click base image correct
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	// This operation does nothing

	return;
}


void CDPCguiDlg::OnBnClickedCheck3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click matching image
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	if (is_mono_s1_image) {
		// 0: double 1: single 2:overlap
		DisplayModeDisplay display_mode_display = DisplayModeDisplay::kSingle;
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// check
		if (display_mode_display == DisplayModeDisplay::kSingle) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);	// disparity data
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);	// mono s0 image
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
		}
		else if (display_mode_display == DisplayModeDisplay::kDual) {
			// 以下は不許可
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);	// disparity data
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data

			// どれかがOnであること
			if (is_mono_s0_image) {
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);	// mono s0 image
			}
		}
		else if (display_mode_display == DisplayModeDisplay::kOverlapped) {
			// 不許可
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image

			// 自動でON
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);	// disparity data
			((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_CHECKED);	// mono s0 image correct
		}
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck6()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click matching image correct
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	// This operation does nothing

	return;
}


void CDPCguiDlg::OnBnClickedCheck4()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click color image
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	if (is_color_image) {
		// 0: double 1: single 2:overlap
		DisplayModeDisplay display_mode_display = DisplayModeDisplay::kSingle;
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// check
		if (display_mode_display == DisplayModeDisplay::kSingle) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);	// disparity data
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);	// mono s0 image
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
			((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
		}
		else if (display_mode_display == DisplayModeDisplay::kDual) {
			// 以下は不許可
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);	// mono s0 image
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image

			// どれかがOnであること
			if (is_disparity || is_dpl_stereo_matching || is_dpl_disparity_filter) {
				if (is_disparity) {
					// 他はOffとする
					((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(BST_UNCHECKED);	// block matching data
					((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(BST_UNCHECKED);	// frame decoder data
				}
				else if (is_dpl_stereo_matching || is_dpl_disparity_filter) {
				}
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);	// disparity data
			}
		}
		else if (display_mode_display == DisplayModeDisplay::kOverlapped) {
			// 不許可
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image

			// 自動でON
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_CHECKED);		// disparity data
			((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(BST_CHECKED);		// mono s0 image correct
		}
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck7()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click color image correct
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	// This operation does nothing

	return;
}


void CDPCguiDlg::OnBnClickedCheck16()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click stereo matching (<= block matching)
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	if (is_dpl_stereo_matching) {
		// 0: double 1: single 2:overlap
		DisplayModeDisplay display_mode_display = DisplayModeDisplay::kSingle;
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// check
		if (display_mode_display == DisplayModeDisplay::kSingle) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);	// disparity data
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);	// mono s0 image
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
		}
		else if (display_mode_display == DisplayModeDisplay::kDual) {
			// 以下は不許可
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);	// disparity data
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image

			// どれかがOnであること
			if (is_mono_s0_image || is_color_image) {
				if (is_mono_s0_image) {
					// こちらはOFFとする
					((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
				}
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);	// mono s0 image
			}
		}
		else if (display_mode_display == DisplayModeDisplay::kOverlapped) {
			// Onであること
			if (is_mono_s0_image) {
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);	// mono s0 image
			}
		}
	}

	// check shutter mode
	OnCbnSelchangeCombo3();

	return;
}


void CDPCguiDlg::OnBnClickedCheck15()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click disparity filter
	bool is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	bool is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	bool is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	if (is_dpl_disparity_filter) {
		// 0: double 1: single 2:overlap
		DisplayModeDisplay display_mode_display = DisplayModeDisplay::kSingle;
		int index = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
		switch (index) {
		case 0:
			display_mode_display = DisplayModeDisplay::kSingle;
			break;
		case 1:
			display_mode_display = DisplayModeDisplay::kDual;
			break;
		case 2:
			display_mode_display = DisplayModeDisplay::kOverlapped;
			break;
		}

		// check
		if (display_mode_display == DisplayModeDisplay::kSingle) {
			// 他はOffとする
			((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);	// disparity data
			((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_UNCHECKED);	// mono s0 image
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image
			((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(BST_UNCHECKED);	// color image
		}
		else if (display_mode_display == DisplayModeDisplay::kDual) {
			// 以下は不許可
			((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(BST_UNCHECKED);	// mono s1 image

			// どれかがOnであること
			if (is_disparity || is_dpl_stereo_matching) {
				if(is_disparity && is_dpl_stereo_matching) {
					((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(BST_UNCHECKED);	// disparity data
				}
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);	// mono s0 image
			}
		}
		else if (display_mode_display == DisplayModeDisplay::kOverlapped) {
			// Onであること
			if (is_mono_s0_image) {
			}
			else {
				// 自動でON
				((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(BST_CHECKED);	// mono s0 image
			}
		}
	}

	// check shutter mode
	OnCbnSelchangeCombo3();

	return;
}


void CDPCguiDlg::OnBnClickedCheck17()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// setup SelfCalibration
	bool is_self_calibration = ((CButton*)GetDlgItem(IDC_CHECK17))->GetCheck() == BST_CHECKED ? true : false;

	if (is_self_calibration) {
		// on
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kSelfCalibration, true);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedCheck17()"), MB_ICONERROR);
		}
	}
	else {
		// off
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kSelfCalibration, false);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedCheck17()"), MB_ICONERROR);
		}
	}

}


void CDPCguiDlg::OnCbnSelchangeCombo3()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// shutter control mode change
	int index = ((CComboBox*)GetDlgItem(IDC_COMBO3))->GetCurSel();

	IscShutterMode shutter_mode = IscShutterMode::kManualShutter;

	switch (index) {
	case 0:
		shutter_mode = IscShutterMode::kManualShutter;
		break;

	case 1:
		shutter_mode = IscShutterMode::kSingleShutter;
		break;

	case 2:
		shutter_mode = IscShutterMode::kDoubleShutter;
		break;

	case 3:
		shutter_mode = IscShutterMode::kDoubleShutter2;
		break;
	}

	// check
	// IDC_CHECK16  IDC_CHECK15 Data Processing Block matching and Frame decoder
	bool is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	//bool is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;

	if (is_dpl_stereo_matching) {
		// Double Shutter mode cannot be used
		if ((shutter_mode == IscShutterMode::kDoubleShutter) || (shutter_mode == IscShutterMode::kDoubleShutter2)) {
			shutter_mode = IscShutterMode::kSingleShutter;
			((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(1);

			::MessageBox(NULL, _T("Double Shutter mode cannot be used"), _T("CDPCguiDlg::OnCbnSelchangeCombo3()"), MB_ICONERROR);
		}
	}
	//else if (!is_dpl_stereo_matching && is_dpl_disparity_filter) {
	//	// Merged Double Shutter mode cannot be used
	//	if (shutter_mode == IscShutterMode::kDoubleShutter) {
	//		shutter_mode = IscShutterMode::kSingleShutter;
	//		((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(1);

	//		::MessageBox(NULL, _T("Merged Double Shutter mode cannot be used"), _T("CDPCguiDlg::OnCbnSelchangeCombo3()"), MB_ICONERROR);
	//	}
	//}

	if (isc_dpl_ != nullptr) {
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kShutterMode, shutter_mode);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnCbnSelchangeCombo3()"), MB_ICONERROR);
		}
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck9()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click high-resolution
	int checked = ((CButton*)GetDlgItem(IDC_CHECK9))->GetCheck();

	if (checked == BST_CHECKED) {
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kHrMode, true);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedCheck9()"), MB_ICONERROR);
		}
	}
	else {
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kHrMode, false);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedCheck9()"), MB_ICONERROR);
		}
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck10()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click hdr mode
	int checked = ((CButton*)GetDlgItem(IDC_CHECK10))->GetCheck();

	if (checked == BST_CHECKED) {
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kHdrMode, true);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedCheck9()"), MB_ICONERROR);
		}
	}
	else {
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kHdrMode, false);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedCheck9()"), MB_ICONERROR);
		}
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck8()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click auto-calibration
	int checked = ((CButton*)GetDlgItem(IDC_CHECK8))->GetCheck();

	if (checked == BST_CHECKED) {
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kAutoCalibration, true);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedCheck9()"), MB_ICONERROR);
		}
	}
	else {
		int ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kAutoCalibration, false);
		if (ret != DPC_E_OK) {
			TCHAR msg[64] = {};
			_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
			MessageBox(msg, _T("CDPCguiDlg::OnBnClickedCheck9()"), MB_ICONERROR);
		}
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck11()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click information-point-0 
	if (mouse_operation_control_.is_pick_event_request) {
		// have other requirements
		((CButton*)GetDlgItem(IDC_CHECK11))->SetCheck(BST_UNCHECKED);
		return;
	}

	bool is_valid = ((CButton*)GetDlgItem(IDC_CHECK11))->GetCheck() == BST_CHECKED ? true : false;

	if (is_valid) {
		mouse_operation_control_.is_pick_event_request = true;
		mouse_operation_control_.pick_event_id = 0;
	}
	else {
		mouse_operation_control_.is_pick_event_request = false;
		mouse_operation_control_.mouse_position_pick_information[0].Clear();
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck12()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click information-point-1 
	if (mouse_operation_control_.is_pick_event_request) {
		// have other requirements
		((CButton*)GetDlgItem(IDC_CHECK12))->SetCheck(BST_UNCHECKED);
		return;
	}

	bool is_valid = ((CButton*)GetDlgItem(IDC_CHECK12))->GetCheck() == BST_CHECKED ? true : false;

	if (is_valid) {
		mouse_operation_control_.is_pick_event_request = true;
		mouse_operation_control_.pick_event_id = 1;
	}
	else {
		mouse_operation_control_.is_pick_event_request = false;
		mouse_operation_control_.mouse_position_pick_information[1].Clear();
	}
	
	return;
}


void CDPCguiDlg::OnBnClickedCheck13()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click information-point-2 
	if (mouse_operation_control_.is_pick_event_request) {
		// have other requirements
		((CButton*)GetDlgItem(IDC_CHECK13))->SetCheck(BST_UNCHECKED);
		return;
	}

	bool is_valid = ((CButton*)GetDlgItem(IDC_CHECK13))->GetCheck() == BST_CHECKED ? true : false;

	if (is_valid) {
		mouse_operation_control_.is_pick_event_request = true;
		mouse_operation_control_.pick_event_id = 2;
	}
	else {
		mouse_operation_control_.is_pick_event_request = false;
		mouse_operation_control_.mouse_position_pick_information[2].Clear();
	}

	return;
}


void CDPCguiDlg::OnBnClickedCheck14()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。

	// click information-area-0 
	bool is_valid = ((CButton*)GetDlgItem(IDC_CHECK14))->GetCheck() == BST_CHECKED ? true : false;

	if (is_valid) {
		mouse_operation_control_.is_set_rect_event_request = true;
		mouse_operation_control_.set_rect_event_state = 0;
		mouse_operation_control_.rect_pick_event_id = 0;
	}
	else {
		mouse_operation_control_.is_set_rect_event_request = false;
		mouse_operation_control_.set_rect_event_state = 0;
		mouse_operation_control_.mouse_rect_information[0].Clear();
	}

	return;
}


void CDPCguiDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: ここにメッセージ ハンドラー コードを追加するか、既定の処理を呼び出します。

	// scroll bar changed
	// IDC_SCROLLBAR1   Gain Scroll Bar
	// IDC_EDIT1		Gain Value
	// IDC_SCROLLBAR2   Expossure Scroll Bar
	// IDC_EDIT2		Exposure Value
	// IDC_SCROLLBAR3   Fine Expossure Scroll Bar
	// IDC_EDIT3		Fine Expossure Value
	// IDC_SCROLLBAR4	Noise Filter Scroll Bar
	// IDC_EDIT4		Nose Filter Value

	// Gain用Scroll Bar?
	if (pScrollBar == (CScrollBar*)GetDlgItem(IDC_SCROLLBAR1)) {
		int scroll_pos = ((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->GetScrollPos();

		int min_value = 0;
		int max_value = 0;
		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->GetScrollRange(&min_value, &max_value);

		TCHAR tc_buff[64] = {};

		// Exposure用ScrollBar処理
		switch (nSBCode)
		{
			// 左端
		case SB_LEFT:
			scroll_pos = min_value;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
			// 右端
		case SB_RIGHT:
			scroll_pos = max_value;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
			// 左ページ/左矢印
		case SB_PAGELEFT:
			scroll_pos -= 10;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_LINELEFT:
			scroll_pos = max(min_value, scroll_pos - 1);
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
			// 右ページ/右矢印
		case SB_PAGERIGHT:
			scroll_pos += 10;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_LINERIGHT:
			scroll_pos = min(max_value, scroll_pos + 1);
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_THUMBPOSITION:
			scroll_pos = nPos;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_THUMBTRACK:
			scroll_pos = nPos;
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
			break;
		case SB_ENDSCROLL:
			break;
		}

		if (nSBCode == SB_ENDSCROLL) {
			// update
			scroll_pos = ((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->GetScrollPos();

			int	ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kGain, scroll_pos);
			if (ret != DPC_E_OK) {
				TCHAR msg[64] = {};
				_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
				MessageBox(msg, _T("CDPCguiDlg::IDC_SCROLLBAR1()"), MB_ICONERROR);
			}
			Sleep(160);

			// Read Back
			int read_value = scroll_pos;
			ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kGain, &read_value);
			if (ret != DPC_E_OK) {
				TCHAR msg[64] = {};
				_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
				MessageBox(msg, _T("CDPCguiDlg::IDC_SCROLLBAR1()"), MB_ICONERROR);
			}

			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(read_value);
			_stprintf_s(tc_buff, _T("%d"), read_value);
			((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
		}
	}

	// Exposure用Scroll Bar?
	if (pScrollBar == (CScrollBar*)GetDlgItem(IDC_SCROLLBAR2)) {
		int scroll_pos = ((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->GetScrollPos();

		int min_value = 0;
		int max_value = 0;
		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->GetScrollRange(&min_value, &max_value);
		
		TCHAR tc_buff[64] = {};

		// Exposure用ScrollBar処理
		switch (nSBCode)
		{
			// 左端
		case SB_LEFT:
			scroll_pos = min_value;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
			break;
			// 右端
		case SB_RIGHT:
			scroll_pos = max_value;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
			break;
			// 左ページ/左矢印
		case SB_PAGELEFT:
			scroll_pos -= 10;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
			break;
		case SB_LINELEFT:
			scroll_pos = max(min_value, scroll_pos - 1);
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
			break;
			// 右ページ/右矢印
		case SB_PAGERIGHT:
			scroll_pos += 10;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
			break;
		case SB_LINERIGHT:
			scroll_pos = min(max_value, scroll_pos + 1);
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
			break;
		case SB_THUMBPOSITION:
			scroll_pos = nPos;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
			break;
		case SB_THUMBTRACK:
			scroll_pos = nPos;
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
			break;
		case SB_ENDSCROLL:
			break;
		}

		if (nSBCode == SB_ENDSCROLL) {
			// update
			scroll_pos = ((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->GetScrollPos();

			int	ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kExposure, scroll_pos);
			if (ret != DPC_E_OK) {
				TCHAR msg[64] = {};
				_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
				MessageBox(msg, _T("CDPCguiDlg::IDC_SCROLLBAR2()"), MB_ICONERROR);
			}
			Sleep(160);

			// Read Back
			int read_value = scroll_pos;
			ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kExposure, &read_value);
			if (ret != DPC_E_OK) {
				TCHAR msg[64] = {};
				_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
				MessageBox(msg, _T("CDPCguiDlg::IDC_SCROLLBAR2()"), MB_ICONERROR);
			}

			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(read_value);
			_stprintf_s(tc_buff, _T("%d"), read_value);
			((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
		}
	}

	// Fine Exposure用Scroll Bar?
	if (pScrollBar == (CScrollBar*)GetDlgItem(IDC_SCROLLBAR3)) {
		int scroll_pos = ((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->GetScrollPos();

		int min_value = 0;
		int max_value = 0;
		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->GetScrollRange(&min_value, &max_value);

		TCHAR tc_buff[64] = {};

		// Exposure用ScrollBar処理
		switch (nSBCode)
		{
			// 左端
		case SB_LEFT:
			scroll_pos = min_value;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
			break;
			// 右端
		case SB_RIGHT:
			scroll_pos = max_value;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
			break;
			// 左ページ/左矢印
		case SB_PAGELEFT:
			scroll_pos -= 10;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
			break;
		case SB_LINELEFT:
			scroll_pos = max(min_value, scroll_pos - 1);
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
			break;
			// 右ページ/右矢印
		case SB_PAGERIGHT:
			scroll_pos += 10;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
			break;
		case SB_LINERIGHT:
			scroll_pos = min(max_value, scroll_pos + 1);
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
			break;
		case SB_THUMBPOSITION:
			scroll_pos = nPos;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
			break;
		case SB_THUMBTRACK:
			scroll_pos = nPos;
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
			break;
		case SB_ENDSCROLL:
			break;
		}

		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(scroll_pos);

		if (nSBCode == SB_ENDSCROLL) {
			// update
			scroll_pos = ((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->GetScrollPos();

			int	ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kFineExposure, scroll_pos);
			if (ret != DPC_E_OK) {
				TCHAR msg[64] = {};
				_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
				MessageBox(msg, _T("CDPCguiDlg::IDC_SCROLLBAR3()"), MB_ICONERROR);
			}
			Sleep(160);

			// Read Back
			int read_value = scroll_pos;
			ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kFineExposure, &read_value);
			if (ret != DPC_E_OK) {
				TCHAR msg[64] = {};
				_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
				MessageBox(msg, _T("CDPCguiDlg::IDC_SCROLLBAR3()"), MB_ICONERROR);
			}

			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(read_value);
			_stprintf_s(tc_buff, _T("%d"), read_value);
			((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
		}
	}

	// Noise Filter?
	if (pScrollBar == (CScrollBar*)GetDlgItem(IDC_SCROLLBAR4)) {
		int scroll_pos = ((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->GetScrollPos();

		int min_value = 0;
		int max_value = 0;
		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->GetScrollRange(&min_value, &max_value);

		TCHAR tc_buff[64] = {};

		// Exposure用ScrollBar処理
		switch (nSBCode)
		{
			// 左端
		case SB_LEFT:
			scroll_pos = min_value;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
			break;
			// 右端
		case SB_RIGHT:
			scroll_pos = max_value;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
			break;
			// 左ページ/左矢印
		case SB_PAGELEFT:
			scroll_pos -= 10;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
			break;
		case SB_LINELEFT:
			scroll_pos = max(min_value, scroll_pos - 1);
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
			break;
			// 右ページ/右矢印
		case SB_PAGERIGHT:
			scroll_pos += 10;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
			break;
		case SB_LINERIGHT:
			scroll_pos = min(max_value, scroll_pos + 1);
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
			break;
		case SB_THUMBPOSITION:
			scroll_pos = nPos;
			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(scroll_pos);
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
			break;
		case SB_THUMBTRACK:
			scroll_pos = nPos;
			_stprintf_s(tc_buff, _T("%d"), scroll_pos);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
			break;
		case SB_ENDSCROLL:
			break;
		}

		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(scroll_pos);

		if (nSBCode == SB_ENDSCROLL) {
			// update
			scroll_pos = ((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->GetScrollPos();

			int	ret = isc_dpl_->DeviceSetOption(IscCameraParameter::kNoiseFilter, scroll_pos);
			if (ret != DPC_E_OK) {
				TCHAR msg[64] = {};
				_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
				MessageBox(msg, _T("CDPCguiDlg::IDC_SCROLLBAR4()"), MB_ICONERROR);
			}
			Sleep(160);

			// Read Back
			int read_value = scroll_pos;
			ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kNoiseFilter, &read_value);
			if (ret != DPC_E_OK) {
				TCHAR msg[64] = {};
				_stprintf_s(msg, _T("[ERROR]isc_dpl_ DeviceSetOption() failure code=0X%08X"), ret);
				MessageBox(msg, _T("CDPCguiDlg::IDC_SCROLLBAR4()"), MB_ICONERROR);
			}

			((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(read_value);
			_stprintf_s(tc_buff, _T("%d"), read_value);
			((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
		}
	}


	return;
}


bool CDPCguiDlg::SetupDialogItemsInitial(bool is_disable_all)
{

	(GetDlgItem(IDOK))->ShowWindow(SW_HIDE);

	GetDlgItem(IDC_STATIC_CAMERASTATUS)->SetWindowText(_T("CAMERA STATUS: STOP"));		// CAMERA STATUS: STOP
	GetDlgItem(IDC_STATIC_CAMERA_FPS)->SetWindowText(_T("0 FPS"));						// STOP
	GetDlgItem(IDC_STATIC_CAMERA_ERROR_STATUS)->SetWindowText(_T("CAMERA: -----"));		// CAMERA: NO ERROR
	GetDlgItem(IDC_STATIC_MOUSE_POS_INFO)->SetWindowText(_T("(-,-) D:- X:- Y:- Z:-"));	// Mouse position information ex) (100,100), D:0.25 X:2m  Y:1.5m Z:7m
	GetDlgItem(IDC_STATIC_DP_STATUS)->SetWindowText(_T("DP STATUS: STOP"));				// DP STATUS: STOP
	GetDlgItem(IDC_STATIC_DP_FPS)->SetWindowText(_T("0 FPS"));							// STOP
	
	TCHAR camera_model_name[16] = {};
	switch (isc_dpl_configuration_.isc_camera_model) {
	case IscCameraModel::kVM:_stprintf_s(camera_model_name, _T("ISC MODEL: VM")); break;
	case IscCameraModel::kXC:_stprintf_s(camera_model_name, _T("ISC MODEL: XC")); break;
	case IscCameraModel::k4K:_stprintf_s(camera_model_name, _T("ISC MODEL: 4K")); break;
	case IscCameraModel::k4KA:_stprintf_s(camera_model_name, _T("ISC MODEL: 4KA")); break;
	case IscCameraModel::k4KJ:_stprintf_s(camera_model_name, _T("ISC MODEL: 4KJ")); break;
	}
	((CEdit*)GetDlgItem(IDC_STATIC_ISC_MODEL))->SetWindowText(camera_model_name);

	// 画像表示内容を設定します
	int listCount = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCount();
	if (listCount == 0) {
		((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("Single"));	// display two images
		((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("Dual"));		// display one image
		((CComboBox*)GetDlgItem(IDC_COMBO1))->InsertString(-1, _T("Overlap"));	// display a superimposed image	
		((CComboBox*)GetDlgItem(IDC_COMBO1))->SetCurSel(1);
	}

	// 視差の表方法を設定します
	listCount = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCount();
	if (listCount == 0) {
		((CComboBox*)GetDlgItem(IDC_COMBO2))->InsertString(-1, _T("Distance"));		// 視差
		((CComboBox*)GetDlgItem(IDC_COMBO2))->InsertString(-1, _T("Disparity"));	// 距離
		((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(0);
	}

	if (is_disable_all) {
		// disable operation
		// information
		(GetDlgItem(IDC_BUTTON1))->EnableWindow(FALSE);
		// start/stop
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);
		// record
		(GetDlgItem(IDC_BUTTON3))->EnableWindow(FALSE);
		// auto calibration manual calibration
		(GetDlgItem(IDC_BUTTON4))->EnableWindow(FALSE);
		// IDC_BUTTON7 save image
		(GetDlgItem(IDC_BUTTON7))->EnableWindow(FALSE);

		(GetDlgItem(IDC_SCROLLBAR1))->EnableWindow(FALSE);
		(GetDlgItem(IDC_SCROLLBAR2))->EnableWindow(FALSE);
		(GetDlgItem(IDC_SCROLLBAR3))->EnableWindow(FALSE);
		(GetDlgItem(IDC_SCROLLBAR4))->EnableWindow(FALSE);

		return true;
	}

	if (isc_dpl_ == nullptr) {
		return true;
	}

	// IDC_CHECK1   Disparity
	// 常に有効

	// IDC_CHECK2   Base Image
	// 常に有効
	// IDC_CHECK5   Base Image Correct
	// 常に有効

	// IDC_CHECK3   Matching Image
	// 常に有効
	// IDC_CHECK6   Matching Image Correct
	// 常に有効

	// IDC_CHECK4   Color Image
	bool is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kColorImage);
	if (is_enabled) {
		(GetDlgItem(IDC_CHECK4))->EnableWindow(TRUE);
	}
	else {
		(GetDlgItem(IDC_CHECK4))->EnableWindow(FALSE);
	}

	// IDC_CHECK7   Color Image Correct
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kColorImageCorrect);
	if (is_enabled) {
		(GetDlgItem(IDC_CHECK7))->EnableWindow(TRUE);
	}
	else {
		(GetDlgItem(IDC_CHECK7))->EnableWindow(FALSE);
	}

	// IDC_CHECK16  Data Processing Block matching and Frame decoder
	is_enabled = isc_dpl_configuration_.enabled_data_proc_module;
	if (is_enabled) {
		(GetDlgItem(IDC_CHECK16))->EnableWindow(TRUE);
		(GetDlgItem(IDC_CHECK15))->EnableWindow(TRUE);
	}
	else {
		(GetDlgItem(IDC_CHECK16))->EnableWindow(FALSE);
		(GetDlgItem(IDC_CHECK15))->EnableWindow(FALSE);
	}

	// IDC_COMBO3   Shutter Control Mode
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kShutterMode);
	if (is_enabled) {
		listCount = ((CComboBox*)GetDlgItem(IDC_COMBO3))->GetCount();
		if (listCount == 0) {
			is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kManualShutter);
			if (is_enabled) {
				((CComboBox*)GetDlgItem(IDC_COMBO3))->InsertString(-1, _T("Manual"));	// 手動
			}

			is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kSingleShutter);
			if (is_enabled) {
				((CComboBox*)GetDlgItem(IDC_COMBO3))->InsertString(-1, _T("Single"));	// シングルシャッター
			}

			is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kDoubleShutter);
			if (is_enabled) {
				((CComboBox*)GetDlgItem(IDC_COMBO3))->InsertString(-1, _T("Double"));	// ダブルシャッター
			}

			is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kDoubleShutter2);
			if (is_enabled) {
				((CComboBox*)GetDlgItem(IDC_COMBO3))->InsertString(-1, _T("Double2"));	// ダブルシャッター2
			}
		}

		// 初期値
		IscShutterMode shutter_mode = IscShutterMode::kManualShutter;
		int ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kShutterMode, &shutter_mode);
		if (ret == DPC_E_OK) {
			switch (shutter_mode) {
			case IscShutterMode::kManualShutter:
				((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(0);
				break;

			case IscShutterMode::kSingleShutter:
				((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(1);
				break;

			case IscShutterMode::kDoubleShutter:
				((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(2);
				break;

			case IscShutterMode::kDoubleShutter2:
				((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(3);
				break;
			}
		}
	}

	// IDC_SCROLLBAR1   Gain Scroll Bar
	// IDC_EDIT1    Gain Value
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kGain);
	if (is_enabled) {
		(GetDlgItem(IDC_SCROLLBAR1))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT1))->EnableWindow(TRUE);

		int min_value = 0;
		int ret = isc_dpl_->DeviceGetOptionMin(IscCameraParameter::kGain, &min_value);

		int max_value = 0;
		ret = isc_dpl_->DeviceGetOptionMax(IscCameraParameter::kGain, &max_value);

		int inc_value = 0;
		ret = isc_dpl_->DeviceGetOptionInc(IscCameraParameter::kGain, &inc_value);

		int current_value = 0;
		ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kGain, &current_value);

		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollRange(min_value, max_value, TRUE);
		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR1))->SetScrollPos(current_value);

		TCHAR tc_buff[32] = {};
		_stprintf_s(tc_buff, _T("%d"), current_value);
		((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(tc_buff);
	}
	else {
		(GetDlgItem(IDC_SCROLLBAR1))->EnableWindow(FALSE);
		(GetDlgItem(IDC_EDIT1))->EnableWindow(FALSE);
	}

	// IDC_SCROLLBAR2   Expossure Scroll Bar
	// IDC_EDIT2    Exposure Value
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kExposure);
	if (is_enabled) {
		(GetDlgItem(IDC_SCROLLBAR2))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT2))->EnableWindow(TRUE);

		int min_value = 0;
		int ret = isc_dpl_->DeviceGetOptionMin(IscCameraParameter::kExposure, &min_value);

		int max_value = 0;
		ret = isc_dpl_->DeviceGetOptionMax(IscCameraParameter::kExposure, &max_value);

		int inc_value = 0;
		ret = isc_dpl_->DeviceGetOptionInc(IscCameraParameter::kExposure, &inc_value);

		int current_value = 0;
		ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kExposure, &current_value);

		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollRange(min_value, max_value, TRUE);
		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR2))->SetScrollPos(current_value);

		TCHAR tc_buff[32] = {};
		_stprintf_s(tc_buff, _T("%d"), current_value);
		((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowText(tc_buff);
	}
	else {
		(GetDlgItem(IDC_SCROLLBAR2))->EnableWindow(FALSE);
		(GetDlgItem(IDC_EDIT2))->EnableWindow(FALSE);
	}

	// IDC_SCROLLBAR3   Fine Expossure Scroll Bar
	// IDC_EDIT3    Fine Expossure Value
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kFineExposure);
	if (is_enabled) {
		(GetDlgItem(IDC_SCROLLBAR3))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT3))->EnableWindow(TRUE);

		(GetDlgItem(IDC_SCROLLBAR2))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT2))->EnableWindow(TRUE);

		int min_value = 0;
		int ret = isc_dpl_->DeviceGetOptionMin(IscCameraParameter::kFineExposure, &min_value);

		int max_value = 0;
		ret = isc_dpl_->DeviceGetOptionMax(IscCameraParameter::kFineExposure, &max_value);

		int inc_value = 0;
		ret = isc_dpl_->DeviceGetOptionInc(IscCameraParameter::kFineExposure, &inc_value);

		int current_value = 0;
		ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kFineExposure, &current_value);

		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollRange(min_value, max_value, TRUE);
		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR3))->SetScrollPos(current_value);

		TCHAR tc_buff[32] = {};
		_stprintf_s(tc_buff, _T("%d"), current_value);
		((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowText(tc_buff);
	}
	else {
		(GetDlgItem(IDC_SCROLLBAR3))->EnableWindow(FALSE);
		(GetDlgItem(IDC_EDIT3))->EnableWindow(FALSE);
	}

	// IDC_SCROLLBAR4   Noise filter Scroll Bar
	// IDC_EDIT4    Noise filter Value
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kNoiseFilter);
	if (is_enabled) {
		(GetDlgItem(IDC_SCROLLBAR4))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT4))->EnableWindow(TRUE);

		int min_value = 0;
		int ret = isc_dpl_->DeviceGetOptionMin(IscCameraParameter::kNoiseFilter, &min_value);

		int max_value = 0;
		ret = isc_dpl_->DeviceGetOptionMax(IscCameraParameter::kNoiseFilter, &max_value);

		int inc_value = 0;
		ret = isc_dpl_->DeviceGetOptionInc(IscCameraParameter::kNoiseFilter, &inc_value);

		int current_value = 0;
		ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kNoiseFilter, &current_value);

		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollRange(min_value, max_value, TRUE);
		((CScrollBar*)GetDlgItem(IDC_SCROLLBAR4))->SetScrollPos(current_value);

		TCHAR tc_buff[32] = {};
		_stprintf_s(tc_buff, _T("%d"), current_value);
		((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowText(tc_buff);
	}
	else {
		(GetDlgItem(IDC_SCROLLBAR4))->EnableWindow(FALSE);
		(GetDlgItem(IDC_EDIT4))->EnableWindow(FALSE);
	}

	// IDC_CHECK9   High Resolution
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kHrMode);
	if (is_enabled) {
		(GetDlgItem(IDC_CHECK9))->EnableWindow(TRUE);

		bool current_value = false;
		int ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kHrMode, &current_value);
		if (current_value) {
			((CButton*)GetDlgItem(IDC_CHECK9))->SetCheck(BST_CHECKED);
		}
		else {
			((CButton*)GetDlgItem(IDC_CHECK9))->SetCheck(BST_UNCHECKED);
		}
	}
	else {
		(GetDlgItem(IDC_CHECK9))->EnableWindow(FALSE);
	}

	// IDC_CHECK10  HDR Mode
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kHdrMode);
	if (is_enabled) {
		(GetDlgItem(IDC_CHECK10))->EnableWindow(TRUE);

		bool current_value = false;
		int ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kHdrMode, &current_value);
		if (current_value) {
			((CButton*)GetDlgItem(IDC_CHECK10))->SetCheck(BST_CHECKED);
		}
		else {
			((CButton*)GetDlgItem(IDC_CHECK10))->SetCheck(BST_UNCHECKED);
		}
	}
	else {
		(GetDlgItem(IDC_CHECK10))->EnableWindow(FALSE);
	}

	// IDC_CHECK8   Auto calibration
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kAutoCalibration);
	if (is_enabled) {
		(GetDlgItem(IDC_CHECK8))->EnableWindow(TRUE);

		bool current_value = false;
		int ret = isc_dpl_->DeviceGetOption(IscCameraParameter::kAutoCalibration, &current_value);
		if (current_value) {
			((CButton*)GetDlgItem(IDC_CHECK8))->SetCheck(BST_CHECKED);
		}
		else {
			((CButton*)GetDlgItem(IDC_CHECK8))->SetCheck(BST_UNCHECKED);
		}
	}
	else {
		(GetDlgItem(IDC_CHECK8))->EnableWindow(FALSE);
	}

	// IDC_BUTTON4  Manual calibration Run
	is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kManualCalibration);
	if (is_enabled) {
		(GetDlgItem(IDC_BUTTON4))->EnableWindow(TRUE);
	}
	else {
		(GetDlgItem(IDC_BUTTON4))->EnableWindow(FALSE);
	}

	// IDC_BUTTON4 Manual calibration
	// It is only available during image capture
	(GetDlgItem(IDC_BUTTON4))->EnableWindow(FALSE);

	// IDC_BUTTON7 save image
	(GetDlgItem(IDC_BUTTON7))->EnableWindow(FALSE);

	// 初期設定

	// IDC_CHECK1   Disparity
	// ((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(TRUE);

	// IDC_CHECK2   Base Image
	((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(TRUE);

	// IDC_CHECK5   Base Image Correct
	((CButton*)GetDlgItem(IDC_CHECK5))->SetCheck(TRUE);

	if (isc_dpl_configuration_.enabled_data_proc_module) {
		// IDC_CHECK16  Data Processing Block Matching
		((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(TRUE);
		// IDC_CHECK15  Data Processing Frame Decoder
		((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(TRUE);
		// IDC_CHECK1   Disparity
		((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(FALSE);
	}
	else {
		// IDC_CHECK16  Data Processing Block Matching
		((CButton*)GetDlgItem(IDC_CHECK16))->SetCheck(FALSE);
		// IDC_CHECK15  Data Processing Frame Decoder
		((CButton*)GetDlgItem(IDC_CHECK15))->SetCheck(FALSE);
		// IDC_CHECK1   Disparity
		((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(TRUE);
	}

	return true;
}


bool CDPCguiDlg::SetupDialogItems(const bool is_start, const IscControl* isc_control)
{
	if (isc_dpl_ == nullptr) {
		return true;
	}

	bool is_streaming = false;
	bool is_recording = false;
	bool is_play = false;

	if (is_start) {
		if (isc_control->isc_start_mode.isc_grab_start_mode.isc_record_mode == IscRecordMode::kRecordOn) {
			is_recording = true;
		}
		else if (isc_control->isc_start_mode.isc_grab_start_mode.isc_play_mode == IscPlayMode::kPlayOn) {
			is_play = true;
		}
		else {
			is_streaming = true;
		}
	}

	if (is_start) {
		if (is_streaming) {
			// IDC_BUTTON2 start/stop
			GetDlgItem(IDC_BUTTON2)->SetWindowText(_T("Stop"));

			// IDC_BUTTON3 Record
			(GetDlgItem(IDC_BUTTON3))->EnableWindow(FALSE);

			// IDC_BUTTON6 Play
			(GetDlgItem(IDC_BUTTON6))->EnableWindow(FALSE);
		}
		else if (is_recording) {
			// IDC_BUTTON2 start/stop
			(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);

			// IDC_BUTTON3 Record
			GetDlgItem(IDC_BUTTON3)->SetWindowText(_T("Stop"));

			// IDC_BUTTON6 Play
			(GetDlgItem(IDC_BUTTON6))->EnableWindow(FALSE);

			// IDC_BUTTON8 Bloack Matching Parameter
			(GetDlgItem(IDC_BUTTON8))->EnableWindow(FALSE);

			// IDC_BUTTON9 Frame Decorder Parameter
			(GetDlgItem(IDC_BUTTON9))->EnableWindow(FALSE);
		}
		else if (is_play) {
			// IDC_BUTTON2 start/stop
			(GetDlgItem(IDC_BUTTON2))->EnableWindow(FALSE);

			// IDC_BUTTON3 Record
			(GetDlgItem(IDC_BUTTON3))->EnableWindow(FALSE);

			// IDC_BUTTON6 Play
			GetDlgItem(IDC_BUTTON6)->SetWindowText(_T("Stop"));
		}

		// IDC_COMBO1 display mode
		(GetDlgItem(IDC_COMBO1))->EnableWindow(FALSE);

		// IDC_COMBO2 draw distance mode
		(GetDlgItem(IDC_COMBO2))->EnableWindow(FALSE);

		// IDC_BUTTON4 Manual calibration
		(GetDlgItem(IDC_BUTTON4))->EnableWindow(TRUE);

		// IDC_BUTTON7 save image
		(GetDlgItem(IDC_BUTTON7))->EnableWindow(TRUE);

		// IDC_CHECK1   Disparity
		(GetDlgItem(IDC_CHECK1))->EnableWindow(FALSE);

		// IDC_CHECK2   Base Image
		(GetDlgItem(IDC_CHECK2))->EnableWindow(FALSE);

		// IDC_CHECK5   Base Image Correct
		(GetDlgItem(IDC_CHECK5))->EnableWindow(FALSE);

		// IDC_CHECK3   Matching Image
		(GetDlgItem(IDC_CHECK3))->EnableWindow(FALSE);

		// IDC_CHECK6   Matching Image Correct
		(GetDlgItem(IDC_CHECK6))->EnableWindow(FALSE);

		// IDC_CHECK4   Color Image
		(GetDlgItem(IDC_CHECK4))->EnableWindow(FALSE);

		// IDC_CHECK7   Color Image Correct
		(GetDlgItem(IDC_CHECK7))->EnableWindow(FALSE);

		// IDC_CHECK16  Data Processing Block Matching
		(GetDlgItem(IDC_CHECK16))->EnableWindow(FALSE);

		// IDC_CHECK15  Data Processing Frame Decoder
		(GetDlgItem(IDC_CHECK15))->EnableWindow(FALSE);

		// IDC_CHECK17  Data Processing Frame Decoder
		(GetDlgItem(IDC_CHECK17))->EnableWindow(FALSE);

		// IDC_BUTTON5 Adcanced Setting
		(GetDlgItem(IDC_BUTTON5))->EnableWindow(FALSE);

		// IDC_STATIC_CAMERASTATUS Camera Status
		GetDlgItem(IDC_STATIC_CAMERASTATUS)->SetWindowText(_T("CAMERA STATUS: RUNNING"));

		if (isc_dpl_configuration_.enabled_data_proc_module) {
			// IDC_STATIC_DP_STATUS Data Processing Status
			if (isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching ||
				isc_control->isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter) {
				GetDlgItem(IDC_STATIC_DP_STATUS)->SetWindowText(_T("DP STATUS: RUNNING"));
			}
		}
	}
	else {
		// IDC_BUTTON2 start/stop
		(GetDlgItem(IDC_BUTTON2))->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON2)->SetWindowText(_T("Live"));

		// Record
		(GetDlgItem(IDC_BUTTON3))->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON3)->SetWindowText(_T("Record"));

		// Play
		(GetDlgItem(IDC_BUTTON6))->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON6)->SetWindowText(_T("Play"));

		// IDC_COMBO1 display mode
		(GetDlgItem(IDC_COMBO1))->EnableWindow(TRUE);

		// IDC_COMBO2 draw distance mode
		(GetDlgItem(IDC_COMBO2))->EnableWindow(TRUE);

		// IDC_BUTTON4 Manual calibration
		(GetDlgItem(IDC_BUTTON4))->EnableWindow(FALSE);

		// IDC_BUTTON7 save image
		(GetDlgItem(IDC_BUTTON7))->EnableWindow(FALSE);

		// IDC_CHECK1   Disparity
		(GetDlgItem(IDC_CHECK1))->EnableWindow(TRUE);

		// IDC_CHECK2   Base Image
		(GetDlgItem(IDC_CHECK2))->EnableWindow(TRUE);

		// IDC_CHECK5   Base Image Correct
		(GetDlgItem(IDC_CHECK5))->EnableWindow(TRUE);

		// IDC_CHECK3   Matching Image
		(GetDlgItem(IDC_CHECK3))->EnableWindow(TRUE);

		// IDC_CHECK6   Matching Image Correct
		(GetDlgItem(IDC_CHECK6))->EnableWindow(TRUE);

		// IDC_CHECK4   Color Image
		bool is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kColorImage);
		if (is_enabled) {
			(GetDlgItem(IDC_CHECK4))->EnableWindow(TRUE);
		}
		else {
			(GetDlgItem(IDC_CHECK4))->EnableWindow(FALSE);
		}

		// IDC_CHECK7   Color Image Correct
		is_enabled = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kColorImageCorrect);
		if (is_enabled) {
			(GetDlgItem(IDC_CHECK7))->EnableWindow(TRUE);
		}
		else {
			(GetDlgItem(IDC_CHECK7))->EnableWindow(FALSE);
		}

		// IDC_CHECK16  Data Processing Block Matching and Frame Decoder
		is_enabled = isc_dpl_configuration_.enabled_data_proc_module;
		if (is_enabled) {
			(GetDlgItem(IDC_CHECK16))->EnableWindow(TRUE);
			(GetDlgItem(IDC_CHECK15))->EnableWindow(TRUE);
		}
		else {
			(GetDlgItem(IDC_CHECK16))->EnableWindow(FALSE);
			(GetDlgItem(IDC_CHECK15))->EnableWindow(FALSE);
		}

		// IDC_CHECK17  Data Processing Frame Decoder
		(GetDlgItem(IDC_CHECK17))->EnableWindow(TRUE);

		// IDC_BUTTON5 Adcanced Setting
		(GetDlgItem(IDC_BUTTON5))->EnableWindow(TRUE);

		// IDC_STATIC_CAMERASTATUS Camera Status
		GetDlgItem(IDC_STATIC_CAMERASTATUS)->SetWindowText(_T("CAMERA STATUS: STOP"));

		// IDC_STATIC_CAMERA_FPS Camera speed
		GetDlgItem(IDC_STATIC_CAMERA_FPS)->SetWindowText(_T("0 FPS"));

		// IDC_STATIC_DP_STATUS Data Processing Status
		GetDlgItem(IDC_STATIC_DP_STATUS)->SetWindowText(_T("DP STATUS: STOP"));

		// IDC_STATIC_DP_FPS Data Processing speed
		GetDlgItem(IDC_STATIC_DP_FPS)->SetWindowText(_T("0 FPS"));

		// IDC_BUTTON8 Bloack Matching Parameter
		(GetDlgItem(IDC_BUTTON8))->EnableWindow(TRUE);

		// IDC_BUTTON9 Frame Decorder Parameter
		(GetDlgItem(IDC_BUTTON9))->EnableWindow(TRUE);
	}

	return true;
}

void DebugOutFrameTimeMsg(const int frame_no, const __int64 frame_time)
{
	ULARGE_INTEGER ul_int = {};
	ul_int.QuadPart = frame_time;
	struct timespec tm = {};
	tm.tv_sec = ul_int.QuadPart / 1000LL;
	tm.tv_nsec = static_cast<long>((ul_int.QuadPart - (tm.tv_sec * 1000LL)) * 1000000);

	struct tm ltm;
	localtime_s(&ltm, &tm.tv_sec);
	long millisecond = tm.tv_nsec / 1000000LL;

	char time_str[256] = {};
	char dayofweek[7][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	sprintf_s(time_str, "%d: %d/%d/%d %s %02d:%02d:%02d.%03d\n",
		frame_no,
		ltm.tm_year + 1900,
		ltm.tm_mon + 1,
		ltm.tm_mday,
		dayofweek[ltm.tm_wday],
		ltm.tm_hour,
		ltm.tm_min,
		ltm.tm_sec,
		millisecond);

	OutputDebugStringA(time_str);

	return;
}

bool CDPCguiDlg::ImageCaptureProc()
{
	isc_control_.is_isc_image_info_valid = false;
	isc_control_.is_data_proc_result_valid = false;

	// camera images
	DPL_RESULT dpl_result = isc_dpl_->GetCameraData(&isc_control_.isc_image_info);
	if (dpl_result != DPC_E_OK) {
		return false;
	}

	// throw away previous data
	if (isc_control_.isc_start_mode.isc_grab_start_mode.isc_grab_mode != isc_control_.isc_image_info.grab) {
		return false;
	}

	isc_control_.is_isc_image_info_valid = true;

	if (0) {
		OutputDebugStringA("[INFO]Camera Time\n");
		DebugOutFrameTimeMsg(isc_control_.isc_image_info.frame_data[0].frameNo, isc_control_.isc_image_info.frame_data[0].frame_time);
	}

	// deta processing result
	dpl_result = isc_dpl_->GetDataProcModuleData(&isc_control_.isc_data_proc_result_data);
	if (dpl_result == DPC_E_OK) {
		isc_control_.is_data_proc_result_valid = true;

		if (0) {
			OutputDebugStringA("[INFO]Data Proc Time\n");
			DebugOutFrameTimeMsg(isc_control_.isc_data_proc_result_data.isc_image_info.frame_data[0].frameNo, isc_control_.isc_data_proc_result_data.isc_image_info.frame_data[0].frame_time);
		}
	}

	return true;
}

bool CDPCguiDlg::ImageCaptureProcForPlay()
{
	isc_control_.is_isc_image_info_valid = false;
	isc_control_.is_data_proc_result_valid = false;

	if (isc_control_.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching ||
		isc_control_.isc_start_mode.isc_dataproc_start_mode.enabled_disparity_filter) {

		// camera images
		DPL_RESULT dpl_result = isc_dpl_->GetCameraData(&isc_control_.isc_image_info);
		if (dpl_result != DPC_E_OK) {
			return false;
		}

		// throw away previous data
		if (isc_control_.isc_start_mode.isc_grab_start_mode.isc_grab_mode != isc_control_.isc_image_info.grab) {
			return false;
		}

		isc_control_.is_isc_image_info_valid = true;

		// debug
		if (0) {
			DebugOutFrameTimeMsg(isc_control_.isc_image_info.frame_data[0].frameNo, isc_control_.isc_image_info.frame_data[0].frame_time);
		}

		// deta processing result
		dpl_result = isc_dpl_->GetDataProcModuleData(&isc_control_.isc_data_proc_result_data);
		if (dpl_result == DPC_E_OK) {
			isc_control_.is_data_proc_result_valid = true;
		}
		else {
			return false;
		}

		// throw away previous data
		if (isc_control_.isc_start_mode.isc_grab_start_mode.isc_grab_mode != isc_control_.isc_data_proc_result_data.isc_image_info.grab) {
			return false;
		}

	}
	else {
		// camera images
		DPL_RESULT dpl_result = isc_dpl_->GetCameraData(&isc_control_.isc_image_info);
		if (dpl_result != DPC_E_OK) {
			return false;
		}

		// throw away previous data
		if (isc_control_.isc_start_mode.isc_grab_start_mode.isc_grab_mode != isc_control_.isc_image_info.grab) {
			return false;
		}

		isc_control_.is_isc_image_info_valid = true;

		// debug
		if (0) {
			DebugOutFrameTimeMsg(isc_control_.isc_image_info.frame_data[0].frameNo, isc_control_.isc_image_info.frame_data[0].frame_time);
		}

		// deta processing result
		dpl_result = isc_dpl_->GetDataProcModuleData(&isc_control_.isc_data_proc_result_data);
		if (dpl_result == DPC_E_OK) {
			isc_control_.is_data_proc_result_valid = true;
		}
	}

	return true;
}

static LARGE_INTEGER tact_previous = {}, tact_current = {};
static double tact_time_list[100] = {};
static bool tact_time_count_over = false;
static int tact_time_list_count = 0;

bool CDPCguiDlg::ImageDrawProc()
{
	// debug --------
	QueryPerformanceCounter(&tact_current);
	double elapsed_time_1 = (double)((double)((tact_current.QuadPart - tact_previous.QuadPart) * 1000.0) / (double)performance_freq_.QuadPart);
	QueryPerformanceCounter(&tact_previous);
	tact_time_list[tact_time_list_count] = elapsed_time_1;
	tact_time_list_count++;
	if (tact_time_list_count >= 100) {
		tact_time_list_count = 0;
		if (!tact_time_count_over) {
			tact_time_count_over = true;
		}
	}
	if (tact_time_count_over) {
		double average_time = std::accumulate(&tact_time_list[0], &tact_time_list[99], 0.0F);
		average_time /= 100;

		//TCHAR tact_msg[128] = {};
		//_stprintf_s(tact_msg, _T("[INFO]Draw Tact Time=%.1f\n"), average_time);
		//OutputDebugString(tact_msg);
	}
	// debug --------

	// draw mode setup
	IscFeatureRequest isc_feature_request = {};

	isc_feature_request.is_disparity = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	isc_feature_request.is_mono_s0_image = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	isc_feature_request.is_mono_s0_image_correct = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	isc_feature_request.is_mono_s1_image = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	isc_feature_request.is_mono_s1_image_correct = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	isc_feature_request.is_color_image = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	isc_feature_request.is_color_image_correct = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	isc_feature_request.is_dpl_stereo_matching = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	isc_feature_request.is_dpl_disparity_filter = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;
	if (isc_feature_request.is_dpl_stereo_matching || isc_feature_request.is_dpl_disparity_filter) {
		isc_feature_request.is_dpl_frame_decoder = true;
	}

	DpcDrawLib::ImageDrawMode mode = GetDrawMode(&isc_feature_request, &isc_control_);

	// setup draw image buffer
	bool is_dpresult_mode = false;
	bool setup_result = SetupDrawImageDataSet(mode, &isc_control_, &image_data_set_[0], &image_data_set_[1], &is_dpresult_mode);

	// 文字用
	DpcDrawLib::TextDataSet text_data_set = {};

	// 選択位置を表示
	int setindex = 0;
	for (int i = 0; i < 3; i++) {
		if (mouse_operation_control_.mouse_position_pick_information[i].valid) {
			POINT txt_position = mouse_operation_control_.mouse_position_pick_information[i].position_at_client;

			text_data_set.count++;
			text_data_set.text_data[setindex].x = txt_position.x;
			text_data_set.text_data[setindex].y = txt_position.y;
			_stprintf_s(text_data_set.text_data[setindex].string, _T("V%d"), i);

			setindex++;
		}
	}

	// 矩形用
	DpcDrawLib::RectDataSet rect_data_set;
	rect_data_set.count = 0;

	if (mouse_operation_control_.is_set_rect_event_request && (mouse_operation_control_.set_rect_event_state == 1)) {
		// 途中経過
		rect_data_set.count = 1;
		rect_data_set.rect_data[0].top = mouse_operation_control_.mouse_rect_information[0].rect_at_client.top;
		rect_data_set.rect_data[0].bottom = mouse_operation_control_.mouse_rect_information[0].rect_at_client.bottom;
		rect_data_set.rect_data[0].left = mouse_operation_control_.mouse_rect_information[0].rect_at_client.left;
		rect_data_set.rect_data[0].right = mouse_operation_control_.mouse_rect_information[0].rect_at_client.right;
	}
	else if(mouse_operation_control_.mouse_rect_information[0].valid ) {
		rect_data_set.count = 1;
		rect_data_set.rect_data[0].top = mouse_operation_control_.mouse_rect_information[0].rect_at_client.top;
		rect_data_set.rect_data[0].bottom = mouse_operation_control_.mouse_rect_information[0].rect_at_client.bottom;
		rect_data_set.rect_data[0].left = mouse_operation_control_.mouse_rect_information[0].rect_at_client.left;
		rect_data_set.rect_data[0].right = mouse_operation_control_.mouse_rect_information[0].rect_at_client.right;
	}

	// パラメータ設定
	DpcDrawLib::DrawParameter draw_parameter;

	if (isc_control_.draw_settings.disparity_mode == DisplayModeDepth::kDistance) {
		draw_parameter.depth_draw_distance = true;
	}
	else {
		draw_parameter.depth_draw_distance = false;
	}
	draw_parameter.draw_outside_bounds = dpl_gui_configuration_->IsDrawOutsideBounds();

	draw_parameter.save_image_request = isc_control_.one_shot_save_request;
	// Clear to indicate that it is a one time only
	isc_control_.one_shot_save_request = false;

	draw_parameter.magnification		= isc_control_.draw_settings.magnification;
	draw_parameter.magnification_center	= isc_control_.draw_settings.magnification_center;

	draw_parameter.camera_b			= isc_control_.isc_image_info.camera_specific_parameter.base_length;	// = isc_control_.camera_parameter.b;
	draw_parameter.camera_dinf		= isc_control_.isc_image_info.camera_specific_parameter.d_inf;			// = isc_control_.camera_parameter.dinf;
	draw_parameter.camera_bf		= isc_control_.isc_image_info.camera_specific_parameter.bf;				// = isc_control_.camera_parameter.bf;
	draw_parameter.camera_set_angle = 0;																	// = isc_control_.camera_parameter.setup_angle;

	// 描画開始
	CDC* cdc1 = GetDlgItem(IDC_PIC1)->GetDC();
	HDC hdc1 = cdc1->GetSafeHdc();
	RECT wrect = {};
	GetDlgItem(IDC_PIC1)->GetWindowRect(&wrect);

	RECT rect1 = {};
	rect1.top = 0;
	rect1.bottom = wrect.bottom - wrect.top;
	rect1.left = 0;
	rect1.right = wrect.right - wrect.left;

	// 未使用
	RECT rect2 = {};

	// 描画
	draw_data_lib_->Render(hdc1, &rect1, NULL, &rect2, &image_data_set_[0], &image_data_set_[1], &text_data_set, &rect_data_set, &draw_parameter);
	GetDlgItem(IDC_PIC1)->ReleaseDC(cdc1);

	// カメラの状態を表示
	const int fd_index = kISCIMAGEINFO_FRAMEDATA_LATEST;

	TCHAR status_msg_error[256] = {};
	if (isc_control_.isc_image_info.frame_data[fd_index].camera_status.error_code == 0) {
		_stprintf_s(status_msg_error, _T("CAMERA: -----"));
	}
	else {
		_stprintf_s(status_msg_error, _T("CAMERA: ERROR CODE(%d)"), isc_control_.isc_image_info.frame_data[fd_index].camera_status.error_code);
	}
	GetDlgItem(IDC_STATIC_CAMERA_ERROR_STATUS)->SetWindowText(status_msg_error);

	TCHAR status_msg_tact[256] = {};
	if (isc_control_.isc_image_info.frame_data[fd_index].camera_status.data_receive_tact_time > 0) {
		double current_fps = 1000.0f / isc_control_.isc_image_info.frame_data[fd_index].camera_status.data_receive_tact_time;
		_stprintf_s(status_msg_tact, _T("%d FPS"), (int)current_fps);
	}
	else {
		_stprintf_s(status_msg_tact, _T("0 FPS"));
	}
	GetDlgItem(IDC_STATIC_CAMERA_FPS)->SetWindowText(status_msg_tact);

	// マウス位置の情報
	if (mouse_operation_control_.mouse_position_real_time_monitor.valid) {
		POINT image_position = {};

		draw_data_lib_->ScreenPostionToDrawImagePosition(mouse_operation_control_.mouse_position_real_time_monitor.position_at_client, &image_position);
		
		POINT image_position_on_original_image = {};
		int currently_selected_index = -1;
		draw_data_lib_->ScreenPostionToImagePosition(mouse_operation_control_.mouse_position_real_time_monitor.position_at_client, &image_position_on_original_image, &currently_selected_index);

		IscImageInfo* isc_image_info = &isc_control_.isc_image_info;
		TCHAR disparity_src_string[16] = {};
		_stprintf_s(disparity_src_string, _T("CAMERA"));

		if (is_dpresult_mode) {
			if (isc_image_info->grab == IscGrabMode::kParallax) {
				if (mode == DpcDrawLib::ImageDrawMode::kDplDepthDepth) {
					if (currently_selected_index == 0) {
						isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
						if (isc_control_.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching) {
							_stprintf_s(disparity_src_string, _T("SOFT-MATCH"));
						}
					}
				}
				else {
					isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
					if (isc_control_.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching) {
						_stprintf_s(disparity_src_string, _T("SOFT-MATCH"));
					}
				}
			}
			else if (isc_image_info->grab == IscGrabMode::kCorrect) {
				isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
				if (isc_control_.isc_start_mode.isc_dataproc_start_mode.enabled_stereo_matching) {
					_stprintf_s(disparity_src_string, _T("SOFT-MATCH"));
				}
			}
			else {
				// none, it may mode error
			}
		}

		POINT position_on_depth_image = {};
		draw_data_lib_->ScreenPostionToDepthImagePosition(mouse_operation_control_.mouse_position_real_time_monitor.position_at_client, &position_on_depth_image);

		float disparity = 0, depth = 0;
		int get_success = isc_dpl_->GetPositionDepth(position_on_depth_image.x, position_on_depth_image.y, isc_image_info, &disparity, &depth);
		if (get_success != DPC_E_OK) {
			disparity = 0;
			depth = 0;
		}

		float x_d = 0, y_d = 0, z_d = 0;
		get_success = isc_dpl_->GetPosition3D(position_on_depth_image.x, position_on_depth_image.y, isc_image_info, &x_d, &y_d, &z_d);
		float xr_d = 0, yr_d = 0, zr_d = 0;
		if (get_success == DPC_E_OK) {
			draw_data_lib_->Image3DPositionToScreenPostion(x_d, y_d, z_d, &xr_d, &yr_d, &zr_d);
		}

		TCHAR grab_mmode_string[32] = {};
		GetGrabModeString(&isc_control_.isc_image_info, grab_mmode_string, 32);

		TCHAR msg[256] = {};
		_stprintf_s(msg, _T("GRAB_MODE(%s) SCREEN(%d,%d) IMAGE(%d)(%d,%d) IMAGE-ORG(%d,%d) D:%.3f X:%.3f Y:%.3f Z:%.3f (from %s)"),
			grab_mmode_string,
			mouse_operation_control_.mouse_position_real_time_monitor.position_at_client.x,
			mouse_operation_control_.mouse_position_real_time_monitor.position_at_client.y,
			currently_selected_index,
			image_position.x,
			image_position.y,
			image_position_on_original_image.x,
			image_position_on_original_image.y,
			disparity,
			xr_d,
			yr_d,
			zr_d,
			disparity_src_string);
		GetDlgItem(IDC_STATIC_MOUSE_POS_INFO)->SetWindowText(msg);
	}

	// 選択位置の情報
	if (mouse_operation_control_.mouse_position_pick_information[0].valid) {
		POINT image_position = mouse_operation_control_.mouse_position_pick_information[0].position_at_image;

		POINT position_on_depth_image = mouse_operation_control_.mouse_position_pick_information[0].position_at_depth_image;
		int currently_selected_index = mouse_operation_control_.mouse_position_pick_information[0].currently_selected_index;

		IscImageInfo* isc_image_info = &isc_control_.isc_image_info;
		if (is_dpresult_mode) {
			if (isc_image_info->grab == IscGrabMode::kParallax) {
				if (mode == DpcDrawLib::ImageDrawMode::kDplDepthDepth) {
					if (currently_selected_index == 0) {
						isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
					}
				}
				else {
					isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
				}
			}
			else if (isc_image_info->grab == IscGrabMode::kCorrect) {
				isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
			}
			else {
				// none, it may mode error
			}
		}

		float disparity = 0, depth = 0;
		int get_success = isc_dpl_->GetPositionDepth(position_on_depth_image.x, position_on_depth_image.y, isc_image_info, &disparity, &depth);
		if (get_success != DPC_E_OK) {
			disparity = 0;
			depth = 0;
		}

		TCHAR str[64] = {};
		_stprintf_s(str, _T("(%d,%d)"), image_position.x, image_position.y);
		GetDlgItem(IDC_STATIC_IP0_XY)->SetWindowText(str);

		float x_d = 0, y_d = 0, z_d = 0;
		get_success = isc_dpl_->GetPosition3D(position_on_depth_image.x, position_on_depth_image.y, isc_image_info, &x_d, &y_d, &z_d);
		float xr_d = 0, yr_d = 0, zr_d = 0;
		if (get_success == DPC_E_OK) {
			draw_data_lib_->Image3DPositionToScreenPostion(x_d, y_d, z_d, &xr_d, &yr_d, &zr_d);

			_stprintf_s(str, _T("%.3f"), disparity);
			GetDlgItem(IDC_STATIC_IP0_DISP)->SetWindowText(str);

			_stprintf_s(str, _T("(%.2f,%.2f,%.2f)m"), xr_d, yr_d, zr_d);
			GetDlgItem(IDC_STATIC_IP0_XYZ)->SetWindowText(str);
		}
		else {
			GetDlgItem(IDC_STATIC_IP0_DISP)->SetWindowText(_T("---"));
			GetDlgItem(IDC_STATIC_IP0_XYZ)->SetWindowText(_T("---"));
		}
	}
	else {
		GetDlgItem(IDC_STATIC_IP0_XY)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_IP0_DISP)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_IP0_XYZ)->SetWindowText(_T(""));
	}

	if (mouse_operation_control_.mouse_position_pick_information[1].valid) {
		POINT image_position = mouse_operation_control_.mouse_position_pick_information[1].position_at_image;

		POINT position_on_depth_image = mouse_operation_control_.mouse_position_pick_information[1].position_at_depth_image;
		int currently_selected_index = mouse_operation_control_.mouse_position_pick_information[1].currently_selected_index;

		IscImageInfo* isc_image_info = &isc_control_.isc_image_info;
		if (is_dpresult_mode) {
			if (isc_image_info->grab == IscGrabMode::kParallax) {
				if (mode == DpcDrawLib::ImageDrawMode::kDplDepthDepth) {
					if (currently_selected_index == 0) {
						isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
					}
				}
				else {
					isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
				}
			}
			else if (isc_image_info->grab == IscGrabMode::kCorrect) {
				isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
			}
			else {
				// none, it may mode error
			}
		}

		float disparity = 0, depth = 0;
		int get_success = isc_dpl_->GetPositionDepth(position_on_depth_image.x, position_on_depth_image.y, isc_image_info, &disparity, &depth);
		if (get_success != DPC_E_OK) {
			disparity = 0;
			depth = 0;
		}

		TCHAR str[64] = {};
		_stprintf_s(str, _T("(%d,%d)"), image_position.x, image_position.y);
		GetDlgItem(IDC_STATIC_IP1_XY)->SetWindowText(str);

		float x_d = 0, y_d = 0, z_d = 0;
		get_success = isc_dpl_->GetPosition3D(position_on_depth_image.x, position_on_depth_image.y, isc_image_info, &x_d, &y_d, &z_d);
		float xr_d = 0, yr_d = 0, zr_d = 0;
		if (get_success == DPC_E_OK) {
			draw_data_lib_->Image3DPositionToScreenPostion(x_d, y_d, z_d, &xr_d, &yr_d, &zr_d);

			_stprintf_s(str, _T("%.3f"), disparity);
			GetDlgItem(IDC_STATIC_IP1_DISP)->SetWindowText(str);

			_stprintf_s(str, _T("(%.2f,%.2f,%.2f)m"), xr_d, yr_d, zr_d);
			GetDlgItem(IDC_STATIC_IP1_XYZ)->SetWindowText(str);
		}
		else {
			GetDlgItem(IDC_STATIC_IP1_DISP)->SetWindowText(_T(""));
			GetDlgItem(IDC_STATIC_IP1_XYZ)->SetWindowText(_T(""));
		}
	}
	else {
		GetDlgItem(IDC_STATIC_IP1_XY)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_IP1_DISP)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_IP1_XYZ)->SetWindowText(_T(""));
	}

	if (mouse_operation_control_.mouse_position_pick_information[2].valid) {
		POINT image_position = mouse_operation_control_.mouse_position_pick_information[2].position_at_image;

		POINT position_on_depth_image = mouse_operation_control_.mouse_position_pick_information[2].position_at_depth_image;
		int currently_selected_index = mouse_operation_control_.mouse_position_pick_information[2].currently_selected_index;

		IscImageInfo* isc_image_info = &isc_control_.isc_image_info;
		if (is_dpresult_mode) {
			if (isc_image_info->grab == IscGrabMode::kParallax) {
				if (mode == DpcDrawLib::ImageDrawMode::kDplDepthDepth) {
					if (currently_selected_index == 0) {
						isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
					}
				}
				else {
					isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
				}
			}
			else if (isc_image_info->grab == IscGrabMode::kCorrect) {
				isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
			}
			else {
				// none, it may mode error
			}
		}

		float disparity = 0, depth = 0;
		int get_success = isc_dpl_->GetPositionDepth(position_on_depth_image.x, position_on_depth_image.y, isc_image_info, &disparity, &depth);
		if (get_success != DPC_E_OK) {
			disparity = 0;
			depth = 0;
		}

		TCHAR str[64] = {};
		_stprintf_s(str, _T("(%d,%d)"), image_position.x, image_position.y);
		GetDlgItem(IDC_STATIC_IP2_XY)->SetWindowText(str);

		float x_d = 0, y_d = 0, z_d = 0;
		get_success = isc_dpl_->GetPosition3D(position_on_depth_image.x, position_on_depth_image.y, isc_image_info, &x_d, &y_d, &z_d);
		float xr_d = 0, yr_d = 0, zr_d = 0;
		if (get_success == DPC_E_OK) {
			draw_data_lib_->Image3DPositionToScreenPostion(x_d, y_d, z_d, &xr_d, &yr_d, &zr_d);

			_stprintf_s(str, _T("%.3f"), disparity);
			GetDlgItem(IDC_STATIC_IP2_DISP)->SetWindowText(str);

			_stprintf_s(str, _T("(%.2f,%.2f,%.2f)m"), xr_d, yr_d, zr_d);
			GetDlgItem(IDC_STATIC_IP2_XYZ)->SetWindowText(str);
		}
		else {
			GetDlgItem(IDC_STATIC_IP2_DISP)->SetWindowText(_T(""));
			GetDlgItem(IDC_STATIC_IP2_XYZ)->SetWindowText(_T(""));
		}
	}
	else {
		GetDlgItem(IDC_STATIC_IP2_XY)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_IP2_DISP)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_IP2_XYZ)->SetWindowText(_T(""));
	}

	if (mouse_operation_control_.mouse_rect_information[0].valid) {
		POINT image_position[2] = { {},{} };
		image_position[0].y = mouse_operation_control_.mouse_rect_information[0].rect_at_image.top;
		image_position[0].x = mouse_operation_control_.mouse_rect_information[0].rect_at_image.left;
		image_position[1].y = mouse_operation_control_.mouse_rect_information[0].rect_at_image.bottom;
		image_position[1].x = mouse_operation_control_.mouse_rect_information[0].rect_at_image.right;

		POINT position_on_depth_image[2] = { {}, {} };
		position_on_depth_image[0].y = mouse_operation_control_.mouse_rect_information[0].rect_at_depth_image.top;
		position_on_depth_image[0].x = mouse_operation_control_.mouse_rect_information[0].rect_at_depth_image.left;
		position_on_depth_image[1].y = mouse_operation_control_.mouse_rect_information[0].rect_at_depth_image.bottom;
		position_on_depth_image[1].x = mouse_operation_control_.mouse_rect_information[0].rect_at_depth_image.right;

		int currently_selected_index[2] = { -1, -1 };
		currently_selected_index[0] = mouse_operation_control_.mouse_rect_information[0].currently_selected_index[0];
		currently_selected_index[1] = mouse_operation_control_.mouse_rect_information[0].currently_selected_index[1];

		TCHAR str[64] = {};
		_stprintf_s(str, _T("(%d,%d)->(%d,%d)"), image_position[0].x, image_position[0].y, image_position[1].x, image_position[1].y);
		GetDlgItem(IDC_STATIC_IA0_XY)->SetWindowText(str);

		int roi_width = 0;
		int roi_x = 0;
		if (position_on_depth_image[1].x > position_on_depth_image[0].x) {
			roi_width = position_on_depth_image[1].x - position_on_depth_image[0].x;
			roi_x = position_on_depth_image[0].x;
		}
		else {
			roi_width = position_on_depth_image[0].x - position_on_depth_image[1].x;
			roi_x = position_on_depth_image[1].x;
		}
		int roi_height = 0;
		int roi_y = 0;
		if (position_on_depth_image[1].y > position_on_depth_image[0].y) {
			roi_height = position_on_depth_image[1].y - position_on_depth_image[0].y;
			roi_y = position_on_depth_image[0].y;
		}
		else {
			roi_height = position_on_depth_image[0].y - position_on_depth_image[1].y;
			roi_y = position_on_depth_image[1].y;
		}

		IscImageInfo* isc_image_info = &isc_control_.isc_image_info;
		if (is_dpresult_mode) {
			if (isc_image_info->grab == IscGrabMode::kParallax) {
				if (mode == DpcDrawLib::ImageDrawMode::kDplDepthDepth) {
					if (currently_selected_index[0] == 0) {
						isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
					}
				}
				else {
					isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
				}
			}
			else if (isc_image_info->grab == IscGrabMode::kCorrect) {
				isc_image_info = &isc_control_.isc_data_proc_result_data.isc_image_info;
			}
			else {
				// none, it may mode error
			}
		}

		IscAreaDataStatistics isc_data_statistics = {};
		double min_distance = 0, max_distance = 0;
		draw_data_lib_->GetMinMaxDistance(&min_distance, &max_distance);
		isc_data_statistics.min_distance = (float)min_distance;
		isc_data_statistics.max_distance = (float)max_distance;

		int get_success = isc_dpl_->GetAreaStatistics(roi_x, roi_y, roi_width, roi_height, isc_image_info, &isc_data_statistics);
		if (get_success == DPC_E_OK) {
			_stprintf_s(str, _T("%.3f"), isc_data_statistics.statistics_depth.average);
			GetDlgItem(IDC_STATIC_IA0_DISP)->SetWindowText(str);

			_stprintf_s(str, _T("(%.2f,%.2f,%.2f)m"), isc_data_statistics.roi_3d.width, isc_data_statistics.roi_3d.height, isc_data_statistics.roi_3d.distance);
			GetDlgItem(IDC_STATIC_IA0_WHZ)->SetWindowText(str);
		}
		else {
			GetDlgItem(IDC_STATIC_IA0_DISP)->SetWindowText(_T("---"));
			GetDlgItem(IDC_STATIC_IA0_WHZ)->SetWindowText(_T("---"));
		}
	}
	else {
		GetDlgItem(IDC_STATIC_IA0_XY)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_IA0_DISP)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_IA0_WHZ)->SetWindowText(_T(""));

	}

	// data Processingの情報
	if (isc_dpl_configuration_.enabled_data_proc_module) {
		TCHAR dp_status_msg[256] = {};

		double dp_fps = (isc_control_.isc_data_proc_result_data.status.proc_tact_time > 0)? (1000.0F / isc_control_.isc_data_proc_result_data.status.proc_tact_time) : 0;
		_stprintf_s(dp_status_msg, _T("%d FPS"), (int)dp_fps);

		GetDlgItem(IDC_STATIC_DP_FPS)->SetWindowText(dp_status_msg);
	}

	// 画像保存時のパラメータ保存
	if (draw_parameter.save_image_request) {
		SaveDPLparameterFileToImageFolder();
	}

	// how to use display classes
	if (0) {	
		// for example 
		image_data_set_[0].valid = true;
		image_data_set_[0].mode = DpcDrawLib::ImageDrawMode::kMonoS0;

		image_data_set_[0].image_data_list[0].image_mono_s0.width = 752;
		image_data_set_[0].image_data_list[0].image_mono_s0.height = 480;
		image_data_set_[0].image_data_list[0].image_mono_s0.channel_count = 1;
		memset(image_data_set_[0].image_data_list[0].image_mono_s0.buffer, 0, image_data_set_[0].image_data_list[0].image_mono_s0.width * image_data_set_[0].image_data_list[0].image_mono_s0.height);

		for (int i = 0; i < image_data_set_[0].image_data_list[0].image_mono_s0.height; i++) {
			unsigned char* dst = image_data_set_[0].image_data_list[0].image_mono_s0.buffer + (i * image_data_set_[0].image_data_list[0].image_mono_s0.width);

			unsigned char value = (i * 10) % 255;
			memset(dst, value, image_data_set_[0].image_data_list[0].image_mono_s0.width);
		}

		CDC* cdc1 = GetDlgItem(IDC_PIC1)->GetDC();
		HDC hdc1 = cdc1->GetSafeHdc();
		RECT wrect = {};
		GetDlgItem(IDC_PIC1)->GetWindowRect(&wrect);

		RECT rect1 = {};
		rect1.top = 0;
		rect1.bottom = wrect.bottom - wrect.top;
		rect1.left = 0;
		rect1.right = wrect.right - wrect.left;

		RECT rect2 = {};

		DpcDrawLib::TextDataSet text_data_set;
		text_data_set.count = 0;

		// 矩形用
		DpcDrawLib::RectDataSet rect_data_set;
		rect_data_set.count = 0;

		DpcDrawLib::DrawParameter draw_parameter;
		draw_parameter.depth_draw_distance = true;

		draw_data_lib_->Render(hdc1, &rect1, NULL, &rect2, &image_data_set_[0], &image_data_set_[1], &text_data_set, &rect_data_set, &draw_parameter);

		GetDlgItem(IDC_PIC1)->ReleaseDC(cdc1);
	}

	return true;
}

bool CDPCguiDlg::SaveDPLparameterFileToImageFolder()
{
	TCHAR write_file_name[_MAX_PATH] = {};

	SYSTEMTIME st = {};
	wchar_t date_time_name[_MAX_PATH] = {};
	GetLocalTime(&st);

	// YYYYMMDD_HHMMSS
	_stprintf_s(date_time_name, _T("%04d%02d%02d_%02d%02d%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	int total_module_count = 0;
	isc_dpl_->GetTotalModuleCount(&total_module_count);

	if (total_module_count != 0) {

		TCHAR dpc_module_name[64] = {};
		TCHAR dpc_parameter_file_name[_MAX_PATH] = {};
		TCHAR temp_name[_MAX_PATH] = {};

		for (int i = 0; i < total_module_count; i++) {
			isc_dpl_->GetModuleNameByIndex(i, dpc_module_name, 64);
			isc_dpl_->GetParameterFileName(i, dpc_parameter_file_name, _MAX_PATH);

			TCHAR* src_file_name = PathFindFileName(dpc_parameter_file_name);
			_stprintf_s(temp_name, _T("%s"), src_file_name);
			PathRemoveExtension(temp_name);

			_stprintf_s(write_file_name, _T("%s\\%s-%s.ini"), isc_dpl_configuration_.save_image_path, temp_name, date_time_name);

			BOOL ret = CopyFile(dpc_parameter_file_name, write_file_name, FALSE);
		}
	}

	return true;
}

void CDPCguiDlg::UpdateCb(const int id, const bool is_enabled)
{
	if (is_enabled) {
		((CButton*)GetDlgItem(id))->SetCheck(BST_CHECKED);
	}
	else {
		((CButton*)GetDlgItem(id))->SetCheck(BST_UNCHECKED);
	}

	return;
}

bool CDPCguiDlg::SetupGuiControlDefault(bool enabled_camera)
{
	/*
		IDC_COMBO1	Disparity Setting Dispaly
		IDC_COMBO2	Disparity Setting Depth

		IDC_CHECK16	S/W Stereo Matching
		IDC_CHECK15	Dispality Filter
		IDC_CHECK17	S/W Calibration

		IDC_CHECK1	Dispatity
		IDC_CHECK2	Base Image
		IDC_CHECK5	Base Image Correct
		IDC_CHECK3	Matching Image
		IDC_CHECK6	Matching Image Correct
		IDC_CHECK4	Color Image
		IDC_CHECK7	Color Image Correct

		IDC_COMBO3	Shutter Control Mode
	*/

	int mode = dpl_gui_configuration_->GetGuiLbDisplay();
	((CComboBox*)GetDlgItem(IDC_COMBO1))->SetCurSel(mode);

	mode = dpl_gui_configuration_->GetGuiLbDepth();
	((CComboBox*)GetDlgItem(IDC_COMBO2))->SetCurSel(mode);

	UpdateCb(IDC_CHECK16, dpl_gui_configuration_->IsGuiCbSwStereoMathing());
	UpdateCb(IDC_CHECK15, dpl_gui_configuration_->IsGuiCbDisparityFilter());
	UpdateCb(IDC_CHECK17, dpl_gui_configuration_->IsGuiCbSwCalibration());

	UpdateCb(IDC_CHECK1, dpl_gui_configuration_->IsGuiCbDisparity());
	UpdateCb(IDC_CHECK2, dpl_gui_configuration_->IsGuiCbBaseImage());
	UpdateCb(IDC_CHECK5, dpl_gui_configuration_->IsGuiCbBaseImageCorrected());
	UpdateCb(IDC_CHECK3, dpl_gui_configuration_->IsGuiCbMatchingImage());
	UpdateCb(IDC_CHECK6, dpl_gui_configuration_->IsGuiCbMatchingImageCorrected());
	UpdateCb(IDC_CHECK4, dpl_gui_configuration_->IsGuiCbColorImage());
	UpdateCb(IDC_CHECK7, dpl_gui_configuration_->IsGuiCbColorImageCorrected());

	if (enabled_camera) {
		int mode = dpl_gui_configuration_->GetGuiCmbShutterControlMode();
		((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(mode);
		OnCbnSelchangeCombo3();
	}
	else {
		((CComboBox*)GetDlgItem(IDC_COMBO3))->SetCurSel(0);
	}

	return true;
}

bool CDPCguiDlg::SaveGuiControlDefault()
{

	int mode = ((CComboBox*)GetDlgItem(IDC_COMBO1))->GetCurSel();
	dpl_gui_configuration_->SetGuiLbDisplay(mode);

	mode = ((CComboBox*)GetDlgItem(IDC_COMBO2))->GetCurSel();
	dpl_gui_configuration_->SetGuiLbDepth(mode);


	bool is_enabled = ((CButton*)GetDlgItem(IDC_CHECK16))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbSwStereoMathing(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK15))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbDisparityFilter(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK17))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbSwCalibration(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbDisparity(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbBaseImage(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK5))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbBaseImageCorrected(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbMatchingImage(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK6))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbMatchingImageCorrected(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbColorImage(is_enabled);

	is_enabled = ((CButton*)GetDlgItem(IDC_CHECK7))->GetCheck() == BST_CHECKED ? true : false;
	dpl_gui_configuration_->SetGuiCbColorImageCorrected(is_enabled);

	mode = ((CComboBox*)GetDlgItem(IDC_COMBO3))->GetCurSel();
	dpl_gui_configuration_->SetGuiCmbShutterControlMode(mode);

	// save
	dpl_gui_configuration_->SaveGuiDefault();

	return true;
}

bool CDPCguiDlg::SetupCameraOptions(bool enabled_camera)
{

	if (!enabled_camera) {
		return true;
	}

	// Extended Matciong
	bool is_supported = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kEnableExtendedMatching);
	bool is_writable = isc_dpl_->DeviceOptionIsWritable(IscCameraParameter::kEnableExtendedMatching);
	if (is_supported && is_writable) {
		bool is_enabled = dpl_gui_configuration_->IsOptionExtendedMatching();

		isc_dpl_->DeviceSetOption(IscCameraParameter::kEnableExtendedMatching, is_enabled);
	}

	// Search Range
	is_supported = isc_dpl_->DeviceOptionIsImplemented(IscCameraParameter::kSadSearchRange128);
	is_writable = isc_dpl_->DeviceOptionIsWritable(IscCameraParameter::kSadSearchRange128);
	if (is_supported && is_writable) {
		bool is_enabled = dpl_gui_configuration_->IsOptionSearchRange128();

		isc_dpl_->DeviceSetOption(IscCameraParameter::kSadSearchRange128, is_enabled);
	}

	return true;
}
