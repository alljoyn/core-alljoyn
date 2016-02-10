#ifndef _ALLJOYN_XML_VALIDATOR_H
#define _ALLJOYN_XML_VALIDATOR_H
/**
 * @file
 * This file defines a base class for XML validators.
 */

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

#ifndef __cplusplus
#error Only include XmlValidator.h in C++ code.
#endif

#include <unordered_set>
#include <unordered_map>
#include <string>

#include <qcc/platform.h>
#include <qcc/XmlElement.h>
#include <alljoyn/Status.h>

namespace ajn {

class XmlValidator {
  public:

    /**
     * Default destructor.
     */
    virtual ~XmlValidator() { }

  protected:

    /**
     * Validates if the qcc::XmlElement's name is equal to the expected name.
     *
     * @param[in]    xmlElement  Xml element being verified.
     * @param[in]    name        Expeceted element name.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateElementName(const qcc::XmlElement* xmlElement, AJ_PCSTR name);

    /**
     * Validates if the qcc::XmlElement contains at least one child.
     *
     * @param[in]    xmlElement              Xml element being verified.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateChildrenCountPositive(const qcc::XmlElement* xmlElement);

    /**
     * Validates if the qcc::XmlElement contains the correct number of children.
     *
     * @param[in]    xmlElement              Xml element being verified.
     * @param[in]    expectedChildrenCount   Expeceted number of children elements.
     *
     * @return
     *            #ER_OK if the input is correct.
     *            #ER_XML_MALFORMED otherwise.
     */
    static QStatus ValidateChildrenCountEqual(const qcc::XmlElement* xmlElement, uint8_t expectedChildrenCount);

    /**
     * Helper function to add a value to the set or detect duplicates.
     *
     * @param[in]    value     The value that is supposed to be added to the set.
     * @param[out]   valuesSet Set accepting the new value.
     *
     * @return #ER_OK if successfull.
     *         #ER_FAUL if a value already exists in the set.
     */
    static QStatus InsertUniqueOrFail(const std::string& value, std::unordered_set<std::string>& valuesSet);
};
} /* namespace ajn */
#endif
