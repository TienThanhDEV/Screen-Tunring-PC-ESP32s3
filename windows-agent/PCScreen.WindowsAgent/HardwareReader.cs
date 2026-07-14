using System.Diagnostics;
using System.Text;
using LibreHardwareMonitor.Hardware;
using Microsoft.Win32;

namespace PCScreen.WindowsAgent;

internal sealed class HardwareReader : IDisposable
{
    private readonly object _sync = new();
    private readonly Computer _computer = new()
    {
        IsCpuEnabled = true,
        IsGpuEnabled = true,
        IsMemoryEnabled = true,
        IsMotherboardEnabled = true,
        IsStorageEnabled = true,
        IsNetworkEnabled = true,
        IsControllerEnabled = true,
        IsPowerMonitorEnabled = true
    };

    public HardwareReader() => _computer.Open();

    public TelemetrySnapshot Read()
    {
        lock (_sync) return ReadCore();
    }

    private TelemetrySnapshot ReadCore()
    {
        foreach (var hardware in _computer.Hardware)
            UpdateRecursive(hardware);

        var cpu = Hardware(HardwareType.Cpu).ToArray();
        var gpu = Hardware(HardwareType.GpuAmd, HardwareType.GpuNvidia,
                           HardwareType.GpuIntel).ToArray();
        var memory = Hardware(HardwareType.Memory).ToArray();
        var storage = Hardware(HardwareType.Storage).ToArray();
        var network = Hardware(HardwareType.Network).ToArray();
        var board = Hardware(HardwareType.Motherboard,
                             HardwareType.SuperIO,
                             HardwareType.Cooler,
                             HardwareType.EmbeddedController).ToArray();
        var memoryUsed = First(memory, SensorType.Data, "Used");
        var memoryAvailable = First(memory, SensorType.Data, "Available");
        float? memoryTotal = memoryUsed.HasValue && memoryAvailable.HasValue
            ? memoryUsed + memoryAvailable : null;

        var cpuTemperature = First(cpu, SensorType.Temperature,
                                   "Package", "Tctl", "CPU")
                             ?? Maximum(cpu, SensorType.Temperature);
        var cpuLoad = First(cpu, SensorType.Load, "Total", "CPU Total")
                      ?? Maximum(cpu, SensorType.Load);
        var gpuTemperature = First(gpu, SensorType.Temperature,
                                   "GPU Core", "Core")
                             ?? Maximum(gpu, SensorType.Temperature);
        var gpuFan = Maximum(gpu, SensorType.Fan);
        var cpuFan = First(board, SensorType.Fan, "CPU")
                     ?? First(board, SensorType.Fan);
        var cpuClock = PositiveAverage(cpu, SensorType.Clock, "Core")
                       ?? PositiveAverage(cpu, SensorType.Clock)
                       ?? RegistryCpuClockMHz();
        var gpuLoad = First(gpu, SensorType.Load, "Core", "GPU Core")
                      ?? Maximum(gpu, SensorType.Load, "GPU")
                      ?? Maximum(gpu, SensorType.Load);
        var gpuClock = PositiveMaximum(gpu, SensorType.Clock,
                                       "Core", "GPU Core")
                       ?? PositiveMaximum(gpu, SensorType.Clock);
        var coreCount = (byte)Math.Min(byte.MaxValue,
            Environment.ProcessorCount);

        return new TelemetrySnapshot
        {
            CpuTemperature = cpuTemperature,
            CpuLoad = cpuLoad,
            CpuClockMHz = cpuClock,
            CpuPowerWatts = PositiveMaximum(cpu, SensorType.Power,
                                            "Package", "CPU")
                            ?? PositiveMaximum(cpu, SensorType.Power),
            CpuFanRpm = cpuFan,
            GpuTemperature = gpuTemperature,
            GpuHotspot = Maximum(gpu, SensorType.Temperature,
                                 "Hot Spot", "Hotspot", "Junction"),
            GpuLoad = gpuLoad,
            GpuClockMHz = gpuClock,
            GpuPowerWatts = PositiveMaximum(gpu, SensorType.Power,
                                            "Package", "GPU")
                            ?? PositiveMaximum(gpu, SensorType.Power),
            GpuFanRpm = gpuFan,
            GpuMemoryLoad = First(gpu, SensorType.Load, "Memory"),
            MemoryLoad = First(memory, SensorType.Load, "Memory"),
            MemoryUsedGb = memoryUsed,
            MemoryTotalGb = memoryTotal,
            MemoryAvailableGb = memoryAvailable,
            DiskLoad = Maximum(storage, SensorType.Load, "Used Space", "Total"),
            DiskTemperature = Maximum(storage, SensorType.Temperature),
            MotherboardTemperature = Maximum(board, SensorType.Temperature,
                                             "System", "Motherboard", "VRM"),
            NetworkDownMbps = ThroughputMbps(network, "Download", "Receive"),
            NetworkUpMbps = ThroughputMbps(network, "Upload", "Send"),
            LoadAverage = cpuLoad.HasValue
                ? cpuLoad.Value * coreCount / 100.0f : null,
            UptimeSeconds = (uint)Math.Min(uint.MaxValue,
                Environment.TickCount64 / 1000L),
            ProcessCount = CountProcesses(),
            CpuCoreCount = coreCount
        };
    }

