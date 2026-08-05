using System.IO.Ports;

namespace TmsControlPanel;

public partial class Form1 : Form
{
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
        btnConnect.Click += BtnConnect_Click;
        btnEnterMenu.Click += BtnEnterMenu_Click;
        btnSend.Click += BtnSend_Click;              // YENİ
        txtInput.KeyDown += TxtInput_KeyDown;        // YENİ: Enter ile gönder
        this.FormClosing += Form1_FormClosing;

        SetConnectedState(false);
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
            _serialPort = new SerialPort(portName, baud, Parity.None, 8, StopBits.One);
            _serialPort.DataReceived += SerialPort_DataReceived;
            _serialPort.Open();

            SetConnectedState(true);
            lblStatus.Text = $"Bağlı: {portName} @ {baud}";
            AppendLog($"--- Bağlandı: {portName} @ {baud} ---\r\n");
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Port açılamadı:\n{ex.Message}", "Bağlantı hatası");
            _serialPort?.Dispose();
            _serialPort = null;
        }
    }

    private void Disconnect()
    {
        AppendLog("--- Bağlantı kesildi ---\r\n");
        _serialPort?.Close();
        _serialPort?.Dispose();
        _serialPort = null;

        SetConnectedState(false);
    }

    private void SerialPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
    {
        try
        {
            SerialPort sp = (SerialPort)sender;
            string data = sp.ReadExisting();
            AppendLog(data);
        }
        catch
        {
        }
    }

    private void AppendLog(string text)
    {
        if (txtLog.IsDisposed) return;

        if (txtLog.InvokeRequired)
        {
            txtLog.BeginInvoke(new Action(() => AppendLog(text)));
            return;
        }

        txtLog.AppendText(text);
    }

    private void BtnEnterMenu_Click(object? sender, EventArgs e)
    {
        _serialPort?.Write("q");
    }

    // YENİ: Yazılan metni karta gönder (satır sonu '\r' ekleyerek)
    private void SendLine(string text)
    {
        if (_serialPort == null || !_serialPort.IsOpen)
        {
            return;
        }
        _serialPort.Write(text + "\r");   // menü girişi '\r' ile biter
    }

    // YENİ: "Gönder" butonu
    private void BtnSend_Click(object? sender, EventArgs e)
    {
        SendLine(txtInput.Text);
        txtInput.Clear();
        txtInput.Focus();
    }

    // YENİ: Giriş kutusunda Enter'a basınca da gönder
    private void TxtInput_KeyDown(object? sender, KeyEventArgs e)
    {
        if (e.KeyCode == Keys.Enter)
        {
            SendLine(txtInput.Text);
            txtInput.Clear();
            e.SuppressKeyPress = true;   // Enter'ın 'ding' sesini engelle
        }
    }

    private void SetConnectedState(bool connected)
    {
        btnConnect.Text = connected ? "Kes" : "Bağlan";

        cmbPort.Enabled = !connected;
        cmbBaud.Enabled = !connected;
        btnRefresh.Enabled = !connected;

        btnEnterMenu.Enabled = connected;
        txtInput.Enabled = connected;     // YENİ
        btnSend.Enabled = connected;      // YENİ

        if (!connected)
        {
            lblStatus.Text = "Bağlı değil";
        }
    }

    private void Form1_FormClosing(object? sender, FormClosingEventArgs e)
    {
        _serialPort?.Close();
        _serialPort?.Dispose();
    }
}