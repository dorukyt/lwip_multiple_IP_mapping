using System.Text.Json;

namespace TmsControlPanel;

/// <summary>
/// Uygulama açılışında hatırlanan küçük ayarlar (son COM port ve baud).
/// %AppData%\TmsControlPanel\settings.json içinde saklanır.
/// </summary>
public class AppSettings
{
    public string? Port { get; set; }
    public int Baud { get; set; } = 9600;

    private static string FilePath => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
        "TmsControlPanel", "settings.json");

    public static AppSettings Load()
    {
        try
        {
            if (File.Exists(FilePath))
                return JsonSerializer.Deserialize<AppSettings>(File.ReadAllText(FilePath))
                       ?? new AppSettings();
        }
        catch
        {
            // bozuk/okunamayan ayar dosyası: varsayılanlarla devam et
        }
        return new AppSettings();
    }

    public void Save()
    {
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(FilePath)!);
            File.WriteAllText(FilePath, JsonSerializer.Serialize(this));
        }
        catch
        {
            // ayar kaydedilemezse sessiz geç (uygulamayı engellemesin)
        }
    }
}
