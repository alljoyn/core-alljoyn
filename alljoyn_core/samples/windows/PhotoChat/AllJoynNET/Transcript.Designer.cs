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
partial class Transcript {
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
        this.txtTranscript = new System.Windows.Forms.TextBox();
        this.btnConnect = new System.Windows.Forms.Button();
        this.button2 = new System.Windows.Forms.Button();
        this.SuspendLayout();
        //
        // txtTranscript
        //
        this.txtTranscript.Location = new System.Drawing.Point(-2, -1);
        this.txtTranscript.Multiline = true;
        this.txtTranscript.Name = "txtTranscript";
        this.txtTranscript.Size = new System.Drawing.Size(458, 592);
        this.txtTranscript.TabIndex = 0;
        //
        // btnConnect
        //
        this.btnConnect.Location = new System.Drawing.Point(75, 632);
        this.btnConnect.Name = "btnConnect";
        this.btnConnect.Size = new System.Drawing.Size(75, 23);
        this.btnConnect.TabIndex = 1;
        this.btnConnect.Text = "AllJoyn";
        this.btnConnect.UseVisualStyleBackColor = true;
        this.btnConnect.Click += new System.EventHandler(this.btnAllJoyn_Click);
        //
        // button2
        //
        this.button2.Location = new System.Drawing.Point(267, 631);
        this.button2.Name = "button2";
        this.button2.Size = new System.Drawing.Size(75, 23);
        this.button2.TabIndex = 2;
        this.button2.Text = "Exit";
        this.button2.UseVisualStyleBackColor = true;
        this.button2.Click += new System.EventHandler(this.button2_Click);
        //
        // Transcript
        //
        this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        this.ClientSize = new System.Drawing.Size(456, 696);
        this.Controls.Add(this.button2);
        this.Controls.Add(this.btnConnect);
        this.Controls.Add(this.txtTranscript);
        this.Name = "Transcript";
        this.Text = "AllJoyn Bus";
        this.ResumeLayout(false);
        this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.TextBox txtTranscript;
    private System.Windows.Forms.Button btnConnect;
    private System.Windows.Forms.Button button2;
}
}
