/*
 * Copyright (c) 2009-2011,2013, AllSeen Alliance. All rights reserved.
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
 *  This will get a String array containing all of the contact found on the phone.
 *  or the list of phone number(s) and e-mail addresses for a contact based on their name.
 */
package org.alljoyn.bus.samples.contacts_client;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.ProxyBusObject;
import org.alljoyn.bus.SessionListener;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.Status;

import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.res.Resources;
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
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.Toast;

/*
 * This is a sample client that will receive information about the contacts stored on
 * the phone from a contacts service.
 */
public class ContactsClient extends Activity {
    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final int DIALOG_CONTACT = 1;

    private static final int MESSAGE_DISPLAY_ALL_CONTACTS = 1;
    private static final int MESSAGE_DISPLAY_CONTACT = 2;
    private static final int MESSAGE_POST_TOAST = 3;
    private static final int MESSAGE_START_PROGRESS_DIALOG = 4;
    private static final int MESSAGE_STOP_PROGRESS_DIALOG = 5;

    private static final String TAG = "ContactsClient";

    private Button mGetContactsBtn;
    private ArrayAdapter<String> mContactsListAdapter;
    private ListView mContactsListView;
    private Menu menu;

    private Contact mAddressEntry;
    private NameId[] mContactNames;

    String mSingleName;
    int mSingleUserId;

    BusHandler mBusHandler;
    private ProgressDialog mDialog;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case MESSAGE_DISPLAY_ALL_CONTACTS:
                mContactNames = (NameId[]) msg.obj;
                /*
                 * Make sure the Contacts list is clear of any old information before filling the list.
                 */
                mContactsListAdapter.clear();

