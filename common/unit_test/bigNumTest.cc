/******************************************************************************
 *
 * Copyright (c) 2011, 2014, AllSeen Alliance. All rights reserved.
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
#include <qcc/BigNum.h>
#include <gtest/gtest.h>

using namespace qcc;

//class BigNumTest : public testing::Test {
// Prime number used by SRP
static const uint8_t Prime1024[] = {
    0xEE, 0xAF, 0x0A, 0xB9, 0xAD, 0xB3, 0x8D, 0xD6, 0x9C, 0x33, 0xF8, 0x0A, 0xFA, 0x8F, 0xC5, 0xE8,
    0x60, 0x72, 0x61, 0x87, 0x75, 0xFF, 0x3C, 0x0B, 0x9E, 0xA2, 0x31, 0x4C, 0x9C, 0x25, 0x65, 0x76,
    0xD6, 0x74, 0xDF, 0x74, 0x96, 0xEA, 0x81, 0xD3, 0x38, 0x3B, 0x48, 0x13, 0xD6, 0x92, 0xC6, 0xE0,
    0xE0, 0xD5, 0xD8, 0xE2, 0x50, 0xB9, 0x8B, 0xE4, 0x8E, 0x49, 0x5C, 0x1D, 0x60, 0x89, 0xDA, 0xD1,
    0x5D, 0xC7, 0xD7, 0xB4, 0x61, 0x54, 0xD6, 0xB6, 0xCE, 0x8E, 0xF4, 0xAD, 0x69, 0xB1, 0x5D, 0x49,
    0x82, 0x55, 0x9B, 0x29, 0x7B, 0xCF, 0x18, 0x85, 0xC5, 0x29, 0xF5, 0x66, 0x66, 0x0E, 0x57, 0xEC,
    0x68, 0xED, 0xBC, 0x3C, 0x05, 0x72, 0x6C, 0xC0, 0x2F, 0xD4, 0xCB, 0xF4, 0x97, 0x6E, 0xAA, 0x9A,
    0xFD, 0x51, 0x38, 0xFE, 0x83, 0x76, 0x43, 0x5B, 0x9F, 0xC6, 0x1D, 0x2F, 0xC0, 0xEB, 0x06, 0xE3
};

// Prime number used by SRP
static const uint8_t Prime1536[] = {
    0x9D, 0xEF, 0x3C, 0xAF, 0xB9, 0x39, 0x27, 0x7A, 0xB1, 0xF1, 0x2A, 0x86, 0x17, 0xA4, 0x7B, 0xBB,
    0xDB, 0xA5, 0x1D, 0xF4, 0x99, 0xAC, 0x4C, 0x80, 0xBE, 0xEE, 0xA9, 0x61, 0x4B, 0x19, 0xCC, 0x4D,
    0x5F, 0x4F, 0x5F, 0x55, 0x6E, 0x27, 0xCB, 0xDE, 0x51, 0xC6, 0xA9, 0x4B, 0xE4, 0x60, 0x7A, 0x29,
    0x15, 0x58, 0x90, 0x3B, 0xA0, 0xD0, 0xF8, 0x43, 0x80, 0xB6, 0x55, 0xBB, 0x9A, 0x22, 0xE8, 0xDC,
    0xDF, 0x02, 0x8A, 0x7C, 0xEC, 0x67, 0xF0, 0xD0, 0x81, 0x34, 0xB1, 0xC8, 0xB9, 0x79, 0x89, 0x14,
    0x9B, 0x60, 0x9E, 0x0B, 0xE3, 0xBA, 0xB6, 0x3D, 0x47, 0x54, 0x83, 0x81, 0xDB, 0xC5, 0xB1, 0xFC,
    0x76, 0x4E, 0x3F, 0x4B, 0x53, 0xDD, 0x9D, 0xA1, 0x15, 0x8B, 0xFD, 0x3E, 0x2B, 0x9C, 0x8C, 0xF5,
    0x6E, 0xDF, 0x01, 0x95, 0x39, 0x34, 0x96, 0x27, 0xDB, 0x2F, 0xD5, 0x3D, 0x24, 0xB7, 0xC4, 0x86,
    0x65, 0x77, 0x2E, 0x43, 0x7D, 0x6C, 0x7F, 0x8C, 0xE4, 0x42, 0x73, 0x4A, 0xF7, 0xCC, 0xB7, 0xAE,
    0x83, 0x7C, 0x26, 0x4A, 0xE3, 0xA9, 0xBE, 0xB8, 0x7F, 0x8A, 0x2F, 0xE9, 0xB8, 0xB5, 0x29, 0x2E,
    0x5A, 0x02, 0x1F, 0xFF, 0x5E, 0x91, 0x47, 0x9E, 0x8C, 0xE7, 0xA2, 0x8C, 0x24, 0x42, 0xC6, 0xF3,
    0x15, 0x18, 0x0F, 0x93, 0x49, 0x9A, 0x23, 0x4D, 0xCF, 0x76, 0xE3, 0xFE, 0xD1, 0x35, 0xF9, 0xBB
};

//static const char Prime28[] = "BB891A5A1B7B22188DB2E2F781A710467CA3EBBF36D27D9985D35FCB";
static const char Prime30[] = "E377C6030FDC3CAD3DD128E3FB510225ED3E6C497DA2B5A4EA0ADA043091";
//static const char Prime32[] = "E44090AED6F57F841D202DB624F081585975C1C4E3A59D2BD4A5F5205F1A3839";
//static const char Prime34[] = "C28CE0B02955349DAA05B43FD300680F62EDF4A73C4CA6062DAB452E79E10EE3B305";
//static const char Prime36[] = "1EB861FCF9A9AB36E25C54AAD153B70A674BDDC9781680977275337938577288E1C7ECBF";
//static const char Prime38[] = "9636EC1FD2FEA9254A3CEF00C9E220B6DB45E9B8E9B246A615A35A26BFB05CC09AD929CAE4F9";
//static const char Prime40[] = "7AB7581B958AECD633E218728D5A268A10794E95EC9F29D221052C1640305CEE8FFCD15D2AAA7B0F";
//static const char Prime42[] = "CADF2F0237E6983F197CF0E39CE474A31ED144720747AD07500C6399A896B88462BFCCB682D1BAA9ECF3F";
//static const char Prime45[] = "8DD0FE5E00CFC5FF4577BCCC29B115B63A02836A8EE9E87B0E22DD858E3E5777BC9F17686AAFE225BC2057468D";
//static const char Prime48[] = "82F9F1CA3DF619271030FA6E907318F6F4D925EFCA114736A86C7F55DC73C4E337E17FB17F0428AED18358DAC8A35C65";
static const char Prime50[] = "BC5A136B0D466A89DEB3128C9EC165E3185E1CD887944721F7ED50DC9E6382AF7B6CA3792ADF94317FE8866D35D55B3AE41D";
//static const char Prime60[] = "35A1CA5BF199C4358281E5DE37F564991DBA8844C0D32A069A16C447B0F5CE750705A33E324DC7E01FA9110CE656CF5A298BCC3E77B28880319A8B3F";
//static const char Prime62[] = "8E82D6EE289D224EBCA3FD35151D04A2446E33A715391611CC659AEF0E5E799B97650ABC4BFEA43564ED773A14E08FBEA4E8C4BD1904B6E3153CE24C8745";
/*
 * Primes[] is currently not used
 */
