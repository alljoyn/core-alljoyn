/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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
 ******************************************************************************/

package org.alljoyn.about.test;

import java.util.Timer;
import java.util.TimerTask;

import org.alljoyn.about.test.AboutApplication.IconDetails;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ImageView;
import android.widget.TextView;

/**
 * The IconActivity displays all the icon data of the
 * device (the icon version, icon mimetype, icon url, icon size and of course the icon itself.).
 */
public class IconActivity extends Activity{

    //General
    protected static final String TAG = AboutApplication.TAG;
    private AboutApplication m_application;
    private BroadcastReceiver m_receiver;
    private SoftAPDetails m_device;
    private Timer m_timer;
    private int m_tasksToPerform = 0;
    private ProgressDialog m_loadingPopup;

    //Current Network
    private TextView m_currentNetwork;

    //Icon version
    private TextView m_iconVersion;

    //Icon data
    private TextView m_iconMimeType;
    private TextView m_iconSize;
    private TextView m_iconUrl;
    private ImageView m_iconContent;


    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onCreate(android.os.Bundle)
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        //General
        setContentView(R.layout.icon_layout);
        String deviceId = getIntent().getStringExtra(Keys.Extras.EXTRA_DEVICE_ID);
        m_application = (AboutApplication)getApplication();
        m_device = m_application.getDevice(deviceId);
        if(m_device == null){
            closeScreen();
            return;
        }
        startIconSession();

        m_loadingPopup = new ProgressDialog(this);

        //Current Network
        m_currentNetwork = (TextView) findViewById(R.id.current_network_name);
        String ssid = m_application.getCurrentSSID();
        m_currentNetwork.setText(getString(R.string.current_network, ssid));

        //Icon version
        m_iconVersion = (TextView)findViewById(R.id.icon_version_value);

        //Icon data
        m_iconMimeType = (TextView)findViewById(R.id.icon_mime_type_value);
        m_iconSize = (TextView)findViewById(R.id.icon_size_value);
        m_iconUrl = (TextView)findViewById(R.id.icon_url_value);
        m_iconContent = (ImageView)findViewById(R.id.icon_content);

        m_receiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {

                if(Keys.Actions.ACTION_ERROR.equals(intent.getAction())){
                    String error = intent.getStringExtra(Keys.Extras.EXTRA_ERROR);
                    m_application.showAlert(IconActivity.this, error);
                }
                else if(Keys.Actions.ACTION_CONNECTED_TO_NETWORK.equals(intent.getAction())){

                    String ssid = intent.getStringExtra(Keys.Extras.EXTRA_NETWORK_SSID);
                    m_currentNetwork.setText(getString(R.string.current_network, ssid));
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(Keys.Actions.ACTION_ERROR);
        filter.addAction(Keys.Actions.ACTION_CONNECTED_TO_NETWORK);
        registerReceiver(m_receiver, filter);

        m_tasksToPerform = 2;
        getVersion();
        getIconDetails();

    }
    //====================================================================

    private void startIconSession() {

        final AsyncTask<Void, Void, Void> task = new AsyncTask<Void, Void, Void>(){

            @Override
            protected void onPreExecute() {
                Log.d(TAG, "startSession: onPreExecute");
            }

            @Override
            protected Void doInBackground(Void... params) {
                m_application.startIconSession(m_device);
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                Log.d(TAG, "startSession: onPostExecute");
            }
        };
        task.execute();
    }
    //====================================================================

    // Gets the device icon version and put it on the screen
    private void getVersion() {
        final AsyncTask<Void, Void, Short> task = new AsyncTask<Void, Void, Short>(){

            @Override
            protected void onPreExecute() {
                Log.d(TAG, "setIconVersion: onPreExecute");
                showLoadingPopup("getting icon version");
            }

            @Override
            protected Short doInBackground(Void... params){
                return m_application.getIconVersion();
            }

            @Override
            protected void onPostExecute(Short result){
                short version = result.shortValue();
                m_iconVersion.setText(String.valueOf(version));
                Log.d(TAG, "setIconVersion: onPostExecute");
                m_tasksToPerform--;
                dismissLoadingPopup();
            }
        };
        task.execute();
    }
    //====================================================================

    // Gets all the icon data put it on the screen (icon mimetype, url, size and content)
    private void getIconDetails() {

        final AsyncTask<Void, Void, IconDetails> task = new AsyncTask<Void, Void, IconDetails>(){

            @Override
            protected void onPreExecute() {
                Log.d(TAG, "getIconDetails: onPreExecute");
                showLoadingPopup("getting icon detilas");
            }

            @Override
            protected IconDetails doInBackground(Void... params){
                return m_application.getIconDetails();
            }

            @Override
            protected void onPostExecute(IconDetails result){

                String url = result.url;
                int size = result.size;
                byte[] content = result.content;
                String mimeType = result.mimeType;

                m_iconMimeType.setText(mimeType+"");
                m_iconSize.setText(size+"");
                m_iconUrl.setText(url);
                if(size == content.length){
                    Bitmap aboutIcon = BitmapFactory.decodeByteArray(content, 0, content.length);
                    m_iconContent.setImageBitmap(aboutIcon);
                }
                m_application.makeToast("Get icon done");
                Log.d(TAG, "getIconDetails: onPostExecute");
                m_tasksToPerform--;
                dismissLoadingPopup();
            }
        };
        task.execute();

    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onDestroy()
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        m_application.stopIconSession();
        if(m_receiver != null){
            try{
                unregisterReceiver(m_receiver);
            } catch (IllegalArgumentException e) {}
        }
    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.icon_menu, menu);
        return true;
    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getItemId()) {
        case R.id.menu_icon_refresh:
            m_tasksToPerform = 2;
            getVersion();
            getIconDetails();
            break;
        }
        return true;
    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onConfigurationChanged(android.content.res.Configuration)
     */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    }
    //====================================================================

    //Let the user know the device not found and we cannot move to the About screen
    private void closeScreen() {

        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle("Error");
        alert.setMessage("Device was not found");

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                dialog.dismiss();
                finish();
            }
        });
        alert.show();
    }
    //====================================================================

    // Display a progress dialog with the given msg.
    // If the dialog is already showing - it will update its message to the given msg.
    // The dialog will dismiss after 30 seconds if no response has returned.
    private void showLoadingPopup(String msg)
    {
        if (m_loadingPopup !=null){
            if(!m_loadingPopup.isShowing()){
                m_loadingPopup = ProgressDialog.show(this, "", msg, true);
                Log.d(TAG, "showLoadingPopup with msg = "+msg);
            }
            else{
                m_loadingPopup.setMessage(msg);
                Log.d(TAG, "setMessage with msg = "+msg);
            }
        }
        m_timer = new Timer();
        m_timer.schedule(new TimerTask() {
            public void run() {
                if (m_loadingPopup !=null && m_loadingPopup.isShowing()){
                    Log.d(TAG, "showLoadingPopup dismissed the popup");
                    m_loadingPopup.dismiss();
                };
            }

        },30*1000);
    }
    //====================================================================

    // Dismiss the progress dialog (only if it is showing).
    private void dismissLoadingPopup()
    {
        if(m_tasksToPerform == 0){
            if (m_loadingPopup != null){
                Log.d(TAG, "dismissLoadingPopup dismissed the popup");
                m_loadingPopup.dismiss();
                if(m_timer != null){
                    m_timer.cancel();
                }
            }
        }
    }
    //====================================================================
}
