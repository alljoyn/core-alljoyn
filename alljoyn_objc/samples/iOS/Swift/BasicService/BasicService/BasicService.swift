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

let kBasicServiceName = "org.alljoyn.Bus.sample"
let kBasicServicePath = "/sample"
let kBasicServicePort: AJNSessionPort = 25

protocol BasicServiceDelegate: class {

    func didReceiveStatusUpdateMessage(message: String)
}

class BasicService: NSObject, AJNBusListener, AJNSessionPortListener, AJNSessionListener {

    weak var delegate: BasicServiceDelegate?

    private var bus: AJNBusAttachment?
    private var basicObject: BasicObject?

    func startServiceAsync() {
        DispatchQueue.global(qos: .default).async {
            self.startService()
        }
    }

    func stopServiceAsync() {
        DispatchQueue.global(qos: .default).async {
            self.stopService(bus: self.bus)
        }
    }

    func printVersionInformation() {
        if let versionInformation = AJNVersion.versionInformation() {
            delegate?.didReceiveStatusUpdateMessage(message: versionInformation)
        }

        if let buildInformation = AJNVersion.buildInformation() {
            delegate?.didReceiveStatusUpdateMessage(message: buildInformation)
        }
    }

    private func startService() {

        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        print("+ Creating bus attachment                                                                 +")
        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")

        var status: QStatus

        // create the message bus
        //
        bus = AJNBusAttachment(applicationName: "BasicService", allowRemoteMessages: true)

        guard let bus = self.bus else {
            print("BusAttachment:: initialization bus is failed")
            return
        }

        // register a bus listener
        //
        bus.register(self)

        // allocate the service's bus object, which also creates and activates the service's interface
        //
        basicObject = BasicObject(busAttachment: bus, onPath: kBasicServicePath)

        // start the message bus
        //
        status = bus.start()

        if status != ER_OK {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment::Start failed")
            return
        }

        // register the bus object
        //
        status = bus.register(basicObject)

        delegate?.didReceiveStatusUpdateMessage(message: status == ER_OK ? "Object registered successfully." : "ERROR: Could not register bus object")

        // connect to the message bus
        //
        status = bus.connect(withArguments: "null:")

        delegate?.didReceiveStatusUpdateMessage(message: status == ER_OK ? "Bus is connected to null: transport" : "Failed to connect to null: transport")

        // Advertise this service on the bus
        // There are three steps to advertising this service on the bus
        //   1) Request a well-known name that will be used by the client to discover
        //       this service
        //   2) Create a session
        //   3) Advertise the well-known name
        //

        // request the name
        //
        status = bus.requestWellKnownName(kBasicServiceName, withFlags: kAJNBusNameFlagReplaceExisting | kAJNBusNameFlagDoNotQueue)

        if (status != ER_OK) {
            print("ERROR: Request for name: \(kBasicServiceName) is failed")
        }

        // bind a session to a service port
        //
        let sessionOptions = AJNSessionOptions(trafficType: kAJNTrafficMessages, supportsMultipoint: true, proximity: kAJNProximityAny, transportMask: kAJNTransportMaskAny)

        status = bus.bindSession(onPort: kBasicServicePort, with: sessionOptions, withDelegate: self)

        if (status != ER_OK) {
            print("ERROR: Could not bind session on port \(kBasicServicePort)")
        }

        // advertise a name
        //
        status = bus.advertiseName(kBasicServiceName, withTransportMask: kAJNTransportMaskAny)
        if (status != ER_OK) {
            print("Could not advertise \(kBasicServiceName)")
        }
    }

    private func stopService(bus: AJNBusAttachment?) {

        guard let bus = self.bus else {
            print("stopService: Bus is nil")
            return
        }

        // cancel name advertising
        //
        bus.cancelAdvertisedName(kBasicServiceName, withTransportMask: kAJNTransportMaskAny)

        // unbind session
        //
        bus.unbindSession(fromPort: kBasicServicePort)

        // clean up
        //
        bus.unregisterBusObject(basicObject)

        // unregister listener
        //
        bus.unregisterBusListener(self)

        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        print("+ Destroying bus attachment                                                               +")
        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")

        // destroy listener
        //
        self.bus!.destroy(self)

        delegate?.didReceiveStatusUpdateMessage(message: "Bus listener unregistered.")

        self.bus!.disconnect()
        self.bus!.stop()

        delegate?.didReceiveStatusUpdateMessage(message: "Bus is stopped.")
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
    }

    func didLoseAdvertisedName(_ name: String, withTransportMask transport:AJNTransportMask, namePrefix:String) {
        print("AJNBusListener::didLoseAdvertisedName:\(name) withTransportMask:\(transport) namePrefix:\(namePrefix)")
    }

    func nameOwnerChanged(_ name: String, to newOwner: String, from previousOwner:String) {
        print("AJNBusListener::nameOwnerChanged:\(name) to:\(newOwner) from:\(previousOwner)")
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

    func sessionWasLost(sessionId: AJNSessionId, forReason reason: AJNSessionLostReason) {
        print("AJNSessionListener::sessionWasLost \(sessionId) forReason \(reason)")
    }

    func didAddMemberNamed(_ memberName: String, toSession sessionId: AJNSessionId) {
        print("AJNSessionListener::didAddMemberNamed \(memberName) toSession \(sessionId)")
    }

    func didRemoveMemberNamed(_ memberName: String, fromSession sessionId: AJNSessionId) {
        print("AJNSessionListener::didRemoveMemberNamed \(memberName) fromSession \(sessionId)")
    }


    ////////////////////////////////////////////////////////////////////////////////
    //
    // MARK: AJNSessionPortListener delegate methods
    //

    func shouldAcceptSessionJoinerNamed(_ joiner: String, onSessionPort sessionPort: AJNSessionPort, with options: AJNSessionOptions) -> Bool {

        print("AJNSessionPortListener::shouldAcceptSessionJoinerNamed:\(joiner) onSessionPort:\(sessionPort) withSessionOptions:")
        let shouldAcceptSessionJoiner = kBasicServicePort == sessionPort
        self.delegate?.didReceiveStatusUpdateMessage(message: "Request from \(joiner) to join session is \(shouldAcceptSessionJoiner ? "accepted" : "rejected").")
        return shouldAcceptSessionJoiner
    }

    func didJoin(_ joiner: String, inSessionWithId sessionId: AJNSessionId, onSessionPort sessionPort: AJNSessionPort) {
        print("AJNSessionPortListener::didJoyn")

        //set session listener to be able get callbacks of AJNSessionListener delegate
        //
        self.bus?.enableConcurrentCallbacks()
        self.bus?.setSessionListener(self, toSession: sessionId)
    }
}
