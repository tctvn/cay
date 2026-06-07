#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct ConfigData {
    // 0 = None, VK codes for modifiers and key
    bool ctrl = true;
    bool shift = true;
    bool alt = false;
    bool win = false;
    unsigned int vkCode = 0; // 0 means no specific key, just modifiers like Ctrl+Shift

    // Macro dictionary (trigger -> replacement)
    std::unordered_map<std::wstring, std::wstring> macros;
    
    // App exception state
    bool autoRememberApp = true;
    std::vector<std::wstring> disabledApps;
    
    // Engine options
    bool autoRestore = true;
};

class ConfigManager {
public:
    static ConfigData LoadConfig();
    static void SaveConfig(const ConfigData& data);
    
private:
    static std::wstring GetConfigPath();
};
