namespace PCScreen.WindowsAgent;

internal static class LanguageText
{
    public static bool English { get; set; }
    public static string T(string vietnamese, string english) =>
        English ? english : vietnamese;
}
