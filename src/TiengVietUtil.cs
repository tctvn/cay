using System.Collections.Generic;
using System.Diagnostics;
using System.Windows.Forms;
using System.Linq;

namespace BoGoViet.TiengViet
{
    class TiengvietUtil
    {
        public Dictionary<string, string[]> cChar = new Dictionary<string, string[]>();
        public Dictionary<string, string> cCharInvert = new Dictionary<string, string>();
        public Dictionary<string, string> cVekepOU = new Dictionary<string, string>();
        public Dictionary<string, string> cVekepA = new Dictionary<string, string>();
        public Dictionary<string, string> cVekep = new Dictionary<string, string>();

        public Dictionary<string, string> cVekepInvertOU = new Dictionary<string, string>();
        public Dictionary<string, string> cVekepInvertA = new Dictionary<string, string>();
        public Dictionary<string, string> cVekepInvert = new Dictionary<string, string>();

        public Dictionary<string, int> cDauNguyenAm = new Dictionary<string, int>();
        public Dictionary<string, string> cDouble = new Dictionary<string, string>();
        public Dictionary<string, string> cDoubleA = new Dictionary<string, string>();
        public Dictionary<string, string> cDoubleE = new Dictionary<string, string>();
        public Dictionary<string, string> cDoubleO = new Dictionary<string, string>();

        public Dictionary<string, string> cDoubleInvert = new Dictionary<string, string>();
        public Dictionary<string, string> cDoubleAInvert = new Dictionary<string, string>();
        public Dictionary<string, string> cDoubleEInvert = new Dictionary<string, string>();
        public Dictionary<string, string> cDoubleOInvert = new Dictionary<string, string>();
        
        public TiengvietUtil()
        {
            SetUpChar();
            SetUpCharInvert();
            SetUpVekep();
            SetUpDauNguyenAm();
            SetUpDouble();
        }

        public void SetUpCharInvert()
        {
            cCharInvert["á"] = "a";
            cCharInvert["à"] = "a";
            cCharInvert["ã"] = "a";
            cCharInvert["ả"] = "a";
            cCharInvert["ạ"] = "a";

            cCharInvert["ắ"] = "ă";
            cCharInvert["ằ"] = "ă";
            cCharInvert["ẵ"] = "ă";
            cCharInvert["ẳ"] = "ă";
            cCharInvert["ặ"] = "ă";

            cCharInvert["ấ"] = "â";
            cCharInvert["ầ"] = "â";
            cCharInvert["ẫ"] = "â";
            cCharInvert["ẩ"] = "â";
            cCharInvert["ậ"] = "â";

            cCharInvert["ó"] = "o";
            cCharInvert["ò"] = "o";
            cCharInvert["õ"] = "o";
            cCharInvert["ỏ"] = "o";
            cCharInvert["ọ"] = "o";

            cCharInvert["ố"] = "ô";
            cCharInvert["ồ"] = "ô";
            cCharInvert["ỗ"] = "ô";
            cCharInvert["ổ"] = "ô";
            cCharInvert["ộ"] = "ô";

            cCharInvert["ớ"] = "ơ";
            cCharInvert["ờ"] = "ơ";
            cCharInvert["ỡ"] = "ơ";
            cCharInvert["ở"] = "ơ";
            cCharInvert["ợ"] = "ơ";

            cCharInvert["é"] = "e";
            cCharInvert["è"] = "e";
            cCharInvert["ẽ"] = "e";
            cCharInvert["ẻ"] = "e";
            cCharInvert["ẹ"] = "e";

            cCharInvert["ế"] = "ê";
            cCharInvert["ề"] = "ê";
            cCharInvert["ễ"] = "ê";
            cCharInvert["ể"] = "ê";
            cCharInvert["ệ"] = "ê";

            cCharInvert["í"] = "i";
            cCharInvert["ù"] = "i";
            cCharInvert["ĩ"] = "i";
            cCharInvert["ỉ"] = "i";
            cCharInvert["ị"] = "i";

            cCharInvert["ú"] = "u";
            cCharInvert["ù"] = "u";
            cCharInvert["ũ"] = "u";
            cCharInvert["ủ"] = "u";
            cCharInvert["ụ"] = "u";

            cCharInvert["ứ"] = "ư";
            cCharInvert["ừ"] = "ư";
            cCharInvert["ữ"] = "ư";
            cCharInvert["ử"] = "ư";
            cCharInvert["ự"] = "ư";

            cCharInvert["ý"] = "y";
            cCharInvert["ỳ"] = "y";
            cCharInvert["ỹ"] = "y";
            cCharInvert["ỷ"] = "y";
            cCharInvert["ỵ"] = "y";
        }

