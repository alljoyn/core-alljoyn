// ****************************************************************************
// // 
//    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
//    Source Project Contributors and others.
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0

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