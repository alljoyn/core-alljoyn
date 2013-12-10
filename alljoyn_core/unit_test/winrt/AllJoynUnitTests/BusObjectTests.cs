//-----------------------------------------------------------------------
// <copyright file="BusObjectTests.cs" company="AllSeen Alliance.">
//     Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//        Permission to use, copy, modify, and/or distribute this software for any
//        purpose with or without fee is hereby granted, provided that the above
//        copyright notice and this permission notice appear in all copies.
//
//        THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//        WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//        MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//        ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//        ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//        OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestPlatform.UnitTestFramework;
using AllJoyn;
using System.Threading;
using System.Threading.Tasks;

namespace AllJoynUnitTests
{
    [TestClass]
    public class BusObjectTests
    {
        public string connectSpec = "tcp:addr=127.0.0.1,port=9956";

        string signalIntf = "" +
        "<interface name=\"org.alljoyn.Signals\">" +
        "   <signal name=\"StreamClosed\">" +
        "   </signal>" +
        "   <signal name=\"StreamPaused\">" +
        "   </signal>" +
        "</interface>";

        string methodIntf = "" +
        "<interface name=\"org.alljoyn.Methods\">" +
        "   <method name=\"Pause\">" +
        "      <arg name=\"success\" type=\"b\" direction=\"out\"/>" +
        "   </method>" +
        "   <method name=\"SeekRelative\">" +
        "      <arg name=\"offset\" type=\"i\" direction=\"in\"/>" +
        "      <arg name=\"units\" type=\"y\" direction=\"in\"/>" +
        "      <arg name=\"success\" type=\"b\" direction=\"out\"/>" +
        "   </method>" +
        "</interface>";

        string propertyIntf = "" +
        "<interface name=\"org.alljoyn.Properties\">" +
        "   <property name=\"SampleFrequency\" type=\"u\" access=\"read\"/>" +
        "   <property name=\"SamplesPerFrame\" type=\"u\" access=\"read\"/>" +
        "   <property name=\"BitRate\" type=\"u\" access=\"read\"/>" +
        "</interface>";

        string mixedIntf = "" +
        "<interface name=\"org.alljoyn.Mixed\">" +
        "   <property name=\"BitRate\" type=\"u\" access=\"read\"/>" +
        "   <method name=\"Pause\">" +
        "      <arg name=\"success\" type=\"b\" direction=\"out\"/>" +
        "   </method>" +
        "   <signal name=\"StreamPaused\">" +
        "   </signal>" +
        "</interface>";

        string emptyIntf = "" +
        "<interface name=\"org.alljoyn.Empty\">" +
        "</interface>";

        [TestMethod]
        public void AddInterfaceTest()
        {
            BusAttachment bus = new BusAttachment("addinterface", true, 4);
            bus.CreateInterfacesFromXml(signalIntf);
            bus.CreateInterfacesFromXml(methodIntf);
            bus.CreateInterfacesFromXml(propertyIntf);
            bus.CreateInterfacesFromXml(mixedIntf);
            bus.CreateInterfacesFromXml(emptyIntf);
            InterfaceDescription[] annIntf = new InterfaceDescription[1];
            bus.CreateInterface("org.alljoyn.Annotated", annIntf, false);
            annIntf[0].AddMethod("method", "ss", "s", "in1,in2,out1", (byte)0, "");
            annIntf[0].AddProperty("property", "s", (byte)PropAccessType.PROP_ACCESS_RW);
            annIntf[0].AddSignal("signal", "suy", "str,uint,byte", (byte)0, "");
            annIntf[0].AddAnnotation("org.freedesktop.DBus.Deprecated", "false");
            annIntf[0].AddMemberAnnotation("method", "org.freedesktop.DBus.Method.NoReply", "true");
            annIntf[0].AddPropertyAnnotation("property", "org.freedesktop.DBus.Property.EmitsChangedSignal", "true");
            annIntf[0].Activate();

            BusObject busObj = new BusObject(bus, "/addinterfaces", false);
            busObj.AddInterface(bus.GetInterface("org.alljoyn.Signals"));
            busObj.AddInterface(bus.GetInterface("org.alljoyn.Methods"));
            busObj.AddInterface(bus.GetInterface("org.alljoyn.Properties"));
            busObj.AddInterface(bus.GetInterface("org.alljoyn.Empty"));
            busObj.AddInterface(bus.GetInterface("org.alljoyn.Annotated"));
        }

