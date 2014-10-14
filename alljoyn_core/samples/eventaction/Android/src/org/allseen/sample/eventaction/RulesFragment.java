/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

package org.allseen.sample.eventaction;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import org.allseen.sample.event.tester.ActionDescription;
import org.allseen.sample.event.tester.Description;

import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.ListView;
import android.widget.SimpleAdapter;
import android.widget.TextView;

public class RulesFragment extends Fragment {
	private final static String TAG = "RulesFragment";
    
    private final static String KEY_RULE = "rule";
   
    private ListView rulesListView;
	private static MySimpleAdapter simpleAdapter;
	public static Vector<Rules> getRules() { return simpleAdapter.mRules; }
	public static void addRules(Rules r) { simpleAdapter.mRules.add(r);  saveRule(r.toString()); simpleAdapter.notifyChanged(); }
	public static void updateRules(String sessionName, String friendlyName) { simpleAdapter.updateRules(sessionName, friendlyName); }
	
	 private static String getParentDir() {
 	    String parentDirPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/DEMO";
 	    File parentDir = new File(parentDirPath);
 	    if (!parentDir.exists()) {
 	        parentDir.mkdirs();
 	    }
 	    return parentDirPath;
 	}
     
     private static void saveRule(String data) {
 		String rules = readFromFile();
 		rules += data + ";";
 		writeToFile(rules);
 		Log.d(TAG, "Saved Rule: "+data);
 	}
 	
 	public static void loadRules() {
 		String ruleString = readFromFile();
 		Log.d(TAG, "Read saved string: "+ruleString);
 		if(ruleString.length() > 0) {
 			String[] rules = ruleString.split("\\;");
 			for (int i = 0; i < rules.length; i++) {
 				if (rules[i] != null && rules[i].length() > 0) { 
 					Log.d(TAG, "Parsing saved rule: "+rules[i]);
 					Rules r = new Rules();
 					r.createFromString(rules[i]);
					BackgroundService.mEventHandler.rawEnable(r);
					simpleAdapter.mRules.add(r);
					simpleAdapter.notifyChanged();
 				}
 			}
 		}
 	}
 	
 	private static void writeToFile(String data) {
         try {
         	Log.d(TAG, "saving out: "+data);
         	File saveFile = new File(getParentDir() + "/ruleData.conf");
         	if(!saveFile.exists()) {
         		saveFile.createNewFile();
         	} 
         	FileOutputStream outputStreamWriter = new FileOutputStream(saveFile);
             outputStreamWriter.write(data.getBytes());
             outputStreamWriter.close();
         }
         catch (Exception e) {
         	e.printStackTrace();
     		Log.d(TAG, "ERROR: "+e);
         } 
     }

     private static String readFromFile() {
         String ret = "";

         try {
         	File saveFile = new File(getParentDir() + "/ruleData.conf");
         	if(!saveFile.exists()) {
         		saveFile.createNewFile();
         	} 
         	FileInputStream inputStream = new FileInputStream(saveFile);

             if ( inputStream != null ) {
                 InputStreamReader inputStreamReader = new InputStreamReader(inputStream);
                 BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
                 String receiveString = "";
                 StringBuilder stringBuilder = new StringBuilder();

                 while ( (receiveString = bufferedReader.readLine()) != null ) {
                     stringBuilder.append(receiveString);
                 }

                 inputStream.close();
                 ret = stringBuilder.toString();
             	Log.d(TAG, "Read in: "+ret);
             }
         }
         catch (Exception e) {
             e.printStackTrace();
     		Log.d(TAG, "ERROR: "+e);
         }

         return ret;
     }
	
	private static Description mSelectedEvent;
	
	public Description getSelectedEvent() { return mSelectedEvent; }
	
