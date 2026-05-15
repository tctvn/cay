using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace CayIME
{
    public static unsafe class InputInjector
    {
        public static readonly System.IntPtr MAGIC_EXTRA_INFO = new System.IntPtr(0x1234);

        [StructLayout(LayoutKind.Sequential)]
        struct KEYBDINPUT
        {
            public ushort wVk;
            public ushort wScan;
            public uint dwFlags;
            public uint time;
            public System.IntPtr dwExtraInfo;
        }

        [StructLayout(LayoutKind.Explicit, Size = 40)]
        struct INPUT
        {
            [FieldOffset(0)] public int type;
            [FieldOffset(8)] public KEYBDINPUT ki;
        }

        [DllImport("user32.dll")]
        static extern uint SendInput(uint n, INPUT* inputs, int size);

        [DllImport("user32.dll")]
        static extern short GetAsyncKeyState(int vk);

        [DllImport("user32.dll")]
        static extern short GetKeyState(int vk);

        private const int INPUT_KEYBOARD = 1;
        private const uint KEYEVENTF_KEYUP = 0x0002;
        private const uint KEYEVENTF_UNICODE = 0x0004;
        private const ushort VK_BACK = 0x08;

        public static void SendBackspaces(int count)
        {
            if (count <= 0) return;
            var inputs = stackalloc INPUT[count * 2];
            for (int i = 0; i < count; i++)
            {
                inputs[i * 2].type = INPUT_KEYBOARD;
                inputs[i * 2].ki = new KEYBDINPUT { wVk = VK_BACK, dwFlags = 0, dwExtraInfo = MAGIC_EXTRA_INFO };
                inputs[i * 2 + 1].type = INPUT_KEYBOARD;
                inputs[i * 2 + 1].ki = new KEYBDINPUT { wVk = VK_BACK, dwFlags = KEYEVENTF_KEYUP, dwExtraInfo = MAGIC_EXTRA_INFO };
            }
            SendInput((uint)(count * 2), inputs, sizeof(INPUT));
        }

        public static void SendUnicodeString(string text)
        {
            if (string.IsNullOrEmpty(text)) return;
            var inputs = stackalloc INPUT[text.Length * 2];
            for (int i = 0; i < text.Length; i++)
            {
                inputs[i * 2].type = INPUT_KEYBOARD;
                inputs[i * 2].ki = new KEYBDINPUT { wScan = text[i], dwFlags = KEYEVENTF_UNICODE, dwExtraInfo = MAGIC_EXTRA_INFO };
                inputs[i * 2 + 1].type = INPUT_KEYBOARD;
                inputs[i * 2 + 1].ki = new KEYBDINPUT { wScan = text[i], dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP, dwExtraInfo = MAGIC_EXTRA_INFO };
            }
            SendInput((uint)(text.Length * 2), inputs, sizeof(INPUT));
        }

        public static bool IsKeyDown(Keys key) => (GetAsyncKeyState((int)key) & 0x8000) != 0;
        public static bool IsCapsLockOn() => (GetKeyState(0x14) & 1) != 0;

        public static void SendShiftUp()
        {
            var i = stackalloc INPUT[1];
            i[0].type = INPUT_KEYBOARD;
            i[0].ki = new KEYBDINPUT { wVk = 0x10, dwFlags = KEYEVENTF_KEYUP, dwExtraInfo = MAGIC_EXTRA_INFO };
            SendInput(1, i, sizeof(INPUT));
        }

        public static void SendShiftDown()
        {
            var i = stackalloc INPUT[1];
            i[0].type = INPUT_KEYBOARD;
            i[0].ki = new KEYBDINPUT { wVk = 0x10, dwFlags = 0, dwExtraInfo = MAGIC_EXTRA_INFO };
            SendInput(1, i, sizeof(INPUT));
        }
    }
}
