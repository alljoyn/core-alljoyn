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

import java.util.Collections;
import java.util.List;

import org.alljoyn.securitymgr.access.Manifest;
import org.alljoyn.securitymgr.access.Peer;
import org.alljoyn.securitymgr.access.Policy;
import org.alljoyn.securitymgr.access.Rule;
import org.alljoyn.securitymgr.access.Term;

/**
 * The DefaultSecurityHandler is a utility class wrapping around a
 * SecurityManager. It takes care of identity, guild, policy and memberships. It
 * will create a single identity, a single guild. Every application which is
 * claimed via this DefaultSecurityHandler will act in name of the single
 * identity. Will receive a membership certificate for the guild along with a
 * policy for the guild where everything is allowed. Access control is done
 * purely based on {@link Manifest} enforcing.
 */
public class DefaultSecurityHandler {

    private final SecurityManager smgr;
    private Guild theGuild;
    private Identity theIdenity;

    public DefaultSecurityHandler(ApplicationEventListener listener,
            ManifestApprover manifestApprover)
                    throws SecurityMngtException {

        smgr = SecurityManager.getSecurityManager();
        init(listener, manifestApprover);
    }

    public DefaultSecurityHandler(ApplicationEventListener listener,
            ManifestApprover manifestApprover, String path) throws SecurityMngtException {

        smgr = SecurityManager.getSecurityManager(path);
        init(listener, manifestApprover);
    }
    public List<ApplicationInfo> getApplications() {
        try {
            return smgr.getApplications();
        } catch (SecurityMngtException e) {
            return Collections.emptyList();
        }
    }

    /**
     * Claims and configure the specified application
     *
     * @param info a non-null ApplicationInfo
     * @throws SecurityMngtException when the claiming or configuration fails
     */
    public void claimApplication(ApplicationInfo info) throws SecurityMngtException {
        smgr.claimApplication(info, theIdenity);
        smgr.installMembership(info, theGuild);
        Policy policy = new Policy(1);
        Term term = new Term();
        term.addPeer(new Peer(theGuild));
        term.addRule(new Rule("*"));
        policy.addTerm(term);
        smgr.installPolicy(info, policy);
    }

    /**
     * Unclaims the specified application.
     * @param info a non-null ApplicationInfo
     * @throws SecurityMngtException when the unclaiming fails
     */
    public void unclaimApplication(ApplicationInfo info) throws SecurityMngtException {
        smgr.unclaimApplication(info);
    }

    private void init(ApplicationEventListener listener,
            ManifestApprover manifestApprover) throws SecurityMngtException {
        if (listener == null || manifestApprover == null) {
            throw new IllegalArgumentException("Null Argument(s) provided");
        }
        smgr.setManifestApprover(manifestApprover);
        smgr.addApplicationEventListener(listener);

        byte[] guidData = new byte[16];
        guidData[15] = 1;
        GUID guid = new GUID(guidData);
        try {
            theGuild = smgr.getGuild(guid);
        } catch (SecurityMngtException sme) {
            // Guild does not exist.
            theGuild = new Guild("defaultGuild",
                    "The default GUild of this Security manager", guidData,
                    smgr.getPublicKey());
            smgr.createGuild(theGuild);
        }
        try {
            theIdenity = smgr.getIdentity(guid);
        } catch (SecurityMngtException sme) {
            // Identity does not exist.
            theIdenity = new Identity("Admin", guidData, smgr.getPublicKey());
            smgr.createIdentity(theIdenity);
        }
    }
}
