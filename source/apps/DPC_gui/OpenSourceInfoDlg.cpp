﻿// OpenSourceInfoDlg.cpp : 実装ファイル
//

#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <locale.h>

#include "DPC_gui.h"
#include "afxdialogex.h"
#include "OpenSourceInfoDlg.h"

constexpr TCHAR open_src_str[] = { _T("using the following open source libraries.\r\n\
\r\n\
A)OpenCV\r\n\
By downloading, copying, installing or using the software you agree to this license.\r\n\
If you do not agree to this license, do not download, install,\r\n\
copy or use the software.\r\n\
\r\n\
\r\n\
                          License Agreement\r\n\
               For Open Source Computer Vision Library\r\n\
                       (3 - clause BSD License)\r\n\
\r\n\
Copyright(C) 2000 - 2020, Intel Corporation, all rights reserved.\r\n\
Copyright(C) 2009 - 2011, Willow Garage Inc., all rights reserved.\r\n\
Copyright(C) 2009 - 2016, NVIDIA Corporation, all rights reserved.\r\n\
Copyright(C) 2010 - 2013, Advanced Micro Devices, Inc., all rights reserved.\r\n\
Copyright(C) 2015 - 2016, OpenCV Foundation, all rights reserved.\r\n\
Copyright(C) 2015 - 2016, Itseez Inc., all rights reserved.\r\n\
Copyright(C) 2019 - 2020, Xperience AI, all rights reserved.\r\n\
Third party copyrights are property of their respective owners.\r\n\
\r\n\
Redistribution and use in source and binary forms, with or without modification,\r\n\
are permitted provided that the following conditions are met :\r\n\
\r\n\
  *Redistributions of source code must retain the above copyright notice,\r\n\
    this list of conditions and the following disclaimer.\r\n\
\r\n\
  * Redistributions in binary form must reproduce the above copyright notice,\r\n\
    this list of conditions and the following disclaimer in the documentation\r\n\
    and /or other materials provided with the distribution.\r\n\
\r\n\
  * Neither the names of the copyright holders nor the names of the contributors\r\n\
    may be used to endorse or promote products derived from this software\r\n\
    without specific prior written permission.\r\n\
\r\n\
This software is provided by the copyright holders and contributors \"as is\" and\r\n\
any express or implied warranties, including, but not limited to, the implied\r\n\
warranties of merchantability and fitness for a particular purpose are disclaimed.\r\n\
In no event shall copyright holders or contributors be liable for any direct,\r\n\
indirect, incidental, special, exemplary, or consequential damages\r\n\
(including, but not limited to, procurement of substitute goods or services;\r\n\
loss of use, data, or profits; or business interruption) however caused\r\n\
and on any theory of liability, whether in contract, strict liability,\r\n\
or tort(including negligence or otherwise) arising in any way out of\r\n\
the use of this software, even if advised of the possibility of such damage.\r\n\
\r\n\
\r\n\
\r\n\
             Copyright(C) 2001 Fabrice Bellard\r\n\
\r\n\
    FFmpeg is free software; you can redistribute it and /or\r\n\
    modify it under the terms of the GNU Lesser General Public\r\n\
    License as published by the Free Software Foundation; either\r\n\
    version 2.1 of the License, or (at your option) any later version.\r\n\
\r\n\
    FFmpeg is distributed in the hope that it will be useful,\r\n\
    but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n\
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU\r\n\
    Lesser General Public License for more details.\r\n\
\r\n\
    You should have received a copy of the GNU Lesser General Public\r\n\
    License along with FFmpeg; if not, write to the Free Software\r\n\
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110 - 1301 USA\r\n\
    ") };

// OpenSourceInfoDlg ダイアログ

IMPLEMENT_DYNAMIC(OpenSourceInfoDlg, CDialogEx)

OpenSourceInfoDlg::OpenSourceInfoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG5, pParent)
{

}

OpenSourceInfoDlg::~OpenSourceInfoDlg()
{
}

void OpenSourceInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(OpenSourceInfoDlg, CDialogEx)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// OpenSourceInfoDlg メッセージ ハンドラー


void OpenSourceInfoDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: ここにメッセージ ハンドラー コードを追加します。

	CString str = open_src_str;
	((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowText(str);

}
