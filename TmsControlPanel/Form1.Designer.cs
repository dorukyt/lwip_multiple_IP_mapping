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
            panelTop = new Panel();
            lblStatus = new Label();
            btnClear = new Button();
            btnConnect = new Button();
            btnRefresh = new Button();
            cmbBaud = new ComboBox();
            cmbPort = new ComboBox();
            flpNetifs = new FlowLayoutPanel();
            panelRight = new Panel();
            grpActions = new GroupBox();
            btnArpReset = new Button();
            btnScan = new Button();
            grpTerminal = new GroupBox();
            btnEnterMenu = new Button();
            btnSend = new Button();
            txtInput = new TextBox();
            panelLogs = new Panel();
            splitLogs = new SplitContainer();
            grpRx = new GroupBox();
            txtRxLog = new TextBox();
            grpTx = new GroupBox();
            txtTxLog = new TextBox();
            panelTop.SuspendLayout();
            panelRight.SuspendLayout();
            grpActions.SuspendLayout();
            grpTerminal.SuspendLayout();
            panelLogs.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)splitLogs).BeginInit();
            splitLogs.Panel1.SuspendLayout();
            splitLogs.Panel2.SuspendLayout();
            splitLogs.SuspendLayout();
            grpRx.SuspendLayout();
            grpTx.SuspendLayout();
            SuspendLayout();
            //
            // panelTop
            //
            panelTop.Controls.Add(lblStatus);
            panelTop.Controls.Add(btnClear);
            panelTop.Controls.Add(btnConnect);
            panelTop.Controls.Add(btnRefresh);
            panelTop.Controls.Add(cmbBaud);
            panelTop.Controls.Add(cmbPort);
            panelTop.Dock = DockStyle.Top;
            panelTop.Location = new Point(0, 0);
            panelTop.Name = "panelTop";
            panelTop.Size = new Size(1150, 40);
            panelTop.TabIndex = 0;
            //
            // lblStatus
            //
            lblStatus.AutoSize = true;
            lblStatus.Location = new Point(494, 12);
            lblStatus.Name = "lblStatus";
            lblStatus.Size = new Size(62, 15);
            lblStatus.TabIndex = 5;
            lblStatus.Text = "Bağlı değil";
            //
            // btnClear
            //
            btnClear.Location = new Point(410, 7);
            btnClear.Name = "btnClear";
            btnClear.Size = new Size(72, 25);
            btnClear.TabIndex = 4;
            btnClear.Text = "Temizle";
            btnClear.UseVisualStyleBackColor = true;
            //
            // btnConnect
            //
            btnConnect.Location = new Point(320, 7);
            btnConnect.Name = "btnConnect";
            btnConnect.Size = new Size(84, 25);
            btnConnect.TabIndex = 3;
            btnConnect.Text = "Bağlan";
            btnConnect.UseVisualStyleBackColor = true;
            //
            // btnRefresh
            //
            btnRefresh.Location = new Point(242, 7);
            btnRefresh.Name = "btnRefresh";
            btnRefresh.Size = new Size(72, 25);
            btnRefresh.TabIndex = 2;
            btnRefresh.Text = "Yenile";
            btnRefresh.UseVisualStyleBackColor = true;
            //
            // cmbBaud
            //
            cmbBaud.FormattingEnabled = true;
            cmbBaud.Location = new Point(146, 8);
            cmbBaud.Name = "cmbBaud";
            cmbBaud.Size = new Size(90, 23);
            cmbBaud.TabIndex = 1;
            //
            // cmbPort
            //
            cmbPort.FormattingEnabled = true;
            cmbPort.Location = new Point(10, 8);
            cmbPort.Name = "cmbPort";
            cmbPort.Size = new Size(130, 23);
            cmbPort.TabIndex = 0;
            //
            // flpNetifs
            //
            flpNetifs.AutoScroll = true;
            flpNetifs.Dock = DockStyle.Fill;
            flpNetifs.FlowDirection = FlowDirection.TopDown;
            flpNetifs.Location = new Point(0, 40);
            flpNetifs.Name = "flpNetifs";
            flpNetifs.Padding = new Padding(8);
            flpNetifs.Size = new Size(850, 510);
            flpNetifs.TabIndex = 1;
            flpNetifs.WrapContents = false;
            //
            // panelRight
            //
            panelRight.Controls.Add(grpActions);
            panelRight.Controls.Add(grpTerminal);
            panelRight.Dock = DockStyle.Right;
            panelRight.Location = new Point(850, 40);
            panelRight.Name = "panelRight";
            panelRight.Padding = new Padding(8);
            panelRight.Size = new Size(300, 510);
            panelRight.TabIndex = 2;
            //
            // grpActions
            //
            grpActions.Controls.Add(btnArpReset);
            grpActions.Controls.Add(btnScan);
            grpActions.Dock = DockStyle.Top;
            grpActions.Location = new Point(8, 104);
            grpActions.Name = "grpActions";
            grpActions.Size = new Size(284, 66);
            grpActions.TabIndex = 1;
            grpActions.TabStop = false;
            grpActions.Text = "İşlemler";
            //
            // btnArpReset
            //
            btnArpReset.Enabled = false;
            btnArpReset.Location = new Point(146, 26);
            btnArpReset.Name = "btnArpReset";
            btnArpReset.Size = new Size(128, 27);
            btnArpReset.TabIndex = 1;
            btnArpReset.Text = "ARP Sıfırla";
            btnArpReset.UseVisualStyleBackColor = true;
            //
            // btnScan
            //
            btnScan.Enabled = false;
            btnScan.Location = new Point(10, 26);
            btnScan.Name = "btnScan";
            btnScan.Size = new Size(128, 27);
            btnScan.TabIndex = 0;
            btnScan.Text = "Ağ Taraması";
            btnScan.UseVisualStyleBackColor = true;
            //
            // grpTerminal
            //
            grpTerminal.Controls.Add(btnEnterMenu);
            grpTerminal.Controls.Add(btnSend);
            grpTerminal.Controls.Add(txtInput);
            grpTerminal.Dock = DockStyle.Top;
            grpTerminal.Location = new Point(8, 8);
            grpTerminal.Name = "grpTerminal";
            grpTerminal.Size = new Size(284, 96);
            grpTerminal.TabIndex = 0;
            grpTerminal.TabStop = false;
            grpTerminal.Text = "Terminal";
            //
            // btnEnterMenu
            //
            btnEnterMenu.Location = new Point(10, 58);
            btnEnterMenu.Name = "btnEnterMenu";
            btnEnterMenu.Size = new Size(120, 25);
            btnEnterMenu.TabIndex = 2;
            btnEnterMenu.Text = "Menüye Gir (q)";
            btnEnterMenu.UseVisualStyleBackColor = true;
            //
            // btnSend
            //
            btnSend.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            btnSend.Location = new Point(196, 25);
            btnSend.Name = "btnSend";
            btnSend.Size = new Size(80, 25);
            btnSend.TabIndex = 1;
            btnSend.Text = "Gönder";
            btnSend.UseVisualStyleBackColor = true;
            //
            // txtInput
            //
            txtInput.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            txtInput.Location = new Point(10, 26);
            txtInput.Name = "txtInput";
            txtInput.Size = new Size(180, 23);
            txtInput.TabIndex = 0;
            //
            // panelLogs
            //
            panelLogs.Controls.Add(splitLogs);
            panelLogs.Dock = DockStyle.Bottom;
            panelLogs.Location = new Point(0, 550);
            panelLogs.Name = "panelLogs";
            panelLogs.Padding = new Padding(8, 4, 8, 8);
            panelLogs.Size = new Size(1150, 190);
            panelLogs.TabIndex = 3;
            //
            // splitLogs
            //
            splitLogs.Dock = DockStyle.Fill;
            splitLogs.Location = new Point(8, 4);
            splitLogs.Name = "splitLogs";
            //
            // splitLogs.Panel1
            //
            splitLogs.Panel1.Controls.Add(grpRx);
            //
            // splitLogs.Panel2
            //
            splitLogs.Panel2.Controls.Add(grpTx);
            splitLogs.Size = new Size(1134, 178);
            splitLogs.SplitterDistance = 560;
            splitLogs.TabIndex = 0;
            //
            // grpRx
            //
            grpRx.Controls.Add(txtRxLog);
            grpRx.Dock = DockStyle.Fill;
            grpRx.Location = new Point(0, 0);
            grpRx.Name = "grpRx";
            grpRx.Size = new Size(560, 178);
            grpRx.TabIndex = 0;
            grpRx.TabStop = false;
            grpRx.Text = "Gelen (RX)";
            //
            // txtRxLog
            //
            txtRxLog.BackColor = Color.White;
            txtRxLog.Dock = DockStyle.Fill;
            txtRxLog.Font = new Font("Consolas", 9F);
            txtRxLog.Location = new Point(3, 19);
            txtRxLog.Multiline = true;
            txtRxLog.Name = "txtRxLog";
            txtRxLog.ReadOnly = true;
            txtRxLog.ScrollBars = ScrollBars.Vertical;
            txtRxLog.Size = new Size(554, 156);
            txtRxLog.TabIndex = 0;
            //
            // grpTx
            //
            grpTx.Controls.Add(txtTxLog);
            grpTx.Dock = DockStyle.Fill;
            grpTx.Location = new Point(0, 0);
            grpTx.Name = "grpTx";
            grpTx.Size = new Size(570, 178);
            grpTx.TabIndex = 0;
            grpTx.TabStop = false;
            grpTx.Text = "Giden (TX)";
            //
            // txtTxLog
            //
            txtTxLog.BackColor = Color.White;
            txtTxLog.Dock = DockStyle.Fill;
            txtTxLog.Font = new Font("Consolas", 9F);
            txtTxLog.Location = new Point(3, 19);
            txtTxLog.Multiline = true;
            txtTxLog.Name = "txtTxLog";
            txtTxLog.ReadOnly = true;
            txtTxLog.ScrollBars = ScrollBars.Vertical;
            txtTxLog.Size = new Size(564, 156);
            txtTxLog.TabIndex = 0;
            //
            // Form1
            //
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1150, 740);
            Controls.Add(flpNetifs);
            Controls.Add(panelRight);
            Controls.Add(panelLogs);
            Controls.Add(panelTop);
            MinimumSize = new Size(980, 620);
            Name = "Form1";
            Text = "TMS570 Kontrol Paneli";
            panelTop.ResumeLayout(false);
            panelTop.PerformLayout();
            panelRight.ResumeLayout(false);
            grpActions.ResumeLayout(false);
            grpTerminal.ResumeLayout(false);
            grpTerminal.PerformLayout();
            panelLogs.ResumeLayout(false);
            splitLogs.Panel1.ResumeLayout(false);
            splitLogs.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)splitLogs).EndInit();
            splitLogs.ResumeLayout(false);
            grpRx.ResumeLayout(false);
            grpRx.PerformLayout();
            grpTx.ResumeLayout(false);
            grpTx.PerformLayout();
            ResumeLayout(false);
        }

        #endregion

        private Panel panelTop;
        private Label lblStatus;
        private Button btnClear;
        private Button btnConnect;
        private Button btnRefresh;
        private ComboBox cmbBaud;
        private ComboBox cmbPort;
        private FlowLayoutPanel flpNetifs;
        private Panel panelRight;
        private GroupBox grpActions;
        private Button btnArpReset;
        private Button btnScan;
        private GroupBox grpTerminal;
        private Button btnEnterMenu;
        private Button btnSend;
        private TextBox txtInput;
        private Panel panelLogs;
        private SplitContainer splitLogs;
        private GroupBox grpRx;
        private TextBox txtRxLog;
        private GroupBox grpTx;
        private TextBox txtTxLog;
    }
}
