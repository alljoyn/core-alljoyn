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
partial class AllJoynConnectForm {
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
        this.label2 = new System.Windows.Forms.Label();
        this.txtSession = new System.Windows.Forms.TextBox();
        this.rbAdvertise = new System.Windows.Forms.RadioButton();
        this.rbJoin = new System.Windows.Forms.RadioButton();
        this.txtHandle = new System.Windows.Forms.TextBox();
        this.label1 = new System.Windows.Forms.Label();
        this.btnCancel = new System.Windows.Forms.Button();
        this.btnConnect = new System.Windows.Forms.Button();
        this.SuspendLayout();
        //
        // label2
        //
        this.label2.AutoSize = true;
        this.label2.Location = new System.Drawing.Point(16, 58);
        this.label2.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
        this.label2.Name = "label2";
        this.label2.Size = new System.Drawing.Size(75, 13);
        this.label2.TabIndex = 13;
        this.label2.Text = "Session Name";
        //
        // txtSession
        //
        this.txtSession.Location = new System.Drawing.Point(95, 58);
        this.txtSession.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.txtSession.Name = "txtSession";
        this.txtSession.Size = new System.Drawing.Size(188, 20);
        this.txtSession.TabIndex = 12;
        //
        // rbAdvertise
        //
        this.rbAdvertise.AutoSize = true;
        this.rbAdvertise.Checked = true;
        this.rbAdvertise.Location = new System.Drawing.Point(19, 91);
        this.rbAdvertise.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.rbAdvertise.Name = "rbAdvertise";
        this.rbAdvertise.Size = new System.Drawing.Size(87, 17);
        this.rbAdvertise.TabIndex = 11;
        this.rbAdvertise.TabStop = true;
        this.rbAdvertise.Text = "Start Session";
        this.rbAdvertise.UseVisualStyleBackColor = true;
        //
        // rbJoin
        //
        this.rbJoin.AutoSize = true;
        this.rbJoin.Location = new System.Drawing.Point(115, 91);
        this.rbJoin.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.rbJoin.Name = "rbJoin";
        this.rbJoin.Size = new System.Drawing.Size(84, 17);
        this.rbJoin.TabIndex = 10;
        this.rbJoin.Text = "Join Session";
        this.rbJoin.UseVisualStyleBackColor = true;
        //
        // txtHandle
        //
        this.txtHandle.Location = new System.Drawing.Point(95, 20);
        this.txtHandle.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.txtHandle.Name = "txtHandle";
        this.txtHandle.Size = new System.Drawing.Size(188, 20);
        this.txtHandle.TabIndex = 9;
        //
        // label1
        //
        this.label1.AutoSize = true;
        this.label1.Location = new System.Drawing.Point(16, 20);
        this.label1.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
        this.label1.Name = "label1";
        this.label1.Size = new System.Drawing.Size(66, 13);
        this.label1.TabIndex = 8;
        this.label1.Text = "Your Handle";
        //
        // btnCancel
        //
        this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
        this.btnCancel.Location = new System.Drawing.Point(162, 126);
        this.btnCancel.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.btnCancel.Name = "btnCancel";
        this.btnCancel.Size = new System.Drawing.Size(71, 19);
        this.btnCancel.TabIndex = 15;
        this.btnCancel.Text = "Cancel";
        this.btnCancel.UseVisualStyleBackColor = true;
        //
        // btnConnect
        //
        this.btnConnect.DialogResult = System.Windows.Forms.DialogResult.OK;
        this.btnConnect.Location = new System.Drawing.Point(59, 126);
        this.btnConnect.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.btnConnect.Name = "btnConnect";
        this.btnConnect.Size = new System.Drawing.Size(71, 19);
        this.btnConnect.TabIndex = 14;
        this.btnConnect.Text = "Connect";
        this.btnConnect.UseVisualStyleBackColor = true;
        this.btnConnect.Click += new System.EventHandler(this.btnConnect_Click);
        //
        // AllJoynConnectForm
        //
        this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        this.ClientSize = new System.Drawing.Size(302, 168);
        this.Controls.Add(this.btnCancel);
        this.Controls.Add(this.btnConnect);
        this.Controls.Add(this.label2);
        this.Controls.Add(this.txtSession);
        this.Controls.Add(this.rbAdvertise);
        this.Controls.Add(this.rbJoin);
        this.Controls.Add(this.txtHandle);
        this.Controls.Add(this.label1);
        this.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
        this.Name = "AllJoynConnectForm";
        this.Text = "AlljoynConnect";
        this.ResumeLayout(false);
        this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.TextBox txtSession;
    private System.Windows.Forms.RadioButton rbAdvertise;
    private System.Windows.Forms.RadioButton rbJoin;
    private System.Windows.Forms.TextBox txtHandle;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Button btnCancel;
    private System.Windows.Forms.Button btnConnect;
}
}
