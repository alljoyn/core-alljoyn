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

namespace WinChat {
public partial class AlljoynSetup : Form {
    public AlljoynSetup(ChatDialog owner)
    {
        _owner = owner;
        InitializeComponent();
        this.Hide();
        _alljoyn = new AlljoynChatComponant();
        InterfaceName = _alljoyn.InterfaceName;
        NamePrefix = _alljoyn.NamePrefix;
        ObjectPath = _alljoyn.ObjectPath;
    }

    public AlljoynChatComponant Alljoyn { get { return _alljoyn; } }
    public string SessionName { get { return txtSession.Text; } }
    public string MyHandle { get { return txtHandle.Text; } }
    public bool IsNameOwner { get { return rbAdvertise.Checked; } }

    private AlljoynChatComponant _alljoyn = null;
    private ChatDialog _owner = null;

    internal string InterfaceName = "";
    internal string NamePrefix = "";
    internal string ObjectPath = "";

    protected override void OnShown(EventArgs e)
    {
        if (_owner.Connected) {
            btnOk.Text = "Disconnect";
            txtSession.Enabled = false;
            txtHandle.Enabled = false;
            rbAdvertise.Enabled = false;
            rbJoin.Enabled = false;

        } else {
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
}

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void ReceiverDelegate(string data, ref int size, ref int type);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void SubscriberDelegate(string data, ref int size);

public class AlljoynChatComponant {
    public AlljoynChatComponant()
    {
        char[] charbuffer = new char[1024];

        charbuffer.Initialize();
        int sz = charbuffer.Length;
        GetInterfaceName(charbuffer, ref sz);
        _iface = new string(charbuffer);
        _iface = _iface.Remove(sz);

        charbuffer.Initialize();
        sz = charbuffer.Length;
        GetNamePrefix(charbuffer, ref sz);
        _prefix = new string(charbuffer);
        _prefix = _prefix.Remove(sz);

        charbuffer.Initialize();
        sz = charbuffer.Length;
        GetObjectPath(charbuffer, ref sz);
        _objectPath = new string(charbuffer);
        _objectPath = _objectPath.Remove(sz);
    }

    internal void ConnectAlljoyn()
    {
        Connect();
    }

    internal void DisconnectAlljoyn()
    {
    }

    internal void StartChat(string chatName)
    {
        int charCount = chatName.Length;
        SetupChat(chatName.ToCharArray(), true, ref charCount);
    }

    internal void JoinChat(string chatName)
    {
        int charCount = chatName.Length;
        SetupChat(chatName.ToCharArray(), false, ref charCount);
    }

    internal void Send(string msg)
    {
        int charCount = msg.Length;
        if (charCount < 1)
            return;
        char[] mchars = msg.ToCharArray();
        MessageOut(mchars, ref charCount);;
    }

    internal void RemoteStream(ReceiverDelegate callback)
    {
        SetOutStream(callback);
    }

    internal void RemoteListener(SubscriberDelegate callback)
    {
        SetListener(callback);
    }

    [DllImport("DaemonLib.dll", EntryPoint = "SetLogFile", CharSet = CharSet.Unicode)]
    internal extern static void SetLogFile(String path);
    private string _iface = "";


    private string _prefix = "";
    private string _objectPath = "";

    // Properties
    public string InterfaceName { get { return _iface; } }
    public string NamePrefix { get { return _prefix; } }
    public string ObjectPath { get { return _objectPath; } }


    [DllImport(@"ChatLib32.dll")]
    public static extern void GetInterfaceName([Out] char[] arg, [In, Out] ref int size);
    [DllImport(@"ChatLib32.dll")]
    public static extern void GetNamePrefix([Out] char[] arg, [In, Out] ref int size);
    [DllImport(@"ChatLib32.dll")]
    public static extern void GetObjectPath([Out] char[] arg, [In, Out] ref int size);

    // Function
    [DllImport(@"ChatLib32.dll")]
    public static extern void SetOutStream([MarshalAs(UnmanagedType.FunctionPtr)] ReceiverDelegate callBack);
    [DllImport(@"ChatLib32.dll")]
    public static extern void Connect();
    [DllImport(@"ChatLib32.dll")]
    public static extern void SetupChat([In] char[] chatName, bool asAdvertiser, ref int size);
    [DllImport(@"ChatLib32.dll")]
    public static extern void SetListener([MarshalAs(UnmanagedType.FunctionPtr)] SubscriberDelegate callBack);
    [DllImport(@"ChatLib32.dll")]
    public static extern void MessageOut([In] char[] msg, ref int size);
}
}
