/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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

#include <qcc/platform.h>
#include <gtest/gtest.h>
#include <qcc/GUID.h>
#include <qcc/StringUtil.h>

#include <alljoyn/KeyStoreListener.h>
#include <alljoyn/Status.h>
#include <alljoyn/Message.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/BusObject.h>
#include <alljoyn/ProxyBusObject.h>
#include <alljoyn/InterfaceDescription.h>
#include <qcc/Log.h>
#include "KeyStore.h"
#include "InMemoryKeyStore.h"

using namespace ajn;
using namespace qcc;

static const char* ONOFF_IFC_NAME = "org.allseenalliance.control.OnOff";

class AuthListenerECDHETest : public BusObject, public testing::Test {
  public:

    class ECDHEKeyXListener : public AuthListener {
      public:
        ECDHEKeyXListener(bool server) :
            sendKeys(true),
            sendExpiry(true),
            expirationSeconds(100u), /* set the master secret expiry time to 100 seconds */
            sendPrivateKey(true),
            sendCertChain(true),
            sendEmptyCertChain(false),
            sendExpiredChain(false),
            sendInvalidChain(false),
            sendInvalidChainNoCA(false),
            failVerifyCertChain(false),
            useCAPICerts(false),
            authComplete(false),
            pskName("<anonymous>"),
            /*
             * In this example, the pre shared secret is a hard coded string.
             * Pre-shared keys should be 128 bits long, and generated with a
             * cryptographically secure random number generator.
             */
            psk("faaa0af3dd3f1e0379da046a3ab6ca44"),
            server(server)
        {
            QCC_UseOSLogging(true);
        }

        bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds)
        {
            QCC_UNUSED(authPeer);
            QCC_UNUSED(authCount);
            QCC_UNUSED(userId);

            if (strcmp(authMechanism, "ALLJOYN_ECDHE_NULL") == 0) {
                if (!sendKeys) {
                    return false;
                }
                if (sendExpiry) {
                    creds.SetExpiration(expirationSeconds);
                }
                return true;
            } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_PSK") == 0) {
                /*
                 * Solicit the Pre shared secret
                 * Based on the pre shared secret id, the application can retrieve
                 * the pre shared secret from storage or from the end user.
                 * In this example, the pre shared secret is a hard coded string
                 */
                if (!sendKeys) {
                    return false;
                }
                if ((credMask& AuthListener::CRED_USER_NAME) == AuthListener::CRED_USER_NAME) {
                    if (pskName != creds.GetUserName()) {
                        return false;
                    }
                }
                if (pskName != "<anonymous>") {
                    creds.SetUserName(pskName);
                }
                creds.SetPassword(psk);
                if (sendExpiry) {
                    creds.SetExpiration(expirationSeconds);
                }
                return true;
            } else if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {

                const char* serverPrivateKeyPEM;
                const char* serverCertChainX509PEM;
                const char* clientPrivateKeyPEM;
                const char* clientCertChainX509PEM;

                if (useCAPICerts == false) {
                    /* the server key and certificate are generated the unit test common/unit_test/CertificateECCTest::GenSelfSignECCX509CertForBBservice */
                    serverPrivateKeyPEM =
                        "-----BEGIN EC PRIVATE KEY-----\n"
                        "MDECAQEEICCRJMbxSiWUqj4Zs7jFQRXDJdBRPWX6fIVqE1BaXd08oAoGCCqGSM49\n"
                        "AwEH\n"
                        "-----END EC PRIVATE KEY-----";

                    serverCertChainX509PEM =
                        "-----BEGIN CERTIFICATE-----\n"
                        "MIIBuDCCAV2gAwIBAgIHMTAxMDEwMTAKBggqhkjOPQQDAjBCMRUwEwYDVQQLDAxv\n"
                        "cmdhbml6YXRpb24xKTAnBgNVBAMMIDgxM2FkZDFmMWNiOTljZTk2ZmY5MTVmNTVk\n"
                        "MzQ4MjA2MB4XDTE1MDcyMjIxMDYxNFoXDTE2MDcyMTIxMDYxNFowQjEVMBMGA1UE\n"
                        "CwwMb3JnYW5pemF0aW9uMSkwJwYDVQQDDCAzOWIxZGNmMjBmZDJlNTNiZGYzMDU3\n"
                        "NzMzMjBlY2RjMzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABGJ/9F4xHn3Klw7z\n"
                        "6LREmHJgzu8yJ4i09b4EWX6a5MgUpQoGKJcjWgYGWb86bzbciMCFpmKzfZ42Hg+k\n"
                        "BJs2ZWajPjA8MAwGA1UdEwQFMAMBAf8wFQYDVR0lBA4wDAYKKwYBBAGC3nwBATAV\n"
                        "BgNVHSMEDjAMoAoECELxjRK/fVhaMAoGCCqGSM49BAMCA0kAMEYCIQDixoulcO7S\n"
                        "df6Iz6lvt2CDy0sjt/bfuYVW3GeMLNK1LAIhALNklms9SP8ZmTkhCKdpC+/fuwn0\n"
                        "+7RX8CMop11eWCih\n"
                        "-----END CERTIFICATE-----";

                    /* the client key and certificate are generated using openssl */

                    clientPrivateKeyPEM =
                        "-----BEGIN EC PRIVATE KEY-----\n"
                        "MHcCAQEEIAzfibK85el6fvczuL5vIaKBiZ5hTTaNIo0LEkvJ2dCMoAoGCCqGSM49\n"
                        "AwEHoUQDQgAE3KsljHhEdm5JLdpRr0g1zw9EMmMqcQJdxYoMr8AAF//G8fujudM9\n"
                        "HMlXLcyBk195YnGp+hY8Tk+QNNA3ZVNavw==\n"
                        "-----END EC PRIVATE KEY-----";

                    clientCertChainX509PEM =
                        "-----BEGIN CERTIFICATE-----\n"
                        "MIIBYTCCAQigAwIBAgIJAKdvmRDLDVWQMAoGCCqGSM49BAMCMCQxIjAgBgNVBAoM\n"
                        "GUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwNzIyMjAxMTA3WhcNMTUw\n"
                        "ODIxMjAxMTA3WjAgMR4wHAYDVQQKDBVBbGxKb3luVGVzdENsaWVudE5hbWUwWTAT\n"
                        "BgcqhkjOPQIBBggqhkjOPQMBBwNCAATcqyWMeER2bkkt2lGvSDXPD0QyYypxAl3F\n"
                        "igyvwAAX/8bx+6O50z0cyVctzIGTX3lican6FjxOT5A00DdlU1q/oycwJTAVBgNV\n"
                        "HSUEDjAMBgorBgEEAYLefAEBMAwGA1UdEwEB/wQCMAAwCgYIKoZIzj0EAwIDRwAw\n"
                        "RAIgQsvHZ747URkPCpYtBxi56V1OcMF3oKWnGuz2jazWr4YCICCU5/itaYVt1SzQ\n"
                        "cBYyChWx/4KXL4QKWLdm9/6ispdq\n"
                        "-----END CERTIFICATE-----\n"
                        "\n"
                        "-----BEGIN CERTIFICATE-----\n"
                        "MIIBdDCCARugAwIBAgIJANOdlTtGQiNsMAoGCCqGSM49BAMCMCQxIjAgBgNVBAoM\n"
                        "GUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwNzIyMjAxMTA2WhcNMjkw\n"
                        "MzMwMjAxMTA2WjAkMSIwIAYDVQQKDBlBbGxKb3luVGVzdFNlbGZTaWduZWROYW1l\n"
                        "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEfN5/iDyZAHt9zLEvR2/y02jVovfW\n"
                        "U+lxLtDe0I+fTOoZn3WMd3EyZWKKdfela66adLWwzijKpBlXpj5KKQn5vKM2MDQw\n"
                        "IQYDVR0lBBowGAYKKwYBBAGC3nwBAQYKKwYBBAGC3nwBBTAPBgNVHRMBAf8EBTAD\n"
                        "AQH/MAoGCCqGSM49BAMCA0cAMEQCIDT7r6txazffbFN8VxPg3tRuyWvtTNwYiS2y\n"
                        "tn0H/nsaAiBzKmTHjrmhSLmYidtNvcU/OjKzmRHmdGTaURz0s2NBcQ==\n"
                        "-----END CERTIFICATE-----\n";
                } else {
                    /*
                     * Use server and client certificates generated with the Windows
                     * Cryptography APIs (CAPI2 and CNG).
                     * See alljoyn_core\test\scripts\CAPI_Test_Cert_Generation.cmd and
                     * pfx2pem.cmd.
                     */
                    serverPrivateKeyPEM =
                        "-----BEGIN EC PRIVATE KEY-----\n"
                        "MHcCAQEEICZ4ITrSMWazoGPBCjxcv7goxSGvfOWAgSsebNq822JPoAoGCCqGSM49\n"
                        "AwEHoUQDQgAEp+fTvMjkcXZsE6h6i3JtBiO5N5kiOjXvGMHPhmPH70YLB3lw7tZK\n"
                        "E2im95nQWWOno9g5sdJOhZ1jftT1ksXKng==\n"
                        "-----END EC PRIVATE KEY-----";

                    serverCertChainX509PEM =
                        "-----BEGIN CERTIFICATE-----"
                        "MIIBhjCCASygAwIBAgIQXiWpM8rMappGXsKSElFtNzAKBggqhkjOPQQDAjAgMR4w\n"
                        "HAYDVQQDDBVBbGxKb3luVGVzdFNlcnZlck5hbWUwHhcNMTUwNzI4MTkzNjA4WhcN\n"
                        "MjkwNDA1MTk0NjA4WjAgMR4wHAYDVQQDDBVBbGxKb3luVGVzdFNlcnZlck5hbWUw\n"
                        "WTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAASn59O8yORxdmwTqHqLcm0GI7k3mSI6\n"
                        "Ne8Ywc+GY8fvRgsHeXDu1koTaKb3mdBZY6ej2Dmx0k6FnWN+1PWSxcqeo0gwRjAO\n"
                        "BgNVHQ8BAf8EBAMCB4AwFQYDVR0lBA4wDAYKKwYBBAGC3nwBATAdBgNVHQ4EFgQU\n"
                        "vgKPPyMDzPUyW+t+q2krMQm5UUwwCgYIKoZIzj0EAwIDSAAwRQIhAKwlQpn74JbA\n"
                        "rYfeZcXyuPkXh733BxYprQlEkRbnYJzrAiBuzBFeuc812oCBUGSdHbp4kAJSEtAc\n"
                        "Cj7u3Pnwi1XLpg==\n"
                        "-----END CERTIFICATE-----";

                    clientPrivateKeyPEM =
                        "-----BEGIN EC PRIVATE KEY-----\n"
                        "MHcCAQEEIMEI3eVg2wJu9vteb8HA9rPZWznDpYy9daaDt5MFoAAeoAoGCCqGSM49\n"
                        "AwEHoUQDQgAEI63hx/KJK4CB4hexkkliN8NLWypIHd1sy8qcJgzGgHGlLzgTiPL1\n"
                        "V8vI7NQj3tL1oeuhlAuCgp6gZrYG+qbZIQ==\n"
                        "-----END EC PRIVATE KEY-----";

                    clientCertChainX509PEM =
                        "-----BEGIN CERTIFICATE-----\n"
                        "MIIBrDCCAVGgAwIBAgIQNwCHFovV/4FOTrJFpn6vKTAKBggqhkjOPQQDAjAkMSIw\n"
                        "IAYDVQQDDBlBbGxKb3luVGVzdFNlbGZTaWduZWROYW1lMB4XDTE1MDcyODE5Mzcw\n"
                        "NVoXDTI5MDQwNTE5NDcwNVowIDEeMBwGA1UEAwwVQWxsSm95blRlc3RDbGllbnRO\n"
                        "YW1lMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEI63hx/KJK4CB4hexkkliN8NL\n"
                        "WypIHd1sy8qcJgzGgHGlLzgTiPL1V8vI7NQj3tL1oeuhlAuCgp6gZrYG+qbZIaNp\n"
                        "MGcwDgYDVR0PAQH/BAQDAgeAMBUGA1UdJQQOMAwGCisGAQQBgt58AQEwHwYDVR0j\n"
                        "BBgwFoAUg4ZsZU12ghreVKZ0u2eyYHkiqeUwHQYDVR0OBBYEFFKYyjampcsD5WSv\n"
                        "+6EmxXIhXlOHMAoGCCqGSM49BAMCA0kAMEYCIQC8+DwrriZ4P/Mj+O4WUsIBiXNx\n"
                        "0Elf/P5YsoTnT3ycPgIhAMmIBBZTE+Xg7Hnpc8paraTlvvdBlsvyrYir9JalL0jy\n"
                        "-----END CERTIFICATE-----\n"
                        "\n"
                        "-----BEGIN CERTIFICATE-----\n"
                        "MIIBqzCCAVGgAwIBAgIQXlRHtjYelopJmcl8ZqvOXzAKBggqhkjOPQQDAjAkMSIw\n"
                        "IAYDVQQDDBlBbGxKb3luVGVzdFNlbGZTaWduZWROYW1lMB4XDTE1MDcyODE5MzYw\n"
                        "N1oXDTI5MDQwNTE5NDYwN1owJDEiMCAGA1UEAwwZQWxsSm95blRlc3RTZWxmU2ln\n"
                        "bmVkTmFtZTBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABFd3KMaxDMpXEkunL/my\n"
                        "LKou5xLNIJdSoMatDjC0Z8YMo9Ipk6hyIRoomZ1y0NvzPuvxW/HSiHuk91aTsQAS\n"
                        "iYijZTBjMA4GA1UdDwEB/wQEAwIBhjAhBgNVHSUEGjAYBgorBgEEAYLefAEBBgor\n"
                        "BgEEAYLefAEFMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFIOGbGVNdoIa3lSm\n"
                        "dLtnsmB5IqnlMAoGCCqGSM49BAMCA0gAMEUCIQDs7425v9snEPXNDQpP6JDWY0E4\n"
                        "fH4qVDQmeI0hisytZgIgINH4bxNiXXEL7V9cHOnm3gYdVTNpArbQvRjhwCuhX+M=\n"
                        "-----END CERTIFICATE-----\n";
                }

                /* There is one set of invalid test certs, for both values of useCAPICerts */
                static const char pkWithInvalidChain[] =
                    "-----BEGIN EC PRIVATE KEY-----\n"
                    "MHcCAQEEIAzfibK85el6fvczuL5vIaKBiZ5hTTaNIo0LEkvJ2dCMoAoGCCqGSM49\n"
                    "AwEHoUQDQgAE3KsljHhEdm5JLdpRr0g1zw9EMmMqcQJdxYoMr8AAF//G8fujudM9\n"
                    "HMlXLcyBk195YnGp+hY8Tk+QNNA3ZVNavw==\n"
                    "-----END EC PRIVATE KEY-----";

                static const char invalidChainCert2HasCA[] =
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBYTCCAQigAwIBAgIJAKdvmRDLDVWQMAoGCCqGSM49BAMCMCQxIjAgBgNVBAoM\n"
                    "GUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwNzIyMjAxMTA3WhcNMTUw\n"
                    "ODIxMjAxMTA3WjAgMR4wHAYDVQQKDBVBbGxKb3luVGVzdENsaWVudE5hbWUwWTAT\n"
                    "BgcqhkjOPQIBBggqhkjOPQMBBwNCAATcqyWMeER2bkkt2lGvSDXPD0QyYypxAl3F\n"
                    "igyvwAAX/8bx+6O50z0cyVctzIGTX3lican6FjxOT5A00DdlU1q/oycwJTAVBgNV\n"
                    "HSUEDjAMBgorBgEEAYLefAEBMAwGA1UdEwEB/wQCMAAwCgYIKoZIzj0EAwIDRwAw\n"
                    "RAIgQsvHZ747URkPCpYtBxi56V1OcMF3oKWnGuz2jazWr4YCICCU5/itaYVt1SzQ\n"
                    "cBYyChWx/4KXL4QKWLdm9/6ispdq\n"
                    "-----END CERTIFICATE-----\n"
                    "\n"
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBszCCAVmgAwIBAgIJALDTHYnf6i6VMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
                    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAzMDQxNzA2MjBaFw0x\n"
                    "NjAzMDMxNzA2MjBaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n"
                    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n"
                    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABANqoKVY9sET8YCj4gjTeYql\n"
                    "GXwLEK4I2aI0SxHZVNj+SQdGltEpnPRHO4jd/tGMnNpwGx0O6acOLrLGH/RIc3Cj\n"
                    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgPY25+ozlDxgXVJ6T\n"
                    "Uh/vcIUonFt3pqqKtIe99Sc8AdMCIQC8VrFHBFp38e6UkY+Azuikrqi8tXDz8cr3\n"
                    "noKTwIxMpw==\n"
                    "-----END CERTIFICATE-----\n";

                static const char invalidChainCert2HasNoCA[] =
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBYTCCAQigAwIBAgIJAKdvmRDLDVWQMAoGCCqGSM49BAMCMCQxIjAgBgNVBAoM\n"
                    "GUFsbEpveW5UZXN0U2VsZlNpZ25lZE5hbWUwHhcNMTUwNzIyMjAxMTA3WhcNMTUw\n"
                    "ODIxMjAxMTA3WjAgMR4wHAYDVQQKDBVBbGxKb3luVGVzdENsaWVudE5hbWUwWTAT\n"
                    "BgcqhkjOPQIBBggqhkjOPQMBBwNCAATcqyWMeER2bkkt2lGvSDXPD0QyYypxAl3F\n"
                    "igyvwAAX/8bx+6O50z0cyVctzIGTX3lican6FjxOT5A00DdlU1q/oycwJTAVBgNV\n"
                    "HSUEDjAMBgorBgEEAYLefAEBMAwGA1UdEwEB/wQCMAAwCgYIKoZIzj0EAwIDRwAw\n"
                    "RAIgQsvHZ747URkPCpYtBxi56V1OcMF3oKWnGuz2jazWr4YCICCU5/itaYVt1SzQ\n"
                    "cBYyChWx/4KXL4QKWLdm9/6ispdq\n"
                    "-----END CERTIFICATE-----\n"
                    "\n"
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBsDCCAVagAwIBAgIJAP0No5ho6xiVMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDZkODVjMjkyMjYxM2IzNmUyZWVlZjUyNzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1\n"
                    "YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQyY2M1NjAeFw0xNTAzMDQxNzA2MjFaFw0x\n"
                    "NjAzMDMxNzA2MjFaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
                    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
                    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABA5Xe+uAlcKzCNfFylIEnggx\n"
                    "F6Gq9tmtLY9mxdyOvTsYwpYuirZAQ2wA+wKBPP7zh7+a3plbedd9GDZ8gow8KCmj\n"
                    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAIOU2n6o8QXXbbJVEQe+\n"
                    "n5VkU6DybD3lnsjXSH+1PQVZAiBPCpi8p5xwlBUcFZI1EMPHoLi9XHZtchiJHEo/\n"
                    "OkxLog==\n"
                    "-----END CERTIFICATE-----\n";

                static const char pkForExpiredChain[] =
                    "-----BEGIN EC PRIVATE KEY-----\n"
                    "MHcCAQEEICpnmYJ+rYZyCB2GEbg4waemxF1edz1qGaSnbDFZwwmeoAoGCCqGSM49\n"
                    "AwEHoUQDQgAEl3JuZdX4Pd7APz2FKlHnpgK7pTkuwXlNM2U7krA8uDFTcY0TNEHV\n"
                    "94RlsWApksy4DJrjmOI9SIrQawMemG4IRw==\n"
                    "-----END EC PRIVATE KEY-----";

                static const char expiredChain[] =
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBrzCCAVagAwIBAgIJAIfm4O/IwDXyMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
                    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAyMDEwMDAwMDBaFw0x\n"
                    "NTAyMjEwMDAwMDBaMFYxKTAnBgNVBAsMIDZkODVjMjkyMjYxM2IzNmUyZWVlZjUy\n"
                    "NzgwNDJjYzU2MSkwJwYDVQQDDCA2ZDg1YzI5MjI2MTNiMzZlMmVlZWY1Mjc4MDQy\n"
                    "Y2M1NjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABJdybmXV+D3ewD89hSpR56YC\n"
                    "u6U5LsF5TTNlO5KwPLgxU3GNEzRB1feEZbFgKZLMuAya45jiPUiK0GsDHphuCEej\n"
                    "DTALMAkGA1UdEwQCMAAwCgYIKoZIzj0EAwIDRwAwRAIgAIGsd9RKxw2JDGcwYV9d\n"
                    "EGA4ZUBEXoZqMhRaIw6EjSECIGqablZqrDDzOr6ZGDG6f5X1/HWLOLmHStfHNA/1\n"
                    "BoXu\n"
                    "-----END CERTIFICATE-----"
                    "\n"
                    "-----BEGIN CERTIFICATE-----\n"
                    "MIIBszCCAVmgAwIBAgIJAMPSLBBoNwQIMAoGCCqGSM49BAMCMFYxKTAnBgNVBAsM\n"
                    "IDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBmNzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4\n"
                    "YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5ZGQwMjAeFw0xNTAzMjQxNzA0MTlaFw0x\n"
                    "NjAzMjMxNzA0MTlaMFYxKTAnBgNVBAsMIDdhNDhhYTI2YmM0MzQyZjZhNjYyMDBm\n"
                    "NzdhODlkZDAyMSkwJwYDVQQDDCA3YTQ4YWEyNmJjNDM0MmY2YTY2MjAwZjc3YTg5\n"
                    "ZGQwMjBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOZknbv1si4H58TcDniPnlKm\n"
                    "zxR2xVh1VsZ7anvgSNlxzsiF/Y7qRXeE3G+3sBFjPhrWG63DZuGn96Y+u7qTbcCj\n"
                    "EDAOMAwGA1UdEwQFMAMBAf8wCgYIKoZIzj0EAwIDSAAwRQIgL7NAi2iY0fHaFtIC\n"
                    "d58shzZcoR8IMN3uZ1r+9UFboP8CIQDca5XNPYXn+IezASVqdGfs6KodmVIFK2IO\n"
                    "vAx+KmwF4Q==\n"
                    "-----END CERTIFICATE-----";

                if (!sendKeys) {
                    return false;
                }
                if (sendPrivateKey && ((credMask& AuthListener::CRED_PRIVATE_KEY) == AuthListener::CRED_PRIVATE_KEY)) {
                    if (sendInvalidChain || sendInvalidChainNoCA) {
                        String pk(pkWithInvalidChain);
                        creds.SetPrivateKey(pk);
                    } else if (sendExpiredChain) {
                        String pk(pkForExpiredChain);
                        creds.SetPrivateKey(pk);
                    } else if (server) {
                        String pk(serverPrivateKeyPEM);
                        creds.SetPrivateKey(pk);
                    } else {
                        String pk(clientPrivateKeyPEM);
                        creds.SetPrivateKey(pk);
                    }
                }
                if (sendCertChain && ((credMask& AuthListener::CRED_CERT_CHAIN) == AuthListener::CRED_CERT_CHAIN)) {
                    if (sendEmptyCertChain) {
                        String cert;
                        creds.SetCertChain(cert);
                    } else if (sendInvalidChain) {
                        String cert(invalidChainCert2HasCA);
                        creds.SetCertChain(cert);
                    } else if (sendInvalidChainNoCA) {
                        String cert(invalidChainCert2HasNoCA);
                        creds.SetCertChain(cert);
                    } else if (sendExpiredChain) {
                        String cert(expiredChain);
                        creds.SetCertChain(cert);
                    } else if (server) {
                        String cert(serverCertChainX509PEM);
                        creds.SetCertChain(cert);
                    } else {
                        String cert(clientCertChainX509PEM);
                        creds.SetCertChain(cert);
                    }
                }
                if (sendExpiry) {
                    creds.SetExpiration(expirationSeconds);
                }
                return true;

            }
            return false;
        }

        bool VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& creds)
        {
            QCC_UNUSED(authPeer);

            /* only the ECDHE_ECDSA calls for peer credential verification */
            if (strcmp(authMechanism, "ALLJOYN_ECDHE_ECDSA") == 0) {
                if (failVerifyCertChain) {
                    return false;
                }
                if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                    /*
                     * AllJoyn sends back the certificate chain for the application to verify.
                     * The application has to option to verify the certificate
                     * chain.  If the cert chain is validated and trusted then return true; otherwise, return false.
                     */
                    return true;
                }
                return true;
            }
            return false;
        }

        void AuthenticationComplete(const char* authMechanism, const char* authPeer, bool success) {
            QCC_UNUSED(authPeer);

            authComplete = success;
            chosenMechanism = authMechanism;
        }

        bool sendKeys;
        bool sendExpiry;
        uint32_t expirationSeconds;
        bool sendPrivateKey;
        bool sendCertChain;
        bool sendEmptyCertChain;
        bool sendExpiredChain;
        bool sendInvalidChain;
        bool sendInvalidChainNoCA;
        bool failVerifyCertChain;
        bool useCAPICerts;
        bool authComplete;
        qcc::String pskName;
        qcc::String psk;
        qcc::String chosenMechanism;
      private:
        bool server;
    };

    AuthListenerECDHETest() : BusObject("/AuthListenerECDHETest"),
        clientBus("AuthListenerECDHETestClient", false),
        secondClientBus("AuthListenerECDHETestClient", false),
        serverBus("AuthListenerECDHETestServer", false),
        clientAuthListener(false),
        serverAuthListener(true)
    {
    }

    void SetUp()
    {
        EXPECT_EQ(ER_OK, clientBus.Start());
        EXPECT_EQ(ER_OK, clientBus.Connect());
        EXPECT_EQ(ER_OK, clientBus.RegisterKeyStoreListener(clientKeyStoreListener));
        CreateOnOffAppInterface(clientBus, false);
        /* Although secondClientBus is currently used in only one test, it's simpler to handle
         * setup and teardown of it here rather than duplicate code in the test itself. */
        EXPECT_EQ(ER_OK, secondClientBus.Start());
        EXPECT_EQ(ER_OK, secondClientBus.Connect());
        EXPECT_EQ(ER_OK, secondClientBus.RegisterKeyStoreListener(clientKeyStoreListener));
        CreateOnOffAppInterface(secondClientBus, false);
        EXPECT_EQ(ER_OK, serverBus.Start());
        EXPECT_EQ(ER_OK, serverBus.Connect());
        EXPECT_EQ(ER_OK, serverBus.RegisterKeyStoreListener(serverKeyStoreListener));
        CreateOnOffAppInterface(serverBus, true);
    }

    void TearDown()
    {
        clientBus.UnregisterKeyStoreListener();
        clientBus.UnregisterBusObject(*this);
        EXPECT_EQ(ER_OK, clientBus.Disconnect());
        EXPECT_EQ(ER_OK, clientBus.Stop());
        EXPECT_EQ(ER_OK, clientBus.Join());
        secondClientBus.UnregisterKeyStoreListener();
        secondClientBus.UnregisterBusObject(*this);
        EXPECT_EQ(ER_OK, secondClientBus.Disconnect());
        EXPECT_EQ(ER_OK, secondClientBus.Stop());
        EXPECT_EQ(ER_OK, secondClientBus.Join());
        serverBus.UnregisterKeyStoreListener();
        serverBus.UnregisterBusObject(*this);
        EXPECT_EQ(ER_OK, serverBus.Disconnect());
        EXPECT_EQ(ER_OK, serverBus.Stop());
        EXPECT_EQ(ER_OK, serverBus.Join());
    }

    QStatus EnableSecurity(bool server, const char* keyExchange)
    {
        if (server) {
            return serverBus.EnablePeerSecurity(keyExchange, &serverAuthListener, NULL, false);
        } else {
            return clientBus.EnablePeerSecurity(keyExchange, &clientAuthListener, NULL, false);
        }
    }

    void CreateOnOffAppInterface(BusAttachment& busAttachment, bool addService)
    {
        InterfaceDescription* ifc = NULL;
        QStatus status = busAttachment.CreateInterface(ONOFF_IFC_NAME, ifc, AJ_IFC_SECURITY_REQUIRED);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        EXPECT_TRUE(ifc != NULL);
        if (ifc != NULL) {
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "On", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            status = ifc->AddMember(MESSAGE_METHOD_CALL, "Off", NULL, NULL, NULL);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            ifc->Activate();

            if (!addService) {
                return;  /* done */
            }
            status = AddInterface(*ifc);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
            AddMethodHandler(ifc->GetMember("On"), static_cast<MessageReceiver::MethodHandler>(&AuthListenerECDHETest::OnOffOn));
            AddMethodHandler(ifc->GetMember("Off"), static_cast<MessageReceiver::MethodHandler>(&AuthListenerECDHETest::OnOffOff));
            status = busAttachment.RegisterBusObject(*this);
            EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        }
    }

    BusAttachment clientBus;
    BusAttachment secondClientBus;
    BusAttachment serverBus;
    ECDHEKeyXListener clientAuthListener;
    ECDHEKeyXListener serverAuthListener;

    void OnOffOn(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);
        MethodReply(msg, ER_OK);
    }
    void OnOffOff(const InterfaceDescription::Member* member, Message& msg)
    {
        QCC_UNUSED(member);
        MethodReply(msg, ER_OK);
    }

    QStatus ExerciseOn(bool useSecondBus = false)
    {
        BusAttachment& selectedClientBus = useSecondBus ? secondClientBus : clientBus;

        ProxyBusObject proxyObj(selectedClientBus, serverBus.GetUniqueName().c_str(), GetPath(), 0, false);
        const InterfaceDescription* itf = selectedClientBus.GetInterface(ONOFF_IFC_NAME);
        proxyObj.AddInterface(*itf);
        Message reply(selectedClientBus);

        return proxyObj.MethodCall(ONOFF_IFC_NAME, "On", NULL, 0, reply, 5000);
    }

  private:

    InMemoryKeyStoreListener clientKeyStoreListener;
    InMemoryKeyStoreListener serverKeyStoreListener;
};