	private List<Map<String, String>> mapList = new ArrayList<Map<String, String>>();
	private String[] from = new String[] {KEY_RULE};
	private int[] to = new int[] {R.id.rule_text};
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstance) {
		View view = inflater.inflate(R.layout.rule_fragment, container, false);
		rulesListView = (ListView) view.findViewById(R.id.rule_list_view);
		if (null == simpleAdapter) {
		    simpleAdapter = new MySimpleAdapter(getActivity(), mapList, R.layout.rule_item_rule, from, to);
		}
		rulesListView.setAdapter(simpleAdapter);
		return view;
	}
	
	public void clearAllRules() {
	    mapList.clear();
	    simpleAdapter.clearChecked();
	    notifyChanged();
	}
	
	public void clearChecked() {
	    simpleAdapter.clearChecked();
	    notifyChanged();
	}
	
	public void removeSelected() {
		simpleAdapter.removeSelected();
	    notifyChanged();
	}
	
	public void deleteAllRules() {
		simpleAdapter.deleteAllRules();
		writeToFile("");
		notifyChanged();
	}
	
	private void notifyChanged() {
		simpleAdapter.notifyChanged();
	}
	
    @Override
    public void onStart() {
        super.onStart();
    };
    
    @Override
    public void onStop() {
        super.onStop();
    };

    private class MySimpleAdapter extends SimpleAdapter {
    	private Vector<Rules> mRules = new Vector<Rules>();
        private List<Boolean> checkedList = new ArrayList<Boolean>();
        private LayoutInflater inflater;
        
        public MySimpleAdapter(Context context, List<? extends Map<String, ?>> data, int resource, String[] from, int[] to) {
            super(context, data, resource, from, to);
            this.inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        }
        
        @Override
        public int getCount() {
			return mRules.size();
        }
        
        @Override
        public View getView(final int position, View convertView, ViewGroup parent) {
            if (null == convertView) {
                convertView = inflater.inflate(R.layout.rule_item_rule, null);
            }
            while (simpleAdapter.getCount() > checkedList.size()) {
                checkedList.add(false);
            }
            CheckBox cb = (CheckBox) convertView.findViewById(R.id.rule_selected);
            cb.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View v) {
                    while (simpleAdapter.getCount() > checkedList.size()) {
                        checkedList.add(false);
                    }
                    CheckBox cb = (CheckBox) v;
                    checkedList.set(position, cb.isChecked());
                }
            });
            cb.setChecked(checkedList.get(position));

            TextView textViewRuleText = (TextView) convertView.findViewById(R.id.rule_text);
            String actionDescs = "";
            Vector<ActionDescription> actions = mRules.get(position).getActions();
            for(int i = 0; i < actions.size(); i++) {
            	actionDescs += actions.get(i).getDescription();
            	if(i+1 !=actions.size() ) {
            		actionDescs += " and ";
            	}
            }
            textViewRuleText.setText(mRules.get(position).getEvent().getDescription() + ", " +actionDescs);
            
            return convertView;
        }
        
        
        private void notifyChanged() {
    		getActivity().runOnUiThread(new Runnable() {
    			@Override
                public void run() {
    			    notifyDataSetChanged();
    			}
    		});
    	}
        
        public void clearChecked() {
            for (int i=0; i<checkedList.size(); i++) {
                checkedList.set(i, false);
            }
        }
        
        public void removeSelected() {
        	for (int i=0; i<checkedList.size(); i++) {
                if(checkedList.get(i) == true) {
                	mRules.get(i).removeAllActions();
                	mRules.remove(i);
                }
                checkedList.set(i, false);
            }
        }
        
        public void deleteAllRules() {
        	for (int i=0; i<checkedList.size(); i++) {
                checkedList.set(i, false);
            }
        	for (int i=0; i<mRules.size(); i++) {
            	mRules.get(i).removeAllActions();
            	mRules.remove(i);
            }
        	mRules.clear();
        }
        
        public void updateRules(String sessionName, String friendlyName)  {
        	boolean change = false;
        	for(Rules r : mRules) {
	        	Vector<ActionDescription> actions = r.getActions();
	            for(int i = 0; i < actions.size(); i++) {
	            	ActionDescription a = actions.get(i);
	            	if(a.getFriendlyName().compareTo(friendlyName) == 0) {
	            		a.setSessionName(sessionName);
	            		actions.set(i, a);
	            		change = true;
	            	}
	            }
        	}
        	if (change) {
        		writeToFile("");
        		for(Rules r : mRules) {
        			saveRule(r.toString());
        		}
        	}
        }
    }
}
