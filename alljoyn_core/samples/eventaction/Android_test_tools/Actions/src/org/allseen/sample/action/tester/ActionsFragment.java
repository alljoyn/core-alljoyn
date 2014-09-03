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

package org.allseen.sample.action.tester;

import org.allseen.sample.action.tester.R;

import java.util.Vector;

import android.support.v4.app.Fragment;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.widget.BaseExpandableListAdapter;
import android.widget.CheckBox;
import android.widget.ExpandableListView;
import android.widget.TextView;

public class ActionsFragment extends Fragment {
	
	private ExpandableListView actionDevices;
	private static ExpandableAdapter actionAdapter;
	
	static private Vector<Description> mSelectedActions = new Vector<Description>();
	
	public Vector<Description> getSelectedActions() { return mSelectedActions; }
	public void clearSelectedActions() { mSelectedActions.clear(); }
	
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstance) {
		View view = inflater.inflate(R.layout.action_fragment, container, false);
		actionDevices = (ExpandableListView)view.findViewById(R.id.device_list_view);
		if(actionAdapter == null)
			actionAdapter = new ExpandableAdapter(getActivity());
		actionDevices.setAdapter(actionAdapter);
		
		return view;
	}
	
	public void addDevice(Device info) {
		actionAdapter.add(info);
		notifyChanged();
	}
	
	public void removeDevice(int sessionId) {
		actionAdapter.remove(sessionId);
		notifyChanged();
	}
	
	public void unsetAllChecks() {
		for(int i = 0; i < actionAdapter.checkboxDirtyFlags.size(); i++) {
			for(int j = 0; j < actionAdapter.checkboxDirtyFlags.elementAt(i).size(); j++) {
				actionAdapter.checkboxDirtyFlags.elementAt(i).set(j, true);
			}
		}
		notifyChanged();
	}
	
	private void notifyChanged() {
		getActivity().runOnUiThread(new Runnable() {
			public void run() {
				actionAdapter.notifyDataSetChanged();
			}
		});
	}
	
	private class ExpandableAdapter extends BaseExpandableListAdapter {

		private Vector<Vector<Boolean>> checkboxDirtyFlags = new Vector<Vector<Boolean>>();
		private Vector<Device> data = new Vector<Device>();
		private LayoutInflater inflater;

		ExpandableAdapter(Context context) {
			this.inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		}
		
		public void add(Device info) {
			int loc = 0;
			for(; loc < data.size(); loc++) {
				Device d = data.get(loc);
				if(d.getSessionName().compareTo(info.getSessionName()) == 0) {
					data.remove(loc);
					checkboxDirtyFlags.remove(loc);
					break;
				}
			}
			data.add(loc,info);
			Vector<Boolean> dirtyFlags = new Vector<Boolean>();
			for(int i = 0; i < info.getActions().size(); i++) {
				dirtyFlags.add(true);
			}
			checkboxDirtyFlags.add(loc,dirtyFlags);
		}
		
		public void remove(int sessionId) {
			for(int i = 0; i < data.size(); i++) {
				Device d = data.get(i);
				if(d.getSessionId() == sessionId) {
					data.remove(i);
					checkboxDirtyFlags.remove(i);
				}
			}
		}

		@Override
		public Object getChild(int groupPosition, int childPosition) {
			return data.elementAt(groupPosition).getActions().get(childPosition);
		}

		@Override
		public long getChildId(int groupPosition, int childPosition) {
			return childPosition;
		}
		
		public void checkboxChanged(View v) {
			CheckBox check = (CheckBox)v;
			if(check.isChecked()) {
				mSelectedActions.add((ActionDescription)check.getTag());
			} else {
				mSelectedActions.remove((ActionDescription)check.getTag());
			}
		}

		@Override
		public View getChildView(int groupPosition, int childPosition,
				boolean isLastChild, View convertView, ViewGroup parent) {
			if(convertView == null) {
				convertView = inflater.inflate(R.layout.rule_action_item_actions, null);
			}
			{	
				final Description eai = (Description)getChild(groupPosition, childPosition);
				TextView tv = (TextView)convertView.findViewById(R.id.action_name);
				tv.setText(eai.getDescription());
	//			tv = (TextView)convertView.findViewById(R.id.action_path);
	//			tv.setText(eai.getPath());
				CheckBox check = (CheckBox)convertView.findViewById(R.id.action_selected);
				check.setOnClickListener(new OnClickListener() {
					@Override
					public void onClick(View v) {
						checkboxChanged(v);
					}
				});
				check.setTag(eai);
				check.setChecked(false);
				
				convertView.setOnLongClickListener(new OnLongClickListener() {
					@Override
	                public boolean onLongClick(View v) {
						try{
							new AlertDialog.Builder(ActionsFragment.this.getActivity())
						    .setTitle("Event Info")
						    .setMessage(
						    		"BusName: "+eai.getSessionName()+"\n" +
						    		"Path: "+eai.getPath()+"\n" +
				    				"IFace: "+eai.getIface()+"\n" +
				    				"Member: "+eai.getMemberName()+"\n" +
				    				"Sig: "+eai.getSignature()+"\n"
						    		)
						    .setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
						        public void onClick(DialogInterface dialog, int which) { 
						        	dialog.dismiss();
						        }
						     })
						    .setIcon(android.R.drawable.ic_dialog_alert)
						     .show();
						}catch(Exception e) {
							e.printStackTrace();
						}
	                    return false;
	                }
				});
			}
			CheckBox check = (CheckBox)convertView.findViewById(R.id.action_selected);
			if(check.isChecked() && checkboxDirtyFlags.elementAt(groupPosition).elementAt(childPosition) == true) {
				check.setChecked(false);
				checkboxDirtyFlags.elementAt(groupPosition).set(childPosition, false);
			} else if (mSelectedActions.contains(check.getTag())) {
				check.setChecked(true);
			}
			return convertView;
		}

		@Override
		public int getChildrenCount(int groupPosition) {
			return data.get(groupPosition).getActions().size();
		}

		@Override
		public Object getGroup(int groupPosition) {
			return data.get(groupPosition);
		}

		@Override
		public int getGroupCount() {
			return data.size();
		}

		@Override
		public long getGroupId(int groupPosition) {
			return groupPosition;
		}

		@Override
		public View getGroupView(int groupPosition, boolean isExpanded,
				View convertView, ViewGroup parent) {
			if(convertView == null) {
				convertView = inflater.inflate(R.layout.action_item, null);
			}
			
			TextView tv = (TextView)convertView.findViewById(R.id.device_name);
			try{
				Device info = (Device) getGroup(groupPosition);
				tv.setText(info.getFriendlyName());
			} catch(Exception e) {
				tv.setText("Error, restart application");
			}
			
			return convertView;
		}

		@Override
		public boolean hasStableIds() {
			return false;
		}

		@Override
		public boolean isChildSelectable(int groupPosition, int childPosition) {
			return true;
		}
	}
}