TEST_F(AuthListenerECDHETest, ECDHE_NULL_Success)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_Fail)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL"));
    clientAuthListener.sendKeys = false;
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_Success_DoNotSendExpiry)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL"));
    clientAuthListener.sendExpiry = false;
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Success_SendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Success_DoNotSendExpiry)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.sendExpiry = false;
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_DoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.sendKeys = false;
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_ServerDoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_ClientDoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_DifferentPSK)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.psk = "faaa0af3dd3f1e0379da046a3ab6ca44";
    serverAuthListener.psk = "faaa0af3dd3f1e0379da046a3ab6ca45";
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Success_PSKName)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.pskName = "abc";
    serverAuthListener.pskName = "abc";
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_Fail_DifferentPSKName)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    clientAuthListener.pskName = "abc";
    serverAuthListener.pskName = "dfe";
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Success)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Success_CAPI)
{
    /*
     * Since a new AuthListenerECDHETest instance is created for each test,
     * setting useCAPICerts = true will not have side effects in other tests.
     */
    serverAuthListener.useCAPICerts = true;
    clientAuthListener.useCAPICerts = true;
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Success_DoNotSendExpiry)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendExpiry = false;
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_DoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendKeys = false;
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ServerDoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    serverAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ClientDoNotSendKeys)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendKeys = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_DoNotSendPrivateKey)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendPrivateKey = false;
    serverAuthListener.sendPrivateKey = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ServerDoNotSendPrivateKey)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    serverAuthListener.sendPrivateKey = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ClientDoNotSendPrivateKey)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendPrivateKey = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_DoNotSendCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendCertChain = false;
    serverAuthListener.sendCertChain = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ServerDoNotSendCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    serverAuthListener.sendCertChain = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ClientDoNotSendCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendCertChain = false;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_SendEmptyCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendEmptyCertChain = true;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_SendExpiredCertChain)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendExpiredChain = true;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_SendInvalidCertChainWithCA)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendInvalidChain = true;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_SendInvalidCertChainWithNoCA)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendInvalidChainNoCA = true;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_FailVerification)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.failVerifyCertChain = true;
    serverAuthListener.failVerifyCertChain = true;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ServerFailVerification)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    serverAuthListener.failVerifyCertChain = true;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(serverAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Fail_ClientFailVerification)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.failVerifyCertChain = true;
    EXPECT_NE(ER_OK, ExerciseOn());
    EXPECT_FALSE(clientAuthListener.authComplete);
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_ECDSA_SuccessChosenByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_ECDSA_SuccessChosenByClient)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_ECDSA_SuccessChosenByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_ECDSA_SuccessChosenByClient)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_SuccessChosenByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_SuccessChosenByClient)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_ECDSA_SuccessChosenByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_ECDSA_SuccessChosenByClient)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_ECDSA_AcceptableDowngradeByServer)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_ECDSA_Prioritized_To_ECDSA)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_PSK_ECDSA_Prioritized_To_ECDSA)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

