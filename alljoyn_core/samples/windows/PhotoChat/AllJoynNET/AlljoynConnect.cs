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
