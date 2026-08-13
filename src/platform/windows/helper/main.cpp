#include <windows.h>
#include <setupapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <map>
#include "../shared/InputInjector.h"
#include "VkToHid.h"

#pragma comment(lib, "setupapi.lib")

const GUID GUID_DEVINTERFACE_HID = { 0x4D1E55B2, 0xF16F, 0x11CF, { 0x88, 0xCB, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };

HANDLE g_hPicoDevice = INVALID_HANDLE_VALUE;
HHOOK g_hHook = NULL;

std::queue<std::vector<uint8_t>> g_writeQueue;
std::mutex g_writeMutex;
std::condition_variable g_writeCv;

void PicoWriterThread() {
    while (true) {
        std::vector<uint8_t> packet;
        {
            std::unique_lock<std::mutex> lock(g_writeMutex);
            g_writeCv.wait(lock, [] { return !g_writeQueue.empty(); });
            packet = g_writeQueue.front();
            g_writeQueue.pop();
        }
        if (g_hPicoDevice != INVALID_HANDLE_VALUE) {
            OVERLAPPED ovWrite = {0};
            ovWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (WriteFile(g_hPicoDevice, packet.data(), packet.size(), NULL, &ovWrite) || GetLastError() == ERROR_IO_PENDING) {
                DWORD bytesWritten;
                GetOverlappedResult(g_hPicoDevice, &ovWrite, &bytesWritten, TRUE);
            }
            CloseHandle(ovWrite.hEvent);
        }
    }
}

std::string FindPicoDevicePath() {
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_HID, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hDevInfo == INVALID_HANDLE_VALUE) return "";

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
            for(auto& c : path) c = tolower(c);
            
            if (path.find("vid_cafe") != std::string::npos && 
                path.find("pid_4003") != std::string::npos &&
                path.find("mi_01") != std::string::npos) {
                
                SetupDiDestroyDeviceInfoList(hDevInfo);
                return detailData->DevicePath;
            }
        }
    }
    
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return "";
}

