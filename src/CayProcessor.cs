using System;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;

namespace Cay
{
    public partial class TelexEngine
    {
        private static readonly Regex SyllableRegex = new Regex(
            @"^(?<initial>ngh|ng|nh|ch|gh|gi|kh|ph|qu|th|tr|[bcdđghklmnpqrstvx])?(?<nucleus>[aeiouyăâêôơư]+)(?<final>ng|nh|ch|c|m|n|p|t)?(?<tail>[iouy])?$",
            RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

        private static readonly Regex SafeSyllableRegex = new Regex(
            @"^(?<initial>ngh|ng|nh|ch|gh|gi|kh|ph|qu|th|tr|[bcd\u0111ghklmnpqrstvx])?(?<nucleus>[aeiouy\u0103\u00e2\u00ea\u00f4\u01a1\u01b0]+)(?<final>ng|nh|ch|c|m|n|p|t)?(?<tail>[iouy])?$",
            RegexOptions.Compiled | RegexOptions.IgnoreCase | RegexOptions.CultureInvariant);

        private void Run(ref KeyEventArgs e, bool isBack)
        {
            if (_buffer.Count == 0)
            {
                _lastOutput = "";
                return;
            }

            StringBuilder sb = new StringBuilder();
            foreach (var mk in _buffer)
            {
                char c = (char)mk.key;
                if (!mk.isUpper) c = char.ToLower(c);
                sb.Append(c);
            }

            string raw = sb.ToString();
            string transformed = TransformTelex(raw);
            if (ShouldBypassWord(raw, transformed))
            {
                BypassCurrentWord(raw);
                return;
            }

            UpdateScreen(transformed);
        }

        private string TransformTelex(string input)
        {
            if (string.IsNullOrEmpty(input)) return "";

            string result = input;
            result = ApplyDoubleKeys(result);
            result = ApplyHookKeys(result);
            result = ApplyToneMarks(result);
            return result;
        }

        private string ApplyDoubleKeys(string s)
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < s.Length; i++)
            {
                if (i < s.Length - 2 && IsDoubleKeyUndoSequence(s, i))
                {
                    sb.Append(GetDoubleKeyBase(s[i]));
                    i += 2;
                    continue;
                }

                if (i < s.Length - 1)
                {
                    string pair = s.Substring(i, 2);
                    if (IsChar(s[i + 1], 'w'))
                    {
                        sb.Append(s[i]);
                        continue;
                    }

                    if (CayData.TelexRules.TryGetValue(pair, out char transformed))
                    {
                        sb.Append(transformed);
                        i++;
                        continue;
                    }
                }

                sb.Append(s[i]);
            }

            return sb.ToString();
        }

        private bool IsDoubleKeyUndoSequence(string s, int index)
        {
            char first = char.ToLower(s[index]);
            char second = char.ToLower(s[index + 1]);
            char third = char.ToLower(s[index + 2]);

            return first == second && second == third && (first == 'a' || first == 'e' || first == 'o' || first == 'd');
        }

        private char GetDoubleKeyBase(char c)
        {
            char lower = char.ToLower(c);
            char result = lower == 'd' ? 'd' : lower;
            return char.IsUpper(c) ? char.ToUpper(result) : result;
        }

        private string ApplyHookKeys(string s)
        {
            if (s.IndexOf('w') < 0 && s.IndexOf('W') < 0) return s;

            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < s.Length; i++)
            {
                if (!IsChar(s[i], 'w'))
                {
                    sb.Append(s[i]);
                    continue;
                }

                if (i < s.Length - 1 && IsChar(s[i + 1], 'w') && IsHookKeyTarget(sb))
                {
                    sb.Append(s[i]);
                    i++;
                }
                else if (sb.Length >= 2 && IsChar(sb[sb.Length - 2], 'u') && IsChar(sb[sb.Length - 1], 'o'))
                {
                    bool firstUpper = char.IsUpper(sb[sb.Length - 2]);
                    bool secondUpper = char.IsUpper(sb[sb.Length - 1]);
                    sb.Remove(sb.Length - 2, 2);
                    sb.Append(firstUpper ? '\u01af' : '\u01b0');
                    sb.Append(secondUpper ? '\u01a0' : '\u01a1');
                }
                else if (sb.Length > 0 && CayData.HookRules.TryGetValue(sb[sb.Length - 1], out char hooked))
                {
                    sb.Remove(sb.Length - 1, 1);
                    sb.Append(hooked);
                }
                else
                {
                    sb.Append(char.IsUpper(s[i]) ? '\u01af' : '\u01b0');
                }
            }

