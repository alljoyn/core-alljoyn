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

namespace PhotoChat {
partial class AllJoynSetup {
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
        this.label1 = new System.Windows.Forms.Label();
        this.txtHandle = new System.Windows.Forms.TextBox();
        this.btnOk = new System.Windows.Forms.Button();
        this.btnCancel = new System.Windows.Forms.Button();
        this.rbJoin = new System.Windows.Forms.RadioButton();
        this.rbAdvertise = new System.Windows.Forms.RadioButton();
        this.txtSession = new System.Windows.Forms.TextBox();
        this.label2 = new System.Windows.Forms.Label();
        this.label3 = new System.Windows.Forms.Label();
        this.label4 = new System.Windows.Forms.Label();
        this.label5 = new System.Windows.Forms.Label();
        this.label6 = new System.Windows.Forms.Label();
        this.label7 = new System.Windows.Forms.Label();
        this.label8 = new System.Windows.Forms.Label();
        this.SuspendLayout();
        //
        // label1
        //
        this.label1.AutoSize = true;
        this.label1.Location = new System.Drawing.Point(20, 25);
        this.label1.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
        this.label1.Name = "label1";
        this.label1.Size = new System.Drawing.Size(66, 13);
        this.label1.TabIndex = 0;
        this.label1.Text = "Your Handle";
        //
        // txtHandle
        //
        this.txtHandle.Location = new System.Drawing.Point(117, 25);
        this.txtHandle.Margin = new System.Windows.Forms.Padding(2);
        this.txtHandle.Name = "txtHandle";
        this.txtHandle.Size = new System.Drawing.Size(184, 20);
        this.txtHandle.TabIndex = 1;
        //
        // btnOk
        //
        this.btnOk.DialogResult = System.Windows.Forms.DialogResult.OK;
        this.btnOk.Location = new System.Drawing.Point(32, 319);
        this.btnOk.Margin = new System.Windows.Forms.Padding(2);
        this.btnOk.Name = "btnOk";
        this.btnOk.Size = new System.Drawing.Size(90, 19);
        this.btnOk.TabIndex = 2;
        this.btnOk.Text = "Connect";
        this.btnOk.UseVisualStyleBackColor = true;
        this.btnOk.Click += new System.EventHandler(this.btnOk_Click);
        //
        // btnCancel
        //
        this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
        this.btnCancel.Location = new System.Drawing.Point(180, 319);
        this.btnCancel.Margin = new System.Windows.Forms.Padding(2);
        this.btnCancel.Name = "btnCancel";
        this.btnCancel.Size = new System.Drawing.Size(90, 19);
        this.btnCancel.TabIndex = 3;
        this.btnCancel.Text = "Cancel";
        this.btnCancel.UseVisualStyleBackColor = true;
        this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
        //
        // rbJoin
        //
        this.rbJoin.AutoSize = true;
        this.rbJoin.Location = new System.Drawing.Point(23, 130);
        this.rbJoin.Margin = new System.Windows.Forms.Padding(2);
        this.rbJoin.Name = "rbJoin";
        this.rbJoin.Size = new System.Drawing.Size(84, 17);
        this.rbJoin.TabIndex = 4;
        this.rbJoin.Text = "Join Session";
        this.rbJoin.UseVisualStyleBackColor = true;
        //
        // rbAdvertise
        //
        this.rbAdvertise.AutoSize = true;
        this.rbAdvertise.Checked = true;
        this.rbAdvertise.Location = new System.Drawing.Point(23, 99);
        this.rbAdvertise.Margin = new System.Windows.Forms.Padding(2);
        this.rbAdvertise.Name = "rbAdvertise";
        this.rbAdvertise.Size = new System.Drawing.Size(87, 17);
        this.rbAdvertise.TabIndex = 5;
        this.rbAdvertise.TabStop = true;
        this.rbAdvertise.Text = "Start Session";
        this.rbAdvertise.UseVisualStyleBackColor = true;
        //
        // txtSession
        //
        this.txtSession.Location = new System.Drawing.Point(117, 56);
        this.txtSession.Margin = new System.Windows.Forms.Padding(2);
        this.txtSession.Name = "txtSession";
        this.txtSession.Size = new System.Drawing.Size(184, 20);
        this.txtSession.TabIndex = 6;
        //
        // label2
        //
        this.label2.AutoSize = true;
        this.label2.Location = new System.Drawing.Point(20, 63);
        this.label2.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
        this.label2.Name = "label2";
        this.label2.Size = new System.Drawing.Size(75, 13);
        this.label2.TabIndex = 7;
        this.label2.Text = "Session Name";
        //
        // label3
        //
        this.label3.AutoSize = true;
        this.label3.Location = new System.Drawing.Point(26, 177);
        this.label3.Name = "label3";
        this.label3.Size = new System.Drawing.Size(55, 13);
        this.label3.TabIndex = 8;
        this.label3.Text = "Interface :";
        //
        // label4
        //
        this.label4.AutoSize = true;
        this.label4.Location = new System.Drawing.Point(117, 177);
        this.label4.Name = "label4";
        this.label4.Size = new System.Drawing.Size(139, 13);
        this.label4.TabIndex = 9;
        this.label4.Text = "org.alljoyn.bus.samples.chat";
        //
        // label5
        //
        this.label5.AutoSize = true;
        this.label5.Location = new System.Drawing.Point(117, 225);
        this.label5.Name = "label5";
        this.label5.Size = new System.Drawing.Size(139, 13);
        this.label5.TabIndex = 10;
        this.label5.Text = "org.alljoyn.bus.samples.chat";
        //
        // label6
        //
        this.label6.AutoSize = true;
        this.label6.Location = new System.Drawing.Point(26, 225);
        this.label6.Name = "label6";
        this.label6.Size = new System.Drawing.Size(74, 13);
        this.label6.TabIndex = 11;
        this.label6.Text = "Name Prefex :";
        //
        // label7
        //
        this.label7.AutoSize = true;
        this.label7.Location = new System.Drawing.Point(120, 277);
        this.label7.Name = "label7";
        this.label7.Size = new System.Drawing.Size(69, 13);
        this.label7.TabIndex = 12;
        this.label7.Text = "/chatService";
        //
        // label8
        //
        this.label8.AutoSize = true;
        this.label8.Location = new System.Drawing.Point(29, 277);
        this.label8.Name = "label8";
        this.label8.Size = new System.Drawing.Size(69, 13);
        this.label8.TabIndex = 13;
        this.label8.Text = "Object Path :";
        //
        // AlljoynSetup
        //
        this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        this.ClientSize = new System.Drawing.Size(309, 368);
        this.Controls.Add(this.label8);
        this.Controls.Add(this.label7);
        this.Controls.Add(this.label6);
        this.Controls.Add(this.label5);
        this.Controls.Add(this.label4);
        this.Controls.Add(this.label3);
        this.Controls.Add(this.label2);
        this.Controls.Add(this.txtSession);
        this.Controls.Add(this.rbAdvertise);
        this.Controls.Add(this.rbJoin);
        this.Controls.Add(this.btnCancel);
        this.Controls.Add(this.btnOk);
        this.Controls.Add(this.txtHandle);
        this.Controls.Add(this.label1);
        this.Margin = new System.Windows.Forms.Padding(2);
        this.Name = "AlljoynSetup";
        this.Text = "AlljoynSetup";
        this.ResumeLayout(false);
        this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.TextBox txtHandle;
    private System.Windows.Forms.Button btnOk;
    private System.Windows.Forms.Button btnCancel;
    private System.Windows.Forms.RadioButton rbJoin;
    private System.Windows.Forms.RadioButton rbAdvertise;
    private System.Windows.Forms.TextBox txtSession;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.Label label6;
    private System.Windows.Forms.Label label7;
    private System.Windows.Forms.Label label8;
}
}