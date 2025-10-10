#include <iostream>
#include <vector>

#include "efe/core.h"
#include "mio/mmap.hpp"

#include <windows.h>
#include <stdio.h>
#include <signal.h>
#include "efeum/driver1/communication.h"

#include "efeum/ml/lgbm.h"

template<typename T = feature_t>
constexpr int getLGBMInputDataType() {
    if constexpr (std::is_same_v<T, float>) {
        return C_API_DTYPE_FLOAT32;
    } else {
        static_assert( std::is_same_v<T, double> );
        return C_API_DTYPE_FLOAT64;
    }
}

char const* MODEL_FILE = "C:\\emb\\meta.dat";




HANDLE g_hDevice = INVALID_HANDLE_VALUE;
volatile bool g_running = true;
BoosterHandle g_booster = NULL;


void SignalHandler(int signal)
{
    printf("\n=== Shutdown Signal Received ===\n");
    g_running = false;
}

// TODO: Replace this with your actual AI model
bool RunAIModelScan(SCAN_TASK_DTO* pTask)
{
    // Placeholder: Allow all processes for now
    printf("  [AI] Running AI model on PID %p...\n", pTask->Pid);

    printf("  [AI] Analysis complete: BENIGN\n");
    return false;  // false = not malicious, allow execution
}

void ScanningLoop(HANDLE hDevice)
{
    printf("\n=== Entering Scanning Loop ===\n");

    while (g_running) {
        SCAN_TASK_DTO scanTask;

        // Get next task from driver
        if (!GetPendingTask(hDevice, &scanTask)) {
            Sleep(1000);
            continue;
        }

        // Check if task is empty (no pending tasks)
        if (scanTask.Pid == 0) {
            Sleep(100);  // No tasks, wait a bit
            continue;
        }

        printf("\n--- New Scan Task ---\n");
        printf("  PID: %p\n", scanTask.Pid);
        printf("  File ID: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X", scanTask.FileId.Identifier[i]);
        }
        printf("\n");
        printf("  Volume Serial: 0x%08X\n", scanTask.VolumeSerialNumber);

        // Run AI model
        bool isMalicious = RunAIModelScan(&scanTask);

        // Prepare verdict
        SCAN_VERDICT_DTO verdict = { 0 };
        verdict.Pid = scanTask.Pid;
        verdict.AllowExecution = !isMalicious;

        // Send verdict
        if (!SendVerdict(hDevice, &scanTask, &verdict)) {
            printf("  ERROR: Failed to send verdict\n");
            continue;
        }

        printf("  ✓ Verdict sent: %s\n", verdict.AllowExecution ? "ALLOW" : "DENY");
    }
}

int main()
{
    printf("╔══════════════════════════════════════╗\n");
    printf("║   AI Model Scanner for Driver1       ║\n");
    printf("╚══════════════════════════════════════╝\n\n");

    int num_features;

    // Load the pretrained model
    std::cout << "Loading model from " << MODEL_FILE << "..." << std::endl;
    int ret = LoadLGBMModel(MODEL_FILE, &g_booster, &num_features);
    if (ret != 0) {
        return 1;
    }
    std::cout << "Model expects " << num_features << " features" << std::endl;












    // Set up signal handlers
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    // Open driver device
    printf("Opening driver device...\n");
    g_hDevice = OpenDriverDevice();
    if (g_hDevice == INVALID_HANDLE_VALUE) {
        printf("ERROR: Failed to open driver device\n");
        printf("Make sure:\n");
        printf("  1. Driver is loaded\n");
        printf("  2. Running as Administrator\n");
        printf("  3. GUID matches driver\n");
        return 1;
    }
    printf("✓ Driver device opened successfully\n\n");

    // Register with driver
    if (!RegisterWithDriver(g_hDevice)) {
        printf("ERROR: Failed to register with driver\n");
        CloseHandle(g_hDevice);
        return 1;
    }

    // Main scanning loop
    ScanningLoop(g_hDevice);

    // Cleanup
    printf("\n=== Cleaning up ===\n");
    CloseHandle(g_hDevice);
    FreeLGBMModel(g_booster);
    printf("✓ Shutdown complete\n");

    return 0;
}
