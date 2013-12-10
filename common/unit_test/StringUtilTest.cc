/******************************************************************************
 *
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
#include <gtest/gtest.h>
#include <qcc/Util.h>
#include <qcc/StringUtil.h>

#include <cmath>

/*
 * Intentional copy-paste of code from StringUtil.cc
 *
 * StringUtil.cc #defines a constant 'NAN' and hides it (static).
 * The function StringToDouble makes use of 'NAN'. To be able to test the
 * return values of this function, we need access to 'NAN'.
 *
 * One option is to include the .cc file to get access.
 * This is not really straight-forward to do on all platforms without
 * causing disturbance.
 *
 * Other option is to shamefully copy paste the necessary lines,
 * acknowledging code duplication and increased test brittleness.
 */
#ifndef NAN
// IEEE-754 quiet NaN constant for systems that lack one.
static const unsigned long __qcc_nan = 0x7fffffff;
#define NAN (*reinterpret_cast<const float*>(&__qcc_nan))
#endif

/*
 * In the murky waters of floating point numbers, testing for NaN
 * is tricky. The only thing that language guarantees is that
 * NaN is 'unordered'. That is, NaN is:
 * a. NOT less than anything
 * b. NOT greater than anything
 * c. NOT equal to anything, including itself.
 *
 * NaN is the only one such entity for which this holds true.
 *
 * The combination of the above three characteristics makes is different
 * from INF and -INF also.
 */
#define IS_NAN(fp_val) ((fp_val < 0) || (fp_val > 0) || (fp_val == fp_val)) ? false : true

using namespace qcc;

TEST(StringUtilTest, hex_string_to_byte_array_conversion_off_by_one) {
    // String of odd length - "fee"
    String fee = String("fee");
    // The substring, "fe", of even length
    String substring_of_fee = fee.substr(0, (fee.length() - 1));

    uint8_t* bytes_corresponding_to_string = new uint8_t [fee.length() / 2];
    uint8_t* bytes_corresponding_to_substring = new uint8_t [substring_of_fee.length() / 2];

    size_t desired_number_of_bytes_to_be_copied = fee.length() / 2;
    size_t actual_number_of_bytes_copied = HexStringToBytes(fee,
                                                            bytes_corresponding_to_string,
                                                            desired_number_of_bytes_to_be_copied);

    EXPECT_EQ(desired_number_of_bytes_to_be_copied,
              actual_number_of_bytes_copied) <<
    "The function HexStringToBytes was unable to copy the entire string \"" <<
    fee.c_str() << "\" to a byte array.";

    desired_number_of_bytes_to_be_copied = substring_of_fee.length() / 2;
    actual_number_of_bytes_copied = HexStringToBytes(substring_of_fee,
                                                     bytes_corresponding_to_substring,
                                                     desired_number_of_bytes_to_be_copied);

    EXPECT_EQ(desired_number_of_bytes_to_be_copied,
              actual_number_of_bytes_copied) <<
    "The function HexStringToBytes was unable to copy the entire string \"" <<
    substring_of_fee.c_str() << "\" to a byte array.";

    // Compare the byte arrays
    for (uint8_t i = 0; i < fee.length() / 2; i++) {
        EXPECT_EQ(bytes_corresponding_to_string[i],
                  bytes_corresponding_to_substring[i]) <<
        "At arrray index " << (unsigned int) i <<
        ", element of byte array created from String \"" << fee.c_str() <<
        "\", does not match the element of byte array created from \"" <<
        substring_of_fee.c_str() << "\".";
    }

    // Clean up
    delete [] bytes_corresponding_to_string;
    bytes_corresponding_to_string = NULL;
    delete [] bytes_corresponding_to_substring;
    bytes_corresponding_to_substring = NULL;
}

