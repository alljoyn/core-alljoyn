/******************************************************************************
 * Copyright (c) AllSeen Alliance. All rights reserved.
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

package org.alljoyn.securitymgr;

import java.util.Arrays;

public class ApplicationInfo implements Cloneable {

    private String userFriendlyName;
    private String deviceName;
    private String applicationName;
    private String applicationId;

    private RunningState runningState;
    private ClaimState claimState;
    /* Used by the JNI layer only for now */
    private byte[] publicKey;

    /**
     * Creates an ApplicationInfo with claim and running state set. Called from
     * JNI.
     *
     * @param isRunning
     *            the running state.
     * @param isClaimed
     *            the claim state.
     */
    ApplicationInfo(int isRunning, int isClaimed) {
        RunningState[] rss = RunningState.values();
        runningState = (isRunning < 0 || isRunning >= rss.length) ? RunningState.UNKNOWN
                : rss[isRunning];
        ClaimState[] css = ClaimState.values();
        claimState = (isClaimed < 0 || isClaimed >= css.length) ? ClaimState.UNKNOWN
                : css[isClaimed];
    }

    public ApplicationInfo() {
    }

    public String getUserFriendlyName() {
        return userFriendlyName;
    }

    public void setUserFriendlyName(String userFriendlyName) {
        this.userFriendlyName = userFriendlyName;
    }

    public String getDeviceName() {
        return deviceName;
    }

    public void setDeviceName(String deviceName) {
        this.deviceName = deviceName;
    }

    public String getApplicationName() {
        return applicationName;
    }

    public void setApplicationName(String applicationName) {
        this.applicationName = applicationName;
    }

    public String getApplicationId() {
        return applicationId;
    }

    public void setApplicationId(String applicationId) {
        this.applicationId = applicationId;
    }

    public RunningState getRunningState() {
        return runningState;
    }

    public void setRunningState(RunningState runningState) {
        this.runningState = runningState;
    }

    public ClaimState getClaimState() {
        return claimState;
    }

    public void setClaimState(ClaimState claimState) {
        this.claimState = claimState;
    }

    public byte[] getPublicKey() {
        return publicKey.clone();
    }

    @Override
    public ApplicationInfo clone() {
        try {
            ApplicationInfo clonedInfo = (ApplicationInfo) super.clone();
            clonedInfo.publicKey = publicKey.clone();
            return clonedInfo;
        } catch (CloneNotSupportedException cnse) {
            throw new IllegalArgumentException(cnse);
        }
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result
                + ((applicationId == null) ? 0 : applicationId.hashCode());
        result = prime * result + Arrays.hashCode(publicKey);
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        ApplicationInfo other = (ApplicationInfo) obj;
        if (applicationId == null) {
            if (other.applicationId != null)
                return false;
        } else if (!applicationId.equals(other.applicationId))
            return false;
        if (!Arrays.equals(publicKey, other.publicKey))
            return false;
        return true;
    }

    @Override
    public String toString() {
        return "ApplicationInfo [userFriendlyName=" + userFriendlyName
                + ", deviceName=" + deviceName + ", applicationName="
                + applicationName + ", applicationId=" + applicationId
                + ", runningState=" + runningState + ", claimState="
                + claimState + "]";
    }
}
