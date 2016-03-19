/******************************************************************************
 *
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
#include <utility>
#include <gtest/gtest.h>

#include <Status.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>

#include <qcc/Crypto.h>

using namespace qcc;

typedef std::pair<AJ_PCSTR, AJ_PCSTR> Base64TestInput;

class ASN1Base64Test : public testing::TestWithParam<Base64TestInput> {
  public:
    ASN1Base64Test() :
        m_input(GetParam().first),
        m_expectedOutput(GetParam().second)
    { }

  protected:
    String m_input;
    String m_expectedOutput;
};

class ANS1DecodeBase64Test : public ASN1Base64Test { };
class ANS1EncodeBase64Test : public ASN1Base64Test { };

class ASN1DecodeBase64VectorTest : public testing::TestWithParam<Base64TestInput> {
  public:
    ASN1DecodeBase64VectorTest() :
        m_input(GetParam().first)
    { }

    virtual void SetUp()
    {
        String outputString = GetParam().second;
        m_expectedOutput.resize(outputString.size() / 2);

        HexStringToBytes(outputString, m_expectedOutput.data(), m_expectedOutput.size());
    }

  protected:
    String m_input;
    std::vector<uint8_t> m_decodedVector;
    std::vector<uint8_t> m_expectedOutput;
};

class ASN1EncodeBase64VectorTest : public testing::TestWithParam<Base64TestInput> {
  public:
    ASN1EncodeBase64VectorTest() :
        m_expectedOutput(GetParam().second)
    { }

    virtual void SetUp()
    {
        String inputString = GetParam().first;
        m_input.resize(inputString.size() / 2);

        HexStringToBytes(inputString, m_input.data(), m_input.size());
    }

  protected:
    std::vector<uint8_t> m_input;
    String m_encodedString;
    String m_expectedOutput;
};

// The following test vectors were taken from
// RFC4648 - https://tools.ietf.org/html/rfc4648#section-10
INSTANTIATE_TEST_CASE_P(ASN1EncodeBase64Tests,
                        ANS1EncodeBase64Test,
                        ::testing::Values(Base64TestInput("", ""),
                                          Base64TestInput("f", "Zg==\n"),
                                          Base64TestInput("fo", "Zm8=\n"),
                                          Base64TestInput("foo", "Zm9v\n"),
                                          Base64TestInput("foob", "Zm9vYg==\n"),
                                          Base64TestInput("fooba", "Zm9vYmE=\n"),
                                          Base64TestInput("foobar", "Zm9vYmFy\n")));
TEST_P(ANS1EncodeBase64Test, shouldPassEncodeBase64)
{
    String actualEncodedBase64;
    QStatus status = Crypto_ASN1::EncodeBase64(m_input, actualEncodedBase64);

    EXPECT_EQ(ER_OK, status) <<
        "The function EncodeBase64 was unable to encode the string \"" <<
        m_input.c_str() << "\" to Base64 format. "
        "The status returned was: " << QCC_StatusText(status);
}

TEST_P(ANS1EncodeBase64Test, shouldCorrectlyEncodeBase64)
{
    String actualEncodedBase64;

    ASSERT_EQ(ER_OK, Crypto_ASN1::EncodeBase64(m_input, actualEncodedBase64));

    EXPECT_EQ(m_expectedOutput, actualEncodedBase64) <<
        "The string \"" << m_input.c_str() << "\" was converted to "
        "Base64 format. The result \"" << actualEncodedBase64.c_str() <<
        "\" did not match the expected value \"" <<
        m_expectedOutput.c_str() << "\".";
}

INSTANTIATE_TEST_CASE_P(ASN1DecodeBase64Tests,
                        ANS1DecodeBase64Test,
                        ::testing::Values(Base64TestInput("", ""),
                                          Base64TestInput("Zg==", "f"),
                                          Base64TestInput("Zm8=", "fo"),
                                          Base64TestInput("Zm9v", "foo"),
                                          Base64TestInput("Zm9vYg==", "foob"),
                                          Base64TestInput("Zm9vYmE=", "fooba"),
                                          Base64TestInput("Zm9vYmFy", "foobar")));
TEST_P(ANS1DecodeBase64Test, shouldPassDecodeBase64)
{
    String actualDecodedString;
    QStatus status = Crypto_ASN1::DecodeBase64(m_input, actualDecodedString);

    EXPECT_EQ(ER_OK, status) <<
        "The function DecodeBase64 was unable to decode the string \"" <<
        m_expectedOutput.c_str() << "\".";
}

TEST_P(ANS1DecodeBase64Test, shouldCorrectlyDecodeBase64)
{
    String actualDecodedString;

    ASSERT_EQ(ER_OK, Crypto_ASN1::DecodeBase64(m_input, actualDecodedString));

    EXPECT_EQ(m_expectedOutput, actualDecodedString) <<
        "The string \"" << m_input.c_str() << "\" was decoded from "
        "Base64 format. The result \"" << actualDecodedString.c_str() <<
        "\" did not match the expected value \"" <<
        m_expectedOutput.c_str() << "\".";
}

INSTANTIATE_TEST_CASE_P(ASN1EncodeBase64VectorTests,
                        ASN1EncodeBase64VectorTest,
                        ::testing::Values(Base64TestInput("", ""),
                                          Base64TestInput("66", "NjY=\n"),
                                          Base64TestInput("666f", "NjY2Zg==\n"),
                                          Base64TestInput("666f6f", "NjY2ZjZm\n"),
                                          Base64TestInput("666f6f62", "NjY2ZjZmNjI=\n"),
                                          Base64TestInput("666f6f6261", "NjY2ZjZmNjI2MQ==\n"),
                                          Base64TestInput("666f6f626172", "NjY2ZjZmNjI2MTcy\n")));
TEST_P(ASN1EncodeBase64VectorTest, shouldCorrectlyEncodeBase64)
{
    ASSERT_EQ(ER_OK, Crypto_ASN1::EncodeBase64(m_input, m_encodedString));

    EXPECT_EQ(m_expectedOutput, m_encodedString);
}

TEST(ASN1DecodeBase64VectorErrorTest, shouldReturnErrorIfBinaryValueNotMultipleOf2)
{
    std::vector<uint8_t> m_decodedVector;
    EXPECT_EQ(ER_FAIL, Crypto_ASN1::DecodeBase64("NjZm", m_decodedVector));
}

TEST(ASN1DecodeBase64VectorErrorTest, shouldReturnErrorIfBase64DoesNotMapToBinary)
{
    std::vector<uint8_t> m_decodedVector;
    EXPECT_EQ(ER_FAIL, Crypto_ASN1::DecodeBase64("ZHVwYXRhaw==", m_decodedVector));
}

TEST(ASN1DecodeBase64VectorErrorTest, shouldReturnErrorIfInputNotMultipleOf4)
{
    std::vector<uint8_t> m_decodedVector;
    EXPECT_EQ(ER_FAIL, Crypto_ASN1::DecodeBase64("ZHVwYXRhaw=", m_decodedVector));
}

INSTANTIATE_TEST_CASE_P(ASN1DecodeBase64VectorTests,
                        ASN1DecodeBase64VectorTest,
                        ::testing::Values(Base64TestInput("", ""),
                                          Base64TestInput("NjY=", "66"),
                                          Base64TestInput("NjY2Zg==", "666f"),
                                          Base64TestInput("NjY2ZjZm", "666f6f"),
                                          Base64TestInput("NjY2ZjZmNjI=", "666f6f62"),
                                          Base64TestInput("NjY2ZjZmNjI2MQ==", "666f6f6261"),
                                          Base64TestInput("NjY2ZjZmNjI2MTcy", "666f6f626172")));
TEST_P(ASN1DecodeBase64VectorTest, shouldPassEncodeBase64ForVectorInput)
{
    EXPECT_EQ(ER_OK, Crypto_ASN1::DecodeBase64(m_input, m_decodedVector));
}

TEST_P(ASN1DecodeBase64VectorTest, shouldCorrectlyEncodeBase64ForVectorInput)
{
    ASSERT_EQ(ER_OK, Crypto_ASN1::DecodeBase64(m_input, m_decodedVector));

    EXPECT_EQ(m_expectedOutput, m_decodedVector);
}


TEST(ASN1Test, decode_base64_negative_test) {
    QStatus status = ER_FAIL;

    AJ_PCSTR quote_of_stephen_colbert =
        "Twenty-two astronauts were born in Ohio. What is it about your state "
        "that makes people want to flee the Earth?";

    String raw_string = String(quote_of_stephen_colbert);
    String actual_encoded_base64;

    status = Crypto_ASN1::EncodeBase64(raw_string, actual_encoded_base64);
    EXPECT_EQ(ER_OK, status) <<
        "The function EncodeBase64 was unable to encode the string \"" <<
        raw_string.c_str() << "\" to Base64 format.";

    if (ER_OK == status) {
        String improperly_encoded_base64;
        String actual_decoded_string;

        // 1. Size of Base64 encoded data must be a multiple of 4
        improperly_encoded_base64 = "foo" + actual_encoded_base64;
        status = Crypto_ASN1::DecodeBase64(improperly_encoded_base64,
                                           actual_decoded_string);
        EXPECT_EQ(ER_FAIL, status) <<
            "The function DecodeBase64 should have rejected the improperly "
            "formatted Base64 data \"" << improperly_encoded_base64.c_str() <<
            "\" of length " << improperly_encoded_base64.length() <<
            ", which is not a multiple of 4. The status returned was: " <<
            QCC_StatusText(status);

        // 2. The number of pad '=' characters must not exceed 2
        improperly_encoded_base64 = actual_encoded_base64 + "====";
        status = Crypto_ASN1::DecodeBase64(improperly_encoded_base64,
                                           actual_decoded_string);
        EXPECT_EQ(ER_FAIL, status) <<
            "The function DecodeBase64 should have rejected the improperly "
            "formatted Base64 data \"" << improperly_encoded_base64.c_str() <<
            "\", which has more than two pad characters.";

        /*
         * 3. The Base64 encoded data must only contain characters from
         *    'The Base 64 Alphabet' (65-character subset of US-ASCII)
         *
         *    The Alphabet used for Base64 is:
         *    ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
         */

        // Remaining ASCII printable, non-whitespace, characters
        String remaining_printable_ascii_characters =
            String("`~!@#$%^&*()-_[]{}\\|;:'\",<.>/?");

        for (size_t i = 0; i < remaining_printable_ascii_characters.length();
             i++) {
            /*
             * The length of the original string is 110.
             * The length of the Base64 encoded string would be 144 (aprox)
             * The number of remaining printable ascii characters is 30
             * Hence, it is safe to insert a character at the 'i'th index
             * into Base64 encoded string.
             * 'i'th index is a crude random position to insert a character.
             */
            improperly_encoded_base64 = actual_encoded_base64.insert(
                i, &remaining_printable_ascii_characters[i]);

            status = Crypto_ASN1::DecodeBase64(improperly_encoded_base64,
                                               actual_decoded_string);
            EXPECT_EQ(ER_FAIL, status) <<
                "The function DecodeBase64 should have rejected the improperly "
                "formatted Base64 data \"" << improperly_encoded_base64.c_str() <<
                "\", which contains the character " <<
                remaining_printable_ascii_characters[i] << ", that is not in "
                "The Base 64 Alphabet. The status returned was: " <<
                QCC_StatusText(status);
        }
    }
}

