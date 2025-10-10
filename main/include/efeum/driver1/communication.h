#pragma once

#include <windows.h>
#include <winioctl.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include "efeum/driver1/config.h"

HANDLE OpenDriverDevice();
bool RegisterWithDriver(HANDLE hDevice);
bool GetPendingTask(HANDLE hDevice, SCAN_TASK_DTO* pTask);
bool SendVerdict(HANDLE hDevice, SCAN_TASK_DTO* pTask, SCAN_VERDICT_DTO* pVerdict);
