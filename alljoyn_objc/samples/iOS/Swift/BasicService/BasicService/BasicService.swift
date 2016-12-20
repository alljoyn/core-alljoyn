//
//  BasicService.swift
//  BasicService
//
//  Created by Olga Konoreva on 30/11/16.
//  Copyright © 2016 AllJoyn. All rights reserved.
//

import Foundation

////////////////////////////////////////////////////////////////////////////////
//
// Constants
//

let kBasicServiceInterfaceName = "org.alljoyn.Bus.sample"
let kBasicServiceName = "org.alljoyn.Bus.sample"
let kBasicServicePath = "/sample"
let kBasicServicePort: AJNSessionPort = 25

protocol BasicServiceDelegate: class {

    func didReceiveStatusUpdateMessage(message: String)
}

class BasicService: NSObject, AJNBusListener, AJNSessionPortListener, AJNSessionListener {

    weak var delegate: BasicServiceDelegate?

    private let waitCondition = NSCondition()
    private var serviceQueue = DispatchQueue(label: "org.alljoyn.basic-service.serviceQueue")
    private var bus: AJNBusAttachment?
    private var basicObject: BasicObject?

    func startService() {
        serviceQueue.async {
            self.run()
        }
    }

    private func run() {

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
        bus = AJNBusAttachment(applicationName: "BasicService", allowRemoteMessages: true)

        guard let bus = self.bus else {
            print("BusAttachment:: initialization bus failed")
            return
        }

        self.waitCondition.lock()

        // register a bus listener
        //
        bus.register(self)

        // allocate the service's bus object, which also creates and activates the service's interface
        //
        basicObject = BasicObject(busAttachment: bus, onPath:kBasicServicePath)

        // start the message bus
        //
        status = bus.start()

        if status != ER_OK {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment::Start failed\n")
        }

        // register the bus object
        //
        status = bus.register(basicObject)

        delegate?.didReceiveStatusUpdateMessage(message: status == ER_OK ? "Object registered successfully.\n" : "ERROR: Could not register bus object/n")

        // connect to the message bus
        //
        status = bus.connect(withArguments: "null:")

        delegate?.didReceiveStatusUpdateMessage(message: status == ER_OK ? "Bus now connected to null: transport\n" : "Failed to connect to null: transport")

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
            print("ERROR: Request for name: \(kBasicServiceName) failed")
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

        // wait until the client leaves before tearing down the service
        //
        waitCondition.wait(until: Date(timeIntervalSinceNow: 600))

        // clean up
        //
        bus.unregisterBusObject(basicObject)

        waitCondition.unlock()

        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        print("+ Destroying bus attachment                                                               +")
        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")

        // unregister listener
        //
        bus.unregisterBusListener(self)

        // Destroy listener (temporary decision). When framework unregistering is fixed,
        // direct destroy calling will be removed.
        //
        self.bus!.destroy(self)

        print("destroyed bus")

        delegate?.didReceiveStatusUpdateMessage(message: "Bus listener is unregistered.\n")
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
        self.delegate?.didReceiveStatusUpdateMessage(message: "Request from \(joiner) to join session is \(shouldAcceptSessionJoiner ? "accepted" : "rejected").\n")
        return shouldAcceptSessionJoiner
    }

    func didJoin(_ joiner: String, inSessionWithId sessionId: AJNSessionId, onSessionPort sessionPort: AJNSessionPort) {
        print("AJNSessionPortListener::didJoyn")
    }
}
