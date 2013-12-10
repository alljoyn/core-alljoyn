
$ = function (id) {
    return document.getElementById(id);
}
var video = $('video'),
    playlist = $('useSources');

var onStart = function (guid) {
    $('guid').innerHTML = guid;
};

var onLoad = function (url) {
    video.src = url;
    video.load();
   // video.controls = true;
};

var onPlay = function () {
    video.play();
};

var onPause = function () {
    video.pause();
};

var onPlaylistChanged = function () {
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
        span.innerHTML = sink.playlist[i].fname;
        li.appendChild(span);

        playlist.appendChild(li);
    }

    if (sink.nowPlaying === -1) {
        video.pause();
    }
};

sink.start(onStart, onLoad, onPlay, onPause, onPlaylistChanged);
