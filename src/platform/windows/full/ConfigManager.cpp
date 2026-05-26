#include "ConfigManager.h"
#include <windows.h>
#include <sstream>

std::wstring ConfigManager::GetConfigPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring p(path);
    size_t pos = p.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        return p.substr(0, pos) + L"\\cayy.dat";
    }
    return L"cayy.dat";
}

// Chuyển UTF-8 sang UTF-16
std::wstring Utf8ToUtf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.length(), nullptr, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.length(), &result[0], size);
    return result;
}

// Chuyển UTF-16 sang UTF-8
std::string Utf16ToUtf8(const std::wstring& utf16) {
    if (utf16.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.length(), nullptr, 0, nullptr, nullptr);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.length(), &result[0], size, nullptr, nullptr);
    return result;
}

ConfigData ConfigManager::LoadConfig() {
    ConfigData data;
    std::wstring path = GetConfigPath();
    
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return data; // Mặc định
    }
    
    DWORD fileSize = GetFileSize(hFile, nullptr);
    std::string content(fileSize, '\0');
    DWORD bytesRead;
    ReadFile(hFile, &content[0], fileSize, &bytesRead, nullptr);
    CloseHandle(hFile);
    
    std::wstring text = Utf8ToUtf16(content);
    
    // Parse đơn giản
    size_t pos = 0;
    int section = 0; // 1 = Hotkey, 2 = Macro
    while (pos < text.length()) {
        size_t end = text.find(L'\n', pos);
        if (end == std::wstring::npos) end = text.length();
        
        std::wstring line = text.substr(pos, end - pos);
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        
        if (line == L"[Hotkeys]") section = 1;
        else if (line == L"[Macros]") section = 2;
        else if (!line.empty()) {
            size_t eq = line.find(L'=');
            if (eq != std::wstring::npos) {
                std::wstring key = line.substr(0, eq);
                std::wstring val = line.substr(eq + 1);
                
                if (section == 1) {
                    if (key == L"Ctrl") data.ctrl = (val == L"1");
                    else if (key == L"Shift") data.shift = (val == L"1");
                    else if (key == L"Alt") data.alt = (val == L"1");
                    else if (key == L"Win") data.win = (val == L"1");
                    else if (key == L"VK") data.vkCode = std::wcstoul(val.c_str(), nullptr, 10);
                } else if (section == 2) {
                    data.macros[key] = val;
                }
            }
        }
        
        pos = end + 1;
    }
    
    return data;
}

void ConfigManager::SaveConfig(const ConfigData& data) {
    std::wstring path = GetConfigPath();
    
    std::wstring text = L"[Hotkeys]\r\n";
    text += L"Ctrl=" + std::wstring(data.ctrl ? L"1" : L"0") + L"\r\n";
    text += L"Shift=" + std::wstring(data.shift ? L"1" : L"0") + L"\r\n";
    text += L"Alt=" + std::wstring(data.alt ? L"1" : L"0") + L"\r\n";
    text += L"Win=" + std::wstring(data.win ? L"1" : L"0") + L"\r\n";
    text += L"VK=" + std::to_wstring(data.vkCode) + L"\r\n";
    
    text += L"\r\n[Macros]\r\n";
    for (const auto& pair : data.macros) {
        text += pair.first + L"=" + pair.second + L"\r\n";
    }
    
    std::string utf8 = Utf16ToUtf8(text);
    
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hFile, utf8.c_str(), (DWORD)utf8.length(), &bytesWritten, nullptr);
        CloseHandle(hFile);
    }
}
