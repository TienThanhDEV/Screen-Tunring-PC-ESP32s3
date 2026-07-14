using Microsoft.Win32;
using System.Diagnostics;
using System.Text.Json;

namespace PCScreen.WindowsAgent;

internal sealed class MainForm : Form
{
    private static string T(string vi, string en) => LanguageText.T(vi, en);
    private static readonly Color Background = Color.FromArgb(7, 8, 10);
    private static readonly Color PanelColor = Color.FromArgb(18, 19, 23);
    private static readonly Color Border = Color.FromArgb(47, 49, 56);
    private static readonly Color Muted = Color.FromArgb(155, 157, 166);
    private static readonly Color Accent = Color.FromArgb(237, 28, 36);
    private static readonly Color Good = Color.FromArgb(73, 223, 143);
    private static readonly Color Warning = Color.FromArgb(255, 190, 86);
    private static readonly Color Error = Color.FromArgb(255, 115, 120);

    private readonly AgentSettings _settings = AgentSettings.Load();
    private readonly HardwareReader _hardware = new();
    private readonly DeviceConnection _device = new();
    private readonly System.Windows.Forms.Timer _timer = new();
    private readonly System.Windows.Forms.Timer _settingsSaveTimer = new();
    private readonly System.Windows.Forms.Timer _brightnessSendTimer = new();
    private readonly CancellationTokenSource _shutdown = new();
    private readonly ComboBox _ports = new();
    private readonly ComboBox _language = new();
    private readonly NumericUpDown _interval = new();
    private readonly Button _run = new();
    private readonly Label _connection = new();
    private readonly Label _lastUpdate = new();
    private readonly Label _sensorHealth = new();
    private readonly CheckBox _screenEnabled = new();
    private readonly TrackBar _brightness = new();
    private readonly CheckBox _autoStart = new();
    private readonly CheckBox _minimizeToTray = new();
    private readonly CheckBox _startMinimized = new();
    private readonly FlowLayoutPanel _metricFlow = new();
    private readonly Dictionary<string, Label> _values = new();
    private readonly NotifyIcon _tray = new();
    private readonly ContextMenuStrip _trayMenu = new();
    private readonly ToolStripMenuItem _trayPause = new();
    private readonly bool _requestedHidden;
    private readonly DateTime _startedAt = DateTime.Now;

    private TelemetrySnapshot? _lastSnapshot;
    private DateTime _nextConnectAttempt = DateTime.MinValue;
    private bool _running = true;
    private bool _updating;
    private bool _allowExit;
    private bool _initializing = true;
    private bool _shownTrayHint;
    private long _sentPackets;
    private long _operationErrors;
    private string _lastLoggedStatus = string.Empty;

    public MainForm(bool requestedHidden)
    {
        LanguageText.English = _settings.Language == "en";
        _requestedHidden = requestedHidden;
        Text = "PCScreen Windows Agent • AMD MI50";
        ClientSize = new Size(1040, 720);
        MinimumSize = new Size(560, 520);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Background;
        ForeColor = Color.White;
        Font = new Font("Segoe UI", 9.5f);
        AutoScaleMode = AutoScaleMode.Dpi;
        FormBorderStyle = FormBorderStyle.Sizable;
        MaximizeBox = true;
        try
        {
            Icon = System.Drawing.Icon.ExtractAssociatedIcon(
                Application.ExecutablePath);
        }
        catch { }
        AppLog.Write("PCScreen Windows Agent v1.5.0 started");

        BuildInterface();
        BuildTray();
        ApplySettings();
        RefreshPorts();
        _autoStart.Checked = ReadAutoStart();
        _initializing = false;

        _timer.Interval = (int)_interval.Value;
        _timer.Tick += OnTick;
        _timer.Start();
        _settingsSaveTimer.Interval = 700;
        _settingsSaveTimer.Tick += (_, _) =>
        {
            _settingsSaveTimer.Stop();
            PersistSettings();
        };
        _brightnessSendTimer.Interval = 160;
        _brightnessSendTimer.Tick += (_, _) =>
        {
            _brightnessSendTimer.Stop();
            SendBrightness();
        };

        Shown += OnShown;
        Resize += OnWindowResize;
        FormClosing += OnClosing;
    }

    private void BuildInterface()
    {
        var root = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            Padding = new Padding(18),
            ColumnCount = 1,
            RowCount = 4,
            BackColor = Background,
            AutoScroll = true
        };
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        Controls.Add(root);

