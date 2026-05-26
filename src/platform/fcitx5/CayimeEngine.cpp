#include "CayimeEngine.h"
#include <fcitx-utils/keysymgen.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/inputcontext.h>

#include <fcitx/inputpanel.h>
#include <fcitx/text.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterfacemanager.h>
static CayimeEngine* g_current_engine = nullptr;

// Helper to convert wstring to utf8 string without deprecated wstring_convert
static std::string utf8_from_wstring(const std::wstring& wstr) {
    std::string result;
    for (wchar_t wc : wstr) {
        if (wc <= 0x7F) {
            result.push_back(static_cast<char>(wc));
        } else if (wc <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | ((wc >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        } else if (wc <= 0xFFFF) {
            result.push_back(static_cast<char>(0xE0 | ((wc >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | ((wc >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((wc >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (wc & 0x3F)));
        }
    }
    return result;
}

static void GlobalInjectText(int backspaceCount, const wchar_t* newText, int newTextLen) {
    if (g_current_engine) {
        g_current_engine->injectText(backspaceCount, newText, newTextLen);
    }
}

void CayimeEngine::injectText(int backspaceCount, const wchar_t* newText, int newTextLen) {
    if (!current_ic_) return;
    
    bool usePreedit = forcePreedit_ || !current_ic_->capabilityFlags().test(fcitx::CapabilityFlag::SurroundingText);
    
    if (!usePreedit) {
        if (backspaceCount > 0) {
            current_ic_->deleteSurroundingText(-backspaceCount, backspaceCount);
        }
        
        if (newTextLen > 0) {
            std::wstring wstr(newText, newTextLen);
            current_ic_->commitString(utf8_from_wstring(wstr));
        }
    } else {
        if (backspaceCount > 0 && current_preedit_.length() >= (size_t)backspaceCount) {
            current_preedit_.erase(current_preedit_.length() - backspaceCount);
        }
        
        if (newTextLen > 0) {
            current_preedit_.append(newText, newTextLen);
        }
        
        fcitx::Text preedit;
        preedit.append(utf8_from_wstring(current_preedit_), fcitx::TextFormatFlag::Underline);
        preedit.setCursor(preedit.textLength());
        
        if (current_ic_->capabilityFlags().test(fcitx::CapabilityFlag::Preedit)) {
            current_ic_->inputPanel().setClientPreedit(preedit);
            current_ic_->inputPanel().setPreedit(fcitx::Text());
        } else {
            current_ic_->inputPanel().setPreedit(preedit);
            current_ic_->inputPanel().setClientPreedit(fcitx::Text());
        }
        
        current_ic_->updatePreedit();
        current_ic_->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    }
}

CayimeEngine::CayimeEngine(fcitx::Instance* instance)
    : instance_(instance) {
    engine_.OnInjectText = GlobalInjectText;

    forcePreeditAction_.setShortText("Gạch chân: Tự động");
    forcePreeditAction_.setCheckable(true);
    forcePreeditAction_.setChecked(false);

    instance_->userInterfaceManager().registerAction("cayime-force-preedit", &forcePreeditAction_);

    forcePreeditAction_.connect<fcitx::SimpleAction::Activated>([this](fcitx::InputContext*) {
        forcePreedit_ = !forcePreedit_;
        forcePreeditAction_.setChecked(forcePreedit_);
        if (forcePreedit_) {
            forcePreeditAction_.setShortText("Gạch chân: Luôn bật");
        } else {
            forcePreeditAction_.setShortText("Gạch chân: Tự động");
        }
    });
}

CayimeEngine::~CayimeEngine() {
}

void CayimeEngine::activate(const fcitx::InputMethodEntry& /*entry*/, fcitx::InputContextEvent& event) {
    event.inputContext()->statusArea().addAction(fcitx::StatusGroup::InputMethod, &forcePreeditAction_);
}

void CayimeEngine::reset(const fcitx::InputMethodEntry& /*entry*/, fcitx::InputContextEvent& event) {
    if (!current_preedit_.empty()) {
        // Clear preedit without committing so it doesn't jump to the new cursor position
        current_preedit_.clear();
        
        // Explicitly tell Fcitx5 to clear the UI to remove the underline frame
        event.inputContext()->inputPanel().reset();
        event.inputContext()->updatePreedit();
        event.inputContext()->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
    }
    
    // Reset CayEngine
    engine_.ResetFull();
}

bool CayimeEngine::convertKeyEvent(fcitx::KeyEvent& fcitxEvent, Cay::KeyEvent& cayEvent) {
    fcitx::Key key = fcitxEvent.key();
    
    if (key.sym() == FcitxKey_space) {
        cayEvent.keyCode = Cay::KeyCode::Space;
        cayEvent.character = ' ';
        cayEvent.handled = false;
        return true;
    }
    
    if (key.sym() > FcitxKey_space && key.sym() <= FcitxKey_asciitilde) {
        cayEvent.keyCode = static_cast<Cay::KeyCode>(toupper(key.sym()));
        cayEvent.character = static_cast<wchar_t>(key.sym());
        cayEvent.handled = false;
        return true;
    }
    
    if (key.sym() == FcitxKey_BackSpace) {
        cayEvent.keyCode = Cay::KeyCode::Backspace;
        cayEvent.character = 0;
        cayEvent.handled = false;
        return true;
    }
    if (key.sym() == FcitxKey_Return || key.sym() == FcitxKey_KP_Enter) { 
        engine_.ResetFull();
        return false; 
    }
    if (key.sym() == FcitxKey_Escape) { 
        engine_.ResetFull();
        return false; 
    }
    if (key.sym() == FcitxKey_Tab) { 
        engine_.ResetFull();
        return false; 
    }
    if (key.sym() == FcitxKey_Left || key.sym() == FcitxKey_KP_Left) { cayEvent.keyCode = Cay::KeyCode::Left; return true; }
    if (key.sym() == FcitxKey_Right || key.sym() == FcitxKey_KP_Right) { cayEvent.keyCode = Cay::KeyCode::Right; return true; }
    if (key.sym() == FcitxKey_Up || key.sym() == FcitxKey_KP_Up) { cayEvent.keyCode = Cay::KeyCode::Up; return true; }
    if (key.sym() == FcitxKey_Down || key.sym() == FcitxKey_KP_Down) { cayEvent.keyCode = Cay::KeyCode::Down; return true; }
    if (key.sym() == FcitxKey_Home || key.sym() == FcitxKey_KP_Home) { cayEvent.keyCode = Cay::KeyCode::Home; return true; }
    if (key.sym() == FcitxKey_End || key.sym() == FcitxKey_KP_End) { cayEvent.keyCode = Cay::KeyCode::End; return true; }
    if (key.sym() == FcitxKey_Page_Up || key.sym() == FcitxKey_KP_Page_Up) { cayEvent.keyCode = Cay::KeyCode::PageUp; return true; }
    if (key.sym() == FcitxKey_Page_Down || key.sym() == FcitxKey_KP_Page_Down) { cayEvent.keyCode = Cay::KeyCode::PageDown; return true; }
    if (key.sym() == FcitxKey_Delete || key.sym() == FcitxKey_KP_Delete) { cayEvent.keyCode = Cay::KeyCode::Delete; return true; }
    
    return false;
}

void CayimeEngine::keyEvent(const fcitx::InputMethodEntry& /*entry*/, fcitx::KeyEvent& keyEvent) {
    if (keyEvent.isRelease()) {
        return;
    }
    
    fcitx::Key key = keyEvent.key();
    
    // Check modifiers using uint32_t cast to avoid enum class bitwise operator issues
    uint32_t states = static_cast<uint32_t>(key.states());
    uint32_t mask = static_cast<uint32_t>(fcitx::KeyState::Ctrl) | 
                    static_cast<uint32_t>(fcitx::KeyState::Alt) | 
                    static_cast<uint32_t>(fcitx::KeyState::Super);
                    
    if (states & mask) {
        // Toggle Force Preedit on Ctrl+Alt+U
        if ((states & mask) == mask && key.sym() == FcitxKey_U) {
            forcePreedit_ = !forcePreedit_;
            forcePreeditAction_.setChecked(forcePreedit_);
            if (forcePreedit_) {
                forcePreeditAction_.setShortText("Gạch chân: Luôn bật");
            } else {
                forcePreeditAction_.setShortText("Gạch chân: Tự động");
            }
            keyEvent.filterAndAccept();
            return;
        }

        engine_.ResetFull(); // Reset engine state on shortcuts (like Ctrl+A)
        current_preedit_.clear();
        return;
    }

    Cay::KeyEvent cayEvent;
    if (convertKeyEvent(keyEvent, cayEvent)) {
        g_current_engine = this;
        current_ic_ = keyEvent.inputContext();
        
        bool usePreedit = forcePreedit_ || !current_ic_->capabilityFlags().test(fcitx::CapabilityFlag::SurroundingText);
        
        engine_.OnKeyDown(cayEvent);
        
        if (cayEvent.handled) {
            keyEvent.filterAndAccept();
        } else {
            // Key not handled by CayEngine. If we have a preedit, commit it now before passing key.
            if (usePreedit && !current_preedit_.empty()) {
                current_ic_->commitString(utf8_from_wstring(current_preedit_));
                current_preedit_.clear();
                current_ic_->inputPanel().reset();
                current_ic_->updatePreedit();
                current_ic_->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
            }
        }
        
        current_ic_ = nullptr;
        g_current_engine = nullptr;
    } else {
        // Phím không được CayIME xử lý (ví dụ: phím mũi tên, Esc, F1-F12...).
        // Bắt buộc phải chốt (commit) và dọn sạch khung chữ đang gõ dở TRƯỚC KHI phím lọt xuống ứng dụng.
        // Nếu không, khung chữ sẽ khóa cứng phím mũi tên hoặc làm loạn con trỏ chuột.
        if (!current_preedit_.empty()) {
            bool usePreedit = forcePreedit_ || !keyEvent.inputContext()->capabilityFlags().test(fcitx::CapabilityFlag::SurroundingText);
            if (usePreedit) {
                keyEvent.inputContext()->commitString(utf8_from_wstring(current_preedit_));
                keyEvent.inputContext()->inputPanel().reset();
                keyEvent.inputContext()->updatePreedit();
                keyEvent.inputContext()->updateUserInterface(fcitx::UserInterfaceComponent::InputPanel);
            }
            current_preedit_.clear();
            engine_.ResetFull();
        }
    }
}
