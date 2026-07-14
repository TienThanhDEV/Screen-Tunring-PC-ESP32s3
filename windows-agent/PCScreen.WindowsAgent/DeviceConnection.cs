using System.IO.Ports;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace PCScreen.WindowsAgent;

internal sealed class DeviceConnection : IDisposable
{
    private const int CommandAckTimeoutMilliseconds = 2200;
    private SerialPort? _port;
    private readonly object _ioLock = new();
    private readonly JsonSerializerOptions _json = new()
    {
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
    };

    public bool IsConnected => _port?.IsOpen == true;
    public string PortName => _port?.PortName ?? "--";

    public static string[] Ports() => SerialPort.GetPortNames()
        .OrderBy(x => x, StringComparer.OrdinalIgnoreCase).ToArray();

    public async Task<string> ConnectAsync(string? preferredPort,
                                           CancellationToken cancellationToken)
    {
        DisposePort();
        var candidates = string.IsNullOrWhiteSpace(preferredPort)
            ? Ports() : new[] { preferredPort };
        foreach (var name in candidates)
        {
            cancellationToken.ThrowIfCancellationRequested();
            try
            {
                var port = new SerialPort(name, 115200)
                {
                    NewLine = "\n",
                    ReadTimeout = 250,
                    WriteTimeout = 1500,
                    DtrEnable = false,
                    RtsEnable = false
                };
                port.Open();
                await Task.Delay(350, cancellationToken);
                port.DiscardInBuffer();
                var deadline = DateTime.UtcNow.AddSeconds(10);
                var nextHello = DateTime.MinValue;
                while (DateTime.UtcNow < deadline)
                {
                    cancellationToken.ThrowIfCancellationRequested();
                    if (DateTime.UtcNow >= nextHello)
                    {
                        port.WriteLine("{\"type\":\"hello\"}");
                        nextHello = DateTime.UtcNow.AddMilliseconds(700);
                    }
                    try
                    {
                        var line = port.ReadLine();
                        using var document = JsonDocument.Parse(line);
                        var root = document.RootElement;
                        if (root.TryGetProperty("device", out var device) &&
                            device.GetString() == "PCSCREEN-S3")
                        {
                            _port = port;
                            var firmware = root.TryGetProperty("firmware", out var fw)
                                ? fw.GetString() : "--";
                            return $"{LanguageText.T("Đã kết nối", "Connected")} {name} • firmware {firmware}";
                        }
                    }
                    catch (TimeoutException) { }
                }
                port.Dispose();
            }
            catch (OperationCanceledException)
            {
                throw;
            }
            catch (Exception) { }
        }
        throw new IOException(LanguageText.T(
            "Không tìm thấy PCScreen trên các cổng COM.",
            "PCScreen was not found on any COM port."));
    }

    public bool SendTelemetry(TelemetrySnapshot snapshot) =>
        SendLine(JsonSerializer.Serialize(snapshot, _json), "telemetry", 0,
                 false);

    public void SetBacklight(bool enabled) =>
        SendLine(JsonSerializer.Serialize(new
        {
            type = "display", command = "backlight", enabled
        }), "display", CommandAckTimeoutMilliseconds, true);

    public void SetBrightness(int value) =>
        SendLine(JsonSerializer.Serialize(new
        {
            type = "display", command = "brightness", value
        }), "display", CommandAckTimeoutMilliseconds, true);

    private bool SendLine(string line, string expectedType,
                          int responseWindowMilliseconds,
                          bool responseRequired)
    {
        lock (_ioLock)
        {
            if (_port?.IsOpen != true)
                throw new IOException(LanguageText.T(
                    "Thiết bị chưa kết nối.", "The device is not connected."));
            try
            {
                // Remove delayed telemetry ACKs before a new transaction. A
                // telemetry write is intentionally fire-and-forget; control
                // commands below still wait for a matching device ACK.
                _port.DiscardInBuffer();
                _port.WriteLine(line);
                if (!responseRequired) return true;
                var deadline = DateTime.UtcNow.AddMilliseconds(
                    responseWindowMilliseconds);
                while (DateTime.UtcNow < deadline)
                {
                    try
                    {
                        var response = _port.ReadLine();
                        using var document = JsonDocument.Parse(response);
                        var root = document.RootElement;
                        if (!root.TryGetProperty("type", out var type) ||
                            type.GetString() != expectedType)
                            continue;
                        if (root.TryGetProperty("ok", out var ok) && ok.GetBoolean())
                        {
                            return true;
                        }
                        var error = root.TryGetProperty("error", out var detail)
                            ? detail.GetString() : "device_rejected";
                        throw new IOException($"{LanguageText.T("ESP32 từ chối lệnh", "ESP32 rejected the command")}: {error}");
                    }
                    catch (TimeoutException) { }
                    catch (JsonException) { }
                }
                throw new TimeoutException(LanguageText.T(
                    "ESP32 chưa xác nhận lệnh điều khiển; cổng vẫn được giữ.",
                    "ESP32 did not confirm the control command; the port remains open."));
            }
            catch (TimeoutException)
            {
                throw;
            }
            catch
            {
                DisposePort();
                throw;
            }
        }
    }

    private void DisposePort()
    {
        try { if (_port?.IsOpen == true) _port.Close(); } catch { }
        _port?.Dispose();
        _port = null;
    }

    public void Disconnect() => DisposePort();

    public void Dispose() => DisposePort();
}
