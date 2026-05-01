using System;
using System.Drawing;
using System.Windows.Forms;

namespace Cay
{
    public class AboutForm : Form
    {
        public AboutForm()
        {
            Text = "Thông tin";
            Size = new Size(380, 280);
            FormBorderStyle = FormBorderStyle.FixedDialog;
            MaximizeBox = false;
            MinimizeBox = false;
            StartPosition = FormStartPosition.CenterScreen;
            BackColor = Color.White;

            var lblTitle = new Label
            {
                Text = "CAY",
                Font = new Font("Segoe UI", 28, FontStyle.Bold),
                ForeColor = Color.FromArgb(220, 53, 69),
                Location = new Point(20, 15),
                Size = new Size(340, 50)
            };

            var lblSub = new Label
            {
                Text = "Bộ gõ tiếng Việt đơn giản",
                Font = new Font("Segoe UI", 10),
                ForeColor = Color.Gray,
                Location = new Point(20, 65),
                Size = new Size(340, 20)
            };

            var pnl = new Panel
            {
                Location = new Point(20, 95),
                Size = new Size(320, 100),
                BackColor = Color.FromArgb(248, 249, 250),
                BorderStyle = BorderStyle.None
            };

            var lblInfo = new Label
            {
                Text = "Tham khảo: OpenKey\n" +
                       "Kiểu gõ: Telex\n" +
                       "Bảng mã: Unicode\n\n" +
                       "Phím tắt:\n" +
                       "• Ctrl + Shift: Bật/Tắt bộ gõ\n" +
                       "• Click chuột trái icon: Bật/Tắt",
                Font = new Font("Segoe UI", 9),
                Location = new Point(12, 10),
                Size = new Size(295, 85)
            };
            pnl.Controls.Add(lblInfo);

            var btn = new Button
            {
                Text = "Đóng",
                Location = new Point(255, 210),
                Size = new Size(85, 28),
                BackColor = Color.FromArgb(220, 53, 69),
                ForeColor = Color.White,
                FlatStyle = FlatStyle.Flat
            };
            btn.FlatAppearance.BorderSize = 0;
            btn.Click += (s, e) => Close();

            Controls.Add(lblTitle);
            Controls.Add(lblSub);
            Controls.Add(pnl);
            Controls.Add(btn);

            AcceptButton = btn;
        }
    }
}