TEST_F(AuthListenerECDHETest, ECDHE_NULL_PSK_Prioritized_To_PSK)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_NULL ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_PSK_NULL_Prioritized_To_ECDSA)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_ECDSA");
}

/**
 * In this test, the ECDHE_ECDSA key exchange fails.  The key exchange
 * downgrades to ECDHE_NULL and it should succeed.
 */
TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Downgrade_To_ECDHE_NULL)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_NULL"));
    serverAuthListener.failVerifyCertChain = true;
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_NULL");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_NULL");
}

/**
 * In this test, the ECDHE_PSK key exchange fails.  The key exchange
 * downgrades to ECDHE_NULL and it should succeed.
 */
TEST_F(AuthListenerECDHETest, ECDHE_PSK_Downgrade_To_ECDHE_NULL)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_NULL"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_PSK ALLJOYN_ECDHE_NULL"));
    serverAuthListener.psk = "03781075975973295739873982aabbcc";
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_NULL");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_NULL");
}

/**
 * In this test, the ECDHE_ECDSA key exchange fails.  The key exchange
 * downgrades to ECDHE_PSK and it should succeed.
 */
TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_Downgrade_To_ECDHE_PSK)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA ALLJOYN_ECDHE_PSK"));
    serverAuthListener.failVerifyCertChain = true;
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
    EXPECT_STREQ(clientAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
    EXPECT_STREQ(serverAuthListener.chosenMechanism.c_str(), "ALLJOYN_ECDHE_PSK");
}

