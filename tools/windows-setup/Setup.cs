using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Win32;

[assembly: AssemblyTitle("Jedi Academy Remixed Setup")]
[assembly: AssemblyProduct("Jedi Academy Remixed")]

internal static class Setup
{
    internal static string GameData(string location)
    {
        if (String.IsNullOrWhiteSpace(location)) return null;
        try
        {
            foreach (string path in new[] { location, Path.Combine(location, "GameData") })
                if (Enumerable.Range(0, 4).All(i => File.Exists(Path.Combine(path, "base", "assets" + i + ".pk3"))))
                    return Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar);
        }
        catch (ArgumentException) { }
        catch (NotSupportedException) { }
        return null;
    }

    internal static string Detect()
    {
        var libraries = new List<string>();
        foreach (string root in new[] {
            Registry.GetValue(@"HKEY_CURRENT_USER\Software\Valve\Steam", "SteamPath", null) as string,
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Steam")
        })
        {
            if (String.IsNullOrEmpty(root)) continue;
            libraries.Add(root);
            string manifest = Path.Combine(root, "steamapps", "libraryfolders.vdf");
            try
            {
                if (File.Exists(manifest))
                    foreach (Match match in Regex.Matches(File.ReadAllText(manifest), @"""path""\s+""([^""]+)"""))
                        libraries.Add(match.Groups[1].Value.Replace(@"\\", @"\"));
            }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
        }
        return libraries.Select(path => GameData(Path.Combine(path, "steamapps", "common", "Jedi Academy")))
                        .FirstOrDefault(path => path != null);
    }

    internal static void Install(string game, Action<string> status)
    {
        string target = GameData(game);
        if (target == null)
            throw new InvalidOperationException("Select your Jedi Academy or GameData folder. Setup needs base/assets0.pk3 through assets3.pk3.");
        string temp = Path.Combine(Path.GetTempPath(), "JAR-setup-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(temp);
        string logs = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Jedi Academy Remixed", "Setup");
        Directory.CreateDirectory(logs);
        string log = Path.Combine(logs, DateTime.UtcNow.ToString("yyyyMMdd-HHmmss-fff") + ".log");
        try
        {
            status("Checking the setup package");
            using (Stream bundle = Assembly.GetExecutingAssembly().GetManifestResourceStream("bundle"))
            using (SHA256 sha = SHA256.Create())
            {
                string hash = BitConverter.ToString(sha.ComputeHash(bundle)).Replace("-", "").ToLowerInvariant();
                if (hash != Release.Hash) throw new InvalidDataException("The installer is damaged. Download a fresh copy.");
                bundle.Position = 0;
                using (var zip = new ZipArchive(bundle, ZipArchiveMode.Read))
                {
                    foreach (ZipArchiveEntry entry in zip.Entries)
                    {
                        string path = Path.GetFullPath(Path.Combine(temp, entry.FullName));
                        if (!path.StartsWith(temp + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase))
                            throw new InvalidDataException("Unexpected package path.");
                        Directory.CreateDirectory(Path.GetDirectoryName(path));
                        entry.ExtractToFile(path);
                    }
                }
            }
            using (var output = new StreamWriter(log))
            using (var process = new Process())
            {
                string failure = null;
                output.AutoFlush = true;
                process.StartInfo = new ProcessStartInfo(Path.Combine(temp, "python", "python.exe"),
                    "-X utf8 -B -u \"" + Path.Combine(temp, "install.py") + "\" \"" + target
                    + "\" --setup-exe \"" + Application.ExecutablePath + "\"");
                process.StartInfo.WorkingDirectory = temp;
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.RedirectStandardError = true;
                process.StartInfo.EnvironmentVariables["PYTHONIOENCODING"] = "utf-8";
                process.StartInfo.StandardOutputEncoding = System.Text.Encoding.UTF8;
                process.StartInfo.StandardErrorEncoding = System.Text.Encoding.UTF8;
                DataReceivedEventHandler receive = (sender, e) =>
                {
                    if (e.Data == null) return;
                    lock (output)
                    {
                        output.WriteLine(e.Data);
                        if (e.Data.StartsWith("ERROR ")) failure = e.Data.Substring(6);
                    }
                    if (e.Data.StartsWith("STATUS ")) status(e.Data.Substring(7));
                    else if (e.Data.StartsWith("UI artwork: ")) status("Preparing interface textures (" + e.Data.Substring(12) + ")");
                };
                process.OutputDataReceived += receive;
                process.ErrorDataReceived += receive;
                process.Start();
                process.BeginOutputReadLine();
                process.BeginErrorReadLine();
                process.WaitForExit();
                if (process.ExitCode != 0)
                    throw new IOException((failure ?? "Setup could not complete.") + "\n\nSetup log: " + log);
            }
        }
        finally
        {
            try { Directory.Delete(temp, true); }
            catch (IOException) { }
            catch (UnauthorizedAccessException) { }
        }
    }

    [STAThread]
    private static int Main(string[] args)
    {
        bool quiet = args.Length == 2 && args[0] == "--install-to";
        using (var mutex = new Mutex(false, @"Local\JediAcademyRemixed.WindowsSetup"))
        {
            try
            {
                try
                {
                    if (!mutex.WaitOne(0)) throw new IOException("Another JA:R setup is already running.");
                }
                catch (AbandonedMutexException) { }
                if (quiet) Install(args[1], message => Console.WriteLine(message));
                else
                {
                    if (args.Length != 0) throw new ArgumentException("Usage: Setup.exe [--install-to <GameData>]");
                    Application.EnableVisualStyles();
                    Application.SetCompatibleTextRenderingDefault(false);
                    Application.Run(new SetupWindow());
                }
                return 0;
            }
            catch (Exception error)
            {
                if (quiet) Console.Error.WriteLine(error.Message);
                else MessageBox.Show(error.Message, "Jedi Academy Remixed Setup", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return 1;
            }
        }
    }
}

internal sealed class SetupWindow : Form
{
    private readonly TextBox folder = new TextBox();
    private readonly Label info = new Label();
    private readonly Button browse = new Button();
    private readonly Button action = new Button();
    private readonly Button play = new Button();
    private readonly ProgressBar progress = new ProgressBar();
    private bool installing;
    private bool done;

    internal SetupWindow()
    {
        Text = "Jedi Academy Remixed Setup";
        Font = new Font("Segoe UI", 10);
        AutoScaleMode = AutoScaleMode.Dpi;
        ClientSize = new Size(550, 244);
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = false;
        StartPosition = FormStartPosition.CenterScreen;
        var title = new Label {Text = "Install Jedi Academy Remixed " + Release.Version,
                               Font = new Font(Font.FontFamily, 15), AutoSize = true, Location = new Point(18, 14)};
        var label = new Label {Text = "Jedi Academy game folder", AutoSize = true, Location = new Point(20, 58)};
        folder.SetBounds(20, 82, 411, 28);
        folder.Text = Setup.Detect() ?? "";
        browse.SetBounds(440, 80, 90, 30);
        browse.Text = "Browse…";
        browse.Click += delegate
        {
            using (var dialog = new FolderBrowserDialog {Description = "Select Jedi Academy or its GameData folder", ShowNewFolderButton = false})
                if (dialog.ShowDialog(this) == DialogResult.OK) folder.Text = dialog.SelectedPath;
        };
        info.SetBounds(20, 122, 510, 44);
        info.Text = "Close the game before installing. Setup prepares single-player and multiplayer from your game files.";
        progress.SetBounds(20, 171, 510, 8);
        progress.Visible = false;
        action.SetBounds(340, 193, 190, 34);
        action.Text = "Install";
        action.Click += InstallClicked;
        play.SetBounds(140, 193, 190, 34);
        play.Text = "Play multiplayer";
        play.Visible = false;
        play.Click += delegate
        {
            Process.Start(new ProcessStartInfo(Path.Combine(folder.Text, "jar_mp.x86_64.exe")) { WorkingDirectory = folder.Text });
            Close();
        };
        Controls.AddRange(new Control[] {title, label, folder, browse, info, progress, action, play});
        AcceptButton = action;
        FormClosing += (sender, e) => { if (installing && e.CloseReason == CloseReason.UserClosing) e.Cancel = true; };
    }

    private async void InstallClicked(object sender, EventArgs args)
    {
        if (done)
        {
            Process.Start(new ProcessStartInfo(folder.Text) {UseShellExecute = true});
            Close();
            return;
        }
        string game = Setup.GameData(folder.Text.Trim().Trim('"'));
        if (game == null)
        {
            info.Text = "Choose the installed Jedi Academy folder or its GameData folder.";
            folder.Focus();
            return;
        }
        folder.Text = game;
        installing = true;
        action.Enabled = browse.Enabled = folder.Enabled = false;
        progress.Style = ProgressBarStyle.Marquee;
        progress.Visible = true;
        try
        {
            await Task.Run(() => Setup.Install(game, message =>
                BeginInvoke(new Action(() => { info.Text = message; }))));
            done = true;
            Text = "Installation complete";
            info.Text = "Ready to play. For single-player, open the game folder and run jar_sp.x86_64.exe.";
            action.Text = "Open game folder";
            play.Visible = true;
        }
        catch (Exception error)
        {
            info.Text = "Installation did not complete.";
            MessageBox.Show(this, error.Message, Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            installing = false;
            action.Enabled = true;
            folder.Enabled = browse.Enabled = !done;
            progress.Visible = false;
        }
    }
}