                /*
                 * Change the name of the button from "Get Contacts List" to "Update Contacts List"
                 */
                mGetContactsBtn.setText(getString(R.string.update_contacts));
                for (int i = 0; i < mContactNames.length; i++) {
                    mContactsListAdapter.add(mContactNames[i].displayName);
                }
                break;
            case MESSAGE_DISPLAY_CONTACT:
                mAddressEntry = (Contact) msg.obj;
                showDialog(DIALOG_CONTACT);
                break;
            case MESSAGE_POST_TOAST:
                Toast.makeText(getApplicationContext(), (String) msg.obj, Toast.LENGTH_LONG).show();
                break;
            case MESSAGE_START_PROGRESS_DIALOG:
                mDialog = ProgressDialog.show(ContactsClient.this,
                                              "",
                                              "Finding Contacts Service.\nPlease wait...",
                                              true,
                                              true);
                break;
            case MESSAGE_STOP_PROGRESS_DIALOG:
                mDialog.dismiss();
                break;
            default:
                break;
            }
        }
    };

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mContactsListAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1);
        mContactsListView = (ListView) findViewById(R.id.contact_list);
        mContactsListView.setAdapter(mContactsListAdapter);
        mContactsListView.setTextFilterEnabled(true);
        mContactsListView.setOnItemClickListener(new GetContactInformation());

        mGetContactsBtn = (Button) findViewById(R.id.get_contacts_btn);
        mGetContactsBtn.setOnClickListener(new GetContactsListener());

        mAddressEntry = new Contact();

        HandlerThread busThread = new HandlerThread("BusHandler");
        busThread.start();
        mBusHandler = new BusHandler(busThread.getLooper());
        mBusHandler.sendEmptyMessage(BusHandler.CONNECT);
        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
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
     * Implementation of the OnClickListener attached to the "Get Contacts List"
     * "Update Contacts List"  button.  When clicked this will fill the
     * mcontactsListAdapter with an alphabetized list of all the contacts on
     * the phone.
     */
    private class GetContactsListener implements View.OnClickListener {
        public void onClick(View v) {
            mBusHandler.sendEmptyMessage(BusHandler.GET_ALL_CONTACT_NAMES);
        }
    }

    /**
     * Implementation of the OnItemClickListener for any item in the contacts
     * list.  The listener will use the the string from the list and use that
     * name to lookup an individual contact based on that name.
     */
    private class GetContactInformation implements AdapterView.OnItemClickListener {
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            Message msg = mBusHandler.obtainMessage(BusHandler.GET_CONTACT, mContactNames[position]);
            mBusHandler.sendMessage(msg);
        }
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        Dialog dialog = null;
        try {
            switch (id) {
            case DIALOG_CONTACT: {
                dialog = new Dialog(ContactsClient.this);
                break;
            }
            default: {
                dialog = null;
                break;
            }
            }
        } catch (Throwable ex ) {
            Log.e(TAG, String.format("Throwable exception found"), ex);
        }
        return dialog;
    }

    @Override
    protected void onPrepareDialog(int id, Dialog dialog) {
        switch (id) {
        /*
         * build a dialog that show the contents of the an individual contact
         * This will dynamically build the dialog based on how much information
         * is known about the contact.
         */
        case DIALOG_CONTACT: {
            /*
             * Individual dialog elements
             */
            int PHONE_TABLE_OFFSET = 2; /* two columns before the phone numbers start */
            int EMAIL_TABLE_OFFSET = 1 + PHONE_TABLE_OFFSET + mAddressEntry.phone.length;

            /*
             * Reset the dialog to a known starting point
             */
            dialog.setContentView(R.layout.contact);
            dialog.setTitle(getString(R.string.contact_dialog_title));

            /*
             * Add the contact Name to the top of the table
             */
            TextView contactName = (TextView) dialog.findViewById(R.id.contact_name);
            contactName.setText(mAddressEntry.name);

            /*
             * Get the table layout so items can be added to it.
             */
            TableLayout contactTable = (TableLayout) dialog.findViewById(R.id.contact_table);

            /*
             * Add a phone number entry to the dialog displayed on screen for each phone number.
             */
            if (mAddressEntry.phone.length > 0) {
                for (int i = 0; i < mAddressEntry.phone.length; i++) {
                    insertPhoneToTable(contactTable, mAddressEntry.phone[i], i + PHONE_TABLE_OFFSET);
                }
            }

            /*
             * Add an email number entry to the dialog displayed on screen for each email address.
             */
            if (mAddressEntry.email.length > 0) {
                for (int i = 0; i < mAddressEntry.email.length; i++) {
                    insertEmailToTable(contactTable, mAddressEntry.email[i], i + EMAIL_TABLE_OFFSET);
                }
            }
            break;
        }
        default:
            break;
        }
    }

    /**
     * Insert a phone number into the table at the indicated position
     */
    private void insertPhoneToTable(TableLayout table, Contact.Phone phone, int position) {
        TableRow tr = new TableRow(getApplicationContext());
        TextView type = new TextView(getApplicationContext());
        type.setLayoutParams(new TableRow.LayoutParams(1));
        /*
         * If the phone type has a custom label use that label other wise pull
         * the type from the phone_types string array.
         */
        if (phone.type == ContactsContract.CommonDataKinds.Phone.TYPE_CUSTOM) {
            type.setText(phone.label);
        } else {
            Resources res = getResources();
            String[] phoneTypes = res.getStringArray(R.array.phone_types);
            type.setText(phoneTypes[phone.type]);
        }

        TextView number = new TextView(getApplicationContext());
        number.setText(phone.number);

        tr.addView(type);
        tr.addView(number);
        table.addView(tr, position);
    }

    /*
     * Insert an email address into the table at the indicated position
     */
    private void insertEmailToTable(TableLayout table, Contact.Email email, int position) {
        TableRow tr = new TableRow(getApplicationContext());
        TextView type = new TextView(getApplicationContext());
        type.setLayoutParams(new TableRow.LayoutParams(1));
        /*
         * if the email type has a custom label use that label other wise pull
         * the type from the email_types string array.
         */
        if (email.type == ContactsContract.CommonDataKinds.Email.TYPE_CUSTOM) {
            type.setText(email.label);
        } else {
            Resources res = getResources();
            String[] emailTypes = res.getStringArray(R.array.email_types);
            type.setText(emailTypes[email.type]);
        }

        TextView address = new TextView(getApplicationContext());
        address.setText(email.address);

        tr.addView(type);
        tr.addView(address);
        table.addView(tr, position);
    }

    /*
     * See the SimpleClient sample for a more complete description of the code used
     * to connect this code to the Bus
     */
    class BusHandler extends Handler {
        private static final String SERVICE_NAME = "org.alljoyn.bus.addressbook";
        private static final short CONTACT_PORT = 42;

        public static final int CONNECT = 1;
        public static final int DISCONNECT = 2;
        public static final int GET_CONTACT = 3;
        public static final int GET_ALL_CONTACT_NAMES = 4;
        public static final int JOIN_SESSION = 5;

        private BusAttachment mBus;
        private ProxyBusObject mProxyObj;
        private AddressBookInterface mAddressBookInterface;

        private int     mSessionId;
        private boolean mIsConnected;
        private boolean mIsStoppingDiscovery;

        public BusHandler(Looper looper) {
            super(looper);
            mIsConnected = false;
            mIsStoppingDiscovery = false;
        }

        @Override
        public void handleMessage(Message msg) {
            switch(msg.what) {
            case CONNECT: {
                org.alljoyn.bus.alljoyn.DaemonInit.PrepareDaemon(getApplicationContext());
                mBus = new BusAttachment(getPackageName(), BusAttachment.RemoteMessage.Receive);

                mBus.registerBusListener(new BusListener() {
                    @Override
                    public void foundAdvertisedName(String name, short transport, String namePrefix) {
                        logInfo(String.format("MyBusListener.foundAdvertisedName(%s, 0x%04x, %s)", name, transport, namePrefix));
                        /*
                         * This client will only join the first service that it sees advertising
                         * the indicated well-known name.  If the program is already a member of
                         * a session (i.e. connected to a service) we will not attempt to join
                         * another session.
                         * It is possible to join multiple session however joining multiple
                         * sessions is not shown in this sample.
                         */
                        if (! mIsConnected){
                            Message msg = obtainMessage(JOIN_SESSION, name);
                            sendMessage(msg);
                        }
                    }
                });

                Status status = mBus.connect();
                logStatus("BusAttachment.connect()", status);

                status = mBus.findAdvertisedName(SERVICE_NAME);
                logStatus(String.format("BusAttachement.findAdvertisedName(%s)", SERVICE_NAME), status);
                break;
            }
            case JOIN_SESSION: {
                if (mIsStoppingDiscovery) {
                    break;
                }

                short contactPort = CONTACT_PORT;
                SessionOpts sessionOpts = new SessionOpts();
                Mutable.IntegerValue sessionId = new Mutable.IntegerValue();

                Status status = mBus.joinSession((String) msg.obj, contactPort, sessionId, sessionOpts, new SessionListener(){
                    @Override
                    public void sessionLost(int sessionId, int reason) {
                        mIsConnected = false;
                        logInfo(String.format("MyBusListener.sessionLost(sessionId = %d, reason = %d)", sessionId, reason));
                        mHandler.sendEmptyMessage(MESSAGE_START_PROGRESS_DIALOG);
                    }
                });
                logStatus("BusAttachment.joinSession()", status);

                if (status == Status.OK) {
                    mProxyObj = mBus.getProxyBusObject(SERVICE_NAME, "/addressbook", sessionId.value,
                                                       new Class[] { AddressBookInterface.class });
                    mAddressBookInterface = mProxyObj.getInterface(AddressBookInterface.class);
                    mSessionId = sessionId.value;
                    mIsConnected = true;
                    mHandler.sendEmptyMessage(MESSAGE_STOP_PROGRESS_DIALOG);
                }
                break;
            }

            case DISCONNECT: {
                mIsStoppingDiscovery = true;
                if (mIsConnected) {
                    Status status = mBus.leaveSession(mSessionId);
                    logStatus("BusAttachment.leaveSession()", status);
                }
                mBus.disconnect();
                getLooper().quit();
                break;


            }
            // Call AddressBookInterface.getContact method and send the result to the UI handler.
            case GET_CONTACT: {
                if (mAddressBookInterface == null) {
                    break;
                }
                try {
                    NameId nameId = (NameId)msg.obj;
                    Contact reply = mAddressBookInterface.getContact(nameId.displayName, nameId.userId);
                    Message replyMsg = mHandler.obtainMessage(MESSAGE_DISPLAY_CONTACT, reply);
                    mHandler.sendMessage(replyMsg);
                } catch (BusException ex) {
                    logException("AddressBookInterface.getContact()", ex);
                }
                break;
            }
            // Call AddressBookInterface.getAllContactNames and send the result to the UI handler
            case GET_ALL_CONTACT_NAMES: {
                try {
                    NameId[] reply = mAddressBookInterface.getAllContactNames();
                    Message replyMsg = mHandler.obtainMessage(MESSAGE_DISPLAY_ALL_CONTACTS, (Object) reply);
                    mHandler.sendMessage(replyMsg);
                } catch (BusException ex) {
                    logException("AddressBookInterface.getAllContactNames()", ex);
                }
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

    private void logException(String msg, BusException ex) {
        String log = String.format("%s: %s", msg, ex);
        Message toastMsg = mHandler.obtainMessage(MESSAGE_POST_TOAST, log);
        mHandler.sendMessage(toastMsg);
        Log.e(TAG, log, ex);
    }

    private void logInfo(String msg) {
        Log.i(TAG, msg);
    }
}
