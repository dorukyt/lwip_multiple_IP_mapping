using System.IO.Ports;
using System.Net;

namespace TmsControlPanel;

public partial class Form1 : Form
{
    private SerialManager _serial = null!;      // ctor'da (InitializeComponent'ten sonra) oluşturulur
    private AddNetifCard _addCard = null!;

    private readonly System.Windows.Forms.Timer _pollTimer = new();
    private DateTime _pausePollUntil = DateTime.MinValue;  // menü/terminal kullanımında oto-yenileme molası
    private bool _refreshBusy;
    private string _lastSignature = "";                    // son çizilen listenin imzası (gereksiz yeniden çizim olmasın)

    public Form1()
    {
        InitializeComponent();

        // SerialManager UI thread bağlamını yakalar -> event'leri bize doğru thread'de verir
        _serial = new SerialManager();
        _serial.RawReceived += chunk => txtRxLog.AppendText(chunk);
        _serial.TextSent += text => txtTxLog.AppendText(text.TrimEnd('\r') + Environment.NewLine);

        cmbPort.DropDownStyle = ComboBoxStyle.DropDownList;
        cmbBaud.DropDownStyle = ComboBoxStyle.DropDownList;

        LoadBaudRates();
        LoadPorts();

        btnRefresh.Click += (s, e) => LoadPorts();
        btnConnect.Click += BtnConnect_Click;
        btnEnterMenu.Click += BtnEnterMenu_Click;
        btnSend.Click += (s, e) => SendTerminalLine();
        btnClear.Click += (s, e) => { txtRxLog.Clear(); txtTxLog.Clear(); };
        txtInput.KeyDown += TxtInput_KeyDown;
        FormClosing += (s, e) => _serial.Disconnect();
        flpNetifs.SizeChanged += (s, e) => UpdateCardWidths();

        // "+" kartı listenin sonunda hep durur
        _addCard = new AddNetifCard();
        _addCard.AddRequested += AddCard_AddRequested;
        flpNetifs.Controls.Add(_addCard);

        // oto-yenileme: boştayken 2 sn'de bir NETIF LIST sor
        _pollTimer.Interval = 2000;
        _pollTimer.Tick += async (s, e) => await PollTickAsync();

        SetConnectedState(false);
        UpdateCardWidths();
    }

    // ------------------------------------------------------------------
    //  Bağlantı
    // ------------------------------------------------------------------

    private void LoadBaudRates()
    {
        int[] baudRates = { 9600, 19200, 38400, 57600, 115200 };
        cmbBaud.Items.Clear();
        foreach (int baud in baudRates)
            cmbBaud.Items.Add(baud);
        cmbBaud.SelectedItem = 9600;
    }

    private void LoadPorts()
    {
        cmbPort.Items.Clear();
        cmbPort.Items.AddRange(SerialPort.GetPortNames());
        if (cmbPort.Items.Count > 0)
            cmbPort.SelectedIndex = 0;
    }

    private void BtnConnect_Click(object? sender, EventArgs e)
    {
        if (_serial.IsOpen)
        {
            _serial.Disconnect();
            SetConnectedState(false);
            return;
        }

        if (cmbPort.SelectedItem == null)
        {
            MessageBox.Show("Önce bir COM port seç.", "Port yok");
            return;
        }

        string portName = cmbPort.SelectedItem.ToString()!;
        int baud = (int)cmbBaud.SelectedItem!;

        try
        {
            _serial.Connect(portName, baud);
            SetConnectedState(true);
            lblStatus.Text = $"Bağlı: {portName} @ {baud}";
            _pausePollUntil = DateTime.MinValue;
            _ = ForceRefreshAsync();   // ilk listeyi beklemeden çek (fire-and-forget)
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Port açılamadı:\n{ex.Message}", "Bağlantı hatası");
        }
    }

    private void SetConnectedState(bool connected)
    {
        btnConnect.Text = connected ? "Kes" : "Bağlan";

        cmbPort.Enabled = !connected;
        cmbBaud.Enabled = !connected;
        btnRefresh.Enabled = !connected;

        txtInput.Enabled = connected;
        btnSend.Enabled = connected;
        btnEnterMenu.Enabled = connected;
        _addCard.Enabled = connected;

        if (connected)
        {
            _pollTimer.Start();
        }
        else
        {
            _pollTimer.Stop();
            lblStatus.Text = "Bağlı değil";
            ClearCards();
            _lastSignature = "";
        }
    }

    // ------------------------------------------------------------------
    //  Netif kartları + oto-yenileme
    // ------------------------------------------------------------------

    private async Task PollTickAsync()
    {
        if (DateTime.Now < _pausePollUntil) return;   // menü/terminal molası
        await RefreshNetifsAsync();
    }

    /// <summary>İmzayı sıfırlayıp yenile: değişiklik olmasa bile kartlar tazelenir.</summary>
    private async Task ForceRefreshAsync()
    {
        _lastSignature = "";
        await RefreshNetifsAsync();
    }

