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
