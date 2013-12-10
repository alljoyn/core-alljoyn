// ****************************************************************************
// Copyright (c) 2011, AllSeen Alliance. All rights reserved.
//
//    Permission to use, copy, modify, and/or distribute this software for any
//    purpose with or without fee is hereby granted, provided that the above
//    copyright notice and this permission notice appear in all copies.
//
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// ******************************************************************************

namespace PhotoChat {
partial class PhotoChatForm {
    /// <summary>
    /// Required designer variable.
    /// </summary>
    private System.ComponentModel.IContainer components = null;

    /// <summary>
    /// Clean up any resources being used.
    /// </summary>
    /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
    protected override void Dispose(bool disposing)
    {
        if (disposing && (components != null)) {
            components.Dispose();
        }
        base.Dispose(disposing);
    }

    #region Windows Form Designer generated code

    /// <summary>
    /// Required method for Designer support - do not modify
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
        this.components = new System.ComponentModel.Container();
        System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(PhotoChatForm));
        this.txtMessage = new System.Windows.Forms.TextBox();
        this.richTextOutput = new System.Windows.Forms.RichTextBox();
        this.splitContainer1 = new System.Windows.Forms.SplitContainer();
        this.pnlChatters = new System.Windows.Forms.FlowLayoutPanel();
        this.label1 = new System.Windows.Forms.Label();
        this.clbChatters = new System.Windows.Forms.CheckedListBox();
        this.panel2 = new System.Windows.Forms.Panel();
        this.btnSendFile = new System.Windows.Forms.Button();
        this.panel1 = new System.Windows.Forms.Panel();
        this.btnSend = new System.Windows.Forms.Button();
        this.btnAllJoyn = new System.Windows.Forms.Button();
        this.lblStatus = new System.Windows.Forms.Label();
        this.lblStart = new System.Windows.Forms.Label();
        this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
        this.lblPanel = new System.Windows.Forms.Label();
        this.btnAddFiles = new System.Windows.Forms.Button();
        this.btnAddFolder = new System.Windows.Forms.Button();
        this.lvPic = new System.Windows.Forms.ListView();
        this.btnTranscript = new System.Windows.Forms.Button();
        this.btnClose = new System.Windows.Forms.Button();
        this.progressBar1 = new System.Windows.Forms.ProgressBar();
        this.imageList1 = new System.Windows.Forms.ImageList(this.components);
        this.dlgBrowseFolders = new System.Windows.Forms.FolderBrowserDialog();
        this.btnShare = new System.Windows.Forms.Button();
        this.splitContainer1.Panel1.SuspendLayout();
        this.splitContainer1.Panel2.SuspendLayout();
        this.splitContainer1.SuspendLayout();
        this.pnlChatters.SuspendLayout();
        this.panel2.SuspendLayout();
        this.flowLayoutPanel1.SuspendLayout();
        this.SuspendLayout();
        //
        // txtMessage
        //
        this.txtMessage.AcceptsReturn = true;
        this.txtMessage.BackColor = System.Drawing.Color.AliceBlue;
        this.txtMessage.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
        this.txtMessage.Dock = System.Windows.Forms.DockStyle.Fill;
        this.txtMessage.Location = new System.Drawing.Point(0, 0);
        this.txtMessage.Multiline = true;
        this.txtMessage.Name = "txtMessage";
        this.txtMessage.Size = new System.Drawing.Size(640, 154);
        this.txtMessage.TabIndex = 2;
        //
        // richTextOutput
        //
        this.richTextOutput.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
        this.richTextOutput.Dock = System.Windows.Forms.DockStyle.Fill;
        this.richTextOutput.Location = new System.Drawing.Point(0, 0);
        this.richTextOutput.Name = "richTextOutput";
        this.richTextOutput.ReadOnly = true;
        this.richTextOutput.Size = new System.Drawing.Size(640, 249);
        this.richTextOutput.TabIndex = 1;
        this.richTextOutput.Text = "";
        //
        // splitContainer1
        //
        this.splitContainer1.Location = new System.Drawing.Point(9, 11);
        this.splitContainer1.Name = "splitContainer1";
        this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
        //
        // splitContainer1.Panel1
        //
        this.splitContainer1.Panel1.Controls.Add(this.pnlChatters);
        this.splitContainer1.Panel1.Controls.Add(this.richTextOutput);
        //
        // splitContainer1.Panel2
        //
        this.splitContainer1.Panel2.Controls.Add(this.txtMessage);
        this.splitContainer1.Size = new System.Drawing.Size(640, 407);
        this.splitContainer1.SplitterDistance = 249;
        this.splitContainer1.TabIndex = 3;
        //
        // pnlChatters
        //
        this.pnlChatters.Anchor = System.Windows.Forms.AnchorStyles.None;
        this.pnlChatters.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
        this.pnlChatters.Controls.Add(this.label1);
        this.pnlChatters.Controls.Add(this.clbChatters);
        this.pnlChatters.Controls.Add(this.panel2);
        this.pnlChatters.Location = new System.Drawing.Point(271, 83);
        this.pnlChatters.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.pnlChatters.Name = "pnlChatters";
        this.pnlChatters.Size = new System.Drawing.Size(96, 152);
        this.pnlChatters.TabIndex = 4;
        this.pnlChatters.Visible = false;
        //
        // label1
        //
        this.label1.AutoSize = true;
        this.label1.Location = new System.Drawing.Point(2, 0);
        this.label1.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
        this.label1.Name = "label1";
        this.label1.Size = new System.Drawing.Size(51, 13);
        this.label1.TabIndex = 4;
        this.label1.Text = "Send To:";
        //
        // clbChatters
        //
        this.clbChatters.CheckOnClick = true;
        this.clbChatters.FormattingEnabled = true;
        this.clbChatters.Location = new System.Drawing.Point(2, 15);
        this.clbChatters.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.clbChatters.Name = "clbChatters";
        this.clbChatters.Size = new System.Drawing.Size(91, 94);
        this.clbChatters.TabIndex = 5;
        //
        // panel2
        //
        this.panel2.Controls.Add(this.btnSendFile);
        this.panel2.Location = new System.Drawing.Point(2, 113);
        this.panel2.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.panel2.Name = "panel2";
        this.panel2.Size = new System.Drawing.Size(90, 30);
        this.panel2.TabIndex = 6;
        //
        // btnSendFile
        //
        this.btnSendFile.Anchor = System.Windows.Forms.AnchorStyles.None;
        this.btnSendFile.Location = new System.Drawing.Point(2, -1);
        this.btnSendFile.Name = "btnSendFile";
        this.btnSendFile.Size = new System.Drawing.Size(85, 28);
        this.btnSendFile.TabIndex = 6;
        this.btnSendFile.Text = "OK";
        this.btnSendFile.UseVisualStyleBackColor = true;
        this.btnSendFile.Click += new System.EventHandler(this.btnSendFile_Click);
        //
        // panel1
        //
        this.panel1.Location = new System.Drawing.Point(120, 493);
        this.panel1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.panel1.Name = "panel1";
        this.panel1.Size = new System.Drawing.Size(530, 86);
        this.panel1.TabIndex = 14;
        //
        // btnSend
        //
        this.btnSend.FlatAppearance.MouseOverBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
        this.btnSend.Location = new System.Drawing.Point(10, 432);
        this.btnSend.Name = "btnSend";
        this.btnSend.Size = new System.Drawing.Size(105, 23);
        this.btnSend.TabIndex = 11;
        this.btnSend.Text = "Send";
        this.btnSend.UseVisualStyleBackColor = true;
        this.btnSend.Click += new System.EventHandler(this.btnSend_Click);
        //
        // btnAllJoyn
        //
        this.btnAllJoyn.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("btnAllJoyn.BackgroundImage")));
        this.btnAllJoyn.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
        this.btnAllJoyn.FlatAppearance.MouseDownBackColor = System.Drawing.Color.White;
        this.btnAllJoyn.FlatAppearance.MouseOverBackColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
        this.btnAllJoyn.Location = new System.Drawing.Point(10, 463);
        this.btnAllJoyn.Name = "btnAllJoyn";
        this.btnAllJoyn.Size = new System.Drawing.Size(105, 116);
        this.btnAllJoyn.TabIndex = 8;
        this.btnAllJoyn.UseVisualStyleBackColor = true;
        this.btnAllJoyn.Click += new System.EventHandler(this.btnAllJoyn_Click);
        //
        // lblStatus
        //
        this.lblStatus.AutoSize = true;
        this.lblStatus.Location = new System.Drawing.Point(159, 474);
        this.lblStatus.Name = "lblStatus";
        this.lblStatus.Size = new System.Drawing.Size(0, 13);
        this.lblStatus.TabIndex = 10;
        //
        // lblStart
        //
        this.lblStart.AutoSize = true;
        this.lblStart.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
        this.lblStart.Location = new System.Drawing.Point(118, 472);
        this.lblStart.Name = "lblStart";
        this.lblStart.Size = new System.Drawing.Size(89, 15);
        this.lblStart.TabIndex = 9;
        this.lblStart.Text = "<- Start Here";
        //
        // flowLayoutPanel1
        //
        this.flowLayoutPanel1.Controls.Add(this.lblPanel);
        this.flowLayoutPanel1.Controls.Add(this.btnAddFiles);
        this.flowLayoutPanel1.Controls.Add(this.btnAddFolder);
        this.flowLayoutPanel1.Controls.Add(this.lvPic);
        this.flowLayoutPanel1.Location = new System.Drawing.Point(656, 11);
        this.flowLayoutPanel1.Name = "flowLayoutPanel1";
        this.flowLayoutPanel1.Size = new System.Drawing.Size(131, 567);
        this.flowLayoutPanel1.TabIndex = 1;
        //
        // lblPanel
        //
        this.lblPanel.Font = new System.Drawing.Font("Microsoft Sans Serif", 9F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
        this.lblPanel.Location = new System.Drawing.Point(3, 0);
        this.lblPanel.Name = "lblPanel";
        this.lblPanel.Size = new System.Drawing.Size(121, 30);
        this.lblPanel.TabIndex = 0;
        this.lblPanel.Text = "Photo Share";
        this.lblPanel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
        //
        // btnAddFiles
        //
        this.btnAddFiles.Location = new System.Drawing.Point(3, 33);
        this.btnAddFiles.Name = "btnAddFiles";
        this.btnAddFiles.Size = new System.Drawing.Size(124, 23);
        this.btnAddFiles.TabIndex = 3;
        this.btnAddFiles.Text = "Add Files";
        this.btnAddFiles.UseVisualStyleBackColor = true;
        this.btnAddFiles.Click += new System.EventHandler(this.btnAddFiles_Click);
        //
        // btnAddFolder
        //
        this.btnAddFolder.Location = new System.Drawing.Point(3, 62);
        this.btnAddFolder.Name = "btnAddFolder";
        this.btnAddFolder.Size = new System.Drawing.Size(124, 23);
        this.btnAddFolder.TabIndex = 2;
        this.btnAddFolder.Text = "Add Folder";
        this.btnAddFolder.UseVisualStyleBackColor = true;
        this.btnAddFolder.Click += new System.EventHandler(this.btnAddFolder_Click);
        //
        // lvPic
        //
        this.lvPic.AllowDrop = true;
        this.lvPic.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
        this.lvPic.Location = new System.Drawing.Point(3, 91);
        this.lvPic.Name = "lvPic";
        this.lvPic.Size = new System.Drawing.Size(125, 466);
        this.lvPic.TabIndex = 1;
        this.lvPic.UseCompatibleStateImageBehavior = false;
        this.lvPic.View = System.Windows.Forms.View.Details;
        this.lvPic.MouseDown += new System.Windows.Forms.MouseEventHandler(this.lvPic_MouseDown);
        //
        // btnTranscript
        //
        this.btnTranscript.Location = new System.Drawing.Point(520, 432);
        this.btnTranscript.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.btnTranscript.Name = "btnTranscript";
        this.btnTranscript.Size = new System.Drawing.Size(131, 23);
        this.btnTranscript.TabIndex = 15;
        this.btnTranscript.Text = "Show Transcript";
        this.btnTranscript.UseVisualStyleBackColor = true;
        this.btnTranscript.Click += new System.EventHandler(this.btnTranscript_Click);
        //
        // btnClose
        //
        this.btnClose.Location = new System.Drawing.Point(9, 584);
        this.btnClose.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.btnClose.Name = "btnClose";
        this.btnClose.Size = new System.Drawing.Size(106, 19);
        this.btnClose.TabIndex = 16;
        this.btnClose.Text = "Exit";
        this.btnClose.UseVisualStyleBackColor = true;
        //
        // progressBar1
        //
        this.progressBar1.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(64)))), ((int)(((byte)(64)))));
        this.progressBar1.Location = new System.Drawing.Point(119, 584);
        this.progressBar1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.progressBar1.Name = "progressBar1";
        this.progressBar1.Size = new System.Drawing.Size(532, 10);
        this.progressBar1.TabIndex = 17;
        //
        // imageList1
        //
        this.imageList1.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
        this.imageList1.ImageSize = new System.Drawing.Size(64, 64);
        this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
        //
        // dlgBrowseFolders
        //
        this.dlgBrowseFolders.Description = "Add pictures to share";
        this.dlgBrowseFolders.SelectedPath = "C:\\Users\\dad\\Pictures";
        this.dlgBrowseFolders.ShowNewFolderButton = false;
        //
        // btnShare
        //
        this.btnShare.Location = new System.Drawing.Point(658, 584);
        this.btnShare.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.btnShare.Name = "btnShare";
        this.btnShare.Size = new System.Drawing.Size(122, 19);
        this.btnShare.TabIndex = 4;
        this.btnShare.Text = "Share";
        this.btnShare.UseVisualStyleBackColor = true;
        this.btnShare.Click += new System.EventHandler(this.btnShare_Click);
        //
        // PhotoChatForm
        //
        this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(255)))), ((int)(((byte)(255)))));
        this.ClientSize = new System.Drawing.Size(790, 615);
        this.Controls.Add(this.progressBar1);
        this.Controls.Add(this.btnClose);
        this.Controls.Add(this.btnTranscript);
        this.Controls.Add(this.flowLayoutPanel1);
        this.Controls.Add(this.btnShare);
        this.Controls.Add(this.panel1);
        this.Controls.Add(this.btnSend);
        this.Controls.Add(this.btnAllJoyn);
        this.Controls.Add(this.lblStatus);
        this.Controls.Add(this.lblStart);
        this.Controls.Add(this.splitContainer1);
        this.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.Name = "PhotoChatForm";
        this.Text = "Alljoyn Photo Chat";
        this.splitContainer1.Panel1.ResumeLayout(false);
        this.splitContainer1.Panel2.ResumeLayout(false);
        this.splitContainer1.Panel2.PerformLayout();
        this.splitContainer1.ResumeLayout(false);
        this.pnlChatters.ResumeLayout(false);
        this.pnlChatters.PerformLayout();
        this.panel2.ResumeLayout(false);
        this.flowLayoutPanel1.ResumeLayout(false);
        this.ResumeLayout(false);
        this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.TextBox txtMessage;
    private System.Windows.Forms.RichTextBox richTextOutput;
    private System.Windows.Forms.SplitContainer splitContainer1;
    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.Button btnSend;
    private System.Windows.Forms.Button btnAllJoyn;
    private System.Windows.Forms.Label lblStatus;
    private System.Windows.Forms.Label lblStart;
    private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
    private System.Windows.Forms.Button btnAddFiles;
    private System.Windows.Forms.Label lblPanel;
    private System.Windows.Forms.Button btnAddFolder;
    private System.Windows.Forms.ListView lvPic;
    private System.Windows.Forms.Button btnTranscript;
    private System.Windows.Forms.Button btnClose;
    private System.Windows.Forms.ProgressBar progressBar1;
    private System.Windows.Forms.ImageList imageList1;
    private System.Windows.Forms.FolderBrowserDialog dlgBrowseFolders;
    private System.Windows.Forms.FlowLayoutPanel pnlChatters;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.CheckedListBox clbChatters;
    private System.Windows.Forms.Button btnShare;
    private System.Windows.Forms.Button btnSendFile;
    private System.Windows.Forms.Panel panel2;
}
}

