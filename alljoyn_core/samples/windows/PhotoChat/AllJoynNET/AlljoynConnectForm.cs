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

namespace AllJoynNET {
/// <summary>
/// a Windows Form for Connecting to the AllJoyn bus
/// </summary>
public partial class AllJoynConnectForm : Form {
    #region Constructors


    public AllJoynConnectForm() : this(new AllJoynBus(), null) { }
    public AllJoynConnectForm(AllJoynBus bus, Form owner)
    {
        _ajBus = bus;
        _owner = owner;
        InitializeComponent();
        this.Hide();
    }

    #endregion

    #region Properties
    public bool IsConnected { get { return _connected; } set { _connected = value; } }
    public string SessionName { get { return txtSession.Text; } }
    public string MyHandle { get { return txtHandle.Text; } }
    public bool IsNameOwner { get { return rbAdvertise.Checked; } }

    public AllJoynBus AJBus { get { return _ajBus; } }
    public AllJoynSession AJSession { get { return _session; } }
    #endregion

    #region shared methods

    /// <summary>
    /// This function must be called before displaying the form.
    /// </summary>
    /// <param name="receiver">a ReceiverDelegate for handling output from the AllJoynBusConnection</param>
    /// <param name="subscribe">a SubscriberDelegate that will handle adding "joiners" to the session"</param>
    /// <returns></returns>
    public bool InstallDelegates(ReceiverDelegate receiver, SubscriberDelegate subscribe)
    {
        if (_ajBus != null) {
            _ajBus.SetOutputStream(receiver);
            _ajBus.SetLocalListener(subscribe);
            return true;
        }
        // else do notning
        return false;
    }

    /// <summary>
    /// override to update any local changes
    /// </summary>
    /// <param name="e">unused - passed to base class</param>
    protected override void OnShown(EventArgs e)
    {
        updateUI();
        base.OnShown(e);
    }

    #endregion

    #region private variables

    private Form _owner = null;
    private bool _connected = false;
    private AllJoynBus _ajBus;
    private AllJoynSession _session;

    #endregion

    #region private methods
    /// <summary>
    /// PRIVATE update form fields when visible
    /// </summary>
    private void updateUI()
    {
        if (IsConnected) {
            btnConnect.Text = "Disconnect";
            txtSession.Enabled = false;
            txtHandle.Enabled = false;
            rbAdvertise.Enabled = false;
            rbJoin.Enabled = false;
        } else   {
            btnConnect.Text = "Connect";
            txtSession.Enabled = true;
            txtHandle.Enabled = true;
            rbAdvertise.Enabled = true;
            rbJoin.Enabled = true;
        }
    }

    private void btnConnect_Click(object sender, EventArgs e)
    {
        if (!_connected) {
            if (checkInvariants()) {
                bool success = rbAdvertise.Checked;
                _connected = _ajBus.Connect(txtHandle.Text, ref success);
            } else
                MessageBox.Show("Missing input - handle or session names must not be empty",
                                "AllJoyn Connection", MessageBoxButtons.OK, MessageBoxIcon.Hand);
        } else   {
            _ajBus.Disconnect();
            _connected = false;
        }

        updateUI();
    }

    private bool checkInvariants()
    {
        return ((txtHandle.Text.Length > 0) && (txtSession.Text.Length > 0));
    }
    #endregion
}
}

