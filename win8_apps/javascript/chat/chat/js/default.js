// For an introduction to the Fixed Layout template, see the following documentation:
// http://go.microsoft.com/fwlink/?LinkId=232508
(function () {
    "use strict";

    var app = WinJS.Application;

    // This function responds to all application activations.
    app.onactivated = function (eventObject) {
        if (eventObject.detail.kind === Windows.ApplicationModel.Activation.ActivationKind.launch) {
            // TODO: Initialize your application here.
            WinJS.UI.processAll();
        }
    };

    app.start();
})();
