#pragma once

namespace CayIME {
class MacHookManager {
public:
    static bool Initialize();
    static void Shutdown();
    static void ResetEngine();
};
} // namespace CayIME