        public void SetUpChar()
        {
            cChar["a"] = new string[] { "a", "à", "á", "ã", "ả", "ạ" };
            cChar["ă"] = new string[] { "ă", "ằ", "ắ", "ẵ", "ẳ", "ặ" };
            cChar["â"] = new string[] { "â", "ầ", "ấ", "ẫ", "ẩ", "ậ" };
            cChar["o"] = new string[] { "o", "ò", "ó", "õ", "ỏ", "ọ" };
            cChar["ô"] = new string[] { "ô", "ồ", "ố", "ỗ", "ổ", "ộ" };
            cChar["ơ"] = new string[] { "ơ", "ờ", "ớ", "ỡ", "ở", "ợ" };
            cChar["e"] = new string[] { "e", "è", "é", "ẽ", "ẻ", "ẹ" };
            cChar["ê"] = new string[] { "ê", "ề", "ế", "ễ", "ể", "ệ" };
            cChar["i"] = new string[] { "i", "ì", "í", "ĩ", "ỉ", "ị" };
            cChar["u"] = new string[] { "u", "ù", "ú", "ũ", "ủ", "ụ" };
            cChar["ư"] = new string[] { "ư", "ừ", "ứ", "ữ", "ử", "ự" };
            cChar["y"] = new string[] { "y", "ỳ", "ý", "ỹ", "ỷ", "ỵ" };
        }

        // Kept for backward compatibility - does nothing
        public void SetKieuGo(string _kieugo)
        {
        }

        public void SetUpVekep()
        {
            cVekepA["a"] = "ă";
            cVekepA["â"] = "ă";
            cVekepA["oa"] = "oă";
            cVekepA["oâ"] = "oă";

            cVekepOU["o"] = "ơ";
            cVekepOU["ô"] = "ơ";
            cVekepOU["u"] = "ư";
            cVekepOU["oi"] = "ơi";
            cVekepOU["ôi"] = "ơi";
            cVekepOU["ua"] = "ưa";
            cVekepOU["ui"] = "ưi";
            cVekepOU["uo"] = "ươ";
            cVekepOU["uô"] = "ươ";
            cVekepOU["ưo"] = "ươ";
            cVekepOU["uơ"] = "ươ";
            cVekepOU["uu"] = "ưu";
            cVekepOU["uoi"] = "ươi";
            cVekepOU["uôi"] = "ươi";
            cVekepOU["uou"] = "ươu";
            cVekepOU["ưoi"] = "ươi";
            cVekepOU["ưou"] = "ươu";

            cVekepInvertA["ă"] = "a";
            cVekepInvertA["oă"] = "oa";

            cVekepInvertOU["ơ"] = "o";
            cVekepInvertOU["ư"] = "u";
            cVekepInvertOU["ơi"] = "oi";
            cVekepInvertOU["ưa"] = "ua";
            cVekepInvertOU["ưi"] = "ui";
            cVekepInvertOU["ươ"] = "uo";
            cVekepInvertOU["ưu"] = "uu";
            cVekepInvertOU["uu"] = "ưu";
            cVekepInvertOU["ươi"] = "uoi";
            cVekepInvertOU["ươu"] = "uou";

            foreach (var a in cVekepA)
            {
                cVekep[a.Key] = a.Value;
            }
            foreach (var ou in cVekepOU)
            {
                cVekep[ou.Key] = ou.Value;
            }

            foreach (var a in cVekepInvertA)
            {
                cVekepInvert[a.Key] = a.Value;
            }
            foreach (var ou in cVekepInvertOU)
            {
                cVekepInvert[ou.Key] = ou.Value;
            }
        }

