using System;
using System.Drawing;
using System.Media;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Win32;
using System.Diagnostics;
using Cay;

namespace CayIME
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            if (Environment.OSVersion.Version.Major >= 6) SetProcessDPIAware();

            string mutexName = @"Global\CayVN" + Environment.UserName + Process.GetCurrentProcess().ProcessName;
            Mutex mutex = new Mutex(true, mutexName);
            if (!mutex.WaitOne(TimeSpan.Zero, true))
            {
                MessageBox.Show("Cay đang chạy!", "Thông báo", MessageBoxButtons.OK, MessageBoxIcon.Information);
                return;
            }
            try
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new CayApplication());
            }
            finally { mutex.ReleaseMutex(); }
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        static extern bool SetProcessDPIAware();
    }

    public class CayApplication : ApplicationContext
    {
        private NotifyIcon _trayIcon;
        private TelexEngine _engine;
        private InputHookManager _hook;
        private bool _enabled = true;
        private Icon _iconOn, _iconOff;
        private RegistryKey _rk;
        private bool _ctrl, _win, _alt, _pendingToggle;

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        static extern bool DestroyIcon(IntPtr h);

        public CayApplication()
        {
            _engine = new TelexEngine();
            _hook   = new InputHookManager();
            CreateIcons();
            try { _rk = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\Run", true); } catch { }
            SetupTray();
            _hook.KeyDown    += OnKeyDown;
            _hook.KeyUp      += OnKeyUp;
            _hook.MouseClick += (s, e) => _engine.ResetFull();
        }

        private void CreateIcons()
        {
            _iconOn  = MakeIcon(Color.Red);
            _iconOff = MakeIcon(Color.Gray);
        }

        private Icon MakeIcon(Color color)
        {
            IntPtr hIcon = IntPtr.Zero;
            try
            {
                using (var bmp = new Bitmap(16, 16))
                using (var g = Graphics.FromImage(bmp))
                {
                    g.Clear(Color.Transparent);
                    using (var br = new SolidBrush(color))
                    using (var f = new Font("Arial", 10, FontStyle.Bold))
                    {
                        var sz = g.MeasureString("V", f);
                        g.DrawString("V", f, br, (16 - sz.Width) / 2, (16 - sz.Height) / 2);
                    }
                    hIcon = bmp.GetHicon();
                }
                using (var t = Icon.FromHandle(hIcon)) return (Icon)t.Clone();
            }
            finally { if (hIcon != IntPtr.Zero) DestroyIcon(hIcon); }
        }

        private void SetupTray()
        {
            _trayIcon = new NotifyIcon
            {
                Icon    = _iconOn,
                Text    = "Cay - Bật (Ctrl+Shift để tắt)",
                Visible = true
            };
            var menu = new ContextMenuStrip();
            var miAuto = new ToolStripMenuItem("&Tự khởi động", null, (s, e) => ToggleAutoStart()) { Checked = IsAutoStart() };
            menu.Items.Add(miAuto);
            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add("&Thông tin", null, (s, e) => ShowAbout());
            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add("T&hoát", null, (s, e) => Exit());
            _trayIcon.ContextMenuStrip = menu;
            _trayIcon.MouseClick += (s, e) => { if (e.Button == MouseButtons.Left) Toggle(); };
        }

        private void OnKeyDown(object sender, HookKeyEventArgs e)
        {
            if (e.ExtraInfo == InputInjector.MAGIC_EXTRA_INFO) return;

            if (e.KeyCode == Keys.LControlKey || e.KeyCode == Keys.RControlKey)
            { _ctrl = true; _engine.ResetFull(); return; }
            if (e.KeyCode == Keys.LWin || e.KeyCode == Keys.RWin)
            { _win = true; _engine.ResetFull(); return; }
            if (e.KeyCode == Keys.LMenu || e.KeyCode == Keys.RMenu)
            { _alt = true; _engine.ResetFull(); return; }

            if (_ctrl && (e.KeyCode == Keys.LShiftKey || e.KeyCode == Keys.RShiftKey))
            { _pendingToggle = true; return; }

            if (_ctrl || _win || _alt) return;
            if (!_enabled) return;

            KeyEventArgs ka = e; object dummy = null;
            _engine.OnKeyDown(ref dummy, ref ka);
        }

        private void OnKeyUp(object sender, HookKeyEventArgs e)
        {
            if (e.ExtraInfo == InputInjector.MAGIC_EXTRA_INFO) return;

            if (e.KeyCode == Keys.LControlKey || e.KeyCode == Keys.RControlKey)
            { _ctrl = false; if (_pendingToggle) { _pendingToggle = false; Toggle(); } return; }
            if (e.KeyCode == Keys.LWin || e.KeyCode == Keys.RWin) { _win = false; return; }
            if (e.KeyCode == Keys.LMenu || e.KeyCode == Keys.RMenu) { _alt = false; return; }
            if (_pendingToggle && (e.KeyCode == Keys.LShiftKey || e.KeyCode == Keys.RShiftKey))
            { _pendingToggle = false; Toggle(); return; }

            if (!_enabled) return;
            KeyEventArgs ka = e; object dummy = null;
            _engine.OnKeyUp(ref dummy, ref ka);
        }

        private void Toggle()
        {
            _enabled = !_enabled;
            _trayIcon.Icon = _enabled ? _iconOn : _iconOff;
            _trayIcon.Text = _enabled ? "Cay - Bật (Ctrl+Shift để tắt)" : "Cay - Tắt (Ctrl+Shift để bật)";
            _engine.ResetFull();
            SystemSounds.Beep.Play();
        }

        private void ToggleAutoStart()
        {
            bool on = !IsAutoStart();
            try
            {
                if (on) _rk.SetValue("Cay", Application.ExecutablePath);
                else    _rk.DeleteValue("Cay", false);
                if (_trayIcon.ContextMenuStrip?.Items[0] is ToolStripMenuItem mi) mi.Checked = on;
            }
            catch { }
        }

        private bool IsAutoStart() { try { return _rk?.GetValue("Cay") != null; } catch { return false; } }

        private void ShowAbout() =>
            MessageBox.Show(
                "Cay – Bộ gõ tiếng Việt Telex v1.0\n\n" +
                "Ctrl+Shift = Bật / Tắt\n\n" +
                "aa→â  aw→ă  dd→đ  ee→ê  oo→ô  ow→ơ  uw→ư\n" +
                "s=sắc  f=huyền  r=hỏi  x=ngã  j=nặng",
                "Giới thiệu", MessageBoxButtons.OK, MessageBoxIcon.Information);

        private void Exit()
        {
            _hook?.Dispose();
            _trayIcon?.Dispose();
            _iconOn?.Dispose(); _iconOff?.Dispose();
            _rk?.Dispose();
            Application.Exit();
        }
    }
}
