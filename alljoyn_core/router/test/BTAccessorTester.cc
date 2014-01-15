/**
 * @file
 * AllJoyn service that implements interfaces and members affected test.conf.
 */

/******************************************************************************
 * Copyright (c) 2010-2011, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/
#include <qcc/platform.h>

#include <assert.h>

#include <list>
#include <set>

#include <qcc/Crypto.h>
#include <qcc/GUID.h>
#include <qcc/Logger.h>
#include <qcc/Mutex.h>
#include <qcc/Stream.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/time.h>

#include "BDAddress.h"
#include "BTBusAddress.h"
#include "Bus.h"
#include "Transport.h"


using namespace std;
using namespace qcc;

static const size_t NUM_PRIMARY_NAMES = 100;
static const size_t NUM_SECONDARY_NAMES = 5;
static const size_t NUM_SECONDARY_NODES = 100;

static const size_t EXCHANGE_DATA_LARGE = 256 * 1024;
static const size_t EXCHANGE_DATA_SMALL = 1;

static const size_t CONNECT_MULTIPLE_MAX_CONNECTIONS = 19;

static const size_t HASH_SIZE = Crypto_SHA1::DIGEST_SIZE;

static const ajn::BTBusAddress redirectAddress(ajn::BDAddress("11:22:33:44:55:66"), 0x4321);
static ajn::TransportFactoryContainer cntr;

namespace ajn {

/********************
 * TEST STUBS
 ********************/

#define _ALLJOYNBTTRANSPORT_H
class BTTransport {

  public:
    class BTAccessor;

    std::set<RemoteEndpoint*> threadList;
    Mutex threadListLock;

    BTTransport() { }
    virtual ~BTTransport() { }

    void BTDeviceAvailable(bool avail);
    bool CheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const;
    void DisconnectAll();

  protected:
    virtual void TestBTDeviceAvailable(bool avail) = 0;
    virtual bool TestCheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const = 0;
    virtual void TestDeviceChange(const BDAddress& bdAddr, uint32_t uuidRev, bool eirCapable) = 0;

  private:
    void DeviceChange(const BDAddress& bdAddr, uint32_t uuidRev, bool eirCapable);

};


void BTTransport::BTDeviceAvailable(bool avail)
{
    TestBTDeviceAvailable(avail);
}
bool BTTransport::CheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const
{
    return TestCheckIncomingAddress(addr, redirectAddr);
}
void BTTransport::DeviceChange(const BDAddress& bdAddr, uint32_t uuidRev, bool eirCapable)
{
    TestDeviceChange(bdAddr, uuidRev, eirCapable);
}
void BTTransport::DisconnectAll()
{
}

}


#include "BTNodeDB.h"
#include "BTNodeInfo.h"
#include "BTEndpoint.h"

#if defined QCC_OS_GROUP_POSIX
#if defined(QCC_OS_DARWIN)
#error Darwin support for bluetooth to be implemented
#else
#include "bt_bluez/BTAccessor.h"
#endif
#elif defined QCC_OS_GROUP_WINDOWS
#include "bt_windows/BTAccessor.h"
#endif


#define TestCaseFunction(_tcf) static_cast<TestDriver::TestCase>(&_tcf)


using namespace ajn;

void XorByteArray(const uint8_t* in1, const uint8_t* in2, uint8_t* out, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        out[i] = in1[i] ^ in2[i];
    }
}


struct CmdLineOptions {
    String basename;
    bool client;
    bool server;
    bool reportDetails;
    bool local;
    bool fastDiscovery;
    bool quiet;
    bool keepgoing;
    CmdLineOptions() :
        basename("org.alljoyn.BTAccessorTester"),
        client(false),
        server(false),
        reportDetails(false),
        local(false),
        fastDiscovery(false),
        quiet(false),
        keepgoing(false)
    {
    }
};

class TestDriver : public BTTransport {
  public:
    typedef bool (TestDriver::*TestCase)();

    struct TestCaseInfo {
        TestDriver::TestCase tc;
        String description;
        bool success;
        TestCaseInfo(TestDriver::TestCase tc, const String& description) :
            tc(tc), description(description), success(false)
        {
        }
    };

    struct DeviceChange {
        BDAddress addr;
        uint32_t uuidRev;
        bool eirCapable;
        DeviceChange(const BDAddress& addr, uint32_t uuidRev, bool eirCapable) :
            addr(addr), uuidRev(uuidRev), eirCapable(eirCapable)
        {
        }
    };


    TestDriver(const CmdLineOptions& opts);
    virtual ~TestDriver();

    void AddTestCase(TestDriver::TestCase tc, const String description);
    int RunTests();

    bool TC_CreateBTAccessor();
    bool TC_DestroyBTAccessor();
    bool TC_StartBTAccessor();
    bool TC_StopBTAccessor();
    bool TC_IsEIRCapable();
    bool TC_StartConnectable();
    bool TC_StopConnectable();

  protected:
    BTAccessor* btAccessor;
    Bus bus;
    const CmdLineOptions& opts;
    qcc::GUID128 busGuid;
    RemoteEndpoint ep;

    deque<bool> btDevAvailQueue;
    Event btDevAvailEvent;
    Mutex btDevAvailLock;

    deque<DeviceChange> devChangeQueue;
    Event devChangeEvent;
    Mutex devChangeLock;

    bool eirCapable;
    BTNodeInfo self;
    BTNodeDB nodeDB;

    void TestBTDeviceAvailable(bool available);
    virtual bool TestCheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const;
    virtual void TestDeviceChange(const BDAddress& bdAddr, uint32_t uuidRev, bool eirCapable);

    void ReportTestDetail(const String& detail, size_t indent = 0) const;
    bool SendBuf(const uint8_t* buf, size_t size);
    bool RecvBuf(uint8_t* buf, size_t size);
    String BuildName(const BTBusAddress& addr, const GUID128& guid, size_t entry);
    void HashName(const BTBusAddress& addr, const GUID128& guid, uint32_t serial, const String& name, String& hash);

    // Don't report the transfer rate unless the number of bytes transferred is >= this many bytes.
    static const size_t TRANFER_RATE_MIN_BYTES = 10000;

    /**
     * If bytesTransferred is >= TRANFER_RATE_MIN_BYTES then report to the user the number of
     * bytes transferred per second.
     *
     *  @param t0[in] The start time of the transfer.
     *  @param t1[in] The end time of the transfer.
     *  @param bytesTransferred[in] The number of bytes transferred.
     *  @param sending[in] True if the transfer was a send, false if it was a receive.
     */
    void ReportTransferRate(uint64_t t0, uint64_t t1, size_t bytesTransferred, bool sending) const;

  private:
    list<TestCaseInfo> tcList;
    uint32_t testcase;
    bool success;
    list<TestCaseInfo>::iterator insertPos;
    size_t maxWidth;
    size_t tcNumWidth;
    static const size_t tcWidth = 2;
    static const size_t tcColonWidth = 2;
    static const size_t pfWidth = 5;
    static const size_t dashWidth = 2;
    size_t detailIndent;
    size_t detailWidth;

    mutable String lastLine;
    mutable uint32_t lastLineRepeat;
    mutable size_t lastIndent;
    mutable bool lastBullet;

    bool silenceDetails;

    void RunTest(TestCaseInfo& test);
    void OutputLine(String line, size_t indent = 0, bool bullet = false) const;

    // Should never be called so making it private.
    // This is created to stop code analysis tools from complaining about the possibilty of
    // double freeing of memory.
    TestDriver& operator=(const TestDriver& source)
    {
        // Not implemented so make sure we don't do anything with it.
        assert(0);
        return *this;
    }

