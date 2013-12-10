/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
    var $ = function(id) {
        return document.getElementById(id);
    };

    var video = $('video'),
        playlist = $('useSources');

    var onStart = function(guid) {
        $('guid').innerHTML = guid;
    };

    var onLoad = function(url) {
        video.src = url;
        video.load();
    };

    var onPlay = function() {
        video.play();
    };

    var onPause = function() {
        video.pause();
    };

    var onPlaylistChanged = function() {
        var i,
            li;

        while (playlist.hasChildNodes()) {
            playlist.removeChild(playlist.lastChild);
        }

        for (i = 0; i < sink.playlist.length; ++i) {
            li = document.createElement('li');
            li.className = 'use';

            span = document.createElement('span');
            if (i === sink.nowPlaying) {
                span.className = 'nowplaying';
            }
            span.innerHTML = sink.playlist[i];
            li.appendChild(span);

            playlist.appendChild(li);
        }

        if (sink.nowPlaying === -1) {
            video.pause();
        }
    };

    navigator.requestPermission('org.alljoyn.bus', function() {
        sink.start(onStart, onLoad, onPlay, onPause, onPlaylistChanged);
    });
};

if (window.cordova) {
    document.addEventListener('deviceready', onDeviceReady, false);
} else {
    onDeviceReady();
}