TEST_F(AuthListenerECDHETest, ECDHE_ECDSA_TestExpiredSessionKey)
{
    EXPECT_EQ(ER_OK, EnableSecurity(true, "ALLJOYN_ECDHE_ECDSA"));
    EXPECT_EQ(ER_OK, EnableSecurity(false, "ALLJOYN_ECDHE_ECDSA"));
    clientAuthListener.sendExpiry = true;
    clientAuthListener.expirationSeconds = 10000u;
    serverAuthListener.sendExpiry = true;
    serverAuthListener.expirationSeconds = 1u;
    EXPECT_EQ(ER_OK, ExerciseOn());
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);

    /* Despite saying the credential expires in 1 second, it seems to be the case that the minimum expiration
     * is 30 seconds. Sleep for 35 just to be sure.
     */
    printf("*** Sleep 35 secs since the minimum key expiration time is 30 seconds\n");
    qcc::Sleep(35000); /* Parameter for Sleep is ms. */

    /* Use a different bus attachment but use the same client key store. We need a different bus attachment
     * because the default one considers the server peer already secure.
     */
    EXPECT_EQ(ER_OK, secondClientBus.EnablePeerSecurity("ALLJOYN_ECDHE_ECDSA", &clientAuthListener, NULL, false));

    EXPECT_EQ(ER_OK, ExerciseOn(true)); /* `true' parameter will use secondClientBus. */
    EXPECT_TRUE(clientAuthListener.authComplete);
    EXPECT_TRUE(serverAuthListener.authComplete);
}