    private async Task RefreshNetifsAsync()
    {
        if (!_serial.IsOpen || _refreshBusy) return;
        _refreshBusy = true;
        try
        {
            // quiet: oto-yenileme trafiği RX/TX loglarını doldurmasın
            ProtocolResponse resp = await _serial.SendCommandAsync("NETIF LIST", 4000, quiet: true);
            if (!resp.Ok) return;

            List<NetifInfo> netifs = Protocol.ParseNetifList(resp);
            string sig = BuildSignature(netifs);
            if (sig == _lastSignature) return;   // değişiklik yok -> ekrana dokunma

            _lastSignature = sig;
            RebuildCards(netifs);
        }
        catch
        {
            // zaman aşımı vb: oto-yenilemede sessiz geç (ör. insan menüsü açıkken normaldir)
        }
        finally
        {
            _refreshBusy = false;
        }
    }

    private static string BuildSignature(List<NetifInfo> netifs) =>
        string.Join("|", netifs.Select(n =>
            $"{n.Index},{n.Ip},{n.Mask},{n.Gw},{string.Join("+", n.Sockets.Select(s => s.Port))}"));

    private void RebuildCards(List<NetifInfo> netifs)
    {
        flpNetifs.SuspendLayout();

        foreach (NetifCard old in flpNetifs.Controls.OfType<NetifCard>().ToList())
        {
            flpNetifs.Controls.Remove(old);
            old.Dispose();
        }

        foreach (NetifInfo n in netifs)
        {
            var card = new NetifCard(n);
            card.DeleteRequested += Card_DeleteRequested;
            card.SocketDeleteRequested += Card_SocketDeleteRequested;
            flpNetifs.Controls.Add(card);
        }

        // "+" kartı her zaman en sonda
        flpNetifs.Controls.SetChildIndex(_addCard, flpNetifs.Controls.Count - 1);

        UpdateCardWidths();
        flpNetifs.ResumeLayout();
    }

    private void ClearCards()
    {
        foreach (NetifCard old in flpNetifs.Controls.OfType<NetifCard>().ToList())
        {
            flpNetifs.Controls.Remove(old);
            old.Dispose();
        }
    }

    private void UpdateCardWidths()
    {
        int w = flpNetifs.ClientSize.Width - 24;   // padding + scrollbar payı
        if (w < 300) w = 300;
        foreach (Control c in flpNetifs.Controls)
            c.Width = w;
    }

    // ------------------------------------------------------------------
    //  Kart olayları (sil / soket sil / netif ekle)
    // ------------------------------------------------------------------

    private async void Card_DeleteRequested(NetifCard card)
    {
        NetifInfo n = card.Netif;
        DialogResult answer = MessageBox.Show(
            $"{n.Index}.netif ({n.Ip}) silinsin mi?\nÜzerindeki tüm soketler de silinir.",
            "Netif Sil", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
        if (answer != DialogResult.Yes) return;

        try
        {
            ProtocolResponse resp = await _serial.SendCommandAsync($"NETIF DEL {n.Index}");
            if (!resp.Ok)
            {
                string msg = resp.Status.Contains("remove_failed")
                    ? "Bu netif silinemedi (fiziksel netif silinemez)."
                    : $"Hata: {resp.Status}";
                MessageBox.Show(msg, "Netif Sil");
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Hata: {ex.Message}", "Netif Sil");
        }

        await ForceRefreshAsync();
    }

    private async void Card_SocketDeleteRequested(NetifCard card, SocketInfo sock)
    {
        try
        {
            ProtocolResponse resp = await _serial.SendCommandAsync(
                $"SOCK CLOSE {card.Netif.Index} {sock.Nth}");
            if (!resp.Ok)
                MessageBox.Show($"Soket silinemedi: {resp.Status}", "Soket Sil");
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Hata: {ex.Message}", "Soket Sil");
        }

        await ForceRefreshAsync();
    }

    private async void AddCard_AddRequested(string ip, string mask, string gw)
    {
        if (!IPAddress.TryParse(ip, out _) ||
            !IPAddress.TryParse(mask, out _) ||
            !IPAddress.TryParse(gw, out _))
        {
            MessageBox.Show("Geçersiz IP / Netmask / GW değeri.", "Netif Ekle");
            return;
        }

        try
        {
            ProtocolResponse resp = await _serial.SendCommandAsync($"NETIF ADD {ip} {mask} {gw}");
            if (!resp.Ok)
            {
                MessageBox.Show($"Netif eklenemedi: {resp.Status}", "Netif Ekle");
                return;
            }
            _addCard.Collapse();
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Hata: {ex.Message}", "Netif Ekle");
        }

        await ForceRefreshAsync();
    }

    // ------------------------------------------------------------------
    //  Terminal (ham giriş) — menü kullanımı için hâlâ gerekli (ör. ping)
    // ------------------------------------------------------------------

    private void BtnEnterMenu_Click(object? sender, EventArgs e)
    {
        _serial.SendText("q");
        // insan menüsü açıkken NETIF LIST baytları menü girişine karışmasın
        _pausePollUntil = DateTime.Now.AddSeconds(60);
    }

    private void SendTerminalLine()
    {
        string text = txtInput.Text;
        if (text.Length == 0) return;

        _serial.SendText(text + "\r");
        _pausePollUntil = DateTime.Now.AddSeconds(15);   // etkileşim sürüyor olabilir
        txtInput.Clear();
        txtInput.Focus();
    }

    private void TxtInput_KeyDown(object? sender, KeyEventArgs e)
    {
        if (e.KeyCode == Keys.Enter)
        {
            SendTerminalLine();
            e.SuppressKeyPress = true;
        }
    }
}