    // Should never be called so making it private.
    // This is created to stop code analysis tools from complaining about the possibilty of
    // double freeing of memory.
    TestDriver(const TestDriver& source) :
        btAccessor(NULL),
        bus("BTAccessorTester", cntr, ""),
        opts(opts),
        testcase(0),
        success(true),
        maxWidth(80),
        tcNumWidth(2),
        detailIndent(tcWidth + tcNumWidth + 1),
        detailWidth(maxWidth - (detailIndent + dashWidth)),
        lastLineRepeat(0),
        lastIndent(0),
        lastBullet(false),
        silenceDetails(false)
    {
        // Not implemented so make sure we don't do anything with it.
        assert(0);
    }
};


class ClientTestDriver : public TestDriver {
  public:
    struct FoundInfo {
        uint32_t found;
        uint32_t changed;
        uint32_t uuidRev;
        bool checked;
        FoundInfo() :
            found(0), changed(0), uuidRev(0), checked(false)
        {
        }
        FoundInfo(uint32_t uuidRev) :
            found(1), changed(0), uuidRev(uuidRev), checked(false)
        {
        }
    };

    ClientTestDriver(const CmdLineOptions& opts);
    virtual ~ClientTestDriver() { }

    bool TestCheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const;
    void TestDeviceChange(const BDAddress& bdAddr, uint32_t uuidRev, bool eirCapable);

    bool TC_StartDiscovery();
    bool TC_StopDiscovery();
    bool TC_GetDeviceInfo();
    bool TC_ConnectSingle();
    bool TC_ConnectSingleReject();
    bool TC_ConnectSingleRedirect();
    bool TC_ConnectMultiple();
    bool TC_ExchangeSmallData();
    bool TC_ExchangeLargeData();
    bool TC_IsMaster();
    bool TC_RequestBTRole();

  private:
    map<BDAddress, FoundInfo> foundInfo;
    uint32_t connUUIDRev;
    BTBusAddress connAddr;
    BTNodeInfo connNode;

    bool ExchangeData(size_t size);
};


class ServerTestDriver : public TestDriver {
  public:
    ServerTestDriver(const CmdLineOptions& opts);
    virtual ~ServerTestDriver() { }

    bool TestCheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const;
    void TestDeviceChange(const BDAddress& bdAddr, uint32_t uuidRev, bool eirCapable);

    bool TC_StartDiscoverability();
    bool TC_StopDiscoverability();
    bool TC_SetSDPInfo();
    bool TC_GetL2CAPConnectEvent();
    bool TC_AcceptSingle();
    bool TC_RejectSingle();
    bool TC_RedirectSingle();
    bool TC_AcceptMultiple();
    bool TC_ExchangeSmallData();
    bool TC_ExchangeLargeData();

  private:
    bool allowIncomingAddress;
    bool redirect;
    uint32_t uuidRev;

    bool ExchangeData(size_t size);
};


TestDriver::TestDriver(const CmdLineOptions& opts) :
    btAccessor(NULL),
    bus("BTAccessorTester", cntr, ""),
    opts(opts),
    testcase(0),
    success(true),
    maxWidth(80),
    tcNumWidth(2),
    detailIndent(tcWidth + tcNumWidth + 1),
    detailWidth(maxWidth - (detailIndent + dashWidth)),
    lastLineRepeat(0),
    lastIndent(0),
    lastBullet(false),
    silenceDetails(false)
{
    String uniqueName = ":";
    uniqueName += busGuid.ToShortString();
    uniqueName += ".1";
    self->SetGUID(busGuid);
    self->SetRelationship(_BTNodeInfo::SELF);
    self->SetUniqueName(uniqueName);

    tcList.push_back(TestCaseInfo(TestCaseFunction(TestDriver::TC_CreateBTAccessor), "Create BT Accessor"));
    tcList.push_back(TestCaseInfo(TestCaseFunction(TestDriver::TC_StartBTAccessor), "Start BTAccessor"));
    tcList.push_back(TestCaseInfo(TestCaseFunction(TestDriver::TC_IsEIRCapable), "Check EIR capability"));
    tcList.push_back(TestCaseInfo(TestCaseFunction(TestDriver::TC_StartConnectable), "Start Connectable"));
    tcList.push_back(TestCaseInfo(TestCaseFunction(TestDriver::TC_StopConnectable), "Stop Connectable"));
    insertPos = tcList.end();
    --insertPos;
    tcList.push_back(TestCaseInfo(TestCaseFunction(TestDriver::TC_StopBTAccessor), "Stop BTAccessor"));
    tcList.push_back(TestCaseInfo(TestCaseFunction(TestDriver::TC_DestroyBTAccessor), "Destroy BTAccessor"));

    bus.Start();
}

TestDriver::~TestDriver()
{
    if (btAccessor) {
        delete btAccessor;
    }
}

void TestDriver::AddTestCase(TestDriver::TestCase tc, const String description)
{
    tcList.insert(insertPos, TestCaseInfo(tc, description));
    tcNumWidth = 1 + ((tcList.size() >= 100) ? 3 :
                      ((tcList.size() >= 10) ? 2 : 1));
    if ((tcWidth + tcNumWidth + 1 + description.size() + tcColonWidth + pfWidth) > (maxWidth)) {
        maxWidth = (tcWidth + tcNumWidth + description.size() + tcColonWidth + pfWidth);
    }

    detailIndent = (tcWidth + tcNumWidth + 2);
    detailWidth = maxWidth - (detailIndent + dashWidth);
}

int TestDriver::RunTests()
{
    list<TestCaseInfo>::iterator it = tcList.begin();

    while ((opts.keepgoing || success) && (it != tcList.end())) {
        TestCaseInfo& test = *it;
        RunTest(test);
        ++it;
        if ((opts.keepgoing || success) && (it != tcList.end())) {
            printf("-------------------------------------------------------------------------------\n");
        }
    }

    printf("===============================================================================\n"
           "Overall: %s\n", success ? "PASS" : "FAIL");

    // If TC_DestroyBTAccessor() was already called then btAccessor will be NULL.
    // But if opts.keepgoing is false and there was a failure then btAccessor needs to be
    // gracefully shut down and deleted.
    if (btAccessor) {
        silenceDetails = true;
        btAccessor->StopConnectable();
        btAccessor->Stop();
        Event::Wait(btDevAvailEvent, 30000);
        delete btAccessor;
        btAccessor = NULL;
    }

    return success ? 0 : 1;
}

void TestDriver::RunTest(TestCaseInfo& test)
{
    String tcLine;
    String line;

    tcLine.append("TC");
    tcLine.append(U32ToString(++testcase, 10, tcNumWidth, ' '));
    tcLine.append(": ");
    tcLine.append(test.description);

    line = tcLine;
    line.append(": Start");
    OutputLine(line);

    test.success = (this->*(test.tc))();

    line = tcLine;
    line.append(": ");
    line.append(test.success ? "PASS" : "FAIL");
    OutputLine(line);

    success = success && test.success;
}

void TestDriver::OutputLine(String line, size_t indent, bool bullet) const
{

    if (line == lastLine) {
        ++lastLineRepeat;
    } else {
        String l;
        if (lastLineRepeat > 0) {
            l = "(Previous line repeated ";
            l += U32ToString(lastLineRepeat);
            l += " times.)";
            lastLineRepeat = 0;
            OutputLine(l, lastIndent, lastBullet);
        } else {
            if (!line.empty()) {
                bool wrapped = false;
                size_t lineWidth = maxWidth - (indent + (bullet ? 2 : 0));
                lastLine = line;
                lastIndent = indent;
                lastBullet = bullet;
                while (!line.empty()) {
                    for (size_t i = 0; i < indent; ++i) {
                        l.push_back(' ');
                    }
                    if (bullet) {
                        l.append(wrapped ? "  " : "- ");
                    }
                    if (line.size() > lineWidth) {
                        String subline = line.substr(0, line.find_last_of(' ', lineWidth));
                        line = line.substr(line.find_first_not_of(" ", subline.size()));
                        l.append(subline);
                    } else {
                        l.append(line);
                        line.clear();
                    }
                    printf("%s\n", l.c_str());
                    l.clear();
                    wrapped = !line.empty();
                }
            }

        }
    }
}