TEST(StringUtilTest, hex_string_to_byte_array_conversion) {
    size_t size_of_byte_array = 0;
    size_t desired_number_of_bytes_to_be_copied = 0;
    size_t actual_number_of_bytes_copied = 0;

    bool prefer_lower_case = true;

    // String of even length - and thus should get converted completely.
    String ate_bad_f00d = String("8badf00d");
    size_of_byte_array = ate_bad_f00d.length() / 2;
    uint8_t* bytes_corresponding_to_string = new uint8_t [size_of_byte_array];

    // copy the entire string into the byte array
    desired_number_of_bytes_to_be_copied = size_of_byte_array;
    actual_number_of_bytes_copied = HexStringToBytes(ate_bad_f00d,
                                                     bytes_corresponding_to_string,
                                                     desired_number_of_bytes_to_be_copied);

    EXPECT_EQ(desired_number_of_bytes_to_be_copied,
              actual_number_of_bytes_copied) <<
    "The function HexStringToBytes was unable to convert the string \"" <<
    ate_bad_f00d.c_str() << "\" into a byte array.";

    // If string is copied into byte array completely,
    // perform the reverse conversion and verify equality.
    if (desired_number_of_bytes_to_be_copied == actual_number_of_bytes_copied) {
        size_of_byte_array = actual_number_of_bytes_copied;

        String converted_string = BytesToHexString(bytes_corresponding_to_string,
                                                   size_of_byte_array,
                                                   prefer_lower_case);

        EXPECT_STREQ(ate_bad_f00d.c_str(), converted_string.c_str()) <<
        "The string \"" << ate_bad_f00d.c_str() <<
        "\" was converted into a byte array, which was again converted back "
        "to the string \"" << converted_string.c_str() << "\".";
    }

    // Clean up
    delete [] bytes_corresponding_to_string;
    bytes_corresponding_to_string = NULL;
}

TEST(StringUtilTest, hex_string_to_byte_array_conversion_with_delimiter) {
    size_t size_of_byte_array = 0;
    size_t desired_number_of_bytes_to_be_copied = 0;
    size_t actual_number_of_bytes_copied = 0;

    bool prefer_lower_case = true;

    // String with a non-hex character and a delimiter.
    String bad_cafe = String("BA,D:,CA,FE");
    char separator = ',';
    // A non-exact upper-bound value for the size of byte array
    size_of_byte_array = bad_cafe.length() / 2;
    uint8_t* bytes_corresponding_to_string = new uint8_t [size_of_byte_array];
    // Force the function to copy both 0xBA (valid) and OxD: (invalid)
    desired_number_of_bytes_to_be_copied = 2;
    actual_number_of_bytes_copied = HexStringToBytes(bad_cafe,
                                                     bytes_corresponding_to_string,
                                                     desired_number_of_bytes_to_be_copied,
                                                     separator);

    EXPECT_NE(desired_number_of_bytes_to_be_copied,
              actual_number_of_bytes_copied) <<
    "Tried to force the HexStringToBytes function to process "
    "the non-hex-digit character \':\' of String \"" <<
    bad_cafe.c_str() << "\" and expected it to be skipped.";

    EXPECT_EQ((unsigned int) 1, actual_number_of_bytes_copied) <<
    "The function did not copy the expected number of bytes (= " << 1 <<
    ") from the string \"" << bad_cafe.c_str() << "\".";

    // If it copied one byte as expected,
    // Perform the reverse conversion and verify
    if (1 == actual_number_of_bytes_copied) {
        size_of_byte_array = actual_number_of_bytes_copied;
        prefer_lower_case = false;

        String converted_string = BytesToHexString(bytes_corresponding_to_string,
                                                   size_of_byte_array,
                                                   prefer_lower_case, separator);

        String expected_string = bad_cafe.substr(0, 2);

        EXPECT_STREQ(expected_string.c_str(), converted_string.c_str()) <<
        "Expected the string \"" << converted_string.c_str() <<
        "\" created from the byte array, to match the original string \"" <<
        expected_string.c_str() << "\".";
    }

    // Clean up
    delete [] bytes_corresponding_to_string;
    bytes_corresponding_to_string = NULL;
}

TEST(StringUtilTest, u8_to_hex_character_conversion_border_case) {
    // use a decimal number beyond the hex digits
    // Answer to the Ultimate Question of Life, The Universe, and Everything
    EXPECT_EQ('\0', U8ToChar(42));

}

