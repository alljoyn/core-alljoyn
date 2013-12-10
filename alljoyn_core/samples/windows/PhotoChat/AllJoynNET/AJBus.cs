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
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections;

namespace AllJoynNET {
#region     // general client helper classes
public class AllJoynRemoteObject {
    public string UID { get { return _uid; } set { _uid = value; } }
    private string _uid = "";

    public string Identity { get { return _id; } set { _id = value; } }
    private string _id = "";

    public int ProxyIndex { get { return _proxy; } set { _proxy = value; } }
    private int _proxy = -1;

    public AllJoynRemoteObject(string uid, string identity)
    {
        _uid = uid;
        _id = identity;
    }

    public AllJoynRemoteObject(string uid)
    {
        _uid = uid;
        _id = nextUID();
    }

    private static int _next = 0;
    private string nextUID()
    {
        _next++;
        string uid = "Anonymous(";
        uid += _next.ToString() + ")";
        return uid;
    }
}

public class AllJoynSession {
    public AllJoynSession(AllJoynBus bus)
    {
        _ajBus = bus;
        _handles = new Dictionary<string, AllJoynRemoteObject>();
    }

    public Int32 Count { get { return _handles.Count; } }

    private AllJoynBus _ajBus;

    public bool NewParticipant(string UID)
    {
        if (_handles.Keys.Contains(UID))
            return false;
        AllJoynRemoteObject p = new AllJoynRemoteObject(UID);
        _handles.Add(UID, p);
        return true;
    }

    public bool NewParticipant(string UID, string identity)
    {
        if (_handles.Keys.Contains(UID))
            return false;
        AllJoynRemoteObject p = new AllJoynRemoteObject(UID, identity);
        _handles.Add(UID, p);
        AddIdentity(UID, identity);
        return true;
    }

    public void AddIdentity(string key, string ident)
    {
        if (_handles.Keys.Contains(key)) {
            AllJoynRemoteObject obj = null;
            obj = _handles[key];
            obj.Identity = ident;
        }
    }

    public bool HasKey(string key)
    {
        return _handles.Keys.Contains(key);
    }

    public string GetIdentity(string UID)
    {
        if (!_handles.Keys.Contains(UID))
            NewParticipant(UID);
        if (!_handles.Keys.Contains(UID))
            return ("UNKNOWN UID");
        return _handles[UID].Identity;
    }

    public int CreateProxy(string UID)
    {
        string name = UID;
        if (!_handles.Keys.Contains(UID))
            return -1;
        AllJoynRemoteObject proxy = _handles[UID];
        proxy.ProxyIndex = _ajBus.CreateProxyIndex(name);
        return proxy.ProxyIndex;
    }


    public void FreeProxy(string UID, int proxyIndex)
    {
        if (!_handles.Keys.Contains(UID))
            return;
        AllJoynRemoteObject proxy = _handles[UID];
        _ajBus.ReleaseProxyIndex(UID, proxy.ProxyIndex);
        proxy.ProxyIndex = -1;
    }


    public string FindUID(string ident)
    {
        foreach (string uid in _handles.Keys) {
            if (_handles[uid].Identity == ident)
                return uid;
        }
        return "Unknown";
    }

    public ArrayList IdentityList()
    {
        ArrayList list = new ArrayList();
        foreach (AllJoynRemoteObject remote in _handles.Values) {
            list.Add(remote);
        }
        return list;
    }

