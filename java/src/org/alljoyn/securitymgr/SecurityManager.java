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

package org.alljoyn.securitymgr;

import org.alljoyn.securitymgr.access.Policy;
import java.util.ArrayList;
import java.util.List;

public abstract class SecurityManager {

    public static SecurityManager getSecurityManager(String path) {
        SecurityManagerJNI smJNI = new SecurityManagerJNI();
        boolean ret = false;
        try {
            ret = smJNI.init(path);
        } catch (SecurityMngtException e) {
            e.printStackTrace();
        }

        if(ret) {
            return smJNI;
        }

        return null;
    }

    public static SecurityManager getSecurityManager() {
        return new SecurityManagerJNI();
    }

    private List<ApplicationEventListener> listeners = new ArrayList<ApplicationEventListener>();

    protected ManifestApprover approver;

    SecurityManager() {
    }

    public void addApplicationEventListener(ApplicationEventListener l) {
        listeners.add(l);
    }

    public void removeApplicationEventListiner(ApplicationEventListener l) {
        listeners.remove(l);
    }

    public void setManifestApprover(ManifestApprover mnfApprover) {
        approver = mnfApprover;
    }

    public Guild getGuild(Guild guild) throws SecurityMngtException {
        return getGuild(guild.getGuid());
    }

    public Identity getIdentity(Identity id) throws SecurityMngtException {
        return getIdentity(id.getGuid());
    }

    public void installMembership(ApplicationInfo app, Guild guild)
            throws SecurityMngtException {
        installMembership(app, guild, null);
    }

    public abstract void installMembership(ApplicationInfo app, Guild guild,
            Policy policy) throws SecurityMngtException;

    public abstract void deleteMembership(ApplicationInfo app, Guild guild)
            throws SecurityMngtException;

    public abstract void installPolicy(ApplicationInfo app, Policy policy)
            throws SecurityMngtException;

    public abstract List<ApplicationInfo> getApplications()
            throws SecurityMngtException;

    public abstract void claimApplication(ApplicationInfo app, Identity user)
            throws SecurityMngtException;

    public abstract void unclaimApplication(ApplicationInfo app)
            throws SecurityMngtException;

    public abstract void createGuild(Guild guild)
            throws SecurityMngtException;

    public abstract List<Guild> getGuilds() throws SecurityMngtException;

    public abstract Guild getGuild(GUID guid) throws SecurityMngtException;

    public abstract void updateGuild(Guild guild) throws SecurityMngtException;

    public abstract void deleteGuild(Guild guild) throws SecurityMngtException;

    public abstract void createIdentity(Identity identity)
            throws SecurityMngtException;

    public abstract List<Identity> getIdentities() throws SecurityMngtException;

    public abstract Identity getIdentity(GUID guid)
            throws SecurityMngtException;

    public abstract void updateIdentity(Identity id)
            throws SecurityMngtException;

    public abstract void deleteIdentity(Identity identity)
            throws SecurityMngtException;

    void dispatchEvent(ApplicationInfo newInfo, ApplicationInfo oldInfo) {
        for (ApplicationEventListener applicationEventListener : listeners) {
            applicationEventListener.onApplicationEvent(newInfo == null ? null : newInfo.clone(), oldInfo == null ? null : oldInfo.clone());
        }
    }

    public abstract byte[] getPublicKey() throws SecurityMngtException;

}
