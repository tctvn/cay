#pragma once

#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <fcitx/action.h>
#include "CayEngine.h"

class CayimeEngine : public fcitx::InputMethodEngineV2 {
public:
    CayimeEngine(fcitx::Instance* instance);
    ~CayimeEngine() override;

    void keyEvent(const fcitx::InputMethodEntry& entry, fcitx::KeyEvent& keyEvent) override;
    void activate(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) override;
    void reset(const fcitx::InputMethodEntry& entry, fcitx::InputContextEvent& event) override;

    // Member helper for injection
    void injectText(int backspaceCount, const wchar_t* newText, int newTextLen);

private:
    fcitx::Instance* instance_;
    Cay::TelexEngine engine_;
    fcitx::InputContext* current_ic_;
    std::wstring current_preedit_;
    
    bool forcePreedit_ = false;
    fcitx::SimpleAction forcePreeditAction_;

    // Helper to convert fcitx::Key to Cay::KeyEvent
    bool convertKeyEvent(fcitx::KeyEvent& fcitxEvent, Cay::KeyEvent& cayEvent);
};
