namespace PCScreen.WindowsAgent;

internal static class AppLog
{
    private const long MaximumBytes = 1_000_000;
    private static readonly object Sync = new();

    public static string DirectoryPath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "PCScreen");

    public static string FilePath => Path.Combine(DirectoryPath, "agent.log");

    public static void Write(string message)
    {
        try
        {
            lock (Sync)
            {
                Directory.CreateDirectory(DirectoryPath);
                RotateIfNeeded();
                File.AppendAllText(FilePath,
                    $"{DateTime.Now:yyyy-MM-dd HH:mm:ss.fff} | {message}{Environment.NewLine}");
            }
        }
        catch
        {
            // Logging must never interrupt sensor collection or serial output.
        }
    }

    private static void RotateIfNeeded()
    {
        if (!File.Exists(FilePath) || new FileInfo(FilePath).Length < MaximumBytes)
            return;
        var previous = Path.Combine(DirectoryPath, "agent.previous.log");
        File.Move(FilePath, previous, true);
    }
}
