using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;

namespace MoonfallLauncher;
internal static class Program
{
    [STAThread]
    static void Main(string[] args)
    {
        if (args.Length == 1 && args[0] == "--check-install")
        {
            Environment.ExitCode = LaunchWindow.HasGameFiles(AppContext.BaseDirectory) ? 0 : 2;
            return;
        }
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
        ClientSize = new Size(860, 600);
        MinimumSize = new Size(760, 550);
        BackColor = Color.FromArgb(17, 14, 30);
        ForeColor = Color.WhiteSmoke;
        Font = new Font("Segoe UI", 11);
        StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.Sizable;
        MaximizeBox = true;

        var title = new Label { Text = "MOONFALL", Font = new Font("Segoe UI", 34, FontStyle.Bold),
            ForeColor = Color.FromArgb(230, 139, 210), TextAlign = ContentAlignment.MiddleCenter,
            Bounds = new Rectangle(120, 335, 620, 74), Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right };
        var subtitle = new Label { Text = "UNREAL ENGINE EDITION", TextAlign = ContentAlignment.MiddleCenter,
            ForeColor = Color.FromArgb(174, 173, 204), Bounds = new Rectangle(120, 401, 620, 30), Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right };
        play = new Button { Text = "PLAY", Bounds = new Rectangle(315, 447, 230, 58), Anchor = AnchorStyles.Bottom,
            BackColor = Color.FromArgb(96, 42, 128), ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat, TabIndex = 0 };
        play.FlatAppearance.BorderSize = 0;
        play.Click += (_, _) => LaunchGame();
        status = new Label { Bounds = new Rectangle(125, 515, 610, 25), Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right,
            TextAlign = ContentAlignment.MiddleCenter, ForeColor = Color.FromArgb(198, 196, 222) };
        var exit = new Button { Text = "Exit", Bounds = new Rectangle(745, 562, 95, 28), Anchor = AnchorStyles.Bottom | AnchorStyles.Right,
            FlatStyle = FlatStyle.Flat, ForeColor = Color.Gainsboro, BackColor = BackColor, TabIndex = 1 };
        exit.Click += (_, _) => Close();
        var folder = new Button { Text = "GAME FOLDER", Bounds = new Rectangle(22, 562, 160, 28),
            Anchor = AnchorStyles.Bottom | AnchorStyles.Left, FlatStyle = FlatStyle.Flat,
            ForeColor = Color.WhiteSmoke, BackColor = Color.FromArgb(49, 35, 68), TabIndex = 2 };
        folder.Click += (_, _) => OpenFolder();
        var retry = new Button { Text = "CHECK FILES", Bounds = new Rectangle(190, 562, 160, 28),
            Anchor = AnchorStyles.Bottom | AnchorStyles.Left, FlatStyle = FlatStyle.Flat,
            ForeColor = Color.WhiteSmoke, BackColor = Color.FromArgb(49, 35, 68), TabIndex = 3 };
        retry.Click += (_, _) => UpdateStatus();
        var help = new Button { Text = "HELP", Bounds = new Rectangle(358, 562, 95, 28),
            Anchor = AnchorStyles.Bottom | AnchorStyles.Left, FlatStyle = FlatStyle.Flat,
            ForeColor = Color.WhiteSmoke, BackColor = Color.FromArgb(49, 35, 68), TabIndex = 4 };
        help.Click += (_, _) => MessageBox.Show(this,
            "Moonfall must be installed as a complete Unreal package.\n" +
            "Use CHECK FILES to refresh installation status.\n" +
            "Use GAME FOLDER to inspect the files.\n" +
            "If files are missing, reinstall the complete game.\n" +
            "Settings and gameplay controls are inside Moonfall.",
            "Moonfall Help", MessageBoxButtons.OK, MessageBoxIcon.Information);
        var logs = new Button { Text = "GAME LOGS", Bounds = new Rectangle(461, 562, 160, 28),
            Anchor = AnchorStyles.Bottom | AnchorStyles.Left, FlatStyle = FlatStyle.Flat,
            ForeColor = Color.WhiteSmoke, BackColor = Color.FromArgb(49, 35, 68), TabIndex = 5 };
        logs.Click += (_, _) => OpenLogs();
        Controls.AddRange([title, subtitle, play, status, exit, folder, retry, help, logs]);
        AcceptButton = play;
        DoubleBuffered = true;
        Resize += (_, _) => Invalidate();
        UpdateStatus();
    }
    protected override void OnShown(EventArgs e)
    {
        base.OnShown(e);
        play.Focus();
    }
    protected override void OnPaintBackground(PaintEventArgs e)
    {
        base.OnPaintBackground(e);
        var g = e.Graphics;
        g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;
        var width = ClientSize.Width;
        var artHeight = Math.Max(280, ClientSize.Height - 265);
        using var sky = new System.Drawing.Drawing2D.LinearGradientBrush(
            new Rectangle(0, 0, width, artHeight),
            Color.FromArgb(13, 12, 34), Color.FromArgb(60, 23, 70), 90f);
        g.FillRectangle(sky, new Rectangle(0, 0, width, artHeight));
        var stars = new Random(2207);
        using var star = new SolidBrush(Color.FromArgb(170, 207, 181, 241));
        for (var i = 0; i < 80; ++i)
        {
            float x = stars.Next(ClientSize.Width), y = stars.Next(artHeight);
            float size = stars.Next(1, 4);
            g.FillEllipse(star, x, y, size, size);
        }
        using var glow = new SolidBrush(Color.FromArgb(42, 239, 80, 183));
        using var outerGlow = new SolidBrush(Color.FromArgb(24, 250, 73, 193));
        using var moon = new SolidBrush(Color.FromArgb(242, 101, 162));
        using var moonCore = new SolidBrush(Color.FromArgb(242, 154, 195));
        var cx = width / 2;
        g.FillEllipse(outerGlow, cx-122, 18, 244, 244);
        g.FillEllipse(glow, cx-98, 42, 196, 196);
        g.FillEllipse(moon, cx-73, 62, 146, 146);
        g.FillEllipse(moonCore, cx-52, 81, 95, 95);
        using var crater = new SolidBrush(Color.FromArgb(38, 89, 35, 95));
        g.FillEllipse(crater, cx-41, 111, 29, 21);
        g.FillEllipse(crater, cx+6, 138, 17, 13);
        g.FillEllipse(crater, cx-8, 87, 22, 17);
        var mountains = new[] {
            new Point(0, artHeight-28), new Point(width/8, artHeight-106),
            new Point(width/5, artHeight-41), new Point(width/3, artHeight-113),
            new Point(width/2, artHeight-30), new Point(width*2/3, artHeight-118),
            new Point(width*4/5, artHeight-47), new Point(width*9/10, artHeight-102),
            new Point(width, artHeight-26), new Point(width, artHeight+5),
            new Point(0, artHeight+5)
        };
        using var ridge = new SolidBrush(Color.FromArgb(24, 18, 44));
        g.FillPolygon(ridge, mountains);
        using var horizon = new Pen(Color.FromArgb(135, 190, 67, 156), 2);
        g.DrawLine(horizon, 0, artHeight, width, artHeight);
    }
    internal static bool HasGameFiles(string directory) =>
        File.Exists(Path.Combine(directory, "MoonfallUnreal.exe")) &&
        Directory.Exists(Path.Combine(directory, "Engine")) &&
        Directory.Exists(Path.Combine(directory, "MoonfallUnreal")) &&
        Directory.EnumerateFiles(Path.Combine(directory, "MoonfallUnreal"), "*", SearchOption.AllDirectories).Any();

