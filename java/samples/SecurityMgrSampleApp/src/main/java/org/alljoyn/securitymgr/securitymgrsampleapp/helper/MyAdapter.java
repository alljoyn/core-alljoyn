package org.alljoyn.securitymgr.securitymgrsampleapp.helper;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import org.alljoyn.securitymgr.ClaimState;
import org.alljoyn.securitymgr.RunningState;
import org.alljoyn.securitymgr.securitymgrsampleapp.AppsListFragment;
import org.alljoyn.securitymgr.securitymgrsampleapp.R;

import android.content.Context;
import android.os.Handler;
import android.support.v7.widget.CardView;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class MyAdapter extends RecyclerView.Adapter<MyAdapter.ViewHolder>
{
    private static final String TAG = "MyAdapter";
    private List<ApplicationsTracker.AppInfoItem> mAppsList = new ArrayList<ApplicationsTracker.AppInfoItem>();
    private final Handler mHandler;
    private final ItemClickListener mListener;
    private final AppsListFragment.ClaimFilter mClaimFilter;
    private final Context mCtx;

    // Provide a reference to the views for each data item
    // Complex data items may need more than one view per item, and
    // you provide access to all the views for a data item in a view holder
    public static class ViewHolder extends RecyclerView.ViewHolder implements View.OnClickListener
    {
        final CardView mRootView;
        public final TextView mTextViewApplicationName;
        final TextView mTextViewDeviceName;
        final TextView mTextViewClaimState;
        final TextView mTextViewRunningState;
        private final ItemClickListener mListener;
        ApplicationsTracker.AppInfoItem appInfoItem;

        public ViewHolder(CardView v, ItemClickListener listener)
        {
            super(v);
            mRootView = v;
            mListener = listener;
            mTextViewApplicationName = (TextView) v.findViewById(R.id.recyclerItem_ApplicationName);
            mTextViewDeviceName = (TextView) v.findViewById(R.id.recyclerItem_DeviceName);
            mTextViewClaimState = (TextView) v.findViewById(R.id.recyclerItem_ClaimState);
            mTextViewRunningState = (TextView) v.findViewById(R.id.recyclerItem_RunningState);

            v.setOnClickListener(this);
        }

        @Override
        public void onClick(View v)
        {
            mListener.onDeviceClicked(appInfoItem);
        }
    }

    public MyAdapter(Context ctx, ItemClickListener listener, AppsListFragment.ClaimFilter claimFilter)
    {
        mCtx = ctx;
        mHandler = new Handler();
        mListener = listener;
        mClaimFilter = claimFilter;
    }

    // Create new views (invoked by the layout manager)
    @Override
    public MyAdapter.ViewHolder onCreateViewHolder(ViewGroup parent,
                                                   int viewType)
    {

        CardView v = (CardView) LayoutInflater.from(parent.getContext())
            .inflate(R.layout.recycler_item, parent, false);
        //Log.d(TAG, "Create viewHolder: " + v.hashCode());
        ViewHolder vh = new ViewHolder(v, mListener);
        return vh;
    }

    // Replace the contents of a view (invoked by the layout manager)
    @Override
    public void onBindViewHolder(ViewHolder holder, int position)
    {
        // - get element from your dataset at this position
        // - replace the contents of the view with that element
        ApplicationsTracker.AppInfoItem item;
        synchronized (mAppsList) {
            item = mAppsList.get(position);
        }
        //Log.d(TAG, "onBindViewHolder: " + holder.mRootView.hashCode() + " -- " + item.content.getApplicationId());
        holder.mTextViewApplicationName.setText(item.content.getApplicationName());
        holder.mTextViewDeviceName.setText("@" + item.content.getDeviceName());
        holder.mTextViewClaimState.setText(item.content.getClaimState().toString());
        holder.mTextViewRunningState.setText(item.content.getRunningState().toString());

        if (RunningState.RUNNING != item.content.getRunningState()) {
            holder.mTextViewApplicationName.setEnabled(false);
            holder.mTextViewDeviceName.setEnabled(false);
            holder.mTextViewClaimState.setEnabled(false);
            holder.mTextViewRunningState.setEnabled(false);
        }
        else {
            holder.mTextViewApplicationName.setEnabled(true);
            holder.mTextViewDeviceName.setEnabled(true);
            holder.mTextViewClaimState.setEnabled(true);
            holder.mTextViewRunningState.setEnabled(true);
        }

        if (item.content.getClaimState() == ClaimState.CLAIMABLE) {
            holder.mRootView.setClickable(true);
        }
        else {
            holder.mRootView.setClickable(false);
        }

        holder.appInfoItem = item;

    }

    @Override
    public int getItemCount()
    {
        synchronized (mAppsList) {
            return mAppsList.size();
        }
    }

    public void setItems(Collection<ApplicationsTracker.AppInfoItem> items)
    {
        synchronized (mAppsList) {
            mAppsList.clear();
            for (ApplicationsTracker.AppInfoItem item : items) {
                if (mClaimFilter.isAllowed(item.content.getClaimState())) {
                    mAppsList.add(item);
                }
            }
        }
        notifyOnUI();
    }

    private boolean addItemInternal(ApplicationsTracker.AppInfoItem item)
    {
        boolean update = false;
        synchronized (mAppsList) {
            int id = mAppsList.indexOf(item);
            if (mClaimFilter.isAllowed(item.content.getClaimState())) {
                //need to be added/updated
                if (id != -1) {
                    //update
                    mAppsList.set(id, item);
                }
                else {
                    //new
                    mAppsList.add(item);
                }
                update = true;
            }
            else {
                //filtered-out, but may need to be removed
                if (id != -1) {
                    mAppsList.remove(id);
                    update = true;
                }
            }
        }
        return update;
    }

    public void addItem(ApplicationsTracker.AppInfoItem item)
    {
        if (addItemInternal(item)) {
            notifyOnUI();
        }
    }

    public void clear()
    {
        synchronized (mAppsList) {
            mAppsList.clear();
        }
        notifyOnUI();
    }

    private void notifyOnUI()
    {
        mHandler.post(new Runnable()
        {
            @Override
            public void run()
            {
                notifyDataSetChanged();
            }
        });
    }

    public interface ItemClickListener
    {
        void onDeviceClicked(ApplicationsTracker.AppInfoItem appInfoItem);
    }
}
