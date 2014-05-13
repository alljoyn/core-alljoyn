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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Timer;
import java.util.TimerTask;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.about.test.AboutApplication.AboutProperty;
import org.alljoyn.services.common.BusObjectDescription;

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
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;


/**
 * The AboutActivity displays all the about data of the
 * device (the about version, bus object description and the about fields).
 * It also enables the user to choose the about language on the menu 
 * and get the about in that selected language.
 */
public class AboutActivity extends Activity {

    //General
    protected static final String TAG = AboutApplication.TAG;
    private AboutApplication m_application;
    private BroadcastReceiver m_receiver;
    private Timer m_timer;
    private int m_tasksToPerform = 0;
    private SoftAPDetails m_device;
    private ProgressDialog m_loadingPopup;

    //Current network
    private TextView m_currentNetwork;

    //Version
    private TextView m_aboutVersion;

    //Bus Object Description
    private ListView m_busDescriptionList;
    private BusDescriptionAdapter m_busDescriptionAdapter;

    //About data
    private AboutAdapter m_aboutAdapter;
    private TextView m_aboutLanguage;
    private ListView m_aboutList;
    private Map<String, Object> m_aboutMap;

    //====================================================================

    /* (non-Javadoc)
     * @see android.app.Activity#onCreate(android.os.Bundle)
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        //General
        setContentView(R.layout.about_layout);
        String deviceId = getIntent().getStringExtra(Keys.Extras.EXTRA_DEVICE_ID);
        m_application = (AboutApplication)getApplication();
        m_device = m_application.getDevice(deviceId);
        if(m_device == null){
            closeScreen();
            return;
        }
        startAboutSession();

        m_loadingPopup = new ProgressDialog(this);

        //Current Network
        m_currentNetwork = (TextView) findViewById(R.id.current_network_name);
        String ssid = m_application.getCurrentSSID();
        m_currentNetwork.setText(getString(R.string.current_network, ssid));

        //Version
        m_aboutVersion = (TextView)findViewById(R.id.about_version_value);

        //Bus Object Description
        m_busDescriptionList = (ListView)findViewById(R.id.bus_description_list);
        m_busDescriptionAdapter = new BusDescriptionAdapter(this, R.layout.bus_description_property);
        m_busDescriptionAdapter.setLayoutInflator(getLayoutInflater());
        m_busDescriptionList.setAdapter(m_busDescriptionAdapter);

        //About data
        m_aboutLanguage = (TextView)findViewById(R.id.about_language_value);
        m_aboutLanguage.setText(m_device.aboutLanguage);
        m_aboutList = (ListView)findViewById(R.id.about_map_list);
        m_aboutAdapter = new AboutAdapter(this, R.layout.about_property);
        m_aboutAdapter.setLayoutInflator(getLayoutInflater());

        //Receiver
        m_receiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {

                if(Keys.Actions.ACTION_ERROR.equals(intent.getAction())){
                    String error = intent.getStringExtra(Keys.Extras.EXTRA_ERROR);
                    m_application.showAlert(AboutActivity.this, error);
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

        m_tasksToPerform = 3;
        getVersion();
        getBusObjectDescription();
        getAboutData();
    }
    //====================================================================

    private void startAboutSession() {

        final AsyncTask<Void, Void, Void> task = new AsyncTask<Void, Void, Void>(){

            @Override
            protected void onPreExecute() {
                Log.d(TAG, "startSession: onPreExecute");
            }

            @Override
            protected Void doInBackground(Void... params) {
                m_application.startAboutSession(m_device);
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
    // Gets the device about version and put it on the screen
    private void getVersion() {

        final AsyncTask<Void, Void, Short> task = new AsyncTask<Void, Void, Short>(){

            @Override
            protected void onPreExecute() {
                Log.d(TAG, "getAboutVersion: onPreExecute");
                showLoadingPopup("getting about version");
            }

            @Override
            protected Short doInBackground(Void... params){
                return m_application.getAboutVersion();
            }

            @Override
            protected void onPostExecute(Short result){
                short version = result.shortValue();
                m_aboutVersion.setText(String.valueOf(version));
                Log.d(TAG, "getAboutVersion: onPostExecute");
                m_tasksToPerform--;
                dismissLoadingPopup();
            }
        };
        task.execute();
    }
    //====================================================================

    // Gets the device about bus object description and put it on the screen

    private void getBusObjectDescription() {

        final AsyncTask<Void, Void, BusObjectDescription[]> task = new AsyncTask<Void, Void, BusObjectDescription[]>(){

            @Override
            protected void onPreExecute() {
                Log.d(TAG, "setBusObjectDescription: onPreExecute");
                showLoadingPopup("getting bus object description");
            }

            @Override
            protected BusObjectDescription[] doInBackground(Void... params){
                return m_application.getBusObjectDescription();
            }

            @Override
            protected void onPostExecute(BusObjectDescription[] result){

                if(result != null){
                    BusObjectDescription[] busObjectDescription = result;
                    if(busObjectDescription != null && busObjectDescription.length > 0){
                        List<BusObjectDescription> list = Arrays.asList(busObjectDescription);
                        m_busDescriptionAdapter.clear();
                        m_busDescriptionAdapter.addAll(list);
                    }
                }
                Log.d(TAG, "setBusObjectDescription: onPostExecute");
                m_tasksToPerform--;
                dismissLoadingPopup();
            }
        };
        task.execute();
    }
    //====================================================================

    // Gets the device about data and put it on the screen
    private void getAboutData() {

        final AsyncTask<Void, Void, Map<String, Object>> task = new AsyncTask<Void, Void, Map<String, Object>>(){

            @Override
            protected void onPreExecute() {
                Log.d(TAG, "setAboutData: onPreExecute");
                showLoadingPopup("getting about data");
            }

            @Override
            protected Map<String, Object> doInBackground(Void... params){
                return m_application.getAbout(m_device.aboutLanguage);
            }

            @Override
            protected void onPostExecute(Map<String, Object> result){

                if(result != null){
                    m_aboutMap = result;
                    List<AboutProperty> results = new ArrayList<AboutProperty>();
                    for (Entry<String, Object> entry: m_aboutMap.entrySet()){

                        AboutProperty property = m_application.new AboutProperty(entry.getKey(), entry.getValue());
                        results.add(property);
                    }
                    m_aboutAdapter.setData(results);
                    m_aboutList.setAdapter(m_aboutAdapter);
                }
                Log.d(TAG, "setAboutData: onPostExecute");
                m_tasksToPerform--;
                dismissLoadingPopup();
            }
        };
        task.execute();

    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onStop()
     */
    @Override
    protected void onStop() {
        super.onStop();
        if(m_timer != null){
            m_timer.cancel();
        }
    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onDestroy()
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        m_application.stopAboutSession();
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

        getMenuInflater().inflate(R.menu.about_menu, menu);
        return true;
    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getItemId()) {
        case R.id.menu_about_refresh:
            m_tasksToPerform = 3;
            getVersion();
            getBusObjectDescription();
            getAboutData();
            break;

        case R.id.menu_about_set_language:
            setAboutLanguage();
            break;

        }
        return true;
    }
    //====================================================================

    // Display a dialog where you can enter the about language.
    // Pressing OK will request the about data again in the selected language
    private void setAboutLanguage() {

        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle("Set about language");
        alert.setCancelable(false);

        View view = getLayoutInflater().inflate(R.layout.set_language_popup, null);
        alert.setView(view);
        final EditText newLanguage = (EditText)view.findViewById(R.id.language_edit_text);
        TextView supportedLangs = (TextView)view.findViewById(R.id.supportedLanguages);
        TextView supportedLangsValue = (TextView)view.findViewById(R.id.supported_languages_value);

        if(m_aboutMap != null){
            String[] ss = (String[])m_aboutMap.get(AboutKeys.ABOUT_SUPPORTED_LANGUAGES);
            String res = "";
            for (int i = 0; i < ss.length; i++) {
                if(!ss[i].equals(""))
                    res += ","+ss[i];
            }
            if(res.length() > 0)
                res = res.substring(1);
            supportedLangsValue.setText(res);
        }
        else{
            supportedLangs.setVisibility(View.GONE);
        }

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {

            public void onClick(DialogInterface dialog, int whichButton) {
                m_device.aboutLanguage = newLanguage.getEditableText().toString();
                m_aboutLanguage.setText(newLanguage.getEditableText().toString());
                m_tasksToPerform = 1;
                getAboutData();
            }
        });

        alert.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
            }
        });
        alert.show();
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

