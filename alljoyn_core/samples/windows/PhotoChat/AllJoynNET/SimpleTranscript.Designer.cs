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
partial class SimpleTranscript {
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
        this.SuspendLayout();
        //
        // txtTranscript
        //
        this.txtTranscript.Location = new System.Drawing.Point(1, 0);
        this.txtTranscript.Margin = new System.Windows.Forms.Padding(2);
        this.txtTranscript.Multiline = true;
        this.txtTranscript.Name = "txtTranscript";
        this.txtTranscript.ReadOnly = true;
        this.txtTranscript.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
        this.txtTranscript.Size = new System.Drawing.Size(308, 477);
        this.txtTranscript.TabIndex = 0;
        this.txtTranscript.WordWrap = false;
        //
        // SimpleTranscript
        //
        this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        this.ClientSize = new System.Drawing.Size(308, 522);
        this.Controls.Add(this.txtTranscript);
        this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
        this.Location = new System.Drawing.Point(200, 0);
        this.Margin = new System.Windows.Forms.Padding(2);
        this.MaximizeBox = false;
        this.MinimizeBox = false;
        this.Name = "SimpleTranscript";
        this.ShowInTaskbar = false;
        this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
        this.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
        this.Text = "SimpleTranscript";
        this.ResumeLayout(false);
        this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.TextBox txtTranscript;
}
}
