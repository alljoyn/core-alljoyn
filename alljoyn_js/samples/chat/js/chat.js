/*
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
var onDeviceReady = function() {
    $ = function(id) {
        return document.getElementById(id);
    }

    var joinChannelButton = $('joinChannelButton'),
        log = $('log'),
        startChannelButton = $('startChannelButton'),
        tabHost = $('tabHost'),
        tabHostButton = $('tabHostButton'),
        tabUse = $('tabUse'),
        tabUseButton = $('tabUseButton');

    /*
     * The chat log fills the remainder of the vertical height.
     */
    var resizeLog = function() {
        var innerHeight = window.innerHeight,
            offsetHeight = log.offsetParent.offsetHeight,
            scrollHeight = log.offsetParent.scrollHeight,
            delta = scrollHeight - offsetHeight;
        log.style.maxHeight = innerHeight - log.offsetTop - delta;
    };
    window.onload = resizeLog;
    window.onresize = resizeLog;

    tabUseButton.onclick = function() {
        tabHostButton.className = 'disabled';
        tabHost.style.display = 'none';
        tabUseButton.className = 'enabled';
        tabUse.style.display = 'block';
        return false;
    };

    tabHostButton.onclick = function() {
        tabUseButton.className = 'disabled';
        tabUse.style.display = 'none';
        tabHostButton.className = 'enabled';
        tabHost.style.display = 'block';
        return false;
    };

    joinChannelButton.onclick = function() {
        var channelNames = document.forms['joinChannel'].channelNames,
            channelName = channelNames.value,
            onJoined,
            onError,
            err;

        if (joinChannelButton.innerHTML === 'Join') {
            onJoined = function() {
                while (log.hasChildNodes()) {
                    log.removeChild(log.firstChild);
                }
                channelNames.disabled = true;
                joinChannelButton.innerHTML = 'Leave';
                joinChannelButton.disabled = false;
            };
            onError = function() {
                joinChannelButton.disabled = false;
            };
            alljoyn.joinChannel(onJoined, onError, channelName);
            joinChannelButton.disabled = true;
        } else {
            alljoyn.leaveChannel();
            joinChannelButton.innerHTML = 'Join';
            alljoyn.onname();   /* Refresh list of channels being advertised. */
        }
        return false;
    };

    startChannelButton.onclick = function() {
        var channelName = document.forms['setChannel'].channelName,
            err;

        if (startChannelButton.innerHTML === 'Start') {
            alljoyn.startChannel(channelName.value);
            startChannelButton.innerHTML = 'Stop';
            channelName.disabled = true;
        } else {
            alljoyn.stopChannel();
            startChannelButton.innerHTML = 'Start';
            channelName.disabled = false;
        }
        return false;
    };

    alljoyn.onname = function() {
        var channelNames = document.forms['joinChannel'].channelNames,
            i,
            option;

        /*
         * Don't touch the channel list while we are joined.  The session can be "stopped" by the host,
         * but it is still alive for any joined parties.
         */
        if (joinChannelButton.innerHTML === 'Leave') {
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
    alljoyn.onchat = function(sender, message) {
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
        name.className = 'sender';
        name.innerHTML = '(' + sender + ') ';
        li.appendChild(name);
        text = document.createElement('span');
        text.className = 'text';
        text.innerHTML = message;
        li.appendChild(text);

        log.appendChild(li);

        /* Scroll the log if needed to show the most recent message at the bottom. */
        log.scrollTop = log.scrollHeight - log.clientHeight;
    };
    alljoyn.onlost = function(name) {
        joinChannelButton.innerHTML = 'Join';
        alljoyn.onname();   /* Refresh list of channels being advertised. */
    }

    /* The initial state of the page. */
    tabUseButton.onclick();

    alljoyn.start();
};

var onKeyPress = function(event) {
    var c = event.which ? event.which : event.keyCode,
        message;

    if (c === 13) {
        message = document.forms['message'];
        alljoyn.chat(message.text.value);
        message.reset();
        return false;
    }
};

if (window.cordova) {
    document.addEventListener('deviceready', onDeviceReady, false);
} else {
    onDeviceReady();
}