/*static const char* Primes[] = {
    Prime60,
    Prime50,
    Prime48,
    Prime45,
    Prime42,
    Prime40,
    Prime38,
    Prime36,
    Prime34,
    Prime32,
    Prime30,
    Prime28
   };*/

static const uint8_t zeroes[256] = { 0 };
//}

TEST(BigNumTest, bit_len) {
    BigNum bn3 = 1;
    BigNum bn4;

    EXPECT_EQ(static_cast<size_t>(1), bn3.bit_len());
    EXPECT_EQ(static_cast<size_t>(0), bn4.bit_len());
}

TEST(BigNumTest, SetGetBytes) {
    BigNum bn1;
    bn1.set_bytes(Prime1024, sizeof(Prime1024));
    uint8_t* buf = new uint8_t[16 + bn1.byte_len()];
    bn1.get_bytes(buf, sizeof(Prime1024));
    EXPECT_EQ(0, memcmp(buf, Prime1024, sizeof(Prime1024)));
    delete [] buf;
}

TEST(BigNumTest, ZeroPadding) {
    BigNum bn1;

    bn1.set_bytes(Prime1024, sizeof(Prime1024));
    uint8_t* buf = new uint8_t[16 + bn1.byte_len()];
    bn1.get_bytes(buf, sizeof(Prime1024));
    EXPECT_EQ(0, memcmp(buf, Prime1024, sizeof(Prime1024)));

    // test that zero padding works
    bn1.get_bytes(buf, 13 + sizeof(Prime1024), true /* pad */);
    ASSERT_EQ(0, memcmp(buf + 13, Prime1024, sizeof(Prime1024)));
    ASSERT_EQ(0, memcmp(buf, zeroes, 13));

    delete [] buf;
}