TEST(StringUtilTest, hex_character_to_u8_conversion_border_case) {
    // use a character outside the hex-digit characters
    EXPECT_EQ(255, CharToU8(':'));
}

TEST(StringUtilTest, u8_hex_character_conversion_stress) {
    // run through the whole range of hex digits
    uint8_t min_hex_digit =  0;
    uint8_t max_hex_digit = 15;

    for (uint8_t i = min_hex_digit; i <= max_hex_digit; i++) {
        EXPECT_EQ(i, CharToU8(U8ToChar(i)));
    }
}

TEST(StringUtilTest, uint32_to_string_conversion_stress) {
    uint32_t some_u32 = 0;

    uint16_t number_of_iterations = 1000;

    for (uint16_t i = 0; i < number_of_iterations; i++) {
        some_u32 = Rand32();
        EXPECT_EQ(some_u32, StringToU32(U32ToString(some_u32)));
    }
}

TEST(StringUtilTest, int32_to_string_conversion_stress) {
    int32_t some_i32 = 0;

    uint16_t number_of_iterations = 1000;

    for (uint16_t i = 0; i < number_of_iterations; i++) {
        some_i32 = (int32_t) Rand32();
        EXPECT_EQ(some_i32, StringToI32(I32ToString(some_i32)));
    }
}

TEST(StringUtilTest, uint64_to_string_conversion_stress) {
    uint64_t some_u64 = 0;

    uint16_t number_of_iterations = 1000;

    for (uint16_t i = 0; i < number_of_iterations; i++) {
        some_u64 = Rand64();
        EXPECT_EQ(some_u64, StringToU64(U64ToString(some_u64)));
    }
}

TEST(StringUtilTest, int64_to_string_conversion_stress) {
    int64_t some_i64 = 0;

    uint16_t number_of_iterations = 1000;

    for (uint16_t i = 0; i < number_of_iterations; i++) {
        some_i64 = (int64_t) Rand64();
        EXPECT_EQ(some_i64, StringToI64(I64ToString(some_i64)));
    }
}

TEST(StringUtilTest, string_to_double_conversion_negative_testcases) {
    /*
     * 'NAN' is a #define and googletest's EXPECT_EQ is a #define as well
     * So, we can't quite have things viz. EXPECT_EQ(NAN, something_else)
     */
    double nan_representation = NAN;

    bool is_nan = false;

    const char* improperly_formatted_fp_string_array[] = {
        "", // empty string
        "A",
        "0.a",
        "-a",
        "-1.A",
        "0.1EA",
        "1.0E-a"
    };

    String improper_fp_string;

    for (uint8_t i = 0; i < ArraySize(improperly_formatted_fp_string_array); i++) {
        improper_fp_string = String(improperly_formatted_fp_string_array[i]);
        is_nan = IS_NAN(StringToDouble(improper_fp_string));
        EXPECT_TRUE(is_nan) <<
        "The function StringToDouble did not return: " << nan_representation <<
        ", when the string \"" << improper_fp_string.c_str() << "\" was passed."
        " The return value was: " << StringToDouble(improper_fp_string);
    }
}

TEST(StringUtilTest, string_to_double_conversion) {
    double known_double_values[] = {
        3.14159, // pi
        1.6180339887, // golden ratio
        -1 * 2.4142135623730950488, // negative silver ratio
        6.626 * pow(10.0, -34), // plancks constant
        6.022 * pow(10.0, 23) // avogadro constant
    };

    const char* string_representation[] = {
        "3.14159",
        "16.180339887E-1",
        "-2.4142135623730950488E0",
        "0.6626E-33",
        "6022E20"
    };

    for (uint8_t i = 0; i < ArraySize(known_double_values); i++) {
        String double_string = String(string_representation[i]);
        EXPECT_DOUBLE_EQ(known_double_values[i], StringToDouble(double_string)) <<
        "The StringToDouble did not return the expected value " <<
        known_double_values[i] << " when converting the string \"" <<
        double_string.c_str() << "\".";
    }
}