void TestDriver::ReportTestDetail(const String& detail, size_t indent) const
{
    if (opts.reportDetails && !silenceDetails) {
        OutputLine(detail, detailIndent + indent, true);
    }
}

bool TestDriver::SendBuf(const uint8_t* buf, size_t size)
{
    QStatus status;
    size_t offset = 0;
    size_t sent = 0;

    if (!ep->IsValid()) {
        ReportTestDetail("No connection to send data to.  Skipping.");
        return true;
    }

    const size_t totalToSend = size;
    uint64_t t0 = GetTimestamp64();

    while (size > 0) {
        status = ep->GetSink().PushBytes(buf + offset, size, sent);
        if (status != ER_OK) {
            String detail = "Sending ";
            detail += U32ToString(size);
            detail += " bytes failed: ";
            detail += QCC_StatusText(status);
            detail += ".";
            ReportTestDetail(detail);
            return false;
        }

        offset += sent;
        size -= sent;
    }

    uint64_t t1 = GetTimestamp64();

    ReportTransferRate(t0, t1, totalToSend, true);

    return true;
}

bool TestDriver::RecvBuf(uint8_t* buf, size_t size)
{
    QStatus status;
    size_t offset = 0;
    size_t received = 0;

    if (!ep->IsValid()) {
        ReportTestDetail("No connection to send data to.  Skipping.");
        return true;
    }

    const size_t totalToReceive = size;
    uint64_t t0 = GetTimestamp64();

    while (size > 0) {
        status = ep->GetSource().PullBytes(buf + offset, size, received, 30000);
        if (status != ER_OK || 0 == received) {
            String detail = "Receiving ";
            detail += U32ToString(size);
            detail += " bytes failed: ";
            detail += QCC_StatusText(status);
            detail += ". Total received = ";
            detail += U32ToString(offset);
            detail += ". Last received = ";
            detail += U32ToString(received);
            detail += ".";
            ReportTestDetail(detail);
            return false;
        }
        offset += received;
        size -= received;
    }

    uint64_t t1 = GetTimestamp64();

    ReportTransferRate(t0, t1, totalToReceive, false);

    return true;
}

String TestDriver::BuildName(const BTBusAddress& addr, const GUID128& guid, size_t entry)
{
    String hash;
    String baseName = (opts.basename +
                       ".E" +
                       U32ToString((uint32_t)entry, 16, 4, '0') +
                       ".R" +
                       RandHexString(4) +
                       ".H");
    HashName(addr, guid, (uint32_t)entry, baseName, hash);
    return baseName + hash;
}

void TestDriver::HashName(const BTBusAddress& addr,
                          const GUID128& guid,
                          uint32_t serial,
                          const String& name,
                          String& hash)
{
    Crypto_SHA1 sha1;
    uint8_t digest[Crypto_SHA1::DIGEST_SIZE];
    sha1.Init();

    sha1.Update(addr.ToString());
    sha1.Update(guid.ToString());
    sha1.Update(U32ToString(serial, 16, 8, '0'));
    sha1.Update(name);
    sha1.GetDigest(digest);

    hash = BytesToHexString(digest, HASH_SIZE);
}

void TestDriver::TestBTDeviceAvailable(bool available)
{
    String detail = "Received device ";
    detail += available ? "available" : "unavailable";
    detail += " indication from BTAccessor.";
    ReportTestDetail(detail);
    btDevAvailLock.Lock(MUTEX_CONTEXT);
    btDevAvailQueue.push_back(available);
    btDevAvailLock.Unlock(MUTEX_CONTEXT);
    btDevAvailEvent.SetEvent();
}

bool TestDriver::TestCheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const
{
    String detail = "BTAccessor needs BD Address ";
    detail += addr.ToString().c_str();
    detail += " checked: REJECTED (base test driver).";
    ReportTestDetail(detail);

    return false;
}

void TestDriver::ReportTransferRate(uint64_t t0, uint64_t t1, size_t bytesTransferred, bool sending) const
{
    uint64_t tDelta = t1 - t0;

    if (bytesTransferred > TRANFER_RATE_MIN_BYTES && tDelta > 0) {
        uint64_t bytesPerSecond = (bytesTransferred * 1000) / tDelta;
        String detail = sending ? "Sent " : "Received ";

        detail += U64ToString(bytesTransferred);
        detail += " bytes in ";
        detail += U64ToString(tDelta / 1000);
        detail += " seconds. Or ";
        detail += U64ToString(bytesPerSecond);
        detail += " bytes per second.";

        ReportTestDetail(detail);
    }
}

bool ClientTestDriver::TestCheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const
{
    String detail = "BTAccessor needs BD Address ";
    detail += addr.ToString().c_str();
    detail += " checked: REJECTED (client test driver).";
    ReportTestDetail(detail);

    return false;
}

bool ServerTestDriver::TestCheckIncomingAddress(const BDAddress& addr, BTBusAddress& redirectAddr) const
{
    String detail = "BTAccessor needs BD Address ";
    detail += addr.ToString().c_str();
    detail += " checked: ";
    if (redirect) {
        redirectAddr = redirectAddress;
        detail += "redirected to ";
        detail += redirectAddr.ToString();
        detail += ".";
    } else {
        detail += allowIncomingAddress ? "allowed." : "rejected.";
    }
    ReportTestDetail(detail);

    return allowIncomingAddress;
}

void TestDriver::TestDeviceChange(const BDAddress& bdAddr,
                                  uint32_t uuidRev,
                                  bool eirCapable)
{
    ReportTestDetail("BTAccessor reported a found device to use.  Ignoring since this is the base Test Driver.");
}

void ClientTestDriver::TestDeviceChange(const BDAddress& bdAddr,
                                        uint32_t uuidRev,
                                        bool eirCapable)
{
    String detail = "BTAccessor reported a found device to use: ";
    detail += bdAddr.ToString().c_str();
    if (eirCapable) {
        detail += ".  It is EIR capable with a UUID revision of 0x";
        detail += U32ToString(uuidRev, 16, 8, '0');
        detail += ".";
    } else {
        detail += ".  It is not EIR capable.";
    }
    ReportTestDetail(detail);

    devChangeLock.Lock(MUTEX_CONTEXT);
    devChangeQueue.push_back(DeviceChange(bdAddr, uuidRev, eirCapable));
    devChangeLock.Unlock(MUTEX_CONTEXT);
    devChangeEvent.SetEvent();
}

void ServerTestDriver::TestDeviceChange(const BDAddress& bdAddr,
                                        uint32_t uuidRev,
                                        bool eirCapable)
{
    ReportTestDetail("BTAccessor reported a found device to use.  Ignoring since this is the Server Test Driver.");
}


/****************************************/

bool TestDriver::TC_CreateBTAccessor()
{
    btAccessor = new BTAccessor(this, busGuid.ToString());

    return true;
}

bool TestDriver::TC_DestroyBTAccessor()
{
    delete btAccessor;
    btAccessor = NULL;
    return true;
}

bool TestDriver::TC_StartBTAccessor()
{
    bool available = false;

    btDevAvailLock.Lock(MUTEX_CONTEXT);
    btDevAvailQueue.clear();
    btDevAvailLock.Unlock(MUTEX_CONTEXT);
    btDevAvailEvent.ResetEvent();

    QStatus status = btAccessor->Start();
    if (status != ER_OK) {
        String detail = "Call to start BT device failed: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        goto exit;
    }

    do {
        status = Event::Wait(btDevAvailEvent, 30000);
        if (status != ER_OK) {
            String detail = "Waiting for BT device available notification failed: ";
            detail += QCC_StatusText(status);
            detail += ".";
            ReportTestDetail(detail);
            goto exit;
        }

        btDevAvailEvent.ResetEvent();

        btDevAvailLock.Lock(MUTEX_CONTEXT);
        while (!btDevAvailQueue.empty()) {
            available = btDevAvailQueue.front();
            btDevAvailQueue.pop_front();
        }
        btDevAvailLock.Unlock(MUTEX_CONTEXT);

        if (!available) {
            fprintf(stderr, "Please enable system's Bluetooth.\n");
        }
    } while (!available);

exit:
    return (status == ER_OK);
}

