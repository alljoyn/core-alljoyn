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

import org.alljoyn.services.common.BusObjectDescription;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;


/**
 * This class is an adapter that displays the bus object description data of the device. 
 */
public class BusDescriptionAdapter extends ArrayAdapter<BusObjectDescription> {

    private LayoutInflater m_layoutInflater;

    //=================================================================
    /**
     * Creates the adapter given a context and a text view resource id.
     * @param context
     * @param resource
     */
    public BusDescriptionAdapter(Context context, int resource) {
        super(context, resource);
    }
    //=================================================================
    /**
     * @param layoutInflater Set a layout inflater.
     * Will be used to inflate the views to display.
     */
    public void setLayoutInflator(LayoutInflater layoutInflater) {
        m_layoutInflater = layoutInflater;
    }
    //=================================================================
    /* (non-Javadoc)
     * @see android.widget.ArrayAdapter#getView(int, android.view.View, android.view.ViewGroup)
     */
    @Override
    public View getView(int position, View convertView, ViewGroup parent) {

        View row;

        if (convertView == null)
            row = m_layoutInflater.inflate(R.layout.bus_description_property, parent, false);
        else
            row = convertView;

        BusObjectDescription busObject = getItem(position);

        //Bus path
        TextView propertyName = (TextView) row.findViewById(R.id.busPath);
        propertyName.setText(busObject.getPath());

        //Bus Interfaces
        TextView propertyValue = (TextView) row.findViewById(R.id.busInterfaces);
        String[] interfaces = busObject.getInterfaces();

        String res = "";
        for(int i = 0; i < interfaces.length; i++){
            res += interfaces[i];
            res += ",";
        }
        if(res.length() > 0){
            res = res.substring(0, res.length()-1);
        }

        propertyValue.setText(res);

        return row;
    }
    //=================================================================

}
