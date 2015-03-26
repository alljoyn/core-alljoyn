/*
 * Copyright AllSeen Alliance. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

package org.alljoyn.bus.samples.observer;

import java.util.Vector;

import android.os.Handler;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.TextView;

/**
 * An Adapter class for representing the observed doors in a ListView.
 */
public class DoorAdapter extends BaseAdapter {

    private static class DoorViewHolder {
        final TextView mName;
        final CheckBox mIsOpen;

        public DoorViewHolder(TextView name, CheckBox isOpen) {
            this.mName = name;
            this.mIsOpen = isOpen;
        }
    }

    /**
     * The list of door info objects. A Vector is used to synchronize access on
     * it between threads.
     */
    private final Vector<DoorAdapterItem> list;
    /** The handler for sending messages to the UI thread. */
    private final Handler mHandler;

    public DoorAdapter(Handler handler) {
        list = new Vector<DoorAdapterItem>();
        mHandler = handler;
    }

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public Object getItem(int position) {
        return list.get(position);
    }

    @Override
    public long getItemId(int position) {
        return System.identityHashCode(list.get(position));
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (convertView == null) {
            convertView = LayoutInflater.from(parent.getContext()).inflate(
                    R.layout.door, parent, false);
            TextView name = (TextView) convertView.findViewById(R.id.mName);
            CheckBox check = (CheckBox) convertView.findViewById(R.id.mIsOpen);
            convertView.setTag(new DoorViewHolder(name, check));
        }
        DoorViewHolder data = (DoorViewHolder) convertView.getTag();
        DoorAdapterItem item = list.get(position);
        data.mName.setText(item.getName());
        data.mIsOpen.setChecked(item.isOpen());
        return convertView;
    }

    /** Add an extra door to the adapter and update the UI accordingly. */
    public void add(DoorAdapterItem door) {
        list.add(door);
        updateUI();
    }

    /** Removes a door from the adapter and update the UI accordingly. */
    public void remove(DoorAdapterItem door) {
        if (list.remove(door)) {
            updateUI();
        }
    }

    /** Updates the UI. Make sure the latest changes are shown on the UI. */
    private void updateUI() {
        mHandler.sendMessage(mHandler.obtainMessage(Client.MESSAGE_UPDATE_UI,
                this));
    }

    /** Show a notification in the UI upon receiving an signal from a door. */
    public void sendSignal(String string) {
        mHandler.sendMessage(mHandler.obtainMessage(
                Client.MESSAGE_INCOMING_EVENT, string));
    }

    /** Update UI after receiving an event from a door. */
    public void propertyUpdate(DoorAdapterItem item) {
        sendSignal(item.getName() + " " + (item.isOpen() ? "opened" : "closed"));
        updateUI();
    }

    /** Sends notification of events from local provided doors. */
    public void sendDoorEvent(String string) {
        mHandler.sendMessage(mHandler.obtainMessage(Client.MESSAGE_DOOR_EVENT,
                string));
    }
}