    private IEnumerable<IHardware> Hardware(params HardwareType[] types) =>
        AllHardware().Where(x => types.Contains(x.HardwareType));

    private IEnumerable<IHardware> AllHardware()
    {
        foreach (var hardware in _computer.Hardware)
        {
            yield return hardware;
            foreach (var child in Descendants(hardware)) yield return child;
        }
    }

    private static IEnumerable<IHardware> Descendants(IHardware hardware)
    {
        foreach (var child in hardware.SubHardware)
        {
            yield return child;
            foreach (var nested in Descendants(child)) yield return nested;
        }
    }

    private static void UpdateRecursive(IHardware hardware)
    {
        hardware.Update();
        foreach (var child in hardware.SubHardware) UpdateRecursive(child);
    }

    private static IEnumerable<float> Values(IEnumerable<IHardware> hardware,
                                             SensorType type,
                                             params string[] names) =>
        hardware.SelectMany(x => x.Sensors)
            .Where(x => x.SensorType == type && x.Value.HasValue &&
                float.IsFinite(x.Value.Value) &&
                (names.Length == 0 || names.Any(name =>
                    x.Name.Contains(name, StringComparison.OrdinalIgnoreCase))))
            .Select(x => x.Value!.Value);

    private static float? First(IEnumerable<IHardware> hardware, SensorType type,
                                params string[] names)
    {
        var values = Values(hardware, type, names).Take(1).ToArray();
        return values.Length == 0 ? null : values[0];
    }

    private static float? Maximum(IEnumerable<IHardware> hardware, SensorType type,
                                  params string[] names)
    {
        var values = Values(hardware, type, names).ToArray();
        return values.Length == 0 ? null : values.Max();
    }

    private static float? Average(IEnumerable<IHardware> hardware, SensorType type,
                                  params string[] names)
    {
        var values = Values(hardware, type, names).ToArray();
        return values.Length == 0 ? null : values.Average();
    }

    private static float? PositiveMaximum(IEnumerable<IHardware> hardware,
                                           SensorType type,
                                           params string[] names)
    {
        var values = Values(hardware, type, names).Where(x => x > 0).ToArray();
        return values.Length == 0 ? null : values.Max();
    }

    private static float? PositiveAverage(IEnumerable<IHardware> hardware,
                                           SensorType type,
                                           params string[] names)
    {
        var values = Values(hardware, type, names).Where(x => x > 0).ToArray();
        return values.Length == 0 ? null : values.Average();
    }

    private static float? RegistryCpuClockMHz()
    {
        try
        {
            using var key = Registry.LocalMachine.OpenSubKey(
                @"HARDWARE\DESCRIPTION\System\CentralProcessor\0");
            var value = key?.GetValue("~MHz");
            return value is null ? null : Convert.ToSingle(value);
        }
        catch
        {
            return null;
        }
    }

    private static float? ThroughputMbps(IEnumerable<IHardware> hardware,
                                         params string[] names)
    {
        var bytesPerSecond = Values(hardware, SensorType.Throughput, names).ToArray();
        return bytesPerSecond.Length == 0 ? null
            : bytesPerSecond.Sum() * 8.0f / 1_000_000.0f;
    }

    private static ushort CountProcesses()
    {
        var processes = Process.GetProcesses();
        try
        {
            return (ushort)Math.Min(ushort.MaxValue, processes.Length);
        }
        finally
        {
            foreach (var process in processes) process.Dispose();
        }
    }

    public string CreateSensorReport()
    {
        lock (_sync)
        {
            foreach (var hardware in _computer.Hardware)
                UpdateRecursive(hardware);

            var builder = new StringBuilder();
            builder.AppendLine("PCScreen Windows Agent - Sensor Report");
            builder.AppendLine($"Created: {DateTime.Now:O}");
            builder.AppendLine($"OS: {Environment.OSVersion}");
            builder.AppendLine($"64-bit process: {Environment.Is64BitProcess}");
            builder.AppendLine($"Hardware nodes: {AllHardware().Count()}");
            builder.AppendLine();

            foreach (var hardware in AllHardware())
            {
                builder.AppendLine($"[{hardware.HardwareType}] {hardware.Name}");
                builder.AppendLine($"  Identifier: {hardware.Identifier}");
                if (hardware.Sensors.Length == 0)
                {
                    builder.AppendLine("  (no sensors exposed)");
                }
                foreach (var sensor in hardware.Sensors)
                {
                    var value = sensor.Value.HasValue
                        ? sensor.Value.Value.ToString("0.###") : "null";
                    builder.AppendLine(
                        $"  {sensor.SensorType,-12} | {sensor.Name} | {value}");
                }
                builder.AppendLine();
            }

            return builder.ToString();
        }
    }

    public void Dispose()
    {
        lock (_sync) _computer.Close();
    }
}
