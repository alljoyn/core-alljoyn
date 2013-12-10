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
var $ = function(id) {
    return document.getElementById(id);
};

var log = $('log');

var onKeyPress = function(event) {
    var c = event.which ? event.which : event.keyCode,
    message;

    if (c === 13) {
        message = document.forms['message'];
        client.ping(message.text.value);
        message.reset();
        return false;
    }
};

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

var logLine = function(line) {
    log.appendChild(line);

    /* Scroll the log if needed to show the most recent message at the bottom. */
    log.scrollTop = log.scrollHeight - log.clientHeight;
};

var logPing = function(sender, message) {
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

    logLine(li);
};

var logStatus = function(message) {
    var li,
        timestamp,
        text;

    li = document.createElement('li');
    timestamp = document.createElement('span');
    timestamp.className = 'timestamp';
    timestamp.innerHTML = '[' + new Date().toLocaleTimeString() + '] ';
    li.appendChild(timestamp);
    text = document.createElement('span');
    text.className = 'text';
    text.innerHTML = message;
    li.appendChild(text);

    logLine(li);
};
