/*
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
//----------------------------------------------------------------------------------------------

namespace WinChat {
partial class ChatDialog {
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
        System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ChatDialog));
        this.btnAlljoyn = new System.Windows.Forms.Button();
        this.btnSend = new System.Windows.Forms.Button();
        this.lblStatus = new System.Windows.Forms.Label();
        this.splitContainer1 = new System.Windows.Forms.SplitContainer();
        this.richTextTranscript = new System.Windows.Forms.RichTextBox();
        this.txtMessage = new System.Windows.Forms.TextBox();
        this.lblStat = new System.Windows.Forms.Label();
        this.rbAuto = new System.Windows.Forms.RadioButton();
        this.rbSend = new System.Windows.Forms.RadioButton();
        this.splitContainer1.Panel1.SuspendLayout();
        this.splitContainer1.Panel2.SuspendLayout();
        this.splitContainer1.SuspendLayout();
        this.SuspendLayout();
        //
        // btnAlljoyn
        //
        this.btnAlljoyn.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("btnAlljoyn.BackgroundImage")));
        this.btnAlljoyn.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
        this.btnAlljoyn.Location = new System.Drawing.Point(12, 646);
        this.btnAlljoyn.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
        this.btnAlljoyn.Name = "btnAlljoyn";
        this.btnAlljoyn.Size = new System.Drawing.Size(112, 109);
        this.btnAlljoyn.TabIndex = 2;
        this.btnAlljoyn.UseVisualStyleBackColor = true;
        this.btnAlljoyn.Click += new System.EventHandler(this.Alljoyn_Click);
        //
        // btnSend
        //
        this.btnSend.Location = new System.Drawing.Point(168, 719);
        this.btnSend.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
        this.btnSend.Name = "btnSend";
        this.btnSend.Size = new System.Drawing.Size(111, 36);
        this.btnSend.TabIndex = 3;
        this.btnSend.Text = "Send";
        this.btnSend.UseVisualStyleBackColor = true;
        this.btnSend.Click += new System.EventHandler(this.Send_Click);
        //
        // lblStatus
        //
        this.lblStatus.AutoSize = true;
        this.lblStatus.Font = new System.Drawing.Font("Calibri", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
        this.lblStatus.Location = new System.Drawing.Point(245, 668);
        this.lblStatus.Name = "lblStatus";
        this.lblStatus.Size = new System.Drawing.Size(0, 23);
        this.lblStatus.TabIndex = 4;
        //
        // splitContainer1
        //
        this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Top;
        this.splitContainer1.Location = new System.Drawing.Point(0, 0);
        this.splitContainer1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
        this.splitContainer1.Name = "splitContainer1";
        this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
        //
        // splitContainer1.Panel1
        //
        this.splitContainer1.Panel1.Controls.Add(this.richTextTranscript);
        //
        // splitContainer1.Panel2
        //
        this.splitContainer1.Panel2.Controls.Add(this.txtMessage);
        this.splitContainer1.Size = new System.Drawing.Size(800, 625);
        this.splitContainer1.SplitterDistance = 442;
        this.splitContainer1.TabIndex = 5;
        //
        // richTextTranscript
        //
        this.richTextTranscript.BackColor = System.Drawing.Color.WhiteSmoke;
        this.richTextTranscript.Dock = System.Windows.Forms.DockStyle.Fill;
        this.richTextTranscript.Location = new System.Drawing.Point(0, 0);
        this.richTextTranscript.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
        this.richTextTranscript.Name = "richTextTranscript";
        this.richTextTranscript.ReadOnly = true;
        this.richTextTranscript.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
        this.richTextTranscript.Size = new System.Drawing.Size(800, 442);
        this.richTextTranscript.TabIndex = 0;
        this.richTextTranscript.Text = "";
        //
        // txtMessage
        //
        this.txtMessage.AcceptsReturn = true;
        this.txtMessage.BackColor = System.Drawing.Color.AliceBlue;
        this.txtMessage.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.txtMessage.Dock = System.Windows.Forms.DockStyle.Fill;
        this.txtMessage.Location = new System.Drawing.Point(0, 0);
        this.txtMessage.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
        this.txtMessage.Multiline = true;
        this.txtMessage.Name = "txtMessage";
        this.txtMessage.Size = new System.Drawing.Size(800, 179);
        this.txtMessage.TabIndex = 0;
        //
        // lblStat
        //
        this.lblStat.AutoSize = true;
        this.lblStat.Font = new System.Drawing.Font("Verdana", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
        this.lblStat.Location = new System.Drawing.Point(164, 672);
        this.lblStat.Margin = new System.Windows.Forms.Padding(4, 0, 4, 0);
        this.lblStat.Name = "lblStat";
        this.lblStat.Size = new System.Drawing.Size(124, 17);
        this.lblStat.TabIndex = 6;
        this.lblStat.Text = "<-- Start Here!";
        //
        // rbAuto
        //
        this.rbAuto.AutoSize = true;
        this.rbAuto.Checked = true;
        this.rbAuto.Location = new System.Drawing.Point(381, 726);
        this.rbAuto.Margin = new System.Windows.Forms.Padding(4);
        this.rbAuto.Name = "rbAuto";
        this.rbAuto.Size = new System.Drawing.Size(80, 21);
        this.rbAuto.TabIndex = 7;
        this.rbAuto.TabStop = true;
        this.rbAuto.Text = "Multiline";
        this.rbAuto.UseVisualStyleBackColor = true;
        //
        // rbSend
        //
        this.rbSend.AutoSize = true;
        this.rbSend.Location = new System.Drawing.Point(529, 726);
        this.rbSend.Margin = new System.Windows.Forms.Padding(4);
        this.rbSend.Name = "rbSend";
        this.rbSend.Size = new System.Drawing.Size(95, 21);
        this.rbSend.TabIndex = 8;
        this.rbSend.Text = "Auto Send";
        this.rbSend.UseVisualStyleBackColor = true;
        //
        // ChatDialog
        //
        this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(255)))), ((int)(((byte)(192)))));
        this.ClientSize = new System.Drawing.Size(800, 775);
        this.Controls.Add(this.rbSend);
        this.Controls.Add(this.rbAuto);
        this.Controls.Add(this.lblStat);
        this.Controls.Add(this.btnAlljoyn);
        this.Controls.Add(this.splitContainer1);
        this.Controls.Add(this.lblStatus);
        this.Controls.Add(this.btnSend);
        this.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
        this.Name = "ChatDialog";
        this.Text = "Alljoyn Chat Sample";
        this.splitContainer1.Panel1.ResumeLayout(false);
        this.splitContainer1.Panel2.ResumeLayout(false);
        this.splitContainer1.Panel2.PerformLayout();
        this.splitContainer1.ResumeLayout(false);
        this.ResumeLayout(false);
        this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Button btnAlljoyn;
    private System.Windows.Forms.Button btnSend;
    private System.Windows.Forms.Label lblStatus;
    private System.Windows.Forms.SplitContainer splitContainer1;
    private System.Windows.Forms.RichTextBox richTextTranscript;
    private System.Windows.Forms.TextBox txtMessage;
    private System.Windows.Forms.Label lblStat;
    private System.Windows.Forms.RadioButton rbAuto;
    private System.Windows.Forms.RadioButton rbSend;
}
}