bool TestDriver::TC_StopBTAccessor()
{
    bool available = true;
    QStatus status = ER_OK;

    btAccessor->Stop();

    do {
        status = Event::Wait(btDevAvailEvent, 30000);
        if (status != ER_OK) {
            String detail = "Waiting for BT device available notification failed: ";
            detail += QCC_StatusText(status);
            detail += ".";
            ReportTestDetail(detail);
            goto exit;
        }

        btDevAvailEvent.ResetEvent();

        btDevAvailLock.Lock(MUTEX_CONTEXT);
        while (!btDevAvailQueue.empty()) {
            available = btDevAvailQueue.front();
            btDevAvailQueue.pop_front();
        }
        btDevAvailLock.Unlock(MUTEX_CONTEXT);
    } while (available);

exit:
    return (status == ER_OK);
}

bool TestDriver::TC_IsEIRCapable()
{
    eirCapable = btAccessor->IsEIRCapable();
    self->SetEIRCapable(eirCapable);
    String detail = "The local device is ";
    detail += eirCapable ? "EIR capable" : "not EIR capable";
    detail += ".";
    ReportTestDetail(detail);
    return true;
}

bool TestDriver::TC_StartConnectable()
{
    QStatus status;
    BTBusAddress addr;
    String detail;

    status = btAccessor->StartConnectable(addr.addr, addr.psm);
    bool tcSuccess = (status == ER_OK);
    if (tcSuccess) {
        self->SetBusAddress(addr);
        nodeDB.AddNode(self);
        detail = "Now connectable on ";
        detail += self->GetBusAddress().ToString();
    } else {
        String detail = "Call to start connectable returned failure code: ";
        detail += QCC_StatusText(status);
    }
    detail += ".";
    ReportTestDetail(detail);

    return tcSuccess;
}

bool TestDriver::TC_StopConnectable()
{
    bool tcSuccess = true;
    btAccessor->StopConnectable();
    Event* l2capEvent = btAccessor->GetL2CAPConnectEvent();
    if (l2capEvent) {
        QStatus status = Event::Wait(*l2capEvent, 500);
        if ((status == ER_OK) ||
            (status == ER_TIMEOUT)) {
            ReportTestDetail("L2CAP connect event object is still valid.");
            tcSuccess = false;
        }
    }

    nodeDB.RemoveNode(self);

    return tcSuccess;
}


bool ClientTestDriver::TC_StartDiscovery()
{
    bool tcSuccess = true;
    QStatus status;
    BDAddressSet ignoreAddrs;
    String detail;
    map<BDAddress, FoundInfo>::iterator fit;

    if (!opts.fastDiscovery) {
        uint64_t now;
        uint64_t stop;
        Timespec tsNow;

        GetTimeNow(&tsNow);
        now = tsNow.GetAbsoluteMillis();
        stop = now + 35000;

        devChangeLock.Lock(MUTEX_CONTEXT);
        devChangeQueue.clear();
        devChangeEvent.ResetEvent();
        devChangeLock.Unlock(MUTEX_CONTEXT);

        ReportTestDetail("Starting discovery for 30 seconds.");
        status = btAccessor->StartDiscovery(ignoreAddrs, 30);
        if (status != ER_OK) {
            detail = "Call to start discovery failed: ";
            detail += QCC_StatusText(status);
            detail += ".";
            ReportTestDetail(detail);
            tcSuccess = false;
            goto exit;
        }

        while (now < stop) {
            status = Event::Wait(devChangeEvent, stop - now);
            if (status == ER_TIMEOUT) {
                break;
            } else if (status != ER_OK) {
                detail = "Wait for device change event failed: ";
                detail += QCC_StatusText(status);
                detail += ".";
                ReportTestDetail(detail);
                tcSuccess = false;
                goto exit;
            }

            devChangeEvent.ResetEvent();

            devChangeLock.Lock(MUTEX_CONTEXT);
            while (!devChangeQueue.empty()) {
                fit = foundInfo.find(devChangeQueue.front().addr);
                if (fit == foundInfo.end()) {
                    foundInfo[devChangeQueue.front().addr] = FoundInfo(devChangeQueue.front().uuidRev);
                } else {
                    ++(fit->second.found);
                    if (fit->second.uuidRev != devChangeQueue.front().uuidRev) {
                        ++(fit->second.changed);
                        fit->second.uuidRev = devChangeQueue.front().uuidRev;
                    }
                }
                devChangeQueue.pop_front();
            }
            devChangeLock.Unlock(MUTEX_CONTEXT);

            GetTimeNow(&tsNow);
            now = tsNow.GetAbsoluteMillis();
        }

        if (foundInfo.empty()) {
            ReportTestDetail("No devices found.");
        } else {
            for (fit = foundInfo.begin(); fit != foundInfo.end(); ++fit) {
                detail = "Found ";
                detail += fit->first.ToString().c_str();
                detail += " ";
                detail += U32ToString(fit->second.found).c_str();
                detail += " times";
                if (fit->second.changed > 0) {
                    detail += " - changed ";
                    detail += U32ToString(fit->second.changed).c_str();
                    detail += " times";
                }
                detail += " (UUID Rev: 0x";
                detail += U32ToString(fit->second.uuidRev, 16, 8, '0');
                detail += ")";
                detail += ".";
                ReportTestDetail(detail);
            }
        }

        qcc::Sleep(5000);

        devChangeLock.Lock(MUTEX_CONTEXT);
        devChangeQueue.clear();
        devChangeEvent.ResetEvent();
        devChangeLock.Unlock(MUTEX_CONTEXT);

        ReportTestDetail("Waiting for 30 seconds after discovery should have stopped for late found device indications.");
        status = Event::Wait(devChangeEvent, 30000);
        if (status != ER_TIMEOUT) {
            ReportTestDetail("Received device found notification long after discovery should have stopped.");
            tcSuccess = false;
            devChangeLock.Lock(MUTEX_CONTEXT);
            devChangeQueue.clear();
            devChangeEvent.ResetEvent();
            devChangeLock.Unlock(MUTEX_CONTEXT);
            goto exit;
        }
    }

    ReportTestDetail("Starting infinite discovery.");
    status = btAccessor->StartDiscovery(ignoreAddrs, 0);
    if (status != ER_OK) {
        detail = "Call to start discovery with infinite timeout failed: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
    }

exit:
    return tcSuccess;
}

bool ClientTestDriver::TC_StopDiscovery()
{
    QStatus status;
    bool tcSuccess = true;
    BDAddressSet ignoreAddrs;
    String detail;

    ReportTestDetail("Stopping infinite discovery.");
    status = btAccessor->StopDiscovery();
    if (status != ER_OK) {
        detail = "Call to stop discovery failed: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    if (!opts.fastDiscovery) {
        qcc::Sleep(5000);

        devChangeLock.Lock(MUTEX_CONTEXT);
        devChangeQueue.clear();
        devChangeEvent.ResetEvent();
        devChangeLock.Unlock(MUTEX_CONTEXT);

        ReportTestDetail("Waiting for 30 seconds after stopping discovery for late found device indications.");
        status = Event::Wait(devChangeEvent, 30000);
        if (status != ER_TIMEOUT) {
            ReportTestDetail("Received device found notification long after discovery should have stopped.");
            tcSuccess = false;
            devChangeLock.Lock(MUTEX_CONTEXT);
            devChangeQueue.clear();
            devChangeEvent.ResetEvent();
            devChangeLock.Unlock(MUTEX_CONTEXT);
        }
    }

exit:
    return tcSuccess;
}