            return sb.ToString();
        }

        private bool IsHookKeyTarget(StringBuilder sb)
        {
            if (sb.Length == 0) return false;
            if (sb.Length >= 2 && IsChar(sb[sb.Length - 2], 'u') && IsChar(sb[sb.Length - 1], 'o')) return true;

            char previous = char.ToLower(sb[sb.Length - 1]);
            return previous == 'a' || previous == 'o' || previous == 'u' || previous == '\u00e2';
        }

        private string ApplyToneMarks(string s)
        {
            if (string.IsNullOrEmpty(s)) return s;

            char last = char.ToLower(s[s.Length - 1]);
            if (!IsToneKey(last)) return s;

            string word = s.Substring(0, s.Length - 1);
            if (string.IsNullOrEmpty(word)) return s;

            char previous = char.ToLower(word[word.Length - 1]);
            if (previous == last)
            {
                return word.Substring(0, word.Length - 1);
            }

            if (!HasAnyVowel(word)) return s;
            if (last == 'z') return word;

            return AddTone(word, last);
        }

        private string AddTone(string word, char tone)
        {
            int toneType = " fsrxj".IndexOf(tone);
            if (toneType <= 0) return word;

            int toneIndex = FindToneIndex(word);
            if (toneIndex < 0) return word + tone;

            char vWithTone = ApplyToneToChar(word[toneIndex], toneType);
            return word.Substring(0, toneIndex) + vWithTone + word.Substring(toneIndex + 1);
        }

        private int FindToneIndex(string word)
        {
            SyllableParts parts;
            if (!TryParseSyllable(word, out parts))
            {
                return FindLastVowel(word);
            }

            int targetVowel = SelectToneVowelOffset(parts.Nucleus, parts.HasConsonantFinal);
            return FindNthVowel(word, parts.NucleusStart, targetVowel);
        }

        private int SelectToneVowelOffset(string nucleus, bool hasConsonantFinal)
        {
            string n = NormalizeForSyllable(nucleus);
            int vowelCount = CountVowels(n);
            if (vowelCount <= 1) return 0;

            if (CayData.VowelsOffset1.Contains(n)) return 1;
            if (CayData.VowelsOffset2.Contains(n)) return 2;

            return hasConsonantFinal ? 1 : 0;
        }

        private bool TryParseSyllable(string word, out SyllableParts parts)
        {
            parts = new SyllableParts();
            string normalized = NormalizeForSyllable(word);
            Match match = SafeSyllableRegex.Match(normalized);
            if (!match.Success) return false;

            int nucleusStart = match.Groups["nucleus"].Index;
            string nucleus = match.Groups["nucleus"].Value + match.Groups["tail"].Value;
            if (string.IsNullOrEmpty(nucleus)) return false;

            parts.Nucleus = nucleus;
            parts.NucleusStart = nucleusStart;
            parts.HasConsonantFinal = match.Groups["final"].Success && match.Groups["final"].Value.Length > 0;
            return true;
        }

        private int CountVowels(string s)
        {
            int count = 0;
            for (int i = 0; i < s.Length; i++)
            {
                if (IsVowel(s[i])) count++;
            }

            return count;
        }

        private int FindNthVowel(string word, int start, int vowelOrdinal)
        {
            int seen = 0;
            for (int i = start; i < word.Length; i++)
            {
                if (!IsVowel(word[i])) continue;
                if (seen == vowelOrdinal) return i;
                seen++;
            }

            return FindLastVowel(word);
        }

        private int FindLastVowel(string word)
        {
            for (int i = word.Length - 1; i >= 0; i--)
            {
                if (IsVowel(word[i])) return i;
            }

            return -1;
        }

        private bool ShouldBypassWord(string raw, string transformed)
        {
            if (raw.Length < 4) return false;
            if (!HasAnyVowel(raw)) return false;
            if (HasVietnameseMark(transformed)) return false;
            if (IsPotentialVietnamesePrefix(transformed)) return false;

            SyllableParts parts;
            return !TryParseSyllable(transformed, out parts);
        }

        private bool HasVietnameseMark(string word)
        {
            for (int i = 0; i < word.Length; i++)
            {
                char lower = char.ToLower(word[i]);
                if (RemoveTone(word[i]) != lower) return true;
                if (IsVietnameseBaseVowel(word[i]) && !IsPlainAsciiVowel(lower)) return true;
            }

            return false;
        }

        private bool IsVietnameseBaseVowel(char c)
        {
            return CayData.ToneMap.ContainsKey(c);
        }

        private bool IsPotentialVietnamesePrefix(string word)
        {
            string normalized = NormalizeForSyllable(word);
            int firstVowel = FindFirstVowel(normalized);
            if (firstVowel < 0) return true;

            string initial = normalized.Substring(0, firstVowel);
            if (!IsValidInitialPrefix(initial)) return false;

            string rest = normalized.Substring(firstVowel);
            foreach (string nucleus in CayData.ValidNuclei)
            {
                if (nucleus.StartsWith(rest, StringComparison.OrdinalIgnoreCase)) return true;
            }

            return false;
        }

        private int FindFirstVowel(string word)
        {
            if (word.StartsWith("gi") && word.Length > 2 && IsVowel(word[2])) return 2;
            if (word.StartsWith("qu") && word.Length > 2 && IsVowel(word[2])) return 2;

            for (int i = 0; i < word.Length; i++)
            {
                if (IsVowel(word[i])) return i;
            }

            return -1;
        }

        private bool IsValidInitialPrefix(string initial)
        {
            return CayData.ValidInitials.Contains(initial);
        }

        private string NormalizeForSyllable(string word)
        {
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < word.Length; i++)
            {
                sb.Append(RemoveTone(char.ToLower(word[i])));
            }

            return sb.ToString();
        }

        private bool HasAnyVowel(string word)
        {
            for (int i = 0; i < word.Length; i++)
            {
                if (IsVowel(word[i])) return true;
            }

            return false;
        }

        private bool IsPlainAsciiVowel(char c)
        {
            char lower = char.ToLower(c);
            return lower == 'a' || lower == 'e' || lower == 'i' || lower == 'o' || lower == 'u' || lower == 'y';
        }

        private bool IsToneKey(char c) => CayData.ToneMarks.ContainsKey(c);

        private bool IsChar(char value, char expected) => char.ToLower(value) == expected;

        private bool IsVowel(char c) => GetToneRow(c) != null;

        private char RemoveTone(char v)
        {
            char lower = char.ToLower(v);
            return CayData.BaseCharMap.TryGetValue(lower, out char baseChar) ? baseChar : lower;
        }

        private char ApplyToneToChar(char v, int toneType)
        {
            string row = GetToneRow(v);
            if (row == null) return v;

            char result = row[toneType];
            return char.IsUpper(v) ? char.ToUpper(result) : result;
        }

        private string GetToneRow(char v)
        {
            return CayData.ToneMap.TryGetValue(v, out string row) ? row : null;
        }

        private struct SyllableParts
        {
            public string Nucleus;
            public int NucleusStart;
            public bool HasConsonantFinal;
        }
    }
}
