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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace PhotoChat {
/// <summary>
/// An unused Form at this time.
/// Will eventually allow editing of AllJoyn session parameters
/// </summary>
public partial class AllJoynSetup : Form {
    #region Constructor
    //---------------------------------------------------------------------
    /// <summary>
    /// Construct the
    /// </summary>
    /// <param name="owner">
    /// The parent form.
    /// </param>
    public AllJoynSetup(ChatForm owner)
    {
        _owner = owner;
        InitializeComponent();
        this.Hide();
        _alljoyn = new AllJoynChatComponant();
        InterfaceName = _alljoyn.InterfaceName;
        NamePrefix = _alljoyn.NamePrefix;
        ObjectPath = _alljoyn.ObjectPath;
    }
    #endregion
    //---------------------------------------------------------------------
    #region Properties - internal (can be accessed by owner Form)

    internal AllJoynChatComponant AllJoyn { get { return _alljoyn; } }
    internal string SessionName { get { return txtSession.Text; } }
    internal string MyHandle { get { return txtHandle.Text; } }
    internal bool IsNameOwner { get { return rbAdvertise.Checked; } }

    #endregion
    //---------------------------------------------------------------------
    #region private variables

    private AllJoynChatComponant _alljoyn = null;
    private ChatForm _owner = null;

    internal string InterfaceName = "";
    internal string NamePrefix = "";
    internal string ObjectPath = "";

    #endregion
    //---------------------------------------------------------------------
    #region protected methods

    protected override void OnShown(EventArgs e)
    {
        if (_owner.Connected) {
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

    private void btnOk_Click(object sender, EventArgs e)
    {

    }

    private void btnCancel_Click(object sender, EventArgs e)
    {

    }
    #endregion
}
}