    private Dictionary<string, AllJoynRemoteObject> _handles;

}

#endregion

//-----------------------------------------------------------------------
[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void ReceiverDelegate(string data, ref int size, ref int type);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void SubscriberDelegate(string data, ref int size);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void IncomingQueryDelegate(string data, ref int accept);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void IncomingXferDelegate(string command, ref int serial);

//-----------------------------------------------------------------------
/// <summary>
/// Provides a managed object with access to the native AllJoyn.Lib
///
/// </summary>
public class AllJoynBus {
    public AllJoynBus() { }

    public string NamePrefix { get { return ((_prefix.Length == 0) ? fetchPrefix() : _prefix); } }

    public bool Connect(string ident, ref bool asAdvertiser)
    {
        bool aa = asAdvertiser;
        ConnectToAllJoyn(ident.ToCharArray(), ref aa);
        return aa;
    }

    public void Disconnect()
    {
        DisconnectFromAllJoyn();
    }

    public void SetOutputStream(ReceiverDelegate callback)
    {
        SetLocalOutputStream(callback);
    }

    public void SetLocalListener(SubscriberDelegate callback)
    {
        SetJoinListener(callback);
    }

    public int CreateProxyIndex(string name)
    {
        int index = -1;
        CreateXferProxyFor(name.ToCharArray(), ref index);
        return index;
    }

    public bool ReleaseProxyIndex(string name, int index)
    {
        int pi = index;
        ReleaseXferProxy(name.ToCharArray(), ref pi);
        return true;
    }

    private string _prefix = "";
    private string fetchPrefix()
    {
        int sz = 0;
        char[] chars = new char[1024];
        chars.Initialize();
        sz = chars.Length;
        GetNamePrefix(chars, ref sz);
        string ret = new string(chars);
        ret = ret.Remove(sz);
        return ret;
    }

    public void Send(string p)
    {
        int len = p.Length;
        if (len > 0) {
            char[] arg = p.ToCharArray();
            MessageOut(arg, ref len);
        }
    }

    // XferObject
    public void SetLocalIncomingXferInterface(IncomingQueryDelegate qcb, IncomingXferDelegate xcb)
    {
        SetIncomingXferInterface(qcb, xcb);
    }

    // XferObject (Out)
    public bool OutgoingQueryXfer(int proxyIndex, string filename, int filesize)
    {
        int accept = 0;
        QueryRemoteXfer(proxyIndex, filename.ToCharArray(), ref filesize, ref accept);
        return (accept != 0);
    }
    public bool InitiateOutgoingXfer(int proxyIndex, int segmentSize, int nSegments)
    {
        bool success = false;
        InitiateXfer(proxyIndex, segmentSize, nSegments, ref success);
        return success;
    }
    public bool TransferSegmentOut(int proxyIndex, byte[] bytes, int serialNum, int segmentSize)
    {
        bool success = false;
        Console.WriteLine("Proxy " + proxyIndex.ToString());
        Console.WriteLine("Serial " + serialNum.ToString());
        Console.WriteLine("Size " + segmentSize.ToString());
        TransferSegment(proxyIndex, bytes, serialNum, segmentSize, ref success);
        return success;
    }
    public string RemoteTransferStatus(int proxyIndex)
    {
        int state = 0;
        int errCode = 0;
        string str = "Status (" + proxyIndex.ToString();
        str += ") ";
        GetRemoteTransferStatus(proxyIndex, ref state, ref errCode);
        switch (state) {
        case 0:
            str += " Idle";
            break;

        case 1:
            str += " Busy";
            break;

        case -1:
            str += " ERROR ";
            str += " code = " + errCode.ToString();
            break;

        default:
            str += " UNKNOWN STATE???? ";
            break;
        }
        return str;
    }
    public bool EndOutgoingTransfer(int proxyIndex)
    {
        bool success = false;
        EndRemoteTransfer(proxyIndex, ref success);
        return success;
    }

    // XferObject (In)
    public void SetPendingInputFilename(string filename)
    {
        bool success = false;
        SetPendingTransferIn(filename.ToCharArray(), ref success);
    }

    //-----------------------------------------------------------------------
    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void GetNamePrefix([Out] char[] arg, [In, Out] ref int size);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void SetLocalOutputStream([MarshalAs(UnmanagedType.FunctionPtr)] ReceiverDelegate callBack);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void SetJoinListener([MarshalAs(UnmanagedType.FunctionPtr)] SubscriberDelegate callBack);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void ConnectToAllJoyn([In] char[] identity, ref bool asAdvertiser);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void DisconnectFromAllJoyn();

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void MessageOut(char[] arg, ref int maxchars);

    //-----------------------------------------------------------------------
    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void SetIncomingXferInterface(
        [MarshalAs(UnmanagedType.FunctionPtr)] IncomingQueryDelegate qcb,
        [MarshalAs(UnmanagedType.FunctionPtr)] IncomingXferDelegate xcb);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void CreateXferProxyFor([In] char[] name, ref int index);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void ReleaseXferProxy(char[] name, ref int index);

    //-----------------------------------------------------------------------
    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void QueryRemoteXfer([In] int index, [In] char[] filename, ref int filesize, ref int accept);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void InitiateXfer([In] int proxyIndex, int segmentSize, int nSegments, ref bool success);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void TransferSegment([In] int proxyIndex, [In, Out] byte[] bytes, int serial, int segSize, ref bool success);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void SetPendingTransferIn([In] char[] filename, ref bool success);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void EndRemoteTransfer([In] int proxyIndex, ref bool success);

    [DllImport(@"AllJoynBusLib.dll")]
    private static extern void GetRemoteTransferStatus([In] int proxyIndex, ref int state, ref int errorCode);
    // 0 - available 1 - busy -1 error
}
}
