namespace TmsControlPanel;

/// <summary>
/// Tek bir netif'i gösteren kart: başlık satırı (no, IP, Mask, GW, sil butonu)
/// + tıklayınca açılan yatay soket şeridi (her sokette sil butonu).
/// </summary>
public class NetifCard : Panel
{
    public NetifInfo Netif { get; }

    /// <summary>Karttaki 🗑 butonuna basıldı.</summary>
    public event Action<NetifCard>? DeleteRequested;
    /// <summary>Bir soket çipindeki ✕ butonuna basıldı.</summary>
    public event Action<NetifCard, SocketInfo>? SocketDeleteRequested;

    private const int HeaderHeight = 62;
    private const int SocketRowHeight = 44;

    private readonly FlowLayoutPanel _socketRow;
    private readonly Label _lblToggle;
    private bool _expanded;

    public NetifCard(NetifInfo netif)
    {
        Netif = netif;

        BorderStyle = BorderStyle.FixedSingle;
        BackColor = Color.White;
        Margin = new Padding(4);
        Height = HeaderHeight;

        // --- soket şeridi (başlangıçta kapalı) ---
        _socketRow = new FlowLayoutPanel
        {
            Dock = DockStyle.Fill,
            FlowDirection = FlowDirection.LeftToRight,
            WrapContents = false,
            Padding = new Padding(8, 4, 8, 4),
            BackColor = Color.FromArgb(245, 245, 248),
            Visible = false
        };
        BuildSocketChips();

        // --- başlık ---
        var header = new Panel { Dock = DockStyle.Top, Height = HeaderHeight, Cursor = Cursors.Hand };

        var lblTitle = new Label
        {
            Text = $"{netif.Index}.netif",
            Font = new Font("Segoe UI", 10f, FontStyle.Bold),
            AutoSize = true,
            Location = new Point(10, 9)
        };
        _lblToggle = new Label
        {
            Text = $"▸ {netif.Sockets.Count} soket",
            AutoSize = true,
            ForeColor = Color.SteelBlue,
            Location = new Point(96, 11)
        };
        var lblInfo = new Label
        {
            Text = $"IP: {netif.Ip}      Mask: {netif.Mask}      GW: {netif.Gw}",
            AutoSize = true,
            ForeColor = Color.FromArgb(60, 60, 60),
            Location = new Point(10, 35)
        };

        // sağa yaslı sil butonu (kendi mini panelinde, kart genişlese de sağda kalır)
        var rightPane = new Panel { Dock = DockStyle.Right, Width = 48 };
        var btnDelete = new Button
        {
            Text = "🗑",
            Width = 36,
            Height = 30,
            Location = new Point(4, 16),
            FlatStyle = FlatStyle.Flat,
            Cursor = Cursors.Hand
        };
        btnDelete.FlatAppearance.BorderSize = 0;
        btnDelete.Click += (s, e) => DeleteRequested?.Invoke(this);
        rightPane.Controls.Add(btnDelete);

        header.Controls.Add(lblTitle);
        header.Controls.Add(_lblToggle);
        header.Controls.Add(lblInfo);
        header.Controls.Add(rightPane);

        // başlığa (ya da üzerindeki yazılara) tıklayınca soketleri aç/kapa
        header.Click += (s, e) => ToggleSockets();
        lblTitle.Click += (s, e) => ToggleSockets();
        lblInfo.Click += (s, e) => ToggleSockets();
        _lblToggle.Click += (s, e) => ToggleSockets();

        // Dock sırası: önce Fill (soketler) sonra Top (başlık) eklenir
        Controls.Add(_socketRow);
        Controls.Add(header);
    }

    private void ToggleSockets()
    {
        _expanded = !_expanded;
        _socketRow.Visible = _expanded;
        Height = HeaderHeight + (_expanded ? SocketRowHeight : 0);
        _lblToggle.Text = $"{(_expanded ? "▾" : "▸")} {Netif.Sockets.Count} soket";
    }

    private void BuildSocketChips()
    {
        _socketRow.Controls.Clear();

        if (Netif.Sockets.Count == 0)
        {
            _socketRow.Controls.Add(new Label
            {
                Text = "soket yok",
                ForeColor = Color.Gray,
                AutoSize = true,
                Margin = new Padding(6, 10, 4, 4)
            });
            return;
        }

        foreach (SocketInfo sock in Netif.Sockets)
        {
            var chip = new Panel
            {
                Width = 118,
                Height = 30,
                BorderStyle = BorderStyle.FixedSingle,
                BackColor = Color.White,
                Margin = new Padding(4, 3, 4, 3)
            };
            chip.Controls.Add(new Label
            {
                Text = $"port {sock.Port}",
                AutoSize = true,
                Location = new Point(6, 7)
            });
            var btnX = new Button
            {
                Text = "✕",
                Width = 24,
                Height = 24,
                Location = new Point(88, 2),
                FlatStyle = FlatStyle.Flat,
                ForeColor = Color.Firebrick,
                Cursor = Cursors.Hand
            };
            btnX.FlatAppearance.BorderSize = 0;
            btnX.Click += (s, e) => SocketDeleteRequested?.Invoke(this, sock);
            chip.Controls.Add(btnX);

            _socketRow.Controls.Add(chip);
        }
    }
}
