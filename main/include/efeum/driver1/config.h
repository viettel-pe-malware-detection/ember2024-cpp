// ATTENTION: Must be in sync with driver/driver1/config.h

#pragma once

#ifdef DRIVER1_KERNEL_MODE
// In kernel mode, don't include headers here!
// Just forward declare what you need or use basic types

#ifndef _NTDDK_
    // Only include if not already included
#include <ntddk.h>
#include <minwindef.h>
#endif

#define FILE_INVALID_FILE_ID ((LONGLONG)-1LL) 

typedef struct _FILE_ID_128 {
    BYTE Identifier[16];
} FILE_ID_128, * PFILE_ID_128;

#else
// User mode headers
#include <windows.h>
#include <winioctl.h>
// Don't include ntdef.h in user mode - it conflicts with windows.h

//#define FILE_INVALID_FILE_ID ((LONGLONG)-1LL)
//
//typedef struct _FILE_ID_128 {
//    BYTE Identifier[16];
//} FILE_ID_128, * PFILE_ID_128;

#endif // DRIVER1_KERNEL_MODE


extern GUID const DRIVER1_DEVICE_GUID;

///////// IOCTLS //////////

#define IOCTL_REGISTER_SCANNER          CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GET_PENDING_EXECUTABLE    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_POST_SCANNING_RESULT      CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GET_NEXT_DECISION         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS)

#pragma pack(push, 1)

typedef struct _SCAN_TASK_DTO {
    UINT32 Version;
    UINT32 Reserved1;

    HANDLE Pid;
    BYTE Nonce[32];
    LARGE_INTEGER Timestamp;

    FILE_ID_128 FileId;
    ULONG VolumeSerialNumber;

    UINT32 Reserved2;
} SCAN_TASK_DTO, * PSCAN_TASK_DTO;

typedef struct _SCAN_VERDICT_DTO {
    UINT32 Version;
    BOOLEAN AllowExecution;
    BOOLEAN Reserved1;
    UINT16 Reserved2;
    
    // Unfortunately we can't use DOUBLE in kernel drivers
#define SIZEOF_DOUBLE 8
    BYTE PredScore[SIZEOF_DOUBLE];

    HANDLE Pid;

    BYTE Attestation[256];
} SCAN_VERDICT_DTO, *PSCAN_VERDICT_DTO;

typedef struct _DECISION_DTO {
    UINT32 Version;

    BOOLEAN IsValid;
    BOOLEAN AllowExecution;
    UINT16 Reserved2;
    ULONG VolumeSerialNumber;
    FILE_ID_128 FileId;

    BYTE PredScore[SIZEOF_DOUBLE];

    LARGE_INTEGER Timestamp;
} DECISION_DTO, *PDECISION_DTO;

#pragma pack(pop)
