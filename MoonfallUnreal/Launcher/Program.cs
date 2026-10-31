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
        ClientSize = new Size(520, 325);
        MinimumSize = new Size(470, 300);
        BackColor = Color.FromArgb(17, 14, 30);
        ForeColor = Color.WhiteSmoke;
        Font = new Font("Segoe UI", 11);
        StartPosition = FormStartPosition.CenterScreen;
        FormBorderStyle = FormBorderStyle.FixedSingle;
        MaximizeBox = false;

        var title = new Label { Text = "MOONFALL", Font = new Font("Segoe UI", 34, FontStyle.Bold),
            ForeColor = Color.FromArgb(230, 139, 210), TextAlign = ContentAlignment.MiddleCenter,
            Bounds = new Rectangle(20, 24, 480, 74) };
        var subtitle = new Label { Text = "UNREAL ENGINE EDITION", TextAlign = ContentAlignment.MiddleCenter,
            ForeColor = Color.FromArgb(174, 173, 204), Bounds = new Rectangle(20, 94, 480, 30) };
        play = new Button { Text = "PLAY", Bounds = new Rectangle(155, 158, 210, 54),
            BackColor = Color.FromArgb(96, 42, 128), ForeColor = Color.White,
            FlatStyle = FlatStyle.Flat, TabIndex = 0 };
        play.FlatAppearance.BorderSize = 0;
        play.Click += (_, _) => LaunchGame();
        status = new Label { Bounds = new Rectangle(30, 228, 460, 45),
            TextAlign = ContentAlignment.MiddleCenter, ForeColor = Color.FromArgb(198, 196, 222) };
        var exit = new Button { Text = "Exit", Bounds = new Rectangle(209, 281, 102, 28),
            FlatStyle = FlatStyle.Flat, ForeColor = Color.Gainsboro, BackColor = BackColor, TabIndex = 1 };
        exit.Click += (_, _) => Close();
        Controls.AddRange([title, subtitle, play, status, exit]);
        AcceptButton = play;
        UpdateStatus();
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
