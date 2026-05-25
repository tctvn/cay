using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace CayIME
{
    public class HookKeyEventArgs : KeyEventArgs
    {
        public IntPtr ExtraInfo { get; }
        public HookKeyEventArgs(Keys keyData, IntPtr extraInfo) : base(keyData) { ExtraInfo = extraInfo; }
    }

    public class InputHookManager : IDisposable
    {
        private const int WH_KEYBOARD_LL = 13, WH_MOUSE_LL = 14;
        private const int WM_KEYDOWN = 0x0100, WM_KEYUP = 0x0101;
        private const int WM_SYSKEYDOWN = 0x0104, WM_SYSKEYUP = 0x0105;
        private const int WM_LBUTTONDOWN = 0x0201, WM_RBUTTONDOWN = 0x0204, WM_MBUTTONDOWN = 0x0207;

        private delegate IntPtr LowLevelProc(int nCode, IntPtr wParam, IntPtr lParam);
        private LowLevelProc _kbProc, _msProc;
        private IntPtr _kbHookID = IntPtr.Zero, _msHookID = IntPtr.Zero;

        public event EventHandler<HookKeyEventArgs> KeyDown;
        public event EventHandler<HookKeyEventArgs> KeyUp;
        public event EventHandler MouseClick;

        [StructLayout(LayoutKind.Sequential)]
        public struct KBDLLHOOKSTRUCT
        {
            public uint vkCode, scanCode, flags, time;
            public IntPtr dwExtraInfo;
        }

        public InputHookManager()
        {
            _kbProc = KeyboardHookCallback;
            _msProc = MouseHookCallback;
            using (var p = Process.GetCurrentProcess())
            using (var m = p.MainModule)
            {
                IntPtr hMod = GetModuleHandle(m.ModuleName);
                _kbHookID = SetWindowsHookEx(WH_KEYBOARD_LL, _kbProc, hMod, 0);
                _msHookID = SetWindowsHookEx(WH_MOUSE_LL, _msProc, hMod, 0);
            }
        }

        private IntPtr KeyboardHookCallback(int nCode, IntPtr wParam, IntPtr lParam)
        {
            if (nCode >= 0)
            {
                try
                {
                    int msg = wParam.ToInt32();
                    var kb = (KBDLLHOOKSTRUCT)Marshal.PtrToStructure(lParam, typeof(KBDLLHOOKSTRUCT));
                    var e = new HookKeyEventArgs((Keys)kb.vkCode, kb.dwExtraInfo);
                    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) { KeyDown?.Invoke(this, e); if (e.Handled || e.SuppressKeyPress) return (IntPtr)1; }
                    else if (msg == WM_KEYUP || msg == WM_SYSKEYUP) { KeyUp?.Invoke(this, e); if (e.Handled || e.SuppressKeyPress) return (IntPtr)1; }
                }
                catch { }
            }
            return CallNextHookEx(_kbHookID, nCode, wParam, lParam);
        }

        private IntPtr MouseHookCallback(int nCode, IntPtr wParam, IntPtr lParam)
        {
            if (nCode >= 0 && (wParam == (IntPtr)WM_LBUTTONDOWN || wParam == (IntPtr)WM_RBUTTONDOWN || wParam == (IntPtr)WM_MBUTTONDOWN))
            {
                try { MouseClick?.Invoke(this, EventArgs.Empty); } catch { }
            }
            return CallNextHookEx(_msHookID, nCode, wParam, lParam);
        }

        public void Dispose()
        {
            if (_kbHookID != IntPtr.Zero) { UnhookWindowsHookEx(_kbHookID); _kbHookID = IntPtr.Zero; }
            if (_msHookID != IntPtr.Zero) { UnhookWindowsHookEx(_msHookID); _msHookID = IntPtr.Zero; }
        }

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern IntPtr SetWindowsHookEx(int idHook, LowLevelProc lpfn, IntPtr hMod, uint dwThreadId);
        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool UnhookWindowsHookEx(IntPtr hhk);
        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern IntPtr CallNextHookEx(IntPtr hhk, int nCode, IntPtr wParam, IntPtr lParam);
        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern IntPtr GetModuleHandle(string lpModuleName);
    }
}
