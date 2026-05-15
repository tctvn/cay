using System;
using System.Collections.Generic;
using System.Windows.Forms;

namespace Cay
{
    public struct MyKey
    {
        public Keys key;
        public bool isUpper;
        public MyKey(Keys k, bool u) { key = k; isUpper = u; }
    }

    public static class CayData
    {
        public static readonly HashSet<string> ValidInitials = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "", "b", "c", "ch", "d", "\u0111", "g", "gh", "gi", "h", "k", "kh", "l", "m", "n",
            "ng", "ngh", "nh", "ph", "qu", "r", "s", "t", "th", "tr", "v", "x"
        };

        public static readonly HashSet<string> ValidNuclei = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "a", "\u0103", "\u00e2", "e", "\u00ea", "i", "o", "\u00f4", "\u01a1", "u", "\u01b0", "y",
            "ai", "ao", "au", "ay", "\u00e2u", "\u00e2y", "eo", "\u00eau", "ia", "i\u00ea", "i\u00eau", "iu",
            "oa", "oai", "oay", "o\u0103", "oe", "oi", "\u00f4i", "\u01a1i",
            "ua", "u\u00e2", "u\u00ea", "ui", "u\u00f4", "u\u00f4i", "uy", "uya", "uy\u00ea",
            "\u01b0a", "\u01b0i", "\u01b0u", "\u01b0\u01a1", "\u01b0\u01a1i", "\u01b0\u01a1u", "ya", "y\u00ea", "y\u00eau"
        };

        public static readonly HashSet<string> VowelsOffset1 = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "i\u00eau", "y\u00eau", "\u01b0\u01a1u", "u\u00f4i"
        };

        public static readonly HashSet<string> VowelsOffset2 = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
        {
            "uy\u00ea"
        };

        public static readonly string[] ToneRows =
        {
            "a\u00e0\u00e1\u1ea3\u00e3\u1ea1",
            "\u0103\u1eb1\u1eaf\u1eb3\u1eb5\u1eb7",
            "\u00e2\u1ea7\u1ea5\u1ea9\u1eab\u1ead",
            "e\u00e8\u00e9\u1ebb\u1ebd\u1eb9",
            "\u00ea\u1ec1\u1ebf\u1ec3\u1ec5\u1ec7",
            "i\u00ec\u00ed\u1ec9\u0129\u1ecb",
            "o\u00f2\u00f3\u1ecf\u00f5\u1ecd",
            "\u00f4\u1ed3\u1ed1\u1ed5\u1ed7\u1ed9",
            "\u01a1\u1edd\u1edb\u1edf\u1ee1\u1ee3",
            "u\u00f9\u00fa\u1ee7\u0169\u1ee5",
            "\u01b0\u1eeb\u1ee9\u1eed\u1eef\u1ef1",
            "y\u1ef3\u00fd\u1ef7\u1ef9\u1ef5"
        };

        public static readonly Dictionary<char, string> ToneMap = new Dictionary<char, string>();
        public static readonly Dictionary<char, char> BaseCharMap = new Dictionary<char, char>();

        public static readonly Dictionary<string, char> TelexRules = new Dictionary<string, char>
        {
            {"aa", '\u00e2'}, {"aw", '\u0103'}, {"ee", '\u00ea'}, {"oo", '\u00f4'}, {"ow", '\u01a1'}, {"uw", '\u01b0'}, {"dd", '\u0111'},
            {"AA", '\u00c2'}, {"AW", '\u0102'}, {"EE", '\u00ca'}, {"OO", '\u00d4'}, {"OW", '\u01a0'}, {"UW", '\u01af'}, {"DD", '\u0110'},
            {"Aa", '\u00c2'}, {"Aw", '\u0102'}, {"Ee", '\u00ca'}, {"Oo", '\u00d4'}, {"Ow", '\u01a0'}, {"Uw", '\u01af'}, {"Dd", '\u0110'},
            {"aA", '\u00e2'}, {"aW", '\u0103'}, {"eE", '\u00ea'}, {"oO", '\u00f4'}, {"oW", '\u01a1'}, {"uW", '\u01b0'}, {"dD", '\u0111'}
        };

        public static readonly Dictionary<char, char> HookRules = new Dictionary<char, char>
        {
            {'a', '\u0103'}, {'o', '\u01a1'}, {'u', '\u01b0'},
            {'A', '\u0102'}, {'O', '\u01a0'}, {'U', '\u01af'},
            {'\u00e2', '\u0103'}, {'\u00c2', '\u0102'}
        };

        public static readonly Dictionary<char, char> ToneMarks = new Dictionary<char, char>
        {
            {'s', '1'}, {'f', '2'}, {'r', '3'}, {'x', '4'}, {'j', '5'}, {'z', '0'},
            {'S', '1'}, {'F', '2'}, {'R', '3'}, {'X', '4'}, {'J', '5'}, {'Z', '0'}
        };

        public static readonly char[] Vowels = { 'a', 'e', 'o', 'u', 'i', 'y', '\u00e2', '\u0103', '\u00ea', '\u00f4', '\u01a1', '\u01b0' };
        public static readonly char[] VowelsUpper = { 'A', 'E', 'O', 'U', 'I', 'Y', '\u00c2', '\u0102', '\u00ca', '\u00d4', '\u01a0', '\u01af' };

        public static readonly string[] VowelGroups = { "oa", "oe", "uy", "uo", "ie", "uo", "ua" };

        static CayData()
        {
            for (int i = 0; i < ToneRows.Length; i++)
            {
                string row = ToneRows[i];
                char baseChar = row[0];
                ToneMap[baseChar] = row;
                ToneMap[char.ToUpper(baseChar)] = row;

                for (int j = 0; j < row.Length; j++)
                {
                    char toned = row[j];
                    BaseCharMap[toned] = baseChar;
                    BaseCharMap[char.ToUpper(toned)] = baseChar;
                }
            }
        }
    }
}
