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
        private List<MyKey> _savedBuffer = new List<MyKey>();
        private string _savedOutput = "";
        private bool _canRestore = false;

        public void ResetFull()
        {
            _buffer.Clear();
            _lastOutput = "";
            _bypassCurrentWord = false;
            _canRestore = false;
            _savedBuffer.Clear();
            _savedOutput = "";
        }

        private void CommitWord()
        {
            if (_buffer.Count > 0)
            {
                _savedBuffer = new List<MyKey>(_buffer);
                _savedOutput = _lastOutput;
                _canRestore = true;
            }
            else
            {
                _canRestore = false;
            }
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
                else if (_canRestore)
                {
                    // Restore the previous word state
                    _buffer = new List<MyKey>(_savedBuffer);
                    _lastOutput = _savedOutput;
                    _canRestore = false;
                    // Let the OS handle the Backspace to delete the Space/Punctuation on screen
                }
                else
                {
                    ResetFull();
                }
                return;
            }

            if (e.KeyCode >= Keys.A && e.KeyCode <= Keys.Z)
            {
                _canRestore = false; // Typing a new letter invalidates restore
                bool isShift = (Control.ModifierKeys & Keys.Shift) != 0;
                bool isCaps = InputInjector.IsCapsLockOn();
                bool isUpper = isShift ^ isCaps;

                _buffer.Add(new MyKey(e.KeyCode, isUpper));
                e.Handled = true;
                e.SuppressKeyPress = true;
                Run(ref e, false);
            }
            else if (e.KeyCode == Keys.Space || e.KeyCode == Keys.Enter || e.KeyCode == Keys.Oemcomma || e.KeyCode == Keys.OemPeriod)
            {
                // When typing space or basic punctuation, save the word state so Backspace can restore it
                CommitWord();
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

            int commonPrefixLen = 0;
            int minLen = System.Math.Min(_lastOutput.Length, newOutput.Length);
            for (int i = 0; i < minLen; i++)
            {
                if (_lastOutput[i] == newOutput[i]) commonPrefixLen++;
                else break;
            }

            int backspacesNeeded = _lastOutput.Length - commonPrefixLen;
            string textToType = newOutput.Substring(commonPrefixLen);

            if (backspacesNeeded > 0)
            {
                string dummy = "";
                if (_lastOutput.Length > 0)
                {
                    dummy = "\u200D"; // Zero Width Joiner: breaks selection without triggering Chrome autocomplete
                    backspacesNeeded++;
                }
                InputInjector.ReplaceText(dummy, backspacesNeeded, textToType);
            }
            else
            {
                InputInjector.SendUnicodeString(textToType);
            }

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