        public class MethodHandlerBusObject
        {
            public BusObject busObject;
            public InterfaceMember catMember;
            public InterfaceMember sayHiMember;
            public MethodHandlerBusObject(BusAttachment busAtt, string path)
            {
                this.busObject = new BusObject(busAtt, path, false);
                InterfaceDescription[] intf = new InterfaceDescription[1];
                busAtt.CreateInterface("org.alljoyn.methodhandler", intf, false);
                intf[0].AddMethod("cat", "ss", "s", "in1,in2,out", (byte)0, "");
                intf[0].AddMethod("sayhi", "s", "s", "in,out", (byte)0, "");
                intf[0].Activate();
                this.busObject.AddInterface(intf[0]);
                this.catMember = intf[0].GetMethod("cat");
                this.sayHiMember = intf[0].GetMethod("sayhi");

                MessageReceiver catReceiver = new MessageReceiver(busAtt);
                catReceiver.MethodHandler += new MessageReceiverMethodHandler(this.CatHandler);
                MessageReceiver sayHiReceiver = new MessageReceiver(busAtt);
                sayHiReceiver.MethodHandler += new MessageReceiverMethodHandler(this.SayHiHandler);
                try
                {
                    busObject.AddMethodHandler(null, catReceiver);
                    Assert.IsFalse(true);
                }
                catch (Exception ex)
                {
                    Logger.LogMessage("%s", ex.Message);
                }
                try
                {
                    busObject.AddMethodHandler(intf[0].GetMethod("cat"), null);
                    Assert.IsFalse(true);
                }
                catch (Exception ex)
                {
                    Logger.LogMessage("%s", ex.Message);
                }

                busObject.AddMethodHandler(intf[0].GetMethod("cat"), catReceiver);
                busObject.AddMethodHandler(intf[0].GetMethod("sayhi"), sayHiReceiver);

                busAtt.RegisterBusObject(this.busObject);
            }
            public void CatHandler(InterfaceMember member, Message message)
            {
                string ret = message.GetArg(0).Value.ToString() + message.GetArg(1).Value.ToString();
                MsgArg retArg = new MsgArg("s", new object[] { ret });
                this.busObject.MethodReply(message, new MsgArg[] { retArg });
            }
            public void SayHiHandler(InterfaceMember member, Message message)
            {
                Assert.AreEqual("hello", message.GetArg(0).Value.ToString());
                MsgArg retArg = new MsgArg("s", new object[] { "aloha" });
                try
                {
                    // BUGBUG: Throws exception saying signature of the msgArg is not what was expected, but its correct.
                    this.busObject.MethodReplyWithQStatus(message, QStatus.ER_OK);
                }
                catch (Exception ex)
                {
#if DEBUG
                    string err = AllJoynException.GetErrorMessage(ex.HResult);
#else
                    QStatus err = AllJoynException.GetErrorCode(ex.HResult);
#endif
                    Assert.IsFalse(true);
                }
            }
        }
        ManualResetEvent catMethodCalled = new ManualResetEvent(false);
        ManualResetEvent foundMethodObjectName = new ManualResetEvent(false);

