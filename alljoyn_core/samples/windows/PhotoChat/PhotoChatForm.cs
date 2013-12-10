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
using System.IO;

using AllJoynNET;
using System.Collections;
using System.Threading;

namespace PhotoChat {
public partial class PhotoChatForm : Form {
    public PhotoChatForm()
    {
        InitializeComponent();
        _allJoyn = new AllJoynBus();
        _connectForm = new AllJoynConnectForm(_allJoyn, this);
        _transcriptForm = new SimpleTranscript();
        _buffer = new RichTextBuffer(richTextOutput);
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

        // large thumbnails
        imageList1.ImageSize = new Size(64, 64);
        imageList1.TransparentColor = Color.White;

        // small thumbnails
        _largeList = new ImageList();
        _largeList.ImageSize = new Size(32, 32);
        _largeList.TransparentColor = Color.White;

        lvPic.LargeImageList = _largeList;
        lvPic.SmallImageList = _largeList;
        lvPic.View = View.Tile;

        richTextOutput.AllowDrop = true;
        richTextOutput.DragEnter += richTextOutput_DragEnter;
        richTextOutput.DragDrop += richTextOutput_DragDrop;

    }

    private bool _connected = false;

    public bool Connected { get { return _connected; } }

    private AllJoynConnectForm _connectForm = null;

    private bool _transcriptVisible = false;
    private SimpleTranscript _transcriptForm;

    private StringBuilder _transcriptText = null;
    private StringBuilder _chatText = null;

    private AllJoynSession _session = null;
    private AllJoynBus _allJoyn = null;

    private System.Windows.Forms.Timer _timer;

    private static RichTextBuffer _buffer = null;

    private bool _selectingChatters = false;

    private ArrayList _recipients = null;

    private void timedEvent(Object o, EventArgs e)
    {
        if (_selectingChatters) {
            if (_recipients != null) {
                if (_recipients.Count > 0) {
                    processRecipients();
                } else
                    _recipients = null;
                pnlChatters.Visible = false;
                _selectingChatters = false;
            }
        }
        // Xfer
        if (_pendingQuery) {
            _pendingQuery = false;
            processQuery();
        }
        incrementProgress();
        updateSentFiles();
        refreshUI();
    }

    private bool _showProgress = false;
    private int _progressAmount = 0;
    private int _progressIncr = 0;

    private void incrementProgress()
    {
        if (!_showProgress) {
            progressBar1.Visible = true;
            _progressAmount += _progressIncr;
            return;
        }
        if ((_progressAmount == 0) && (_progressIncr == 0))
            return;
        progressBar1.Visible = true;
        _progressAmount += _progressIncr;
        if (_progressAmount > 100)
            _progressAmount = 100;
        progressBar1.Value = _progressAmount;
    }

    private Thread _sendThread = null;

    private void processRecipients()
    {
        if (_recipients.Count < 1) {
            _recipients = null;
            return;
        }
        _sendThread = new Thread(sendToRecipients);
        _sendThread.Start();
    }

    private void sendToRecipients()
    {
        _showProgress = true;
        foreach (string chatter in _recipients) {
            string UID = _session.FindUID(chatter);
            int proxy = _session.CreateProxy(UID);
            if (proxy == -1) {
                string msg = "INVALID PROXY" + proxy.ToString();
                int sz = msg.Length;
                int type = 0;     // error type
                receiveOutput(msg, ref sz, ref type);
            } else   {
                _progressAmount = 0;
                sendFilesTo(chatter, proxy);
                _session.FreeProxy(UID, proxy);
            }
        }
        _sendThread.Abort();
        _sendThread = null;
        _showProgress = false;
    }

    private string[] _filesToSend = null;

    private void sendFilesTo(string chatter, int proxy)
    {
        foreach (string file in _filesToSend) {
            sendFile(file, chatter, proxy);
        }
    }

    private void refreshUI()
    {
        _buffer.ShowDeferredTexts();
        if (_deferredStatus.Length > 0) {
            lblStart.Text = "Status:";
            lblStatus.Text = _deferredStatus;
            _deferredStatus = "";
        }
        if (_transcriptText.Length > 0) {
            _transcriptForm.AddText(_transcriptText.ToString());
            _transcriptText.Remove(0, _transcriptText.Length);
        }
        if (!_showProgress) {
            _progressAmount = 0;
            progressBar1.Visible = false;
        }
        if ((_session == null) || (lvPic.Items.Count < 1) || (_session.Count == 0))
            btnShare.Enabled = false;
        else
            btnShare.Enabled = true;
        updateSentFiles();
    }

