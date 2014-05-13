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

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnCreateContextMenuListener;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * The DeviceListActivity displays all the devices we are announced on.
 */
public class DeviceListActivity extends ListActivity implements OnCreateContextMenuListener{

    protected static final String TAG = AboutApplication.TAG;
    private AboutApplication m_application;
    private BroadcastReceiver m_receiver;
    private DeviceListAdapter m_announcedNamedAdapter;
    private Button m_AJConnect;
    private TextView m_currentNetwork;

    //======================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onCreate(android.os.Bundle)
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.device_list);
        m_application = (AboutApplication)getApplication();

        //************** AllJoyn Connect/Disconnect Button ******************
        m_AJConnect = (Button) findViewById(R.id.AllJoynConnect);
        m_AJConnect.setOnClickListener(new OnClickListener() {

            @Override
            public void onClick(View v) {

                if(m_AJConnect.getText().equals(getString(R.string.AllJoynConnect))){
                    allJoynConnect();
                }
                else if(m_AJConnect.getText().equals(getString(R.string.AllJoynDisconnect))){
                    allJoynDisconnect();
                }
            }
        });
        //***************** Current Network *********************
        m_currentNetwork = (TextView) findViewById(R.id.current_network_name);
        String ssid = m_application.getCurrentSSID();
        m_currentNetwork.setText(getString(R.string.current_network, ssid));

        //************** Announced names list ******************
        m_announcedNamedAdapter = new DeviceListAdapter(this, R.layout.device_list_property);
        m_announcedNamedAdapter.setLayoutInflator(getLayoutInflater());
        setListAdapter(m_announcedNamedAdapter);
        registerForContextMenu(getListView());

        //************** Class receiver ******************
        m_receiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {

                if(Keys.Actions.ACTION_DEVICE_FOUND.equals(intent.getAction())){

                    String appId = intent.getExtras().getString(Keys.Extras.EXTRA_DEVICE_ID);
                    SoftAPDetails device = m_application.getDevice(appId);
                    m_announcedNamedAdapter.addDevice(device);
                    setListAdapter(m_announcedNamedAdapter);
                }

                else if(Keys.Actions.ACTION_DEVICE_LOST.equals(intent.getAction())){

                    String busName = intent.getExtras().getString(Keys.Extras.EXTRA_BUS_NAME);
                    m_announcedNamedAdapter.removeByBusName(busName);
                    setListAdapter(m_announcedNamedAdapter);
                }
                else if(Keys.Actions.ACTION_CONNECTED_TO_NETWORK.equals(intent.getAction())){

                    String ssid = intent.getStringExtra(Keys.Extras.EXTRA_NETWORK_SSID);
                    m_currentNetwork.setText(getString(R.string.current_network, ssid));

                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(Keys.Actions.ACTION_DEVICE_FOUND);
        filter.addAction(Keys.Actions.ACTION_DEVICE_LOST);
        filter.addAction(Keys.Actions.ACTION_CONNECTED_TO_NETWORK);
        registerReceiver(m_receiver, filter);

    }

    //======================================================================

    // Connect to AllJoyn.
    private void allJoynConnect() {

        AlertDialog.Builder alert = new AlertDialog.Builder(DeviceListActivity.this);
        alert.setTitle("Set realm name");

        final EditText input = new EditText(DeviceListActivity.this);
        input.setText("org.alljoyn.BusNode.aboutClient");
        alert.setView(input);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            public void onClick(final DialogInterface dialog, int whichButton) {

                runOnUiThread(new Runnable() {

                    @Override
                    public void run() {

                        dialog.dismiss();
                        m_application.setRealmName(input.getText().toString());
                        m_AJConnect.setText(R.string.AllJoynDisconnect);
                        m_application.doConnect();
                        m_application.makeToast("AJ connect done");
                    }
                });
            }
        });
        alert.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
            }
        });
        alert.show();
    }
    //====================================================================

    // Disconnect from AllJoyn.
    private void allJoynDisconnect(){

        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setMessage("Are you sure you want to disconnect from AllJoyn?");

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            public void onClick(final DialogInterface dialog, int whichButton) {

                runOnUiThread(new Runnable() {

                    @Override
                    public void run() {
                        dialog.dismiss();
                        m_AJConnect.setText(R.string.AllJoynConnect);
                        m_announcedNamedAdapter.clear();
                        setListAdapter(m_announcedNamedAdapter);
                        m_application.doDisconnect();
                        m_application.makeToast("AJ disconnect done");
                    }
                });
            }
        });

        alert.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                dialog.dismiss();
            }
        });
        alert.show();

    }
    //====================================================================

    // Given a device parameter, displays a dialog with the device announcement data.
    private void showAnnounce(SoftAPDetails device) {

        AlertDialog.Builder alert = new AlertDialog.Builder(DeviceListActivity.this);
        alert.setTitle("Announcement of "+device.busName);
        String msg = device.getAnnounce();
        alert.setMessage(msg);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int whichButton) {
                dialog.dismiss();
            }
        });
        alert.show();

    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.ListActivity#onDestroy()
     */
    @Override
    protected void onDestroy() {
        super.onDestroy();
        m_application.doDisconnect();
        m_application.makeToast("AJ disconnect done");
        unregisterReceiver(m_receiver);
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

    /**
     * @param msg Send a DEBUG log message with the given msg.
     */
    public void log(String msg){
        Log.d(AboutApplication.TAG, msg);
    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onContextItemSelected(android.view.MenuItem)
     */
    @Override
    public boolean onContextItemSelected(MenuItem item) {

        AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();
        SoftAPDetails device = (SoftAPDetails) getListAdapter().getItem(info.position);
        Class<?> intentClass = null;

        if(item.getItemId() == R.id.context_menu_announce){
            showAnnounce(device);
        }
        else{
            if(item.getItemId() == R.id.context_menu_about){
                intentClass = AboutActivity.class;
            }
            else if(item.getItemId() == R.id.context_menu_icon){
                intentClass = IconActivity.class;
            }
            Intent intent = new Intent(this, intentClass);
            intent.putExtra(Keys.Extras.EXTRA_DEVICE_ID, device.appId);
            startActivity(intent);
        }
        return true;

    }
    //====================================================================
    /* (non-Javadoc)
     * @see android.app.Activity#onCreateContextMenu(android.view.ContextMenu, android.view.View, android.view.ContextMenu.ContextMenuInfo)
     */
    @Override
    public void onCreateContextMenu(ContextMenu menu, View v,ContextMenuInfo menuInfo) {// :-)

        super.onCreateContextMenu(menu, v, menuInfo);
        getMenuInflater().inflate(R.menu.device_context_menu, menu);

        AdapterContextMenuInfo info = (AdapterContextMenuInfo) menuInfo;
        ListAdapter adapter = getListAdapter();
        SoftAPDetails item = (SoftAPDetails) adapter.getItem(info.position);

        if(!item.supportAbout)
            menu.removeItem(R.id.context_menu_about);
        if(!item.supportIcon)
            menu.removeItem(R.id.context_menu_icon);
    }

    //====================================================================
    /* (non-Javadoc)
     * @see android.app.ListActivity#onListItemClick(android.widget.ListView, android.view.View, int, long)
     */
    @Override
    public void onListItemClick(ListView lv, View view, int position, long id) {
        Toast.makeText(getBaseContext(), R.string.device_list_item_click_toast, Toast.LENGTH_SHORT).show();
        super.onListItemClick(lv, view, position, id);
    }
}