bool ClientTestDriver::TC_GetDeviceInfo()
{
    QStatus status;
    bool tcSuccess = true;
    map<BDAddress, FoundInfo>::iterator fit;
    bool found = false;
    uint64_t now;
    uint64_t stop;
    Timespec tsNow;
    String detail;
    BTNodeDB connAdInfo;

    GetTimeNow(&tsNow);
    now = tsNow.GetAbsoluteMillis();
    stop = now + 70000;

    while (!found && (now < stop)) {
        for (fit = foundInfo.begin(); !found && (fit != foundInfo.end()); ++fit) {
            if (!fit->second.checked) {
                detail = "Checking ";
                detail += fit->first.ToString();
                detail += ".";
                ReportTestDetail(detail);
                status = btAccessor->GetDeviceInfo(fit->first, &connUUIDRev, &connAddr, &connAdInfo);
                if (status != ER_OK) {
                    detail = "Failed to get device information from ";
                    detail += fit->first.ToString();
                    detail += " (non-critical): ";
                    detail += QCC_StatusText(status);
                    detail += ".";
                    ReportTestDetail(detail);
                } else if (connUUIDRev != bt::INVALID_UUIDREV) {
                    BTNodeDB::const_iterator nit;
                    for (nit = connAdInfo.Begin(); !found && (nit != connAdInfo.End()); ++nit) {
                        NameSet::const_iterator nsit;
                        for (nsit = (*nit)->GetAdvertiseNamesBegin();
                             !found && (nsit != (*nit)->GetAdvertiseNamesEnd());
                             ++nsit) {
                            if (nsit->compare(0, opts.basename.size(), opts.basename) == 0) {
                                found = true;
                            }
                        }
                    }
                }
            }
        }

        if (!found) {
            status = Event::Wait(devChangeEvent, 60000);
            if (status != ER_OK) {
                detail = "Wait for device change event failed: ";
                detail += QCC_StatusText(status);
                detail += ".";
                ReportTestDetail(detail);
                tcSuccess = false;
                goto exit;
            }

            devChangeEvent.ResetEvent();

            devChangeLock.Lock(MUTEX_CONTEXT);
            while (!devChangeQueue.empty()) {
                fit = foundInfo.find(devChangeQueue.front().addr);
                if (fit == foundInfo.end()) {
                    foundInfo[devChangeQueue.front().addr] = FoundInfo(devChangeQueue.front().uuidRev);
                } else {
                    ++(fit->second.found);
                    if (fit->second.uuidRev != devChangeQueue.front().uuidRev) {
                        ++(fit->second.changed);
                        fit->second.uuidRev = devChangeQueue.front().uuidRev;
                    }
                }
                devChangeQueue.pop_front();
            }
            devChangeLock.Unlock(MUTEX_CONTEXT);

            GetTimeNow(&tsNow);
            now = tsNow.GetAbsoluteMillis();
        }
    }

    if (found) {
        detail = "Found \"";
        detail += opts.basename;
        detail += "\" in advertisement for device with connect address ";
        detail += connAddr.ToString();
        detail += ".";
        ReportTestDetail(detail);
        connNode = connAdInfo.FindNode(connAddr);

        // Validate the SDP info
        if (connAdInfo.Size() != (NUM_SECONDARY_NODES + 1)) {
            tcSuccess = false;
            detail = "Not enough nodes in advertisement: only ";
            detail += U32ToString((uint32_t)connAdInfo.Size());
            detail += " out of ";
            detail += U32ToString((uint32_t)(NUM_SECONDARY_NODES + 1));
            ReportTestDetail(detail);
            goto exit;
        }

        BTNodeDB::const_iterator it;
        for (it = connAdInfo.Begin(); it != connAdInfo.End(); ++it) {
            const size_t expectedNameCount = (*it == connNode) ? NUM_PRIMARY_NAMES : NUM_SECONDARY_NAMES;
            BTNodeInfo node = *it;
            NameSet::const_iterator nit;
            size_t entry;

            if (node->AdvertiseNamesSize() != expectedNameCount) {
                tcSuccess = false;
                detail = "Not enough advertised names for ";
                detail += node->GetBusAddress().ToString();
                detail += ": only ";
                detail += U32ToString((uint32_t)node->AdvertiseNamesSize());
                detail += " out of ";
                detail += U32ToString((uint32_t)expectedNameCount);
                ReportTestDetail(detail);
                goto exit;
            }

            for (entry = 0, nit = node->GetAdvertiseNamesBegin(); nit != node->GetAdvertiseNamesEnd(); ++entry, ++nit) {
                const String& fullName = *nit;
                String hash;
                String baseName = fullName.substr(0, fullName.size() - (2 * HASH_SIZE));
                HashName(node->GetBusAddress(), node->GetGUID(), (uint32_t)entry, baseName, hash);
                if (fullName.compare(fullName.size() - (2 * HASH_SIZE), String::npos, hash) != 0) {
                    tcSuccess = false;
                    ReportTestDetail("Check of SDP information failed:");
                    detail = "addr = ";
                    detail += node->GetBusAddress().ToString();
                    ReportTestDetail(detail, 2);
                    detail = "GUID = ";
                    detail += node->GetGUID().ToString();
                    ReportTestDetail(detail, 2);
                    detail = "name = ";
                    detail += fullName;
                    ReportTestDetail(detail, 2);
                    detail = "exp =  ";
                    detail += baseName + hash;
                    ReportTestDetail(detail, 2);
                    goto exit;
                }
            }
        }

    } else {
        ReportTestDetail("Failed to find corresponding device running BTAccessorTester in service mode.");
        tcSuccess = false;
    }

exit:
    return tcSuccess;
}