    private void UpdateStatus()
    {
        bool ready = HasGameFiles(AppContext.BaseDirectory);
        play.Enabled = ready;
        status.Text = ready ? "Game files found. Ready to launch." :
            !File.Exists(gamePath) ? "Game executable missing. Reinstall the complete package." :
            !Directory.Exists(Path.Combine(AppContext.BaseDirectory, "Engine")) ? "Unreal runtime folder missing. Reinstall Moonfall." :
            "Cooked game content missing. Reinstall the complete package.";
    }
    private void OpenLogs()
    {
        var logs = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "MoonfallUnreal", "Saved", "Logs");
        if (!Directory.Exists(logs))
        {
            MessageBox.Show(this, "No game logs have been created yet. Start the packaged game first.",
                "Game Logs", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }
        try { Process.Start(new ProcessStartInfo("explorer.exe", logs) { UseShellExecute = true }); }
        catch (Exception ex) { MessageBox.Show(this, ex.Message, "Could not open logs", MessageBoxButtons.OK, MessageBoxIcon.Warning); }
    }
    private void OpenFolder()
    {
        try { Process.Start(new ProcessStartInfo("explorer.exe", AppContext.BaseDirectory) { UseShellExecute = true }); }
        catch (Exception ex) { MessageBox.Show(this, ex.Message, "Could not open folder", MessageBoxButtons.OK, MessageBoxIcon.Warning); }
    }
    private void LaunchGame()
    {
        UpdateStatus();
        if (!play.Enabled) return;
        try
        {
            var process = Process.Start(new ProcessStartInfo(gamePath) {
                WorkingDirectory = AppContext.BaseDirectory,
                UseShellExecute = true
            });
            if (process == null) throw new InvalidOperationException("Windows did not start the game process.");
            Close();
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, "Could not start Moonfall:\n" + ex.Message,
                "Launch failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }
}
