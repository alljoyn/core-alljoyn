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
var $ = function (id) {
    return document.getElementById(id);
};

var sources = {},
    sinks = {};

var sourceFiles = {}

var streamList = new WinJS.Binding.List([]);
var hostSourceList = new WinJS.Binding.List([]);

var onStart = function (guid) {
    $('guid').innerHTML = guid;
};

// Custom templating function
var StreamItemTemplate = WinJS.Utilities.markSupportedForProcessing(function StreamItemTemplate(itemPromise) {
    return itemPromise.then(function (currentItem) {
        var result = document.createElement("div");
        result.className = "streamListTemplate";
        result.style.overflow = "hidden";
        var image = document.createElement("img");
        image.id = "streamListItemPic";
        image.src = currentItem.data.thumbnail;
        result.appendChild(image);

        var title = document.createElement("h3");
        title.id = "streamListItemTitle";
        title.innerText = currentItem.data.title;
        result.appendChild(title);

        var button = document.createElement("button");
        button.id = "streamListItemPushBtn";
        button.innerText = "PUSH";
        button.addEventListener('click', function () {
            var d = currentItem.data.data;
            onPush(d);
        });
        result.appendChild(button);
        return result;
    });
});


var onSourceChanged = function (props) {
    var li,
        button,
        span,
        i;

    while (streamList.length > 0) {
        streamList.splice(0, 1);
    }

    for (i = 0; i < props.length; ++i) {
        streamList.push(
            { title: props[i][0], thumbnail: "images/video.png", data: props[i] });
    }
};

document.forms['browseSource'].sourceNames.onchange = function () {
    var name = document.forms['browseSource'].sourceNames.value;

    browser.leaveSource();
    if (name !== 'Searching...') {
        browser.joinSource(onSourceChanged, 'trm.streaming.source-' + name);
    }
};

var onSource = function () {
    var sourceNames = document.forms['browseSource'].sourceNames,
        sourceName = sourceNames.value,
        i,
        n,
        option;

    while (sourceNames.hasChildNodes()) {
        sourceNames.removeChild(sourceNames.firstChild);
    }
    n = 0;
    for (i in sources) {
        option = document.createElement('option');
        option.innerHTML = i.replace('trm.streaming.source-', '');
        sourceNames.appendChild(option);
        ++n;
    }
    if (n > 0) {
        sourceNames.disabled = false;
    } else {
        option = document.createElement('option');
        option.innerHTML = 'Searching...';
        sourceNames.appendChild(option);
        sourceNames.disabled = true;
    }
    if (sourceName !== sourceNames.value) {
        sourceNames.onchange();
    }
};

var onFoundSource = function (name) {
    sources[name] = {};
    onSource();
};

var onLostSource = function (name) {
    delete sources[name];
    onSource();
};

var onPush = function (stream) {
    var fname = stream[0],
        name = document.forms['browseSource'].sourceNames.value,
        port = stream[1],
        sink = document.forms['browseSink'].sinkNames.value;

    browser.push(fname, 'trm.streaming.source-' + name, port, 'trm.streaming.sink-' + sink);
    return false;
};

var onSinkChanged = function (props) {
    var li,
        button,
        span,
        i;

    while ($('useSinks').hasChildNodes()) {
        $('useSinks').removeChild($('useSinks').firstChild);
    }

    var nowPlayingIndex = -1;
    var playlist = null;
    for (i = 0; i < props.length; i++) {
        if (props[i].key == "NowPlaying") {
            nowPlayingIndex = props[i].value.value;
        }
    }

    var enableCtrl = false;
    for (i = 0; i < props.length; i++) {
        if (props[i].key == "Playlist") {
            for (j = 0; props[i].value && props[i].value.value && (j < props[i].value.value.length) ; ++j) {
                enableCtrl = true;
                li = document.createElement('li');
                li.className = 'use';

                span = document.createElement('span');
                if (j === nowPlayingIndex) {
                    span.className = 'nowplaying';
                }
                span.innerHTML = props[i].value.value[j].value;
                li.appendChild(span);

                $('useSinks').appendChild(li);
            }
        }
    }
    if (enableCtrl > 0) {
        $('play').disabled = false;
        $('prev').disabled = false;
        $('next').disabled = false;
    }
};

