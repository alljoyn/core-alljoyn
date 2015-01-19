package org.alljoyn.securitymgr.securitymgrsampleapp;

import java.util.concurrent.Semaphore;

import org.alljoyn.securitymgr.ApplicationInfo;
import org.alljoyn.securitymgr.ClaimState;
import org.alljoyn.securitymgr.ManifestApprover;
import org.alljoyn.securitymgr.RunningState;
import org.alljoyn.securitymgr.access.Action;
import org.alljoyn.securitymgr.access.Manifest;
import org.alljoyn.securitymgr.access.Member;
import org.alljoyn.securitymgr.access.Rule;
import org.alljoyn.securitymgr.securitymgrsampleapp.helper.ApplicationsTracker;
import org.alljoyn.securitymgr.securitymgrsampleapp.helper.AppsListAdapter;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.content.DialogInterface;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.widget.DefaultItemAnimator;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.Toast;

/**
 * Fragment containing a list of claimed/unclaimed/claimable apps.<br>
 * Activities containing this fragment MUST implement the
 * {@link org.alljoyn.securitymgr.securitymgrsampleapp.AppsListFragment.AppsListFragmentCallbacks} interface.
 */
public class AppsListFragment extends Fragment implements SecurityManagerService.SecurityManagerServiceConnection,
    AppsListAdapter.ItemClickListener
{

    private static final String TAG = "ItemFragment";
    private static final String ARG_CLAIM_FILTER = "claimFilter";

    //private AppsListFragmentCallbacks mListener;
    private SecurityManagerService mService;
    private AppsListAdapter mAdapter;
    private SecurityManagerService.LocalServiceConnection mServiceConnection;
    private MyManifestApprover mManifestApprover;
    private ClaimFilter mClaimFilter;


    /**
     * Mandatory empty constructor for the fragment manager to instantiate the
     * fragment (e.g. upon screen orientation changes).
     */
    public AppsListFragment()
    {
    }

    public static AppsListFragment newInstance(ClaimFilter claimFilter)
    {
        AppsListFragment fragment = new AppsListFragment();
        Bundle args = new Bundle();
        args.putString(ARG_CLAIM_FILTER, claimFilter.name());
        fragment.setArguments(args);
        return fragment;
    }

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        if (getArguments() != null) {
            //Populate arguments
            mClaimFilter = ClaimFilter.valueOf(getArguments().getString(ARG_CLAIM_FILTER));
        }
        else {
            //default: show all.
            mClaimFilter = ClaimFilter.ALL;
        }
        mAdapter = new AppsListAdapter(getActivity(), this, mClaimFilter);
        mManifestApprover = new MyManifestApprover();
        mServiceConnection = SecurityManagerService.getLocalBind(getActivity(), this);
    }

    @Override
    public void onDestroy()
    {
        if (mService != null) {
            //cleanup
            mService.getApplicationsTracker().setAdapter(null);
            mService.setManifestApprover(null);
        }
        mServiceConnection.unBind();
        super.onDestroy();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState)
    {
        View view = inflater.inflate(R.layout.fragment_item, container, false);

        // Set the adapter
        RecyclerView recyclerView = (RecyclerView) view.findViewById(R.id.my_recycler_view);

        recyclerView.setHasFixedSize(false);

        // use a linear layout manager
        recyclerView.setLayoutManager(new LinearLayoutManager(getActivity()));
        recyclerView.setItemAnimator(new DefaultItemAnimator());
        recyclerView.setAdapter(mAdapter);

        return view;
    }

    @Override
    public void onAttach(Activity activity)
    {
        super.onAttach(activity);
//        try {
//            mListener = (AppsListFragmentCallbacks) activity;
//        }
//        catch (ClassCastException e) {
//            throw new ClassCastException(activity.toString()
//                + " must implement OnFragmentInteractionListener");
//        }
    }

    @Override
    public void onDetach()
    {
        super.onDetach();
        //mListener = null;
    }


    @Override
    public void onLocalServiceConnected(SecurityManagerService service)
    {
        Log.d(TAG, "LocalService Connected, setting adapter");
        mService = service;
        mService.getApplicationsTracker().setAdapter(mAdapter);
        mService.setManifestApprover(mManifestApprover);
    }

    @Override
    public void onLocalServiceDisconnected()
    {
        Log.d(TAG, "Disconnected, stopping");
        getActivity().finish();
    }

    @Override
    public void onDeviceClicked(ApplicationsTracker.AppInfoItem appInfoItem)
    {
        final ApplicationInfo appInfo = appInfoItem.content;

        if ((appInfo.getClaimState() == ClaimState.CLAIMABLE) && (appInfo.getRunningState() == RunningState.RUNNING)) {

            //somebody clicked on a claimable device.
            //Show confirmation first.
            final AlertDialog.Builder manifestDialogBuilder = new AlertDialog.Builder(getActivity());
            manifestDialogBuilder.setTitle(getActivity().getString(R.string.claiming_dialog_title));
            manifestDialogBuilder.setMessage(getActivity().getString(R.string.claiming_dialog_msg,
                appInfo.getApplicationName()));
            manifestDialogBuilder.setNegativeButton(android.R.string.no, null);
            //don't set a click listener on the positive button yet
            manifestDialogBuilder.setPositiveButton(android.R.string.yes, null);

            final AlertDialog dialog = manifestDialogBuilder.create();
            dialog.show();

            //set the click listener on the positive button like this.
            //This on done to be able to delay the dialog.dismiss() call until the manifest is approved.
            final Button b = dialog.getButton(DialogInterface.BUTTON_POSITIVE);
            b.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    //user clicked yes, claim the device.
                    Log.d(TAG, "Trying to claim : " + appInfo.toString());
                    dialog.setMessage(getActivity().getString(R.string.claiming_dialog_claiming_msg));

                    b.setEnabled(false);
                    dialog.getButton(DialogInterface.BUTTON_NEGATIVE).setEnabled(false);

                    //start the claim action on a background thread as this call will be blocking!
                    new AsyncTask<Void, Void, Void>()
                    {

                        @Override
                        protected Void doInBackground(Void... params)
                        {
                            try {
                                mService.claimApplication(appInfo);
                            }
                            catch (Exception e) {
                                Log.e(TAG, "Error in claiming", e);
                            }
                            return null;
                        }

                        @Override
                        protected void onPostExecute(Void aVoid)
                        {
                            //close dialog
                            dialog.dismiss();
                        }
                    }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
                }
            });
        }
    }

    @Override
    public void onDeviceLongClicked(ApplicationsTracker.AppInfoItem appInfoItem)
    {
        final ApplicationInfo appInfo = appInfoItem.content;

        //somebody clicked on a claimed device.
        //Show confirmation first.
        final AlertDialog.Builder manifestDialogBuilder = new AlertDialog.Builder(getActivity());
        manifestDialogBuilder.setTitle(getActivity().getString(R.string.unclaiming_dialog_title));
        manifestDialogBuilder.setMessage(getActivity().getString(R.string.unclaiming_dialog_msg,
            appInfo.getApplicationName()));
        manifestDialogBuilder.setNegativeButton(android.R.string.no, null);
        manifestDialogBuilder.setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener()
        {
            @Override
            public void onClick(DialogInterface dialog, int which)
            {
                Toast.makeText(getActivity(), getString(R.string.unclaiming_dialog_claiming_msg),
                    Toast.LENGTH_SHORT).show();

                //start the unclaim action on a background thread as this call will be blocking!
                new AsyncTask<Void, Void, Void>()
                {

                    @Override
                    protected Void doInBackground(Void... params)
                    {
                        try {
                            mService.unClaimApplication(appInfo);
                        }
                        catch (Exception e) {
                            Log.e(TAG, "Error in unclaiming", e);
                        }
                        return null;
                    }
                }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
            }
        });

        final AlertDialog dialog = manifestDialogBuilder.create();
        dialog.show();
    }


    /**
     * This interface must be implemented by activities that contain this
     * fragment to allow an interaction in this fragment to be communicated
     * to the activity and potentially other fragments contained in that
     * activity.
     */
    public interface AppsListFragmentCallbacks
    {
        //currently nothing defined
    }

    private class MyManifestApprover implements ManifestApprover, DialogInterface.OnClickListener
    {

        private boolean choice = false;
        private Semaphore sem = new Semaphore(0);

        @Override
        public boolean approveManifest(ApplicationInfo applicationInfo, Manifest manifest)
        {
            //Note: this callback will not be on the UI thread!
            Log.d(TAG, "Manifest Approver - appinfo: " + applicationInfo);
            Log.d(TAG, "Manifest Approver - manifest: " + manifest);

            if (manifest.toString().isEmpty()) {
                return false;
            }

            StringBuilder msg = new StringBuilder();
            msg.append(getActivity().getString(R.string.manifestDialog_msg));
            for (Rule rule : manifest.getRules()) {
                msg.append("\n\n");
                msg.append(rule.getName());
                Log.d(TAG, "Manifest Rule: " + rule.getName());
                for (Member member : rule.getMembers()) {
                    Log.d(TAG, "Manifest Member: " + member.getName() + " -- " + member.getType());
                    msg.append("\n  ").append(member.getName());
                    msg.append(" (");
                    boolean first = true;
                    for (Action action : member.getActions()) {
                        if (!first) {
                            msg.append(", ");
                            first = true;
                        }
                        msg.append(action.toString());
                    }
                    msg.append(")");
                }
            }

            final AlertDialog.Builder manifestDialogBuilder = new AlertDialog.Builder(getActivity());
            manifestDialogBuilder.setTitle(getActivity().getString(R.string.manifestDialog_title));
            manifestDialogBuilder.setCancelable(false);
            manifestDialogBuilder.setMessage(msg.toString());
            manifestDialogBuilder.setNegativeButton(getActivity().getString(R.string.reject), this);
            manifestDialogBuilder.setPositiveButton(getActivity().getString(R.string.accept), this);


            //show the dialog and wait for feedback.
            getActivity().runOnUiThread(new Runnable()
            {
                @Override
                public void run()
                {
                    Log.d(TAG, "Showing manifest dialog");
                    manifestDialogBuilder.show();
                }
            });

            try {
                Log.d(TAG, "Waiting for manifest approval");
                sem.acquire();
            }
            catch (InterruptedException e) {
                Log.e(TAG, "Error waiting for manifest approval", e);
            }

            Log.d(TAG, "Sending manifest approval: " + choice);
            return choice;
        }

        @Override
        public void onClick(DialogInterface dialog, int which)
        {
            choice = (which == DialogInterface.BUTTON_POSITIVE);
            Log.d(TAG, "Manifest approver choice: " + choice);
            sem.release(); //unblock the thread.
        }
    }

    /**
     * Enum that list the possibilities to filter.
     */
    public enum ClaimFilter
    {
        ALL,
        CLAIMED,
        CLAIMABLE,
        NOT_CLAIMED;

        /**
         * Check if the currently ClaimState should be shown by this filter or not.
         * @param state The ClaimState
         * @return True if it should be shown, false otherwise.
         */
        public boolean isAllowed(ClaimState state)
        {
            switch (this) {
                case ALL:
                    return true;
                case CLAIMED:
                    return state == ClaimState.CLAIMED;
                case CLAIMABLE:
                    return state == ClaimState.CLAIMABLE;
                case NOT_CLAIMED:
                    return state == ClaimState.NOT_CLAIMED;
                default:
                    throw new IllegalStateException("Unhandled case: " + this);
            }
        }
    }

}
