namespace TmsControlPanel;

/// <summary>
/// Netif listesinin sonundaki "+" kartı. Tıklanınca IP/Netmask/GW
/// giriş formuna dönüşür; Ekle'ye basılınca AddRequested tetiklenir.
/// </summary>
public class AddNetifCard : Panel
{
    /// <summary>(ip, mask, gw) — doğrulama ve gönderim Form tarafında.</summary>
    public event Action<string, string, string>? AddRequested;

    private const int CollapsedHeight = 46;
    private const int ExpandedHeight = 172;

    private readonly Button _btnPlus;
    private readonly Panel _formPanel;
    private readonly TextBox _txtIp = new();
    private readonly TextBox _txtMask = new();
    private readonly TextBox _txtGw = new();

    public AddNetifCard()
    {
        BorderStyle = BorderStyle.FixedSingle;
        BackColor = Color.White;
        Margin = new Padding(4);
        Height = CollapsedHeight;

        // --- kapalı hal: kocaman "+" ---
        _btnPlus = new Button
        {
            Text = "+",
            Dock = DockStyle.Fill,
            FlatStyle = FlatStyle.Flat,
            Font = new Font("Segoe UI", 14f, FontStyle.Bold),
            ForeColor = Color.SteelBlue,
            Cursor = Cursors.Hand
        };
        _btnPlus.FlatAppearance.BorderSize = 0;
        _btnPlus.Click += (s, e) => Expand();

        // --- açık hal: giriş formu ---
        _formPanel = new Panel { Dock = DockStyle.Fill, Visible = false };

        AddRow("IP:", _txtIp, 12);
        AddRow("Netmask:", _txtMask, 42);
        AddRow("GW:", _txtGw, 72);

        var btnOk = new Button { Text = "Ekle", Location = new Point(80, 108), Width = 80 };
        var btnCancel = new Button { Text = "Vazgeç", Location = new Point(166, 108), Width = 80 };
        btnOk.Click += (s, e) =>
            AddRequested?.Invoke(_txtIp.Text.Trim(), _txtMask.Text.Trim(), _txtGw.Text.Trim());
        btnCancel.Click += (s, e) => Collapse();

        _formPanel.Controls.Add(btnOk);
        _formPanel.Controls.Add(btnCancel);

        Controls.Add(_formPanel);
        Controls.Add(_btnPlus);
    }

    private void AddRow(string label, TextBox box, int y)
    {
        _formPanel.Controls.Add(new Label { Text = label, Location = new Point(12, y + 3), AutoSize = true });
        box.Location = new Point(80, y);
        box.Width = 166;
        _formPanel.Controls.Add(box);
    }

    private void Expand()
    {
        _btnPlus.Visible = false;
        _formPanel.Visible = true;
        Height = ExpandedHeight;
        if (_txtMask.Text.Length == 0) _txtMask.Text = "255.255.255.0";   // makul varsayılan
        _txtIp.Focus();
    }

    public void Collapse()
    {
        _txtIp.Clear();
        _txtMask.Clear();
        _txtGw.Clear();
        _formPanel.Visible = false;
        _btnPlus.Visible = true;
        Height = CollapsedHeight;
    }
}
