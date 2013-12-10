/*
 * Copyright (c) 2010-2012, AllSeen Alliance. All rights reserved.
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
package org.alljoyn.bus.bundle.tests.clientbundle;

import android.app.Activity;
import android.app.Dialog;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class DialogBuilder {

	private static final String TAG = "client.Dialogs";

	private TextView mFailReason;

    public Dialog createDebugDialog(final Activity activity, final String reason)
    {

    	Log.i(TAG, "createDebugDialog()");
    	final Dialog dialog = new Dialog(activity);
    	dialog.requestWindowFeature(dialog.getWindow().FEATURE_NO_TITLE);

    	dialog.setContentView(R.layout.debug);

    	mFailReason = (TextView) dialog.findViewById(R.id.failReason);
    	mFailReason.setText(reason);

    	Button cancel = (Button)dialog.findViewById(R.id.clientCancel);
    	cancel.setOnClickListener(new View.OnClickListener() {
    		public void onClick(View view) {
				/*
				 * Android likes to reuse dialogs for performance reasons.
				 * Tell the Android
				 * application framework to forget about this dialog completely.
				 */
    			activity.removeDialog(Client.DIALOG_DEBUG_ID);
    		}
    	});

    	return dialog;
    }
}
