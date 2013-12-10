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
using System.Collections;
using System.Threading;

namespace WinChat {
public partial class ChatDialog : Form {
    public ChatDialog()
    {
        // added by wizard
        InitializeComponent();

        // private instance variables
        _buffer = new RichTextBuffer(richTextTranscript);
        _handles = new Dictionary<string, string>();
        _setupUI = new AlljoynSetup(this);
        _connected = false;

        _timer = new System.Windows.Forms.Timer();
        _timer.Interval = 300;
        _timer.Start();
        _timer.Tick += new EventHandler(timedEvent);
        _timer.Enabled = true;
    }

    private AlljoynSetup _setupUI = null;
    private AlljoynChatComponant _alljoyn = null;
    private static RichTextBuffer _buffer = null;
    private Dictionary<string, string> _handles;
    private System.Windows.Forms.Timer _timer;
    private string _session = "";
    private string _tag = "";
    private string _deferredStatus = "";
    private bool _connected = false;
    private bool _callbacksInstalled = false;

    internal bool Connected { get { return _connected; } }

    private void timedEvent(Object o, EventArgs e)
    {
        _buffer.ShowDeferredTexts();
    }

    private void Alljoyn_Click(object sender, EventArgs e)
    {
        connectionDialog();
    }

    private void connectionDialog()
    {
        DialogResult res = _setupUI.ShowDialog();
        if (res == DialogResult.OK) {
            if (_connected) {
                _alljoyn.DisconnectAlljoyn();
                _connected = false;
                _buffer.Add("Disconnected...", null, TextType.Status);
            } else {
                _alljoyn = _setupUI.Alljoyn;
                if (_alljoyn == null)
                    MessageBox.Show("Unable to create AlljoynComponent");
                else {
                    if (setCallbacks()) {
                        _session = _setupUI.SessionName.Trim();
                        _tag = _setupUI.MyHandle.Trim();
                        if (_session.Length < 1) {
                            MessageBox.Show("ERROR: Session Name is missing or invalid");
                            _connected = false;
                        } else {
                            _connected = true;
                            if (_tag.Length < 1)
                                _tag = "Unknown";
                            if (_setupUI.IsNameOwner) {
                                _alljoyn.StartChat(_session);
                                _alljoyn.ConnectAlljoyn();
                            } else {
                                _alljoyn.JoinChat(_session);
                                _alljoyn.ConnectAlljoyn();
                                Thread.Sleep(2000);
                                sendIdentityMessage();
                            }
                        }
                    }
                }
            }
        }
        updateButtons();
        _setupUI.Hide();
        txtMessage.Focus();
    }

    private ReceiverDelegate _receiveDelegate;
    private SubscriberDelegate _joinDelegate;

    private bool setCallbacks()
    {
        if (!_callbacksInstalled) {
            _receiveDelegate = new ReceiverDelegate(receive);
            _joinDelegate = new SubscriberDelegate(subscribe);
            GC.KeepAlive(_receiveDelegate);
            GC.KeepAlive(_joinDelegate);
            _alljoyn.RemoteStream(_receiveDelegate);
            _alljoyn.RemoteListener(_joinDelegate);
        }
        return true;
    }

    private void subscribe(string data, ref int sz)
    {
        sendIdentityMessage();
    }

    private void receive(string data, ref int sz, ref int informType)
    {

        string handle = null;
        string msg = null;
        TextType type = TextType.Error;
        switch (informType) {
        case 0:
            type = TextType.Status;
            break;

        case 1:
            type = TextType.Remote;
            if (parseForIdentity(data))
                return;
            handle = lookupChatter(data, out msg);
            data = msg;
            break;

        case 2:
            type = TextType.Error;
            break;

        default:
            type = TextType.Error;
            break;
        }
        _buffer.AddDeferred(data, handle, type);
        _deferredStatus = "last message received " + dateTimeString();
    }

    const string token = "<IDENTITY:";

    private void sendIdentityMessage()
    {
        string msg = token + _tag + ">";
        if (_connected && _alljoyn != null)
            _alljoyn.Send(msg);
    }

    private string lookupChatter(string data, out string ret)
    {
        int start = data.IndexOf(':');
        int stop = data.IndexOf(':', start + 1);
        string key = data.Substring(start, stop - start + 1);
        ret = data.Remove(start, stop - start + 1);
        if (_handles.ContainsKey(key))
            return _handles[key];
        else
            return "Unknown";
    }

    private bool parseForIdentity(string data)
    {
        if (data.Contains(token)) {
            //               MessageBox.Show("PARSING " + data);
            int i = data.IndexOf(token);
            if (i > 1) {
                string tmp = data.Replace(token, "<");
                string key = data.Substring(0, i - 1);
//                    MessageBox.Show("|" + key + "|");
                tmp = tmp.Replace(key, " ");
                tmp = tmp.Substring(1, tmp.Length - 1);
                int start = tmp.IndexOf("<");
                int stop = tmp.IndexOf(">");
                tmp = tmp.Remove(stop);
                start++;
                tmp = tmp.Substring(start, tmp.Length - start);
//                    MessageBox.Show(tmp);
                if (!_handles.ContainsKey(key)) {
                    _handles.Add(key, tmp);
                    sendIdentityMessage();
                }
                return true;
            }
        }
        return false;
    }