//Test Basic Arithmetic +, -, and assignment
TEST(BigNumTest, BasicArithmetic) {
    BigNum bn1;
    BigNum bn2;
    BigNum bn3;

    bn1.set_hex("0x10000000000000000");
    bn2.set_hex("0x10000000000000001");

    ASSERT_EQ(static_cast<size_t>(65), bn1.bit_len());
    ASSERT_TRUE(bn1.test_bit(64));
    ASSERT_FALSE(bn1.test_bit(63));

    ASSERT_EQ(static_cast<size_t>(65), bn2.bit_len());
    ASSERT_TRUE(bn2.test_bit(64));
    ASSERT_TRUE(bn2.test_bit(0));

    ASSERT_EQ(bn1, bn1);
    ASSERT_EQ(bn2, bn2);
    ASSERT_LT(bn1, bn2);
    ASSERT_GT(bn2, bn1);

    bn1 = -bn1;
    ASSERT_LT(bn1, bn2);
    bn1 = -bn1;
    bn2 -= bn1;
    ASSERT_TRUE(bn2 == 1);

    bn1.set_hex("0x0000123456789ABCDEF0123456789abcdef");
    bn2 = bn1;
    qcc::String txt = bn2.get_hex();
    ASSERT_STRCASEEQ("123456789ABCDEF0123456789abcdef", txt.c_str());
    EXPECT_FALSE(bn1 != bn2);

    bn1.set_hex("123456789ABCDEF");
    bn1 += 1;
    ASSERT_STRCASEEQ("123456789ABCDF0", bn1.get_hex().c_str());

    bn1.set_hex("FFFFFFFFFFFFFFFFFFFFFFFFF");
    bn1 += 1;
    ASSERT_STRCASEEQ("10000000000000000000000000", bn1.get_hex().c_str());

    bn1 -= 1;
    ASSERT_STRCASEEQ("FFFFFFFFFFFFFFFFFFFFFFFFF", bn1.get_hex().c_str());

    bn1.set_hex("A0A0A0A0A0A0A0A0A0A0A0A0A0A0A0A0A0A0A0A0A");
    bn2.set_hex("0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0");
    bn3 = bn1 + bn2;
    ASSERT_STRCASEEQ("ABABABABABABABABABABABABABABABABABABABABA", bn3.get_hex().c_str());

    bn1 = bn3 - bn2;
    ASSERT_STRCASEEQ("B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0B0", bn2.get_hex().c_str());

    bn1.set_hex("22222222222222222222225");
    bn2.set_hex("22222222222222222222227");
    bn3 = bn1 - bn2;
    ASSERT_STRCASEEQ("-2", bn3.get_hex().c_str());

    bn1.set_hex("FFFFFFFF");
    bn2.set_hex("80000000");
    bn3 = bn1 - bn2;
    ASSERT_STRCASEEQ("7FFFFFFF", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3 == 0x7FFFFFFF);

    bn1.set_hex("FFFFFFFE");
    bn2.set_hex("80000000");
    bn1 -= bn2;
    ASSERT_STRCASEEQ("7FFFFFFF", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3 == 0x7FFFFFFF);
}

