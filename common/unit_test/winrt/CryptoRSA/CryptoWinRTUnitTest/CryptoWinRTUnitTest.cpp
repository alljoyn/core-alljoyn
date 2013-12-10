/******************************************************************************
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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

#include "pch.h"
#include "CppUnitTest.h"

using namespace std;
using namespace qcc;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CryptoWinRTUnitTest {


const char hw[] = "hello world";

static const char x509cert[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n"
    "QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n"
    "N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n"
    "AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n"
    "h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n"
    "xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n"
    "AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n"
    "viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n"
    "PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n"
    "7THIAV79Lg==\n"
    "-----END CERTIFICATE-----"
};
static const wchar_t w_x509cert[] = {
    L"-----BEGIN CERTIFICATE-----\n"
    L"MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n"
    L"QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n"
    L"N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n"
    L"AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n"
    L"h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n"
    L"xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n"
    L"AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n"
    L"viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n"
    L"PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n"
    L"7THIAV79Lg==\n"
    L"-----END CERTIFICATE-----"
};

static const char PEM_DES[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: DES-EDE3-CBC,86B9DBED35AEBAB3\n"
    "\n"
    "f28sibgVCkDz3VNoC/MzazG2tFj+KGf6xm9LQki/GsxpMhJsEEvT9dUluT1T4Ypr\n"
    "NjG+nBleLcfdHxOl5XHnusn8r/JVaQQGVSnDaeP/27KiirtB472p+8Wc2wfXexRz\n"
    "uSUv0DJT+Fb52zYGiGzwgaOinQEBskeO9AwRyG34sFKqyyapyJtSZDjh+wUAIMZb\n"
    "wKifvl1KHSCbXEhjDVlxBw4Rt7I36uKzTY5oax2L6W6gzxfHuOtzfVelAaM46j+n\n"
    "KANZgx6KGW2DKk27aad2HEZUYeDwznpwU5Duw9b0DeMTkez6CuayiZHb5qEod+0m\n"
    "pCCMwpqxFCJ/vg1VJjmxM7wpCQTc5z5cjX8saV5jMUJXp09NuoU/v8TvhOcXOE1T\n"
    "ENukIWYBT1HC9MJArroLwl+fMezKCu+F/JC3M0RfI0dlQqS4UWH+Uv+Ujqa2yr9y\n"
    "20zYS52Z4kyq2WnqwBk1//PLBl/bH/awWXPUI2yMnIILbuCisRYLyK52Ge/rS51P\n"
    "vUgUCZ7uoEJGTX6EGh0yQhp+5jGYVdHHZB840AyxzBQx7pW4MtTwqkw1NZuQcdSN\n"
    "IU9y/PferHhMKZeGfVRVEkAOcjeXOqvSi6NKDvYn7osCkvj9h7K388o37VMPSacR\n"
    "jDwDTT0HH/UcM+5v/74NgE/OebaK3YfxBVyMmBzi0WVFXgxHJir4xpj9c20YQVw9\n"
    "hE3kYepW8gGz/JPQmRszwLQpwQNEP60CgQveqtH7tZVXzDkElvSyveOdjJf1lw4B\n"
    "uCz54678UNNeIe7YB4yV1dMVhhcoitn7G/+jC9Qk3FTnuP+Ws5c/0g==\n"
    "-----END RSA PRIVATE KEY-----"
};

static const char PEM_AES[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: AES-128-CBC,0AE4BAB94CEAA7829273DD861B067DBA\n"
    "\n"
    "LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n"
    "jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n"
    "XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n"
    "w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n"
    "9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n"
    "YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n"
    "wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n"
    "Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n"
    "3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n"
    "AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n"
    "pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n"
    "DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n"
    "bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n"
    "-----END RSA PRIVATE KEY-----"
};
static const wchar_t w_PEM_AES[] = {
    L"-----BEGIN RSA PRIVATE KEY-----\n"
    L"Proc-Type: 4,ENCRYPTED\n"
    L"DEK-Info: AES-128-CBC,0AE4BAB94CEAA7829273DD861B067DBA\n"
    L"\n"
    L"LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n"
    L"jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n"
    L"XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n"
    L"w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n"
    L"9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n"
    L"YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n"
    L"wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n"
    L"Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n"
    L"3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n"
    L"AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n"
    L"pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n"
    L"DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n"
    L"bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n"
    L"-----END RSA PRIVATE KEY-----"
};



static const char PEM_PKCS8_V1_5[] = {
    "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
    "MIICoTAbBgkqhkiG9w0BBQMwDgQIOUsiiy9gId4CAggABIICgM/YtiPQuve9FDVz\n"
    "6kRTKl+6aeIOlURDVkNohPrAjZZL+1n2lckVYgFaUjEEOxutZFYW8F4+UnFy2o/l\n"
    "wK8IZm8EKnXIKHTh8f/5n4V1N3rTJHjY1JHIfw4AhrgBxK2i3I6eIZ7Gt/JTviQ4\n"
    "5MWGC9VI2lrwC3EPQsXbBIKHTg3pxq9NxIwOjvrbqetz9SMYCjMzlsFwvgtFb6Ih\n"
    "B1O9dRAMt3Hh3ZPk9qb2L0NU3581bJV7qDG6MNSTPsvFgbiKpHcLaVZAelpHy69r\n"
    "RlM450FJ/YrzOPEPH89o9Cqk8gZEBxBfwGV9ldMt2uW7LwyIQGAPRYu8IJlvD2fw\n"
    "/CySxgD+LkrkLP1QdMtC3QpBC/C7PEPpg6DoL4VsU/2j6F01K+IgnhTaEsaHLPDa\n"
    "CWt4dRapQvzL2jIy43YcA15GT0qyVBpWZJFvT0ZcTj72lx9nnbkEWMEANfWeqOgC\n"
    "EsUotiEIO6S8+M8MI5oX4DvARd150ePWbu9bNUrQojSjGM2JH/x6kVzsZZP4WG3Q\n"
    "5371FFuXe1QIXtcs2zgj30L397ATHd8979k/8sc+TXd1ba4YzA2j/ncI5jIor0UA\n"
    "hxUYugd1O8FNqahxZpIntxX4dERuX0AT4+4qSG4s10RV1VbbGNot91xq/KM3kZEe\n"
    "r8fvJMIuFNgUqU9ffv0Bt5qeIquPdUH0xhEUoxiTeukz9KobbVZt3hZvG4BrmBC0\n"
    "UYZD6jBcVcA99yDYQ5EUuu7cmHJY2tHdvmhBhAugIfbGldMeripzgiIR1pRblSZB\n"
    "HkY/WUL0IavBvRnAYsYmxXb9Mbp/1vK3xYUTUha2oed2wDPA0ZqBQ+jnb12te1kV\n"
    "kYdjxFM=\n"
    "-----END ENCRYPTED PRIVATE KEY-----"
};

static const char PEM_PKCS8_V2[] = {
    "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"
    "MIICzzBJBgkqhkiG9w0BBQ0wPDAbBgkqhkiG9w0BBQwwDgQIeDCEmXfjzmsCAggA\n"
    "MB0GCWCGSAFlAwQBAgQQpieyiZovXD0OSQPE01x9gASCAoDXhEqWInWJLXyeLKXW\n"
    "bwSXgpQfk38S2jyo7OaNMthNdvQ83K3PctQfwxiiQ9W15FIS27/w4oHXmiukmN5V\n"
    "J+fCPwZ90e4lnuKzyuQcCL0LS+h+EXV5H0b254jOBwmuEfL38tekUa9RnV4e/RxK\n"
    "9uocePeHpFQv1RwwqzLVsptgMNX6NsRQ3YwLpCw9qzPFcejC8WZBLjB9osn4QD18\n"
    "GXORCNUPIJE7LV9/77SNcgchVIXCbSu1sRmiJRpDYc6E91Y6xbDl2KNNgCM3PrU6\n"
    "ERiP/8wetlbZZeX/tKZOCmA+n5pQQmeBkC/JaI8zqH9ZZODIuHDNzJWjtyKENfOT\n"
    "zM4u2RnRFhkp4bzjAZCwfh0Ink1Ge082OHEzN/+4KkSPdxoCKfIPTPS70NQ3vX7F\n"
    "u9IzC+yN1T+pVxluwbhRPQmuOvIX3hca6BIBS+cevppp1E/KXRD5WNtSkJbDknEH\n"
    "3phVQxEu1oaEhb/5e9AgQGg7aEqXX12MQLD+0V3/v65Z4FPvkiejjLL6PU1FuLyG\n"
    "fzZRT+GyiHLfpxZYt7aictQWAT2he7Rn7gJefJLSnFsoKVHoOvmfMvYZU3yZZaZD\n"
    "WenrGheUSrDX5slnqwON0iD/xAh6Z7KVr5U8RNvGrkyYzvXVKS1LTjJ1qfnD7JdF\n"
    "1CbNoCd7rfe5fSxtdKsgP77SMkKO+kN/0Z2P1iIfxE5SsRyxzq/o8dar/olB8Ttz\n"
    "ebDWpX6F16ew1DUDWgi9Dm5Jr17yZjldbcOhpqKYS7Jwe8mQUz+swO/HBIlm7qYg\n"
    "fKdkFYQyjOG2/4nzRPSdw235vs9Bd4R0s+p89cXsZmFHQQU9utYuPl/87a4RwaRT\n"
    "ASbM\n"
    "-----END ENCRYPTED PRIVATE KEY-----\n"
};

static const char x509certChain[] = {
    // User certificate
    "-----BEGIN CERTIFICATE-----\n"
    "MIICxzCCAjCgAwIBAgIJALZkSW0TWinQMA0GCSqGSIb3DQEBBQUAME8xCzAJBgNV\n"
    "BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMQ0wCwYDVQQKEwRRdUlDMQ0wCwYD\n"
    "VQQLEwRNQnVzMQ0wCwYDVQQDEwRHcmVnMB4XDTEwMDgyNTIzMTYwNVoXDTExMDgy\n"
    "NTIzMTYwNVowfzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAO\n"
    "BgNVBAcTB1NlYXR0bGUxIzAhBgNVBAoTGlF1YWxjb21tIElubm92YXRpb24gQ2Vu\n"
    "dGVyMREwDwYDVQQLEwhNQnVzIGRldjERMA8GA1UEAxMIU2VhIEtpbmcwgZ8wDQYJ\n"
    "KoZIhvcNAQEBBQADgY0AMIGJAoGBALz+YZcH0DZn91sjOA5vaTwjQVBnbR9ZRpCA\n"
    "kGD2am0F91juEPFvj/PAlvVLPd5nwGKSPiycN3l3ECxNerTrwIG2XxzBWantFn5n\n"
    "7dDzlRm3aerFr78EJmcCiImwgqsuhUT4eo5/jn457vANO9B5k/1ddc6zJ67Jvuh6\n"
    "0p4YAW4NAgMBAAGjezB5MAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8WHU9wZW5T\n"
    "U0wgR2VuZXJhdGVkIENlcnRpZmljYXRlMB0GA1UdDgQWBBTXau+rH64d658efvkF\n"
    "jkaEZJ+5BTAfBgNVHSMEGDAWgBTu5FqZL5ShsNq4KJjOo8IPZ70MBTANBgkqhkiG\n"
    "9w0BAQUFAAOBgQBNBt7+/IaqGUSOpYAgHun87c86J+R38P2dmOm+wk8CNvKExdzx\n"
    "Hp08aA51d5YtGrkDJdKXfC+Ly0CuE2SCiMU4RbK9Pc2H/MRQdmn7ZOygisrJNgRK\n"
    "Gerh1OQGuc1/USAFpfD2rd+xqndp1WZz7iJh+ezF44VMUlo2fTKjYr5jMQ==\n"
    "-----END CERTIFICATE-----\n"
    // Root certificate
    "-----BEGIN CERTIFICATE-----\n"
    "MIICzjCCAjegAwIBAgIJALZkSW0TWinPMA0GCSqGSIb3DQEBBQUAME8xCzAJBgNV\n"
    "BAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMQ0wCwYDVQQKEwRRdUlDMQ0wCwYD\n"
    "VQQLEwRNQnVzMQ0wCwYDVQQDEwRHcmVnMB4XDTEwMDgyNTIzMTQwNloXDTEzMDgy\n"
    "NDIzMTQwNlowTzELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xDTAL\n"
    "BgNVBAoTBFF1SUMxDTALBgNVBAsTBE1CdXMxDTALBgNVBAMTBEdyZWcwgZ8wDQYJ\n"
    "KoZIhvcNAQEBBQADgY0AMIGJAoGBANc1GTPfvD347zk1NlZbDhTf5txn3AcSG//I\n"
    "gdgdZOY7ubXkNMGEVBMyZDXe7K36MEmj5hfXRiqfZwpZjjzJeJBoPJvXkETzatjX\n"
    "vs4d5k1m0UjzANXp01T7EK1ZdIP7AjLg4QMk+uj8y7x3nElmSpNvPf3tBe3JUe6t\n"
    "Io22NI/VAgMBAAGjgbEwga4wHQYDVR0OBBYEFO7kWpkvlKGw2rgomM6jwg9nvQwF\n"
    "MH8GA1UdIwR4MHaAFO7kWpkvlKGw2rgomM6jwg9nvQwFoVOkUTBPMQswCQYDVQQG\n"
    "EwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjENMAsGA1UEChMEUXVJQzENMAsGA1UE\n"
    "CxMETUJ1czENMAsGA1UEAxMER3JlZ4IJALZkSW0TWinPMAwGA1UdEwQFMAMBAf8w\n"
    "DQYJKoZIhvcNAQEFBQADgYEAg3pDFX0270jUTf8mFJHJ1P+CeultB+w4EMByTBfA\n"
    "ZPNOKzFeoZiGe2AcMg41VXvaKJA0rNH+5z8zvVAY98x1lLKsJ4fb4aIFGQ46UZ35\n"
    "DMrqZYmULjjSXWMxiphVRf1svKGU4WHR+VSvtUNLXzQyvg2yUb6PKDPUQwGi9kDx\n"
    "tCI=\n"
    "-----END CERTIFICATE-----\n"
};

static const char privKey[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: DES-EDE3-CBC,86B9DBED35AEBAB3\n"
    "\n"
    "f28sibgVCkDz3VNoC/MzazG2tFj+KGf6xm9LQki/GsxpMhJsEEvT9dUluT1T4Ypr\n"
    "NjG+nBleLcfdHxOl5XHnusn8r/JVaQQGVSnDaeP/27KiirtB472p+8Wc2wfXexRz\n"
    "uSUv0DJT+Fb52zYGiGzwgaOinQEBskeO9AwRyG34sFKqyyapyJtSZDjh+wUAIMZb\n"
    "wKifvl1KHSCbXEhjDVlxBw4Rt7I36uKzTY5oax2L6W6gzxfHuOtzfVelAaM46j+n\n"
    "KANZgx6KGW2DKk27aad2HEZUYeDwznpwU5Duw9b0DeMTkez6CuayiZHb5qEod+0m\n"
    "pCCMwpqxFCJ/vg1VJjmxM7wpCQTc5z5cjX8saV5jMUJXp09NuoU/v8TvhOcXOE1T\n"
    "ENukIWYBT1HC9MJArroLwl+fMezKCu+F/JC3M0RfI0dlQqS4UWH+Uv+Ujqa2yr9y\n"
    "20zYS52Z4kyq2WnqwBk1//PLBl/bH/awWXPUI2yMnIILbuCisRYLyK52Ge/rS51P\n"
    "vUgUCZ7uoEJGTX6EGh0yQhp+5jGYVdHHZB840AyxzBQx7pW4MtTwqkw1NZuQcdSN\n"
    "IU9y/PferHhMKZeGfVRVEkAOcjeXOqvSi6NKDvYn7osCkvj9h7K388o37VMPSacR\n"
    "jDwDTT0HH/UcM+5v/74NgE/OebaK3YfxBVyMmBzi0WVFXgxHJir4xpj9c20YQVw9\n"
    "hE3kYepW8gGz/JPQmRszwLQpwQNEP60CgQveqtH7tZVXzDkElvSyveOdjJf1lw4B\n"
    "uCz54678UNNeIe7YB4yV1dMVhhcoitn7G/+jC9Qk3FTnuP+Ws5c/0g==\n"
    "-----END RSA PRIVATE KEY-----"
};


TEST_CLASS(CryptoWinRTUnitTest)
{
  public:
    TEST_METHOD(TestMethod1)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);

        QStatus status;
        qcc::Crypto_RSA cr(512);

        cr.MakeSelfCertificate(qcc::String("common name"), qcc::String("app name"));
        qcc::String strPEM;
        status = cr.ExportPEM(strPEM);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");

        Logger::WriteMessage(L"PEM was");
        Logger::WriteMessage(strPEM.data());

        Logger::WriteMessage(cr.CertToString().data());

        Logger::WriteMessage("End: " __FUNCTION__);

    }

    TEST_METHOD(Import_public_key_from_cert)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);
        QStatus status;
        qcc::Crypto_RSA cr;

        status = cr.ImportPEM(x509cert);
        Assert::IsTrue((ER_OK == status), L" ImportPEM failed");

        Logger::WriteMessage(L"Original PEM was");
        Logger::WriteMessage(x509cert);

        qcc::String strPEM;
        status = cr.ExportPEM(strPEM);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");

        Logger::WriteMessage(L"Exported PEM was");
        Logger::WriteMessage(strPEM.data());

        Logger::WriteMessage("End: " __FUNCTION__);
    }
    TEST_METHOD(Import_PEM_endcoded_PKCS8_3DES_encrypted)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);
        // Import PEM encoded PKCS8 (3DES encrypted SSLeay legacy format)
        Crypto_RSA priv;
        QStatus status = priv.ImportPKCS8(PEM_DES, "123456");
        Assert::IsTrue((ER_OK == status), L"ImportPKCS8 failed");

        Logger::WriteMessage("End: " __FUNCTION__);
    }

    TEST_METHOD(Import_PEM_endcoded_PKCS8_AES_encrypted)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);
        // Import PEM encoded PKCS8 (AES encrypted SSLeay legacy format)
        Crypto_RSA priv;
        QStatus status = priv.ImportPKCS8(PEM_AES, "123456");

        Assert::IsTrue((ER_OK == status), L"status == ER_OK");
        Logger::WriteMessage("End: " __FUNCTION__);
    }

    TEST_METHOD(Import_PEM_encoded_PKCS8_v1_5_encrypted)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);
        // Import PEM encoded PKCS8 v2 encrypted format)
        //printf("Testing private key import PKCS8 PKCS#5 v2.0\n");
        Crypto_RSA priv;
        QStatus status = priv.ImportPKCS8(PEM_PKCS8_V1_5, "123456");
        Assert::IsTrue((ER_OK == status), L"ImportPKCS failed");

        qcc::String strPEM;
        status = priv.ExportPEM(strPEM);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");

        Logger::WriteMessage(L"Exported PEM was");
        Logger::WriteMessage(strPEM.data());
        Logger::WriteMessage("End: " __FUNCTION__);
    }

    TEST_METHOD(Import_PEM_encoded_PKCS8_v2_encrypted)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);
        // Import PEM encoded PKCS8 v2 encrypted format)
        //printf("Testing private key import PKCS8 PKCS#5 v2.0\n");
        Crypto_RSA priv;
        QStatus status = priv.ImportPKCS8(PEM_PKCS8_V2, "123456");
        Assert::IsTrue((ER_OK == status), L"status == ER_OK");

        qcc::String strPEM;
        status = priv.ExportPEM(strPEM);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");

        Logger::WriteMessage(L"Exported PEM was");
        Logger::WriteMessage(strPEM.data());
        Logger::WriteMessage("End: " __FUNCTION__);
    }



    TEST_METHOD(encryption_decryption)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);

        qcc::String pubStr;
        qcc::KeyBlob privKey;

        Crypto_RSA pk(512);

        size_t pkSize = pk.GetSize();

        size_t inLen;
        uint8_t in[2048];
        size_t outLen;
        uint8_t out[2048];

        //printf("Public key:\n%s\n", pubStr.c_str());
        QStatus status = pk.ExportPrivateKey(privKey, "pa55pHr@8e");
        Assert::IsTrue((ER_OK == status), L"status == ER_OK");

        //Test encryption with private key and decryption with public key
        memcpy(in, hw, sizeof(hw));
        inLen = sizeof(hw);
        outLen = pkSize;
        status = pk.PublicEncrypt(in, inLen, out, outLen);

        Assert::IsTrue((ER_OK == status), L"status == ER_OK");
        Assert::AreEqual((size_t)64, outLen);

        inLen = outLen;
        outLen = pkSize;
        memcpy(in, out, pkSize);
        status = pk.PrivateDecrypt(in, inLen, out, outLen);
        Assert::IsTrue((ER_OK == status), L"status == ER_OK");
        Assert::AreEqual((size_t)12, outLen);
        Assert::AreEqual("hello world", reinterpret_cast<char*>(out));
        Logger::WriteMessage("End: " __FUNCTION__);
    }


    TEST_METHOD(cert_generation)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);
        qcc::String pubStr;
        qcc::KeyBlob privKey;

        Crypto_RSA pk(512);

        size_t pkSize = pk.GetSize();

        size_t inLen;
        uint8_t in[2048];
        size_t outLen;
        uint8_t out[2048];

        //Test certificate generation
        QStatus status = pk.MakeSelfCertificate("my name", "my app");
        Assert::IsTrue((ER_OK == status), L"MakeSelfCertificate failed");

        //TODO figure out best way to verify the cert
        //printf("Cert:\n%s", pk.CertToString().c_str());

        status = pk.ExportPrivateKey(privKey, "password1234");
        Assert::IsTrue((ER_OK == status), L"ExportPrivateKey failed");

        status = pk.ExportPEM(pubStr);
        Assert::IsTrue((ER_OK == status), L"ExportPEM failed");

        Logger::WriteMessage(L"Exported PEM was");
        Logger::WriteMessage(pubStr.data());

        //Initialize separate private and public key instances and test encryption a decryption
        Crypto_RSA pub;
        status = pub.ImportPEM(pubStr);
        Assert::IsTrue((ER_OK == status), L"ImportPEM failed");

        //TODO figure out best way to verify the PEM
        //printf("PEM:\n%s\n", pubStr.c_str());

        Crypto_RSA pri;
        status = pri.ImportPrivateKey(privKey, "password1234");
        Assert::IsTrue((ER_OK == status), L"ImportPrivateKey");

        pkSize = pub.GetSize();

        memcpy(in, hw, sizeof(hw));
        inLen = sizeof(hw);
        outLen = pkSize;
        status = pub.PublicEncrypt(in, inLen, out, outLen);
        Assert::IsTrue((ER_OK == status), L"PublicEncrypt failed");
        Assert::AreEqual((size_t)64, outLen);

        inLen = outLen;
        outLen = pkSize;
        memcpy(in, out, pkSize);
        status = pri.PrivateDecrypt(in, inLen, out, outLen);
        Assert::IsTrue((ER_OK == status), L"PublicDecrypt failed");
        Assert::AreEqual((size_t)12, outLen);
        Assert::AreEqual(hw, reinterpret_cast<char*>(out));

        Logger::WriteMessage("End: " __FUNCTION__);
    }


    TEST_METHOD(sign_and_verify)
    {
        //printf("Testing empty passphrase\n");
        qcc::String pubStr;
        qcc::KeyBlob privKey;

        Crypto_RSA pk(512);

        //Test certificate generation
        QStatus status = pk.MakeSelfCertificate("my name", "my app");
        Assert::IsTrue((ER_OK == status), L"MakeSelfCertificate failed");

        //TODO figure out best way to verify the cert
        //printf("Cert:\n%s", pk.CertToString().c_str());

        //Check we allow an empty password
        status = pk.ExportPrivateKey(privKey, "123456");
        Assert::IsTrue((ER_OK == status), L" ExportPrivateKey failed");

        status = pk.ExportPEM(pubStr);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");

        // Initialize separate private and public key instances and test signing a verification
        Crypto_RSA pub;
        status = pub.ImportPEM(pubStr);
        Assert::IsTrue((ER_OK == status), L" ImportPEM failed");

        //TODO figure out best way to verify the PEM
        //printf("PEM:\n%s\n", pubStr.c_str());

        Crypto_RSA pri;
        status = pri.ImportPrivateKey(privKey, "123456");
        Assert::IsTrue((ER_OK == status), L" ImportPrivateKey failed");

        const char doc[] = "This document requires a signature";
        uint8_t signature[64];
        size_t sigLen = sizeof(signature);

        //Test Sign and Verify APIs
        status = pri.Sign((const uint8_t*)doc, sizeof(doc), signature, sigLen);
        Assert::IsTrue((ER_OK == status), L" Sign failed");

        // try to tverify usiung the same key.
        status = pri.Verify((const uint8_t*)doc, sizeof(doc), signature, sigLen);
        Assert::IsTrue((ER_OK == status), L" Verify failed");


        status = pub.Verify((const uint8_t*)doc, sizeof(doc), signature, sigLen);
        Assert::IsTrue((ER_OK == status), L" Verify failed");
    }

    TEST_METHOD(empty_passphrase)
    {
        //printf("Testing empty passphrase\n");
        qcc::String pubStr;
        qcc::KeyBlob privKey;

        Crypto_RSA pk(512);

        //Test certificate generation
        QStatus status = pk.MakeSelfCertificate("my name", "my app");
        Assert::IsTrue((ER_OK == status), L"MakeSelfCertificate failed");

        //TODO figure out best way to verify the cert
        //printf("Cert:\n%s", pk.CertToString().c_str());

        // Check we allow an empty password
        status = pk.ExportPrivateKey(privKey, "");
        Assert::IsTrue((ER_OK == status), L" ExportPrivateKey failed");

        status = pk.ExportPEM(pubStr);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");

        // Initialize separate private and public key instances and test signing a verification
        Crypto_RSA pub;
        status = pub.ImportPEM(pubStr);
        Assert::IsTrue((ER_OK == status), L" ImportPEM failed");

        //TODO figure out best way to verify the PEM
        //printf("PEM:\n%s\n", pubStr.c_str());

        Crypto_RSA pri;
        status = pri.ImportPrivateKey(privKey, "");
        Assert::IsTrue((ER_OK == status), L" ImportPrivateKey failed");

        const char doc[] = "This document requires a signature";
        uint8_t signature[64];
        size_t sigLen = sizeof(signature);

        //Test Sign and Verify APIs
        status = pri.Sign((const uint8_t*)doc, sizeof(doc), signature, sigLen);
        Assert::IsTrue((ER_OK == status), L" Sign failed");

        status = pub.Verify((const uint8_t*)doc, sizeof(doc), signature, sigLen);
        Assert::IsTrue((ER_OK == status), L" Verify failed");
    }


    TEST_METHOD(Import_With_Bad_Passphrase)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);

        // use a bad passphrase when importing a cert
        Crypto_RSA priv;
        QStatus status = priv.ImportPKCS8(PEM_AES, "imjustguessingabadpassphrase");

        Assert::IsTrue((ER_AUTH_FAIL == status), L"status == ER_AUTH_FAIL");
        Logger::WriteMessage("End: " __FUNCTION__);
    }

    TEST_METHOD(KeySize1024)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);

        QStatus status;
        qcc::Crypto_RSA cr(1024);

        cr.MakeSelfCertificate(qcc::String("common name"), qcc::String("app name"));
        qcc::String strPEM;
        status = cr.ExportPEM(strPEM);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");
        Logger::WriteMessage(L"PEM was");
        Logger::WriteMessage(strPEM.data());

        Logger::WriteMessage(cr.CertToString().data());

        Logger::WriteMessage("End: " __FUNCTION__);
    }

    TEST_METHOD(KeySize2048)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);

        QStatus status;
        qcc::Crypto_RSA cr(1024);

        cr.MakeSelfCertificate(qcc::String("common name"), qcc::String("app name"));
        qcc::String strPEM;
        status = cr.ExportPEM(strPEM);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");

        Logger::WriteMessage(L"PEM was");
        Logger::WriteMessage(strPEM.data());

        Logger::WriteMessage(cr.CertToString().data());

        Logger::WriteMessage("End: " __FUNCTION__);
    }


    TEST_METHOD(Stress_Create_Loop)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);

        QStatus status;
        qcc::Crypto_RSA* pCR;

        pCR = new qcc::Crypto_RSA(1024);

        for (size_t i = 0; i < 20; i++) {
            delete pCR;
            pCR = new qcc::Crypto_RSA(1024);
            status = pCR->MakeSelfCertificate(qcc::String("common name"), qcc::String("app name"));
            Assert::IsTrue((ER_OK == status), L" MakeSelfCertificate failed");

        }

        qcc::String strPEM;
        status = pCR->ExportPEM(strPEM);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");
        Logger::WriteMessage(L"PEM was");
        Logger::WriteMessage(strPEM.data());

        Logger::WriteMessage(pCR->CertToString().data());
        delete pCR;

        Logger::WriteMessage("End: " __FUNCTION__);
    }

    TEST_METHOD(Import_V3__cert)
    {
        Logger::WriteMessage("Start: " __FUNCTION__);
        QStatus status;
        qcc::Crypto_RSA cr;

        status = cr.ImportPEM(x509certChain);
        Assert::IsTrue((ER_OK == status), L" ImportPEM failed");
        Logger::WriteMessage(L"Original PEM was");
        Logger::WriteMessage(x509cert);

        qcc::String strPEM;
        status = cr.ExportPEM(strPEM);
        Assert::IsTrue((ER_OK == status), L" ExportPEM failed");
        Logger::WriteMessage(L"Exported PEM was");
        Logger::WriteMessage(strPEM.data());

        Logger::WriteMessage("End: " __FUNCTION__);
    }

};
}