bool ClientTestDriver::TC_ConnectSingle()
{
    bool tcSuccess = true;
    BTNodeInfo node;
    String detail;

    if (!connNode->IsValid()) {
        ReportTestDetail("Cannot continue with connection testing.  Connection address not set (no device found).");
        tcSuccess = false;
        goto exit;
    }

    detail = "Connecting to ";
    detail += connNode->GetBusAddress().ToString();
    detail += ".";
    ReportTestDetail(detail);

    ep = btAccessor->Connect(bus, connNode);

    if (!ep->IsValid()) {
        String detail = "Failed to connect to ";
        detail += connNode->GetBusAddress().ToString();
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    node = (BTEndpoint::cast(ep))->GetNode();

    node->SetSessionID(0xdeadbeef);

    if ((node != connNode) || (connNode->GetSessionID() != 0xdeadbeef)) {
        detail = "BTAccessor failed to put the connection BTNodeInfo into the BTEndpoint instance (";
        detail += node->GetBusAddress().ToString();
        detail += " != ";
        detail += connNode->GetBusAddress().ToString();
        detail += " || ";
        detail += U32ToString(node->GetSessionID(), 16, 8, 0);
        detail += " != ";
        detail += U32ToString(connNode->GetSessionID(), 16, 8, 0);
        ReportTestDetail(detail);
        tcSuccess = false;
        ep->Invalidate();
    }

exit:
    return tcSuccess;
}

bool ClientTestDriver::TC_ConnectSingleReject()
{
    bool tcSuccess = true;
    BTNodeInfo node;
    String detail;
    RemoteEndpoint tep;
    char buf[100];
    size_t size = sizeof(buf);
    size_t received;
    QStatus status;

    if (!connNode->IsValid()) {
        ReportTestDetail("Cannot continue with connection testing.  Connection address not set (no device found).");
        tcSuccess = false;
        goto exit;
    }

    detail = "Connecting to ";
    detail += connNode->GetBusAddress().ToString();
    detail += ".";
    ReportTestDetail(detail);

    tep = btAccessor->Connect(bus, connNode);

    if (!tep->IsValid()) {
        String detail = "Connection to ";
        detail += connNode->GetBusAddress().ToString();
        detail += " failed when it should have succeeded.";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    status = tep->GetSource().PullBytes(buf, size, received, 1000);
    if (status != ER_SOCK_OTHER_END_CLOSED) {
        ReportTestDetail("Server side failed to reject the connection.");
        tcSuccess = false;
    }

exit:
    return tcSuccess;
}

bool ClientTestDriver::TC_ConnectSingleRedirect()
{
    bool tcSuccess = true;
    BTNodeInfo node;
    String detail;
    RemoteEndpoint tep;
    BTBusAddress raddr;
    String authName;
    String redirectSpec;
    QStatus status;

    if (!connNode->IsValid()) {
        ReportTestDetail("Cannot continue with connection testing.  Connection address not set (no device found).");
        tcSuccess = false;
        goto exit;
    }

    detail = "Connecting to ";
    detail += connNode->GetBusAddress().ToString();
    detail += ".";
    ReportTestDetail(detail);

    tep = btAccessor->Connect(bus, connNode);

    if (!tep->IsValid()) {
        ReportTestDetail("Failed to create outgoing connection.");
        tcSuccess = false;
        goto exit;
    }

    tep->GetFeatures().isBusToBus = true;
    tep->GetFeatures().allowRemote = true;
    tep->GetFeatures().handlePassing = false;

    status = tep->Establish("ANONYMOUS", authName, redirectSpec);
    if (status != ER_BUS_ENDPOINT_REDIRECTED) {
        detail = "Connection establishment failed to get redirect spec: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    raddr.FromSpec(redirectSpec);

    if (raddr != redirectAddress) {
        detail = "Redirect address ";
        detail += raddr.ToString();
        detail += " (redirectSpec = \"";
        detail += redirectSpec;
        detail += "\")";
        detail += " does not match expected value: ";
        detail += redirectAddress.ToString();
        detail += ".";
        tcSuccess = false;
        ReportTestDetail(detail);
        goto exit;
    }

    detail = "Got redirect address ";
    detail += raddr.ToString();
    detail += " from redirect bus spec \"";
    detail += redirectSpec;
    detail += "\".";
    ReportTestDetail(detail);

exit:
    qcc::Sleep(3000);
    return tcSuccess;
}

bool ClientTestDriver::TC_ConnectMultiple()
{
    bool tcSuccess = true;
    RemoteEndpoint eps[CONNECT_MULTIPLE_MAX_CONNECTIONS];

    memset(eps, 0, sizeof(eps));

    if (!connNode->IsValid()) {
        ReportTestDetail("Cannot continue with connection testing.  Connection address not set (no device found).");
        tcSuccess = false;
        goto exit;
    }

    for (size_t i = 0; i < ArraySize(eps); ++i) {
        eps[i] = btAccessor->Connect(bus, connNode);

        if (!eps[i]->IsValid()) {
            String detail = "Failed connect ";
            detail += U32ToString(i);
            detail += " to ";
            detail += connNode->GetBusAddress().ToString();
            detail += ".";
            ReportTestDetail(detail);
            tcSuccess = false;
            goto exit;
        }

        char sendBuffer[80];
        size_t sent;

        uint8_t length = snprintf(sendBuffer, ArraySize(sendBuffer), "Endpoint %d.", (int) i) + 1;    // Include the nul.
        QStatus status = eps[i]->GetSink().PushBytes(&length, sizeof(length), sent);

        if (ER_OK == status && sizeof(length) == sent) {
            status = eps[i]->GetSink().PushBytes(sendBuffer, length, sent);
        }

        if (ER_OK != status || length != sent) {
            String detail = "Failed PushBytes() on endpoint ";
            detail += U32ToString(i);
            detail += " to ";
            detail += connNode->GetBusAddress().ToString();
            detail += ".";
            ReportTestDetail(detail);
            tcSuccess = false;
            goto exit;
        }

        size_t received;
        char receiveBuffer[ArraySize(sendBuffer)];

        status = eps[i]->GetSource().PullBytes(receiveBuffer, length, received, 10000);

        if (ER_OK != status || length != received || memcmp(sendBuffer, receiveBuffer, length)) {
            String detail = "Failed PullBytes() on endpoint ";
            detail += U32ToString(i);
            detail += " to ";
            detail += connNode->GetBusAddress().ToString();
            detail += ".";
            ReportTestDetail(detail);
            tcSuccess = false;
            goto exit;
        }
    }

exit:
    return tcSuccess;
}

bool ClientTestDriver::TC_ExchangeSmallData()
{
    return ExchangeData(EXCHANGE_DATA_SMALL);
}

bool ClientTestDriver::TC_ExchangeLargeData()
{
    return ExchangeData(EXCHANGE_DATA_LARGE);
}

bool ClientTestDriver::TC_IsMaster()
{
    QStatus status;
    bool tcSuccess = true;
    bool master;
    String detail;

    status = btAccessor->IsMaster(connAddr.addr, master);
    if (status != ER_OK) {
        detail = "Failed to get BT master/slave role: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    detail = "We are ";
    detail += master ? "the master (slave is " : "a slave (";
    detail += "prefered but not required).";
    ReportTestDetail(detail);

exit:
    return tcSuccess;
}

bool ClientTestDriver::TC_RequestBTRole()
{
    QStatus status;
    bool tcSuccess = true;
    bool oldMaster;
    bool newMaster;
    String detail;

    status = btAccessor->IsMaster(connAddr.addr, oldMaster);
    if (status != ER_OK) {
        detail = "Failed to get BT master/slave role: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    btAccessor->RequestBTRole(connAddr.addr, oldMaster ? bt::SLAVE : bt::MASTER);

    status = btAccessor->IsMaster(connAddr.addr, newMaster);
    if (status != ER_OK) {
        detail = "Failed to get BT master/slave role: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    if (newMaster == oldMaster) {
        ReportTestDetail("Failed to change BT master/slave role.");
        tcSuccess = false;
    }

exit:
    return tcSuccess;
}

static String GetOffsetOfDifference(uint8_t* buf, uint8_t* expBuf, size_t bufSize)
{
    String returnValue;
    size_t offset;

    for (offset = 0; offset < bufSize; offset++) {
        if (buf[offset] != expBuf[offset]) {
            String offsetString = U32ToString((uint32_t)offset);
            String bufVal = BytesToHexString(buf + offset, 1);
            String expBufVal = BytesToHexString(expBuf + offset, 1);

            returnValue += "buf[";
            returnValue += offsetString;
            returnValue += "] = 0x";
            returnValue += bufVal;
            returnValue += ", expBuf[";
            returnValue += offsetString;
            returnValue += "] = 0x";
            returnValue += expBufVal;
            break;
        }
    }

    return returnValue;
}

bool ClientTestDriver::ExchangeData(size_t size)
{
    bool tcSuccess = true;
    size_t bufSize = size * qcc::GUID128::SIZE;
    uint8_t* txBuf = new uint8_t[bufSize];
    uint8_t* rxBuf = new uint8_t[bufSize];
    uint8_t* buf = new uint8_t[bufSize];
    uint8_t* expBuf = new uint8_t[bufSize];

    for (size_t i = 0; i < bufSize; i += qcc::GUID128::SIZE) {
        memcpy(txBuf + i, busGuid.GetBytes(), qcc::GUID128::SIZE);
        memcpy(expBuf + i, connNode->GetGUID().GetBytes(), qcc::GUID128::SIZE);
    }

    tcSuccess = SendBuf(txBuf, bufSize);
    if (!tcSuccess) {
        goto exit;
    }

    tcSuccess = RecvBuf(rxBuf, bufSize);
    if (!tcSuccess) {
        goto exit;
    }

    XorByteArray(txBuf, rxBuf, buf, bufSize);

    if (memcmp(buf, expBuf, bufSize) != 0) {
        String detail = "Received bytes does not match expected.";
        ReportTestDetail(detail);
        detail = GetOffsetOfDifference(buf, expBuf, bufSize);
        ReportTestDetail(detail);
        tcSuccess = false;
    }

exit:
    // Give some time for the transfer to complete before terminating the connection.
    qcc::Sleep(1000);

    delete[] txBuf;
    delete[] rxBuf;
    delete[] buf;
    delete[] expBuf;

    return tcSuccess;
}


bool ServerTestDriver::TC_StartDiscoverability()
{
    bool tcSuccess = true;
    QStatus status;

    status = btAccessor->StartDiscoverability();
    if (status != ER_OK) {
        String detail = "Call to start discoverability failed: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
    }

    return tcSuccess;
}

bool ServerTestDriver::TC_StopDiscoverability()
{
    bool tcSuccess = true;
    QStatus status;

    status = btAccessor->StopDiscoverability();
    if (status != ER_OK) {
        String detail = "Call to stop discoverability failed: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
    }

    return tcSuccess;
}

bool ServerTestDriver::TC_SetSDPInfo()
{
    bool tcSuccess = true;
    QStatus status;
    String detail;
    size_t i;

    // Advertise primary names for the local device.
    for (i = 0; i < NUM_PRIMARY_NAMES; ++i) {
        self->AddAdvertiseName(BuildName(self->GetBusAddress(), self->GetGUID(), i));
    }

    // Advertise names for secondary nodes
    for (i = 0; i < NUM_SECONDARY_NODES; ++i) {
        BDAddress addr(RandHexString(6));
        BTBusAddress busAddr(addr, (uint16_t)i + 1);
        BTNodeInfo fakeNode;
        size_t j;
        fakeNode = BTNodeInfo(busAddr);
        for (j = 0; j < NUM_SECONDARY_NAMES; ++j) {
            fakeNode->AddAdvertiseName(BuildName(fakeNode->GetBusAddress(), fakeNode->GetGUID(), j));
        }
        nodeDB.AddNode(fakeNode);
    }

    status = btAccessor->SetSDPInfo(uuidRev,
                                    self->GetBusAddress().addr,
                                    self->GetBusAddress().psm,
                                    nodeDB);
    if (status != ER_OK) {
        detail = "Call to set SDP information returned failure code: ";
        detail += QCC_StatusText(status);
        tcSuccess = false;
    } else {
        detail = "UUID revision for SDP record set to 0x";
        detail += U32ToString(uuidRev, 16, 8, '0');
    }
    detail += ".";
    ReportTestDetail(detail);

    return tcSuccess;
}

bool ServerTestDriver::TC_GetL2CAPConnectEvent()
{
    bool tcSuccess = false;
    Event* l2capEvent = btAccessor->GetL2CAPConnectEvent();
    if (l2capEvent) {
        QStatus status = Event::Wait(*l2capEvent, 500);
        if ((status == ER_OK) ||
            (status == ER_TIMEOUT)) {
            tcSuccess = true;
        } else {
            ReportTestDetail("L2CAP connect event object is invalid.");
        }
    } else {
        ReportTestDetail("L2CAP connect event object does not exist.");
    }
    return tcSuccess;
}

bool ServerTestDriver::TC_AcceptSingle()
{
    bool tcSuccess = true;
    QStatus status;
    BTNodeInfo node;
    String detail;
    BDAddress invalidAddr;

    Event* l2capEvent = btAccessor->GetL2CAPConnectEvent();

    if (!l2capEvent) {
        ReportTestDetail("L2CAP connect event object does not exist.");
        tcSuccess = false;
        goto exit;
    }

    ReportTestDetail("Waiting up to 3 minutes for incoming connection.");
    status = Event::Wait(*l2capEvent, 180000);
    if (status != ER_OK) {
        detail = "Failed to wait for incoming connection: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    ep = btAccessor->Accept(bus, l2capEvent);

    if (!ep->IsValid()) {
        ReportTestDetail("Failed to accept incoming connection.");
        tcSuccess = false;
        goto exit;
    }

    node = (BTEndpoint::cast(ep))->GetNode();

    if ((node->GetBusAddress().addr == invalidAddr) || (node->GetBusAddress().psm != bt::INCOMING_PSM)) {
        ReportTestDetail("BTAccessor failed to fill out the BTNodeInfo with appropriate data in the BTEndpoint instance.");
        ep->Invalidate();
        tcSuccess = false;
        goto exit;
    }

    detail = "Accepted connection from ";
    detail += node->GetBusAddress().addr.ToString();
    detail += ".";
    ReportTestDetail(detail);

exit:
    return tcSuccess;
}

bool ServerTestDriver::TC_RejectSingle()
{
    bool tcSuccess = true;
    QStatus status;
    BTNodeInfo node;
    String detail;
    BDAddress invalidAddr;
    RemoteEndpoint tep;

    allowIncomingAddress = false;

    Event* l2capEvent = btAccessor->GetL2CAPConnectEvent();

    if (!l2capEvent) {
        ReportTestDetail("L2CAP connect event object does not exist.");
        tcSuccess = false;
        goto exit;
    }

    ReportTestDetail("Waiting up to 3 minutes for incoming connection.");
    status = Event::Wait(*l2capEvent, 180000);
    if (status != ER_OK) {
        detail = "Failed to wait for incoming connection: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    tep = btAccessor->Accept(bus, l2capEvent);

    if (tep->IsValid()) {
        ReportTestDetail("Failed to reject incoming connection.");
        tcSuccess = false;
    }

exit:
    allowIncomingAddress = true;

    return tcSuccess;
}

bool ServerTestDriver::TC_RedirectSingle()
{
    bool tcSuccess = true;
    QStatus status;
    BTNodeInfo node;
    String detail;
    BDAddress invalidAddr;
    RemoteEndpoint tep;
    BTBusAddress raddr;
    String authName;
    String unused;

    redirect = true;

    Event* l2capEvent = btAccessor->GetL2CAPConnectEvent();

    if (!l2capEvent) {
        ReportTestDetail("L2CAP connect event object does not exist.");
        tcSuccess = false;
        goto exit;
    }

    ReportTestDetail("Waiting up to 3 minutes for incoming connection.");
    status = Event::Wait(*l2capEvent, 180000);
    if (status != ER_OK) {
        detail = "Failed to wait for incoming connection: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }

    tep = btAccessor->Accept(bus, l2capEvent);

    if (!tep->IsValid()) {
        ReportTestDetail("Failed to accept incoming connection.");
        tcSuccess = false;
        goto exit;
    }

    status = tep->Establish("ANONYMOUS", authName, unused);
    if (status != ER_BUS_ENDPOINT_REDIRECTED) {
        detail = "Failed to redirect communications: ";
        detail += QCC_StatusText(status);
        detail += ".";
        ReportTestDetail(detail);
        tcSuccess = false;
        goto exit;
    }


exit:
    qcc::Sleep(3000);

    redirect = false;

    return tcSuccess;
}

bool ServerTestDriver::TC_AcceptMultiple()
{
    bool tcSuccess = true;
    QStatus status;
    RemoteEndpoint eps[CONNECT_MULTIPLE_MAX_CONNECTIONS];

    Event* l2capEvent = btAccessor->GetL2CAPConnectEvent();

    memset(eps, 0, sizeof(eps));

    if (!l2capEvent) {
        ReportTestDetail("L2CAP connect event object does not exist.");
        tcSuccess = false;
        goto exit;
    }

    ReportTestDetail("Waiting up to 30 seconds for incoming connections.");
    for (size_t i = 0; i < ArraySize(eps); ++i) {
        status = Event::Wait(*l2capEvent, 30000);
        if (status != ER_OK) {
            String detail = "Failed to wait for incoming connection: ";
            detail += QCC_StatusText(status);
            detail += ".";
            ReportTestDetail(detail);
            tcSuccess = false;
            goto exit;
        }

        eps[i] = btAccessor->Accept(bus, l2capEvent);

        if (!eps[i]->IsValid()) {
            String detail = "Failed to accept incoming connection ";
            detail += U32ToString(i);
            detail += ".";
            ReportTestDetail(detail);
            tcSuccess = false;
            goto exit;
        }

        size_t received;
        uint8_t length;
        QStatus status = eps[i]->GetSource().PullBytes(&length, sizeof(length), received);

        char receiveBuffer[80];

        if (ER_OK == status && length <= sizeof(receiveBuffer)) {
            status = eps[i]->GetSource().PullBytes(receiveBuffer, length, received, 10000);
        }

        if (ER_OK != status || length != received) {
            String detail = "Failed PullBytes() on endpoint ";
            detail += U32ToString(i);
            detail += ".";
            ReportTestDetail(detail);
            tcSuccess = false;
            goto exit;
        }

        String endpointDescription = "Received buffer '";

        endpointDescription += receiveBuffer;
        endpointDescription += "'";
        ReportTestDetail(endpointDescription);

        size_t sent;
        status = eps[i]->GetSink().PushBytes(receiveBuffer, length, sent);

        if (ER_OK != status || length != sent) {
            String detail = "Failed PushBytes() on endpoint ";
            detail += U32ToString(i);
            detail += ".";
            ReportTestDetail(detail);
            tcSuccess = false;
            goto exit;
        }
    }

exit:

    return tcSuccess;
}

bool ServerTestDriver::TC_ExchangeSmallData()
{
    return ExchangeData(EXCHANGE_DATA_SMALL);
}

bool ServerTestDriver::TC_ExchangeLargeData()
{
    return ExchangeData(EXCHANGE_DATA_LARGE);
}

bool ServerTestDriver::ExchangeData(size_t size)
{
    bool tcSuccess = true;
    size_t bufSize = size * qcc::GUID128::SIZE;
    uint8_t* txBuf = new uint8_t[bufSize];
    uint8_t* rxBuf = new uint8_t[bufSize];
    uint8_t* buf = new uint8_t[bufSize];

    for (size_t i = 0; i < bufSize; i += qcc::GUID128::SIZE) {
        memcpy(buf + i, busGuid.GetBytes(), qcc::GUID128::SIZE);
    }

    tcSuccess = RecvBuf(rxBuf, bufSize);
    if (!tcSuccess) {
        goto exit;
    }

    XorByteArray(rxBuf, buf, txBuf, bufSize);

    tcSuccess = SendBuf(txBuf, bufSize);
    qcc::Sleep(1000); // Wait for data to be received before disconnecting.

exit:
    delete[] txBuf;
    delete[] rxBuf;
    delete[] buf;

    return tcSuccess;
}


/****************************************/

ClientTestDriver::ClientTestDriver(const CmdLineOptions& opts) :
    TestDriver(opts)
{
    AddTestCase(TestCaseFunction(ClientTestDriver::TC_StartDiscovery), "Start Discovery (~70 sec)");
    if (!opts.local) {
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_GetDeviceInfo), "Get Device Information");
    }
    AddTestCase(TestCaseFunction(ClientTestDriver::TC_StopDiscovery), "Stop Discovery (~35 sec)");
    if (!opts.local) {
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_ConnectSingle), "Single Connection to Server");
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_ConnectSingleReject), "Single Connection to Server - Trigger Reject");
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_ConnectSingleRedirect), "Single Connection to Server - Trigger Redirect on Server");
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_ConnectMultiple), "Multiple Simultaneous Connections to Server");
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_ExchangeSmallData), "Exchange Small Amount of Data");
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_ExchangeLargeData), "Exchange Large Amount of Data");
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_IsMaster), "Check BT master/slave role");
        AddTestCase(TestCaseFunction(ClientTestDriver::TC_RequestBTRole), "Switch BT master/slave role");
    }
}


