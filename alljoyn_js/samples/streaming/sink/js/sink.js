/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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