//
//  ViewController.swift
//  BasicService
//
//  Created by Olga Konoreva on 30/11/16.
//  Copyright © 2016 AllJoyn. All rights reserved.
//

import UIKit

class ViewController: UIViewController, BasicServiceDelegate {

    @IBOutlet weak var eventsTextView: UITextView!

    let basicService = BasicService()

    override func viewDidLoad() {
        super.viewDidLoad()
        basicService.delegate = self
        basicService.startService()
    }

    func didReceiveStatusUpdateMessage(message: String) {

        print(message)

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
    }
}
