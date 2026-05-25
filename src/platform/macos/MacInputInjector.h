#pragma once
#include "CayTypes.h"

namespace CayIME {
class MacInputInjector {
public:
    static void ReplaceText(int backspaceCount, const wchar_t* newText, int newTextLen);
};
} // namespace CayIME
