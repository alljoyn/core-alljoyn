////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
//    Project (AJOSP) Contributors and others.
//    
//    SPDX-License-Identifier: Apache-2.0
//    
//    All rights reserved. This program and the accompanying materials are
//    made available under the terms of the Apache License, Version 2.0
//    which accompanies this distribution, and is available at
//    http://www.apache.org/licenses/LICENSE-2.0
//    
//    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
//    Alliance. All rights reserved.
//    
//    Permission to use, copy, modify, and/or distribute this software for
//    any purpose with or without fee is hereby granted, provided that the
//    above copyright notice and this permission notice appear in all
//    copies.
//    
//    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//    PERFORMANCE OF THIS SOFTWARE.
////////////////////////////////////////////////////////////////////////////////

import Foundation

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

let kBasicClientInterfaceName = "org.alljoyn.Bus.sample"
let kBasicClientServiceName = "org.alljoyn.Bus.sample"
let kBasicClientServicePath = "/sample"
let kBasicClientServicePort: AJNSessionPort = 25

protocol BasicClientDelegate: class {

    func didReceiveStatusUpdateMessage(message: String)
}

class BasicClient: NSObject, AJNBusListener, AJNSessionListener {

    weak var delegate: BasicClientDelegate?

    private let joinedSessionCondition = NSCondition()
    private var bus: AJNBusAttachment?
    private var sessionId: AJNSessionId?
    private var foundServiceName: String?
    private var wasNameAlreadyFound = false
    private var clientQueue = DispatchQueue(label: "org.alljoyn.swift.BasicClient.ClientQueue")
    private var lockQueue = DispatchQueue(label: "org.alljoyn.swift.BasicClient.LockQueue")

    func sendHelloMessage() {
        clientQueue.async {
            self.wasNameAlreadyFound = false
            self.run()
        }
    }

    func run() {

        if let versionInformation = AJNVersion.versionInformation() {
            delegate?.didReceiveStatusUpdateMessage(message: "\(versionInformation)\n")
        }

        if let buildInformation = AJNVersion.buildInformation() {
            delegate?.didReceiveStatusUpdateMessage(message: "\(buildInformation)\n")
        }

        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        print("+ Creating bus attachment                                                                 +")
        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")

        var status: QStatus

        // create the message bus
        //
        bus = AJNBusAttachment(applicationName: "BasicClient", allowRemoteMessages: true)

        guard let bus = self.bus else {
            print("BusAttachment:: initialization bus failed")
            return
        }

        // start the message bus
        //
        status = bus.start()

        if status != ER_OK {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment::Start failed\n")
            return
        } else {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment started.\n")
        }

        // connect to the message bus
        //
        status = bus.connect(withArguments: "null:")

        if status != ER_OK {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment::Connect(\"null:\") failed\n")
            return
        } else {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachement connected to null:\n")
        }

        joinedSessionCondition.lock()

        // register a bus listener in order to receive discovery notifications
        //
        bus.register(self)

        // begin discovery of the well known name of the service to be called
        //
        bus.findAdvertisedName(kBasicClientServiceName)

        delegate?.didReceiveStatusUpdateMessage(message: "Waiting to discover service...\n")

        // wait for the join session to complete:
        // AJNBusListener::didFindAdvertisedName(_:withTransportMask:namePrefix) listener is finished
        //
        if joinedSessionCondition.wait(until: Date(timeIntervalSinceNow: 60)) {


            // once joined to a session, use a proxy object to make the function call
            //
            let basicObjectProxy = BasicObjectProxy(busAttachment: bus, serviceName: foundServiceName, objectPath: kBasicClientServicePath, sessionId: sessionId!)

            basicObjectProxy!.introspectRemoteObject()


            let resultOptional = basicObjectProxy!.concatenateString("Code ", with: "monkeys!")

            if let result = resultOptional {

                // compare  result string with the expected one
                //
                let resultConclusion = result == "Code monkeys!" ? "Successfully" : "Unsuccessfully"
                delegate?.didReceiveStatusUpdateMessage(message: "\(resultConclusion) called method on remote object.\n")
            } else {
                delegate?.didReceiveStatusUpdateMessage(message: "ERROR: ProxyBusObject::MethodCall on org.alljoyn.Bus.sample failed.")
            }

            ping()

            print("finished joinedSessionCondition block")

        } else {
            delegate?.didReceiveStatusUpdateMessage(message: "Timed out while attempting to join a session with BasicService...")
        }

        joinedSessionCondition.unlock()

        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        print("+ Destroying bus attachment                                                               +")
        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")

        // Unregister and destroy listener
        //
        self.bus!.unregisterBusListener(self)
        self.bus!.destroy(self)

        print("destroyed bus")

        delegate?.didReceiveStatusUpdateMessage(message: "Bus listener is unregistered and destroyed.\n")
    }

