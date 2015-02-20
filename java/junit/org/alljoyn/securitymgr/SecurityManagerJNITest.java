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

import java.io.File;
import java.util.List;

import junit.framework.TestCase;

import org.alljoyn.securitymgr.access.Action;
import org.alljoyn.securitymgr.access.Manifest;
import org.alljoyn.securitymgr.access.Member;
import org.alljoyn.securitymgr.access.MemberType;
import org.alljoyn.securitymgr.access.Peer;
import org.alljoyn.securitymgr.access.Policy;
import org.alljoyn.securitymgr.access.Rule;
import org.alljoyn.securitymgr.access.Term;

public class SecurityManagerJNITest extends TestCase {
    private SecurityManagerJNI smngr;
    private File f;

    public void testInit() throws Exception {
        smngr = new SecurityManagerJNI();
        smngr.setManifestApprover(new ManifestApprover() {
            @Override
            public boolean approveManifest(ApplicationInfo appInfo,
                    Manifest manifest) {
                System.out
                .println("SecurityManagerJNITest..approveManifest(" + appInfo + ")");
                System.out.println("   manifest = " + manifest);
                return true;
            }
        });
        smngr.addApplicationEventListener(new ApplicationEventListener() {
            @Override
            public void onApplicationEvent(final ApplicationInfo newInfo,
                    ApplicationInfo oldInfo) {
                System.out
                .println("SecurityManagerJNITest.testInit().onApplicationEvent(NEW)"
                        + newInfo);
                System.out
                .println("SecurityManagerJNITest.testInit()..onApplicationEvent(OLD)"
                        + oldInfo);
                if (newInfo != null
                        && newInfo.getClaimState() == ClaimState.CLAIMABLE) {
                    new Thread(new Runnable() {
                        @Override
                        public void run() {
                            synchronized (SecurityManagerJNITest.this) {

                                try {
                                    List<ApplicationInfo> apps = smngr
                                            .getApplications();
                                    for (ApplicationInfo applicationInfo : apps) {
                                        if (applicationInfo.equals(newInfo)) {
                                            if (applicationInfo.getClaimState() != ClaimState.CLAIMABLE) {
                                                return;
                                            }
                                            break;
                                        }
                                    }

                                    Identity myId = new Identity("myTestName",
                                            new byte[] { '1', '2', '3', '4', '5',
                                            '6', '7', '8', '9', 'A', 'B',
                                            'C', 'D', 'E', 'F', 'G' },
                                            null);
                                    smngr.createIdentity(myId);
                                    smngr.claimApplication(newInfo, myId);
                                    GUID myGuid = new GUID(new byte[] { 6, 1, 2, 3,
                                            4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                                            15 });
                                    Guild myGuild = new Guild("myGuild",
                                            "My own guild", myGuid, smngr
                                            .getPublicKey());

                                    smngr.createGuild(myGuild);

                                    System.out
                                    .println("SecurityManagerJNITest.testInit().new ApplicationEventListener() {...}.onApplicationEvent(...).new Runnable() {...}.run()"
                                            + myGuild);

                                    smngr.installMembership(newInfo, myGuild);
                                    smngr.deleteMembership(newInfo, myGuild);

                                    Policy data = new Policy(1);
                                    Term dataTerm = new Term();
                                    dataTerm.addRule(new Rule("my.test.intf",
                                            new Member("*", Action.fullAccess(),
                                                    MemberType.ANY)));
                                    data.addTerm(dataTerm);
                                    // smngr.installMembership(newInfo, myGuild,
                                    // data);

                                    Policy policy = new Policy(1);
                                    Term term = new Term();
                                    term.addPeer(new Peer(myGuild));
                                    term.addRule(new Rule("my.test.intf",
                                            new Member("*", Action.fullAccess(),
                                                    MemberType.ANY)));
                                    policy.addTerm(term);
                                    smngr.installPolicy(newInfo, policy);
                                    System.out
                                    .println("SecurityManagerJNITest.testInit(policy installed)");
                                } catch (SecurityMngtException e) {
                                    e.printStackTrace();
                                }
                            }
                        }
                    }).start();
                }
            }
        });
        assertNotNull(smngr);
        try {
            smngr.getApplications();
            fail("Not initialized");
        } catch (Exception sme) {
            //OK - not initialized
        }
        assertFalse(smngr.init(null));
        assertTrue(smngr.init(f.getAbsolutePath()));
        List<ApplicationInfo> apps = smngr.getApplications();
        assertNotNull(apps);
        System.out.println("SecurityManagerJNITest.testInit()" + apps);
        assertEquals(0, apps.size());

        // Basic Identities
        Identity myId = new Identity("myID", new GUID(new byte[16]),
                smngr.getPublicKey());
        assertEquals(0, smngr.getIdentities().size());
        smngr.createIdentity(myId);
        List<Identity> ids = smngr.getIdentities();
        assertEquals(1, ids.size());
        assertEquals(myId, ids.get(0));
        assertEquals(myId, smngr.getIdentity(myId.getGuid()));
        assertEquals(myId, smngr.getIdentity(myId));
        String newName = "MyNewName";
        myId.setName(newName);
        smngr.updateIdentity(myId);
        assertEquals(newName, smngr.getIdentity(myId).getName());
        smngr.deleteIdentity(myId);
        assertEquals(0, smngr.getIdentities().size());
        smngr.createIdentity(myId);

        try {
            smngr.claimApplication(null, myId);
            fail("Null not allowed");
        } catch (NullPointerException npe) {
            //OK.
        }
        try {
            smngr.claimApplication(new ApplicationInfo(), myId);
            fail("illegal argument");
        } catch (IllegalArgumentException iae) {
            //OK.
        }

        //Basic Guilds
        List<Guild> guilds = smngr.getGuilds();
        assertNotNull(guilds);
        assertEquals(0, guilds.size());


        GUID myGuid = new GUID(new byte[] { 5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                11, 12, 13, 14, 15 });
        Guild myGuild = new Guild("myGuild", "My own guild", myGuid,
                smngr.getPublicKey());

        smngr.createGuild(myGuild);
        guilds = smngr.getGuilds();
        assertNotNull(guilds);
        assertEquals(1, guilds.size());
        assertTrue(guilds.contains(myGuild));
        assertEquals(myGuild, smngr.getGuild(myGuild));
        assertEquals(myGuild, smngr.getGuild(myGuid));

        String newGuildName = "MyNewGuild";
        String newDesc = "My new description.";
        myGuild.setName(newGuildName);
        myGuild.setDescription(newDesc);
        smngr.updateGuild(myGuild);
        assertEquals(newGuildName, smngr.getGuild(myGuild).getName());
        assertEquals(newDesc, smngr.getGuild(myGuid).getDescription());
        smngr.deleteGuild(myGuild);
        try {
            smngr.getGuild(myGuid);
            fail("Should throw exception");
        } catch (SecurityMngtException sme) {
            // OK.
        }
        guilds = smngr.getGuilds();
        assertNotNull(guilds);
        assertEquals(0, guilds.size());
        smngr.createGuild(myGuild);
        guilds = smngr.getGuilds();
        assertNotNull(guilds);
        assertEquals(1, guilds.size());
        assertTrue(guilds.contains(myGuild));

        smngr.deleteGuild(myGuild);
        smngr.deleteIdentity(myId);
    }

    /*
     * @see junit.framework.TestCase#tearDown()
     */
    @Override
    protected void tearDown() throws Exception {
        if (f != null) {
            for (File file : f.listFiles()) {
                file.delete();
            }
            f.delete();
        }
        super.tearDown();
    }

    /*
     * @see junit.framework.TestCase#setUp()
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        f = File.createTempFile("Junit", "JNI");
        f.delete();
        f.mkdirs();
        f.deleteOnExit();
    }
}
