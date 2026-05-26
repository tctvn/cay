#include <windows.h>
#include <vcclr.h>
#include "CayData.h"
#include "CayEngine.h"
#include "KeyboardHookManager.h"
#include "InputInjector.h"

// Managed dependencies
#using <System.dll>
#using <System.Windows.Forms.dll>
#using <System.Drawing.dll>

using namespace System;
using namespace System::Windows::Forms;
using namespace System::Drawing;

// Global native pointers
Cay::TelexEngine* g_engine = nullptr;
CayIME::InputHookManager* g_hookManager = nullptr;
bool g_enabled = true;
bool g_pendingToggle = false;

// ---------------------------------------------------------------------------
// Native Callbacks
// ---------------------------------------------------------------------------
Cay::KeyCode MapVKToKeyCode(DWORD vk) {
    if (vk >= 'A' && vk <= 'Z') return (Cay::KeyCode)vk;
    switch (vk) {
        case VK_BACK: return Cay::KeyCode::Backspace;
        case VK_ESCAPE: return Cay::KeyCode::Escape;
        case VK_RETURN: return Cay::KeyCode::Enter;
        case VK_TAB: return Cay::KeyCode::Tab;
        case VK_SPACE: return Cay::KeyCode::Space;
        case VK_LEFT: return Cay::KeyCode::Left;
        case VK_RIGHT: return Cay::KeyCode::Right;
        case VK_UP: return Cay::KeyCode::Up;
        case VK_DOWN: return Cay::KeyCode::Down;
        case VK_HOME: return Cay::KeyCode::Home;
        case VK_END: return Cay::KeyCode::End;
        case VK_PRIOR: return Cay::KeyCode::PageUp;
        case VK_NEXT: return Cay::KeyCode::PageDown;
        case VK_DELETE: return Cay::KeyCode::Delete;
        default: return Cay::KeyCode::Unknown;
    }
}

void OnKeyDownHook(CayIME::InputHookManager* sender, CayIME::HookKeyEventArgs& e) {
    if (e.extraInfo == CayIME::InputInjector::MAGIC_EXTRA_INFO) return;

    bool isCtrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool isAltPressed  = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    bool isWinPressed  = (GetAsyncKeyState(VK_LWIN) & 0x8000) != 0 || (GetAsyncKeyState(VK_RWIN) & 0x8000) != 0;

    if (isCtrlPressed && (e.keyCode == VK_LSHIFT || e.keyCode == VK_RSHIFT)) {
        g_pendingToggle = true; 
        g_engine->ResetFull();
        return;
    }

    if (isCtrlPressed || isAltPressed || isWinPressed) {
        g_engine->ResetFull();
        return;
    }

    if (!g_enabled) return;

    Cay::KeyEvent ce;
    ce.keyCode = MapVKToKeyCode(e.keyCode);
    ce.character = e.character;
    ce.handled = false;
    g_engine->OnKeyDown(ce);
    if (ce.handled) e.handled = true;
}

void OnKeyUpHook(CayIME::InputHookManager* sender, CayIME::HookKeyEventArgs& e) {
    if (e.extraInfo == CayIME::InputInjector::MAGIC_EXTRA_INFO) return;

    if (e.keyCode == VK_LCONTROL || e.keyCode == VK_RCONTROL) {
        if (g_pendingToggle) { 
            g_pendingToggle = false; 
            g_enabled = !g_enabled; 
            MessageBeep(MB_OK); 
            // In a real app, we'd trigger an event to update the tray icon here
        } 
        return;
    }
    
    if (g_pendingToggle && (e.keyCode == VK_LSHIFT || e.keyCode == VK_RSHIFT)) {
        g_pendingToggle = false; 
        g_enabled = !g_enabled; 
        MessageBeep(MB_OK); 
        return;
    }

    if (!g_enabled) return;
    Cay::KeyEvent ce;
    ce.keyCode = MapVKToKeyCode(e.keyCode);
    ce.character = e.character;
    ce.handled = false;
    g_engine->OnKeyUp(ce);
    if (ce.handled) e.handled = true;
}

void OnMouseClickHook(CayIME::InputHookManager* sender) {
    g_engine->ResetFull();
}

// ---------------------------------------------------------------------------
// Managed UI
// ---------------------------------------------------------------------------
public ref class MainForm : public Form {
private:
    NotifyIcon^ trayIcon;
    System::Windows::Forms::ContextMenuStrip^ trayMenu;

public:
    MainForm() {
        this->Text = "Cay Full Configuration";
        this->ShowInTaskbar = false;
        this->WindowState = FormWindowState::Minimized;
        this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedDialog;
        this->MaximizeBox = false;
        this->ClientSize = System::Drawing::Size(400, 300);

        Label^ lblIntro = gcnew Label();
        lblIntro->Text = "Cấu hình mở rộng cho CayIME (Bản Full)";
        lblIntro->AutoSize = true;
        lblIntro->Location = System::Drawing::Point(20, 20);
        this->Controls->Add(lblIntro);

        trayMenu = gcnew System::Windows::Forms::ContextMenuStrip();
        trayMenu->Items->Add("Cấu hình", nullptr, gcnew EventHandler(this, &MainForm::OnSettingsClicked));
        trayMenu->Items->Add("-");
        trayMenu->Items->Add("Thoát", nullptr, gcnew EventHandler(this, &MainForm::OnExitClicked));

        trayIcon = gcnew NotifyIcon();
        trayIcon->Text = "Cay (Full)";
        trayIcon->Icon = SystemIcons::Application; // Placeholder icon
        trayIcon->ContextMenuStrip = trayMenu;
        trayIcon->Visible = true;
        trayIcon->DoubleClick += gcnew EventHandler(this, &MainForm::OnSettingsClicked);
    }

protected:
    virtual void OnLoad(EventArgs^ e) override {
        Form::OnLoad(e);
        this->Hide(); // Hide main form on startup
    }

    virtual void OnFormClosing(FormClosingEventArgs^ e) override {
        if (e->CloseReason == CloseReason::UserClosing) {
            e->Cancel = true;
            this->Hide();
        } else {
            Form::OnFormClosing(e);
        }
    }

private:
    void OnSettingsClicked(Object^ sender, EventArgs^ e) {
        this->Show();
        this->WindowState = FormWindowState::Normal;
        this->Activate();
    }

    void OnExitClicked(Object^ sender, EventArgs^ e) {
        trayIcon->Visible = false;
        Application::Exit();
    }
};

// ---------------------------------------------------------------------------
// Entry Point
// ---------------------------------------------------------------------------
[STAThread]
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {
    // Elevate priority
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Initialize Native Engine
    g_engine = new Cay::TelexEngine();
    g_engine->OnInjectText = CayIME::InputInjector::ReplaceText;

    g_hookManager = new CayIME::InputHookManager();
    g_hookManager->KeyDown = OnKeyDownHook;
    g_hookManager->KeyUp = OnKeyUpHook;
    g_hookManager->MouseClick = OnMouseClickHook;

    // Run Managed UI
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    Application::Run(gcnew MainForm());

    // Cleanup
    delete g_hookManager;
    delete g_engine;

    return 0;
}