    func ping() {
        if let bus = self.bus {
            let status = bus.pingPeer(kBasicClientServiceName, withTimeout: 5)
            delegate?.didReceiveStatusUpdateMessage(message: status == ER_OK ? "Ping returned Successfully " : "Ping Failed ")
        }
    }

    func printIntrospectedInterfaces(proxyBusObject: AJNProxyBusObject?) {
        if let size = proxyBusObject?.interfaces.count {
            print("AJNProxyBusObject interfaces count : \(size)")
            for i in 0...size - 1 {
                let description: AJNInterfaceDescription = proxyBusObject?.interfaces[i] as! AJNInterfaceDescription
                print("name = \(description.name)")

                for index in 0...description.members.count - 1 {
                    let interfaceMember: AJNInterfaceMember = description.members[index] as! AJNInterfaceMember

                    print("interface member = \(interfaceMember)")
                    print("member name = \(interfaceMember.name)")
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // MARK: AJNBusListener delegate methods
    //

    func listenerDidRegister(withBus busAttachment: AJNBusAttachment) {
        print("AJNBusListener::listenerDidRegister withBus:\(busAttachment)")
    }

    func listenerDidUnregister(withBus busAttachment: AJNBusAttachment?) {
        print("AJNBusListener::listenerDidUnregister withBus:\(busAttachment)")
    }

    func didFindAdvertisedName(_ name: String, withTransportMask transport: AJNTransportMask, namePrefix: String) {
        print("AJNBusListener::didFindAdvertisedName:\(name) withTransportMask:\(transport) namePrefix:\(namePrefix)")

        if namePrefix.caseInsensitiveCompare(kBasicClientServiceName) == .orderedSame {

            var shouldReturn = false

            lockQueue.sync {
                shouldReturn = self.wasNameAlreadyFound
                self.wasNameAlreadyFound = true
            }

            if shouldReturn {
                delegate?.didReceiveStatusUpdateMessage(message: "Already found an advertised name, ignoring this name \(name)...")
                joinedSessionCondition.signal()
                return
            }

            // Since we are in a callback we must enable concurrent callbacks before calling a
            // synchronous method.
            //
            bus!.enableConcurrentCallbacks()

            let options = AJNSessionOptions(trafficType: kAJNTrafficMessages, supportsMultipoint: true, proximity: kAJNProximityAny, transportMask:kAJNTransportMaskAny)
            sessionId = bus!.joinSession(withName: name, onPort: kBasicClientServicePort, withDelegate: self, options: options)

            if sessionId != nil {

                foundServiceName = name
                delegate?.didReceiveStatusUpdateMessage(message: "JoinSession SUCCESS (Session id=\(sessionId!))")
            } else {
                delegate?.didReceiveStatusUpdateMessage(message: "JoinSession failed\n")
            }

            joinedSessionCondition.signal()
        }
    }

    func didLoseAdvertisedName(_ name: String, withTransportMask transport:AJNTransportMask, namePrefix:String) {
        print("AJNBusListener::didLoseAdvertisedName:\(name) withTransportMask:\(transport) namePrefix:\(namePrefix)")
    }

    func nameOwnerChanged(_ name: String, to newOwner: String, from previousOwner:String) {
        delegate?.didReceiveStatusUpdateMessage(message: "NameOwnerChanged: name=\(name), previousOwner=\(previousOwner), newOwner=\(newOwner)\n")
    }

    func busWillStop() {
        print("AJNBusListener::busWillStop")
    }

    func busDidDisconnect() {
        print("AJNBusListener::busDidDisconnect")
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    // MARK: AJNSessionListener delegate methods
    //

    func sessionWasLost(sessionId: AJNSessionId) {
        print("AJNBusListener::sessionWasLost \(sessionId)")
    }

    func sessionWasLost(sessionId: AJNSessionId, forReason reason: AJNSessionLostReason) {
        print("AJNSessionListener::sessionWasLost \(sessionId) forReason \(reason)")
    }

    func didAddMemberNamed(_ memberName: String, toSession sessionId: AJNSessionId) {
        print("AJNSessionListener::didAddMemberNamed \(memberName) toSession \(sessionId)")
    }

    func didRemoveMemberNamed(_ memberName: String, fromSession sessionId: AJNSessionId) {
        print("AJNSessionListener::didRemoveMemberNamed \(memberName) fromSession \(sessionId)")
    }
}
