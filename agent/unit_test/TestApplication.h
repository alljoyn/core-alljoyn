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

#include <string>
#include <alljoyn/BusAttachment.h>

using namespace std;
using namespace ajn;

namespace secmgr_tests {
class TestApplication {
  public:
    /**
     * Creates a new TestApplication.
     */
    TestApplication(string _appName = "Test");

    /**
     * Starts this TestApplication.
     */
    QStatus Start(bool setDefaultManifest = true);

    /**
     * Retrieves the default manifest of this TestApplication
     */
    void GetManifest(PermissionPolicy::Rule** rules,
                     size_t& count);

    /**
     * Sets the default manifest of this TestApplication.
     */
    QStatus SetManifest();

    /**
     * Sets the application state as permitted by PermissionConfigurator
     */
    QStatus SetApplicationState(const PermissionConfigurator::ApplicationState state);

    /**
     * Resets the keystore of this TestApplication.
     */
    void Reset();

    /*
     * Stops this TestApplication.
     */
    QStatus Stop();

    /*
     * Destructor for TestApplication.
     */
    ~TestApplication();

  private:
    BusAttachment* busAttachment;
    DefaultECDHEAuthListener authListener;
    string appName;
};
} // namespace
