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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace PhotoChat {
public partial class AllJoynConnect : Form {
    public AllJoynConnect()
    {
        _owner = null;
        InitializeComponent();
        this.Hide();
        //            _alljoyn = new AllJoynChatComponant();
    }

    public AllJoynConnect(Form owner)
    {
        _owner = owner;
        InitializeComponent();
        this.Hide();
        //            _alljoyn = new AllJoynChatComponant();
    }

    //        public AllJoynChatComponant Alljoyn { get { return _alljoyn; } }

    public bool IsConnected { get { return _connected; } set { _connected = value; } }
    public string SessionName { get { return txtSession.Text; } }
    public string MyHandle { get { return txtHandle.Text; } }
    public bool IsNameOwner { get { return rbAdvertise.Checked; } }

    //        private AllJoynChatComponant _alljoyn = null;
    private Form _owner = null;
    private bool _connected = false;

    protected override void OnShown(EventArgs e)
    {
        if (IsConnected) {
            btnOk.Text = "Disconnect";
            txtSession.Enabled = false;
            txtHandle.Enabled = false;
            rbAdvertise.Enabled = false;
            rbJoin.Enabled = false;
        } else   {
            btnOk.Text = "Connect";
            txtSession.Enabled = true;
            txtHandle.Enabled = true;
            rbAdvertise.Enabled = true;
            rbJoin.Enabled = true;
        }
        base.OnShown(e);
    }
}
}

