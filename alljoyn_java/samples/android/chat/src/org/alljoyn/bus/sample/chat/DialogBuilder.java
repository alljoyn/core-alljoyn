/*
 * Copyright AllSeen Alliance. All rights reserved.
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

package org.alljoyn.bus.sample.chat;

import android.app.Activity;
import android.app.Dialog;

import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;

import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import android.util.Log;

import java.util.List;

public class DialogBuilder {
    private static final String TAG = "chat.Dialogs";

    public Dialog createUseJoinDialog(final Activity activity, final ChatApplication application) {
        Log.i(TAG, "createUseJoinDialog()");
        final Dialog dialog = new Dialog(activity);
        dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);
        dialog.setContentView(R.layout.usejoindialog);

        ArrayAdapter<String> channelListAdapter = new ArrayAdapter<String>(activity, android.R.layout.test_list_item);
        final ListView channelList = (ListView)dialog.findViewById(R.id.useJoinChannelList);
        channelList.setAdapter(channelListAdapter);

        List<String> channels = application.getFoundChannels();
        for (String channel : channels) {
            int lastDot = channel.lastIndexOf('.');
            if (lastDot < 0) {
                continue;
            }
            channelListAdapter.add(channel.substring(lastDot + 1));
        }
        channelListAdapter.notifyDataSetChanged();

        channelList.setOnItemClickListener(new ListView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                String name = channelList.getItemAtPosition(position).toString();
                application.useSetChannelName(name);
                application.useJoinChannel();
                /*
                 * Android likes to reuse dialogs for performance reasons.  If
                 * we reuse this one, the list of channels will eventually be
                 * wrong since it can change.  We have to tell the Android
                 * application framework to forget about this dialog completely.
                 */
                activity.removeDialog(UseActivity.DIALOG_JOIN_ID);
            }
        });

        Button cancel = (Button)dialog.findViewById(R.id.useJoinCancel);
        cancel.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                /*
                 * Android likes to reuse dialogs for performance reasons.  If
                 * we reuse this one, the list of channels will eventually be
                 * wrong since it can change.  We have to tell the Android
                 * application framework to forget about this dialog completely.
                 */
                activity.removeDialog(UseActivity.DIALOG_JOIN_ID);
            }
        });

        return dialog;
    }

    public Dialog createUseLeaveDialog(Activity activity, final ChatApplication application) {
        Log.i(TAG, "createUseLeaveDialog()");
        final Dialog dialog = new Dialog(activity);
        dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);
        dialog.setContentView(R.layout.useleavedialog);

        Button yes = (Button)dialog.findViewById(R.id.useLeaveOk);
        yes.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                application.useLeaveChannel();
                application.useSetChannelName("Not set");
                dialog.cancel();
            }
        });

        Button no = (Button)dialog.findViewById(R.id.useLeaveCancel);
        no.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                dialog.cancel();
            }
        });

        return dialog;
    }

    public Dialog createHostNameDialog(Activity activity, final ChatApplication application) {
        Log.i(TAG, "createHostNameDialog()");
        final Dialog dialog = new Dialog(activity);
        dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);
        dialog.setContentView(R.layout.hostnamedialog);

        final EditText channel = (EditText)dialog.findViewById(R.id.hostNameChannel);
        channel.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            public boolean onEditorAction(TextView view, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_NULL && event.getAction() == KeyEvent.ACTION_UP) {
                    String name = view.getText().toString();
                    application.hostSetChannelName(name);
                    application.hostInitChannel();
                    dialog.cancel();
                }
                return true;
            }
        });

        Button okay = (Button)dialog.findViewById(R.id.hostNameOk);
        okay.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                String name = channel.getText().toString();
                application.hostSetChannelName(name);
                application.hostInitChannel();
                dialog.cancel();
            }
        });

        Button cancel = (Button)dialog.findViewById(R.id.hostNameCancel);
        cancel.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                dialog.cancel();
            }
        });

        return dialog;
    }

    public Dialog createHostStartDialog(Activity activity, final ChatApplication application) {
        Log.i(TAG, "createHostStartDialog()");
        final Dialog dialog = new Dialog(activity);
        dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);
        dialog.setContentView(R.layout.hoststartdialog);

        Button yes = (Button)dialog.findViewById(R.id.hostStartOk);
        yes.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                application.hostStartChannel();
                dialog.cancel();
            }
        });

        Button no = (Button)dialog.findViewById(R.id.hostStartCancel);
        no.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                dialog.cancel();
            }
        });

        return dialog;
    }

    public Dialog createHostStopDialog(Activity activity, final ChatApplication application) {
        Log.i(TAG, "createHostStopDialog()");
        final Dialog dialog = new Dialog(activity);
        dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);
        dialog.setContentView(R.layout.hoststopdialog);

        Button yes = (Button)dialog.findViewById(R.id.hostStopOk);
        yes.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                application.hostStopChannel();
                dialog.cancel();
            }
        });

        Button no = (Button)dialog.findViewById(R.id.hostStopCancel);
        no.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                dialog.cancel();
            }
        });

        return dialog;
    }

    public Dialog createAllJoynErrorDialog(Activity activity, final ChatApplication application) {
        Log.i(TAG, "createAllJoynErrorDialog()");
        final Dialog dialog = new Dialog(activity);
        dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);
        dialog.setContentView(R.layout.alljoynerrordialog);

        TextView errorText = (TextView)dialog.findViewById(R.id.errorDescription);
        errorText.setText(application.getErrorString());

        Button yes = (Button)dialog.findViewById(R.id.errorOk);
        yes.setOnClickListener(new View.OnClickListener() {
            public void onClick(View view) {
                dialog.cancel();
            }
        });

        return dialog;
    }
}
