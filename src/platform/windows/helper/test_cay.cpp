#include <iostream>
#include <string>
#include "../../../core/CayEngine.h"

// Dummy implementation for testing
void onInjectText(int backspaceCount, const wchar_t* newText, int newTextLen) {
    std::wcout << L"BS: " << backspaceCount << L", Len: " << newTextLen << L", Chars:";
    for (int i=0; i<newTextLen; i++) {
        std::wcout << L" " << std::hex << (unsigned int)newText[i];
    }
    std::wcout << std::endl;
}

int main() {
    Cay::TelexEngine engine;
    engine.OnInjectText = onInjectText;

    std::string keys = "traanf";
    std::wcout << L"Typing: traanf" << std::endl;
    for (char c : keys) {
        Cay::KeyEvent e;
        e.keyCode = (Cay::KeyCode)c;
        e.character = c;
        e.handled = false;
        engine.OnKeyDown(e);
    }
    return 0;
}
