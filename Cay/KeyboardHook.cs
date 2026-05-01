using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace Cay
{
    public class KeyboardHook : IDisposable
    {
        private const int WH_KEYBOARD_LL = 13;
        private const int WH_MOUSE_LL = 14;
        private const int WM_KEYDOWN = 0x0100;
        private const int WM_KEYUP = 0x0101;
        private const int WM_SYSKEYDOWN = 0x0104;
        private const int WM_SYSKEYUP = 0x0105;
        private const int WM_LBUTTONDOWN = 0x0201;
        private const int WM_RBUTTONDOWN = 0x0204;
        private const int WM_MBUTTONDOWN = 0x0207;

        // Modifier key codes
        private const int VK_LSHIFT = 0xA0;
        private const int VK_RSHIFT = 0xA1;
        private const int VK_LCONTROL = 0xA2;
        private const int VK_RCONTROL = 0xA3;
        private const int VK_LMENU = 0xA4;
        private const int VK_RMENU = 0xA5;
        private const int VK_CAPITAL = 0x14;

        private IntPtr _keyHookId = IntPtr.Zero;
        private IntPtr _mouseHookId = IntPtr.Zero;
        private LowLevelKeyboardProc _keyProc;
        private LowLevelKeyboardProc _mouseProc;
        private bool _disposed;

        // Track modifier states
        private bool _ctrlPressed = false;
        private bool _shiftPressed = false;
        private bool _altPressed = false;

        public event EventHandler<KeyEventArgs> KeyPressed;
        public event EventHandler MouseClicked;
        public event EventHandler CtrlShiftPressed;
        public event EventHandler BreakKeyPressed;

        public KeyboardHook()
        {
            _keyProc = KeyboardCallback;
            _mouseProc = MouseCallback;
        }

        public void Install()
        {
            if (_keyHookId != IntPtr.Zero) return;
            using (var p = System.Diagnostics.Process.GetCurrentProcess())
            using (var m = p.MainModule)
            {
                _keyHookId = SetWindowsHookEx(WH_KEYBOARD_LL, _keyProc, GetModuleHandle(m.ModuleName), 0);
                _mouseHookId = SetWindowsHookEx(WH_MOUSE_LL, _mouseProc, GetModuleHandle(m.ModuleName), 0);
            }
        }

        public void Dispose()
        {
            if (!_disposed)
            {
                if (_keyHookId != IntPtr.Zero)
                    UnhookWindowsHookEx(_keyHookId);
                if (_mouseHookId != IntPtr.Zero)
                    UnhookWindowsHookEx(_mouseHookId);
                _disposed = true;
            }
        }

        private bool IsModifierKey(Keys key)
        {
            return key == Keys.LShiftKey || key == Keys.RShiftKey ||
                   key == Keys.LControlKey || key == Keys.RControlKey ||
                   key == Keys.LMenu || key == Keys.RMenu ||
                   key == Keys.ShiftKey || key == Keys.ControlKey || key == Keys.Menu ||
                   key == Keys.CapsLock || key == Keys.NumLock || key == Keys.Scroll;
        }

        private void UpdateModifierState(Keys key, bool isDown)
        {
            switch (key)
            {
                case Keys.LControlKey:
                case Keys.RControlKey:
                case Keys.ControlKey:
                    _ctrlPressed = isDown;
                    break;
                case Keys.LShiftKey:
                case Keys.RShiftKey:
                case Keys.ShiftKey:
                    _shiftPressed = isDown;
                    break;
                case Keys.LMenu:
                case Keys.RMenu:
                case Keys.Menu:
                    _altPressed = isDown;
                    break;
            }
        }

        private bool IsBreakKey(Keys key)
        {
            // Keys that should clear the buffer
            switch (key)
            {
                case Keys.Escape:
                case Keys.Tab:
                case Keys.Enter:
                case Keys.Left:
                case Keys.Right:
                case Keys.Up:
                case Keys.Down:
                case Keys.Home:
                case Keys.End:
                case Keys.PageUp:
                case Keys.PageDown:
                case Keys.Insert:
                case Keys.Delete:
                case Keys.NumLock:
                case Keys.Scroll:
                case Keys.PrintScreen:
                case Keys.Pause:
                case Keys.Apps:
                case Keys.LWin:
                case Keys.RWin:
                case Keys.F1:
                case Keys.F2:
                case Keys.F3:
                case Keys.F4:
                case Keys.F5:
                case Keys.F6:
                case Keys.F7:
                case Keys.F8:
                case Keys.F9:
                case Keys.F10:
                case Keys.F11:
                case Keys.F12:
                    return true;
            }
            return false;
        }

        private IntPtr KeyboardCallback(int nCode, IntPtr wParam, IntPtr lParam)
        {
            if (nCode < 0) return CallNextHookEx(_keyHookId, nCode, wParam, lParam);

            var kb = Marshal.PtrToStructure<KBDLLHOOKSTRUCT>(lParam);
            var key = (Keys)kb.vkCode;
            bool isDown = wParam == (IntPtr)WM_KEYDOWN || wParam == (IntPtr)WM_SYSKEYDOWN;

            // Update modifier states
            if (IsModifierKey(key))
            {
                UpdateModifierState(key, isDown);

                // Check for Ctrl+Shift combo on key release
                if (!isDown && _ctrlPressed && _shiftPressed)
                {
                    // This is when both Ctrl and Shift are pressed and one is being released
                    // Or check if we're releasing Ctrl or Shift while both were pressed
                    if ((key == Keys.LControlKey || key == Keys.RControlKey || key == Keys.ControlKey) &&
                        !_shiftPressed)
                    {
                        CtrlShiftPressed?.Invoke(this, EventArgs.Empty);
                        return (IntPtr)1;
                    }
                    if ((key == Keys.LShiftKey || key == Keys.RShiftKey || key == Keys.ShiftKey) &&
                        !_ctrlPressed)
                    {
                        CtrlShiftPressed?.Invoke(this, EventArgs.Empty);
                        return (IntPtr)1;
                    }
                }

                return CallNextHookEx(_keyHookId, nCode, wParam, lParam);
            }

            // Check for Ctrl+Shift + any non-modifier key
            if (isDown && _ctrlPressed && _shiftPressed)
            {
                CtrlShiftPressed?.Invoke(this, EventArgs.Empty);
                return (IntPtr)1;
            }

            // Check for break keys
            if (isDown && IsBreakKey(key))
            {
                BreakKeyPressed?.Invoke(this, EventArgs.Empty);
                return CallNextHookEx(_keyHookId, nCode, wParam, lParam);
            }

            // Ctrl pressed alone (without Shift) - clear buffer
            if (isDown && _ctrlPressed && !_shiftPressed && !_altPressed)
            {
                BreakKeyPressed?.Invoke(this, EventArgs.Empty);
            }

            if (isDown)
            {
                char? ch = KeyToChar(key);
                var args = new KeyEventArgs(ch ?? '\0', key);

                KeyPressed?.Invoke(this, args);

                if (args.Handled)
                    return (IntPtr)1;
            }

            return CallNextHookEx(_keyHookId, nCode, wParam, lParam);
        }

        private IntPtr MouseCallback(int nCode, IntPtr wParam, IntPtr lParam)
        {
            if (nCode >= 0)
            {
                if (wParam == (IntPtr)WM_LBUTTONDOWN ||
                    wParam == (IntPtr)WM_RBUTTONDOWN ||
                    wParam == (IntPtr)WM_MBUTTONDOWN)
                {
                    MouseClicked?.Invoke(this, EventArgs.Empty);
                }
            }

            return CallNextHookEx(_mouseHookId, nCode, wParam, lParam);
        }

        private char? KeyToChar(Keys key)
        {
            if (key >= Keys.A && key <= Keys.Z)
            {
                bool shift = (Control.ModifierKeys & Keys.Shift) == Keys.Shift;
                bool caps = Control.IsKeyLocked(Keys.CapsLock);
                char c = (char)key;
                return (shift ^ caps) ? c : char.ToLowerInvariant(c);
            }

            if (key >= Keys.D0 && key <= Keys.D9)
            {
                if ((Control.ModifierKeys & Keys.Shift) == Keys.Shift)
                {
                    string shifted = ")!@#$%^&*(";
                    return shifted[key - Keys.D0];
                }
                return (char)('0' + (key - Keys.D0));
            }

            switch (key)
            {
                case Keys.Space: return ' ';
                case Keys.OemMinus: return Shift() ? '_' : '-';
                case Keys.Oemplus: return Shift() ? '+' : '=';
                case Keys.OemOpenBrackets: return Shift() ? '{' : '[';
                case Keys.OemCloseBrackets: return Shift() ? '}' : ']';
                case Keys.OemPipe: return Shift() ? '|' : '\\';
                case Keys.OemSemicolon: return Shift() ? ':' : ';';
                case Keys.OemQuotes: return Shift() ? '"' : '\'';
                case Keys.Oemcomma: return Shift() ? '<' : ',';
                case Keys.OemPeriod: return Shift() ? '>' : '.';
                case Keys.OemQuestion: return Shift() ? '?' : '/';
                case Keys.Oemtilde: return Shift() ? '~' : '`';
            }

            return null;
        }

        private bool Shift() => (Control.ModifierKeys & Keys.Shift) == Keys.Shift;

        [DllImport("user32.dll")] static extern IntPtr SetWindowsHookEx(int idHook, LowLevelKeyboardProc lpfn, IntPtr hMod, uint dwThreadId);
        [DllImport("user32.dll")] static extern bool UnhookWindowsHookEx(IntPtr hhk);
        [DllImport("user32.dll")] static extern IntPtr CallNextHookEx(IntPtr hhk, int nCode, IntPtr wParam, IntPtr lParam);
        [DllImport("kernel32.dll")] static extern IntPtr GetModuleHandle(string lpModuleName);

        private delegate IntPtr LowLevelKeyboardProc(int nCode, IntPtr wParam, IntPtr lParam);

        [StructLayout(LayoutKind.Sequential)]
        struct KBDLLHOOKSTRUCT
        {
            public uint vkCode, scanCode, flags, time;
            public IntPtr dwExtraInfo;
        }
    }

    public class KeyEventArgs : EventArgs
    {
        public char Character { get; }
        public Keys VirtualKey { get; }
        public bool Handled { get; set; }
        public KeyEventArgs(char c, Keys k) { Character = c; VirtualKey = k; }
    }
}
