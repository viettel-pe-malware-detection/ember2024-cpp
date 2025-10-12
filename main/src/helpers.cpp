#include "efeum/helpers.h"
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <strsafe.h>

// #define VERBOSE

#ifdef VERBOSE
#include <iostream>
#endif

std::wstring FormatTimestamp(const LARGE_INTEGER& ts) {
    FILETIME ft;
    ft.dwLowDateTime = ts.LowPart;
    ft.dwHighDateTime = ts.HighPart;

    SYSTEMTIME stUTC, stLocal;
    FileTimeToSystemTime(&ft, &stUTC);
    SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, &stLocal);

    wchar_t buf[64];
    StringCchPrintfW(buf, 64, L"%02d/%02d/%04d %02d:%02d:%02d",
        stLocal.wDay, stLocal.wMonth, stLocal.wYear,
        stLocal.wHour, stLocal.wMinute, stLocal.wSecond);
    return buf;
}







std::wstring FileIdToPath(const FILE_ID_128& fileId, ULONG volumeSerialNumber)
{
    // Enumerate all volumes
    WCHAR volumeName[MAX_PATH] = {};
    HANDLE hFind = FindFirstVolumeW(volumeName, ARRAYSIZE(volumeName));
    if (hFind == INVALID_HANDLE_VALUE) {
        #ifdef VERBOSE
        std::cerr << "FindFirstVolumeW failed: " << GetLastError() << '\n';
        #endif
        return {};
    }

    std::wstring result;

    do {
        // No need to skip trailing backslash for GetVolumeInformationW
        // std::wstring volPath(volumeName);
        // if (volPath.back() == L'\\') volPath.pop_back();

        DWORD serial = 0;
        #ifdef VERBOSE
        std::cerr << "Calling GetVolumeInformationW\n";
        #endif
        if (GetVolumeInformationW(/*volPath.c_str()*/volumeName, nullptr, 0, &serial, nullptr, nullptr, nullptr, 0)) {
            #ifdef VERBOSE
            std::cerr << "Viewing volume with serial: " << std::hex << serial << std::dec << '\n';
            #endif
            if (serial == volumeSerialNumber) {
                // Found matching volume
                #ifdef VERBOSE
                std::cerr << "Found matching volume\n";
                #endif
                HANDLE hVol = CreateFileW(
                    volumeName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    nullptr,
                    OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS,
                    nullptr);

                if (hVol != INVALID_HANDLE_VALUE) {
                    FILE_ID_DESCRIPTOR fileIdDesc = { 0 };
                    fileIdDesc.dwSize = sizeof(FILE_ID_DESCRIPTOR);
                    fileIdDesc.Type = ExtendedFileIdType;
                    RtlCopyMemory(&fileIdDesc.ExtendedFileId, &fileId, sizeof(FILE_ID_128));
                    HANDLE hFile = OpenFileById(
                        hVol,
                        &fileIdDesc,
                        FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        nullptr,
                        0);

                    if (hFile != INVALID_HANDLE_VALUE) {
                        #ifdef VERBOSE
                        std::cerr << "Found matching file\n";
                        #endif
                        thread_local WCHAR pathBuf[MAX_PATH * 3] = {};
                        DWORD len = GetFinalPathNameByHandleW(hFile, pathBuf, ARRAYSIZE(pathBuf), FILE_NAME_NORMALIZED);
                        if (len > 0 && len < ARRAYSIZE(pathBuf)) {
                            result.assign(pathBuf, len);
                        }
                        #ifdef VERBOSE
                        std::cerr << "Matching file path assigned\n";
                        #endif
                        CloseHandle(hFile);
                    }
                    CloseHandle(hVol);
                }
                break;
            }
        }
    } while (FindNextVolumeW(hFind, volumeName, ARRAYSIZE(volumeName)));

    FindVolumeClose(hFind);

    return result;
}

std::wstring FormatFileId128Hex(const FILE_ID_128& id)
{
    // Build two uint64_t values from the 16 identifier bytes in big-endian order.
    uint64_t high = 0;
    uint64_t low  = 0;

    for (int i = 0; i < 8; ++i) {
        high = (high << 8) | static_cast<uint8_t>(id.Identifier[i]);
    }
    for (int i = 0; i < 8; ++i) {
        low = (low << 8) | static_cast<uint8_t>(id.Identifier[8 + i]);
    }

    std::wostringstream woss;
    woss << L"0x"
         << std::hex << std::setw(16) << std::setfill(L'0') << std::uppercase
         << static_cast<unsigned long long>(high)
         << L" | 0x"
         << std::hex << std::setw(16) << std::setfill(L'0')
         << static_cast<unsigned long long>(low);

    return woss.str();
}