        [TestMethod]
        public void AddMethodHandlerTest()
        {
            BusAttachment service = new BusAttachment("methodhandler", true, 4);
            MethodHandlerBusObject busObj = new MethodHandlerBusObject(service, "/handlertest");
            service.Start();
            service.ConnectAsync(connectSpec).AsTask().Wait();
            SessionPortListener spl = new SessionPortListener(service);
            spl.AcceptSessionJoiner += new SessionPortListenerAcceptSessionJoinerHandler((ushort sessionPort, string joiner, SessionOpts opts) =>
            {
                Assert.AreEqual(33, sessionPort);
                return true;
            });
            service.RequestName("org.alljoyn.methodhandlertest", (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
            service.BindSessionPort(33, new ushort[1], new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, 
                ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY), spl);
            service.AdvertiseName("org.alljoyn.methodhandlertest", TransportMaskType.TRANSPORT_ANY);

            BusAttachment client = new BusAttachment("methodcaller", true, 4);
            BusListener bl = new BusListener(client);
            client.RegisterBusListener(bl);
            bl.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(
                (string name, TransportMaskType transport, string namePrefix) =>
                {
                    foundMethodObjectName.Set();
                });
            client.Start();
            client.ConnectAsync(connectSpec).AsTask().Wait();
            client.FindAdvertisedName("org.alljoyn.methodhandlertest");
            foundMethodObjectName.WaitOne();
            Task<JoinSessionResult> joinTask = client.JoinSessionAsync("org.alljoyn.methodhandlertest", 33, new SessionListener(client),
                new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY), new SessionOpts[1], null).AsTask<JoinSessionResult>();
            joinTask.Wait();
            Assert.IsTrue(QStatus.ER_OK == joinTask.Result.Status);
            ProxyBusObject proxy = new ProxyBusObject(client, "org.alljoyn.methodhandlertest", "/handlertest", joinTask.Result.SessionId);
            Task<IntrospectRemoteObjectResult> introTask = proxy.IntrospectRemoteObjectAsync(null).AsTask<IntrospectRemoteObjectResult>();
            introTask.Wait();
            Assert.IsTrue(QStatus.ER_OK == introTask.Result.Status);

            MsgArg[] args1 = new MsgArg[2];
            args1[0] = new MsgArg("s", new object[] { "one" });
            args1[1] = new MsgArg("s", new object[] { "two" });
            Task<MethodCallResult> catTask = proxy.MethodCallAsync(service.GetInterface("org.alljoyn.methodhandler").GetMethod("cat"),
                args1, null, 60000, (byte)0).AsTask<MethodCallResult>();
            catTask.Wait();
            Assert.IsTrue(AllJoynMessageType.MESSAGE_METHOD_RET == catTask.Result.Message.Type);
            Assert.AreEqual("onetwo", catTask.Result.Message.GetArg(0).Value.ToString());

            // Check BUGBUG above
            //MsgArg[] args2 = new MsgArg[1];
            //args2[0] = new MsgArg("s", new object[] { "hello" });
            //Task<MethodCallResult> sayHiTask = proxy.MethodCallAsync(service.GetInterface("org.alljoyn.methodhandler").GetMethod("sayhi"),
            //    args2, null, 60000, (byte)0).AsTask<MethodCallResult>();
            //sayHiTask.Wait();
            //Assert.IsTrue(AllJoynMessageType.MESSAGE_METHOD_RET == sayHiTask.Result.Message.Type);
            //Assert.AreEqual("aloha", sayHiTask.Result.Message.GetArg(0).Value.ToString());

            // TODO: add another method call that test function with signature MethodReply(AllJoyn.Message msg, string error, string errorMessage)
        }

        [TestMethod]
        public void EmitPropertyChangedTest()
        {
            // TODO: TBD
        }

        public class ServiceBusObject
        {
            public BusObject busObject;
            public ServiceBusObject(BusAttachment busAtt, string path)
            {
                this.busObject = new BusObject(busAtt, path, false);
                InterfaceDescription[] intf = new InterfaceDescription[1];
                busAtt.CreateInterface("org.alljoyn.SignalVariety", intf, false);
                intf[0].AddSignal("string", "s", "str", (byte)0, "");
                intf[0].AddSignal("byte", "y", "byte", (byte)0, "");
                intf[0].AddSignal("bool", "b", "bool", (byte)0, "");
                intf[0].AddSignal("int16", "n", "int16", (byte)0, "");
                intf[0].AddSignal("uint16", "q", "uint16", (byte)0, "");
                intf[0].AddSignal("int32", "i", "int32", (byte)0, "");
                intf[0].AddSignal("uint32", "u", "uint32", (byte)0, "");
                intf[0].AddSignal("int64", "x", "int64", (byte)0, "");
                intf[0].AddSignal("uint64", "t", "uint64", (byte)0, "");
                intf[0].AddSignal("double", "d", "double", (byte)0, "");
                intf[0].AddSignal("dArray", "ad", "dArray", (byte)0, "");
                intf[0].AddSignal("byiStruct", "(byi)", "byiStruct", (byte)0, "");
                intf[0].AddSignal("isDict", "{is}", "isDict", (byte)0, "");
                intf[0].Activate();
                this.busObject.AddInterface(intf[0]);
                busAtt.RegisterBusObject(this.busObject);
            }
        }

