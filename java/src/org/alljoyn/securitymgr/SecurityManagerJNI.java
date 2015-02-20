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

import java.util.ArrayList;
import java.util.List;

import org.alljoyn.securitymgr.access.Manifest;
import org.alljoyn.securitymgr.access.Member;
import org.alljoyn.securitymgr.access.Policy;
import org.alljoyn.securitymgr.access.Rule;
import org.alljoyn.securitymgr.access.Term;

public class SecurityManagerJNI extends SecurityManager {

    //This field will be set and used only by the JNI layer.
    private final long pointer = 0;

    private native static void initJNI(Class<ApplicationInfo> infoClazz,
            Class<Rule> ruleClazz, Class<Member> memberClazz);

    static {
        System.loadLibrary("ajsecmgr_java");
        initJNI(ApplicationInfo.class, Rule.class, Member.class);
    }

    void onNewApplicationEvent(ApplicationInfo newInfo, ApplicationInfo oldInfo) {
        dispatchEvent(newInfo, oldInfo);
    }

    boolean approveManifest(ApplicationInfo info, Rule[] manifestRules) {
        ManifestApprover ma = approver;
        if (ma == null) {
            return true;
        }
        ArrayList<Rule> rules = new ArrayList<Rule>(manifestRules.length);
        for (Rule rule : manifestRules) {
            rules.add(rule);
        }
        return ma.approveManifest(info, new Manifest(rules));
    }

    @Override
    public List<ApplicationInfo> getApplications() throws SecurityMngtException {
        List<ApplicationInfo> list = new ArrayList<ApplicationInfo>();
        getApplications(list);
        return list;
    }

    @Override
    public void createGuild(Guild guild) throws SecurityMngtException {
        createGuild(guild.getName(), guild.getDescription(),
                guild.getGuid().guid, guild.key);
    }


    @Override
    public List<Guild> getGuilds() throws SecurityMngtException {
        List<Guild> list = new ArrayList<Guild>();
        getGuilds(list, Guild.class);
        return list;
    }

    @Override
    public Guild getGuild(GUID guid) throws SecurityMngtException {
        return getGuild(guid.guid, Guild.class);
    }

    @Override
    public void updateGuild(Guild guild) throws SecurityMngtException {
        createGuild(guild.getName(), guild.getDescription(),
                guild.getGuid().guid, guild.key);
    }

    @Override
    public void deleteGuild(Guild guild) throws SecurityMngtException {
        deleteGuild(guild.getGuid().guid);
    }

    @Override
    public void claimApplication(ApplicationInfo app, Identity user)
            throws SecurityMngtException {
        claimApplication(app, user.getGuid().guid, user.key);
    }

    @Override
    public void createIdentity(Identity identity) throws SecurityMngtException {
        createIdentity(identity.getName(), identity.getGuid().guid,
                identity.getKey());
    }

    @Override
    public Identity getIdentity(GUID guid) throws SecurityMngtException {
        return new Identity(getIdentity(guid.guid), guid, getPublicKey());
    }

    @Override
    public List<Identity> getIdentities() throws SecurityMngtException {
        List<Identity> list = new ArrayList<Identity>();
        getIdentities(list, Identity.class);
        return list;
    }

    @Override
    public void updateIdentity(Identity id) throws SecurityMngtException {
        createIdentity(id.getName(), id.getGuid().guid, id.key);
    }

    @Override
    public void deleteIdentity(Identity identity) throws SecurityMngtException {
        deleteIdentity(identity.getGuid().guid);
    }

    @Override
    public void installMembership(ApplicationInfo app, Guild guild,
            Policy policy) throws SecurityMngtException {
        installMembership(
                app,
                guild.getGuid().guid,
                guild.key,
                (policy != null ? policy
                        .getTerms().toArray(new Term[0]) : null));
    }

    @Override
    public void deleteMembership(ApplicationInfo app, Guild guild)
            throws SecurityMngtException {
        deleteMembership(app, guild.getGuid().guid, guild.key);
    }

    @Override
    public void installPolicy(ApplicationInfo app, Policy policy)
            throws SecurityMngtException {
        installPolicy(app, policy.getSerialNr(),
                policy.getTerms().toArray(new Term[0]));

    }
    @Override
    public native byte[] getPublicKey() throws SecurityMngtException;

    public native boolean init(String path) throws SecurityMngtException;

    @Override
    public native void unclaimApplication(ApplicationInfo app) throws SecurityMngtException;

    private native void getApplications(List<ApplicationInfo> list)
            throws SecurityMngtException;

    private native void createGuild(String name, String description,
            byte[] guid, byte[] ownerKey)
                    throws SecurityMngtException;

    private native void deleteGuild(byte[] guid) throws SecurityMngtException;

    private native Guild getGuild(byte[] guid, Class<Guild> clazz)
            throws SecurityMngtException;

    private native void getGuilds(List<Guild> list, Class<Guild> clazz)
            throws SecurityMngtException;

    private native void createIdentity(String name, byte[] guid, byte[] key)
            throws SecurityMngtException;

    private native void deleteIdentity(byte[] guid)
            throws SecurityMngtException;

    private native String getIdentity(byte[] guid) throws SecurityMngtException;

    private native void getIdentities(List<Identity> list, Class<Identity> clazz)
            throws SecurityMngtException;

    private native void installPolicy(ApplicationInfo app, long serialNr,
            Term[] array) throws SecurityMngtException;

    private native void installMembership(ApplicationInfo app, byte[] guid,
            byte[] ownerKey, Term[] array) throws SecurityMngtException;

    private native void deleteMembership(ApplicationInfo app, byte[] guid,
            byte[] ownerKey)
                    throws SecurityMngtException;

    private native void claimApplication(ApplicationInfo app, byte[] guid,
            byte[] key)
                    throws SecurityMngtException;

}
