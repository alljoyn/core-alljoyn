/**
 * @file
 * writer.h
 */

/******************************************************************************
 *
 * This file has been directly derived from the json-cpp distribution from
 * www.json.org which has been dedicated to the Public Domain
 *
 ******************************************************************************/

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

#ifndef JSON_WRITER_H_INCLUDED
# define JSON_WRITER_H_INCLUDED

# include "value.h"
# include <vector>
# include <string>

namespace Json {

class Value;

/** \brief Abstract class for writers.
 */
class JSON_API Writer {
  public:
    virtual ~Writer();

    virtual std::string write(const Value& root) = 0;
};

/** \brief Outputs a Value in <a HREF="http://www.json.org">JSON</a> format without formatting (not human friendly).
 *
 * The JSON document is written in a single line. It is not intended for 'human' consumption,
 * but may be usefull to support feature such as RPC where bandwith is limited.
 * \sa Reader, Value
 */
class JSON_API FastWriter : public Writer {
  public:
    FastWriter();
    virtual ~FastWriter() { }

    void enableYAMLCompatibility();

  public:  // overridden from Writer
    virtual std::string write(const Value& root);

  private:
    void writeValue(const Value& value);

    std::string document_;
    bool yamlCompatiblityEnabled_;
};

/** \brief Writes a Value in <a HREF="http://www.json.org">JSON</a> format in a human friendly way.
 *
 * The rules for line break and indent are as follow:
 * - Object value:
 *     - if empty then print {} without indent and line break
 *     - if not empty the print '{', line break & indent, print one value per line
 *       and then unindent and line break and print '}'.
 * - Array value:
 *     - if empty then print [] without indent and line break
 *     - if the array contains no object value, empty array or some other value types,
 *       and all the values fit on one lines, then print the array on a single line.
 *     - otherwise, it the values do not fit on one line, or the array contains
 *       object or non empty array, then print one value per line.
 *
 * If the Value have comments then they are outputed according to their #CommentPlacement.
 *
 * \sa Reader, Value, Value::setComment()
 */
class JSON_API StyledWriter : public Writer {
  public:
    StyledWriter();
    virtual ~StyledWriter() { }

  public:  // overridden from Writer
    /** \brief Serialize a Value in <a HREF="http://www.json.org">JSON</a> format.
     * \param root Value to serialize.
     * \return String containing the JSON document that represents the root value.
     */
    virtual std::string write(const Value& root);

  private:
    void writeValue(const Value& value);
    void writeArrayValue(const Value& value);
    bool isMultineArray(const Value& value);
    void pushValue(const std::string& value);
    void writeIndent();
    void writeWithIndent(const std::string& value);
    void indent();
    void unindent();
    void writeCommentBeforeValue(const Value& root);
    void writeCommentAfterValueOnSameLine(const Value& root);
    bool hasCommentForValue(const Value& value);
    static std::string normalizeEOL(const std::string& text);

    typedef std::vector<std::string> ChildValues;

    ChildValues childValues_;
    std::string document_;
    std::string indentString_;
    int rightMargin_;
    int indentSize_;
    bool addChildValues_;
};

std::string JSON_API valueToString(Int value);
std::string JSON_API valueToString(UInt value);
std::string JSON_API valueToString(double value);
std::string JSON_API valueToString(bool value);
std::string JSON_API valueToQuotedString(const char*value);

} // namespace Json



#endif // JSON_WRITER_H_INCLUDED
