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

    var vcard = document.forms['vcard'],
        details = document.getElementsByClassName('details')[0],
        clear = vcard.clear,
        add = vcard.add,
        search = document.forms['search'],
        vcards = $('vcards');

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

    var addPhoneNumber = function() {
        var tel,
            type,
            value,
            addRemove;

        tel = document.createElement('fieldset');
        tel.className = 'tel';
        type = document.createElement('input');
        type.className = 'type';
        type.placeholder = 'Phone number type';
        tel.appendChild(type);
        tel.appendChild(document.createTextNode(':\n'));
        value = document.createElement('input');
        value.className = 'value';
        value.type = 'tel';
        value.placeholder = 'Phone number';
        tel.appendChild(value);
        addRemove = document.createElement('button');
        addRemove.innerHTML = '+';
        addRemove.onclick = addPhoneNumber;
        tel.appendChild(addRemove);

        details.appendChild(tel);
        return false;
    };

    addPhoneNumber();

    vcard.onreset = function() {
        while (details.hasChildNodes()) {
            details.removeChild(details.firstChild);
        }
        addPhoneNumber();
    };

    clear.onclick = function() {
        this.form.reset();
        return false;
    };

    add.onclick = function() {
        var firstName = this.form['given-name'].value,
            lastName = this.form['family-name'].value,
            tels = this.form.getElementsByClassName('tel'),
            i,
            type,
            value,
            contact;

        if (!lastName.length) {
            alert('Please enter a last name');
            return false;
        }

        contact = {
            firstName: firstName,
            lastName: lastName,
            phoneNumbers: {}
        };

        for (i = 0; i < tels.length; ++i) {
            type = tels[i].getElementsByClassName('type')[0].value;
            value = tels[i].getElementsByClassName('value')[0].value;
            if (type.length && value.length) {
                contact.phoneNumbers[type] = value;
            }
        }

        alljoyn.add(contact);
        this.form.reset();
        return false;
    };

    search.onsubmit = function() {
        var lastNames = this.lastNames.value.split(/\s*,\s*/),
            onGet,
            onError;

        onGet = function(contacts) {
            var i;

            while (vcards.hasChildNodes()) {
                vcards.removeChild(vcards.firstChild);
            }
            for (i = 0; i < contacts.length; ++i) {
                li = createElement('li');
                vcard = toVCard(contacts[i]);
                li.appendChild(vcard);
                vcards.appendChild(li);
            }
        };
        onError = function(error) {
            alert('Get contacts failed ' + error);
        };
        alljoyn.get(onGet, lastNames);
        this.reset();
        return false;
    };

    alljoyn.start();
};

if (window.cordova) {
    document.addEventListener('deviceready', onDeviceReady, false);
} else {
    onDeviceReady();
}
