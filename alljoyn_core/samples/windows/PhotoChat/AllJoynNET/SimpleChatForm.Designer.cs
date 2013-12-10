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

namespace AllJoynNET {
partial class SimpleChatForm {
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
        this.txtChat = new System.Windows.Forms.TextBox();
        this.txtInput = new System.Windows.Forms.TextBox();
        this.btnAllJoyn = new System.Windows.Forms.Button();
        this.btnSend = new System.Windows.Forms.Button();
        this.btnClose = new System.Windows.Forms.Button();
        this.txtTranscript = new System.Windows.Forms.TextBox();
        this.SuspendLayout();
        //
        // txtChat
        //
        this.txtChat.Location = new System.Drawing.Point(13, 13);
        this.txtChat.Multiline = true;
        this.txtChat.Name = "txtChat";
        this.txtChat.ReadOnly = true;
        this.txtChat.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
        this.txtChat.Size = new System.Drawing.Size(418, 383);
        this.txtChat.TabIndex = 0;
        //
        // txtInput
        //
        this.txtInput.Location = new System.Drawing.Point(12, 402);
        this.txtInput.Multiline = true;
        this.txtInput.Name = "txtInput";
        this.txtInput.Size = new System.Drawing.Size(419, 147);
        this.txtInput.TabIndex = 1;
        //
        // btnAllJoyn
        //
        this.btnAllJoyn.Location = new System.Drawing.Point(13, 569);
        this.btnAllJoyn.Name = "btnAllJoyn";
        this.btnAllJoyn.Size = new System.Drawing.Size(146, 134);
        this.btnAllJoyn.TabIndex = 2;
        this.btnAllJoyn.Text = "AllJoyn";
        this.btnAllJoyn.UseVisualStyleBackColor = true;
        this.btnAllJoyn.Click += new System.EventHandler(this.btnAllJoyn_Click);
        //
        // btnSend
        //
        this.btnSend.Location = new System.Drawing.Point(183, 579);
        this.btnSend.Name = "btnSend";
        this.btnSend.Size = new System.Drawing.Size(75, 23);
        this.btnSend.TabIndex = 3;
        this.btnSend.Text = "Send";
        this.btnSend.UseVisualStyleBackColor = true;
        this.btnSend.Click += new System.EventHandler(this.btnSend_Click);
        //
        // btnClose
        //
        this.btnClose.Location = new System.Drawing.Point(183, 631);
        this.btnClose.Name = "btnClose";
        this.btnClose.Size = new System.Drawing.Size(75, 23);
        this.btnClose.TabIndex = 4;
        this.btnClose.Text = "Close";
        this.btnClose.UseVisualStyleBackColor = true;
        this.btnClose.Click += new System.EventHandler(this.btnClose_Click);
        //
        // txtTranscript
        //
        this.txtTranscript.Location = new System.Drawing.Point(451, 13);
        this.txtTranscript.Multiline = true;
        this.txtTranscript.Name = "txtTranscript";
        this.txtTranscript.ReadOnly = true;
        this.txtTranscript.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
        this.txtTranscript.Size = new System.Drawing.Size(401, 690);
        this.txtTranscript.TabIndex = 5;
        //
        // SimpleChatForm
        //
        this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        this.ClientSize = new System.Drawing.Size(864, 715);
        this.Controls.Add(this.txtTranscript);
        this.Controls.Add(this.btnClose);
        this.Controls.Add(this.btnSend);
        this.Controls.Add(this.btnAllJoyn);
        this.Controls.Add(this.txtInput);
        this.Controls.Add(this.txtChat);
        this.Name = "SimpleChatForm";
        this.Text = "SimpleChatForm";
        this.ResumeLayout(false);
        this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.TextBox txtChat;
    private System.Windows.Forms.TextBox txtInput;
    private System.Windows.Forms.Button btnAllJoyn;
    private System.Windows.Forms.Button btnSend;
    private System.Windows.Forms.Button btnClose;
    private System.Windows.Forms.TextBox txtTranscript;
}
}
