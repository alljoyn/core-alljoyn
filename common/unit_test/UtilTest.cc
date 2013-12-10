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

using namespace qcc;

/*
 * Even though not explicitly specified, the qcc::CRC16_Compute
 * seems to use the CRC-16/CCITT KERMIT algorithm (based on the table used).
 * For additional details on this algorithm, see:
 * http://reveng.sourceforge.net/crc-catalogue/16.htm#crc.cat.kermit
 *
 * Note: The 'CHECK' value provided in the above URL is the CRC checksum
 * value obtained, when the ASCII string "123456789" is fed through
 * the specified algorithm, as specified at:
 * http://www.ross.net/crc/download/crc_v3.txt (Section 15)
 */
TEST(UtilTest, crc16_computation_test) {
    const char ascii_string[] = "123456789";
    // 'CHECK' value
    const uint16_t expected_crc_value = 0x2189;

    // Being extra-cautious while using ArraySize on a null-terminated C string
    unsigned char buffer[ArraySize(ascii_string) - sizeof(ascii_string[0])];
    for (uint8_t i = 0; i < ArraySize(buffer); i++) {
        buffer[i] = static_cast<unsigned char> (ascii_string[i]);
    }

    uint16_t actual_crc_value = 0;
    CRC16_Compute(buffer, ArraySize(buffer), &actual_crc_value);

    EXPECT_EQ(expected_crc_value, actual_crc_value) <<
    "The CRC16_Compute did not return the expected checksum value of 0x" <<
    std::hex << expected_crc_value << " when \"" << ascii_string <<
    "\" was fed. It returned 0x" << std::hex << actual_crc_value;
}

TEST(UtilTest, crc16_computation_stress) {
    const char quote_from_the_blind_side[] =
        "Courage is a hard thing to figure. You can have courage based on "
        "a dumb idea or mistake, but you're not supposed to question adults, "
        "or your coach or your teacher, because they make the rules. Maybe "
        "they know best, but maybe they don't. It all depends on who you are, "
        "where you come from. Didn't at least one of the six hundred guys "
        "think about giving up, and joining with the other side? I mean, "
        "valley of death that's pretty salty stuff. That's why courage it's "
        "tricky. Should you always do what others tell you to do? Sometimes "
        "you might not even know why you're doing something. I mean any fool "
        "can have courage. But honor, that's the real reason for you either do "
        "something or you don't. It's who you are and maybe who you want to "
        "be. If you die trying for something important, then you have both "
        "honor and courage, and that's pretty good. I think that's what the "
        "writer was saying, that you should hope for courage and try for "
        "honor. And maybe even pray that the people telling you what to do "
        "have some, too.";

    unsigned char buffer[ArraySize(quote_from_the_blind_side) -
                         sizeof(quote_from_the_blind_side[0])];
    for (uint16_t i = 0; i < ArraySize(buffer); i++) {
        buffer[i] = static_cast<unsigned char> (quote_from_the_blind_side[i]);
    }

    uint16_t expected_crc_value = 0;
    CRC16_Compute(buffer, ArraySize(buffer), &expected_crc_value);

    /*
     * What the following does is to parition the above same buffer into
     * two pieces and compute the CRC of the first portion, followed by
     * the remaining portion. This is done by feeding the CRC of the first
     * portion as the 'runningCrc' for the remaining portion.
     * The final CRC value should be same as the above computed expected value.
     */

    uint8_t number_of_rounds = 200;
    for (uint8_t i = 0; i < number_of_rounds; i++) {
        // intentional integer division
        uint8_t parition_marker = ArraySize(buffer) / (Rand8() + 1);

        uint16_t actual_crc_value = 0;

        // CRC of the first portion
        CRC16_Compute(buffer, parition_marker, &actual_crc_value);

        // CRC of the remaining portion
        CRC16_Compute((buffer + parition_marker),
                      (ArraySize(buffer) - parition_marker),
                      &actual_crc_value);

        // Compare the two CRCs
        EXPECT_EQ(expected_crc_value, actual_crc_value) <<
        "The CRC of the string \"" <<
        String(quote_from_the_blind_side, parition_marker).c_str() <<
        "\" was computed and it was fed as the runningCrc to compute CRC of "
        " the string \"" <<
        String(quote_from_the_blind_side + parition_marker,
               (ArraySize(buffer) - parition_marker)).c_str() <<
        "\". The computed value 0x" << std::hex << actual_crc_value <<
        "does not match the CRC of the concatenated string \"" <<
        quote_from_the_blind_side << "\", which is 0x" << std::hex <<
        expected_crc_value << ".";
    }
}
