using System.IO.Ports;
using System.Text;

namespace TmsControlPanel;

/// <summary>
/// Bir protokol komutunun (#BEGIN ... #END) toplanmış yanıtı.
/// </summary>
public class ProtocolResponse
{
    public string Name { get; init; } = "";           // ör. "NETIFLIST"
    public string Status { get; init; } = "";          // "OK" ya da "ERR:sebep"
    public List<string> DataLines { get; init; } = new(); // aradaki #NETIF / #SOCK satırları

    public bool Ok => Status == "OK";
}

/// <summary>
/// Seri portun tek sahibi. İki tür trafik yönetir:
///  1) Komut/yanıt: SendCommandAsync("NETIF LIST") -> #BEGIN...#END bloğunu bekler.
///  2) Ham trafik: terminal girişi (SendText) ve karttan gelen her şey (RawReceived).
/// Tüm event'ler UI thread'ine post edilir; form tarafında Invoke gerekmez.
/// </summary>
public class SerialManager
{
    private SerialPort? _port;
    private readonly SynchronizationContext _ui;
    private readonly StringBuilder _lineBuf = new();          // satır birleştirme tamponu
    private readonly SemaphoreSlim _cmdLock = new(1, 1);      // aynı anda tek komut

    // O an beklenen komutun yanıtı (yoksa null)
    private TaskCompletionSource<ProtocolResponse>? _pendingTcs;
    private string? _collectName;                              // toplanan bloğun adı
    private List<string> _collectLines = new();
    private volatile bool _quiet;                              // sessiz komut: loglara yazma

    /// <summary>Karttan gelen ham metin parçası (RX log için).</summary>
    public event Action<string>? RawReceived;
    /// <summary>Karta gönderilen metin (TX log için).</summary>
    public event Action<string>? TextSent;
    /// <summary>'#' ile başlamayan tam satır (ör. "UDP RX: ..." olayları).</summary>
    public event Action<string>? AsyncLineReceived;

    public bool IsOpen => _port != null && _port.IsOpen;

    public SerialManager()
    {
        // UI thread'inde oluşturulmalı: event'leri geri post edeceğimiz bağlamı yakala
        _ui = SynchronizationContext.Current ?? new SynchronizationContext();
    }

    public void Connect(string portName, int baud)
    {
        _port = new SerialPort(portName, baud, Parity.None, 8, StopBits.One);
        _port.DataReceived += Port_DataReceived;
        _port.Open();
    }

    public void Disconnect()
    {
        SerialPort? p = _port;
        _port = null;
        if (p != null)
        {
            try { p.Close(); p.Dispose(); } catch { /* kapanırken oluşan hatalar önemsiz */ }
        }
        _pendingTcs?.TrySetCanceled();
        _pendingTcs = null;
        _collectName = null;
        _lineBuf.Clear();
    }

    /// <summary>Ham metin gönder (terminal). '\r' eklemek çağırana ait.</summary>
    public void SendText(string text) => SendTextInternal(text, quiet: false);

    private void SendTextInternal(string text, bool quiet)
    {
        SerialPort? p = _port;
        if (p == null || !p.IsOpen) return;
        p.Write(text);
        if (!quiet)
            Post(() => TextSent?.Invoke(text));
    }

    /// <summary>
    /// Bir protokol komutu gönderir ve #BEGIN/#END çerçeveli yanıtını bekler.
    /// quiet=true ise trafik RX/TX loglarına yazılmaz (oto-yenileme için).
    /// </summary>
    public async Task<ProtocolResponse> SendCommandAsync(string command, int timeoutMs = 4000, bool quiet = false)
    {
        if (!IsOpen) throw new InvalidOperationException("Seri port açık değil.");

        await _cmdLock.WaitAsync();   // önceki komut bitmeden yenisi gitmesin
        try
        {
            var tcs = new TaskCompletionSource<ProtocolResponse>(
                TaskCreationOptions.RunContinuationsAsynchronously);
            _pendingTcs = tcs;
            _collectName = null;
            _quiet = quiet;

            SendTextInternal(command + "\r", quiet);

            using var timeoutCts = new CancellationTokenSource(timeoutMs);
            using (timeoutCts.Token.Register(
                       () => tcs.TrySetException(new TimeoutException($"'{command}' için yanıt gelmedi"))))
            {
                return await tcs.Task;
            }
        }
        finally
        {
            _pendingTcs = null;
            _quiet = false;
            _cmdLock.Release();
        }
    }

    // Seri porttan veri geldi (ARKA PLAN thread'i)
    private void Port_DataReceived(object sender, SerialDataReceivedEventArgs e)
    {
        try
        {
            var p = (SerialPort)sender;
            string chunk = p.ReadExisting();
            if (chunk.Length == 0) return;

            if (!_quiet)
                Post(() => RawReceived?.Invoke(chunk));

            // gelen parçayı satırlara böl (satırlar \r\n ile biter)
            foreach (char c in chunk)
            {
                if (c == '\n')
                {
                    string line = _lineBuf.ToString().TrimEnd('\r');
                    _lineBuf.Clear();
                    ProcessLine(line);
                }
                else
                {
                    _lineBuf.Append(c);
                }
            }
        }
        catch { /* port kapanırken gelebilecek istisnalar */ }
    }

    // Tam bir satırı sınıflandır: protokol mü, serbest metin mi?
    private void ProcessLine(string line)
    {
        if (!line.StartsWith("#"))
        {
            if (line.Length > 0)
                Post(() => AsyncLineReceived?.Invoke(line));
            return;
        }

        if (line.StartsWith("#BEGIN "))
        {
            _collectName = line.Substring("#BEGIN ".Length).Trim();
            _collectLines = new List<string>();
            return;
        }

        if (line.StartsWith("#END "))
        {
            // "#END NETIFLIST OK"  ya da  "#END NETIFADD ERR:bad_args"
            string rest = line.Substring("#END ".Length).Trim();
            int sp = rest.IndexOf(' ');
            string name = sp < 0 ? rest : rest.Substring(0, sp);
            string status = sp < 0 ? "" : rest.Substring(sp + 1).Trim();

            var resp = new ProtocolResponse { Name = name, Status = status, DataLines = _collectLines };
            _collectName = null;
            _pendingTcs?.TrySetResult(resp);
            return;
        }

        if (line.StartsWith("#ERR"))
        {
            // çerçevesiz hata (ör. unknown_command)
            var resp = new ProtocolResponse { Status = "ERR:" + line.Substring(4).Trim() };
            _pendingTcs?.TrySetResult(resp);
            return;
        }

        // #NETIF / #SOCK gibi veri satırları
        if (_collectName != null)
            _collectLines.Add(line);
    }

    // Bir işi UI thread'ine gönder
    private void Post(Action action) => _ui.Post(_ => action(), null);
}
