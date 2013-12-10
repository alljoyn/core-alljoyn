/*
 * Copyright (c) 2011 - 2012, AllSeen Alliance. All rights reserved.
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
 */
var $ = function (id) {
    return document.getElementById(id);
}

var joinChannelButton = $('joinChannelButton'),
    log = $('log'),
    startChannelButton = $('startChannelButton'),
    statusLabel = $('status');

var displayStatus = function (msg) {
    statusLabel.innerHTML = msg;
}

var onKeyPress = function (event) {
    var c = event.which ? event.which : event.keyCode,
        message;

    if (c === 13) {
        message = document.forms['message'];
        alljoyn.chat(message.text.value);
        message.reset();
        return false;
    }
};

joinChannelButton.onclick = function () {
    var channelNames = document.forms['joinChannel'].channelNames,
        channelName = channelNames.value,
        onJoined,
        onError;

    if (joinChannelButton.innerHTML === 'Join Channel') {
        onJoined = function () {
            while (log.hasChildNodes()) {
                log.removeChild(log.firstChild);
            }
            channelNames.disabled = true;
            joinChannelButton.innerHTML = 'Leave Channel';
            joinChannelButton.disabled = false;
            displayStatus('Join channel successfully');
        };
        onError = function (status) {
            joinChannelButton.disabled = false;
            displayStatus('Join channel failed. Error = 0x' + status.toString(16).toUpperCase());
        };
        try {
            alljoyn.joinChannel(onJoined, onError, channelName);
            joinChannelButton.disabled = true;
            displayStatus('Joining channel ' + channelName + ' ...');
        } catch (err) {
            displayStatus('Join channel failed. Error = ' + AllJoyn.AllJoynException.getErrorMessage(err.number));
            console.log(err.stack);
        }
    } else {
        try {
            alljoyn.leaveChannel();
        } catch (err) {
            console.log('Join channel failed. Error = ' + AllJoyn.AllJoynException.getErrorMessage(err.number));
            console.log(err.stack);
        }
        joinChannelButton.innerHTML = 'Join Channel';
        alljoyn.onname();   /* Refresh list of channels being advertised. */
    }
    return false;
};

startChannelButton.onclick = function () {
    var channelName = document.forms['setChannel'].channelName;

    if (startChannelButton.innerHTML === 'Start Channel') {
        try {
            alljoyn.startChannel(channelName.value);
            startChannelButton.innerHTML = 'Stop Channel';
            channelName.disabled = true;
            displayStatus("Start channel " + channelName.value + " successfully.");
        } catch (err) {
            displayStatus("Start channel " + channelName.value + " failed. Error = " + AllJoyn.AllJoynException.getErrorMessage(err.number));
            console.log(err.stack);
        }
    } else {
        try {
            alljoyn.stopChannel();
            startChannelButton.innerHTML = 'Start Channel';
            channelName.disabled = false;
            displayStatus("Stop channel " + channelName.value + " successfully.");
        } catch (err) {
            displayStatus("Stop channel " + channelName.value + " failed. Error = " + AllJoyn.AllJoynException.getErrorMessage(err.number));
            console.log(err.stack);
        }
    }
    return false;
};

alljoyn.onname = function () {
    var channelNames = document.forms['joinChannel'].channelNames,
        i,
        option;

    /*
     * Don't touch the channel list while we are joined.  The session can be "stopped" by the host,
     * but it is still alive for any joined parties.
     */
    if (joinChannelButton.innerHTML === 'Leave Channel') {
        return;
    }

    while (channelNames.hasChildNodes()) {
        channelNames.removeChild(channelNames.firstChild);
    }
    if (alljoyn.channelNames.length) {
        for (i = 0; i < alljoyn.channelNames.length; ++i) {
            option = document.createElement('option');
            option.innerHTML = alljoyn.channelNames[i];
            channelNames.appendChild(option);
        }
        channelNames.disabled = false;
        joinChannelButton.disabled = false;
    } else {
        option = document.createElement('option');
        option.innerHTML = 'Searching...';
        channelNames.appendChild(option);
        channelNames.disabled = true;
        joinChannelButton.disabled = true;
    }
};

alljoyn.onchat = function (sessionId, message) {
    var li,
        timestamp,
        name,
        text;

    li = document.createElement('li');
    timestamp = document.createElement('span');
    timestamp.className = 'timestamp';
    timestamp.innerHTML = '[' + new Date().toLocaleTimeString() + '] ';
    li.appendChild(timestamp);
    name = document.createElement('span');
    name.className = 'ssId';
    name.innerHTML = '(' + sessionId + ') ';
    li.appendChild(name);
    text = document.createElement('span');
    text.className = 'text';
    text.innerHTML = message;
    li.appendChild(text);

    if (log.childElementCount > 0) {
        log.insertBefore(li, log.children[0]);
    } else {
        log.appendChild(li);
    }

    /* Scroll the log if needed to show the most recent message at the bottom. */
    log.scrollTop = log.scrollHeight - log.clientHeight;
};

alljoyn.onlost = function (name) {
    joinChannelButton.innerHTML = 'Join Channel';
    alljoyn.onname();   /* Refresh list of channels being advertised. */
    displayStatus('Lost name : ' + name);
}

alljoyn.start();
