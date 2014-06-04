/*
 * Copyright (c) 2009-2011, AllSeen Alliance. All rights reserved.
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
 *
 *  This is a sample code demonstrating how to use AllJoyn messages to pass complex data types.
 *  This will send a String array containing all of the contacts found on the phone.
 *  or the list of phone number(s) and e-mail addresses for a contact based on their name.
 */
package org.alljoyn.bus.samples.contacts_service;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.database.Cursor;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.provider.ContactsContract;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;

/*
 * This is a simple AllJoyn service that will provide access to contact
 * information on the phone.
 */
public class ContactsService extends Activity {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final String TAG = "ContactsService";

    private static final int MESSAGE_ACTION = 1;
    private static final int MESSAGE_POST_TOAST = 2;

    private BusHandler mBusHandler;

    public Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_ACTION: {
                mListViewArrayAdapter.add((String) msg.obj);
                break;
            }
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            default:
                break;
            }
        }
    };

    private ArrayAdapter<String> mListViewArrayAdapter;
    private Menu menu;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mListViewArrayAdapter = new ArrayAdapter<String>(this, android.R.layout.test_list_item);
        ListView lv = (ListView) findViewById(R.id.ListView);
        lv.setAdapter(mListViewArrayAdapter);

        /*
         * Send an empty message to list to make it clear where the start of
         * the list is.
         */
        Message msg = mHandler.obtainMessage(MESSAGE_ACTION, "");
        mHandler.sendMessage(msg);

        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());
        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
    }


    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.mainmenu, menu);
        this.menu = menu;
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case R.id.quit:
            finish();
            return true;
        default:
            return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        mBusHandler.sendEmptyMessage(BusHandler.DISCONNECT);
    }

    /**
     * Local implementation of the AddressBookInterface
     */
    private class AllJoynContactService implements AddressBookInterface, BusObject {

        /*
         * Given a name find the contact information for that name.
         * return the filled in contact container.
         */
        public Contact getContact(String name, int userId) throws BusException {
            Contact contact = new Contact();
            contact.name = name;

            String contactId = Integer.toString(userId);
            if (contactId != null) {
                /*
                 * Get all the contact details of type PHONE for the contact
                 */
                String where = ContactsContract.Data.CONTACT_ID + " = " + contactId + " AND " +
                        ContactsContract.Data.MIMETYPE + " = '" +
                        ContactsContract.CommonDataKinds.Phone.CONTENT_ITEM_TYPE + "'";
                Cursor dataCursor = getContentResolver().query(ContactsContract.Data.CONTENT_URI,
                                                               null, where, null, null);

                int phoneIdx = dataCursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.Phone.NUMBER);
                int typeIdx  = dataCursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.Phone.TYPE);
                int labelIdx = dataCursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.Phone.LABEL);
                int count = dataCursor.getCount();
                if ( count > 0){
                    contact.setPhoneCount(count);
                    if (dataCursor.moveToFirst()) {
                        do {
                            contact.phone[dataCursor.getPosition()].number = dataCursor.getString(phoneIdx);
                            /*
                             * If a number was not returned fill in the value
                             * with an empty string.  AllJoyn will not allow
                             * a null if a string is expected.
                             */
                            if (contact.phone[dataCursor.getPosition()].number == null) {
                                contact.phone[dataCursor.getPosition()].number = "";
                            }
                            contact.phone[dataCursor.getPosition()].type = dataCursor.getInt(typeIdx);

                            contact.phone[dataCursor.getPosition()].label = dataCursor.getString(labelIdx);
                            if (contact.phone[dataCursor.getPosition()].label == null) {
                                contact.phone[dataCursor.getPosition()].label = "";
                            }
                        } while(dataCursor.moveToNext());
                    } else {
                        /*
                         * If for some reason we are unable to move the dataCursor to the first element
                         * fill in all contacts with a default empty value.
                         */
                        for (int i = 0; i < count; i++) {
                            contact.phone[i].number = "";
                            contact.phone[i].type = 0;
                            contact.phone[i].label = "";
                        }
                    }
                } else {
                    /*
                     * We must have at lease one contact.
                     */
                    contact.setPhoneCount(1);
                    contact.phone[0].number = "";
                    contact.phone[0].type = 0;
                    contact.phone[0].label = "";
                }

                dataCursor.close();

                /*
                 * Get all the contact details of type EMAIL for the contact
                 */
                where = ContactsContract.Data.CONTACT_ID + " = " + contactId + " AND " +
                        ContactsContract.Data.MIMETYPE + " = '" +
                        ContactsContract.CommonDataKinds.Email.CONTENT_ITEM_TYPE + "'";
                dataCursor = getContentResolver().query(ContactsContract.Data.CONTENT_URI, null, where, null, null);
                int addressIdx = dataCursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.Email.DATA);
                typeIdx  = dataCursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.Email.TYPE);
                labelIdx = dataCursor.getColumnIndexOrThrow(ContactsContract.CommonDataKinds.Email.LABEL);
                count = dataCursor.getCount();
                if (count > 0) {
                    contact.setEmailCount(count);
                    if (dataCursor.moveToFirst()) {
                        do {
                            contact.email[dataCursor.getPosition()].address = dataCursor.getString(addressIdx);
                            /*
                             * If an email was not returned fill in the value
                             * with an empty string.  AllJoyn will not allow
                             * a null value if a string is expected.
                             */
                            if (contact.email[dataCursor.getPosition()].address == null) {
                                contact.email[dataCursor.getPosition()].address = "";
                            }
                            contact.email[dataCursor.getPosition()].type = dataCursor.getInt(typeIdx);
                            contact.email[dataCursor.getPosition()].label = dataCursor.getString(labelIdx);
                            if (contact.email[dataCursor.getPosition()].label == null) {
                                contact.email[dataCursor.getPosition()].label = "";
                            }
                        } while(dataCursor.moveToNext());
                    } else {
                        for (int i = 0; i < count; i++){
                            contact.email[i].address = "";
                            contact.email[i].type = 0;
                            contact.email[i].label = "";
                        }
                    }
                } else {
                    /*
                     * We must have at lease one email.
                     */
                    contact.setEmailCount(1);
                    contact.email[0].address = "";
                    contact.email[0].type = 0;
                    contact.email[0].label = "";
                }
                dataCursor.close();
            }

            String action = String.format("Information about %s sent to Client", contact.name);
            Message msg = mHandler.obtainMessage(MESSAGE_ACTION, action);
            mHandler.sendMessage(msg);
            Log.i(TAG, action);

            return contact;
        }

        /*
         * Get the DISPLAY NAME of every single user
         */
        public NameId[] getAllContactNames() {
            Log.i(TAG, String.format("Client requested a list of contacts"));
            /*
             * Get a cursor over every aggregated contact.  Sort the data in
             * ascending order based on the display name.
             */
            Cursor cursor = getContentResolver().query(ContactsContract.Contacts.CONTENT_URI, null, null, null,
                                                       ContactsContract.Contacts.DISPLAY_NAME + " ASC");

            /*
             * Let the activity manage the cursor lifecycle.
             */
            startManagingCursor(cursor);

            /*
             * Use the convenience properties to get the index of the columns.
             */
            int nameIdx = cursor.getColumnIndexOrThrow(ContactsContract.Contacts.DISPLAY_NAME);
            int idIdx   = cursor.getColumnIndexOrThrow(ContactsContract.Contacts._ID);

            NameId[] result;
            int count = cursor.getCount();
            if (count > 0) {
                result = new NameId[cursor.getCount()];
                if (cursor.moveToFirst()) {
                    do {
                        result[cursor.getPosition()] = new NameId();
                        result[cursor.getPosition()].displayName = cursor.getString(nameIdx);
                        result[cursor.getPosition()].userId = cursor.getInt(idIdx);
                    } while(cursor.moveToNext());
                }
            } else {
                /*
                 * we must have at lease one value
                 */
                result = new NameId[1];
                result[0] = new NameId();
                result[0].displayName = "";
                result[0].userId = 0;
            }

            stopManagingCursor(cursor);

            String action = String.format("Sending list of %d contacts to Client", count);
            Message msg = mHandler.obtainMessage(MESSAGE_ACTION, action);
            mHandler.sendMessage(msg);

            Log.i(TAG, action);
            return result;
        }
    }

    class BusHandler extends Handler {
        private static final String SERVICE_NAME = "org.alljoyn.bus.addressbook";
        private static final short CONTACT_PORT = 42;

        BusAttachment mBus;

        private AllJoynContactService mService;

        /* These are the messages sent to the BusHandler from the UI. */
        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;

        public BusHandler(Looper looper) {
            super(looper);

            mService = new AllJoynContactService();
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case CONNECT: {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                mBus.registerBusListener(new BusListener());

                Status status = mBus.registerBusObject(mService, "/addressbook");
                logStatus("BusAttachment.registerBusObject()", status);

                status = mBus.connect();
                logStatus("BusAttachment.connect()", status);

                Mutable.ShortValue contactPort = new Mutable.ShortValue(CONTACT_PORT);

                SessionOpts sessionOpts = new SessionOpts();
                sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
                sessionOpts.isMultipoint = false;
                sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
                sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

                status = mBus.bindSessionPort(contactPort, sessionOpts, new SessionPortListener() {
                    @Override
                    public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                        if (sessionPort == CONTACT_PORT) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                });
                logStatus(String.format("BusAttachment.bindSessionPort(%d, %s)",
                                        contactPort.value, sessionOpts.toString()), status);

                if (status != Status.OK) {
                    finish();
                    return;
                }

                status = mBus.requestName(SERVICE_NAME, BusAttachment.ALLJOYN_REQUESTNAME_FLAG_DO_NOT_QUEUE);
                logStatus("BusAttachment.requestName()", status);

                status = mBus.advertiseName(SERVICE_NAME, SessionOpts.TRANSPORT_ANY);
                logStatus(String.format("BusAttachment.advertiseName(%s)", SERVICE_NAME), status);
                break;
            }
            case DISCONNECT: {
                mBus.unregisterBusObject(mService);
                mBus.disconnect();
                mBusHandler.getLooper().quit();
                break;
            }
            default:
                break;

            }
        }
    }

    private void logStatus(String msg, Status status) {
        String log = String.format("%s: %s", msg, status);
        if (status == Status.OK) {
            Log.i(TAG, log);
        } else {
            Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
            mHandler.sendMessage(toastMsg);
            Log.e(TAG, log);
        }
    }
}