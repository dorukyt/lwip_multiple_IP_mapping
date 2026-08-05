namespace TmsControlPanel
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            btnConnect = new Button();
            cmbPort = new ComboBox();
            cmbBaud = new ComboBox();
            btnRefresh = new Button();
            btnEnterMenu = new Button();
            lblStatus = new Label();
            txtLog = new TextBox();
            SuspendLayout();
            // 
            // btnConnect
            // 
            btnConnect.Location = new Point(645, 140);
            btnConnect.Name = "btnConnect";
            btnConnect.Size = new Size(103, 23);
            btnConnect.TabIndex = 0;
            btnConnect.Text = "Bağlan";
            btnConnect.UseVisualStyleBackColor = true;
            // 
            // cmbPort
            // 
            cmbPort.FormattingEnabled = true;
            cmbPort.Location = new Point(645, 45);
            cmbPort.Name = "cmbPort";
            cmbPort.Size = new Size(121, 23);
            cmbPort.TabIndex = 1;
            // 
            // cmbBaud
            // 
            cmbBaud.FormattingEnabled = true;
            cmbBaud.Location = new Point(645, 84);
            cmbBaud.Name = "cmbBaud";
            cmbBaud.Size = new Size(121, 23);
            cmbBaud.TabIndex = 2;
            // 
            // btnRefresh
            // 
            btnRefresh.Location = new Point(645, 169);
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(103, 23);
            btnRefresh.TabIndex = 3;
            btnRefresh.Text = "Yenile";
            btnRefresh.UseVisualStyleBackColor = true;
            // 
            // btnEnterMenu
            // 
            btnEnterMenu.Location = new Point(645, 198);
            btnEnterMenu.Name = "btnEnterMenu";
            btnEnterMenu.Size = new Size(103, 23);
            btnEnterMenu.TabIndex = 4;
            btnEnterMenu.Text = "Menüye Gir (q)";
            btnEnterMenu.UseVisualStyleBackColor = true;
            // 
            // lblStatus
            // 
            lblStatus.AutoSize = true;
            lblStatus.Location = new Point(704, 23);
            lblStatus.Name = "lblStatus";
            lblStatus.Size = new Size(62, 15);
            lblStatus.TabIndex = 5;
            lblStatus.Text = "Bağlı değil";
            // 
            // txtLog
            // 
            txtLog.Location = new Point(218, 45);
            txtLog.Multiline = true;
            txtLog.Name = "txtLog";
            txtLog.Size = new Size(361, 278);
            txtLog.TabIndex = 6;
            // 
            // Form1
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(800, 450);
            Controls.Add(txtLog);
            Controls.Add(lblStatus);
            Controls.Add(btnEnterMenu);
            Controls.Add(btnRefresh);
            Controls.Add(cmbBaud);
            Controls.Add(cmbPort);
            Controls.Add(btnConnect);
            Name = "Form1";
            Text = "Form1";
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private Button btnConnect;
        private ComboBox cmbPort;
        private ComboBox cmbBaud;
        private Button btnRefresh;
        private Button btnEnterMenu;
        private Label lblStatus;
        private TextBox txtLog;
    }
}