        var header = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            ColumnCount = 3,
            Margin = new Padding(0, 0, 0, 12)
        };
        header.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        header.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        header.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        var title = new Label
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            Text = "PCSCREEN  /  WINDOWS AGENT",
            Font = new Font("Segoe UI", 19, FontStyle.Bold),
            ForeColor = Color.White,
            AutoEllipsis = true
        };
        var subtitle = new Label
        {
            AutoSize = true,
            Anchor = AnchorStyles.Right,
            Text = "ALL-IN-ONE • v1.5.0",
            Font = new Font("Segoe UI", 8.5f, FontStyle.Bold),
            ForeColor = Accent,
            Padding = new Padding(8, 10, 0, 0)
        };
        header.Controls.Add(title, 0, 0);
        header.Controls.Add(subtitle, 1, 0);
        _language.DropDownStyle = ComboBoxStyle.DropDownList;
        _language.Items.AddRange(new object[] { "Tiếng Việt", "English" });
        _language.SelectedIndex = LanguageText.English ? 1 : 0;
        _language.Width = 112;
        _language.Margin = new Padding(12, 7, 0, 0);
        StyleInput(_language);
        _language.SelectedIndexChanged += (_, _) =>
        {
            if (_initializing) return;
            _settings.Language = _language.SelectedIndex == 1 ? "en" : "vi";
            _settings.Save();
            Application.Restart();
        };
        header.Controls.Add(_language, 2, 0);
        root.Controls.Add(header, 0, 0);

        var connectionPanel = Card();
        connectionPanel.Dock = DockStyle.Fill;
        connectionPanel.AutoSize = true;
        connectionPanel.MinimumSize = new Size(0, 114);
        connectionPanel.Padding = new Padding(12);
        var connectionFlow = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = false,
            Height = 88,
            WrapContents = true,
            BackColor = PanelColor
        };
        connectionPanel.Controls.Add(connectionFlow);
        connectionFlow.Controls.Add(Caption(T("Cổng thiết bị", "Device port")));
        _ports.Width = 145;
        _ports.DropDownStyle = ComboBoxStyle.DropDownList;
        _ports.SelectedIndexChanged += (_, _) => SaveSettings();
        StyleInput(_ports);
        connectionFlow.Controls.Add(_ports);

        var refresh = ActionButton(T("Quét cổng", "Scan ports"), false);
        refresh.Click += (_, _) => RefreshPorts();
        connectionFlow.Controls.Add(refresh);
        var reconnect = ActionButton(T("Kết nối lại", "Reconnect"), false);
        reconnect.Click += (_, _) => ForceReconnect();
        connectionFlow.Controls.Add(reconnect);

        connectionFlow.Controls.Add(Caption(T("Chu kỳ", "Interval")));
        _interval.Minimum = 250;
        _interval.Maximum = 5000;
        _interval.Increment = 250;
        _interval.Width = 86;
        _interval.ValueChanged += (_, _) =>
        {
            _timer.Interval = (int)_interval.Value;
            SaveSettings();
        };
        StyleInput(_interval);
        connectionFlow.Controls.Add(_interval);
        connectionFlow.Controls.Add(Caption("ms"));

        _run.Text = T("Tạm dừng", "Pause");
        _run.AutoSize = true;
        StyleButton(_run, true);
        _run.Click += (_, _) => ToggleRunning();
        connectionFlow.Controls.Add(_run);

        _connection.AutoSize = true;
        _connection.Padding = new Padding(9, 8, 9, 7);
        _connection.Text = T("Đang tìm PCScreen…", "Searching for PCScreen…");
        _connection.ForeColor = Warning;
        connectionFlow.Controls.Add(_connection);
        root.Controls.Add(connectionPanel, 0, 1);

        _metricFlow.Dock = DockStyle.Fill;
        _metricFlow.AutoScroll = true;
        _metricFlow.WrapContents = true;
        _metricFlow.FlowDirection = FlowDirection.LeftToRight;
        _metricFlow.Padding = new Padding(0, 12, 0, 12);
        _metricFlow.Margin = new Padding(0);
        _metricFlow.BackColor = Background;
        _metricFlow.SizeChanged += (_, _) => ResizeMetricCards();
        root.Controls.Add(_metricFlow, 0, 2);

        AddMetric("cpuTemp", T("NHIỆT ĐỘ CPU", "CPU TEMP"), "-- °C");
        AddMetric("cpuLoad", T("TẢI CPU", "CPU LOAD"), "-- %");
        AddMetric("cpuClock", T("XUNG CPU", "CPU CLOCK"), "-- MHz");
        AddMetric("cpuPower", T("CÔNG SUẤT CPU", "CPU POWER"), "-- W");
        AddMetric("cpuFan", T("QUẠT CPU", "CPU FAN"), "-- RPM");
        AddMetric("gpuTemp", T("NHIỆT ĐỘ GPU", "GPU TEMP"), "-- °C");
        AddMetric("gpuHotspot", T("ĐIỂM NÓNG GPU", "GPU HOTSPOT"), "-- °C");
        AddMetric("gpuLoad", T("TẢI GPU", "GPU LOAD"), "-- %");
        AddMetric("gpuClock", T("XUNG GPU", "GPU CLOCK"), "-- MHz");
        AddMetric("gpuPower", T("CÔNG SUẤT GPU", "GPU POWER"), "-- W");
        AddMetric("gpuFan", T("QUẠT GPU", "GPU FAN"), "-- RPM");
        AddMetric("gpuMemory", T("BỘ NHỚ GPU", "GPU MEMORY"), "-- %");
        AddMetric("memory", T("TẢI RAM", "RAM LOAD"), "-- %");
        AddMetric("memoryUsed", T("RAM ĐÃ DÙNG", "RAM USED"), "-- GB");
        AddMetric("disk", T("TẢI Ổ ĐĨA", "DISK LOAD"), "-- %");
        AddMetric("diskTemp", T("NHIỆT ĐỘ Ổ ĐĨA", "DISK TEMP"), "-- °C");
        AddMetric("networkDown", T("TẢI XUỐNG", "DOWNLOAD"), "-- Mbps");
        AddMetric("networkUp", T("TẢI LÊN", "UPLOAD"), "-- Mbps");

        var controls = Card();
        controls.Dock = DockStyle.Fill;
        controls.AutoSize = true;
        controls.MinimumSize = new Size(0, 235);
        controls.Padding = new Padding(12);
        var bottom = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            AutoSize = false,
            Height = 208,
            WrapContents = true,
            BackColor = PanelColor
        };
        controls.Controls.Add(bottom);

        _screenEnabled.Text = T("Bật màn hình", "Display on");
        ConfigureCheckBox(_screenEnabled);
        _screenEnabled.CheckedChanged += (_, _) =>
        {
            SendDisplayState();
            SaveSettings();
        };
        bottom.Controls.Add(_screenEnabled);
        bottom.Controls.Add(Caption(T("Độ sáng", "Brightness")));
        _brightness.Minimum = 0;
        _brightness.Maximum = 100;
        _brightness.TickFrequency = 10;
        _brightness.Width = 160;
        _brightness.Scroll += (_, _) =>
        {
            _brightnessSendTimer.Stop();
            _brightnessSendTimer.Start();
            SaveSettings();
        };
        bottom.Controls.Add(_brightness);

        _autoStart.Text = T("Tự chạy cùng Windows", "Start with Windows");
        ConfigureCheckBox(_autoStart);
        _autoStart.CheckedChanged += (_, _) =>
        {
            if (!_initializing) SetAutoStart(_autoStart.Checked, _startMinimized.Checked);
        };
        bottom.Controls.Add(_autoStart);

        _startMinimized.Text = T("Khởi động ẩn", "Start minimized");
        ConfigureCheckBox(_startMinimized);
        _startMinimized.CheckedChanged += (_, _) =>
        {
            SaveSettings();
            if (_autoStart.Checked && !_initializing) SetAutoStart(true, _startMinimized.Checked);
        };
        bottom.Controls.Add(_startMinimized);

        _minimizeToTray.Text = T("Đóng xuống khay", "Close to tray");
        ConfigureCheckBox(_minimizeToTray);
        _minimizeToTray.CheckedChanged += (_, _) => SaveSettings();
        bottom.Controls.Add(_minimizeToTray);

        var hide = ActionButton(T("Ẩn xuống khay", "Hide to tray"), false);
        hide.Click += (_, _) => HideToTray(true);
        bottom.Controls.Add(hide);
        var copy = ActionButton(T("Sao chép dữ liệu", "Copy telemetry"), false);
        copy.Click += (_, _) => CopyLatestTelemetry();
        bottom.Controls.Add(copy);
        var report = ActionButton(T("Xuất báo cáo cảm biến", "Export sensor report"), false);
        report.Click += async (_, _) => await ExportSensorReportAsync();
        bottom.Controls.Add(report);
        var sensorHelp = ActionButton(T("Khắc phục cảm biến", "Sensor help"), false);
        sensorHelp.Click += (_, _) => OpenSensorHelp();
        bottom.Controls.Add(sensorHelp);
        var web = ActionButton(T("Mở Web ESP32", "Open ESP32 web"), false);
        web.Click += (_, _) => OpenDeviceWeb();
        bottom.Controls.Add(web);
        var dataFolder = ActionButton(T("Thư mục dữ liệu", "Data folder"), false);
        dataFolder.Click += (_, _) => OpenDataFolder();
        bottom.Controls.Add(dataFolder);
        var about = ActionButton(T("Thông tin", "About"), false);
        about.Click += (_, _) => ShowAbout();
        bottom.Controls.Add(about);

        _sensorHealth.AutoSize = true;
        _sensorHealth.ForeColor = Warning;
        _sensorHealth.Padding = new Padding(8, 9, 0, 0);
        _sensorHealth.Text = T("Đang kiểm tra nguồn cảm biến…", "Checking sensor sources…");
        bottom.Controls.Add(_sensorHealth);

        _lastUpdate.AutoSize = true;
        _lastUpdate.ForeColor = Muted;
        _lastUpdate.Padding = new Padding(8, 9, 0, 0);
        _lastUpdate.Text = T("Đang khởi tạo bộ đọc cảm biến", "Initializing sensor reader");
        bottom.Controls.Add(_lastUpdate);
        root.Controls.Add(controls, 0, 3);
    }

    private void BuildTray()
    {
        var show = new ToolStripMenuItem(T("Mở PCScreen Agent", "Open PCScreen Agent"));
        show.Click += (_, _) => ShowWindow();
        _trayPause.Text = T("Tạm dừng gửi dữ liệu", "Pause telemetry");
        _trayPause.Click += (_, _) => ToggleRunning();
        var reconnect = new ToolStripMenuItem(T("Kết nối lại ESP32", "Reconnect ESP32"));
        reconnect.Click += (_, _) => ForceReconnect();
        var web = new ToolStripMenuItem(T("Mở Web ESP32", "Open ESP32 web"));
        web.Click += (_, _) => OpenDeviceWeb();
        var dataFolder = new ToolStripMenuItem(T("Mở thư mục log và cấu hình", "Open logs and settings folder"));
        dataFolder.Click += (_, _) => OpenDataFolder();
        var exit = new ToolStripMenuItem(T("Thoát hoàn toàn", "Exit"));
        exit.Click += (_, _) => ExitApplication();
        _trayMenu.Items.AddRange(new ToolStripItem[]
        {
            show, _trayPause, reconnect, web, dataFolder,
            new ToolStripSeparator(), exit
        });
        _tray.Icon = Icon ?? SystemIcons.Application;
        _tray.Text = "PCScreen Windows Agent";
        _tray.ContextMenuStrip = _trayMenu;
        _tray.Visible = true;
        _tray.DoubleClick += (_, _) => ShowWindow();
    }

    private void ApplySettings()
    {
        _interval.Value = Math.Clamp(_settings.IntervalMs,
            (int)_interval.Minimum, (int)_interval.Maximum);
        _screenEnabled.Checked = _settings.ScreenEnabled;
        _brightness.Value = Math.Clamp(_settings.Brightness,
            _brightness.Minimum, _brightness.Maximum);
        _minimizeToTray.Checked = _settings.MinimizeToTray;
        _startMinimized.Checked = _settings.StartMinimized;
    }

    private static void ConfigureCheckBox(CheckBox checkBox)
    {
        checkBox.AutoSize = true;
        checkBox.ForeColor = Color.White;
        checkBox.Margin = new Padding(8, 12, 10, 0);
    }

    private static Panel Card() => new()
    {
        BackColor = PanelColor,
        Margin = new Padding(5),
        BorderStyle = BorderStyle.FixedSingle
    };

    private static Label Caption(string text) => new()
    {
        AutoSize = true,
        Text = text,
        ForeColor = Muted,
        Padding = new Padding(4, 9, 3, 0)
    };

    private void AddMetric(string key, string caption, string initial)
    {
        var card = Card();
        card.Size = new Size(225, 102);
        card.Padding = new Padding(13);
        var layout = new TableLayoutPanel
        {
            Dock = DockStyle.Fill,
            ColumnCount = 1,
            RowCount = 3,
            Margin = Padding.Empty,
            BackColor = PanelColor
        };
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 8));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        var label = new Label
        {
            Dock = DockStyle.Fill,
            AutoSize = true,
            Text = caption,
            ForeColor = Muted,
            Font = new Font("Segoe UI", 8.5f, FontStyle.Bold)
        };
        var barHost = new Panel { Dock = DockStyle.Fill, BackColor = PanelColor };
        var bar = new Panel
        {
            BackColor = Accent,
            Height = 3,
            Width = 45,
            Location = new Point(0, 3)
        };
        barHost.Controls.Add(bar);
        var value = new Label
        {
            Dock = DockStyle.Fill,
            Text = initial,
            ForeColor = Color.White,
            Font = new Font("Segoe UI", 18, FontStyle.Bold),
            TextAlign = ContentAlignment.MiddleLeft,
            AutoEllipsis = true
        };
        layout.Controls.Add(label, 0, 0);
        layout.Controls.Add(barHost, 0, 1);
        layout.Controls.Add(value, 0, 2);
        card.Controls.Add(layout);
        _values[key] = value;
        _metricFlow.Controls.Add(card);
    }

    private void ResizeMetricCards()
    {
        if (_metricFlow.ClientSize.Width <= 0) return;
        var available = Math.Max(240, _metricFlow.ClientSize.Width - 22);
        var columns = available >= 1080 ? 4
                    : available >= 760 ? 3
                    : available >= 500 ? 2 : 1;
        var cardWidth = Math.Max(220, available / columns - 12);
        _metricFlow.SuspendLayout();
        foreach (Control card in _metricFlow.Controls)
        {
            card.Width = cardWidth;
            card.Height = 102;
        }
        _metricFlow.ResumeLayout();
    }

    private Button ActionButton(string text, bool primary)
    {
        var button = new Button { Text = text, AutoSize = true };
        StyleButton(button, primary);
        return button;
    }

    private static void StyleButton(Button button, bool primary)
    {
        button.FlatStyle = FlatStyle.Flat;
        button.FlatAppearance.BorderColor = primary ? Accent : Border;
        button.BackColor = primary ? Accent : Color.FromArgb(36, 38, 44);
        button.ForeColor = Color.White;
        button.Padding = new Padding(6, 3, 6, 3);
        button.Margin = new Padding(6, 3, 6, 3);
    }

    private static void StyleInput(Control input)
    {
        input.BackColor = Color.FromArgb(31, 33, 38);
        input.ForeColor = Color.White;
        input.Margin = new Padding(5, 5, 8, 3);
    }

    private void RefreshPorts()
    {
        var selected = _ports.SelectedIndex <= 0
            ? _settings.Port : _ports.SelectedItem?.ToString();
        _ports.Items.Clear();
        _ports.Items.Add(T("Tự động", "Automatic"));
        _ports.Items.AddRange(DeviceConnection.Ports());
        _ports.SelectedItem = !string.IsNullOrWhiteSpace(selected) &&
                              selected != "auto" && selected != "Tự động" &&
                              _ports.Items.Contains(selected)
            ? selected : _ports.Items[0];
    }

    private async void OnShown(object? sender, EventArgs e)
    {
        ResizeMetricCards();
        if (_requestedHidden || _settings.StartMinimized) HideToTray(false);
        await UpdateOnceAsync();
    }

    private async void OnTick(object? sender, EventArgs e) =>
        await UpdateOnceAsync();

    private async Task UpdateOnceAsync()
    {
        if (!_running || _updating) return;
        _updating = true;
        try
        {
            var snapshot = await Task.Run(_hardware.Read, _shutdown.Token);
            _lastSnapshot = snapshot;
            if (Visible) Render(snapshot);

            if (!_device.IsConnected && DateTime.UtcNow >= _nextConnectAttempt)
            {
                try
                {
                    var preferred = _ports.SelectedIndex <= 0
                        ? null : _ports.SelectedItem?.ToString();
                    SetConnection(await _device.ConnectAsync(preferred, _shutdown.Token), Good);
                }
                catch (Exception ex) when (ex is not OperationCanceledException)
                {
                    _nextConnectAttempt = DateTime.UtcNow.AddSeconds(5);
                    SetConnection($"{ex.Message} {T("Thử lại sau 5 giây.", "Retrying in 5 seconds.")}", Warning);
                }
            }

            if (_device.IsConnected)
            {
                try
                {
                    await Task.Run(() => _device.SendTelemetry(snapshot),
                        _shutdown.Token);
                    _sentPackets++;
                    _lastUpdate.Text =
                        $"{T("Đã gửi", "Sent")} {DateTime.Now:HH:mm:ss} • {_device.PortName} • " +
                        $"{_interval.Value} ms • {T("gói", "packet")} {_sentPackets:N0}";
                    _tray.Text = TrayText(
                        $"PCScreen • {_device.PortName} • {T("đang truyền dữ liệu", "sending telemetry")}");
                }
                catch (Exception ex)
                {
                    _operationErrors++;
                    _nextConnectAttempt = DateTime.UtcNow.AddSeconds(3);
                    SetConnection($"{T("Mất kết nối", "Disconnected")}: {ex.Message}", Error);
                }
            }
            else
            {
                _lastUpdate.Text = $"{T("Đã đọc cảm biến", "Sensors read")} {DateTime.Now:HH:mm:ss} • {T("chờ ESP32", "waiting for ESP32")}";
                _tray.Text = TrayText($"PCScreen • {T("chờ ESP32", "waiting for ESP32")}");
            }
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            _operationErrors++;
            SetConnection($"{T("Lỗi đọc cảm biến", "Sensor read error")}: {ex.Message}", Error);
            _tray.Text = TrayText($"PCScreen • {T("lỗi cảm biến", "sensor error")}");
        }
        finally
        {
            _updating = false;
        }
    }

    private void SetConnection(string text, Color color)
    {
        _connection.Text = text;
        _connection.ForeColor = color;
        if (!string.Equals(_lastLoggedStatus, text, StringComparison.Ordinal))
        {
            AppLog.Write(text);
            _lastLoggedStatus = text;
        }
    }

    private static string TrayText(string text) =>
        text.Length <= 63 ? text : text[..63];

    private void Render(TelemetrySnapshot s)
    {
        _values["cpuTemp"].Text = Value(s.CpuTemperature, "°C");
        _values["cpuLoad"].Text = Value(s.CpuLoad, "%");
        _values["cpuClock"].Text = Value(s.CpuClockMHz, "MHz");
        _values["cpuPower"].Text = Value(s.CpuPowerWatts, "W");
        _values["cpuFan"].Text = Value(s.CpuFanRpm, "RPM");
        _values["gpuTemp"].Text = Value(s.GpuTemperature, "°C");
        _values["gpuHotspot"].Text = Value(s.GpuHotspot, "°C");
        _values["gpuLoad"].Text = Value(s.GpuLoad, "%");
        _values["gpuClock"].Text = Value(s.GpuClockMHz, "MHz");
        _values["gpuPower"].Text = Value(s.GpuPowerWatts, "W");
        _values["gpuFan"].Text = Value(s.GpuFanRpm, "RPM");
        _values["gpuMemory"].Text = Value(s.GpuMemoryLoad, "%");
        _values["memory"].Text = Value(s.MemoryLoad, "%");
        _values["memoryUsed"].Text = Value(s.MemoryUsedGb, "GB", 1);
        _values["disk"].Text = Value(s.DiskLoad, "%");
        _values["diskTemp"].Text = Value(s.DiskTemperature, "°C");
        _values["networkDown"].Text = Value(s.NetworkDownMbps, "Mbps", 1);
        _values["networkUp"].Text = Value(s.NetworkUpMbps, "Mbps", 1);
        UpdateSensorHealth(s);
    }

    private void UpdateSensorHealth(TelemetrySnapshot s)
    {
        var cpuLowLevelMissing = !s.CpuTemperature.HasValue &&
                                 !s.CpuFanRpm.HasValue;
        var gpuMissing = !s.GpuTemperature.HasValue &&
                         !s.GpuLoad.HasValue &&
                         !s.GpuClockMHz.HasValue;
        if (cpuLowLevelMissing && gpuMissing)
        {
            _sensorHealth.Text = T("Thiếu cảm biến mức thấp • kiểm tra PawnIO/driver GPU", "Low-level sensors missing • check PawnIO/GPU driver");
            _sensorHealth.ForeColor = Warning;
        }
        else if (gpuMissing)
        {
            _sensorHealth.Text = T("GPU chưa cung cấp dữ liệu • xuất báo cáo cảm biến", "GPU telemetry unavailable • export a sensor report");
            _sensorHealth.ForeColor = Warning;
        }
        else
        {
            _sensorHealth.Text = T("Nguồn cảm biến đang hoạt động", "Sensor sources are working");
            _sensorHealth.ForeColor = Good;
        }
    }

    private static string Value(float? value, string unit, int decimals = 0) =>
        value.HasValue ? $"{Math.Round(value.Value, decimals)} {unit}" : $"-- {unit}";

    private void ToggleRunning()
    {
        _running = !_running;
        _run.Text = _running ? T("Tạm dừng", "Pause") : T("Tiếp tục", "Resume");
        _trayPause.Text = _running ? T("Tạm dừng gửi dữ liệu", "Pause telemetry") : T("Tiếp tục gửi dữ liệu", "Resume telemetry");
        SetConnection(_running ? T("Đang tiếp tục đọc và gửi dữ liệu…", "Resuming sensor reads and telemetry…")
                               : T("Đã tạm dừng đọc và gửi dữ liệu", "Sensor reads and telemetry paused"),
                      _running ? Warning : Muted);
        if (_running) _ = UpdateOnceAsync();
    }

    private void ForceReconnect()
    {
        _device.Disconnect();
        _nextConnectAttempt = DateTime.MinValue;
        SetConnection(T("Đang kết nối lại PCScreen…", "Reconnecting PCScreen…"), Warning);
        if (_running) _ = UpdateOnceAsync();
    }

    private async void SendDisplayState()
    {
        if (_initializing) return;
        try
        {
            if (_device.IsConnected)
                await Task.Run(() => _device.SetBacklight(_screenEnabled.Checked),
                               _shutdown.Token);
        }
        catch (Exception ex)
        {
            SetConnection(ex.Message, Error);
        }
    }

    private async void SendBrightness()
    {
        if (_initializing) return;
        try
        {
            if (_device.IsConnected)
                await Task.Run(() => _device.SetBrightness(_brightness.Value),
                               _shutdown.Token);
        }
        catch (Exception ex)
        {
            SetConnection(ex.Message, Error);
        }
    }

    private void CopyLatestTelemetry()
    {
        if (_lastSnapshot is null)
        {
            SetConnection(T("Chưa có dữ liệu để sao chép.", "No telemetry is available to copy."), Warning);
            return;
        }
        try
        {
            Clipboard.SetText(JsonSerializer.Serialize(_lastSnapshot,
                new JsonSerializerOptions { WriteIndented = true }));
            SetConnection(T("Đã sao chép gói dữ liệu JSON.", "Telemetry JSON copied."), Good);
        }
        catch (Exception ex)
        {
            SetConnection($"{T("Không sao chép được", "Copy failed")}: {ex.Message}", Error);
        }
    }

    private async Task ExportSensorReportAsync()
    {
        try
        {
            SetConnection(T("Đang lập danh sách toàn bộ cảm biến…", "Building a complete sensor list…"), Warning);
            var report = await Task.Run(_hardware.CreateSensorReport,
                _shutdown.Token);
            using var dialog = new SaveFileDialog
            {
                Title = T("Lưu báo cáo cảm biến PCScreen", "Save PCScreen sensor report"),
                Filter = T("Tệp văn bản (*.txt)|*.txt|Tất cả tệp (*.*)|*.*", "Text file (*.txt)|*.txt|All files (*.*)|*.*"),
                FileName = $"PCScreen-Sensor-Report-{DateTime.Now:yyyyMMdd-HHmmss}.txt",
                InitialDirectory = Environment.GetFolderPath(
                    Environment.SpecialFolder.DesktopDirectory)
            };
            if (dialog.ShowDialog(this) != DialogResult.OK) return;
            await File.WriteAllTextAsync(dialog.FileName, report,
                _shutdown.Token);
            SetConnection(T("Đã lưu báo cáo cảm biến. Hãy gửi tệp này để ánh xạ cảm biến chính xác.", "Sensor report saved. Share this file to map sensors accurately."), Good);
            Process.Start(new ProcessStartInfo
            {
                FileName = "explorer.exe",
                Arguments = $"/select,\"{dialog.FileName}\"",
                UseShellExecute = true
            });
        }
        catch (OperationCanceledException) { }
        catch (Exception ex)
        {
            SetConnection($"{T("Không xuất được báo cáo", "Report export failed")}: {ex.Message}", Error);
        }
    }

    private static void OpenSensorHelp()
    {
        MessageBox.Show(
            T("Nhiệt độ, xung nhịp và quạt CPU cần lớp truy cập phần cứng PawnIO của " +
              "LibreHardwareMonitor 0.9.6. Hãy tải bản chính thức, chạy một lần với " +
              "quyền Quản trị viên, đồng ý cài PawnIO rồi khởi động lại Windows.\n\n" +
              "Sau đó chỉ cần chạy PCScreen Windows Agent; không cần giữ LibreHardwareMonitor mở.",
              "CPU temperature, clock, and fan data require the PawnIO hardware access layer " +
              "from LibreHardwareMonitor 0.9.6. Download the official release, run it once as " +
              "Administrator, approve PawnIO installation, and restart Windows.\n\n" +
              "After that, only PCScreen Windows Agent needs to remain open."),
            T("Khắc phục cảm biến PCScreen", "PCScreen sensor help"), MessageBoxButtons.OK,
            MessageBoxIcon.Information);
        Process.Start(new ProcessStartInfo
        {
            FileName = "https://github.com/LibreHardwareMonitor/LibreHardwareMonitor/releases/tag/v0.9.6",
            UseShellExecute = true
        });
    }

    private static void OpenDeviceWeb()
    {
        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = "http://pcscreen.local/",
                UseShellExecute = true
            });
        }
        catch (Exception ex)
        {
            MessageBox.Show($"{T("Không mở được", "Could not open")} pcscreen.local: {ex.Message}",
                "PCScreen Web", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
    }

    private static void OpenDataFolder()
    {
        try
        {
            Directory.CreateDirectory(AppLog.DirectoryPath);
            Process.Start(new ProcessStartInfo
            {
                FileName = AppLog.DirectoryPath,
                UseShellExecute = true
            });
        }
        catch (Exception ex)
        {
            MessageBox.Show($"{T("Không mở được thư mục dữ liệu", "Could not open the data folder")}: {ex.Message}",
                "PCScreen", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }
    }

    private void ShowAbout()
    {
        var uptime = DateTime.Now - _startedAt;
        var version = Application.ProductVersion;
        MessageBox.Show(
            $"PCScreen Windows Agent {version}\n" +
            T("Phát triển bởi Nguyễn Tiến Thành • GitHub TienThanhDEV\n\n", "Developed by Nguyễn Tiến Thành • GitHub TienThanhDEV\n\n") +
            $"{T("Cổng hiện tại", "Current port")}: {_device.PortName}\n" +
            $"{T("Gói dữ liệu đã gửi", "Telemetry packets sent")}: {_sentPackets:N0}\n" +
            $"{T("Lỗi vận hành", "Operation errors")}: {_operationErrors:N0}\n" +
            $"{T("Thời gian chạy", "Uptime")}: {uptime:dd\\.hh\\:mm\\:ss}\n" +
            "Sensor engine: LibreHardwareMonitorLib 0.9.6\n" +
            $"Log: {AppLog.FilePath}",
            T("Thông tin PCScreen", "About PCScreen"), MessageBoxButtons.OK,
            MessageBoxIcon.Information);
    }

    private void SaveSettings()
    {
        if (_initializing) return;
        _settingsSaveTimer.Stop();
        _settingsSaveTimer.Start();
    }

    private void PersistSettings()
    {
        _settings.Port = _ports.SelectedIndex <= 0
            ? "auto" : _ports.SelectedItem?.ToString() ?? "auto";
        _settings.IntervalMs = (int)_interval.Value;
        _settings.ScreenEnabled = _screenEnabled.Checked;
        _settings.Brightness = _brightness.Value;
        _settings.MinimizeToTray = _minimizeToTray.Checked;
        _settings.StartMinimized = _startMinimized.Checked;
        _settings.Save();
    }

    private void OnWindowResize(object? sender, EventArgs e)
    {
        if (WindowState == FormWindowState.Minimized && _minimizeToTray.Checked)
            BeginInvoke(new Action(() => HideToTray(true)));
        else
            ResizeMetricCards();
    }

    private void HideToTray(bool showHint)
    {
        Hide();
        ShowInTaskbar = false;
        if (showHint && !_shownTrayHint)
        {
            _tray.ShowBalloonTip(2500,
                T("PCScreen vẫn đang chạy", "PCScreen is still running"),
                T("Ứng dụng tiếp tục đọc cảm biến và gửi dữ liệu ở khay hệ thống.",
                  "The app continues reading sensors and sending telemetry from the system tray."),
                ToolTipIcon.Info);
            _shownTrayHint = true;
        }
    }

    private void ShowWindow()
    {
        ShowInTaskbar = true;
        Show();
        WindowState = FormWindowState.Normal;
        Activate();
        if (_lastSnapshot is not null) Render(_lastSnapshot);
        ResizeMetricCards();
    }

    private void ExitApplication()
    {
        _allowExit = true;
        Close();
    }

    private static bool ReadAutoStart()
    {
        using var key = Registry.CurrentUser.OpenSubKey(
            @"Software\Microsoft\Windows\CurrentVersion\Run");
        return key?.GetValue("PCScreenWindowsAgent") is not null;
    }

    private static void SetAutoStart(bool enabled, bool startMinimized)
    {
        using var key = Registry.CurrentUser.CreateSubKey(
            @"Software\Microsoft\Windows\CurrentVersion\Run");
        if (enabled)
        {
            var argument = startMinimized ? " --minimized" : string.Empty;
            key.SetValue("PCScreenWindowsAgent",
                $"\"{Application.ExecutablePath}\"{argument}");
        }
        else
        {
            key.DeleteValue("PCScreenWindowsAgent", false);
        }
    }

    private void OnClosing(object? sender, FormClosingEventArgs e)
    {
        if (!_allowExit && e.CloseReason == CloseReason.UserClosing &&
            _minimizeToTray.Checked)
        {
            e.Cancel = true;
            HideToTray(true);
            return;
        }

        _timer.Stop();
        _settingsSaveTimer.Stop();
        PersistSettings();
        AppLog.Write($"Agent stopping; packets={_sentPackets}; errors={_operationErrors}");
        _shutdown.Cancel();
        _tray.Visible = false;
        _tray.Dispose();
        _trayMenu.Dispose();
        _device.Dispose();
        _hardware.Dispose();
        _shutdown.Dispose();
    }
}
