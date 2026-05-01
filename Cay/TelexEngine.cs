using System;
using System.Text;

namespace Cay
{
    public class TelexEngine
    {
        private StringBuilder _buf = new StringBuilder(40);
        
        public int Backspaces { get; private set; }
        public string Output { get; private set; }
        
        // Free marking always enabled - gõ tự do
        private const bool FreeMarking = true;

        public bool ProcessKey(char c)
        {
            Backspaces = 0;
            Output = null;

            char lower = char.ToLowerInvariant(c);

            // Backspace
            if (c == '\b')
            {
                if (_buf.Length > 0) _buf.Length--;
                return false;
            }

            // Separator
            if (IsSep(c))
            {
                ClearBuffer();
                return false;
            }

            // If buffer is empty, this is first char of word
            // Just append and let it through - fixes IDE/Chrome address bar issues
            if (_buf.Length == 0)
            {
                // Only process if it's a potential Vietnamese key
                if (lower == 'w' || lower == 'd' ||
                    lower == 's' || lower == 'f' || lower == 'r' || lower == 'x' || lower == 'j')
                {
                    // These are control keys (tone/marks), let them through as-is at start
                    Append(c);
                    return false;
                }

                // For normal letters, just add to buffer
                Append(c);
                return false;
            }

            // Check key type for subsequent characters
            // Only process tone/mark keys if we have at least one vowel in buffer
            bool hasVowel = HasVowelInBuffer();

            if (lower == 'w')
                return ProcessW(c);

            if (lower == 'a' || lower == 'e' || lower == 'o' || lower == 'd')
                return ProcessDouble(c);

            if (lower == 's' || lower == 'f' || lower == 'r' || lower == 'x' || lower == 'j')
            {
                // If no vowel in buffer, just append the tone key as normal character
                if (!hasVowel)
                {
                    Append(c);
                    return false;
                }
                return ProcessTone(lower);
            }

            // Normal char - just append
            Append(c);
            return false;
        }

        private bool HasVowelInBuffer()
        {
            for (int i = 0; i < _buf.Length; i++)
            {
                if (IsVowel(_buf[i]))
                    return true;
            }
            return false;
        }

        private bool ProcessW(char c)
        {
            // w -> ă, ơ, ư, or toggle
            // If no vowel in buffer, just append as normal char
            if (!HasVowelInBuffer())
            {
                Append(c);
                return false;
            }

            int start = FreeMarking ? 0 : Math.Max(0, _buf.Length - 6);

            for (int i = _buf.Length - 1; i >= start; i--)
            {
                char ch = _buf[i];
                char target = '\0';

                switch (char.ToLowerInvariant(GetBase(ch)))
                {
                    case 'a': target = 'ă'; break;
                    case 'o': target = 'ơ'; break;
                    case 'u': target = 'ư'; break;
                }

                if (target != '\0')
                {
                    int tone = GetTone(ch);
                    if (tone >= 0) target = AddTone(target, tone);

                    Backspaces = _buf.Length - i;
                    Output = BuildFrom(i, target);
                    _buf[i] = target;
                    return true;
                }

                if (IsSep(ch)) break;
            }

            Append(c);
            return false;
        }

