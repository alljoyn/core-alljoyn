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

    var contacts,
        vcards = $('vcards'),
        id;

    var createElement = function(name, className, innerHTML) {
        var element = document.createElement(name);
        if (className) {
            element.className = className;
        }
        if (innerHTML) {
            element.innerHTML = innerHTML;
        }
        return element;
    };

    var toVCard = function(contact) {
        var vcard,
            n,
            details,
            type,
            tel;

        vcard = createElement('div', 'vcard');

        n = createElement('div', 'index n');
        n.appendChild(createElement('span', 'family-name', contact.lastName));
        n.appendChild(createElement('span', 'given-name', contact.firstName));
        vcard.appendChild(n);

        details = createElement('div', 'details');
        for (type in contact.phoneNumbers) {
            tel = createElement('div', 'tel');
            tel.appendChild(createElement('span', 'type', type));
            tel.appendChild(createElement('span', 'value', contact.phoneNumbers[type]));
            details.appendChild(tel);
        }
        vcard.appendChild(details);

        return vcard;
    };

    contacts = {
        set: function(contact) {
            var vcard,
                li;

            addressbook[contact.lastName] = contact;

            vcard = toVCard(contact);
            li = $(contact.lastName);
            if (li) {
                li.replaceChild(vcard, li.firstChild);
            } else {
                li = createElement('li');
                li.id = contact.lastName;
                li.appendChild(vcard);
                vcards.appendChild(li);
            }
        },
        get: function(lastName) {
            return addressbook[lastName];
        },
    };

    for (id in addressbook) {
        contacts.set(addressbook[id]);
    }

    alljoyn.start(contacts);
};

if (window.cordova) {
    document.addEventListener('deviceready', onDeviceReady, false);
} else {
    onDeviceReady();
}