//
//  ViewController.swift
//  BasicClient
//
//  Created by Olga Konoreva on 03/11/16.
//  Copyright © 2016 AllJoyn. All rights reserved.
//

import UIKit

class ViewController: UIViewController, BasicClientDelegate {

    @IBOutlet weak var callServiceButton: UIButton!
    @IBOutlet weak var eventsTextView: UITextView!
    
    let basicClient = BasicClient()
    
    override func viewDidLoad() {
        super.viewDidLoad()
    }
    
    @IBAction func callServiceButtonTapped(_ sender: Any) {
        basicClient.delegate = self
        basicClient.sendHelloMessage()
    }
    
    func didReceiveStatusUpdateMessage(message: String) {
        
        DispatchQueue.main.async {
            var currentText = self.eventsTextView.text
            
            let date = NSDate()
            let calendar = NSCalendar.current
            let hour = calendar.component(.hour, from: date as Date)
            let minutes = calendar.component(.minute, from: date as Date)
            let seconds = calendar.component(.second, from: date as Date)
            
            currentText!.append("\(hour):\(minutes):\(seconds)\n")
            currentText!.append(message)
            
            self.eventsTextView.text = currentText
            print("\(currentText)")
        }
        
        print(message)
    }
}