        private bool ProcessDouble(char c)
        {
            char lower = char.ToLowerInvariant(c);

            // dd -> đ (always process, as đ is a consonant that can start a word)
            // aa -> â, ee -> ê, oo -> ô (only process if we have vowels or this could be Vietnamese)
            bool hasVowel = HasVowelInBuffer();

            int start = FreeMarking ? 0 : Math.Max(0, _buf.Length - 6);

            for (int i = _buf.Length - 1; i >= start; i--)
            {
                char ch = _buf[i];
                char baseCh = GetBase(ch);

                // dd -> đ
                if (lower == 'd' && baseCh == 'd')
                {
                    char target = char.IsUpper(c) ? 'Đ' : 'đ';
                    Backspaces = _buf.Length - i;
                    Output = BuildFrom(i, target);
                    _buf[i] = target;
                    return true;
                }

                // aa -> â, ee -> ê, oo -> ô
                // Only process if we have vowels in buffer (to avoid non-Vietnamese words like "book" becoming "bôk")
                if (baseCh == lower && hasVowel)
                {
                    char target = '\0';
                    if (lower == 'a') target = 'â';
                    else if (lower == 'e') target = 'ê';
                    else if (lower == 'o') target = 'ô';

                    if (target != '\0')
                    {
                        // Skip "oeo" pattern
                        if (lower == 'o' && i < _buf.Length - 1 &&
                            char.ToLowerInvariant(_buf[i + 1]) == 'e')
                            break;

                        int tone = GetTone(ch);
                        if (tone >= 0) target = AddTone(target, tone);

                        Backspaces = _buf.Length - i;
                        Output = BuildFrom(i, target);
                        _buf[i] = target;
                        return true;
                    }
                }

                if (IsSep(ch)) break;
            }

            Append(c);
            return false;
        }

        private bool ProcessTone(char toneKey)
        {
            int tone = "sfrxj".IndexOf(toneKey);
            if (tone < 0) return false;

            // Find last vowel
            int lastVowel = -1;
            int start = Math.Max(0, _buf.Length - 6);

            for (int i = _buf.Length - 1; i >= start; i--)
            {
                if (IsVowel(_buf[i]))
                {
                    lastVowel = i;
                    break;
                }
                if (IsSep(_buf[i])) break;
            }

            if (lastVowel < 0)
            {
                Append(toneKey);
                return false;
            }

            // Find vowel sequence
            int firstVowel = lastVowel;
            for (int i = lastVowel; i >= Math.Max(0, lastVowel - 2); i--)
            {
                if (!IsVowel(_buf[i])) break;
                firstVowel = i;
            }

            // Determine correct position based on Vietnamese spelling rules
            int count = lastVowel - firstVowel + 1;
            int correctPos = lastVowel;

            if (count == 2)
            {
                // Special rules for 2-vowel sequences
                char v1 = char.ToLowerInvariant(GetBase(_buf[firstVowel]));
                char v2 = char.ToLowerInvariant(GetBase(_buf[lastVowel]));

                // ia, ie, iê, yê -> tone on second vowel (a, e, ê)
                if (v1 == 'i' || v1 == 'y')
                    correctPos = lastVowel;
                // ua, uô -> tone on second vowel (a, ô)
                else if (v1 == 'u')
                    correctPos = lastVowel;
                // ưa, ươ -> tone on second vowel (a, ơ)
                else if (v1 == 'ư')
                    correctPos = lastVowel;
                // oa, oe -> tone on first vowel (o)
                else if (v1 == 'o' && (v2 == 'a' || v2 == 'e'))
                    correctPos = firstVowel;
                // uy -> tone on first vowel (u)
                else if (v1 == 'u' && v2 == 'y')
                    correctPos = firstVowel;
                // Default: tone on second vowel
                else
                    correctPos = lastVowel;
            }
            else if (count == 3)
            {
                // 3-vowel sequences: tone on middle vowel
                correctPos = firstVowel + 1;
            }

            // Check if current tone position is wrong (auto-correction)
            int currentTonePos = -1;
            for (int i = firstVowel; i <= lastVowel; i++)
            {
                if (GetTone(_buf[i]) >= 0)
                {
                    currentTonePos = i;
                    break;
                }
            }

            // Check if we need to remove tone (same tone key pressed again)
            char target = _buf[correctPos];
            char baseChar = GetBase(target);
            char withTone = AddTone(baseChar, tone);

            // If same tone already exists at correct position, remove it
            if (target == withTone)
            {
                // Remove tone - restore to base character
                _buf[correctPos] = baseChar;
                Backspaces = _buf.Length - correctPos;
                Output = BuildFrom(correctPos);
                // After removing tone, append the tone key as normal character
                Append(toneKey);
                Output += toneKey;
                return true;
            }

            // If tone was on wrong position, remove it and add to correct position
            if (currentTonePos >= 0 && currentTonePos != correctPos)
            {
                // Remove tone from wrong position
                char wrongBase = GetBase(_buf[currentTonePos]);
                _buf[currentTonePos] = wrongBase;
                // Add tone to correct position
                _buf[correctPos] = withTone;
                // Calculate backspaces from first vowel
                Backspaces = _buf.Length - firstVowel;
                Output = BuildFrom(firstVowel);
            }
            else
            {
                // Normal case: apply/replace tone
                Backspaces = _buf.Length - correctPos;
                _buf[correctPos] = withTone;
                Output = BuildFrom(correctPos);
            }

            return true;
        }