    private string _deferredStatus = "";

    private void updateMainForm()
    {
        lblStart.Text = "Status:";
        if (_deferredStatus.Length > 0) {
            lblStatus.Text = _deferredStatus;
            _deferredStatus = "";
            return;
        }
        if (_connected) {
            btnSend.Enabled = true;
            lblStatus.Text = "connected to Alljoyn";
        } else   {
            btnSend.Enabled = true;
            lblStatus.Text = "not connected";
        }
    }

    // callback delegates - interface exported to native code
    private ReceiverDelegate _receiveDelegate;
    private SubscriberDelegate _joinDelegate;
    private IncomingQueryDelegate _queryDelegate;
    private IncomingXferDelegate _xferDelegate;

    private bool _pendingQuery = false;
    private bool _pendingResult = false;
    private string _query = "";
    private int _queryResult = 0;

    private string _incomingFile;
    private int _incomingFilesize;

    //      private bool _callbacksInstalled = false;

    private void setCallbacks()
    {
        _receiveDelegate = new ReceiverDelegate(receiveOutput);
        _joinDelegate = new SubscriberDelegate(sessionSubscriber);
        _queryDelegate = new IncomingQueryDelegate(incomingQuery);
        _xferDelegate = new IncomingXferDelegate(xferHandler);
        GC.KeepAlive(_queryDelegate);
        GC.KeepAlive(_xferDelegate);
        GC.KeepAlive(_receiveDelegate);
        GC.KeepAlive(_joinDelegate);
    }

    private void incomingQuery(string data, ref int accept)
    {
        _query = data;
        _queryResult = accept;
        _pendingQuery = true;
        _pendingResult = true;

        // TODO: wrap this in a timeout
        while (_pendingQuery) {
            Thread.Sleep(600);
        }
        while (_pendingResult) {
            Thread.Sleep(600);
        }
        accept = _queryResult;
        // if timeout accept = 0;
    }