TEST(ASN1Test, encode_and_decode_base64_stress_test) {
    QStatus status = ER_FAIL;

    AJ_PCSTR quote_of_atticus_finch =
        "You never really understand a person until you consider things from "
        "his point of view, until you climb inside of his skin and "
        "walk around in it.";

    AJ_PCSTR quote_of_james_bond = "Bond. James Bond.";

    String raw_data = String(quote_of_atticus_finch);
    uint8_t number_of_rounds = 100;
    for (uint8_t i = 0; i < number_of_rounds; i++) {
        raw_data += ' ';
        if (0 == Rand8() % 2) {
            raw_data += quote_of_atticus_finch;
        } else {
            raw_data += quote_of_james_bond;
        }

        String actual_encoded_base64;

        status = Crypto_ASN1::EncodeBase64(raw_data, actual_encoded_base64);
        EXPECT_EQ(ER_OK, status) <<
            "The function EncodeBase64 was unable to encode the string \"" <<
            raw_data.c_str() << "\" to Base64 format. The status returned was: " <<
            QCC_StatusText(status);

        if (ER_OK == status) {
            String actual_decoded_string;
            status = Crypto_ASN1::DecodeBase64(actual_encoded_base64,
                                               actual_decoded_string);
            EXPECT_EQ(ER_OK, status) <<
                "The function DecodeBase64 was unable to decode the string \"" <<
                actual_encoded_base64.c_str() << "\". The status returned was: " <<
                QCC_StatusText(status);

            if (ER_OK == status) {
                EXPECT_STREQ(raw_data.c_str(),
                             actual_decoded_string.c_str()) <<
                    "The string \"" << raw_data.c_str() << "\" was encoded to "
                    "Base64 format and decoded back. The decoded string \"" <<
                    actual_decoded_string.c_str() <<
                    "\" does NOT match the original.";
            }
        }
    }
}
