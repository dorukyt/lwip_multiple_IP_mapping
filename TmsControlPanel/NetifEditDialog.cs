namespace TmsControlPanel;

/// <summary>
/// Bir netif'in IP / Netmask / GW değerlerini düzenleme diyaloğu.
/// Yalnızca DEĞİŞTİRİLEN alanlar için komut üretilir (ChangedFields).
/// </summary>
public class NetifEditDialog : Form
{
    private readonly NetifInfo _netif;
    private readonly TextBox _txtIp = new();
    private readonly TextBox _txtMask = new();
    private readonly TextBox _txtGw = new();

    /// <summary>(alan, yeni değer) — alan: "IP" | "MASK" | "GW"</summary>
    public List<(string Field, string Value)> ChangedFields { get; } = new();

    private NetifEditDialog(NetifInfo netif)
    {
        _netif = netif;

        Text = $"{netif.Index}.netif düzenle";
        FormBorderStyle = FormBorderStyle.FixedDialog;
        StartPosition = FormStartPosition.CenterParent;
        MinimizeBox = false;
        MaximizeBox = false;
        ClientSize = new Size(330, 168);

        AddRow("IP:", _txtIp, netif.Ip, 16);
        AddRow("Netmask:", _txtMask, netif.Mask, 48);
        AddRow("GW:", _txtGw, netif.Gw, 80);

        var btnOk = new Button { Text = "Kaydet", Location = new Point(162, 122), Width = 75 };
        var btnCancel = new Button
        {
            Text = "Vazgeç",
            DialogResult = DialogResult.Cancel,
            Location = new Point(243, 122),
            Width = 75
        };
        btnOk.Click += BtnOk_Click;

        Controls.Add(btnOk);
        Controls.Add(btnCancel);
        AcceptButton = btnOk;
        CancelButton = btnCancel;
    }

    private void AddRow(string label, TextBox box, string value, int y)
    {
        Controls.Add(new Label { Text = label, Location = new Point(14, y + 3), AutoSize = true });
        box.Location = new Point(90, y);
        box.Width = 228;
        box.Text = value;
        Controls.Add(box);
    }

    private void BtnOk_Click(object? sender, EventArgs e)
    {
        ChangedFields.Clear();
        if (!Collect("IP", _txtIp, _netif.Ip)) return;
        if (!Collect("MASK", _txtMask, _netif.Mask)) return;
        if (!Collect("GW", _txtGw, _netif.Gw)) return;

        DialogResult = DialogResult.OK;
        Close();
    }

    /// <summary>Alan değiştiyse doğrula ve listeye ekle. Geçersizse false döner.</summary>
    private bool Collect(string field, TextBox box, string original)
    {
        string value = box.Text.Trim();
        if (value == original) return true;      // değişmemiş, atla

        if (!System.Net.IPAddress.TryParse(value, out _))
        {
            MessageBox.Show($"Geçersiz değer: {value}", "Netif Düzenle");
            box.Focus();
            box.SelectAll();
            return false;
        }

        ChangedFields.Add((field, value));
        return true;
    }

    /// <summary>Diyaloğu göster; Kaydet'e basıldıysa değişen alanları döndür.</summary>
    public static List<(string Field, string Value)>? Show(NetifInfo netif)
    {
        using var dlg = new NetifEditDialog(netif);
        return dlg.ShowDialog() == DialogResult.OK ? dlg.ChangedFields : null;
    }
}
