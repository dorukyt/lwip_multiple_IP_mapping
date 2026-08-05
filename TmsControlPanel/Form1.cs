using System.IO.Ports;

namespace TmsControlPanel;

public partial class Form1 : Form
{
    // YENİ: Karta açtığımız seri port. Bağlı değilken null olur.
    private SerialPort? _serialPort;

    public Form1()
    {
        InitializeComponent();

        cmbPort.DropDownStyle = ComboBoxStyle.DropDownList;
        cmbBaud.DropDownStyle = ComboBoxStyle.DropDownList;
        txtLog.ReadOnly = true;
        txtLog.ScrollBars = ScrollBars.Vertical;

        LoadBaudRates();
        LoadPorts();

        btnRefresh.Click += BtnRefresh_Click;
        btnConnect.Click += BtnConnect_Click;                 // YENİ
        this.FormClosing += Form1_FormClosing;                // YENİ: pencere kapanınca portu kapat

        SetConnectedState(false);                             // YENİ: başlangıçta bağlı değiliz
    }

    private void LoadBaudRates()
    {
        int[] baudRates = { 9600, 19200, 38400, 57600, 115200 };
        cmbBaud.Items.Clear();
        foreach (int baud in baudRates)
        {
            cmbBaud.Items.Add(baud);
        }
        cmbBaud.SelectedItem = 9600;
    }

    private void LoadPorts()
    {
        string[] ports = SerialPort.GetPortNames();
        cmbPort.Items.Clear();
        cmbPort.Items.AddRange(ports);
        if (cmbPort.Items.Count > 0)
        {
            cmbPort.SelectedIndex = 0;
        }
    }

    private void BtnRefresh_Click(object? sender, EventArgs e)
    {
        LoadPorts();
    }

    // YENİ: "Bağlan/Kes" butonu — bağlı değilsek bağlan, bağlıysak kes
    private void BtnConnect_Click(object? sender, EventArgs e)
    {
        if (_serialPort == null || !_serialPort.IsOpen)
        {
            Connect();
        }
        else
        {
            Disconnect();
        }
    }

    // YENİ: Seri portu aç
    private void Connect()
    {
        if (cmbPort.SelectedItem == null)
        {
            MessageBox.Show("Önce bir COM port seç.", "Port yok");
            return;
        }

        string portName = cmbPort.SelectedItem.ToString()!;
        int baud = (int)cmbBaud.SelectedItem!;

        try
        {
            // 8 veri biti, parite yok, 1 stop biti = "8N1"
            _serialPort = new SerialPort(portName, baud, Parity.None, 8, StopBits.One);
            _serialPort.Open();

            SetConnectedState(true);
            lblStatus.Text = $"Bağlı: {portName} @ {baud}";
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Port açılamadı:\n{ex.Message}", "Bağlantı hatası");
            _serialPort?.Dispose();
            _serialPort = null;
        }
    }

    // YENİ: Seri portu kapat
    private void Disconnect()
    {
        _serialPort?.Close();
        _serialPort?.Dispose();
        _serialPort = null;

        SetConnectedState(false);
    }

    // YENİ: Arayüzü bağlı/değil durumuna göre ayarla
    private void SetConnectedState(bool connected)
    {
        btnConnect.Text = connected ? "Kes" : "Bağlan";

        // Bağlıyken port/baud/yenile değiştirilemesin
        cmbPort.Enabled = !connected;
        cmbBaud.Enabled = !connected;
        btnRefresh.Enabled = !connected;

        // "Menüye Gir" sadece bağlıyken aktif olsun
        btnEnterMenu.Enabled = connected;

        if (!connected)
        {
            lblStatus.Text = "Bağlı değil";
        }
    }

    // YENİ: Pencere kapanırken portu düzgünce serbest bırak
    private void Form1_FormClosing(object? sender, FormClosingEventArgs e)
    {
        _serialPort?.Close();
        _serialPort?.Dispose();
    }
}