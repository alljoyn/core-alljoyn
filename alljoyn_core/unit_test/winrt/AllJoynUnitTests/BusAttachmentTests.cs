//-----------------------------------------------------------------------
// <copyright file="BusAttachmentTests.cs" company="AllSeen Alliance.">
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
    public class BusAttachmentTests
    {
        public string connectSpec = "tcp:addr=127.0.0.1,port=9956";

        [TestMethod]
        public void AddLogonEntryTest()
        {

        }

        public class AddMatchBusObj
        {
            private BusObject busObj;
            public bool MatchValid { get; set; }
            public AddMatchBusObj(BusAttachment bus)
            {
                this.busObj = new BusObject(bus, "/test", false);
                InterfaceDescription[] intf = new InterfaceDescription[1];
                bus.CreateInterface("org.alljoyn.addmatchtest", intf, false);
                intf[0].AddSignal("testSig", "s", "str", (byte)0, "");
                intf[0].Activate();
                this.busObj.AddInterface(intf[0]);
                MessageReceiver receiver = new MessageReceiver(bus);
                receiver.SignalHandler += new MessageReceiverSignalHandler(this.SigHandle);
                bus.RegisterSignalHandler(receiver, intf[0].GetSignal("testSig"), "");
                bus.RegisterBusObject(this.busObj);
            }
            public void SigHandle(InterfaceMember member, string srcPath, Message message)
            {
                Assert.IsTrue(MatchValid);
                calledHandle.Set();
            }
        }
        static ManualResetEvent calledHandle = new ManualResetEvent(false);
        static ManualResetEvent foundService = new ManualResetEvent(false);

        [TestMethod]
        public void AddMatchTest()
        {
            BusAttachment bus = new BusAttachment("addmatch", true, 4);
            AddMatchBusObj busObj = new AddMatchBusObj(bus);
            BusListener bl = new BusListener(bus);
            bus.RegisterBusListener(bl);
            busObj.MatchValid = true;
            bus.Start();
            bus.ConnectAsync(connectSpec).AsTask().Wait();

            BusAttachment service = new BusAttachment("service", true, 4);
            BusObject busObj2 = new BusObject(service, "/serviceTest", false);
            InterfaceDescription[] intf = new InterfaceDescription[1];
            service.CreateInterface("org.alljoyn.addmatchtest", intf, false);
            intf[0].AddSignal("testSig", "s", "str", (byte)0, "");
            intf[0].Activate();
            busObj2.AddInterface(intf[0]);
            service.RegisterBusObject(busObj2);
            service.Start();
            service.ConnectAsync(connectSpec).AsTask().Wait();
            service.RequestName("org.alljoyn.addmatch", (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
            service.BindSessionPort(43, new ushort[1], new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, 
                ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY), new SessionPortListener(service));
            service.AdvertiseName("org.alljoyn.addmatch", TransportMaskType.TRANSPORT_ANY);

            bl.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(
                (string name, TransportMaskType transport, string namePrefix) =>
                {
                    if (namePrefix == "org.alljoyn.addmatch")
                    {
                        foundService.Set();
                    }
                });
            bus.FindAdvertisedName("org.alljoyn.addmatch");

            foundService.WaitOne();
            Task<JoinSessionResult> join = bus.JoinSessionAsync("org.alljoyn.addmatch", 43, new SessionListener(bus),
                            new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY),
                            new SessionOpts[1], null).AsTask<JoinSessionResult>();
            join.Wait();
            Assert.IsTrue(QStatus.ER_OK != join.Result.Status);

            bus.AddMatch("type='signal',interface='org.alljoyn.addmatchtest',member='testSig'");
            for (int i = 0; i < 5; i++)
            {
                calledHandle.Reset();
                MsgArg sigArg1 = new MsgArg("s", new object[] { "hello" + i });
                busObj2.Signal("", 0, intf[0].GetSignal("testSig"), new MsgArg[] { sigArg1 }, 0, (byte)AllJoynFlagType.ALLJOYN_FLAG_GLOBAL_BROADCAST);
                calledHandle.WaitOne();
            }

            bus.RemoveMatch("type='signal',interface='org.alljoyn.addmatchtest',member='testSig'");
            busObj.MatchValid = false;
            for (int i = 0; i < 10; i++)
            {
                MsgArg sigArg1 = new MsgArg("s", new object[] { "hello" + i });
                busObj2.Signal("", 0, intf[0].GetSignal("testSig"), new MsgArg[] { sigArg1 }, 0, (byte)AllJoynFlagType.ALLJOYN_FLAG_GLOBAL_BROADCAST);
            }

            bus.AddMatch("type='signal',interface='org.alljoyn.addmatchtest',member='testSig'");
            busObj.MatchValid = true;
            for (int i = 0; i < 5; i++)
            {
                calledHandle.Reset();
                MsgArg sigArg1 = new MsgArg("s", new object[] { "hello" + i });
                busObj2.Signal("", 0, intf[0].GetSignal("testSig"), new MsgArg[] { sigArg1 }, 0, (byte)AllJoynFlagType.ALLJOYN_FLAG_GLOBAL_BROADCAST);
                calledHandle.WaitOne();
            }
        }

        public class AdvertiseBusListener
        {
            private BusListener bl;
            public HashSet<string> FoundNames { get; set; }
            public int FoundCount { get; set; }
            public HashSet<string> LostNames { get; set; }
            public int LostCount { get; set; }
            public AdvertiseBusListener(BusAttachment bus)
            {
                this.bl = new BusListener(bus);
                this.FoundNames = new HashSet<string>();
                this.LostNames = new HashSet<string>();
                this.FoundCount = 0;
                this.LostCount = 0;
                this.bl.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(this.BusListenerFoundAdvertisedName);
                this.bl.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(this.BusListenerLostAdvertisedName);
                bus.RegisterBusListener(this.bl);
            }
            public void BusListenerFoundAdvertisedName(string name, TransportMaskType transport, string namePrefix)
            {
                lock (FoundNames)
                {
                    FoundNames.Add(name);
                    this.FoundCount++;
                    if (this.FoundCount == 4)
                    {
                        gotAllNames.Set();
                    }
                }
            }
            public void BusListenerLostAdvertisedName(string name, TransportMaskType transport, string namePrefix)
            {
                lock (FoundNames)
                {
                    Assert.IsTrue(FoundNames.Contains(name));
                }
                lock (LostNames)
                {
                    LostNames.Add(name);
                    this.LostCount++;
                    if (this.LostCount == 4)
                    {
                        gotAllNames.Set();
                    }
                }
            }
        }
        static ManualResetEvent gotAllNames = new ManualResetEvent(false);

        [TestMethod]
        public void AdvertisingTest()
        {
            BusAttachment bus = new BusAttachment("advertise", true, 4);
            bus.Start();
            bus.ConnectAsync(connectSpec).AsTask().Wait();

            BusAttachment clientConsumer = new BusAttachment("clientConsumer", true, 4);
            AdvertiseBusListener bl = new AdvertiseBusListener(clientConsumer);
            clientConsumer.Start();
            clientConsumer.ConnectAsync(connectSpec).AsTask().Wait();
            clientConsumer.FindAdvertisedName("org.alljoyn.one");
            clientConsumer.FindAdvertisedName("org.alljoyn.two");
            clientConsumer.FindAdvertisedName("org.alljoyn.three");
            clientConsumer.FindAdvertisedName("org.alljoyn.four");

            byte flags = (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE;
            string name1 = "org.alljoyn.one";
            string name2 = "org.alljoyn.two";
            string name3 = "org.alljoyn.three";
            string name4 = "org.alljoyn.four";

            bus.RequestName(name1, flags);
            bus.RequestName(name2, flags);
            bus.RequestName(name3, flags);
            bus.RequestName(name4, flags);

            bus.AdvertiseName(name1, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name2, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name3, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name4, TransportMaskType.TRANSPORT_ANY);

            gotAllNames.WaitOne();

            lock (bl.FoundNames)
            {
                Assert.IsTrue(bl.FoundNames.Contains(name1));
                Assert.IsTrue(bl.FoundNames.Contains(name2));
                Assert.IsTrue(bl.FoundNames.Contains(name3));
                Assert.IsTrue(bl.FoundNames.Contains(name4));
            }

            gotAllNames.Reset();

            bus.CancelAdvertiseName(name1, TransportMaskType.TRANSPORT_ANY);
            bus.CancelAdvertiseName(name2, TransportMaskType.TRANSPORT_ANY);
            bus.CancelAdvertiseName(name3, TransportMaskType.TRANSPORT_ANY);
            bus.CancelAdvertiseName(name4, TransportMaskType.TRANSPORT_ANY);

            gotAllNames.WaitOne();

            lock (bl.LostNames)
            {
                Assert.IsTrue(bl.LostNames.Contains(name1));
                Assert.IsTrue(bl.LostNames.Contains(name2));
                Assert.IsTrue(bl.LostNames.Contains(name3));
                Assert.IsTrue(bl.LostNames.Contains(name4));
            }

            // TODO: Make sure we're not still receiving advertisements for the names
        }

        [TestMethod]
        public void FindAdvertisedNameTest()
        {
            gotAllNames.Reset();
            BusAttachment bus = new BusAttachment("advertise", true, 4);
            bus.Start();
            bus.ConnectAsync(connectSpec).AsTask().Wait();

            BusAttachment clientConsumer = new BusAttachment("clientConsumer", true, 4);
            AdvertiseBusListener bl = new AdvertiseBusListener(clientConsumer);
            clientConsumer.Start();
            clientConsumer.ConnectAsync(connectSpec).AsTask().Wait();
            clientConsumer.FindAdvertisedName("org.alljoyn.prefix");

            byte flags = (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE;
            string name0 = "org.alljoyn.prefix";
            string name1 = "org.alljoyn.prefix.looooooooooonnnnggggggggggg.sssstttttrrrrringgggg";
            string name2 = "org.alljoyn.prefix.m3455555555552349568868979999";
            string name3 = "org.alljoyn.prefix.asfdff.g123555555.jisjjiofdjkdsf.z45566666";
            string name4 = "org.alljoyn.this.should.not.befound";
            string name5 = "org.alljoyn.pre";
            string name6 = "org.alljoyn";
            bus.RequestName(name4, flags);
            bus.RequestName(name5, flags);
            bus.RequestName(name6, flags);
            bus.RequestName(name0, flags);
            bus.RequestName(name1, flags);
            bus.RequestName(name2, flags);
            bus.RequestName(name3, flags);
            bus.AdvertiseName(name4, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name5, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name6, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name0, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name1, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name2, TransportMaskType.TRANSPORT_ANY);
            bus.AdvertiseName(name3, TransportMaskType.TRANSPORT_ANY);

            gotAllNames.WaitOne();
            
            lock (bl.FoundNames)
            {
                Assert.IsTrue(bl.FoundNames.Contains(name0));
                Assert.IsTrue(bl.FoundNames.Contains(name1));
                Assert.IsTrue(bl.FoundNames.Contains(name2));
                Assert.IsTrue(bl.FoundNames.Contains(name3));
                Assert.IsFalse(bl.FoundNames.Contains(name4));
                Assert.IsFalse(bl.FoundNames.Contains(name5));
                Assert.IsFalse(bl.FoundNames.Contains(name6));
            }
        }

        [TestMethod]
        public void BindSessionPortTest()
        {
            BusAttachment bus = new BusAttachment("bindports", true, 4);
            bus.Start();
            bus.ConnectAsync(connectSpec).AsTask().Wait();

            SessionOpts opts = new SessionOpts(TrafficType.TRAFFIC_MESSAGES, true, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY);
            SessionPortListener spl = new SessionPortListener(bus);
            for (ushort i = 1; i <= 10; i++)
            {
                ushort[] portOut = new ushort[1];
                bus.BindSessionPort((ushort)(i * 10), portOut, opts, spl);
                Assert.AreEqual((ushort)(i * 10), portOut[0]);
            }
        }

        [TestMethod]
        public void ConstructorTest()
        {
            BusAttachment bus1 = new BusAttachment("bus", true, 4);
            BusAttachment bus2 = new BusAttachment("bus", true, 4);
            BusAttachment bus3 = new BusAttachment("bus", true, 4);
            BusAttachment bus4 = new BusAttachment("bus", true, 4);
        }

        [TestMethod]
        public void ClearKeysTest()
        {
            // TODO: Will probably have to do this inside of the peer security test
        }

        [TestMethod]
        public void ClearKeyStoreTest()
        {
            // TODO: Is checking the size of the file appropriate?
        }

        [TestMethod]
        public void ConnectAsyncTest()
        {
            BusAttachment bus = new BusAttachment("connect", true, 4);
            bus.Start();

            for (int i = 0; i < 5; i++)
            {
                bus.ConnectAsync(connectSpec).AsTask().Wait();
                Assert.IsTrue(bus.IsConnected());
                bus.DisconnectAsync(connectSpec).AsTask().Wait();
                Assert.IsFalse(bus.IsConnected());
            }
        }

        [TestMethod]
        public void CreateInterfaceTest()
        {
            BusAttachment bus = new BusAttachment("interfacetest", true, 4);
            InterfaceDescription[] secureIntf = new InterfaceDescription[1];
            InterfaceDescription[] nonsecureIntf = new InterfaceDescription[1];

            bus.CreateInterface("secureIntf", secureIntf, true);
            bus.CreateInterface("nonsecureIntf", nonsecureIntf, false);
            secureIntf[0].AddMethod("foo", "say", "ab", "str,arr1,arr2", (byte)0, "");
            nonsecureIntf[0].AddSignal("bar", "say", "str,arr1", (byte)0, "");
            secureIntf[0].Activate();
            nonsecureIntf[0].Activate();
            Assert.IsNotNull(bus.GetInterface("secureIntf"));
            Assert.IsNotNull(bus.GetInterface("nonsecureIntf"));
            InterfaceDescription[] intfs = new InterfaceDescription[12];
            bus.GetInterfaces(intfs);
            Assert.IsNotNull(intfs[0]);
            Assert.IsNotNull(intfs[1]);

            // BUGBUG: DeleteInterface is throwing an exception saying the interfaces don't exist
            //bus.DeleteInterface(bus.GetInterface("secureIntf"));
            //bus.DeleteInterface(bus.GetInterface("nonsecureIntf"));
            //Assert.IsNull(bus.GetInterface("secureIntf"));
            //Assert.IsNull(bus.GetInterface("nonsecureIntf"));
            //InterfaceDescription[] nullIntfs = new InterfaceDescription[2];
            //bus.GetInterfaces(nullIntfs);
            //Assert.IsNull(nullIntfs[0]);
            //Assert.IsNull(nullIntfs[1]);

            // TODO: It doesn't seem like you can create multiple interfaces at a time
            //InterfaceDescription[] multiIntf = new InterfaceDescription[5];
            //bus.CreateInterface("multiIntf", multiIntf, false);

            try
            {
                InterfaceDescription nonExisting = bus.GetInterface("notThere");
                Assert.IsFalse(true);
            }
            catch (Exception ex)
            {
                Logger.LogMessage("%s", ex.Message);
            }
        }

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

        [TestMethod]
        public void CreateInterfacesFromXMLTest()
        {
            BusAttachment bus = new BusAttachment("xmlinterfaces", true, 4);

            bus.CreateInterfacesFromXml(signalIntf);
            bus.CreateInterfacesFromXml(methodIntf);
            bus.CreateInterfacesFromXml(propertyIntf);
            bus.CreateInterfacesFromXml(mixedIntf);
            InterfaceDescription i = bus.GetInterface("org.alljoyn.Signals");
            Assert.IsNotNull(i);
            Assert.IsNotNull(bus.GetInterface("org.alljoyn.Methods"));
            Assert.IsNotNull(bus.GetInterface("org.alljoyn.Properties"));
            Assert.IsNotNull(bus.GetInterface("org.alljoyn.Mixed"));
            InterfaceDescription[] intfs = new InterfaceDescription[14];
            bus.GetInterfaces(intfs);
            Assert.IsNotNull(intfs[0]);
            Assert.IsNotNull(intfs[1]);
            Assert.IsNotNull(intfs[2]);
            Assert.IsNotNull(intfs[3]);

            // BUGBUG: DeleteInterface is throwing an exception saying the interfaces don't exist
            //bus.DeleteInterface(bus.GetInterface("org.alljoyn.Signals"));
            //bus.DeleteInterface(bus.GetInterface("org.alljoyn.Methods"));
            //bus.DeleteInterface(bus.GetInterface("org.alljoyn.Properties"));
            //bus.DeleteInterface(bus.GetInterface("org.alljoyn.Mixed"));
            //Assert.IsNull(bus.GetInterface("org.alljoyn.Signals"));
            //Assert.IsNull(bus.GetInterface("org.alljoyn.Methods"));
            //Assert.IsNull(bus.GetInterface("org.alljoyn.Properties"));
            //Assert.IsNull(bus.GetInterface("org.alljoyn.Mixed"));

            //InterfaceDescription[] nullIntfs = new InterfaceDescription[14];
            //bus.GetInterfaces(nullIntfs);
            //Assert.IsNull(nullIntfs[0]);
            //Assert.IsNull(nullIntfs[1]);
            //Assert.IsNull(nullIntfs[2]);
            //Assert.IsNull(nullIntfs[3]);
        }

        [TestMethod]
        public void EnableConcurrentCallbacksTest()
        {
            // TODO: TBD
        }

        public class ClientAuthListener
        {
            public AuthListener al;
            public ClientAuthListener(BusAttachment busAtt)
            {
                this.al = new AuthListener(busAtt);
                this.al.RequestCredentials += new AuthListenerRequestCredentialsAsyncHandler(this.RequestCreds);
                this.al.AuthenticationComplete += new AuthListenerAuthenticationCompleteHandler(this.AuthComplete);
            }
            public QStatus RequestCreds(string mech, string peer, ushort count, string user, ushort mask, AuthContext context)
            {
                return QStatus.ER_OK;
            }
            public void AuthComplete(string mech, string name, bool success)
            {

            }
        }
        public class SRP_LOGON_AuthListener
        {
            public AuthListener al;
            public SRP_LOGON_AuthListener(BusAttachment busAtt)
            {
                this.al = new AuthListener(busAtt);
                this.al.RequestCredentials += new AuthListenerRequestCredentialsAsyncHandler(this.RequestCreds);
                this.al.AuthenticationComplete += new AuthListenerAuthenticationCompleteHandler(this.AuthComplete);
            }
            public QStatus RequestCreds(string mech, string peer, ushort count, string user, ushort mask, AuthContext context)
            {
                return QStatus.ER_OK;
            }
            public void AuthComplete(string mech, string name, bool success)
            {

            }
        }
        public class SRP_KEYX_AuthListener
        {
            public AuthListener al;
            public SRP_KEYX_AuthListener(BusAttachment busAtt)
            {
                this.al = new AuthListener(busAtt);
                this.al.RequestCredentials += new AuthListenerRequestCredentialsAsyncHandler(this.RequestCreds);
                this.al.AuthenticationComplete += new AuthListenerAuthenticationCompleteHandler(this.AuthComplete);
            }
            public QStatus RequestCreds(string mech, string peer, ushort count, string user, ushort mask, AuthContext context)
            {
                return QStatus.ER_OK;
            }
            public void AuthComplete(string mech, string name, bool success)
            {

            }
        }
        public class RSA_KEYX_AuthListener
        {
            public AuthListener al;
            public RSA_KEYX_AuthListener(BusAttachment busAtt)
            {
                this.al = new AuthListener(busAtt);
                this.al.RequestCredentials += new AuthListenerRequestCredentialsAsyncHandler(this.RequestCreds);
                this.al.AuthenticationComplete += new AuthListenerAuthenticationCompleteHandler(this.AuthComplete);
            }
            public QStatus RequestCreds(string mech, string peer, ushort count, string user, ushort mask, AuthContext context)
            {
                return QStatus.ER_OK;
            }
            public void AuthComplete(string mech, string name, bool success)
            {

            }
        }

        [TestMethod]
        public async void EnablePeerSecurityTest()
        {
            BusAttachment clientBus = new BusAttachment("client", true, 4);
            ClientAuthListener cl = new ClientAuthListener(clientBus);
            clientBus.Start();
            clientBus.EnablePeerSecurity("ALLJOYN_SRP_LOGON", cl.al, "/.alljoyn_keystore/s_central.ks", true);
            await clientBus.ConnectAsync(connectSpec);
            Assert.IsTrue(clientBus.IsPeerSecurityEnabled());

            BusAttachment srpLogonBus = new BusAttachment("srplogon", true, 4);
            SRP_LOGON_AuthListener srpl = new SRP_LOGON_AuthListener(srpLogonBus);
            srpLogonBus.Start();
            srpLogonBus.EnablePeerSecurity("ALLJOYN_SRP_LOGON", srpl.al, "/.alljoyn_keystore/s_central.ks", true);
            await srpLogonBus.ConnectAsync(connectSpec);
            Assert.IsTrue(srpLogonBus.IsPeerSecurityEnabled());

            BusAttachment srpKeyxBus = new BusAttachment("srpkeyx", true, 4);
            SRP_KEYX_AuthListener srpk = new SRP_KEYX_AuthListener(srpKeyxBus);
            srpKeyxBus.Start();
            srpKeyxBus.EnablePeerSecurity("ALLJOYN_SRP_KEYX", srpk.al, "/.alljoyn_keystore/s_central.ks", true);
            await srpKeyxBus.ConnectAsync(connectSpec);
            Assert.IsTrue(srpKeyxBus.IsPeerSecurityEnabled());

            BusAttachment rsaKeyxBus = new BusAttachment("rsakeyx", true, 4);
            RSA_KEYX_AuthListener rsak = new RSA_KEYX_AuthListener(srpKeyxBus);
            rsaKeyxBus.Start();
            rsaKeyxBus.EnablePeerSecurity("ALLJOYN_RSA_KEYX", rsak.al, "/.alljoyn_keystore/s_central.ks", true);
            await rsaKeyxBus.ConnectAsync(connectSpec);
            Assert.IsTrue(rsaKeyxBus.IsPeerSecurityEnabled());

            // TODO: Finish. Find an easier way to activate the auth process than calling interface method
        }

        [TestMethod]
        public void GetConcurrencyTest()
        {
            for (uint i = 0; i <= 4; i++)
            {
                BusAttachment bus = new BusAttachment("one", true, i);
                Assert.AreEqual(i, bus.GetConcurrency());
            }
        }

        [TestMethod]
        public void GetKeyExpirationTest()
        {
            // TODO: TBD
        }

        [TestMethod]
        public void GetPeerGUIDTest()
        {
            // TODO: TBD
        }

        [TestMethod]
        public void GetSessionSocketStreamTest()
        {
            // TODO:
        }

        [TestMethod]
        public void IsSameBusAttachmentTest()
        {
            BusAttachment bus1 = new BusAttachment("one", true, 4);
            BusAttachment bus2 = new BusAttachment("two", true, 4);
            Assert.IsTrue(bus1.IsSameBusAttachment(bus1));
            Assert.IsTrue(bus2.IsSameBusAttachment(bus2));
            Assert.IsFalse(bus1.IsSameBusAttachment(bus2));
            Assert.IsFalse(bus2.IsSameBusAttachment(bus1));
        }

        [TestMethod]
        public void StartTest()
        {
            BusAttachment bus = new BusAttachment("started", true, 4);
            Assert.IsFalse(bus.IsStarted());
            bus.Start();
            Assert.IsTrue(bus.IsStarted());
        }

        [TestMethod]
        public void StoppingTest()
        {
            // TODO:
        }

        public class ServiceSessionPortListener
        {
            public SessionPortListener spl;
            public bool AcceptSessionJoinerCalled { get; set; }
            public bool SessionJoinedCalled { get; set; }
            public uint SessionId { get; set; }
            public ServiceSessionPortListener(BusAttachment busAtt)
            {
                AcceptSessionJoinerCalled = false;
                SessionJoinedCalled = false;
                this.spl = new SessionPortListener(busAtt);
                this.spl.AcceptSessionJoiner += new SessionPortListenerAcceptSessionJoinerHandler(this.SessionPortListenerAcceptSessionJoiner);
                this.spl.SessionJoined += new SessionPortListenerSessionJoinedHandler(this.SessionPortListenerSessionJoined);
            }
            public bool SessionPortListenerAcceptSessionJoiner(ushort sessionPort, string joiner, SessionOpts opts)
            {
                Assert.AreEqual(77, sessionPort);
                AcceptSessionJoinerCalled = true;
                return true;
            }
            public void SessionPortListenerSessionJoined(ushort sessionPort, uint id, string joiner)
            {
                Assert.AreEqual(77, sessionPort);
                SessionJoinedCalled = true;
                SessionId = id;
            }
        }
        public class ClientSessionListener
        {
            public SessionListener sl;
            public ClientSessionListener(BusAttachment busAtt)
            {
                this.sl = new SessionListener(busAtt);
                this.sl.SessionLost += new SessionListenerSessionLostHandler(this.SessionListenerSessionLost);
            }
            public void SessionListenerSessionLost(uint sessionID)
            {
                sessionLost.Set();
            }
        }
        public static ManualResetEvent clientFoundService = new ManualResetEvent(false);
        public static ManualResetEvent sessionLost = new ManualResetEvent(false);

        [TestMethod]
        public void JoinSessionAsyncTest()
        {
            BusAttachment service = new BusAttachment("service", true, 4);
            ServiceSessionPortListener spl = new ServiceSessionPortListener(service);
            service.Start();
            service.ConnectAsync(connectSpec).AsTask().Wait();
            service.RequestName("org.alljoyn.testing.service", (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
            SessionOpts opts = new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY);
            service.BindSessionPort(77, new ushort[1], opts, spl.spl);
            service.AdvertiseName("org.alljoyn.testing.service", TransportMaskType.TRANSPORT_ANY);

            BusAttachment client = new BusAttachment("client", true, 4);
            BusListener cbl = new BusListener(client);
            ClientSessionListener csl = new ClientSessionListener(client);
            cbl.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(
                (string name, TransportMaskType transport, string namePrefix) =>
                {
                    if (namePrefix == "org.alljoyn.testing.service")
                    {
                        clientFoundService.Set();
                    }
                });
            client.RegisterBusListener(cbl);
            client.Start();
            client.ConnectAsync(connectSpec).AsTask().Wait();
            client.FindAdvertisedName("org.alljoyn.testing.service");
            clientFoundService.WaitOne();
            SessionOpts[] optsOut = new SessionOpts[1];
            Task<JoinSessionResult> joinTask = client.JoinSessionAsync("org.alljoyn.testing.service", (ushort)77, csl.sl, opts, optsOut, null).AsTask<JoinSessionResult>();
            joinTask.Wait();
            //The wait of 10ms ensures that the acceptSessionJoiner and sessionJoined callbacks are completed onthe service side.
            Task.Delay(10).Wait();
            if (QStatus.ER_OK == joinTask.Result.Status)
            {
                Assert.IsTrue(spl.AcceptSessionJoinerCalled && spl.SessionJoinedCalled);
                Assert.AreEqual(joinTask.Result.SessionId, spl.SessionId);
                Assert.AreEqual(joinTask.Result.Opts.IsMultipoint, optsOut[0].IsMultipoint);
                Assert.AreEqual(joinTask.Result.Opts.Proximity, optsOut[0].Proximity);
                Assert.AreEqual(joinTask.Result.Opts.Traffic, optsOut[0].Traffic);
                Assert.AreEqual(joinTask.Result.Opts.TransportMask, optsOut[0].TransportMask);
                Assert.AreEqual(joinTask.Result.Opts.IsMultipoint, opts.IsMultipoint);
                Assert.AreEqual(joinTask.Result.Opts.Proximity, opts.Proximity);
                Assert.AreEqual(joinTask.Result.Opts.Traffic, opts.Traffic);
                Assert.AreEqual(joinTask.Result.Opts.TransportMask, opts.TransportMask);
            } 
            else 
            {
                Assert.IsFalse(true);
            }
            service.LeaveSession(spl.SessionId);
            sessionLost.WaitOne();
        }

        [TestMethod]
        public void NameHasOwnerTest()
        {
            BusAttachment bus1 = new BusAttachment("one", true, 4);
            bus1.Start();
            bus1.ConnectAsync(connectSpec).AsTask().Wait();
            bus1.RequestName("org.alljoyn.nametaken", (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);

            BusAttachment bus2 = new BusAttachment("two", true, 4);
            bus2.Start();
            bus2.ConnectAsync(connectSpec).AsTask().Wait();
            bool[] isTaken = new bool[1];
            bus2.NameHasOwner("org.alljoyn.nametaken", isTaken);
            Assert.IsTrue(isTaken[0]);
        }

        public class BusListenerTest
        {
            public BusListener bl;
            public BusListenerTest(BusAttachment busAtt)
            {
                this.bl = new BusListener(busAtt);
                this.bl.BusDisconnected += new BusListenerBusDisconnectedHandler(this.BusListenerBusDisconnected);
                this.bl.BusStopping += new BusListenerBusStoppingHandler(this.BusListenerBusStopping);
                this.bl.FoundAdvertisedName += new BusListenerFoundAdvertisedNameHandler(this.BusListenerFoundAdvertisedName);
                this.bl.ListenerRegistered += new BusListenerListenerRegisteredHandler(this.BusListenerListenerRegistered);
                this.bl.ListenerUnregistered += new BusListenerListenerUnregisteredHandler(this.BusListenerListenerUnregistered);
                this.bl.LostAdvertisedName += new BusListenerLostAdvertisedNameHandler(this.BusListenerLostAdvertisedName);
                this.bl.NameOwnerChanged += new BusListenerNameOwnerChangedHandler(this.BusListenerNameOwnerChanged);
            }
            public void BusListenerListenerRegistered(BusAttachment bus)
            {
                listenerRegistered.Set();
            }
            public void BusListenerNameOwnerChanged(string busName, string prevOwner, string newOwner)
            {
                nameOwnerChanged.Set();
            }
            public void BusListenerFoundAdvertisedName(string name, TransportMaskType transport, string namePrefix)
            {
                foundAdvertisedName.Set();
            }
            public void BusListenerLostAdvertisedName(string name, TransportMaskType transport, string namePrefix)
            {
                lostAdvertisedName.Set();
            }
            public void BusListenerListenerUnregistered()
            {
                listenerUnregistered.Set();
            }
            public void BusListenerBusDisconnected()
            {
                busDisconnected.Set();
            }
            public void BusListenerBusStopping()
            {
                busStopping.Set();
            }
        }
        public static ManualResetEvent listenerRegistered = new ManualResetEvent(false);
        public static ManualResetEvent nameOwnerChanged = new ManualResetEvent(false);
        public static ManualResetEvent foundAdvertisedName = new ManualResetEvent(false);
        public static ManualResetEvent lostAdvertisedName = new ManualResetEvent(false);
        public static ManualResetEvent listenerUnregistered = new ManualResetEvent(false);
        public static ManualResetEvent busDisconnected = new ManualResetEvent(false);
        public static ManualResetEvent busStopping = new ManualResetEvent(false);

        [TestMethod]
        public void RegisterBusListenerTest()
        {
            BusAttachment service = new BusAttachment("service", true, 4);
            service.Start();
            service.ConnectAsync(connectSpec).AsTask().Wait();
            service.RequestName("org.alljoyn.service", (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
            service.BindSessionPort(78, new ushort[1], new SessionOpts(TrafficType.TRAFFIC_MESSAGES, false, 
                ProximityType.PROXIMITY_ANY, TransportMaskType.TRANSPORT_ANY), new SessionPortListener(service));
            service.AdvertiseName("org.alljoyn.service", TransportMaskType.TRANSPORT_ANY);

            BusAttachment bus = new BusAttachment("buslistenertest", true, 4);
            BusListenerTest bl = new BusListenerTest(bus);
            bus.RegisterBusListener(bl.bl);
            listenerRegistered.WaitOne();
            bus.Start();
            bus.ConnectAsync(connectSpec).AsTask().Wait();
            bus.RequestName("org.alljoyn.buslistener", (byte)RequestNameType.DBUS_NAME_DO_NOT_QUEUE);
            nameOwnerChanged.WaitOne();
            bus.FindAdvertisedName("org.alljoyn.service");
            foundAdvertisedName.WaitOne();
            service.CancelAdvertiseName("org.alljoyn.service", TransportMaskType.TRANSPORT_ANY);
            lostAdvertisedName.WaitOne();
            bus.CancelFindAdvertisedName("org.alljoyn.service");
            bus.UnregisterBusListener(bl.bl);
            listenerUnregistered.WaitOne();
            bus.DisconnectAsync(connectSpec).AsTask().Wait();
            // BUGBUG: Don't receive the BusDisconnected signal (this will wait indefinitely)
            //busDisconnected.WaitOne();
            bus.StopAsync().AsTask().Wait();
            // BUGBUG: Don't receive the BusStopping signal (this will wait indefinitely)
            //busStopping.WaitOne();
        }

        [TestMethod]
        public void RegisterBusObjectTest()
        {
            // TODO: TBD
        }
    }
}