        public class SignalBusObject
        {
            public BusObject busObject;
            public SignalBusObject(BusAttachment busAtt, string path)
            {
                this.busObject = new BusObject(busAtt, path, false);
                InterfaceDescription[] intf = new InterfaceDescription[1];
                busAtt.CreateInterface("org.alljoyn.SignalVariety", intf, false);
                intf[0].AddSignal("string", "s", "str", (byte)0, "");
                intf[0].AddSignal("byte", "y", "byte", (byte)0, "");
                intf[0].AddSignal("bool", "b", "bool", (byte)0, "");
                intf[0].AddSignal("int16", "n", "int16", (byte)0, "");
                intf[0].AddSignal("uint16", "q", "uint16", (byte)0, "");
                intf[0].AddSignal("int32", "i", "int32", (byte)0, "");
                intf[0].AddSignal("uint32", "u", "uint32", (byte)0, "");
                intf[0].AddSignal("int64", "x", "int64", (byte)0, "");
                intf[0].AddSignal("uint64", "t", "uint64", (byte)0, "");
                intf[0].AddSignal("double", "d", "double", (byte)0, "");
                intf[0].AddSignal("dArray", "ad", "dArray", (byte)0, "");
                intf[0].AddSignal("byiStruct", "(byi)", "byiStruct", (byte)0, "");
                intf[0].AddSignal("isDict", "{is}", "isDict", (byte)0, "");
                intf[0].Activate();
                this.busObject.AddInterface(intf[0]);

                MessageReceiver msgReceiver1 = new MessageReceiver(busAtt);
                msgReceiver1.SignalHandler += new MessageReceiverSignalHandler(this.StringSig);
                busAtt.RegisterSignalHandler(msgReceiver1, intf[0].GetSignal("string"), "");

                MessageReceiver msgReceiver2 = new MessageReceiver(busAtt);
                msgReceiver2.SignalHandler += new MessageReceiverSignalHandler(this.ByteSig);
                busAtt.RegisterSignalHandler(msgReceiver2, intf[0].GetSignal("byte"), "");

                MessageReceiver msgReceiver3 = new MessageReceiver(busAtt);
                msgReceiver3.SignalHandler += new MessageReceiverSignalHandler(this.BoolSig);
                busAtt.RegisterSignalHandler(msgReceiver3, intf[0].GetSignal("bool"), "");

                MessageReceiver msgReceiver4 = new MessageReceiver(busAtt);
                msgReceiver4.SignalHandler += new MessageReceiverSignalHandler(this.Int16Sig);
                busAtt.RegisterSignalHandler(msgReceiver4, intf[0].GetSignal("int16"), "");

                MessageReceiver msgReceiver5 = new MessageReceiver(busAtt);
                msgReceiver5.SignalHandler += new MessageReceiverSignalHandler(this.Uint16Sig);
                busAtt.RegisterSignalHandler(msgReceiver5, intf[0].GetSignal("uint16"), "");

                MessageReceiver msgReceiver6 = new MessageReceiver(busAtt);
                msgReceiver6.SignalHandler += new MessageReceiverSignalHandler(this.Int32Sig);
                busAtt.RegisterSignalHandler(msgReceiver6, intf[0].GetSignal("int32"), "");

                MessageReceiver msgReceiver7 = new MessageReceiver(busAtt);
                msgReceiver7.SignalHandler += new MessageReceiverSignalHandler(this.Uint32Sig);
                busAtt.RegisterSignalHandler(msgReceiver7, intf[0].GetSignal("uint32"), "");

                MessageReceiver msgReceiver8 = new MessageReceiver(busAtt);
                msgReceiver8.SignalHandler += new MessageReceiverSignalHandler(this.Int64Sig);
                busAtt.RegisterSignalHandler(msgReceiver8, intf[0].GetSignal("int64"), "");

                MessageReceiver msgReceiver9 = new MessageReceiver(busAtt);
                msgReceiver9.SignalHandler += new MessageReceiverSignalHandler(this.Uint64Sig);
                busAtt.RegisterSignalHandler(msgReceiver9, intf[0].GetSignal("uint64"), "");

                MessageReceiver msgReceiver10 = new MessageReceiver(busAtt);
                msgReceiver10.SignalHandler += new MessageReceiverSignalHandler(this.DoubleSig);
                busAtt.RegisterSignalHandler(msgReceiver10, intf[0].GetSignal("double"), "");

                MessageReceiver msgReceiver11 = new MessageReceiver(busAtt);
                msgReceiver11.SignalHandler += new MessageReceiverSignalHandler(this.DArrarySig);
                busAtt.RegisterSignalHandler(msgReceiver11, intf[0].GetSignal("dArray"), "");

                MessageReceiver msgReceiver12 = new MessageReceiver(busAtt);
                msgReceiver12.SignalHandler += new MessageReceiverSignalHandler(this.BYIStructSig);
                busAtt.RegisterSignalHandler(msgReceiver12, intf[0].GetSignal("byiStruct"), "");

                MessageReceiver msgReceiver13 = new MessageReceiver(busAtt);
                msgReceiver13.SignalHandler += new MessageReceiverSignalHandler(this.ISDictSig);
                busAtt.RegisterSignalHandler(msgReceiver13, intf[0].GetSignal("isDict"), "");

                busAtt.RegisterBusObject(this.busObject);
            }
            public void StringSig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void ByteSig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void BoolSig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void Int16Sig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void Uint16Sig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void Int32Sig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void Uint32Sig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void Int64Sig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void Uint64Sig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void DoubleSig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void DArrarySig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void BYIStructSig(InterfaceMember member, string srcPath, Message message)
            {

            }
            public void ISDictSig(InterfaceMember member, string srcPath, Message message)
            {

            }
        }

