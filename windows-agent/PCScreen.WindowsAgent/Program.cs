namespace PCScreen.WindowsAgent;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        LanguageText.English = AgentSettings.Load().Language == "en";
        using var singleInstance = new Mutex(true,
            @"Local\PCScreen.WindowsAgent.SingleInstance", out var createdNew);
        if (!createdNew)
        {
            MessageBox.Show(LanguageText.T(
                "PCScreen Windows Agent đang chạy ở khay hệ thống.",
                "PCScreen Windows Agent is already running in the system tray."),
                "PCScreen Windows Agent", MessageBoxButtons.OK,
                MessageBoxIcon.Information);
            return;
        }

        ApplicationConfiguration.Initialize();
        var requestedHidden = args.Any(x =>
            x.Equals("--minimized", StringComparison.OrdinalIgnoreCase));
        Application.Run(new MainForm(requestedHidden));
        GC.KeepAlive(singleInstance);
    }
}