    private void updateButtons()
    {
        lblStat.Text = "Status:";
        if (_deferredStatus.Length > 0) {
            lblStatus.Text = _deferredStatus;
            _deferredStatus = "";
            return;
        }
        if (_connected) {
            btnSend.Enabled = true;
            lblStatus.Text = "connected to Alljoyn";
        } else {
            btnSend.Enabled = false;
            lblStatus.Text = "not connected";
        }
    }

    private void Send_Click(object sender, EventArgs e)
    {
        _buffer.Add(txtMessage.Text, _tag, TextType.Me);
        if (_connected && _alljoyn != null)
            _alljoyn.Send(txtMessage.Text);
        txtMessage.Text = "";
        txtMessage.Focus();
        lblStatus.Text = "last message sent " + dateTimeString();
    }

    private string dateTimeString()
    {
        DateTime now = DateTime.Now;
        return now.ToString("f");
    }
}

internal enum TextType {
    Error = 0,
    Status = 1,
    Remote = 2,
    Me = 0x10
}

internal struct TextDescriptor {
    internal int StartPos;
    internal int Length;
    internal TextType TypeText;
    internal bool Bold;
    internal TextDescriptor(TextType typeText, int start, int len, bool bold)
    {
        StartPos = start;
        Length = len;
        TypeText = typeText;
        Bold = bold;
    }
}

internal class TextChunk {
    internal TextDescriptor Attributes;
    internal string Text;

    internal TextChunk(string text, int start, TextType originator, bool bold)
    {
        Attributes = new TextDescriptor(originator, start, text.Length, bold);
        Text = text;
    }
}

internal class RichTextBuffer {
    private RichTextBox _control;
    private ArrayList _contents;
    private Queue<TextChunk> _deferred;
    internal int InsertionPoint = 0;

    internal RichTextBuffer(RichTextBox owner)
    {
        _control = owner;
        _control.ShowSelectionMargin = true;
        _control.ScrollBars = RichTextBoxScrollBars.ForcedVertical;
        _contents = new ArrayList();
        _deferred = new Queue<TextChunk>();
        InsertionPoint = 0;
    }

    internal void AddDeferred(string text, string tag, TextType type)
    {
        lock (_deferred)
        {
            if (tag != null&& tag.Length > 0) {
                tag += ": ";
                _deferred.Enqueue(new TextChunk(tag, InsertionPoint, type, true));
                InsertionPoint += tag.Length;
            }
            if (type != TextType.Remote)
                text += '\n';
            _deferred.Enqueue(new TextChunk(text, InsertionPoint, type, false));
            InsertionPoint += text.Length;
        }
    }

    internal void ShowDeferredTexts()
    {
        if (_deferred.Count == 0)
            return;
        lock (_deferred)
        {
            foreach (TextChunk t in _deferred) {
                _contents.Add(t);
                updateControl(t);
            }
            _deferred.Clear();
        }
    }

    internal void Add(string text, string tag, TextType type)
    {
        ShowDeferredTexts();
        if (tag != null&& tag.Length > 0) {
            tag += ": ";
            _contents.Add(new TextChunk(tag, InsertionPoint, type, true));
            InsertionPoint += tag.Length;
            updateControl((TextChunk)_contents[_contents.Count - 1]);
        }
        if (type != TextType.Remote)
            text += '\n';
//            else
//            {
//                MessageBox.Show(text);
//            }
        _contents.Add(new TextChunk(text, InsertionPoint, type, false));
        InsertionPoint += text.Length;
        updateControl((TextChunk)_contents[_contents.Count - 1]);
    }

    private void updateControl(TextChunk chunk)
    {
        _control.AppendText(chunk.Text);
        _control.Select(chunk.Attributes.StartPos, chunk.Attributes.Length);
        switch (chunk.Attributes.TypeText) {
        case TextType.Error:
            _control.SelectionColor = Color.Red;
            break;

        case TextType.Status:
            _control.SelectionColor = Color.Black;
            break;

        case TextType.Remote:
            _control.SelectionColor = Color.Green;
            break;

        case TextType.Me:
            _control.SelectionColor = Color.Blue;
            break;

        default:
            _control.SelectionColor = Color.Black;
            break;
        }
        Font font = _control.SelectionFont;
        if (chunk.Attributes.Bold)
            _control.SelectionFont = new Font(font.FontFamily, font.Size, FontStyle.Bold);
        else
            _control.SelectionFont = new Font(font.FontFamily, font.Size, FontStyle.Regular);
        _control.Select(_control.Text.Length - 1, 1);
        _control.ScrollToCaret();
        _control.Update();
    }
}
}
