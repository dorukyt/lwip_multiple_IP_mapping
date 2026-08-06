using System.IO.Ports;
using System.Net;

namespace TmsControlPanel;

public partial class Form1 : Form
{
    private SerialManager _serial = null!;      // ctor'da (InitializeComponent'ten sonra) oluşturulur
    private AddNetifCard _addCard = null!;

    private readonly System.Windows.Forms.Timer _pollTimer = new();
    private DateTime _pausePollUntil = DateTime.MinValue;  // menü kullanımında oto-yenileme molası
    private bool _refreshBusy;
    private string _lastSignature = "";                    // son çizilen listenin imzası
    private List<NetifInfo> _netifs = new();               // en son bilinen netif listesi

    private CancellationTokenSource? _pingCts;             // sürekli ping'i durdurmak için

    private const int PingAttempts = 4;
    private const int ScanTimeoutMs = 60000;               // ARP taraması ~13 sn sürebiliyor

    public Form1()
    {
        InitializeComponent();

        // SerialManager UI thread bağlamını yakalar -> event'leri bize doğru thread'de verir
        _serial = new SerialManager();
        _serial.RawReceived += chunk => txtRxLog.AppendText(chunk);
        _serial.TextSent += text => txtTxLog.AppendText(text.TrimEnd('\r') + Environment.NewLine);
        _serial.AsyncLineReceived += OnAsyncLine;

        cmbPort.DropDownStyle = ComboBoxStyle.DropDownList;
        cmbBaud.DropDownStyle = ComboBoxStyle.DropDownList;
        cmbTermNetif.DropDownStyle = ComboBoxStyle.DropDownList;
        cmbSendNetif.DropDownStyle = ComboBoxStyle.DropDownList;
        cmbSendSocket.DropDownStyle = ComboBoxStyle.DropDownList;

        LoadBaudRates();
        LoadPorts();

        btnRefresh.Click += (s, e) => LoadPorts();
        btnConnect.Click += BtnConnect_Click;
        btnEnterMenu.Click += BtnEnterMenu_Click;
        btnSend.Click += (s, e) => RunTerminalInput();
        btnStop.Click += (s, e) => _pingCts?.Cancel();
        btnClear.Click += (s, e) => { txtRxLog.Clear(); txtTxLog.Clear(); txtTermOut.Clear(); };
        txtInput.KeyDown += TxtInput_KeyDown;
        btnScan.Click += BtnScan_Click;
        btnArpReset.Click += BtnArpReset_Click;
        btnUdpSend.Click += BtnUdpSend_Click;
        cmbSendNetif.SelectedIndexChanged += (s, e) => FillSocketCombo();
        FormClosing += (s, e) => { _pingCts?.Cancel(); _serial.Disconnect(); };
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
            _pingCts?.Cancel();
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
            _ = ForceRefreshAsync();   // ilk listeyi beklemeden çek
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
        cmbTermNetif.Enabled = connected;
        btnScan.Enabled = connected;
        btnArpReset.Enabled = connected;
        btnUdpSend.Enabled = connected;
        grpSend.Enabled = connected;
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
            _netifs = new List<NetifInfo>();
            _lastSignature = "";
            FillNetifCombos();
        }
    }

    // ------------------------------------------------------------------
    //  Netif kartları + oto-yenileme
    // ------------------------------------------------------------------

    private async Task PollTickAsync()
    {
        if (DateTime.Now < _pausePollUntil) return;   // menü molası
        if (_pingCts != null) return;                 // ping döngüsü sürerken karışma
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
            _netifs = netifs;
            RebuildCards(netifs);
            FillNetifCombos();
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
            card.SocketAddRequested += Card_SocketAddRequested;
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

    /// <summary>Netif combobox'larını tazeler, mümkünse önceki seçimi korur.</summary>
    private void FillNetifCombos()
    {
        FillOneNetifCombo(cmbTermNetif);
        FillOneNetifCombo(cmbSendNetif);
        FillSocketCombo();
    }

    private void FillOneNetifCombo(ComboBox cmb)
    {
        int previous = (cmb.SelectedItem as NetifItem)?.Index ?? -1;

        cmb.BeginUpdate();
        cmb.Items.Clear();
        foreach (NetifInfo n in _netifs)
            cmb.Items.Add(new NetifItem(n));
        cmb.EndUpdate();

        if (cmb.Items.Count == 0) return;

        int restore = 0;
        for (int i = 0; i < cmb.Items.Count; i++)
        {
            if (((NetifItem)cmb.Items[i]!).Index == previous) { restore = i; break; }
        }
        cmb.SelectedIndex = restore;
    }

    private void FillSocketCombo()
    {
        NetifInfo? n = SelectedNetif(cmbSendNetif);
        int previous = (cmbSendSocket.SelectedItem as SocketItem)?.Nth ?? -1;

        cmbSendSocket.BeginUpdate();
        cmbSendSocket.Items.Clear();
        if (n != null)
        {
            foreach (SocketInfo s in n.Sockets)
                cmbSendSocket.Items.Add(new SocketItem(s));
        }
        cmbSendSocket.EndUpdate();

        if (cmbSendSocket.Items.Count == 0) return;

        int restore = 0;
        for (int i = 0; i < cmbSendSocket.Items.Count; i++)
        {
            if (((SocketItem)cmbSendSocket.Items[i]!).Nth == previous) { restore = i; break; }
        }
        cmbSendSocket.SelectedIndex = restore;
    }

    private NetifInfo? SelectedNetif(ComboBox cmb) => (cmb.SelectedItem as NetifItem)?.Netif;

    // ComboBox'ta netif/soket göstermek için küçük sarmalayıcılar (ToString ekranda görünür)
    private sealed record NetifItem(NetifInfo Netif)
    {
        public int Index => Netif.Index;
        public override string ToString() => $"{Netif.Index}.netif — {Netif.Ip}";
    }

    private sealed record SocketItem(SocketInfo Sock)
    {
        public int Nth => Sock.Nth;
        public override string ToString() => $"{Sock.Nth}. soket — port {Sock.Port}";
    }

    // ------------------------------------------------------------------
    //  Kart olayları (netif sil/ekle, soket sil/ekle)
    // ------------------------------------------------------------------

    private async void Card_DeleteRequested(NetifCard card)
    {
        NetifInfo n = card.Netif;
        DialogResult answer = MessageBox.Show(
            $"{n.Index}.netif ({n.Ip}) silinsin mi?\nÜzerindeki tüm soketler de silinir.",
            "Netif Sil", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
        if (answer != DialogResult.Yes) return;

        await RunCommandAsync($"NETIF DEL {n.Index}", "Netif Sil", status =>
            status.Contains("remove_failed")
                ? "Bu netif silinemedi (fiziksel netif silinemez)."
                : $"Hata: {status}");
    }

    private async void Card_SocketDeleteRequested(NetifCard card, SocketInfo sock)
    {
        await RunCommandAsync($"SOCK CLOSE {card.Netif.Index} {sock.Nth}", "Soket Sil");
    }

    private async void Card_SocketAddRequested(NetifCard card)
    {
        if (!InputDialog.Ask($"{card.Netif.Index}.netif — Yeni Soket", "Port numarası:", "5000", out string portText))
            return;

        if (!int.TryParse(portText, out int port) || port < 1 || port > 65535)
        {
            MessageBox.Show("Port 1–65535 arasında bir sayı olmalı.", "Soket Aç");
            return;
        }

        await RunCommandAsync($"SOCK OPEN {card.Netif.Index} {port}", "Soket Aç");
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

        if (await RunCommandAsync($"NETIF ADD {ip} {mask} {gw}", "Netif Ekle"))
            _addCard.Collapse();
    }

    /// <summary>Komutu gönder, hata varsa kullanıcıya göster, sonra listeyi tazele.</summary>
    private async Task<bool> RunCommandAsync(string command, string title, Func<string, string>? errorText = null)
    {
        bool ok = false;
        try
        {
            ProtocolResponse resp = await _serial.SendCommandAsync(command);
            ok = resp.Ok;
            if (!ok)
            {
                string msg = errorText != null ? errorText(resp.Status) : $"Hata: {resp.Status}";
                MessageBox.Show(msg, title);
            }
        }
        catch (Exception ex)
        {
            MessageBox.Show($"Hata: {ex.Message}", title);
        }

        await ForceRefreshAsync();
        return ok;
    }

    // ------------------------------------------------------------------
    //  İşlemler: ağ taraması / ARP sıfırlama
    // ------------------------------------------------------------------

    private async void BtnScan_Click(object? sender, EventArgs e)
    {
        NetifInfo? n = SelectedNetif(cmbTermNetif);
        if (n == null)
        {
            MessageBox.Show("Önce Terminal bölümünden bir netif seç.", "Ağ Taraması");
            return;
        }

        btnScan.Enabled = false;
        TermLine($"[{n.Index}.netif] ağ taraması başladı, ~15 sn sürebilir...");
        try
        {
            ProtocolResponse resp = await _serial.SendCommandAsync($"SCAN {n.Index}", ScanTimeoutMs);
            if (!resp.Ok)
            {
                TermLine($"Tarama hatası: {resp.Status}");
                return;
            }

            List<ScanHost> hosts = Protocol.ParseScan(resp);
            if (hosts.Count == 0)
            {
                TermLine("Tarama bitti: aktif cihaz bulunamadı.");
            }
            else
            {
                TermLine($"Tarama bitti — {hosts.Count} cihaz:");
                foreach (ScanHost h in hosts)
                    TermLine($"   {h.Ip}   MAC {h.Mac}");
            }
        }
        catch (Exception ex)
        {
            TermLine($"Tarama hatası: {ex.Message}");
        }
        finally
        {
            btnScan.Enabled = _serial.IsOpen;
        }
    }

    private async void BtnArpReset_Click(object? sender, EventArgs e)
    {
        btnArpReset.Enabled = false;
        try
        {
            ProtocolResponse resp = await _serial.SendCommandAsync("ARPRESET");
            TermLine(resp.Ok ? "ARP tabloları sıfırlandı." : $"ARP sıfırlama hatası: {resp.Status}");
        }
        catch (Exception ex)
        {
            TermLine($"ARP sıfırlama hatası: {ex.Message}");
        }
        finally
        {
            btnArpReset.Enabled = _serial.IsOpen;
        }
    }

    // ------------------------------------------------------------------
    //  UDP gönderimi
    // ------------------------------------------------------------------

    private async void BtnUdpSend_Click(object? sender, EventArgs e)
    {
        NetifInfo? n = SelectedNetif(cmbSendNetif);
        if (n == null)
        {
            MessageBox.Show("Gönderim için bir netif seç.", "UDP Gönder");
            return;
        }
        if (cmbSendSocket.SelectedItem is not SocketItem sock)
        {
            MessageBox.Show("Bu netif'te soket yok. Önce kart üzerinden + ile soket aç.", "UDP Gönder");
            return;
        }
        if (!IPAddress.TryParse(txtDstIp.Text.Trim(), out _))
        {
            MessageBox.Show("Geçersiz hedef IP.", "UDP Gönder");
            return;
        }
        if (!int.TryParse(txtDstPort.Text.Trim(), out int dstPort) || dstPort < 1 || dstPort > 65535)
        {
            MessageBox.Show("Hedef port 1–65535 arasında olmalı.", "UDP Gönder");
            return;
        }
        string data = txtSendData.Text;
        if (data.Length == 0)
        {
            MessageBox.Show("Gönderilecek veri boş.", "UDP Gönder");
            return;
        }

        string ip = txtDstIp.Text.Trim();
        try
        {
            ProtocolResponse resp = await _serial.SendCommandAsync(
                $"UDP SEND {n.Index} {sock.Nth} {ip} {dstPort} {data}");
            TermLine(resp.Ok
                ? $"UDP gönderildi -> {ip}:{dstPort} ({data.Length} bayt)"
                : $"UDP gönderilemedi: {resp.Status}");
        }
        catch (Exception ex)
        {
            TermLine($"UDP gönderilemedi: {ex.Message}");
        }
    }

    // ------------------------------------------------------------------
    //  Terminal: ping komutları + ham giriş
    // ------------------------------------------------------------------

    private void TxtInput_KeyDown(object? sender, KeyEventArgs e)
    {
        if (e.KeyCode == Keys.Enter)
        {
            RunTerminalInput();
            e.SuppressKeyPress = true;
        }
    }

    private async void RunTerminalInput()
    {
        string text = txtInput.Text.Trim();
        if (text.Length == 0) return;
        txtInput.Clear();
        txtInput.Focus();

        // "ping <IP> [-t]" -> protokol üzerinden, uygulama tarafında döngü
        if (text.StartsWith("ping ", StringComparison.OrdinalIgnoreCase))
        {
            string rest = text.Substring(5).Trim();
            bool continuous = rest.EndsWith("-t", StringComparison.OrdinalIgnoreCase);
            if (continuous) rest = rest.Substring(0, rest.Length - 2).Trim();

            if (!IPAddress.TryParse(rest, out _))
            {
                TermLine($"Geçersiz IP: {rest}");
                return;
            }

            NetifInfo? n = SelectedNetif(cmbTermNetif);
            if (n == null)
            {
                TermLine("Önce bir netif seç.");
                return;
            }

            await RunPingAsync(n.Index, rest, continuous);
            return;
        }

        // tanınmayan giriş: ham olarak karta yolla (insan menüsü için)
        _serial.SendText(text + "\r");
        _pausePollUntil = DateTime.Now.AddSeconds(15);
    }

    private async Task RunPingAsync(int netifIndex, string ip, bool continuous)
    {
        _pingCts = new CancellationTokenSource();
        CancellationToken token = _pingCts.Token;

        btnStop.Enabled = true;
        btnSend.Enabled = false;
        txtInput.Enabled = false;

        int sent = 0, received = 0;
        int rttMin = int.MaxValue, rttMax = 0, rttSum = 0;

        TermLine(continuous
            ? $"{ip} pingleniyor ({netifIndex}.netif) — durdurmak için Durdur:"
            : $"{ip} pingleniyor ({netifIndex}.netif):");

        try
        {
            for (int i = 0; continuous || i < PingAttempts; i++)
            {
                if (token.IsCancellationRequested) break;

                sent++;
                ProtocolResponse resp = await _serial.SendCommandAsync($"PING {netifIndex} {ip}", 5000);
                if (!resp.Ok)
                {
                    TermLine($"Ping hatası: {resp.Status}");
                    break;
                }

                PingReply reply = Protocol.ParsePing(resp);
                if (reply.Success)
                {
                    received++;
                    rttSum += reply.RttMs;
                    if (reply.RttMs < rttMin) rttMin = reply.RttMs;
                    if (reply.RttMs > rttMax) rttMax = reply.RttMs;
                    TermLine($"   {reply.From} yanıt: seq={reply.Seq} bytes={reply.Bytes} " +
                             $"süre={reply.RttMs}ms TTL={reply.Ttl}");
                }
                else
                {
                    TermLine("   İstek zaman aşımına uğradı");
                }

                if (continuous)
                {
                    try { await Task.Delay(1000, token); }
                    catch (TaskCanceledException) { break; }
                }
            }
        }
        catch (Exception ex)
        {
            TermLine($"Ping hatası: {ex.Message}");
        }
        finally
        {
            int loss = sent > 0 ? (sent - received) * 100 / sent : 0;
            TermLine($"--- {ip} istatistik: {sent} gönderildi, {received} alındı, %{loss} kayıp ---");
            if (received > 0)
                TermLine($"    rtt min/ort/max = {rttMin}/{rttSum / received}/{rttMax} ms");

            _pingCts?.Dispose();
            _pingCts = null;
            btnStop.Enabled = false;
            btnSend.Enabled = _serial.IsOpen;
            txtInput.Enabled = _serial.IsOpen;
            txtInput.Focus();
        }
    }

    private void BtnEnterMenu_Click(object? sender, EventArgs e)
    {
        _serial.SendText("q");
        // insan menüsü açıkken NETIF LIST baytları menü girişine karışmasın
        _pausePollUntil = DateTime.Now.AddSeconds(60);
        TermLine("Kart menüsü açıldı — çıkmak için menüde bir işlemi tamamla.");
    }

    /// <summary>Karttan gelen istem dışı satırlar (ör. "UDP RX: ...").</summary>
    private void OnAsyncLine(string line)
    {
        if (line.StartsWith("UDP RX:") || line.StartsWith("Data:"))
            TermLine(line);
    }

    private void TermLine(string text)
    {
        txtTermOut.AppendText(text + Environment.NewLine);
    }
}
