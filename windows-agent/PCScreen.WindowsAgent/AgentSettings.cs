using System.Text.Json;

namespace PCScreen.WindowsAgent;

internal sealed class AgentSettings
{
    public string Port { get; set; } = "auto";
    public string Language { get; set; } = "vi";
    public int IntervalMs { get; set; } = 1000;
    public bool ScreenEnabled { get; set; } = true;
    public int Brightness { get; set; } = 70;
    public bool MinimizeToTray { get; set; } = true;
    public bool StartMinimized { get; set; } = false;

    public static string FilePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "PCScreen", "agent-settings.json");

    public static AgentSettings Load()
    {
        try
        {
            if (!File.Exists(FilePath)) return new AgentSettings();
            return JsonSerializer.Deserialize<AgentSettings>(File.ReadAllText(FilePath))
                   ?? new AgentSettings();
        }
        catch
        {
            return new AgentSettings();
        }
    }

    public void Save()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(this, new JsonSerializerOptions
            {
                WriteIndented = true
            }));
        }
        catch
        {
            // Cấu hình không quan trọng hơn luồng telemetry; bỏ qua lỗi ghi file.
        }
    }
}
