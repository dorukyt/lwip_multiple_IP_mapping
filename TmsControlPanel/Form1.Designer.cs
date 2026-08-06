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
            grpTerminal = new GroupBox();
            txtTermOut = new TextBox();
            panelTermTop = new Panel();
            lblTermNetif = new Label();
            cmbTermNetif = new ComboBox();
            btnEnterMenu = new Button();
            txtInput = new TextBox();
            btnSend = new Button();
            lblPingHint = new Label();
            btnStop = new Button();
            grpSend = new GroupBox();
            lblSendNetif = new Label();
            cmbSendNetif = new ComboBox();
            lblSendSocket = new Label();
            cmbSendSocket = new ComboBox();
            lblDstIp = new Label();
            txtDstIp = new TextBox();
            lblDstPort = new Label();
            txtDstPort = new TextBox();
            lblSendData = new Label();
            txtSendData = new TextBox();
            btnUdpSend = new Button();
            grpActions = new GroupBox();
            btnScan = new Button();
            btnArpReset = new Button();
            panelLogs = new Panel();
            splitLogs = new SplitContainer();
            grpRx = new GroupBox();
            txtRxLog = new TextBox();
            grpTx = new GroupBox();
            txtTxLog = new TextBox();
            panelTop.SuspendLayout();
            panelRight.SuspendLayout();
            grpTerminal.SuspendLayout();
            panelTermTop.SuspendLayout();
            grpSend.SuspendLayout();
            grpActions.SuspendLayout();
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
            panelTop.Size = new Size(1220, 40);
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
            flpNetifs.Size = new Size(860, 510);
            flpNetifs.TabIndex = 1;
            flpNetifs.WrapContents = false;
            //
            // panelRight
            //
            panelRight.Controls.Add(grpTerminal);
            panelRight.Controls.Add(grpSend);
            panelRight.Controls.Add(grpActions);
            panelRight.Dock = DockStyle.Right;
            panelRight.Location = new Point(860, 40);
            panelRight.Name = "panelRight";
            panelRight.Padding = new Padding(8);
            panelRight.Size = new Size(360, 510);
            panelRight.TabIndex = 2;
            //
            // grpTerminal
            //
            grpTerminal.Controls.Add(txtTermOut);
            grpTerminal.Controls.Add(panelTermTop);
            grpTerminal.Dock = DockStyle.Fill;
            grpTerminal.Location = new Point(8, 8);
            grpTerminal.Name = "grpTerminal";
            grpTerminal.Size = new Size(344, 228);
            grpTerminal.TabIndex = 0;
            grpTerminal.TabStop = false;
            grpTerminal.Text = "Terminal";
            //
            // txtTermOut
            //
            txtTermOut.BackColor = Color.White;
            txtTermOut.Dock = DockStyle.Fill;
            txtTermOut.Font = new Font("Consolas", 9F);
            txtTermOut.Location = new Point(3, 111);
            txtTermOut.Multiline = true;
            txtTermOut.Name = "txtTermOut";
            txtTermOut.ReadOnly = true;
            txtTermOut.ScrollBars = ScrollBars.Vertical;
            txtTermOut.Size = new Size(338, 114);
            txtTermOut.TabIndex = 1;
            //
            // panelTermTop
            //
            panelTermTop.Controls.Add(lblTermNetif);
            panelTermTop.Controls.Add(cmbTermNetif);
            panelTermTop.Controls.Add(btnEnterMenu);
            panelTermTop.Controls.Add(txtInput);
            panelTermTop.Controls.Add(btnSend);
            panelTermTop.Controls.Add(lblPingHint);
            panelTermTop.Controls.Add(btnStop);
            panelTermTop.Dock = DockStyle.Top;
            panelTermTop.Location = new Point(3, 19);
            panelTermTop.Name = "panelTermTop";
            panelTermTop.Size = new Size(338, 92);
            panelTermTop.TabIndex = 0;
            //
            // lblTermNetif
            //
            lblTermNetif.AutoSize = true;
            lblTermNetif.Location = new Point(7, 9);
            lblTermNetif.Name = "lblTermNetif";
            lblTermNetif.Size = new Size(38, 15);
            lblTermNetif.TabIndex = 0;
            lblTermNetif.Text = "Netif:";
            //
            // cmbTermNetif
            //
            cmbTermNetif.FormattingEnabled = true;
            cmbTermNetif.Location = new Point(50, 6);
            cmbTermNetif.Name = "cmbTermNetif";
            cmbTermNetif.Size = new Size(184, 23);
            cmbTermNetif.TabIndex = 1;
            //
            // btnEnterMenu
            //
            btnEnterMenu.Location = new Point(240, 5);
            btnEnterMenu.Name = "btnEnterMenu";
            btnEnterMenu.Size = new Size(90, 25);
            btnEnterMenu.TabIndex = 2;
            btnEnterMenu.Text = "Menü (q)";
            btnEnterMenu.UseVisualStyleBackColor = true;
            //
            // txtInput
            //
            txtInput.Location = new Point(7, 36);
            txtInput.Name = "txtInput";
            txtInput.Size = new Size(227, 23);
            txtInput.TabIndex = 3;
            //
            // btnSend
            //
            btnSend.Location = new Point(240, 35);
            btnSend.Name = "btnSend";
            btnSend.Size = new Size(90, 25);
            btnSend.TabIndex = 4;
            btnSend.Text = "Gönder";
            btnSend.UseVisualStyleBackColor = true;
            //
            // lblPingHint
            //
            lblPingHint.AutoSize = true;
            lblPingHint.ForeColor = Color.Gray;
            lblPingHint.Location = new Point(7, 68);
            lblPingHint.Name = "lblPingHint";
            lblPingHint.Size = new Size(160, 15);
            lblPingHint.TabIndex = 5;
            lblPingHint.Text = "ping <IP>   |   ping <IP> -t";
            //
            // btnStop
            //
            btnStop.Enabled = false;
            btnStop.Location = new Point(240, 64);
            btnStop.Name = "btnStop";
            btnStop.Size = new Size(90, 25);
            btnStop.TabIndex = 6;
            btnStop.Text = "Durdur";
            btnStop.UseVisualStyleBackColor = true;
            //
            // grpSend
            //
            grpSend.Controls.Add(lblSendNetif);
            grpSend.Controls.Add(cmbSendNetif);
            grpSend.Controls.Add(lblSendSocket);
            grpSend.Controls.Add(cmbSendSocket);
            grpSend.Controls.Add(lblDstIp);
            grpSend.Controls.Add(txtDstIp);
            grpSend.Controls.Add(lblDstPort);
            grpSend.Controls.Add(txtDstPort);
            grpSend.Controls.Add(lblSendData);
            grpSend.Controls.Add(txtSendData);
            grpSend.Controls.Add(btnUdpSend);
            grpSend.Dock = DockStyle.Bottom;
            grpSend.Location = new Point(8, 236);
            grpSend.Name = "grpSend";
            grpSend.Size = new Size(344, 200);
            grpSend.TabIndex = 1;
            grpSend.TabStop = false;
            grpSend.Text = "UDP Gönder";
            //
            // lblSendNetif
            //
            lblSendNetif.AutoSize = true;
            lblSendNetif.Location = new Point(10, 28);
            lblSendNetif.Name = "lblSendNetif";
            lblSendNetif.Size = new Size(38, 15);
            lblSendNetif.TabIndex = 0;
            lblSendNetif.Text = "Netif:";
            //
            // cmbSendNetif
            //
            cmbSendNetif.FormattingEnabled = true;
            cmbSendNetif.Location = new Point(76, 25);
            cmbSendNetif.Name = "cmbSendNetif";
            cmbSendNetif.Size = new Size(254, 23);
            cmbSendNetif.TabIndex = 1;
            //
            // lblSendSocket
            //
            lblSendSocket.AutoSize = true;
            lblSendSocket.Location = new Point(10, 56);
            lblSendSocket.Name = "lblSendSocket";
            lblSendSocket.Size = new Size(40, 15);
            lblSendSocket.TabIndex = 2;
            lblSendSocket.Text = "Soket:";
            //
            // cmbSendSocket
            //
            cmbSendSocket.FormattingEnabled = true;
            cmbSendSocket.Location = new Point(76, 53);
            cmbSendSocket.Name = "cmbSendSocket";
            cmbSendSocket.Size = new Size(254, 23);
            cmbSendSocket.TabIndex = 3;
            //
            // lblDstIp
            //
            lblDstIp.AutoSize = true;
            lblDstIp.Location = new Point(10, 84);
            lblDstIp.Name = "lblDstIp";
            lblDstIp.Size = new Size(58, 15);
            lblDstIp.TabIndex = 4;
            lblDstIp.Text = "Hedef IP:";
            //
            // txtDstIp
            //
            txtDstIp.Location = new Point(76, 81);
            txtDstIp.Name = "txtDstIp";
            txtDstIp.Size = new Size(254, 23);
            txtDstIp.TabIndex = 5;
            //
            // lblDstPort
            //
            lblDstPort.AutoSize = true;
            lblDstPort.Location = new Point(10, 112);
            lblDstPort.Name = "lblDstPort";
            lblDstPort.Size = new Size(32, 15);
            lblDstPort.TabIndex = 6;
            lblDstPort.Text = "Port:";
            //
            // txtDstPort
            //
            txtDstPort.Location = new Point(76, 109);
            txtDstPort.Name = "txtDstPort";
            txtDstPort.Size = new Size(254, 23);
            txtDstPort.TabIndex = 7;
            //
            // lblSendData
            //
            lblSendData.AutoSize = true;
            lblSendData.Location = new Point(10, 140);
            lblSendData.Name = "lblSendData";
            lblSendData.Size = new Size(34, 15);
            lblSendData.TabIndex = 8;
            lblSendData.Text = "Veri:";
            //
            // txtSendData
            //
            txtSendData.Location = new Point(76, 137);
            txtSendData.Name = "txtSendData";
            txtSendData.Size = new Size(254, 23);
            txtSendData.TabIndex = 9;
            //
            // btnUdpSend
            //
            btnUdpSend.Location = new Point(230, 166);
            btnUdpSend.Name = "btnUdpSend";
            btnUdpSend.Size = new Size(100, 26);
            btnUdpSend.TabIndex = 10;
            btnUdpSend.Text = "Gönder";
            btnUdpSend.UseVisualStyleBackColor = true;
            //
            // grpActions
            //
            grpActions.Controls.Add(btnScan);
            grpActions.Controls.Add(btnArpReset);
            grpActions.Dock = DockStyle.Bottom;
            grpActions.Location = new Point(8, 436);
            grpActions.Name = "grpActions";
            grpActions.Size = new Size(344, 66);
            grpActions.TabIndex = 2;
            grpActions.TabStop = false;
            grpActions.Text = "İşlemler";
            //
            // btnScan
            //
            btnScan.Location = new Point(10, 26);
            btnScan.Name = "btnScan";
            btnScan.Size = new Size(158, 27);
            btnScan.TabIndex = 0;
            btnScan.Text = "Ağ Taraması";
            btnScan.UseVisualStyleBackColor = true;
            //
            // btnArpReset
            //
            btnArpReset.Location = new Point(174, 26);
            btnArpReset.Name = "btnArpReset";
            btnArpReset.Size = new Size(158, 27);
            btnArpReset.TabIndex = 1;
            btnArpReset.Text = "ARP Sıfırla";
            btnArpReset.UseVisualStyleBackColor = true;
            //
            // panelLogs
            //
            panelLogs.Controls.Add(splitLogs);
            panelLogs.Dock = DockStyle.Bottom;
            panelLogs.Location = new Point(0, 550);
            panelLogs.Name = "panelLogs";
            panelLogs.Padding = new Padding(8, 4, 8, 8);
            panelLogs.Size = new Size(1220, 190);
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
            splitLogs.Size = new Size(1204, 178);
            splitLogs.SplitterDistance = 600;
            splitLogs.TabIndex = 0;
            //
            // grpRx
            //
            grpRx.Controls.Add(txtRxLog);
            grpRx.Dock = DockStyle.Fill;
            grpRx.Location = new Point(0, 0);
            grpRx.Name = "grpRx";
            grpRx.Size = new Size(600, 178);
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
            txtRxLog.Size = new Size(594, 156);
            txtRxLog.TabIndex = 0;
            //
            // grpTx
            //
            grpTx.Controls.Add(txtTxLog);
            grpTx.Dock = DockStyle.Fill;
            grpTx.Location = new Point(0, 0);
            grpTx.Name = "grpTx";
            grpTx.Size = new Size(600, 178);
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
            txtTxLog.Size = new Size(594, 156);
            txtTxLog.TabIndex = 0;
            //
            // Form1
            //
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(1220, 740);
            Controls.Add(flpNetifs);
            Controls.Add(panelRight);
            Controls.Add(panelLogs);
            Controls.Add(panelTop);
            MinimumSize = new Size(1060, 660);
            Name = "Form1";
            Text = "TMS570 Kontrol Paneli";
            panelTop.ResumeLayout(false);
            panelTop.PerformLayout();
            panelRight.ResumeLayout(false);
            grpTerminal.ResumeLayout(false);
            grpTerminal.PerformLayout();
            panelTermTop.ResumeLayout(false);
            panelTermTop.PerformLayout();
            grpSend.ResumeLayout(false);
            grpSend.PerformLayout();
            grpActions.ResumeLayout(false);
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
        private GroupBox grpTerminal;
        private TextBox txtTermOut;
        private Panel panelTermTop;
        private Label lblTermNetif;
        private ComboBox cmbTermNetif;
        private Button btnEnterMenu;
        private TextBox txtInput;
        private Button btnSend;
        private Label lblPingHint;
        private Button btnStop;
        private GroupBox grpSend;
        private Label lblSendNetif;
        private ComboBox cmbSendNetif;
        private Label lblSendSocket;
        private ComboBox cmbSendSocket;
        private Label lblDstIp;
        private TextBox txtDstIp;
        private Label lblDstPort;
        private TextBox txtDstPort;
        private Label lblSendData;
        private TextBox txtSendData;
        private Button btnUdpSend;
        private GroupBox grpActions;
        private Button btnScan;
        private Button btnArpReset;
        private Panel panelLogs;
        private SplitContainer splitLogs;
        private GroupBox grpRx;
        private TextBox txtRxLog;
        private GroupBox grpTx;
        private TextBox txtTxLog;
    }
}
