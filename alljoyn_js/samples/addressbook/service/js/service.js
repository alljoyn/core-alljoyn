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
