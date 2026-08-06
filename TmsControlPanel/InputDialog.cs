namespace TmsControlPanel;

/// <summary>
/// Tek satırlık değer soran küçük diyalog (ör. "Port numarası").
/// Kullanım:  if (InputDialog.Ask("Yeni Soket", "Port:", "5000", out string v)) { ... }
/// </summary>
public class InputDialog : Form
{
    private readonly TextBox _txt = new();

    private InputDialog(string title, string prompt, string defaultValue)
    {
        Text = title;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        StartPosition = FormStartPosition.CenterParent;
        MinimizeBox = false;
        MaximizeBox = false;
        ClientSize = new Size(300, 118);

        Controls.Add(new Label { Text = prompt, Location = new Point(12, 16), AutoSize = true });

        _txt.Location = new Point(12, 40);
        _txt.Width = 276;
        _txt.Text = defaultValue;
        _txt.SelectAll();
        Controls.Add(_txt);

        var btnOk = new Button
        {
            Text = "Tamam",
            DialogResult = DialogResult.OK,
            Location = new Point(132, 76),
            Width = 75
        };
        var btnCancel = new Button
        {
            Text = "Vazgeç",
            DialogResult = DialogResult.Cancel,
            Location = new Point(213, 76),
            Width = 75
        };
        Controls.Add(btnOk);
        Controls.Add(btnCancel);

        AcceptButton = btnOk;       // Enter -> Tamam
        CancelButton = btnCancel;   // Esc   -> Vazgeç
    }

    public static bool Ask(string title, string prompt, string defaultValue, out string value)
    {
        using var dlg = new InputDialog(title, prompt, defaultValue);
        if (dlg.ShowDialog() == DialogResult.OK)
        {
            value = dlg._txt.Text.Trim();
            return value.Length > 0;
        }
        value = "";
        return false;
    }
}
