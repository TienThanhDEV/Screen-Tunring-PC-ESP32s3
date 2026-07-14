using System.Text.Json.Serialization;

namespace PCScreen.WindowsAgent;

internal sealed class TelemetrySnapshot
{
    [JsonPropertyName("type")] public string Type => "telemetry";
    [JsonPropertyName("cpuTemp")] public float? CpuTemperature { get; init; }
    [JsonPropertyName("cpuLoad")] public float? CpuLoad { get; init; }
    [JsonPropertyName("cpuClockMHz")] public float? CpuClockMHz { get; init; }
    [JsonPropertyName("cpuPowerWatts")] public float? CpuPowerWatts { get; init; }
    [JsonPropertyName("cpuFanRpm")] public float? CpuFanRpm { get; init; }
    [JsonPropertyName("gpuTemp")] public float? GpuTemperature { get; init; }
    [JsonPropertyName("gpuHotspot")] public float? GpuHotspot { get; init; }
    [JsonPropertyName("gpuLoad")] public float? GpuLoad { get; init; }
    [JsonPropertyName("gpuClockMHz")] public float? GpuClockMHz { get; init; }
    [JsonPropertyName("gpuPowerWatts")] public float? GpuPowerWatts { get; init; }
    [JsonPropertyName("gpuFanRpm")] public float? GpuFanRpm { get; init; }
    [JsonPropertyName("gpuMemoryLoad")] public float? GpuMemoryLoad { get; init; }
    [JsonPropertyName("ramLoad")] public float? MemoryLoad { get; init; }
    [JsonPropertyName("memoryUsedGb")] public float? MemoryUsedGb { get; init; }
    [JsonPropertyName("memoryTotalGb")] public float? MemoryTotalGb { get; init; }
    [JsonPropertyName("memoryAvailableGb")] public float? MemoryAvailableGb { get; init; }
    [JsonPropertyName("fps")] public float? Fps { get; init; }
    [JsonPropertyName("diskLoad")] public float? DiskLoad { get; init; }
    [JsonPropertyName("diskTemp")] public float? DiskTemperature { get; init; }
    [JsonPropertyName("boardTemp")] public float? MotherboardTemperature { get; init; }
    [JsonPropertyName("networkDownMbps")] public float? NetworkDownMbps { get; init; }
    [JsonPropertyName("networkUpMbps")] public float? NetworkUpMbps { get; init; }
    [JsonPropertyName("loadAverage")] public float? LoadAverage { get; init; }
    [JsonPropertyName("uptimeSeconds")] public uint UptimeSeconds { get; init; }
    [JsonPropertyName("processCount")] public ushort ProcessCount { get; init; }
    [JsonPropertyName("cpuCoreCount")] public byte CpuCoreCount { get; init; }
}