ServerTestDriver::ServerTestDriver(const CmdLineOptions& opts) :
    TestDriver(opts),
    allowIncomingAddress(true),
    redirect(false)
{
    while (uuidRev == bt::INVALID_UUIDREV) {
        uuidRev = Rand32();
    }

    AddTestCase(TestCaseFunction(ServerTestDriver::TC_SetSDPInfo), "Set SDP Information");
    AddTestCase(TestCaseFunction(ServerTestDriver::TC_GetL2CAPConnectEvent), "Check L2CAP Connect Event Object");
    AddTestCase(TestCaseFunction(ServerTestDriver::TC_StartDiscoverability), "Start Discoverability");
    if (!opts.local) {
        AddTestCase(TestCaseFunction(ServerTestDriver::TC_AcceptSingle), "Accept Single Incoming Connection");
        AddTestCase(TestCaseFunction(ServerTestDriver::TC_RejectSingle), "Reject Single Incoming Connection");
        AddTestCase(TestCaseFunction(ServerTestDriver::TC_RedirectSingle), "Accept Single Incoming Connection - Check Redirect");
        AddTestCase(TestCaseFunction(ServerTestDriver::TC_AcceptMultiple), "Accept Multiple Incoming Connections");
        AddTestCase(TestCaseFunction(ServerTestDriver::TC_ExchangeSmallData), "Exchange Small Amount of Data");
        AddTestCase(TestCaseFunction(ServerTestDriver::TC_ExchangeLargeData), "Exchange Large Amount of Data");
    }
    AddTestCase(TestCaseFunction(ServerTestDriver::TC_StopDiscoverability), "Stop Discoverability");
}


