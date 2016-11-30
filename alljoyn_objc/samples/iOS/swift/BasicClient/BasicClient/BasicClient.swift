////////////////////////////////////////////////////////////////////////////////
// Copyright AllSeen Alliance. All rights reserved.
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
        } else {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment started.\n")
        }
        
        // connect to the message bus
        //
        status = bus.connect(withArguments: "null:")
        
        if status != ER_OK {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachment::Connect(\"null:\") failed\n")
        } else {
            delegate?.didReceiveStatusUpdateMessage(message: "BusAttachement connected to null:\n")
        }
        
        joinedSessionCondition.lock()
        
        // register a bus listener in order to receive discovery notifications
        //
        bus.register(self)

        // begin discovery of the well known name of the service to be called
        //
        status = bus.findAdvertisedName(kBasicClientServiceName)
        
        delegate?.didReceiveStatusUpdateMessage(message: "Waiting to discover service...\n")
        
        // wait for the join session to complete:
        // AJNBusListener::didFindAdvertisedName(_:withTransportMask:namePrefix) listener is finished
        //
        if joinedSessionCondition.wait(until: Date(timeIntervalSinceNow: 60)) {
            
            let proxyBusObject = AJNProxyBusObject(busAttachment: bus, serviceName: foundServiceName, objectPath: kBasicClientServicePath, sessionId: sessionId!)
            
            // get a description of the interfaces implemented by the remote object before making the call
            //
            proxyBusObject!.introspectRemoteObject()
            printIntrospectedInterfaces(proxyBusObject: proxyBusObject)

            // create the first argument for the remote method call
            //
            let messageArgumentFirst = AJNMessageArgument()
            
            // the approach of strings converting will be improved
            // now it's implemented via NSString to be able to have one method for the both languages: ObjectiveC and Swift
            //
            let nsStringFirst: NSString = "Code "
            
            // utf8 encoding should be used for correct working with C-strings
            //
            let cStringFirst = nsStringFirst.cString(using: String.Encoding.utf8.rawValue)
            
            // 'getVaList' returns CVaListPointer, that is swift implementation of С va_list
            //
            messageArgumentFirst.setValue("s", arguments: getVaList([cStringFirst!]))
            messageArgumentFirst.stabilize()
            
            // create the second argument for the remote method call
            //
            let messageArgumentSecond = AJNMessageArgument()
            let nsStringSecond: NSString = "monkeys!"
            let cStringSecond = nsStringSecond.cString(using: String.Encoding.utf8.rawValue)
            messageArgumentSecond.setValue("s", arguments: getVaList([cStringSecond!]))
            messageArgumentSecond.stabilize()
            
            // create mutable pointer for interacting with C API
            //
            var reply: AJNMessage? = nil
            
            // now make the function call of the remote object created in BasicService sample
            //
            status = proxyBusObject!.callMethod(withName: "cat", onInterfaceWithName: kBasicClientInterfaceName, withArguments: [messageArgumentFirst, messageArgumentSecond], methodReply: &reply, timeout: 5000, flags: 0)
            
            print("Remote method call status: \(status)")

            if status == ER_OK {

                let resultXmlOptional = reply!.arg(0).xml

                // compare result xml in reply with the expected one
                //
                if let resultXml = resultXmlOptional {
                    let resultConclusion = resultXml == "<string>Code monkeys!</string>" ? "Successfully" : "Unsuccessfully"
                    delegate?.didReceiveStatusUpdateMessage(message: "\(resultConclusion) called method on remote object.\n")
                }
            } else {
                delegate?.didReceiveStatusUpdateMessage(message: "ERROR: ProxyBusObject::MethodCall on org.alljoyn.Bus.sample failed. \(AJNStatus.description(forStatusCode: status))")
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
        
        // Unregister listener
        //
        self.bus!.unregisterBusListener(self)
        
        // Destroy listener (temporary decision). When framework unregistering is fixed,
        // direct destroy calling will be removed.
        //
        self.bus!.destroy(self)
        
        print("destroyed bus")
        
        delegate?.didReceiveStatusUpdateMessage(message: "Bus listener is unregistered.\n")
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