        private void Append(char c)
        {
            if (_buf.Length >= 40)
                _buf.Remove(0, 20);
            _buf.Append(c);
        }

        public void ClearBuffer()
        {
            _buf.Clear();
            Backspaces = 0;
            Output = null;
        }

        private string BuildFrom(int start, char replace)
        {
            var sb = new StringBuilder();
            sb.Append(replace);
            for (int i = start + 1; i < _buf.Length; i++)
                sb.Append(_buf[i]);
            return sb.ToString();
        }

        private string BuildFrom(int start)
        {
            var sb = new StringBuilder();
            for (int i = start; i < _buf.Length; i++)
                sb.Append(_buf[i]);
            return sb.ToString();
        }

        private static bool IsVowel(char c)
        {
            return "aâăeêiioôơuưyAÂĂEÊIIOÔƠUƯY".IndexOf(c) >= 0;
        }

        private static bool IsSep(char c)
        {
            return " \t\r\n.,;:!?0123456789".IndexOf(c) >= 0;
        }

        private static char GetBase(char c)
        {
            string[] tones = {
                "áàảãạ", "ấầẩẫậ", "ắằẳẵặ",
                "éèẻẽẹ", "ếềểễệ",
                "íìỉĩị",
                "óòỏõọ", "ốồổỗộ", "ớờởỡợ",
                "úùủũụ", "ứừửữự",
                "ýỳỷỹỵ"
            };
            string bases = "aâăeêioôơuưy";
            
            for (int i = 0; i < tones.Length; i++)
            {
                if (tones[i].IndexOf(char.ToLowerInvariant(c)) >= 0)
                    return char.IsUpper(c) ? char.ToUpperInvariant(bases[i]) : bases[i];
            }
            return c;
        }

        private static int GetTone(char c)
        {
            string[] tones = {
                "áàảãạ", "ấầẩẫậ", "ắằẳẵặ",
                "éèẻẽẹ", "ếềểễệ",
                "íìỉĩị",
                "óòỏõọ", "ốồổỗộ", "ớờởỡợ",
                "úùủũụ", "ứừửữự",
                "ýỳỷỹỵ"
            };
            
            for (int i = 0; i < tones.Length; i++)
            {
                int idx = tones[i].IndexOf(char.ToLowerInvariant(c));
                if (idx >= 0) return idx;
            }
            return -1;
        }

        private static char AddTone(char c, int tone)
        {
            if (tone < 0 || tone > 4) return c;
            
            string[] map = {
                "aáàảãạ", "âấầẩẫậ", "ăắằẳẵặ",
                "eéèẻẽẹ", "êếềểễệ",
                "iíìỉĩị",
                "oóòỏõọ", "ôốồổỗộ", "ơớờởỡợ",
                "uúùủũụ", "ưứừửữự",
                "yýỳỷỹỵ"
            };
            string bases = "aâăeêioôơuưy";
            
            int idx = bases.IndexOf(char.ToLowerInvariant(c));
            if (idx < 0) return c;
            
            char result = map[idx][tone + 1];
            return char.IsUpper(c) ? char.ToUpperInvariant(result) : result;
        }
    }
}
