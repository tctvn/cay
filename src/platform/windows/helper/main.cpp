#include <windows.h>
#include <setupapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "../shared/InputInjector.h"

#pragma comment(lib, "setupapi.lib")

// The HID class GUID
const GUID GUID_DEVINTERFACE_HID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

HANDLE FindPicoDevice() {
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE) return INVALID_HANDLE_VALUE;

    SP_DEVICE_INTERFACE_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &GUID_DEVINTERFACE_HID, i, &devInfoData); ++i) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInfoData, NULL, 0, &requiredSize, NULL);
        
        std::vector<BYTE> detailDataBuf(requiredSize);
        SP_DEVICE_INTERFACE_DETAIL_DATA* detailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)detailDataBuf.data();
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        
        if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInfoData, detailData, requiredSize, NULL, NULL)) {
            std::string path = detailData->DevicePath;
            // Chuyển sang chữ thường để dễ so sánh
            for(auto& c : path) c = tolower(c);
            
            // Tìm VID 0xCAFE, PID 0x4003, MI 01 (Interface 1 - Raw HID)
            if (path.find("vid_cafe") != std::string::npos && 
                path.find("pid_4003") != std::string::npos &&
                path.find("mi_01") != std::string::npos) {
                
                HANDLE hDevice = CreateFile(detailData->DevicePath, GENERIC_READ | GENERIC_WRITE, 
                                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                if (hDevice != INVALID_HANDLE_VALUE) {
                    SetupDiDestroyDeviceInfoList(hDevInfo);
                    return hDevice;
                }
            }
        }
    }
    
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return INVALID_HANDLE_VALUE;
}

int main() {
    std::cout << "Cay Helper is running. Waiting for Pico keyboard..." << std::endl;
    
    while (true) {
        HANDLE hDevice = FindPicoDevice();
        if (hDevice == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        std::cout << "Pico Keyboard found! Connected to Raw HID interface." << std::endl;
        
        // Gửi handshake (Magic byte 0xAA)
        uint8_t outReport[65] = {0};
        outReport[0] = 0; // Report ID 0
        outReport[1] = 0xAA;
        
        DWORD bytesWritten;
        if (!WriteFile(hDevice, outReport, 65, &bytesWritten, NULL)) {
            std::cerr << "Failed to send handshake." << std::endl;
        } else {
            std::cout << "Handshake sent. Pico is now using App Helper mode." << std::endl;
        }
        
        // Vòng lặp nhận dữ liệu
        uint8_t inReport[65] = {0};
        DWORD bytesRead;
        while (ReadFile(hDevice, inReport, 65, &bytesRead, NULL)) {
            // inReport[0] là Report ID. Payload bắt đầu từ inReport[1].
            // Phía Pico gửi raw_report[0] = 0xAA, [1] = backspaceCount, [2] = newTextLen
            if (bytesRead >= 65 && inReport[1] == 0xAA) {
                int backspaceCount = inReport[2];
                int newTextLen = inReport[3];
                
                // Trích xuất chuỗi wchar_t
                wchar_t newText[32] = {0};
                memcpy(newText, &inReport[4], newTextLen * sizeof(wchar_t));
                
                std::wcout << L"Injecting: " << newText << L" (BS: " << backspaceCount << L")" << std::endl;
                
                CayIME::InputInjector::ReplaceText(backspaceCount, newText, newTextLen);
            }
        }
        
        std::cout << "Connection lost. Waiting for device..." << std::endl;
        CloseHandle(hDevice);
    }
    
    return 0;
}