TEST(BigNumTest, ShiftOperations) {
    BigNum bn1;
    BigNum bn2;
    BigNum bn3;

    // Shifts
    bn2.set_hex("100000000");
    bn2 >>= 1;
    ASSERT_STRCASEEQ("80000000", bn2.get_hex().c_str());

    bn1.set_hex("1234567811111111");
    bn2 = bn1 << 64;
    bn3 = bn2 >> 32;
    bn2 = bn3 >> 32;
    ASSERT_TRUE(bn2 == bn1);

    bn2.set_hex("0");
    bn1 = 1;
    for (int i = 0; i < 33; ++i) {
        bn1 <<= 1;
        bn2 += bn1;
    }
    for (int i = 0; i < 33; ++i) {
        bn2 -= bn1;
        bn1 >>= 1;
    }
    ASSERT_TRUE(bn1 == 1 && bn2 == 0);
}

TEST(BigNumTest, Multiplication) {
    BigNum bn1;
    BigNum bn2;
    BigNum bn3;
    BigNum bn4;

    // Multiplication
    bn1.set_hex("22222222222222222222222");
    bn2.set_hex("2");
    bn3 = bn1 * bn2;
    ASSERT_STRCASEEQ("44444444444444444444444", bn3.get_hex().c_str());

    // Check commutivity
    ASSERT_TRUE(bn3 == (bn2 * bn1));

    // Check multiplication by a small integer

    bn1.set_hex("400000004");
    bn2.set_hex("9");
    bn3 = bn1 * bn2;
    ASSERT_STRCASEEQ("2400000024", bn3.get_hex().c_str());

    bn1.set_hex(Prime30);
    bn2 = bn1 * 13;
    bn3 = 0;
    for (int i = 0; i < 13; ++i) {
        bn3 += bn1;
    }
    ASSERT_STRCASEEQ("B8D150E27CE2F14CC239F1393C31D1BED0C2B7FBB6143395FE28D1236775D", bn3.get_hex().c_str());
    ASSERT_STRCASEEQ("B8D150E27CE2F14CC239F1393C31D1BED0C2B7FBB6143395FE28D1236775D", bn2.get_hex().c_str());
    ASSERT_TRUE(bn3 == bn2);

    // Multiple precision multiplication

    bn1.set_hex("FFFFFFFFFFFFFFFFF");
    bn2.set_hex("FFFFFFFFFFFFFFFF");
    bn3 = bn1 * bn2;
    ASSERT_STREQ("FFFFFFFFFFFFFFFEF0000000000000001", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3 == (bn2 * bn1));

    bn1.set_hex("1111111111111111111111");
    bn2.set_hex("11111111111111111111111");
    bn3 = bn1 * bn2;
    ASSERT_STREQ("123456789ABCDF012345677654320FEDCBA987654321", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3 == (bn2 * bn1));

    bn1.set_hex("1234567890ABCDEF");
    bn2.set_hex("FEDCBA0987654321");
    bn3 = bn1 * bn2;
    ASSERT_STREQ("121FA000A3723A57C24A442FE55618CF", bn3.get_hex().c_str());
    bn4 = (bn2 * bn1);
    ASSERT_STREQ("121FA000A3723A57C24A442FE55618CF", bn4.get_hex().c_str());
    ASSERT_TRUE(bn3 == (bn2 * bn1));
}

