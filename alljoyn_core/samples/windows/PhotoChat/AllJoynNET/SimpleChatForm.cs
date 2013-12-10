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
public partial class SimpleChatForm : Form {
    public SimpleChatForm()
    {
        InitializeComponent();
        _connectForm = new AllJoynConnectForm();

        _chatText = new StringBuilder(DateTime.Now.ToString());
        _chatText.AppendLine();
        _transcriptText = new StringBuilder("New Session " + DateTime.Now.ToString());
        _transcriptText.AppendLine();     // EOL

        _connected = false;

        // refresh of the UI and other events related to the UI are managed by this timer
        // see timedEvent() for details
        _timer = new System.Windows.Forms.Timer();
        _timer.Interval = 300;
        _timer.Start();
        _timer.Tick += new EventHandler(timedEvent);
        _timer.Enabled = true;
    }

    private AllJoynConnectForm _connectForm = null;
    private StringBuilder _transcriptText = null;
    private StringBuilder _chatText = null;

    private AllJoynSession _session = null;
    private AllJoynBus _allJoyn = null;

    private System.Windows.Forms.Timer _timer;

    private void timedEvent(Object o, EventArgs e)
    {
        // Xfer
        //if (_pendingQuery)
        //{
        //    _pendingQuery = false;
        //    processQuery();
        //}
        //incrementProgress();
        //_buffer.ShowDeferredTexts();
        //updateSentFiles();
        //
        // Chat
        refreshUI();
    }

    private void refreshUI()
    {
        // update chat
        if (_chatText.Length > 0) {
            txtChat.Text += _chatText.ToString();
            _chatText.Remove(0, _chatText.Length);
            txtChat.ScrollToCaret();
        }

        // update Transcript
        if (_transcriptText.Length > 0) {
            txtTranscript.Text += _transcriptText.ToString();
            _transcriptText.Remove(0, _transcriptText.Length);
            txtTranscript.Select(txtTranscript.Text.Length - 2, 1);
            txtTranscript.ScrollToCaret();
        }
    }

    private void btnClose_Click(object sender, EventArgs e)
    {
        Close();
    }

    private bool _connected = false;

    internal bool Connected { get { return _connected; } }

    private void btnAllJoyn_Click(object sender, EventArgs e)
    {
        if (!_callbacksInstalled) {
            setCallbacks();
        }
        _connectForm.ShowDialog(this);

        if (_connectForm.IsConnected) {
            _allJoyn = _connectForm.AJBus;
            _session = _connectForm.AJSession;
            _identity = _connectForm.MyHandle;
            _connected = true;
            if (!_connectForm.IsNameOwner)
                sendIdentityMessage();
        }
    }

    // callback delegates - interface exported to native code
    private ReceiverDelegate _receiveDelegate;
    private SubscriberDelegate _joinDelegate;

    private bool _callbacksInstalled = false;

    private void setCallbacks()
    {
        if (!_callbacksInstalled) {
            _receiveDelegate = new ReceiverDelegate(receiveOutput);
            _joinDelegate = new SubscriberDelegate(sessionSubscriber);
            GC.KeepAlive(_receiveDelegate);
            GC.KeepAlive(_joinDelegate);
            _connectForm.InstallDelegates(_receiveDelegate, _joinDelegate);
            _callbacksInstalled = true;
        }
    }

    private void receiveOutput(string data, ref int sz, ref int informType)
    {
        string it = informType.ToString() + ":";
        if (informType == 1) {
            if (_allJoyn == null) {
                _chatText.AppendLine(it + data);
                return;
            }
            if (_session == null) {
                _chatText.AppendLine("*** MISSING SESSION *** ");
                _session = new AllJoynSession(_allJoyn);
                //               sendIdentityMessage();
            }
            if (parseForIdentity(data))
                return;             // don't display
            string chatter;
            chatter = lookupChatter(data, out chatter);
            _chatText.AppendLine(chatter + data);
            return;
        }
        _transcriptText.AppendLine(it + data);
    }

    // TODO: new class for handle with proxy support
    private string lookupChatter(string data, out string chatter)
    {
        int start = data.IndexOf(':');
        int stop = data.IndexOf(':', start + 1);
        string key = data.Substring(start, stop - start);
        string msg = data.Remove(start, stop - start);

        if (_session.HasKey(key)) {
            chatter = _session.GetIdentity(key);
        } else
            chatter = "Unknown";
        return msg;
    }


    /*
       // TODO: new class for handle with proxy support
       private string getChattersUID()
       {
        string uid = "";
        foreach (string chatter in _handles.Keys)
        {
            uid = chatter.Substring(0, chatter.Length - 1);
        }
        //            MessageBox.Show('|' + uid + '|');
        return uid;
       }
     */

    const string token = "<IDENTITY:";
    private string _identity = "";

    private void sendIdentityMessage()
    {
        string msg = token + _identity + ">";
        if (_connected && _allJoyn != null) {
            _allJoyn.Send(msg);
            _chatText.AppendLine("*** Sent Identity " + _identity + " **** ");
        }
    }

    private bool parseForIdentity(string data)
    {
        if (data.Contains(token)) {
            //
            //    MessageBox.Show("PARSING " + data);
            int i = data.IndexOf(token);
            if (i > 2) {
                string tmp = data.Replace(token, "<");
                string key = data.Substring(0, i - 2);
                //    MessageBox.Show("KEY = |" + key + "|");
                tmp = tmp.Replace(key, " ");
                tmp = tmp.Substring(1, tmp.Length - 1);
                int start = tmp.IndexOf("<");
                int stop = tmp.IndexOf(">");
                tmp = tmp.Remove(stop);
                start++;
                tmp = tmp.Substring(start, tmp.Length - start);
                //  MessageBox.Show( "TMP = " + tmp);
                if (!_session.HasKey(key)) {
                    _session.NewParticipant(key);
                    _chatText.AppendLine("*** ADD " + key + " **** ");
                    //          sendIdentityMessage();
                } else
                    _chatText.AppendLine("*** Found " + key + " **** ");
                return true;
            }
        }
        //      _chatText.AppendLine("*** NOT FOUND **** ");
        return false;
    }

    private void sessionSubscriber(string data, ref int sz)
    {
        // MessageBox.Show("SUBSCRIBED" + data);
        _chatText.AppendLine("*** SUBSCRIBE **** ");

        if (_session == null)
            _session = new AllJoynSession(_allJoyn);
        _session.NewParticipant(data);
        sendIdentityMessage();
    }

    private string _self = "me";

    private void btnSend_Click(object sender, EventArgs e)
    {
//            _buffer.Add(txtMessage.Text, _tag, TextType.Me);

        if (_connected && _allJoyn != null) {
            string it = _self + ":";
            _allJoyn.Send(txtInput.Text);
            _chatText.AppendLine(it + txtInput.Text);

        }
        txtInput.Text = "";
        txtInput.Update();
        txtInput.Focus();
//            _deferredStatus = "last message sent " + dateTimeString();
    }
}
}
