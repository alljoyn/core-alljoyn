//
//  BasicClient.swift
//  BasicClient
//
//  Created by Olga Konoreva on 03/11/16.
//  Copyright © 2016 AllJoyn. All rights reserved.
//

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
    private var clientQueue, lockQueue: DispatchQueue?

    func sendHelloMessage() {
        clientQueue = DispatchQueue(label: "org.alljoyn.swift.BasicClient.ClientQueue")
        clientQueue!.async {
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
        
        // start the message bus
        //
        status = bus!.start()
        
        if status != ER_OK {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment::Start failed\n")
        } else {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment started.\n")
        }
        
        // connect to the message bus
        //
        status = bus!.connect(withArguments: "null:")
        
        if status != ER_OK {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment::Connect(\"null:\") failed\n")
        } else {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachement connected to null:\n")
        }
        
        joinedSessionCondition.lock()
        
        // register a bus listener in order to receive discovery notifications
        //
        bus!.register(self)

        // begin discovery of the well known name of the service to be called
        //
        status = bus!.findAdvertisedName(kBasicClientServiceName)
        
        delegate?.didReceiveStatusUpdateMessage(message: "Waiting to discover service...\n")
        
        // wait for the join session to complete:
        // AJNBusListener::didFindAdvertisedName(_:withTransportMask:namePrefix) listener is finished
        //
        if joinedSessionCondition.wait(until: Date(timeIntervalSinceNow: 60)) {
            
            // once joined to a session, use a proxy object to make the function call
            //
            let basicObjectProxy = BasicObjectProxy(busAttachment: bus, serviceName: foundServiceName, objectPath: kBasicClientServicePath, sessionId: sessionId!)
            
            basicObjectProxy!.introspectRemoteObject()
            
            
            let result = basicObjectProxy!.concatenateString("Code ", with: "Monkeys!")
            
            if result != nil {
                let resultConclusion = result == "Code Monkeys!" ? "Successfully" : "Unsuccessfully"

                print("[\(result)]\(resultConclusion) concatenated string.")
                delegate?.didReceiveStatusUpdateMessage(message: "Successfully called method on remote object!!!\n")
            }
       
            ping()
            
        } else {
            delegate?.didReceiveStatusUpdateMessage(message: "Timed out while attempting to join a session with BasicService...")
        }
        
        joinedSessionCondition.unlock()
        
        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        print("+ Destroying bus attachment                                                               +")
        print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
        
        // Unregister listener will be called here
        //
        self.bus!.unregisterBusListener(self)
        
        delegate?.didReceiveStatusUpdateMessage(message: "Bus listener is unregistered.\n")
    }
    
    func ping() {
        if let bus = self.bus {
            let status = bus.pingPeer(kBasicClientServiceName, withTimeout: 5)
            delegate?.didReceiveStatusUpdateMessage(message: status == ER_OK ? "Ping returned Successfully " : "Ping Failed ")
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
            
            lockQueue = DispatchQueue(label: "org.alljoyn.swift.BasicClient.LockQueue")
            lockQueue!.sync {
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