TEST(BigNumTest, DivisionAndModulus) {
    BigNum M;
    BigNum E;
    BigNum bn1;
    BigNum bn2;
    BigNum bn3;
    BigNum bn4;
    BigNum bn5;

    bn1.set_hex("1234567890ABCDEF");
    bn2.set_hex("FEDCBA0987654321");
    bn3 = bn1 * bn2;

    // Division
    bn4 = bn3 / bn1;
    ASSERT_TRUE(bn4 == bn2);
    bn4 = bn3 / bn2;
    ASSERT_TRUE(bn4 == bn1);

    bn4.set_hex("1234567");
    ASSERT_TRUE(bn4 / 10 == 0x1D208A);

    bn1.set_hex("10000000000000000");
    bn2 = bn1 / 0x4000;
    ASSERT_STREQ("4000000000000", bn2.get_hex().c_str());
    ASSERT_TRUE(bn2.get_hex() == "4000000000000");

    bn4.set_hex("1234567812345678");
    bn5.set_hex("1FFFFFFFF");
    ASSERT_TRUE(bn4 / bn5 == 0x91A2B3C);

    // Modulus
    bn1.set_hex("1234567890ABCDEF");
    bn2 = 7;
    bn3 = bn1 % bn2;
    ASSERT_STREQ("4", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3 == 4);

    bn1.set_hex("1234567890ABCDEF");
    bn2.set_hex("ABCDEF0987");
    bn3 = bn1 % bn2;
    ASSERT_STREQ("473DD1EB75", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3.get_hex() == "473DD1EB75");

    bn1.set_hex("1234567890ABCDEF819245F34ABE45C0125");
    bn2.set_hex("ABCD450293948561EF0987");
    bn3 = bn1 % bn2;
    ASSERT_STREQ("A64DA5C29FF9A8060DE70C", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3.get_hex() == "A64DA5C29FF9A8060DE70C");

    bn1.set_hex("1234567890ABCDEF819245F34ABE45C0127");
    bn2.set_hex("ABCD450293948561EF09871");
    bn3 = bn1 % bn2;
    ASSERT_STREQ("AB754B0E945925871CCD382", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3.get_hex() == "AB754B0E945925871CCD382");

    bn1.set_bytes(Prime1024, sizeof(Prime1024));
    bn2 = bn1 * 7 + 3;
    ASSERT_STREQ("686C94B13BFE8E0DE456BC84CD9EE695AA320AAB439FAA451566F59184505C"
                 "63FDD321C3020698CC6899EF88ADE03702625D8EE303512D33FE40184CDA3C"
                 "4FBB99076E5EEA951DEFFA5E8B0BDE3D98D0290573E2262A9ABA86425B5CCC"
                 "A646776DE8025A42620F9414ED193B02406AA3CED388EF5983BD7815E6ACC4"
                 "E466D3038", bn2.get_hex().c_str());
    bn3 = bn2 % bn1;
    ASSERT_TRUE(bn3 == 3);

    bn2.set_bytes(Prime1536, sizeof(Prime1536));
    bn3 = bn2 % bn1;

    bn4 = bn2 / bn1;

    bn5 = bn4 * bn1;
    ASSERT_TRUE((bn5 + bn3) == bn2);

    bn1 = 9;
    bn2 = 11;
    bn3 = bn1 % bn2;
    ASSERT_STREQ("9", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3 == 9);

    bn1 = 5;
    bn3 = bn1.exp(14);
    ASSERT_STREQ("16BCC41E9", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3.get_hex() == "16BCC41E9");

    bn1.set_hex("123456789");
    bn3 = bn1.exp(2);
    ASSERT_STREQ("14B66DC326FB98751", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3.get_hex() == "14B66DC326FB98751");

    bn1.set_hex("123456789");
    bn3 = bn1.exp(15);
    ASSERT_STREQ("6EE9AD9ACEB7254BF077CF86C69D3C51A11E6DA3B06E32A50D6CD33C6E4AAC02314E"
                 "2870AFBB35C566FDF7D81C9FD88EF8232924CDF95178E1B5A6139", bn3.get_hex().c_str());
    ASSERT_TRUE(bn3.get_hex() ==
                "6EE9AD9ACEB7254BF077CF86C69D3C51A11E6DA3B06E32A50D6CD33C6E4AAC02314E"
                "2870AFBB35C566FDF7D81C9FD88EF8232924CDF95178E1B5A6139");

    bn4 = bn1.mod_exp(15, 291);
    bn5 = bn3 % 291;
    ASSERT_STREQ("60", bn5.get_hex().c_str());
    ASSERT_STREQ("60", bn4.get_hex().c_str());
    ASSERT_TRUE(bn3 % 291 == bn4);

    bn1 = 3;
    bn3 = bn1.exp(4660);
    ASSERT_TRUE(bn3.get_hex() ==
                "3CC4C0CA53F42E24B0D77B04E687D700BF971365053CF92200B3EE380B2A5630BE06"
                "91E8E373CD0499E5B8A7F376123443A7AB628E914C2D48D062720D1BB512E8287192"
                "E0E0DA964C9A76E5AC15E6154878EB2648FFE6768D96F4DA642582DEF7B1DBBEDF16"
                "FFD52936612C92697D2F500A319627A723FA80634BD33B1B14231DF5B08E6E7A6C01"
                "16E16AF6BBC314F0DFA149D38346908A50E7BB10D1199C1ED37DF33CBF0FF1CE2621"
                "A6674A1D4307E7185838AE01E04AD1B667EE0270BE895BB44A202E3F78BA0FB8D4D7"
                "7E772A985A1E31B13880033CF59B243C3210C1D8E559AC9CEA59E5841202394E1CE7"
                "75B8D5C336AD8FC11274D07AC8A1F79F9F910910BF3A1A4FE51E8B6A203B6BAFE4A0"
                "1906DE24FE80AD9EFCE35CB1131145A6F288B103154AD7F25C6CFFF4BC6EE42583CD"
                "D8C5BD87F74EFF2257F400841446AE5EC3C96BD938D4F222EEB70924E92FBA1406EA"
                "E0B4BCF973162AB94DE8510ACE450C4C4F2FD10CA50DF0A140A71C74F6F5BD31BDCC"
                "A0F8E3BFA5BB2A33D4A0DD1DA3D6E004246BFE7505A9C1F872A31DEB5D6E5EA2A61D"
                "B8ADFB8DC0A3D17CFAFB18C7B84892D74A8E3B75DEAB8508FFCF32EE76FEEC8BCF9C"
                "FD21BF344A1C28F9B4BA2D4F9CBC0B467C851547EECDB9B78B0AF4C808ACFEC1DD69"
                "5515E85EC90A33B6B90F418651FDBD9E14E2DEDC77F2CE92EE72E83C6B597ECE428C"
                "BE5D14F681B88D7B0D57580506C29D41C8B02D740BBCEEE3A89777B019C49F2DEAEF"
                "3896666DFCB5AB9D7C8466F458F99F1EFEC0914BE401D170361D490A41AA8EB3FEDC"
                "96950165572F02370D77FA90508E8A0F4D537CC9B414817AA91F202E002C8D4281E0"
                "12AC0F475BCB4BD27C1CFBE81D3F3068199F7C77929B2C6430B9791D4E8A3486C734"
                "137DB975AE96C41FB8744809458AEDC4806B6CDC8FF2D16CE562A8FB311EC06A6B3B"
                "C2A2731D25F8D62FD95F083673AA26A55842C015119EAC5263605C664ECAFE6DF0D1"
                "DDE6FD372C5D64D0B1BBFF17FF20473AA91402619AEB71C7E83523DF00D8E9B15A8E"
                "5643872CD761E817AA9DC52B10FD64CE0EE71CE70EA9B2861C5F5658E5866A51B59D"
                "5A151B307860A00D633D8BD393DC1ADD55A8DDF20BAA6AB3B86FB1BEA8EAD61838C5"
                "399141C11C92FB93DB8941F602BE3CA08C1E65F702DEF67B1DBF733DD9B238D70B4E"
                "6D0D3A1536C06EB71DAB0474C795457BCA65489D64E0E8469A7CC2B3671E359601C9"
                "DA053FB41C80F00352D95EC92E2CC7B69FAA921C1662738B96D5176EC806CB5E0920"
                "F855770AA11");

    bn1 = 3;
    bn4 = bn1.mod_exp(4660, 290);
    // Checks
    bn3 = bn1.exp(4660);
    bn5 = bn3 % 290;
    ASSERT_STREQ("A1", bn5.get_hex().c_str());
    ASSERT_STREQ("A1", bn4.get_hex().c_str());
    ASSERT_TRUE(bn3 % 290 == bn4);

    bn1 = 3;
    bn4 = bn1.mod_exp(4660, 291);
    // Checks
    bn3 = bn1.exp(4660);
    bn5 = bn3 % 291;
    ASSERT_STREQ("51", bn5.get_hex().c_str());
    ASSERT_STREQ("51", bn4.get_hex().c_str());
    ASSERT_TRUE(bn3 % 291 == bn4);

    // Test case with a small modulus
    M = 291;
    bn2 = 3;

    bn4 = bn2.mod_exp(4660, M);

    // Checks
    bn5 = bn2.exp(4660) % M;
    ASSERT_STREQ("51", bn5.get_hex().c_str());
    ASSERT_STREQ("51", bn4.get_hex().c_str());
    ASSERT_TRUE(bn5 == bn4);

    // Test case with a large modulus

    // modulus
    M.set_hex(Prime50);
    bn1 = 3;
    E.set_bytes(Prime1024, sizeof(Prime1024));

    bn4 = bn1.mod_exp(E, M);

    bn5 = bn1.mod_exp(E, M);
    ASSERT_STREQ("C7346CBCD70D6690D5D2B4ACC532D1C1BC294DEECF0D4878703CF7364F07AB"
                 "D63F5D366C0821A951395B48D349EC9C9D58F", bn5.get_hex().c_str());
    ASSERT_STREQ("C7346CBCD70D6690D5D2B4ACC532D1C1BC294DEECF0D4878703CF7364F07AB"
                 "D63F5D366C0821A951395B48D349EC9C9D58F", bn4.get_hex().c_str());
    ASSERT_TRUE(bn4 == bn5);
}

