using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using CayIME;

namespace Cay
{
    public partial class TelexEngine
    {
        private List<MyKey> _buffer = new List<MyKey>();
        private string _lastOutput = "";
        private bool _bypassCurrentWord = false;

        public void ResetFull()
        {
            _buffer.Clear();
            _lastOutput = "";
            _bypassCurrentWord = false;
        }

        public void OnKeyDown(ref object sender, ref KeyEventArgs e)
        {
            if (_bypassCurrentWord)
            {
                if (e.KeyCode < Keys.A || e.KeyCode > Keys.Z) ResetFull();
                return;
            }

            if (e.KeyCode == Keys.Back)
            {
                if (_buffer.Count > 0)
                {
                    e.Handled = true;
                    e.SuppressKeyPress = true;
                    _buffer.RemoveAt(_buffer.Count - 1);
                    if (_buffer.Count == 0) UpdateScreen("");
                    else Run(ref e, true);
                }
                else ResetFull();
                return;
            }

            if (e.KeyCode >= Keys.A && e.KeyCode <= Keys.Z)
            {
                bool isShift = (Control.ModifierKeys & Keys.Shift) != 0;
                bool isCaps = InputInjector.IsCapsLockOn();
                bool isUpper = isShift ^ isCaps;

                _buffer.Add(new MyKey(e.KeyCode, isUpper));
                e.Handled = true;
                e.SuppressKeyPress = true;
                Run(ref e, false);
            }
            else
            {
                ResetFull();
            }
        }

        public void OnKeyUp(ref object sender, ref KeyEventArgs e)
        {
            // Usually not needed for Telex logic unless we track long presses
        }

        private void UpdateScreen(string newOutput)
        {
            if (newOutput == _lastOutput) return;

            // Xóa chữ cũ
            InputInjector.SendBackspaces(_lastOutput.Length);

            // Bơm chữ mới
            bool shiftWasDown = InputInjector.IsKeyDown(Keys.ShiftKey) || InputInjector.IsKeyDown(Keys.LShiftKey) || InputInjector.IsKeyDown(Keys.RShiftKey);
            
            if (shiftWasDown) InputInjector.SendShiftUp();
            InputInjector.SendUnicodeString(newOutput);
            if (shiftWasDown) InputInjector.SendShiftDown();

            _lastOutput = newOutput;
        }

        private void BypassCurrentWord(string rawOutput)
        {
            UpdateScreen(rawOutput);
            _buffer.Clear();
            _lastOutput = "";
            _bypassCurrentWord = true;
        }
    }
}