document.forms['browseSink'].sinkNames.onchange = function () {
    var name = document.forms['browseSink'].sinkNames.value;

    browser.leaveSink();
    if (name !== 'Searching...') {
        browser.joinSink(onSinkChanged, 'trm.streaming.sink-' + name);
    }
};

var onSink = function (name) {
    var sinkNames = document.forms['browseSink'].sinkNames,
        sinkName = sinkNames.value,
        i,
        n,
        option;

    while (sinkNames.hasChildNodes()) {
        sinkNames.removeChild(sinkNames.firstChild);
    }
    n = 0;
    for (i in sinks) {
        option = document.createElement('option');
        option.innerHTML = i.replace('trm.streaming.sink-', '');
        sinkNames.appendChild(option);
        ++n;
    }
    if (n > 0) {
        sinkNames.disabled = false;
    } else {
        option = document.createElement('option');
        option.innerHTML = 'Searching...';
        sinkNames.appendChild(option);
        sinkNames.disabled = true;
    }
    if (sinkName !== sinkNames.value) {
        sinkNames.onchange();
    }
};

var onFoundSink = function (name) {
    sinks[name] = {};
    onSink();
};

var onLostSink = function (name) {
    delete sinks[name];
    onSink();
};

$('play').onclick = function () {
    var sink = document.forms['browseSink'].sinkNames.value;

    if ($('play').innerHTML === '&gt;') {
        browser.play('trm.streaming.sink-' + sink);
        $('play').innerHTML = '||';
    } else {
        browser.pause('trm.streaming.sink-' + sink);
        $('play').innerHTML = '&gt;';
    }
    return false;
};

$('prev').onclick = function () {
    var sink = document.forms['browseSink'].sinkNames.value;

    browser.previous('trm.streaming.sink-' + sink);
    $('play').innerHTML = '&gt;';
    return false;
};
$('next').onclick = function () {
    var sink = document.forms['browseSink'].sinkNames.value;

    browser.next('trm.streaming.sink-' + sink);
    $('play').innerHTML = '&gt;';
    return false;
};

$('add').onclick = function () {
    var fileOpenPicker = new Windows.Storage.Pickers.FileOpenPicker();
    //fileOpenPicker.viewMode = Windows.Storage.Pickers.PickerViewMode.thumbnail;
    fileOpenPicker.suggestedStartLocation = Windows.Storage.Pickers.PickerLocationId.videoLibrary;
    fileOpenPicker.fileTypeFilter.append(".mp4");
    fileOpenPicker.pickSingleFileAsync().then(function (file) {
        if (file) {
            addSource(file);
            // Application now has read/write access to the picked file
            WinJS.log && WinJS.log("Picked song: " + file.name, "MediaServer", "status");
        } else {
            // The picker was dismissed with no selected file
            WinJS.log && WinJS.log("Operation cancelled.", "MediaServer", "status");
        }
    });
}

var addSource = function (file) {
    source.addSource(file);
    sourceFiles[file.name] = file;
    file.getThumbnailAsync(Windows.Storage.FileProperties.ThumbnailMode.videosView,
        30,
        Windows.Storage.FileProperties.ThumbnailOptions.useCurrentScale
        ).then(function (storageItemThumbnail) {
            var uri = URL.createObjectURL(storageItemThumbnail, { oneTimeOnly: true });
            hostSourceList.push({ "title": file.name, "thumbnail": uri });
        });

};

// Custom templating function
var HostSourceItemTemplate = WinJS.Utilities.markSupportedForProcessing(function StreamItemTemplate(itemPromise) {
    return itemPromise.then(function (currentItem) {
        var result = document.createElement("div");
        result.className = "hostSourceItemTemplate";
        result.style.overflow = "hidden";
        var image = document.createElement("img");
        image.id = "hostSourceItemPic";
        image.src = currentItem.data.thumbnail;
        result.appendChild(image);

        var title = document.createElement("h3");
        title.id = "hostSourceItemTitle";
        title.innerText = currentItem.data.title;
        result.appendChild(title);

        var button = document.createElement("button");
        button.id = "hostSourceRemoveBtn";
        button.innerText = "REMOVE";
        button.addEventListener('click', function () {
            var index = currentItem.index;
            source.removeSource(currentItem.data.title);
            hostSourceList.splice(index, 1);

        });
        result.appendChild(button);
        return result;
    });
});


source.start(onStart);
browser.start(onFoundSource, onLostSource, onFoundSink, onLostSink);