    // this query (set by incomingQuery above) will be called from the main timer loop in the UI thread
    private void processQuery()
    {
        // TODO: replace "Someone" with the senders tag
        string incoming = "Someone wants to send you " + _query;
        incoming += "\nFile size is " + _queryResult.ToString() + " bytes";
        incoming += "\nWill you accept?";
        if (MessageBox.Show(incoming, "Alljoyn Photo Chat", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == System.Windows.Forms.DialogResult.Yes) {
            _queryResult = initiateXferIn(_query, _queryResult);
        } else
            _queryResult = 0;
        _pendingResult = false;
    }

    private void xferHandler(string command, ref int sz)
    {
        switch (command) {
        case "Initiate":
            _showProgress = true;
            _progressIncr = 10;
            break;

        case "Receive":
            break;

        case "Close":
            //              _showProgress = false;
            _progressIncr = 0;
            break;

        default:
            break;
        }
    }

    private int initiateXferIn(string filename, int filesize)
    {
        SaveFileDialog dlg = new SaveFileDialog();
        dlg.FileName = _query;
        DialogResult res = dlg.ShowDialog();
        if (res == DialogResult.OK) {
            _incomingFile = dlg.FileName;
            addFileToListView(_incomingFile);
            _allJoyn.SetPendingInputFilename(_incomingFile);
            // TODO: set queryResult to a recommended sector size
            return _queryResult;
        }
        return 0;
    }

    private string dateTimeString()
    {
        DateTime now = DateTime.Now;
        return now.ToString("f");
    }

    private void receiveOutput(string data, ref int sz, ref int informType)
    {
        string handle = null;
        string msg = null;
        TextType type = TextType.Error;
        if (_session == null) {
            if (_allJoyn == null)
                _allJoyn = _connectForm.AJBus;
            _session = new AllJoynSession(_allJoyn);
            if (!_connectForm.IsNameOwner)
                sendIdentityMessage();
        }
        switch (informType) {
        case 0:
            type = TextType.Error;
            _buffer.AddDeferred(data, "Error", type);
            _transcriptText.AppendLine(data);
            break;

        case 1:
            type = TextType.Remote;
            if (parseForIdentity(data))
                return;
            msg = lookupChatter(data, out handle);
            data = msg;
            _buffer.AddDeferred(data, handle, type);
            _deferredStatus = "last message received " + dateTimeString();
            break;

        case 2:
            type = TextType.Status;
            _transcriptText.AppendLine(data);
            break;

        case 3:
            type = TextType.System;
            _transcriptText.AppendLine(data);
            break;

        default:
            type = TextType.Error;
            _buffer.AddDeferred(data, "Error", type);
            _transcriptText.AppendLine(data);
            break;
        }
    }

    /***
       {
        string it = informType.ToString() + ":";
        if (informType == 1)
        {

            if (parseForIdentity(data))
                return;

            if (_allJoyn == null)
            {
                _chatText.AppendLine(it + data);
                return;
            }

            if (parseForIdentity(data))
                return;     // don't display
            string chatter;
            chatter = lookupChatter(data, out chatter);
            _chatText.AppendLine(chatter + data);
            return;
        }
        _transcriptText.AppendLine(data);
       }
     ***/


    const string token = "<IDENTITY:";
    private string _identity = "";

    private void sendIdentityMessage()
    {
        string msg = token + _identity + ">";
        if (_connected && _allJoyn != null) {
            _allJoyn.Send(msg);
            _buffer.AddDeferred("*** Sent Identity ", _identity, TextType.Remote);
        }
    }

    private bool parseForIdentity(string data)
    {
        if (data.Contains(token)) {
            int i = data.IndexOf(token);
            if (i > 2) {
                string tmp = data.Replace(token, "<");
                string key = data.Substring(0, i - 2);
                tmp = tmp.Replace(key, " ");
                tmp = tmp.Substring(1, tmp.Length - 1);
                int start = tmp.IndexOf("<");
                int stop = tmp.IndexOf(">");
                tmp = tmp.Remove(stop);
                start++;
                tmp = tmp.Substring(start, tmp.Length - start);
                if (_session == null)
                    _session = new AllJoynSession(_allJoyn);
                if (!_session.HasKey(key)) {
                    _session.NewParticipant(key, tmp);
                    _transcriptText.AppendLine("*** ADD " + key + " **** ");
                    sendIdentityMessage();
                } else   {
                    _session.AddIdentity(key, tmp);
                }

                _buffer.AddDeferred("IDENTIFIED " + tmp + " AS " + key, tmp, TextType.Error);
                return true;
            }
        }
        return false;
    }

    private string lookupChatter(string data, out string chatter)
    {
        int start = data.IndexOf(':');
        int stop = data.IndexOf(':', start + 1);
        string key = data.Substring(start, stop - start);
        string msg = data.Remove(start, (stop - start) + 1);

        if (_session.HasKey(key)) {
            chatter = _session.GetIdentity(key);
        } else
            chatter = "Unknown";
        return msg;
    }

    private void sessionSubscriber(string data, ref int sz)
    {

        if (_session == null)
            _session = new AllJoynSession(_allJoyn);
        _session.NewParticipant(data);
        Thread.Sleep(1000);
        sendIdentityMessage();
    }

    private string _outgoingFilename = "";

    private void sendFile(string filename, string chatter, int proxy)
    {
        if (_session.Count < 1)
            return;
        if (_connected && _allJoyn != null) {
            // determine nSegmentSize and nSegments
            FileInfo info = new FileInfo(filename);
            long size = info.Length;
            if (size > int.MaxValue) {
                MessageBox.Show("That file is huge! ... sorry can't send it.", "WHOA!");
                return;
            }
            string path = filename;
            _outgoingFilename = path;
            string[] tokens = path.Split('\\');
            int index = tokens.Length;
            System.Diagnostics.Debug.Assert(index > 0);
            string file = tokens[index - 1];

            int sz = (int)size;
            if (_allJoyn.OutgoingQueryXfer(proxy, file, sz)) {
                _showProgress = true;
                _progressIncr = 10;
            }
            if (initiateXferOut(proxy))
                addToSentFiles(_outgoingFilename);
        }
    }

    long calculate(string filename)
    {
        // determine nSegmentSize and nSegments
        FileInfo info = new FileInfo(filename);
        long size = info.Length;
        _nSegments = (int)size / 65435;
        _segmentSize = 65435;
        _lastSegSize = (int)size % 65435;
        if (_lastSegSize > 0)
            _nSegments++;
        return size;
    }

    private int _nSegments = 0;
    private int _segmentSize = 0;
    private int _lastSegSize = 0;

    private bool initiateXferOut(int proxyIndex)
    {
        calculate(_outgoingFilename);
        _allJoyn.InitiateOutgoingXfer(proxyIndex, _segmentSize, _nSegments);
        FileStream stream = new FileStream(_outgoingFilename, FileMode.Open, FileAccess.Read);
        bool res = xferSegments(stream, proxyIndex);
        if (!res)
            MessageBox.Show("TRANSFER FAILED");
        stream.Close();
        _allJoyn.EndOutgoingTransfer(proxyIndex);
        _progressAmount = 0;
        return res;
    }

    private bool xferSegments(FileStream stream, int proxyIndex)
    {
        //            if( !_active )
        //                return false;
        byte[] bytes = new byte[_segmentSize];
        for (int i = 0; i < _nSegments; i++) {
            int segSize = _segmentSize;
            if (i == _nSegments - 1)
                segSize = _lastSegSize;
            int offset = i * _segmentSize;
            try{
                stream.Seek(offset, SeekOrigin.Begin);
                stream.Read(bytes, 0, segSize);
            }catch{
                return false;
            }
            _allJoyn.TransferSegmentOut(proxyIndex, bytes, i + 1, segSize);
        }
        return true;
    }

    private void showChatterList()
    {
        ArrayList list = _session.IdentityList();
        if (list.Count == 1)
            return;
        foreach (AllJoynRemoteObject remote in list) {
            string id = remote.Identity;
            clbChatters.Items.Add(id);
        }
        pnlChatters.Visible = true;
        pnlChatters.Focus();
    }

    private void processDirectory(string targetDirectory)
    {
        // Process the list of files found in the directory.
        if (!Directory.Exists(targetDirectory))
            return;
        string[] fileEntries = Directory.GetFiles(targetDirectory);
        if (fileEntries.Length < 1)
            return;
        foreach (string fileName in fileEntries) {
            addFileToListView(fileName);
        }
    }

    private ImageList _largeList;

    private void addFileToListView(string fileName)
    {
        Image img = null;
        try{
            img = Image.FromFile(fileName).GetThumbnailImage(64, 64, null, IntPtr.Zero);
        }catch{
            img = null;
        }
        if (img != null) {
            ListViewItem item = new ListViewItem();
            item.Text = fileName;
            item.Tag = fileName;
            item.ImageKey = item.Tag.ToString();
            _largeList.Images.Add(item.Tag.ToString(), img);
            lvPic.Items.Add(item);
        }
    }

    private void addToSentFiles(string filename)
    {
        Image img = null;
        try{
            img = Image.FromFile(filename).GetThumbnailImage(64, 64, null, IntPtr.Zero);
        }catch{
            img = null;
        }
        if (img != null) {
            imageList1.Images.Add(filename, img);
        }
    }

    private void updateSentFiles()
    {
        if (imageList1.Images.Count < 1)
            return;
        Graphics graf = Graphics.FromHwnd(panel1.Handle);
        int currentImage = 0;
        int x = 10;
        int y = 10;
        foreach (Image image in imageList1.Images) {
            imageList1.Draw(graf, x, y, currentImage++);
            x += 68;
            //        panel1.Refresh();
        }
        graf.Dispose();
        //            this.Invalidate();
    }

    #region Button Handlers

    private void btnAllJoyn_Click(object sender, EventArgs e)
    {
        setCallbacks();
        _connectForm.InstallDelegates(_receiveDelegate, _joinDelegate);
        _connectForm.ShowDialog(this);
        if (_connectForm.IsConnected) {
            _identity = _connectForm.MyHandle;
            _connected = true;
            if (!_connectForm.IsNameOwner) {
                Thread.Sleep(1000);
                sendIdentityMessage();
            }
            _allJoyn.SetLocalIncomingXferInterface(_queryDelegate, _xferDelegate);
            updateMainForm();
        }
    }

    private void btnTranscript_Click(object sender, EventArgs e)
    {
        if (_transcriptVisible) {
            _transcriptForm.Hide();
            _transcriptVisible = false;
            btnTranscript.Text = "Show Transcript";
        } else   {
            _transcriptForm.Show();
            _transcriptVisible = true;
            btnTranscript.Text = "Hide Transcript";
        }
    }

    private void btnSend_Click(object sender, EventArgs e)
    {
        _buffer.Add(txtMessage.Text, _identity, TextType.Me);
        if (_connected && _allJoyn != null)
            _allJoyn.Send(txtMessage.Text);
        txtMessage.Text = "";
        txtMessage.Focus();
        _deferredStatus = "last message sent " + dateTimeString();
    }

    private void btnAddFiles_Click(object sender, EventArgs e)
    {
        OpenFileDialog dlg = new OpenFileDialog();
        dlg.Filter = "Images (*.BMP;*.JPG;*.GIF)|*.BMP;*.JPG;*.GIF|" + "All files (*.*)|*.*";
        // Allow the user to select multiple images.
        dlg.Multiselect = true;
        dlg.Title = "Alljoyn Photo Share";
        DialogResult res = dlg.ShowDialog(this);
        if (res == DialogResult.OK) {
            string[] files = dlg.FileNames;
            foreach (string file in files)
                addFileToListView(file);
        }
    }

    private void btnAddFolder_Click(object sender, EventArgs e)
    {
        dlgBrowseFolders.ShowDialog();
        //       MessageBox.Show(dlgBrowseFolders.SelectedPath);
        if (dlgBrowseFolders.SelectedPath.Length < 1)
            return;
        processDirectory(dlgBrowseFolders.SelectedPath);
    }

    private void btnShare_Click(object sender, EventArgs e)
    {
        if (_session == null)
            return;
        determineFileRecipients();
    }

    private void determineFileRecipients()
    {
        if (lvPic.SelectedItems.Count < 1) {
            MessageBox.Show("NO FILES SELECTED");
            return;
        }
        _filesToSend = new string[lvPic.SelectedItems.Count];
        int i = 0;
        foreach (ListViewItem item in lvPic.SelectedItems) {
            string filename = item.Text;
            _filesToSend[i++] = filename;
        }
        ArrayList list = _session.IdentityList();
        if (list.Count > 1) {
            clbChatters.Items.Clear();
            foreach (AllJoynRemoteObject remote in list) {
                string id = remote.Identity;
                clbChatters.Items.Add(id);
            }
            pnlChatters.Visible = true;
        } else   {
            _recipients = new ArrayList();
            AllJoynRemoteObject remote = (AllJoynRemoteObject)list[0];
            _recipients.Add(remote.Identity);
        }
        _selectingChatters = true;
    }

    private void btnSendFile_Click(object sender, EventArgs e)
    {
        ArrayList arr = new ArrayList();
        foreach (object itemChecked in clbChatters.CheckedItems) {
            arr.Add(itemChecked.ToString());
        }
        _recipients = arr;
    }
    #endregion

    #region DragAndDrop
    //-----------------------------------------------------------------------------------------

    private void richTextOutput_DragEnter(object sender, DragEventArgs e)
    {
        // If the data is text, copy the data to the RichTextBox control.
        if (e.Data.GetDataPresent("Text") || (e.Data.GetDataPresent("Rtf")))
            e.Effect = DragDropEffects.Copy;
    }

    private void richTextOutput_DragDrop(object sender, DragEventArgs e)
    {
        string file = (string)e.Data.GetData("Text");
        {
            if (_session == null)
                return;
            determineFileRecipients();
        }
    }

    private void imbed(string file)
    {

        Bitmap bm = new Bitmap(file);
        // Copy the bitmap to the clipboard.
        Clipboard.SetDataObject(bm);
        // Get the format for the object type.
        DataFormats.Format myFormat = DataFormats.GetFormat(DataFormats.Bitmap);
        // After verifying that the data can be pasted, paste
        if (richTextOutput.CanPaste(myFormat)) {
            try{
                richTextOutput.Paste(myFormat);
            }catch{
            }
        }
    }

    private void lvPic_MouseDown(object sender, MouseEventArgs e)
    {
        // Determines which item was selected.
        ListView lv = ((ListView)sender);
        Point cp = base.PointToClient(new Point(e.X, e.Y));
        ListViewItem item = lv.GetItemAt(e.X, e.Y);
        // Starts a drag-and-drop operation with that item.
        if (item != null) {
            lv.DoDragDrop(item.Text, DragDropEffects.All);
        }
    }

    #endregion

}
}
