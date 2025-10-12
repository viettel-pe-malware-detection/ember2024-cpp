#include <ntstatus.h>
#include "efeum/driver1/communication.h"
#include <winternl.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdlib>

#include <setupapi.h>
#include <cfgmgr32.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

HRESULT ComputeAttestation(
    SCAN_TASK_DTO* pScanTask,
    SCAN_VERDICT_DTO* pVerdict
);



HANDLE OpenDriverDevice()
{
    HDEVINFO deviceInfoSet;
    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
    DWORD requiredSize = 0;
    HANDLE hDevice = INVALID_HANDLE_VALUE;

    // Get device interface set
    deviceInfoSet = SetupDiGetClassDevs(
        &DRIVER1_DEVICE_GUID,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        printf("SetupDiGetClassDevs failed: %d\n", GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    // Get the first device interface
    deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(
        deviceInfoSet,
        NULL,
        &DRIVER1_DEVICE_GUID,
        0,
        &deviceInterfaceData
    )) {
        printf("SetupDiEnumDeviceInterfaces failed: %d\n", GetLastError());
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    // Get required size
    SetupDiGetDeviceInterfaceDetail(
        deviceInfoSet,
        &deviceInterfaceData,
        NULL,
        0,
        &requiredSize,
        NULL
    );

    // Allocate buffer
    deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
    if (!deviceInterfaceDetailData) {
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    // Get device path
    if (!SetupDiGetDeviceInterfaceDetail(
        deviceInfoSet,
        &deviceInterfaceData,
        deviceInterfaceDetailData,
        requiredSize,
        NULL,
        NULL
    )) {
        printf("SetupDiGetDeviceInterfaceDetail failed: %d\n", GetLastError());
        free(deviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return INVALID_HANDLE_VALUE;
    }

    printf("Device path: %s\n", deviceInterfaceDetailData->DevicePath);

    // Open the device
    hDevice = CreateFile(
        deviceInterfaceDetailData->DevicePath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed: %d\n", GetLastError());
    }

    free(deviceInterfaceDetailData);
    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    return hDevice;
}

bool RegisterWithDriver(HANDLE hDevice)
{
    DWORD bytesReturned;
    SCAN_TASK_DTO handshakeTask = { 0 };

    printf("=== Starting Driver Registration ===\n");

    // Step 1: Send registration request
    printf("Step 1: Sending IOCTL_REGISTER_SCANNER...\n");
    if (!DeviceIoControl(
        hDevice,
        IOCTL_REGISTER_SCANNER,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL
    )) {
        printf("ERROR: IOCTL_REGISTER_SCANNER failed: %d\n", GetLastError());
        return false;
    }
    printf("  ✓ Registration IOCTL sent successfully\n");

    // Step 2: Get handshake task
    printf("Step 2: Getting handshake task...\n");
    if (!DeviceIoControl(
        hDevice,
        IOCTL_GET_PENDING_EXECUTABLE,
        NULL,
        0,
        &handshakeTask,
        sizeof(handshakeTask),
        &bytesReturned,
        NULL
    )) {
        printf("ERROR: IOCTL_GET_PENDING_EXECUTABLE failed: %d\n", GetLastError());
        return false;
    }

    if (bytesReturned < sizeof(SCAN_TASK_DTO)) {
        printf("ERROR: Invalid response size: %d bytes (expected %zu)\n",
            bytesReturned, sizeof(SCAN_TASK_DTO));
        return false;
    }

    // Verify it's a handshake (PID == 0)
    if (handshakeTask.Pid != 0) {
        printf("ERROR: Expected handshake task (PID=0), got PID=%p\n", handshakeTask.Pid);
        return false;
    }

    printf("  ✓ Received handshake task\n");
    printf("    Nonce: ");
    for (int i = 0; i < 32; i++) {
        printf("%02X", handshakeTask.Nonce[i]);
    }
    printf("\n");

    // Step 3: Compute attestation
    printf("Step 3: Computing attestation...\n");
    SCAN_VERDICT_DTO verdict = { 0 };
    verdict.Pid = 0;  // Handshake
    verdict.AllowExecution = TRUE;

    HRESULT hr = ComputeAttestation(&handshakeTask, &verdict);
    if (FAILED(hr)) {
        printf("ERROR: ComputeAttestation failed: 0x%08X\n", hr);
        return false;
    }

    printf("  ✓ Attestation computed\n");
    printf("    Signature: ");
    for (int i = 0; i < 32; i++) {
        printf("%02X", verdict.Attestation[i]);
    }
    printf("\n");

    // Step 4: Send attestation back
    printf("Step 4: Sending attestation to driver...\n");
    if (!DeviceIoControl(
        hDevice,
        IOCTL_POST_SCANNING_RESULT,
        &verdict,
        sizeof(verdict),
        NULL,
        0,
        &bytesReturned,
        NULL
    )) {
        printf("ERROR: IOCTL_POST_SCANNING_RESULT failed: %d\n", GetLastError());
        printf("  *** ATTESTATION VERIFICATION FAILED ***\n");
        printf("  Driver may terminate this process!\n");
        return false;
    }

    printf("  ✓ Attestation verified by driver!\n");
    printf("=== Registration Complete ===\n\n");
    return true;
}

bool GetPendingTask(HANDLE hDevice, SCAN_TASK_DTO* pTask)
{
    if (!pTask) return false;

    memset(pTask, 0, sizeof(SCAN_TASK_DTO));
    DWORD bytesReturned;

    if (!DeviceIoControl(
        hDevice,
        IOCTL_GET_PENDING_EXECUTABLE,
        NULL,
        0,
        pTask,
        sizeof(SCAN_TASK_DTO),
        &bytesReturned,
        NULL
    )) {
        printf("ERROR: IOCTL_GET_PENDING_EXECUTABLE failed: %d\n", GetLastError());
        return false;
    }

    if (bytesReturned < sizeof(SCAN_TASK_DTO)) {
        printf("No request so far.\n");
        return false;
    }

    return true;
}

bool SendVerdict(HANDLE hDevice, SCAN_TASK_DTO* pTask, SCAN_VERDICT_DTO* pVerdict)
{
    if (!pTask || !pVerdict) return false;

    // Compute attestation
    HRESULT hr = ComputeAttestation(pTask, pVerdict);
    if (FAILED(hr)) {
        printf("ERROR: ComputeAttestation failed: 0x%08X\n", hr);
        return false;
    }

    DWORD bytesReturned;
    if (!DeviceIoControl(
        hDevice,
        IOCTL_POST_SCANNING_RESULT,
        pVerdict,
        sizeof(SCAN_VERDICT_DTO),
        NULL,
        0,
        &bytesReturned,
        NULL
    )) {
        printf("ERROR: IOCTL_POST_SCANNING_RESULT failed: %d\n", GetLastError());
        return false;
    }

    return true;
}






















#pragma comment(lib, "bcrypt.lib")

// CRITICAL: This secret MUST match g_AttestationSecret in the driver
// Generated using driver1/generate_secret.py
static UCHAR g_AttestationSecret[128] = {
    0x2F, 0xDF, 0xED, 0x27, 0x5B, 0x58, 0x4E, 0xDF,
    0x0B, 0xFE, 0x83, 0x9B, 0xEF, 0x11, 0x4C, 0xD2,
    0x78, 0xCA, 0x36, 0xC0, 0x5E, 0x7C, 0xCE, 0x2A,
    0x87, 0x99, 0x1E, 0xAE, 0x66, 0x78, 0xE8, 0x39,
    0x73, 0xE3, 0xBD, 0xE5, 0x8C, 0x6F, 0xDA, 0x19,
    0xBA, 0xC1, 0x8B, 0x00, 0xB9, 0x60, 0x2A, 0xBC,
    0x46, 0xD3, 0x20, 0x28, 0xE2, 0x40, 0xFE, 0x7A,
    0x07, 0xA8, 0xE8, 0xD8, 0xD6, 0xDA, 0x92, 0x88,
    0x5D, 0x36, 0x5E, 0x0B, 0x17, 0x0D, 0x65, 0x9E,
    0x6D, 0x2E, 0x87, 0x06, 0xA8, 0x86, 0xC0, 0xA8,
    0xB7, 0xC6, 0x39, 0xFC, 0x6B, 0xD2, 0xCE, 0x74,
    0xF2, 0x6A, 0x3E, 0x7E, 0x39, 0xBE, 0xCD, 0xA1,
    0x2F, 0x8A, 0x21, 0x3C, 0x8A, 0x8A, 0xEF, 0xC8,
    0x88, 0xCE, 0x64, 0x14, 0x64, 0x4F, 0xBB, 0xE2,
    0xD3, 0x17, 0x95, 0x0E, 0x54, 0x1B, 0xE8, 0x68,
    0x43, 0xCD, 0x55, 0x2A, 0xC1, 0xA2, 0xE4, 0xB2
};

HRESULT ComputeAttestation(
    SCAN_TASK_DTO* pScanTask,
    SCAN_VERDICT_DTO* pVerdict
)
{
    if (!pScanTask || !pVerdict) {
        return E_INVALIDARG;
    }

    NTSTATUS status = STATUS_SUCCESS;
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    UCHAR hashObject[1024];     // Internal buffer for hash state
    UCHAR computedHash[32];   // SHA256 digest (32 bytes)
    HRESULT hr = S_OK;

    // IMPORTANT: Zero out the attestation field before hashing
    // (driver does the same during verification)
    memset(pVerdict->Attestation, 0, sizeof(pVerdict->Attestation));

    // Open SHA256 algorithm provider
    status = BCryptOpenAlgorithmProvider(
        &hAlg,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        printf("BCryptOpenAlgorithmProvider failed: 0x%08X\n", status);
        hr = HRESULT_FROM_NT(status);
        goto Cleanup;
    }

    // Create hash object
    status = BCryptCreateHash(
        hAlg,
        &hHash,
        hashObject,
        sizeof(hashObject),
        NULL,
        0,
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        printf("BCryptCreateHash failed: 0x%08X\n", status);
        hr = HRESULT_FROM_NT(status);
        goto Cleanup;
    }

    // Hash the SCAN_TASK_DTO
    status = BCryptHashData(
        hHash,
        (PUCHAR)pScanTask,
        sizeof(*pScanTask),
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        printf("BCryptHashData (ScanTask) failed: 0x%08X\n", status);
        hr = HRESULT_FROM_NT(status);
        goto Cleanup;
    }

    // Hash the SCAN_VERDICT_DTO (with Attestation field zeroed)
    status = BCryptHashData(
        hHash,
        (PUCHAR)pVerdict,
        sizeof(*pVerdict),
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        printf("BCryptHashData (Verdict) failed: 0x%08X\n", status);
        hr = HRESULT_FROM_NT(status);
        goto Cleanup;
    }

    // Hash the secret
    status = BCryptHashData(
        hHash,
        g_AttestationSecret,
        sizeof(g_AttestationSecret),
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        printf("BCryptHashData (Secret) failed: 0x%08X\n", status);
        hr = HRESULT_FROM_NT(status);
        goto Cleanup;
    }

    // Finalize the hash
    status = BCryptFinishHash(
        hHash,
        computedHash,
        sizeof(computedHash),
        0
    );
    if (!BCRYPT_SUCCESS(status)) {
        printf("BCryptFinishHash failed: 0x%08X\n", status);
        hr = HRESULT_FROM_NT(status);
        goto Cleanup;
    }

    // Copy the computed hash into the attestation field
    // The attestation field is 64 bytes, but SHA256 only produces 32 bytes
    // Copy the 32-byte hash to the first 32 bytes, rest remains zero
    memcpy(pVerdict->Attestation, computedHash, sizeof(computedHash));

Cleanup:
    if (hHash) {
        BCryptDestroyHash(hHash);
    }
    if (hAlg) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
    }

    return hr;
}