        ManualResetEvent foundSignalObjectName = new ManualResetEvent(false);

        [TestMethod]
        public void SignalTest()
        {
            BusAttachment service = new BusAttachment("signalservice", true, 4);
            SignalBusObject busObj = new SignalBusObject(service, "/sigtest");
            service.Start();
            service.ConnectAsync(connectSpec).AsTask().Wait();
            SessionPortListener spl = new SessionPortListener(service);
            spl.AcceptSessionJoiner += new SessionPortListenerAcceptSessionJoinerHandler((ushort sessionPort, string joiner, SessionOpts opts) =>
            {
                Assert.AreEqual(89, sessionPort);
                return true;
            });
            service.RequestName("org.alljoyn.signaltesting", (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
            service.BindSessionPort(89, new ushort[1], new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false,
                ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY), spl);
            service.AdvertiseName("org.alljoyn.signaltesting", TransportMaskType.TRANSPORT_ANY);

            BusAttachment client = new BusAttachment("methodcaller", true, 4);
            ServiceBusObject sbo = new ServiceBusObject(client, "/clientbusobj");
            BusListener bl = new BusListener(client);
            client.RegisterBusListener(bl);
            bl.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(
                (string name, TransportMaskType transport, string namePrefix) =>
                {
                    foundSignalObjectName.Set();
                });
            client.Start();
            client.ConnectAsync(connectSpec).AsTask().Wait();
            client.FindAdvertisedName("org.alljoyn.signaltesting");
            foundSignalObjectName.WaitOne();

            Task<JoinSessionResult> joinTask = client.JoinSessionAsync("org.alljoyn.signaltesting", 89, new SessionListener(client),
                new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY), new SessionOpts[1], null).AsTask<JoinSessionResult>();
            joinTask.Wait();
            Assert.IsTrue(QStatus.ER_OK == joinTask.Result.Status);

            // TODO: call BusObject.Signal() for each one of the signals in the interface and make sure 
            // they're received and the data is consistent with what was sent.
        }
    }
}