TEST(BigNumTest, DivisionAndMultiplicationStress) {
    BigNum bn1;
    BigNum bn2;
    BigNum bn3;
    BigNum bn4;
    // Test over random values
    // division and multiplication stress
    for (int i = 1; i < 200; ++i) {
        //for (int n = 0; n < 1000; ++n) {
        for (int j = 0; j < 50; ++j) {
            //bn1.gen_rand(i + 1 + j / 10);
            bn1.gen_rand(i + 1);
            if ((i % 8) == 1) {
                bn1 = -bn1;
            }
            do {
                bn2.gen_rand(i);
            } while (bn2 == 0);
            if ((i % 16) == 1) {
                bn2 = -bn2;
            }
            bn3 = bn1 / bn2;
            bn4 = bn1 % bn2;
            EXPECT_TRUE(((bn2 * bn3) + bn4) == bn1) <<
            "bn1: " << bn1.get_hex().c_str() << "\n" <<
            "bn2: " << bn2.get_hex().c_str() << "\n" <<
            "bn3: " << bn3.get_hex().c_str() << "\n" <<
            "bn4: " << bn4.get_hex().c_str();
            //}
        }
    }
}

TEST(BigNumTest, ModularExponentiationStress) {
    // Modular exponentiation stress
    for (int i = 2; i < 200; ++i) {
        BigNum e;
        BigNum m;
        BigNum a;
        e.gen_rand(i);
        for (int j = 2; j < 16; ++j) {
            do {
                m.gen_rand(j);
            } while (m.is_even());
            a.gen_rand(1);
            // Brute force modular exponentiation for checking
            BigNum check = 1;
            size_t i = e.bit_len();
            while (i) {
                check = (check * check) % m;
                if (e.test_bit(--i)) {
                    check = (check * a) % m;
                }
            }
            BigNum exp = a.mod_exp(e, m);
            EXPECT_FALSE(exp != check) <<
            "val exp: " << exp.get_hex().c_str() << "\n" <<
            "val a: " << a.get_hex().c_str() << "\n" <<
            "val e: " << e.get_hex().c_str() << "\n" <<
            "val m: " << m.get_hex().c_str();
        }
    }
}
