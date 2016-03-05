/******************************************************************************
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
 ******************************************************************************/

package org.alljoyn.config.test;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.config.test.ConfigApplication.Device;

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
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.TextView;

/**
 * The ConfigActivity displays all the config data of the device and enables all
 * the config methods the config service enables. It enables the user to see the
 * config version and the configurable data of the device, set a field to a new
 * value or reset it to its default value, do factory reset, restart the device
 * and change the device password It also enables the user to choose the config
 * language on the menu and get the configurable fields in that selected
 * language.
 */
public class ConfigActivity extends Activity {

    // General
    protected static final String TAG = ConfigApplication.TAG;
    private BroadcastReceiver mainReceiver;
    private UUID deviceId;
    private ProgressDialog loadingPopup;

    // Current network
    private TextView currentNetworkTextView;

    // Version
    private TextView configVersionTextView;

    // Config data
    private AlertDialog passwordAlertDialog;

    EditText deviceNameValue;
    EditText deviceLangValue;
    CheckBox deviceNameCheckBox;
    CheckBox deviceLangCheckBox;

    /*
     * (non-Javadoc)
     * 
     * @see android.app.Activity#onCreate(android.os.Bundle)
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);

        // General
        setContentView(R.layout.config_layout);
        deviceId = (UUID) getIntent().getSerializableExtra(ConfigApplication.EXTRA_DEVICE_ID);
        Device device = ((ConfigApplication) getApplication()).getDevice(deviceId);
        if (device == null) {
            closeScreen();
            return;
        }
        // startSession();
        loadingPopup = new ProgressDialog(this);

        // Current Network
        currentNetworkTextView = (TextView) findViewById(R.id.current_network_name);
        String ssid = ((ConfigApplication) getApplication()).getCurrentSSID();
        currentNetworkTextView.setText(getString(R.string.current_network, ssid));

        // Version
        configVersionTextView = (TextView) findViewById(R.id.config_version_value);

        deviceNameValue = (EditText) findViewById(R.id.configNameValue);
        deviceNameValue.setText(device.deviceName);
        deviceNameCheckBox = (CheckBox) findViewById(R.id.configNameCheckBox);

        deviceLangValue = (EditText) findViewById(R.id.configLangValue);
        deviceLangValue.setText(device.configLanguage);
        deviceLangCheckBox = (CheckBox) findViewById(R.id.configLangCheckBox);

        Button saveButton = (Button) findViewById(R.id.configSave);
        Button resetButton = (Button) findViewById(R.id.configReset);

        saveButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View arg0) {
                if (deviceNameCheckBox.isChecked() || deviceLangCheckBox.isChecked()) {
                    setConfig();
                } else {
                    noFieldsChosen();
                }
            }
        });

        resetButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (deviceNameCheckBox.isChecked() || deviceLangCheckBox.isChecked()) {
                    resetConfiguration();
                } else {
                    noFieldsChosen();
                }
            }
        });

        mainReceiver = new BroadcastReceiver() {

            @Override
            public void onReceive(Context context, Intent intent) {

                if (ConfigApplication.ACTION_PASSWORD_IS_INCORRECT.equals(intent.getAction())) {
                    if (passwordAlertDialog != null && !passwordAlertDialog.isShowing()) {
                        passwordAlertDialog.show();
                    }
                }

                else if (ConfigApplication.ACTION_ERROR.equals(intent.getAction())) {
                    String error = intent.getStringExtra(ConfigApplication.EXTRA_ERROR);
                    ((ConfigApplication) getApplication()).showAlert(ConfigActivity.this, error);
                } else if (ConfigApplication.ACTION_CONNECTED_TO_NETWORK.equals(intent.getAction())) {

                    String ssid = intent.getStringExtra(ConfigApplication.EXTRA_NETWORK_SSID);
                    currentNetworkTextView.setText(getString(R.string.current_network, ssid));
                }
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(ConfigApplication.ACTION_PASSWORD_IS_INCORRECT);
        filter.addAction(ConfigApplication.ACTION_ERROR);
        filter.addAction(ConfigApplication.ACTION_CONNECTED_TO_NETWORK);
        registerReceiver(mainReceiver, filter);

        initPasswordAlertDialog();
        getVersion();

    }

    // Initialize the pop-up requesting the user to enter its password.
    private void initPasswordAlertDialog() {

        AlertDialog.Builder alert = new AlertDialog.Builder(ConfigActivity.this);
        alert.setTitle(R.string.alert_password_incorrect_title);
        alert.setCancelable(false);

        final EditText input = new EditText(ConfigActivity.this);
        input.setText("");
        alert.setView(input);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {

                String devicePassword = input.getText().toString();
                Device device = ((ConfigApplication) getApplication()).getDevice(deviceId);
                if (device != null) {
                    device.password = devicePassword.toCharArray();
                }
                dialog.dismiss();
            }
        });

        alert.setNegativeButton(android.R.string.cancel, null);
        passwordAlertDialog = alert.create();
    }

    // Gets the device config version and displays it
    private void getVersion() {

        final AsyncTask<Void, Void, Short> task = new AsyncTask<Void, Void, Short>() {

            @Override
            protected void onPreExecute() {
                Log.d(TAG, "getConfigVersion: onPreExecute");
                showLoadingPopup(getString(R.string.loading_get_config_version));
            }

            @Override
            protected Short doInBackground(Void... params) {
                return ((ConfigApplication) getApplication()).getConfigVersion(deviceId);
            }

            @Override
            protected void onPostExecute(Short result) {
                short version = result.shortValue();
                configVersionTextView.setText(String.valueOf(version));
                Log.d(TAG, "getConfigVersion: onPostExecute");
                dismissLoadingPopup();
                getConfigData();
            }
        };
        task.execute();

    }

    // Gets the configurable data of the device and display it
    private void getConfigData() {
        final AsyncTask<Void, Void, Void> task = new AsyncTask<Void, Void, Void>() {

        	/**
        	 * The language to retrieve the configuration data
        	 */
        	private String requestLanguage;
        	
