using System;
using System.Drawing;
using System.Media;
using System.Windows.Forms;
using Microsoft.Win32;
using Gma.System.MouseKeyHook;
using System.Diagnostics;
using System.Threading;
using BoGoViet.TiengViet;

namespace Cay
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            if (Environment.OSVersion.Version.Major >= 6)
                SetProcessDPIAware();

            string currentUser = Environment.UserName;
            string processName = Process.GetCurrentProcess().ProcessName;
            Mutex mutex = new Mutex(true, @"Global\CayVN" + currentUser + processName);
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
            finally
            {
                mutex.ReleaseMutex();
            }
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern bool SetProcessDPIAware();
    }

    public class CayApplication : ApplicationContext
    {
        private NotifyIcon _trayIcon;
        private IKeyboardMouseEvents _hook;
        private TiengViet _tv;
        private bool _enabled = true;
        private Icon _iconOn, _iconOff;
        private RegistryKey _rk;
        private bool _ctrlPressed = false;
        private bool _pendingToggle = false;

        public CayApplication()
        {
            _tv = new TiengViet();
            _tv.SetKieuGo("Telex");

            CreateIcons();
            SetupTrayIcon();
            SetupHooks();

            _rk = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\Run", true);

            CheckFirstRun();
        }

        private void CreateIcons()
        {
            _iconOn = CreateVIcon(Color.Red);
            _iconOff = CreateVIcon(Color.Gray);
        }

        private Icon CreateVIcon(Color color)
        {
            using (var bmp = new Bitmap(16, 16))
            using (var g = Graphics.FromImage(bmp))
            {
                g.Clear(Color.Transparent);
                using (var brush = new SolidBrush(color))
                using (var font = new Font("Arial", 10, FontStyle.Bold))
                {
                    var size = g.MeasureString("V", font);
                    g.DrawString("V", font, brush, (16 - size.Width) / 2, (16 - size.Height) / 2);
                }
                return Icon.FromHandle(bmp.GetHicon());
            }
        }

        private void SetupTrayIcon()
        {
            _trayIcon = new NotifyIcon
            {
                Icon = _enabled ? _iconOn : _iconOff,
                Text = _enabled ? "Cay - Bật (Ctrl+Shift để tắt)" : "Cay - Tắt (Ctrl+Shift để bật)",
                Visible = true
            };

            var menu = new ContextMenuStrip();
            var miAutoStart = new ToolStripMenuItem("&Tự khởi động", null, (s, e) => ToggleAutoStart());
            miAutoStart.Checked = IsAutoStartEnabled();
            menu.Items.Add(miAutoStart);
            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add("&Thông tin...", null, (s, e) => ShowAboutMessage());
            menu.Items.Add(new ToolStripSeparator());
            menu.Items.Add("T&hoát", null, (s, e) => Exit());

            _trayIcon.ContextMenuStrip = menu;
            _trayIcon.MouseClick += (s, e) =>
            {
                if (e.Button == MouseButtons.Left)
                    Toggle();
            };
        }

        private void SetupHooks()
        {
            _hook = Hook.GlobalEvents();
            _hook.KeyDown += OnKeyDown;
            _hook.KeyUp += OnKeyUp;
            _hook.MouseClick += OnMouseClick;
        }

        private void CheckFirstRun()
        {
            try
            {
                using (var key = Registry.CurrentUser.CreateSubKey(@"Software\Cay"))
                {
                    if (key.GetValue("FirstRun") == null)
                    {
                        var result = MessageBox.Show(
                            "Chào mừng sử dụng Cay - Bộ gõ tiếng Việt!\n\n" +
                            "Bạn có muốn Cay khởi động cùng Windows không?\n\n" +
                            "Có: Tự động chạy khi Windows khởi động\n" +
                            "Không: Mở thủ công khi cần",
                            "Khởi động cùng Windows",
                            MessageBoxButtons.YesNo, MessageBoxIcon.Question);

                        if (result == DialogResult.Yes)
                            _rk.SetValue("Cay", Application.ExecutablePath);
                        else
                            _rk.DeleteValue("Cay", false);

                        key.SetValue("FirstRun", 0, RegistryValueKind.DWord);
                    }
                }
            }
            catch { }
        }

        private void OnKeyDown(object sender, KeyEventArgs e)
        {
            // Track Ctrl state
            if (e.KeyCode == Keys.LControlKey || e.KeyCode == Keys.RControlKey)
            {
                _ctrlPressed = true;
                return;
            }

            // Ctrl+Shift toggle: hold Ctrl, press Shift
            if (_ctrlPressed && (e.KeyCode == Keys.LShiftKey || e.KeyCode == Keys.RShiftKey))
            {
                _pendingToggle = true;
                return;
            }

            // Alt key handling for engine
            if (e.KeyCode == Keys.LMenu || e.KeyCode == Keys.RMenu)
            {
                _hook.KeyDown -= OnKeyDown;
                _hook.KeyUp -= OnKeyUp;
                _tv.OnKeyUp(ref sender, ref e);
                _hook.KeyDown += OnKeyDown;
                _hook.KeyUp += OnKeyUp;
                return;
            }

            // Cancel pending toggle if other key pressed
            if (_pendingToggle)
                _pendingToggle = false;

            // Clear buffer on Ctrl+ shortcuts (A, C, V, X, Z, etc.)
            if (_ctrlPressed && (e.KeyCode >= Keys.A && e.KeyCode <= Keys.Z))
            {
                _tv.Reset();
                return;
            }

            if (!_enabled) return;

            // Unsubscribe to prevent re-entrant handling of simulated keys (Backspace from SendText)
            _hook.KeyDown -= OnKeyDown;
            _hook.KeyUp -= OnKeyUp;
            _tv.OnKeyDown(ref sender, ref e);
            _hook.KeyDown += OnKeyDown;
            _hook.KeyUp += OnKeyUp;
        }

        private void OnKeyUp(object sender, KeyEventArgs e)
        {
            // Track Ctrl release
            if (e.KeyCode == Keys.LControlKey || e.KeyCode == Keys.RControlKey)
            {
                _ctrlPressed = false;
                if (_pendingToggle)
                {
                    _pendingToggle = false;
                    Toggle();
                }
                return;
            }

            // Track Shift release for toggle
            if (_pendingToggle && (e.KeyCode == Keys.LShiftKey || e.KeyCode == Keys.RShiftKey))
            {
                _pendingToggle = false;
                Toggle();
                return;
            }

            // Unsubscribe to prevent re-entrant handling
            _hook.KeyDown -= OnKeyDown;
            _hook.KeyUp -= OnKeyUp;
            _tv.OnKeyUp(ref sender, ref e);
            _hook.KeyDown += OnKeyDown;
            _hook.KeyUp += OnKeyUp;
        }

        private void OnMouseClick(object sender, MouseEventArgs e)
        {
            _tv.OnMouseClick(ref sender, ref e);
        }

        private void Toggle()
        {
            _enabled = !_enabled;
            _trayIcon.Icon = _enabled ? _iconOn : _iconOff;
            _trayIcon.Text = _enabled ? "Cay - Bật (Ctrl+Shift để tắt)" : "Cay - Tắt (Ctrl+Shift để bật)";
            _tv.Reset();
            SystemSounds.Beep.Play();
        }

        private void ToggleAutoStart()
        {
            bool enabled = !IsAutoStartEnabled();
            try
            {
                if (enabled)
                    _rk.SetValue("Cay", Application.ExecutablePath);
                else
                    _rk.DeleteValue("Cay", false);

                if (_trayIcon.ContextMenuStrip?.Items[0] is ToolStripMenuItem mi)
                    mi.Checked = enabled;
            }
            catch { }
        }

        private bool IsAutoStartEnabled()
        {
            try { return _rk.GetValue("Cay") != null; }
            catch { return false; }
        }

        private void ShowAboutMessage()
        {
            MessageBox.Show(
                "Cay - Bộ gõ tiếng Việt (Telex) v2.0\n\n" +
                "Phím tắt: Ctrl+Shift = Bật/Tắt\n" +
                "Ctrl = Kết thúc từ\n\n" +
                "Telex: aa→â, aw→ă, dd→đ, ee→ê, oo→ô, ow→ơ, uw→ư\n" +
                "Dấu: s/f/r/x/j = sắc/huyền/hỏi/ngã/nặng\n\n" +
                "© 2025 Cay Vietnamese IME",
                "Giới thiệu - Cay",
                MessageBoxButtons.OK,
                MessageBoxIcon.Information);
        }

        private void Exit()
        {
            _hook?.Dispose();
            _trayIcon?.Dispose();
            _iconOn?.Dispose();
            _iconOff?.Dispose();
            _rk?.Dispose();
            Application.Exit();
        }
    }
}
