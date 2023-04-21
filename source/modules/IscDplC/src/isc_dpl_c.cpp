// Copyright 2023 ITD Lab Corp.All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file isc_main_control.cpp
 * @brief main control class for ISC_DPL
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 * 
 * @details This class provides a function for ISC_DPL.( C interface)
 */

#include "pch.h"

#include "pch.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <stdint.h>
#include <process.h>
#include <mutex>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_log.h"
#include "utility.h"

#include "isc_image_info_ring_buffer.h"
#include "vm_sdk_wrapper.h"
#include "xc_sdk_wrapper.h"
#include "isc_sdk_control.h"
#include "isc_file_write_control_impl.h"
#include "isc_raw_data_decoder.h"
#include "isc_file_read_control_impl.h"
#include "isc_camera_control.h"
#include "isc_dataproc_resultdata_ring_buffer.h"
#include "isc_framedecoder_interface.h"
#include "isc_blockmatching_interface.h"
#include "isc_data_processing_control.h"
#include "isc_main_control_impl.h"
#include "isc_main_control.h"

#include "isc_main_control.h"

#pragma comment (lib, "IscDplMainControl")

namespace ns_isc_dpl_c {

extern "C" {


//
// Functionality in this class will be provided in the future.
//



} /* extern "C" { */

} /* namespace ns_isc_dpl_c { */