        	/**
        	 * The configured device
        	 */
        	private Device device;
        	
            @Override
            protected void onPreExecute() {

            	requestLanguage = deviceLangValue.getText().toString();
            	device          = ((ConfigApplication) getApplication()).getDevice(deviceId);
            	
            	if ( requestLanguage.length() == 0 && device != null ) {
            		
            		requestLanguage = device.configLanguage;
            	}
            	
                Log.d(TAG, "setConfigData: onPreExecute. Language of request: '" + requestLanguage + "'");
                showLoadingPopup(getString(R.string.loading_get_config));
            }

            @Override
            protected Void doInBackground(Void... params) {
                
                if (device != null) {
                    ((ConfigApplication) getApplication()).getConfig(requestLanguage, device.appId);
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {

                Device device = ((ConfigApplication) getApplication()).getDevice(deviceId);
                if (device != null) {
                    deviceNameValue.setText(device.deviceName);
                    deviceLangValue.setText(device.configLanguage);
                }

                Log.d(TAG, "setConfigData: onPostExecute");
                dismissLoadingPopup();
            }
        };
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
            task.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        } else {
            task.execute();
        }

    }

    // Sets the config data to the values the user entered in the screen.
    @SuppressWarnings("unchecked")
	private void setConfig() {

        final AsyncTask<Map<String, Object>, Void, Void> task = new AsyncTask<Map<String, Object>, Void, Void>() {

        	/**
        	 * The language to retrieve the configuration data
        	 */
        	private String requestLanguage;
        	
        	/**
        	 * The configured device
        	 */
        	private Device device;
        	
            @Override
            protected void onPreExecute() {

            	requestLanguage = deviceLangValue.getText().toString();
            	device          = ((ConfigApplication) getApplication()).getDevice(deviceId);
            	
            	if ( requestLanguage.length() == 0 && device != null ) {
            		
            		requestLanguage = device.configLanguage;
            	}
            	
                Log.d(TAG, "setConfig: onPreExecute Language of request: '" + requestLanguage + "'");
                showLoadingPopup(getString(R.string.loading_setting_config));
            }

            @Override
            protected Void doInBackground(Map<String, Object>... params) {
                
                if (device != null) {
                    Map<String, Object> configMap = params[0];
                    ((ConfigApplication) getApplication()).setConfig(configMap, device.appId, requestLanguage);
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                Log.d(TAG, "setConfig: onPostExecute");
                dismissLoadingPopup();
            }

        };

        HashMap<String, Object> configMap = new HashMap<String, Object>();
        if (deviceLangCheckBox.isChecked()) {
            configMap.put(AboutKeys.ABOUT_DEFAULT_LANGUAGE, deviceLangValue.getText().toString());
        }
        if (deviceNameCheckBox.isChecked()) {
            configMap.put(AboutKeys.ABOUT_DEVICE_NAME, deviceNameValue.getText().toString());
        }
        
        task.execute(configMap);
    }

    // Reset the config fields to its default values
    private void resetConfiguration() {
        final AsyncTask<String[], Void, Void> task = new AsyncTask<String[], Void, Void>() {

        	/**
        	 * The language to retrieve the configuration data
        	 */
        	private String requestLanguage;
        	
        	/**
        	 * The configured device
        	 */
        	private Device device;
        	
        	
            @Override
            protected void onPreExecute() {

            	requestLanguage = deviceLangValue.getText().toString();
            	device          = ((ConfigApplication) getApplication()).getDevice(deviceId);
            	
            	if ( requestLanguage.length() == 0 && device != null ) {
            		
            		requestLanguage = device.configLanguage;
            	}
            	
                Log.d(TAG, "resetConfiguration: onPreExecute. Language of request: '" + requestLanguage + "'");
                showLoadingPopup(getString(R.string.loading_reset_configuration));
            }

            @Override
            protected Void doInBackground(String[]... params) {
                Device device = ((ConfigApplication) getApplication()).getDevice(deviceId);
                if (device != null) {
                    String[] fields = params[0];
                    ((ConfigApplication) getApplication()).resetConfiguration(fields, requestLanguage, device.appId);
                }
                return null;
            }

            @Override
            protected void onPostExecute(Void result) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Device device = ((ConfigApplication) getApplication()).getDevice(deviceId);
                        if (device != null) {
                            deviceLangValue.setText(device.configLanguage);
                            deviceNameValue.setText(device.deviceName);

                            deviceLangCheckBox.setChecked(false);
                            deviceNameCheckBox.setChecked(false);
                        }
                    }
                });
                dismissLoadingPopup();
            }
        };

        List<String> listToReset = new ArrayList<String>();
        if (deviceLangCheckBox.isChecked()) {
            listToReset.add(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
        }
        if (deviceNameCheckBox.isChecked()) {
            listToReset.add(AboutKeys.ABOUT_DEVICE_NAME);
        }
        String[] stringArray = new String[listToReset.size()];
        listToReset.toArray(stringArray);
        task.execute(new String[][] { stringArray });

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        ((ConfigApplication) getApplication()).stopConfigSession();
        if (mainReceiver != null) {
            try {
                unregisterReceiver(mainReceiver);
            } catch (IllegalArgumentException e) {
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.config_menu, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        switch (item.getItemId()) {
        case R.id.menu_config_refresh:
            getVersion();
            break;
        case R.id.menu_config_factory_reset:
            factoryReset();
            break;
        case R.id.menu_config_restart:
            restart();
            break;
        case R.id.menu_config_set_password:
            setPassword();
            break;
        case R.id.menu_config_show_announcement:
            showAnnounce();
            break;
        }
        return true;
    }

    private void factoryReset() {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setMessage(R.string.alert_factory_reset);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {

            @Override
            public void onClick(DialogInterface dialog, int whichButton) {

                final AsyncTask<Void, Void, Void> task = new AsyncTask<Void, Void, Void>() {

                    @Override
                    protected void onPreExecute() {
                        Log.d(TAG, "factoryReset: onPreExecute");
                        showLoadingPopup(getString(R.string.loading_factory_reset));
                    }

                    @Override
                    protected Void doInBackground(Void... params) {
                        ((ConfigApplication) getApplication()).doFactoryReset(deviceId);
                        return null;
                    }

                    @Override
                    protected void onPostExecute(Void result) {
                        Log.d(TAG, "factoryReset: onPostExecute");
                        dismissLoadingPopup();
                        finish();
                    }
                };

                task.execute();
            }
        });

        alert.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
                dialog.dismiss();
            }
        });
        alert.show();
    }

    // Restart the device
    private void restart() {
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setMessage(R.string.alert_restart);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {

                final AsyncTask<Void, Void, Void> task = new AsyncTask<Void, Void, Void>() {

                    @Override
                    protected void onPreExecute() {
                        Log.d(TAG, "restart: onPreExecute");
                        showLoadingPopup(getString(R.string.loading_restart));
                    }

                    @Override
                    protected Void doInBackground(Void... params) {
                        ((ConfigApplication) getApplication()).doRestart(deviceId);
                        return null;
                    }

                    @Override
                    protected void onPostExecute(Void result) {
                        Log.d(TAG, "restart: onPostExecute");
                        dismissLoadingPopup();
                        finish();
                    }
                };

                task.execute();
            }
        });

        alert.setNegativeButton(android.R.string.cancel, null);
        alert.show();
    }

    // Sets the device password
    public void setPassword() {

        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(R.string.alert_title_enter_new_password);

        View view = getLayoutInflater().inflate(R.layout.enter_password_popup, null);
        final EditText oldPassEditText = (EditText) view.findViewById(R.id.oldPasswordEditText);
        final EditText newPassEditText = (EditText) view.findViewById(R.id.newPasswordEditText);
        alert.setView(view);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {

                final AsyncTask<Void, Void, Void> task = new AsyncTask<Void, Void, Void>() {

                    private String newPassword;
                    private String oldPassword;

                    @Override
                    protected void onPreExecute() {
                        newPassword = newPassEditText.getText().toString();
                        oldPassword = oldPassEditText.getText().toString();

                        Log.d(TAG, "setPassword: onPreExecute");
                        showLoadingPopup(getString(R.string.loading_set_password));
                    }

                    @Override
                    protected Void doInBackground(Void... params) {
                        Device device = ((ConfigApplication) getApplication()).getDevice(deviceId);
                        device.password = oldPassword.toCharArray();
                        ((ConfigApplication) getApplication()).setPasscodeOnServerSide(newPassword, deviceId);
                        return null;
                    }

                    @Override
                    protected void onPostExecute(Void result) {
                        Log.d(TAG, "setPassword: onPostExecute");
                        dismissLoadingPopup();
                    }
                };

                task.execute();
            }
        });

        alert.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
            }
        });
        alert.show();

    }

    // Given a device parameter, displays a dialog with the device announcement
    // data.
    private void showAnnounce() {
        Device device = ((ConfigApplication) getApplication()).getDevice(deviceId);

        if (device == null) {
            Log.e(TAG, "No device available");
        }
        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        String msg = device.getAnnounce();
        alert.setMessage(msg);

        alert.setPositiveButton(android.R.string.ok, null);
        alert.show();

    }

    /*
     * (non-Javadoc)
     * 
     * @see
     * android.app.Activity#onConfigurationChanged(android.content.res.Configuration
     * )
     */
    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
    }

    // Let the user know the device was not found and we cannot move to the
    // About
    // screen
    private void closeScreen() {

        AlertDialog.Builder alert = new AlertDialog.Builder(this);
        alert.setTitle(R.string.error);
        alert.setMessage(R.string.device_was_not_found);

        alert.setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
                dialog.dismiss();
                finish();
            }
        });
        alert.show();
    }

    // Display a progress dialog with the given msg.
    // If the dialog is already showing - it will update its message to the
    // given msg.
    private void showLoadingPopup(String msg) {
        if (loadingPopup != null) {
            if (!loadingPopup.isShowing()) {
                loadingPopup = ProgressDialog.show(this, "", msg, true);
                Log.d(TAG, "showLoadingPopup with msg = " + msg);
            } else {
                loadingPopup.setMessage(msg);
                Log.d(TAG, "setMessage with msg = " + msg);
            }
        }

    }

    // Dismiss the progress dialog (only if it is showing).
    private void dismissLoadingPopup() {
        if (loadingPopup != null) {
            Log.d(TAG, "dismissLoadingPopup dismissed the popup");
            loadingPopup.dismiss();
        }
    }

    public void noFieldsChosen() {
        AlertDialog.Builder alert = new AlertDialog.Builder(ConfigActivity.this);
        alert.setTitle(R.string.error);
        alert.setMessage(R.string.alert_msg_chose_fields);

        alert.setNeutralButton(android.R.string.ok, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int whichButton) {
            }
        });
        alert.show();
        return;

    }
}