        public void SetUpDauNguyenAm()
        {
            string[] nguyenAm1 = new string[] {
                "a", "ă", "â", "e", "ê", "i", "o", "ô", "ơ", "u", "ư", "y",
                "ai", "ao", "au", "âu", "ay", "ây",
                "eo" ,"êu", "ia", "iu", "oa", "oe","oi", "ôi", "ơi", "oo", "ua", "ưa", "ui",
                "ưi", "uo", "ưu", "uy" };
            for (var i = 0; i < nguyenAm1.Length; i++)
            {
                cDauNguyenAm[nguyenAm1[i]] = 0;
            }

            string[] nguyenAm2 = new string[] { "ie", "iê", "ye", "yê", "oă", "uă", "uâ", "ue", "uê", "uô",
                "uơ", "ươ", "ieu", "iêu", "yeu", "yêu",
                "oai", "oao", "oeo", "uao", "uay", "uây", "uoi", "uôi", "uoi", "ươi",
                "uou", "ươu", "uya", "uyu",
                // "iua", "iưa" // fix for gi
            };
            for (var i = 0; i < nguyenAm2.Length; i++)
            {
                cDauNguyenAm[nguyenAm2[i]] = 1;
            }
            cDauNguyenAm["uye"] = 2;
            cDauNguyenAm["uyê"] = 2;
        }

        public void SetUpDouble()
        {
            cDoubleA["a"] = "â";
            cDoubleA["ă"] = "â";
            cDoubleA["au"] = "âu";

            cDoubleA["ay"] = "ây";
            cDoubleA["ua"] = "uâ";
            cDoubleA["uay"] = "uây";

            cDoubleAInvert["â"] = "a";
            cDoubleAInvert["âu"] = "au";
            cDoubleAInvert["ây"] = "ay";
            cDoubleAInvert["uâ"] = "ua";
            cDoubleAInvert["uây"] = "uay";

            cDoubleE["e"] = "ê";
            cDoubleE["eu"] = "êu";
            cDoubleE["ie"] = "iê";
            cDoubleE["ye"] = "yê";
            cDoubleE["ue"] = "uê";
            cDoubleE["ieu"] = "iêu";
            cDoubleE["yeu"] = "yêu";
            cDoubleE["uye"] = "uyê";

            cDoubleEInvert["ê"] = "e";
            cDoubleEInvert["êu"] = "eu";
            cDoubleEInvert["iê"] = "ie";
            cDoubleEInvert["yê"] = "ye";
            cDoubleEInvert["uê"] = "ue";
            cDoubleEInvert["iêu"] = "ieu";
            cDoubleEInvert["yêu"] = "yeu";
            cDoubleEInvert["uyê"] = "uye";

            cDoubleO["o"] = "ô";
            cDoubleO["ơ"] = "ô";
            cDoubleO["oi"] = "ôi";
            cDoubleO["ơi"] = "ôi";
            cDoubleO["uo"] = "uô";
            cDoubleO["ươ"] = "uô";
            cDoubleO["uoi"] = "uôi";
            cDoubleO["ươi"] = "uôi";

            cDoubleOInvert["ô"] = "o";
            cDoubleOInvert["ôi"] = "oi";
            cDoubleOInvert["uô"] = "uo";
            cDoubleOInvert["uôi"] = "uoi";

            foreach (var a in cDoubleA)
            {
                cDouble[a.Key] = a.Value;
            }
            foreach (var e in cDoubleE)
            {
                cDouble[e.Key] = e.Value;
            }
            foreach (var o in cDoubleO)
            {
                cDouble[o.Key] = o.Value;
            }

            foreach (var a in cDoubleAInvert)
            {
                cDoubleInvert[a.Key] = a.Value;
            }
            foreach (var e in cDoubleEInvert)
            {
                cDoubleInvert[e.Key] = e.Value;
            }
            foreach (var o in cDoubleOInvert)
            {
                cDoubleInvert[o.Key] = o.Value;
            }
        }