/****************************************/

static void Usage(void)
{
    printf("Usage: BTAccessorTester OPTIONS...\n"
           "\n"
           "    -h              Print this help message\n"
           "    -c              Run in client mode\n"
           "    -s              Run in server mode\n"
           "    -l              Only run local tests (skip inter-device tests)\n"
           "    -n <basename>   Set the base name for advertised/find names\n"
           "    -f              Fast discovery (client only - skips some discovery testing)\n"
           "    -q              Quiet - suppress debug and log errors\n"
           "    -d              Output test details\n"
           "    -k              Keep going if a test case fails\n");
}

static void ParseCmdLine(int argc, char** argv, CmdLineOptions& opts)
{
    int i;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            Usage();
            exit(0);
        } else if (strcmp(argv[i], "-c") == 0) {
            if (opts.server) {
                printf("Cannot specify server and client at the same time.\n");
                Usage();
                exit(-1);
            }
            opts.client = true;
        } else if (strcmp(argv[i], "-s") == 0) {
            if (opts.client) {
                printf("Cannot specify server and client at the same time.\n");
                Usage();
                exit(-1);
            }
            opts.server = true;
        } else if (strcmp(argv[i], "-n") == 0) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                Usage();
                exit(-1);
            } else {
                opts.basename = argv[i];
            }
        } else if (strcmp(argv[i], "-d") == 0) {
            opts.reportDetails = true;
        } else if (strcmp(argv[i], "-l") == 0) {
            opts.local = true;
        } else if (strcmp(argv[i], "-f") == 0) {
            opts.fastDiscovery = true;
        } else if (strcmp(argv[i], "-q") == 0) {
            opts.quiet = true;
        } else if (strcmp(argv[i], "-k") == 0) {
            opts.keepgoing = true;
        }
    }
}

void DebugOutputHandler(DbgMsgType type,
                        const char* module,
                        const char* msg,
                        void* context)
{
}

int main(int argc, char** argv)
{
#if defined(NDEBUG) && defined(QCC_OS_ANDROID)
    LoggerSetting::GetLoggerSetting("bbdaemon", LOG_ERR, true, NULL);
#else
    LoggerSetting::GetLoggerSetting("bbdaemon", LOG_DEBUG, false, stdout);
#endif

    TestDriver* driver;

    CmdLineOptions opts;

    ParseCmdLine(argc, argv, opts);

    if (opts.quiet) {
        QCC_RegisterOutputCallback(DebugOutputHandler, NULL);
    }

    if (opts.client) {
        driver = new ClientTestDriver(opts);
    } else if (opts.server) {
        driver = new ServerTestDriver(opts);
    } else {
        driver = new TestDriver(opts);
    }

    int ret = driver->RunTests();
    delete driver;

    return ret;
}
