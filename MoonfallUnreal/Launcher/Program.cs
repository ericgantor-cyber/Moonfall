using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;

namespace MoonfallLauncher;
internal static class Program
{
    [STAThread]
    static void Main()
    {
        ApplicationConfiguration.Initialize();
        Application.Run(new LaunchWindow());
    }
}

internal sealed class LaunchWindow : Form
{
    private readonly Label status;
    private readonly Button play;
    private readonly string gamePath = Path.Combine(AppContext.BaseDirectory, "MoonfallUnreal.exe");
    internal LaunchWindow()
    {
        Text = "Moonfall | Launcher";
        ClientSize = new Size(620, 470);
        MinimumSize = new Size(620, 470);
        BackColor = Color.FromArgb(17, 14, 30);
        ForeColor = Color.WhiteSmoke;
        Font = new Font("Segoe UI", 11);
        StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;

        var title = new Label { Text = "MOONFALL", Font = new Font("Segoe UI", 34, FontStyle.Bold),
            ForeColor = Color.FromArgb(230, 139, 210), TextAlign = ContentAlignment.MiddleCenter,
            Bounds = new Rectangle(20, 260, 580, 74) };
        var subtitle = new Label { Text = "UNREAL ENGINE EDITION", TextAlign = ContentAlignment.MiddleCenter,
            ForeColor = Color.FromArgb(174, 173, 204), Bounds = new Rectangle(20, 330, 580, 30) };
        play = new Button { Text = "PLAY", Bounds = new Rectangle(205, 368, 210, 54),
            BackColor = Color.FromArgb(96, 42, 128), ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat, TabIndex = 0 };
        play.FlatAppearance.BorderSize = 0;
        play.Click += (_, _) => LaunchGame();
        status = new Label { Bounds = new Rectangle(65, 423, 490, 25),
            TextAlign = ContentAlignment.MiddleCenter, ForeColor = Color.FromArgb(198, 196, 222) };
        var exit = new Button { Text = "Exit", Bounds = new Rectangle(520, 438, 88, 27),
            FlatStyle = FlatStyle.Flat, ForeColor = Color.Gainsboro, BackColor = BackColor, TabIndex = 1 };
        exit.Click += (_, _) => Close();
        Controls.AddRange([title, subtitle, play, status, exit]);
        AcceptButton = play;
        DoubleBuffered = true;
        UpdateStatus();
    }
    protected override void OnPaintBackground(PaintEventArgs e)
    {
        base.OnPaintBackground(e);
        var g = e.Graphics;
        g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
        using var sky = new System.Drawing.Drawing2D.LinearGradientBrush(
            new Rectangle(0, 0, ClientSize.Width, 265),
            Color.FromArgb(13, 12, 34), Color.FromArgb(60, 23, 70), 90f);
        g.FillRectangle(sky, new Rectangle(0, 0, ClientSize.Width, 265));
        var stars = new Random(2207);
        using var star = new SolidBrush(Color.FromArgb(170, 207, 181, 241));
        for (var i = 0; i < 80; ++i)
        {
            float x = stars.Next(ClientSize.Width), y = stars.Next(240);
            float size = stars.Next(1, 4);
            g.FillEllipse(star, x, y, size, size);
        }
        using var glow = new SolidBrush(Color.FromArgb(42, 239, 80, 183));
        using var outerGlow = new SolidBrush(Color.FromArgb(24, 250, 73, 193));
        using var moon = new SolidBrush(Color.FromArgb(242, 101, 162));
        using var moonCore = new SolidBrush(Color.FromArgb(242, 154, 195));
        g.FillEllipse(outerGlow, 188, 0, 244, 244);
        g.FillEllipse(glow, 212, 24, 196, 196);
        g.FillEllipse(moon, 237, 44, 146, 146);
        g.FillEllipse(moonCore, 258, 63, 95, 95);
        using var crater = new SolidBrush(Color.FromArgb(38, 89, 35, 95));
        g.FillEllipse(crater, 269, 93, 29, 21);
        g.FillEllipse(crater, 316, 120, 17, 13);
        g.FillEllipse(crater, 302, 69, 22, 17);
        var mountains = new[] {
            new Point(0, 230), new Point(75, 155), new Point(127, 216),
            new Point(200, 165), new Point(280, 231), new Point(365, 174),
            new Point(445, 224), new Point(541, 152), new Point(620, 228),
            new Point(620, 270), new Point(0, 270)
        };
        using var ridge = new SolidBrush(Color.FromArgb(24, 18, 44));
        g.FillPolygon(ridge, mountains);
        using var horizon = new Pen(Color.FromArgb(135, 190, 67, 156), 2);
        g.DrawLine(horizon, 0, 263, ClientSize.Width, 263);
    }
    private void UpdateStatus()
    {
        bool ready = File.Exists(gamePath) &&
                     Directory.Exists(Path.Combine(AppContext.BaseDirectory, "Engine")) &&
                     Directory.Exists(Path.Combine(AppContext.BaseDirectory, "MoonfallUnreal"));
        play.Enabled = ready;
        status.Text = ready ? "Game files found. Ready to launch." :
            "Game files missing. Reinstall the complete Moonfall package.";
    }
    private void LaunchGame()
    {
        UpdateStatus();
        if (!play.Enabled) return;
        try
        {
            Process.Start(new ProcessStartInfo(gamePath) {
                WorkingDirectory = AppContext.BaseDirectory,
                UseShellExecute = true
            });
            Close();
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, "Could not start Moonfall:\n" + ex.Message,
                "Launch failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