// Luồng đọc dữ liệu từ Pico
void PicoReaderThread() {
    OVERLAPPED ovRead = {0};
    ovRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    while (true) {
        if (g_hPicoDevice == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        uint8_t inReport[65] = {0};
        DWORD bytesRead;
        ResetEvent(ovRead.hEvent);
        
        bool readOk = false;
        if (ReadFile(g_hPicoDevice, inReport, 65, NULL, &ovRead) || GetLastError() == ERROR_IO_PENDING) {
            if (GetOverlappedResult(g_hPicoDevice, &ovRead, &bytesRead, TRUE)) {
                readOk = true;
            }
        }
        
        if (readOk && bytesRead >= 65) {
            if (inReport[1] == 0xAA) {
                int backspaceCount = inReport[2];
                int newTextLen = inReport[3];
                
                wchar_t newText[32] = {0};
                memcpy(newText, &inReport[4], newTextLen * sizeof(wchar_t));
                
                std::wcout << L"Injecting: " << newText << L" (BS: " << backspaceCount << L")" << std::endl;
                
                CayIME::InputInjector::ReplaceText(backspaceCount, newText, newTextLen);
            } else if (inReport[1] == 0xAC) {
                // Raw Key Injection
                uint8_t hidCode = inReport[2];
                uint8_t isDown = inReport[3];
                uint8_t modifiers = inReport[4]; // Matches raw[3] from Pico
                
                // Map HID back to VK
                DWORD vkCode = HidToVk(hidCode);
                if (vkCode != 0) {
                    INPUT input = {0};
                    input.type = INPUT_KEYBOARD;
                    input.ki.wVk = vkCode;
                    
                    if (!isDown) {
                        input.ki.dwFlags = KEYEVENTF_KEYUP;
                    }
                    SendInput(1, &input, sizeof(INPUT));
                }
            }
        } else if (!readOk) {
            // Mất kết nối
            std::cout << "Connection lost. Waiting for device..." << std::endl;
            CloseHandle(g_hPicoDevice);
            g_hPicoDevice = INVALID_HANDLE_VALUE;
        }
    }
}

// Luồng kết nối lại
void PicoConnectorThread() {
    while (true) {
        if (g_hPicoDevice == INVALID_HANDLE_VALUE) {
            std::string path = FindPicoDevicePath();
            if (!path.empty()) {
                HANDLE hDevice = CreateFile(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
                
                if (hDevice != INVALID_HANDLE_VALUE) {
                    std::cout << "Pico Keyboard found! Connected to Raw HID interface." << std::endl;
                
                // Gửi handshake Coprocessor mode (Magic byte 0xAA)
                uint8_t outReport[65] = {0};
                outReport[0] = 0;
                outReport[1] = 0xAA;
                
                OVERLAPPED ovWrite = {0};
                ovWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                
                bool writeOk = false;
                if (WriteFile(hDevice, outReport, 65, NULL, &ovWrite) || GetLastError() == ERROR_IO_PENDING) {
                    DWORD bytesWritten;
                    if (GetOverlappedResult(hDevice, &ovWrite, &bytesWritten, TRUE)) {
                        writeOk = true;
                    }
                }
                CloseHandle(ovWrite.hEvent);
                
                if (!writeOk) {
                    std::cerr << "Failed to send handshake." << std::endl;
                    CloseHandle(hDevice);
                } else {
                    std::cout << "Handshake sent. Pico is now using App Helper mode." << std::endl;
                    g_hPicoDevice = hDevice;
                }
            }
        }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
std::map<DWORD, bool> g_keyState;
HHOOK g_hMouseHook = NULL;

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN) {
            // Gửi tín hiệu Reset xuống Pico (0xAD)
            if (g_hPicoDevice != INVALID_HANDLE_VALUE) {
                uint8_t outReport[65] = {0};
                outReport[0] = 0;
                outReport[1] = 0xAD; // Magic byte cho Reset
                {
                    std::lock_guard<std::mutex> lock(g_writeMutex);
                    std::vector<uint8_t> pkt(outReport, outReport + 65);
                    g_writeQueue.push(pkt);
                }
                g_writeCv.notify_one();
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyBoard = (KBDLLHOOKSTRUCT*)lParam;
        
        // Bỏ qua các phím do chính Helper tạo ra
        if (pKeyBoard->flags & LLKHF_INJECTED) {
            return CallNextHookEx(g_hHook, nCode, wParam, lParam);
        }
        
        // Check modifiers
        bool hasModifier = (GetAsyncKeyState(VK_LCONTROL) & 0x8000) ||
                           (GetAsyncKeyState(VK_RCONTROL) & 0x8000) ||
                           (GetAsyncKeyState(VK_LMENU) & 0x8000) ||
                           (GetAsyncKeyState(VK_RMENU) & 0x8000) ||
                           (GetAsyncKeyState(VK_LWIN) & 0x8000) ||
                           (GetAsyncKeyState(VK_RWIN) & 0x8000);
        
        // Bỏ qua nếu là Auto-Repeat của OS để tránh loop và giữ tính năng auto-repeat
        bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        if (isDown) {
            if (g_keyState[pKeyBoard->vkCode]) {
                return CallNextHookEx(g_hHook, nCode, wParam, lParam);
            }
            g_keyState[pKeyBoard->vkCode] = true;
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            g_keyState[pKeyBoard->vkCode] = false;
        }
        
        // Nếu Pico đã kết nối, chuyển tiếp phím xuống Pico
        if (g_hPicoDevice != INVALID_HANDLE_VALUE) {
            uint8_t hidCode = VkToHid(pKeyBoard->vkCode);
            
            if (hidCode != 0) {
                // Đóng gói gửi xuống Pico
                uint8_t outReport[65] = {0};
                outReport[0] = 0;
                outReport[1] = 0xAB; 
                outReport[2] = hidCode;
                outReport[3] = isDown ? 1 : 0;
                
                // Modifiers
                uint8_t modifiers = 0;
                if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) modifiers |= 0x02;
                if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) modifiers |= 0x20;
                if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) modifiers |= 0x01;
                if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) modifiers |= 0x10;
                if (GetAsyncKeyState(VK_LMENU) & 0x8000) modifiers |= 0x04;
                if (GetAsyncKeyState(VK_RMENU) & 0x8000) modifiers |= 0x40;
                outReport[4] = modifiers;
                
                {
                    std::lock_guard<std::mutex> lock(g_writeMutex);
                    std::vector<uint8_t> pkt(outReport, outReport + 65);
                    g_writeQueue.push(pkt);
                }
                g_writeCv.notify_one();
                // CHẶN PHÍM VẬT LÝ NÀY LẠI (Nếu KHÔNG CÓ MODIFIER)
                // Nếu có modifier (Ctrl+A...), ta vẫn gửi xuống Pico (để Pico reset CayEngine),
                // nhưng KHÔNG chặn (không return 1), để OS vẫn nhận được phím tắt đó.
                if (!hasModifier) {
                    return 1; 
                }
            }
        }
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

int main() {
    HANDLE hMutex = CreateMutexA(NULL, TRUE, "CayHelperSingleInstanceMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cerr << "CayHelper is already running! Please close other instances." << std::endl;
        CloseHandle(hMutex);
        return 1;
    }
    
    std::cout << "Cay Helper (Coprocessor Mode) is running..." << std::endl;
    
    std::thread connector(PicoConnectorThread);
    connector.detach();
    
    std::thread reader(PicoReaderThread);
    reader.detach();
    
    std::thread writer(PicoWriterThread);
    writer.detach();
    
    // Cài đặt Keyboard Hook
    g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandle(NULL), 0);
    
    if (g_hHook == NULL || g_hMouseHook == NULL) {
        std::cerr << "Failed to install hooks!" << std::endl;
        return 1;
    }
    
    // Vòng lặp message của Windows
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    UnhookWindowsHookEx(g_hHook);
    return 0;
}