        public int CheckDau(Keys key, bool shiftpress)
        {
            int returnKey = -1;
            // Telex only
            switch (key)
            {
                case Keys.Z:
                    returnKey = 0;
                    break;
                case Keys.F:
                    returnKey = 1;
                    break;
                case Keys.S:
                    returnKey = 2;
                    break;
                case Keys.X:
                    returnKey = 3;
                    break;
                case Keys.R:
                    returnKey = 4;
                    break;
                case Keys.J:
                    returnKey = 5;
                    break;
            }
            return returnKey;
        }

        public bool IsTelexDouble(Keys keycode)
        {
            // Telex: aa, ee, oo
            return keycode == Keys.E || keycode == Keys.A || keycode == Keys.O;
        }

        public bool IsVNIDouble(Keys keycode)
        {
            // Not used - kept for compatibility but always returns false
            return false;
        }

        
        public bool IsDGach(Keys keycode)
        {
            // Telex: dd for đ
            return keycode == Keys.D;
        }

        public bool IsTelexDauMoc(Keys keycode)
        {
            // Telex: aw (ă), ow (ơ), uw (ư)
            return keycode == Keys.W;
        }

        public bool IsVNIDauMocA(Keys keycode)
        {
            // Not used
            return false;
        }

        public bool IsVNIDauMocO(Keys keycode)
        {
            // Not used
            return false;
        }

        public bool IsDauMoc(Keys keycode)
        {
            return IsTelexDauMoc(keycode);
        }

        public bool CheckKeyEndWord(Keys key)
        {
            // Telex only - simpler end-word detection
            Keys[] listEndKey = new Keys[] {
                Keys.Enter, Keys.LControlKey
                , Keys.RControlKey, Keys.LWin, Keys.RWin, Keys.RMenu, Keys.LMenu
                , Keys.Space, Keys.Tab
                , Keys.Oem1 , Keys.Oem2
                , Keys.Oem3 , Keys.Oem4
                , Keys.Oem5 , Keys.Oem6
                , Keys.Oem8
                , Keys.Oemplus
                , Keys.OemPipe
                , Keys.OemBackslash, Keys.OemClear
                , Keys.OemCloseBrackets, Keys.OemOpenBrackets
                , Keys.OemQuotes
                , Keys.Left , Keys.Right
                , Keys.Down , Keys.Up
                , Keys.NumPad0 , Keys.NumPad1
                , Keys.NumPad2 , Keys.NumPad3
                , Keys.NumPad4 , Keys.NumPad5
                , Keys.NumPad6 , Keys.NumPad7
                , Keys.NumPad8 , Keys.NumPad9
                , Keys.F10 , Keys.F1
                , Keys.F2 , Keys.F3
                , Keys.F4 , Keys.F5
                , Keys.F6 , Keys.F7
                , Keys.F8 , Keys.F9
                , Keys.F11 , Keys.F12
                , Keys.D0 , Keys.D1
                , Keys.D2 , Keys.D3
                , Keys.D4 , Keys.D5
                , Keys.D6 , Keys.D7
                , Keys.D8 , Keys.D9
                , Keys.Escape , Keys.Delete
                , Keys.PageDown , Keys.Home
                , Keys.PageUp , Keys.End
                , Keys.Scroll , Keys.Pause
                , Keys.Insert , Keys.Next
                , Keys.CapsLock , Keys.Capital
                , Keys.Apps
                , Keys.OemMinus
                , Keys.Oemtilde , Keys.OemQuestion
                , Keys.Oem7 , Keys.OemPeriod
                , Keys.Oemcomma
            };
            return listEndKey.Contains(key);
        }
        public bool isNguyenAm(Keys k)
        {
            if (k == Keys.A || k == Keys.O || k == Keys.U ||
                k == Keys.E || k == Keys.I || k == Keys.Y)
            {
                return true;
            }
            return false;
        }
    }
}
