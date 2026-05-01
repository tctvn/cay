using System;
using System.Drawing;
using System.Windows.Forms;
using System.Threading;
using Microsoft.Win32;
using System.IO;
using System.Reflection;

namespace Cay
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            bool createdNew;
            using (var mutex = new Mutex(true, "CayVNInput", out createdNew))
            {
                if (!createdNew)
                {
                    MessageBox.Show("Cay đang chạy.", "Cay", MessageBoxButtons.OK, MessageBoxIcon.Information);
                    return;
                }

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);

                using (var app = new CayApplication())
                    Application.Run();
            }
        }
    }

    public class CayApplication : IDisposable
    {
        private NotifyIcon _trayIcon;
        private KeyboardHook _hook;
        private TelexEngine _engine;
        private bool _enabled = true;
        private Icon _iconOn, _iconOff;
        
        private const string RegKey = @"Software\Cay";
        private const string AutoStartKey = @"Software\Microsoft\Windows\CurrentVersion\Run";

        public CayApplication()
        {
            // Load settings from registry only
            _enabled = LoadEnabledState();
            
            CreateIcons();
            InitializeTray();
            
            _hook = new KeyboardHook();
            _engine = new TelexEngine();
            _hook.KeyPressed += OnKeyPressed;
            _hook.MouseClicked += (s, e) => _engine.ClearBuffer();
            _hook.CtrlShiftPressed += (s, e) => Toggle();
            _hook.BreakKeyPressed += (s, e) => _engine.ClearBuffer();
            _hook.Install();
        }

        private void CreateIcons()
        {
            // V đỏ - bật
            using (var bmp = new Bitmap(16, 16))
            using (var g = Graphics.FromImage(bmp))
            {
                g.Clear(Color.Transparent);
                using (var font = new Font("Segoe UI", 11, FontStyle.Bold))
                using (var brush = new SolidBrush(Color.FromArgb(220, 53, 69)))
                {
                    g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;
                    g.DrawString("V", font, brush, -1, -1);
                }
                _iconOn = Icon.FromHandle(bmp.GetHicon());
            }

            // E xanh - tắt
            using (var bmp = new Bitmap(16, 16))
            using (var g = Graphics.FromImage(bmp))
            {
                g.Clear(Color.Transparent);
                using (var font = new Font("Segoe UI", 11, FontStyle.Bold))
                using (var brush = new SolidBrush(Color.FromArgb(0, 123, 255)))
                {
                    g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;
                    g.DrawString("E", font, brush, -1, -1);
                }
                _iconOff = Icon.FromHandle(bmp.GetHicon());
            }
        }

        private void InitializeTray()
        {
            var menu = new ContextMenuStrip();

            // Auto start
            var miAutoStart = new ToolStripMenuItem("&Tự khởi động", null, (s, e) => ToggleAutoStart());
            miAutoStart.Checked = IsAutoStartEnabled();
            menu.Items.Add(miAutoStart);

            menu.Items.Add(new ToolStripSeparator());

            // About
            menu.Items.Add("&Thông tin...", null, (s, e) => ShowAbout());
            menu.Items.Add(new ToolStripSeparator());

            // Exit
            menu.Items.Add("T&hoát", null, (s, e) => Exit());

            _trayIcon = new NotifyIcon
            {
                Icon = _enabled ? _iconOn : _iconOff,
                Text = _enabled ? "Cay - Đang bật (Ctrl+Shift để tắt)" : "Cay - Đang tắt (Ctrl+Shift để bật)",
                ContextMenuStrip = menu,
                Visible = true
            };

            // Click trái = toggle
            _trayIcon.MouseClick += (s, e) =>
            {
                if (e.Button == MouseButtons.Left)
                    Toggle();
            };
        }

        private void OnKeyPressed(object sender, KeyEventArgs e)
        {
            if (!_enabled) return;
            if (e.Character == '\0') return;

            // Check for word separator
            if (IsSeparator(e.Character))
            {
                _engine.ClearBuffer();
                return;
            }

            // Process key
            if (_engine.ProcessKey(e.Character))
            {
                e.Handled = true;
                if (_engine.Backspaces > 0 || !string.IsNullOrEmpty(_engine.Output))
                {
                    SendBackspaces(_engine.Backspaces);
                    SendText(_engine.Output);
                }
            }
        }

        private bool IsSeparator(char c)
        {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n' || 
                   char.IsPunctuation(c) || char.IsDigit(c);
        }

        private void Toggle()
        {
            _enabled = !_enabled;
            _trayIcon.Icon = _enabled ? _iconOn : _iconOff;
            _trayIcon.Text = _enabled ? "Cay - Đang bật (Ctrl+Shift để tắt)" : "Cay - Đang tắt (Ctrl+Shift để bật)";

            _engine.ClearBuffer();

            // Save state to registry
            SaveEnabledState(_enabled);
        }

        private void ToggleAutoStart()
        {
            bool enabled = !IsAutoStartEnabled();
            SetAutoStart(enabled);
            RebuildMenu();
        }

        private bool IsAutoStartEnabled()
        {
            try
            {
                using (var key = Registry.CurrentUser.OpenSubKey(AutoStartKey))
                    return key?.GetValue("Cay") != null;
            }
            catch { return false; }
        }

        private void SetAutoStart(bool enabled)
        {
            try
            {
                using (var key = Registry.CurrentUser.OpenSubKey(AutoStartKey, true))
                {
                    if (enabled)
                        key.SetValue("Cay", Assembly.GetExecutingAssembly().Location);
                    else if (key.GetValue("Cay") != null)
                        key.DeleteValue("Cay");
                }
            }
            catch { }
        }

        private bool LoadEnabledState()
        {
            try
            {
                using (var key = Registry.CurrentUser.OpenSubKey(RegKey))
                    return key?.GetValue("Enabled", 1) as int? != 0;
            }
            catch { return true; }
        }

        private void SaveEnabledState(bool enabled)
        {
            try
            {
                using (var key = Registry.CurrentUser.CreateSubKey(RegKey))
                    key.SetValue("Enabled", enabled ? 1 : 0);
            }
            catch { }
        }

        private void RebuildMenu()
        {
            // Just update the auto-start menu item checked state instead of rebuilding
            // to avoid crash when menu is open
            if (_trayIcon.ContextMenuStrip.Items[0] is ToolStripMenuItem miAutoStart)
            {
                miAutoStart.Checked = IsAutoStartEnabled();
            }
        }

        private void ShowAbout()
        {
            using (var f = new AboutForm())
                f.ShowDialog();
        }

        private void Exit()
        {
            Application.Exit();
        }

        private void SendBackspaces(int count)
        {
            for (int i = 0; i < count; i++)
                SendKeys.SendWait("{BACKSPACE}");
        }

        private void SendText(string text)
        {
            if (!string.IsNullOrEmpty(text))
                SendKeys.SendWait(text);
        }

        public void Dispose()
        {
            _hook?.Dispose();
            _trayIcon?.Dispose();
            _iconOn?.Dispose();
            _iconOff?.Dispose();
        }
    }
}
