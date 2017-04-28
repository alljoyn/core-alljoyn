package org.alljoyn.bus.samples.securitymanager;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by George on 1/2/2017.
 */
public class MainListAdapter extends BaseAdapter {
    private Context context;
    private SecurityApplicationStateListener applicationStateListener;

    public MainListAdapter (Context ct, SecurityApplicationStateListener apsl) {
        context = ct;
        applicationStateListener = apsl;
    }

    @Override
    public int getCount() {
        return applicationStateListener.getMap().size();
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        View view = convertView;
        if (view == null) {
            LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            view = inflater.inflate(R.layout.main_list_item, null, false);
        }

        List<OnlineApplication> tempList = new ArrayList<>(applicationStateListener.getMap().values());

        TextView textBusName = (TextView) view.findViewById(R.id.textBusName);
        textBusName.setText(tempList.get(position).getBusName());

        TextView textStatus = (TextView) view.findViewById(R.id.textStatus);
        textStatus.setText(tempList.get(position).getAppState().toString());

        return view;
    }
}
