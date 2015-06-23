/**
 * @file
 *
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
#include <qcc/XmlElement.h>
#include <qcc/StringSource.h>
#include <qcc/String.h>
#include <list>
#include <string>
#include <fstream>
#include <streambuf>
#include <qcc/StaticGlobals.h>

using namespace qcc;

typedef std::vector<const XmlElement*> ChildVector;
typedef ChildVector::const_iterator ChildVectorIter;

// An "error" is used if the XML is actually invalid and would cause errors or fail to interoperate.
// A "warning" is used for most IRB guidelines.
// An "info" message is used when an IRB guideline says something is ok sometimes, with a condition
// that cannot be verified by this tool.

int g_errors = 0;
int g_warnings = 0;
int g_info = 0;
std::vector<qcc::String> g_structNames;

bool ParseFile(const qcc::String& path, qcc::String& xml)
{
    std::ifstream t(path.c_str());

    if (!t.is_open()) {
        return false;
    }

    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    xml = str.c_str();
    return true;
}

// Return true if 'v' occurs outside [].
bool HasVariant(const qcc::String& type)
{
    size_t start = 0, end, index;
    for (end = type.find_first_of('[', start); end != qcc::String::npos; end = type.find_first_of('[', start)) {
        index = type.find_first_of('v', start);
        if ((index != qcc::String::npos) && (index < end)) {
            return true;
        }
        start = type.find_first_of(']', end);

        // RULE-21: '[' in type must have a matching ']'.
        if (start == qcc::String::npos) {
            g_errors++;
            printf("ERROR-21: type '%s' is missing a matching ']'\n", type.c_str());
            return false;
        }

        // RULE-40: nested type name must have been defined.
        const qcc::String& nestedType = type.substr(end + 1, start - end - 1);
        bool found = false;
        for (size_t i = 0; i < g_structNames.size(); i++) {
            if (g_structNames[i] == nestedType) {
                found = true;
                break;
            }
        }
        if (!found) {
            g_errors++;
            printf("ERROR-40: nested type '%s' not defined\n", nestedType.c_str());
        }
    }
    index = type.find_first_of('v', start);
    if (index != qcc::String::npos) {
        return true;
    }
    return false;
}

// A camel case string cannot contain a sequence of 3 consecutive upper-case letters.
// It is legal to contain two (e.g., "DBus"), where the second one starts a word.
bool IsCamelCase(const qcc::String& value)
{
    int count = 0;
    for (size_t i = 0; i < value.length(); i++) {
        char c = value[i];
        if (isupper(c)) {
            count++;
            if (count > 2) {
                return false;
            }
        } else {
            count = 0;
        }
    }
    return true;
}

bool IsUpperCamelCase(const qcc::String& value)
{
    if (value.empty() || !isupper(value.c_str()[0])) {
        return false;
    }
    return IsCamelCase(value);
}

bool IsLowerCamelCase(const qcc::String& value)
{
    if (value.empty() || !islower(value.c_str()[0])) {
        return false;
    }
    return IsCamelCase(value);
}

const qcc::String& GetAnnotation(
    const XmlElement* interfaceElement,
    const char* annotationName)
{
    const ChildVector annotationElements = interfaceElement->GetChildren("annotation");
    for (ChildVectorIter cit = annotationElements.begin(); cit != annotationElements.end(); ++cit) {
        const XmlElement* annotation = *cit;
        const qcc::String name = annotation->GetAttribute("name");
        if (name != annotationName) {
            continue;
        }
        return annotation->GetAttribute("value");
    }
    return qcc::String::Empty;
}

int CDECL_CALL main(int argc, char** argv)
{
    if (argc != 2) {
        printf("usage: ajxmlcop <xmlfilename>\n");
        return -1;
    }

    QStatus status = qcc::Init();
    if (status != ER_OK) {
        fprintf(stderr, "qcc::Init failed (%s)\n", QCC_StatusText(status));
        return -1;
    }

    String xml;
    if (!ParseFile(argv[1], xml)) {
        fprintf(stderr, "ParseFile failed\n");
        (void) qcc::Shutdown();
        return -1;
    }
    StringSource source(xml);
    XmlParseContext parserContext(source);

    status = XmlElement::Parse(parserContext);
    if (status != ER_OK) {
        printf("Parser Error: %s\n", QCC_StatusText(status));
        (void) qcc::Shutdown();
        return -1;
    }

    // root == 'node';
    const XmlElement* root = parserContext.GetRoot();
    const ChildVector interfaceElements = root->GetChildren("interface");
    const qcc::String objectPath = root->GetAttribute("name");

    // Parse interfaces.
    for (ChildVectorIter it = interfaceElements.begin(); it != interfaceElements.end(); ++it) {
        const XmlElement* interfaceElement = *it;
        const qcc::String interfaceName = interfaceElement->GetAttribute("name");

        if ((interfaceName == "org.freedesktop.DBus.Properties") ||
            (interfaceName == "org.freedesktop.DBus.Introspectable")) {
            continue;
        }

        // RULE-1: Interface names may only use the characters A-Z, a-z, 0-9, "_" (underscore), and "." (dot).
        size_t length = interfaceName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_.");
        if (length != qcc::String::npos) {
            g_errors++;
            printf("ERROR-1: interface name '%s' contains illegal character '%c'\n",
                   interfaceName.c_str(),
                   interfaceName.c_str()[length]);
        }

        // RULE-2: The "." (dot) character is used to separate interface name components.
        if (interfaceName.find("..") != qcc::String::npos) {
            g_errors++;
            printf("ERROR-2: interface name '%s' contains illegal character sequence \"..\"\n", interfaceName.c_str());
        }

        // RULE-3: Interface names must start with the reversed DNS name of the organization that owns the interface.
        if (interfaceName.empty()) {
            g_errors++;
            printf("ERROR-3: empty interface name\n");
        } else {
            // RULE-4: The reversed DNS part of the name shall be in lower case.
            int dnsLabels = 0;
            size_t labelIndex = 0;
            for (;;) {
                size_t nextLabelIndex = interfaceName.find_first_of('.', labelIndex);
                size_t labelLength = (nextLabelIndex == qcc::String::npos) ? nextLabelIndex : nextLabelIndex - labelIndex;
                qcc::String label(interfaceName.substr(labelIndex, labelLength));
                if (label.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != qcc::String::npos) {
                    break;
                }
                dnsLabels++;
                if (nextLabelIndex == qcc::String::npos) {
                    break;
                }
                labelIndex = nextLabelIndex + 1;
            }
            if (dnsLabels == 0) {
                g_warnings++;
                printf("WARNING-4: interface name '%s' does not start with a lower case reversed DNS name\n",
                       interfaceName.c_str());
            }

            // RULE-5: The subsequent parts of an interface name use UpperCamelCase.
            qcc::String subsequentParts(interfaceName.substr(labelIndex));
            if ((labelIndex == qcc::String::npos) || !IsUpperCamelCase(subsequentParts)) {
                g_warnings++;
                printf("WARNING-5: interface name '%s' does not use UpperCamelCase in '%s'\n",
                       interfaceName.c_str(), subsequentParts.c_str());
            }

            // RULE-6: Official AllSeen Alliance names must start with "org.alljoyn", not
            // "org.allseen" or "org.allseenalliance".
            if (interfaceName.compare(0, 11, "org.allseen") == 0) {
                g_warnings++;
                printf("WARNING-6: interface name '%s' does not start with org.alljoyn\n", interfaceName.c_str());
            }

            // RULE-7: Interface names should avoid the word "Error" in their name.
            if (interfaceName.find("Error") != qcc::String::npos) {
                g_warnings++;
                printf("WARNING-7: interface name '%s' contains the word \"Error\"\n", interfaceName.c_str());
            }
        }

        // RULE-24: All interfaces ought to be secured.
        const qcc::String value = GetAnnotation(interfaceElement, "org.alljoyn.Bus.Secure");
        if (value != "true") {
            g_warnings++;
            printf("WARNING-24: interface '%s' is missing annotation org.alljoyn.Bus.Secure=\"true\"\n",
                   interfaceName.c_str());
        }

        // RULE-29: Interfaces should have descriptions.
        const XmlElement* descriptionElement = interfaceElement->GetChild("description");
        if (descriptionElement == nullptr) {
            g_warnings++;
            printf("WARNING-29: interface '%s' missing description element\n",
                   interfaceName.c_str());
        }

        // Parse structs.
        const ChildVector structElements = interfaceElement->GetChildren("struct");
        for (ChildVectorIter cit = structElements.begin(); cit != structElements.end(); ++cit) {
            const XmlElement* structElement = *cit;
            const qcc::String structName = structElement->GetAttribute("name");

            // RULE-34: Struct names must use UpperCamelCase notation without punctuation.
            // They must start with an uppercase letter.
            if (!IsUpperCamelCase(structName)) {
                g_warnings++;
                printf("WARNING-34: struct name '%s' in interface '%s' should be UpperCamelCase\n",
                       structName.c_str(), interfaceName.c_str());
            }

            // RULE-35: They must consist solely of alphanumeric characters.
            length = structName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
            if (length != qcc::String::npos) {
                g_errors++;
                printf("ERROR-35: struct name '%s' in interface '%s' contains illegal character '%c'\n",
                       structName.c_str(), interfaceName.c_str(), structName.c_str()[length]);
            }

            // RULE-36: Structs should have descriptions.
            descriptionElement = structElement->GetChild("description");
            if (descriptionElement == nullptr) {
                g_info++;
                printf("INFO-36: struct '%s.%s' missing description element\n",
                       interfaceName.c_str(), structName.c_str());
            }

            g_structNames.push_back(structName);

            // Parse fields.
            const ChildVector fieldElements = structElement->GetChildren("field");
            for (ChildVectorIter ait = fieldElements.begin(); ait != fieldElements.end(); ++ait) {
                const XmlElement* fieldElement = *ait;
                const qcc::String fieldName = fieldElement->GetAttribute("name");
                const qcc::String type = fieldElement->GetAttribute("type");

                // RULE-37: Field names must use lowerCamelCase notation without punctuation.
                // They must start with a lowercase letter.
                if (!IsLowerCamelCase(fieldName)) {
                    g_warnings++;
                    printf("WARNING-37: field name '%s' in struct '%s.%s' should be lowerCamelCase\n",
                           fieldName.c_str(), interfaceName.c_str(), structName.c_str());
                }

                // RULE-38: Field names must consist solely of alphanumeric characters.
                length = fieldName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
                if (length != qcc::String::npos) {
                    g_errors++;
                    printf("ERROR-38: field name '%s' in struct '%s.%s' contains illegal character '%c'\n",
                           fieldName.c_str(), interfaceName.c_str(), structName.c_str(), fieldName.c_str()[length]);
                }

                // RULE-50: Fields must have types.
                if (type.empty()) {
                    g_errors++;
                    printf("ERROR-50: field '%s' in struct '%s.%s' missing type attribute\n",
                           fieldName.c_str(), interfaceName.c_str(), structName.c_str());
                }

                // RULE-39: Avoid the variant type.
                if (HasVariant(type)) {
                    g_warnings++;
                    printf("WARNING-39: field '%s.%s.%s' has a variant type\n",
                           interfaceName.c_str(), structName.c_str(), fieldName.c_str());
                }
            }
            // RULE-48: Structs must have fields.
            if (fieldElements.size() == 0) {
                g_warnings++;
                printf("WARNING-48: struct '%s.%s' has no fields\n", interfaceName.c_str(), structName.c_str());
            }
        }

        // Parse methods.
        const ChildVector methodElements = interfaceElement->GetChildren("method");
        for (ChildVectorIter cit = methodElements.begin(); cit != methodElements.end(); ++cit) {
            const XmlElement* methodElement = *cit;
            const qcc::String methodName = methodElement->GetAttribute("name");

            // RULE-8: Member names must use UpperCamelCase notation without punctuation.
            // They must start with an uppercase letter.
            if (!IsUpperCamelCase(methodName)) {
                g_warnings++;
                printf("WARNING-8: method name '%s' in interface '%s' should be UpperCamelCase\n",
                       methodName.c_str(), interfaceName.c_str());
            }

            // RULE-9: They must consist solely of alphanumeric characters.
            length = methodName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
            if (length != qcc::String::npos) {
                g_errors++;
                printf("ERROR-9: method name '%s' in interface '%s' contains illegal character '%c'\n",
                       methodName.c_str(), interfaceName.c_str(), methodName.c_str()[length]);
            }

            // RULE-30: Methods should have descriptions.
            descriptionElement = methodElement->GetChild("description");
            if (descriptionElement == nullptr) {
                g_warnings++;
                printf("WARNING-30: method '%s.%s' missing description element\n",
                       interfaceName.c_str(), methodName.c_str());
            }

            const qcc::String noReply = GetAnnotation(methodElement, "org.freedesktop.DBus.Method.NoReply");

            // Parse arguments.
            const ChildVector argElements = methodElement->GetChildren("arg");
            for (ChildVectorIter ait = argElements.begin(); ait != argElements.end(); ++ait) {
                const XmlElement* argElement = *ait;
                const qcc::String argName = argElement->GetAttribute("name");
                const qcc::String type = argElement->GetAttribute("type");

                // RULE-16: Method argument names must use lowerCamelCase notation without punctuation.
                // They must start with a lowercase letter.
                if (!IsLowerCamelCase(argName)) {
                    g_warnings++;
                    printf("WARNING-16: argument name '%s' in method '%s.%s' should be lowerCamelCase\n",
                           argName.c_str(), interfaceName.c_str(), methodName.c_str());
                }

                // RULE-17: Method argument names must consist solely of alphanumeric characters.
                length = argName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
                if (length != qcc::String::npos) {
                    g_errors++;
                    printf("ERROR-17: argument name '%s' in method '%s.%s' contains illegal character '%c'\n",
                           argName.c_str(), interfaceName.c_str(), methodName.c_str(), argName.c_str()[length]);
                }

                // RULE-51: Arguments must have types.
                if (type.empty()) {
                    g_errors++;
                    printf("ERROR-51: argument '%s' in method '%s.%s' missing type attribute\n",
                           argName.c_str(), interfaceName.c_str(), methodName.c_str());
                }

                // RULE-20: Avoid the variant type.
                if (HasVariant(type)) {
                    g_warnings++;
                    printf("WARNING-20: argument '%s' in method '%s.%s' has a variant type\n",
                           argName.c_str(), interfaceName.c_str(), methodName.c_str());
                }

                // RULE-46: Method arguments need a direction.
                const qcc::String direction = argElement->GetAttribute("direction");
                if (direction.empty()) {
                    g_errors++;
                    printf("ERROR-46: method '%s.%s' argument %s missing direction attribute\n",
                           interfaceName.c_str(), methodName.c_str(), argName.c_str());
                }

                // RULE-41: Methods defined with the NoReply annotation must not return anything.
                if ((noReply == "true") && (direction == "out")) {
                    g_errors++;
                    printf("ERROR-41: NoReply method '%s.%s' contains out argument '%s'\n",
                           interfaceName.c_str(), methodName.c_str(), argName.c_str());
                }
            }
        }

        // Parse signals.
        const ChildVector signalElements = interfaceElement->GetChildren("signal");
        for (ChildVectorIter cit = signalElements.begin(); cit != signalElements.end(); ++cit) {
            const XmlElement* signalElement = *cit;
            const qcc::String signalName = signalElement->GetAttribute("name");

            // RULE-10: Member names must use UpperCamelCase notation without punctuation.
            // They must start with an uppercase letter.
            if (!IsUpperCamelCase(signalName)) {
                g_warnings++;
                printf("WARNING-10: signal name '%s' in interface '%s' should be UpperCamelCase\n",
                       signalName.c_str(), interfaceName.c_str());
            }

            // RULE-11: They must consist solely of alphanumeric characters.
            length = signalName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
            if (length != qcc::String::npos) {
                g_errors++;
                printf("ERROR-11: signal name '%s' in interface '%s' contains illegal character '%c'\n",
                       signalName.c_str(), interfaceName.c_str(), signalName.c_str()[length]);
            }

            // RULE-12: The name of a signal should be phrased in terms of past tense.
            if (signalName.compare(signalName.length() - 2, 2, "ed") != 0) {
                g_info++;
                printf("INFO-12: signal name '%s' in interface '%s' should end in a past tense verb\n",
                       signalName.c_str(), interfaceName.c_str());
            }

            // RULE-31: Signals should have descriptions.
            descriptionElement = signalElement->GetChild("description");
            if (descriptionElement == nullptr) {
                g_warnings++;
                printf("WARNING-31: signal '%s.%s' missing description element\n",
                       interfaceName.c_str(), signalName.c_str());
            }

            // RULE-53: Signals should not have a direction attribute.
            const qcc::String direction = signalElement->GetAttribute("direction");
            if (!direction.empty()) {
                g_warnings++;
                printf("WARNING-53: signal '%s.%s' should not have a direction attribute\n",
                       interfaceName.c_str(), signalName.c_str());
            }

            // RULE-23: Choose and document only one signal emission behavior.
            const qcc::String sessionless = signalElement->GetAttribute("sessionless");
            const qcc::String sessioncast = signalElement->GetAttribute("sessioncast");
            const qcc::String unicast = signalElement->GetAttribute("unicast");
            const qcc::String globalBroadcast = signalElement->GetAttribute("globalbroadcast");
            if ((sessionless != "true") && (sessioncast != "true") &&
                (unicast != "true") && (globalBroadcast != "true")) {
                g_warnings++;
                printf("WARNING-23: signal '%s.%s' missing signal behavior attribute\n",
                       interfaceName.c_str(), signalName.c_str());
            } else if (sessioncast != "true") {
                // RULE-28: Use sessioncast signals, or explain why a sessioncast
                // signal is insufficient.
                g_info++;
                printf("INFO-28: signal '%s.%s' is not sessioncast, make sure description explains why\n",
                       interfaceName.c_str(), signalName.c_str());
            } else {
                // RULE-33: Use signals for events and properties for state.
                g_info++;
                printf("INFO-33: consider changing signal '%s.%s' to a counter property that EmitsChangedSignal\n",
                       interfaceName.c_str(), signalName.c_str());
            }

            const ChildVector argElements = signalElement->GetChildren("arg");
            for (ChildVectorIter ait = argElements.begin(); ait != argElements.end(); ++ait) {
                const XmlElement* argElement = *ait;
                const qcc::String argName = argElement->GetAttribute("name");
                const qcc::String type = argElement->GetAttribute("type");

                // RULE-18: Signal argument names must use lowerCamelCase notation without punctuation.
                // They must start with a lowercase letter.
                if (!IsLowerCamelCase(argName)) {
                    g_warnings++;
                    printf("WARNING-18: argument name '%s' in signal '%s.%s' should be lowerCamelCase\n",
                           argName.c_str(), interfaceName.c_str(), signalName.c_str());
                }

                // RULE-19: Signal argument names must consist solely of alphanumeric characters.
                length = argName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
                if (length != qcc::String::npos) {
                    g_errors++;
                    printf("ERROR-19: argument name '%s' in signal '%s.%s' contains illegal character '%c'\n",
                           argName.c_str(), interfaceName.c_str(), argName.c_str(), argName.c_str()[length]);
                }

                // RULE-52: Arguments must have types.
                if (type.empty()) {
                    g_errors++;
                    printf("ERROR-52: argument '%s' in signal '%s.%s' missing type attribute\n",
                           argName.c_str(), interfaceName.c_str(), signalName.c_str());
                }

                // RULE-47: Avoid the variant type.
                if (HasVariant(type)) {
                    g_warnings++;
                    printf("WARNING-47: argument '%s' in signal '%s.%s' has a variant type\n",
                           argName.c_str(), interfaceName.c_str(), signalName.c_str());
                }
            }
        }

        const XmlElement* versionElement = nullptr;
        const ChildVector propertyElements = interfaceElement->GetChildren("property");
        for (ChildVectorIter cit = propertyElements.begin(); cit != propertyElements.end(); ++cit) {
            const XmlElement* propertyElement = *cit;
            const qcc::String propertyName = propertyElement->GetAttribute("name");
            const qcc::String type = propertyElement->GetAttribute("type");

            if (propertyName == "Version") {
                versionElement = propertyElement;
            }

            // RULE-13: Member names must use UpperCamelCase notation without punctuation.
            // They must start with an uppercase letter.
            if (!IsUpperCamelCase(propertyName)) {
                g_warnings++;
                printf("WARNING-13: property name '%s' in interface '%s' should be UpperCamelCase\n",
                       propertyName.c_str(), interfaceName.c_str());
            }

            // RULE-14: They must consist solely of alphanumeric characters.
            length = propertyName.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890");
            if (length != qcc::String::npos) {
                g_errors++;
                printf("ERROR-14: property name '%s' in interface '%s' contains illegal character '%c'\n",
                       propertyName.c_str(), interfaceName.c_str(), propertyName.c_str()[length]);
            }

            // RULE-15: Property names should be nouns or predicates.
            if (propertyName.compare(0, 3, "Get") == 0) {
                g_warnings++;
                printf("WARNING-15: property name '%s' in interface '%s' should be a noun or predicate\n",
                       propertyName.c_str(), interfaceName.c_str());
            }

            // RULE-49: Properties must have types.
            if (type.empty()) {
                g_errors++;
                printf("ERROR-49: property '%s.%s' missing type attribute\n",
                       interfaceName.c_str(), propertyName.c_str());
            }

            // RULE-22: Avoid the variant type.
            if (HasVariant(type)) {
                g_warnings++;
                printf("WARNING-22: property '%s' in interface '%s' has a variant type\n",
                       propertyName.c_str(), interfaceName.c_str());
            }

            // RULE-25: Explicitly call out the update nature of properties.
            const qcc::String value = GetAnnotation(propertyElement, "org.freedesktop.DBus.Property.EmitsChangedSignal");
            if (value.empty()) {
                g_warnings++;
                printf("WARNING-25: property '%s.%s' missing EmitsChangedSignal annotation\n",
                       interfaceName.c_str(), propertyName.c_str());
            }

            // RULE-45: Property must have an access attribute.
            const qcc::String access = propertyElement->GetAttribute("access");
            if (access.empty()) {
                g_errors++;
                printf("ERROR-45: property '%s.%s' missing access attribute\n",
                       interfaceName.c_str(), propertyName.c_str());
            }

            // RULE-26: Never create write-only properties.
            if (access == "write") {
                g_warnings++;
                printf("WARNING-26: property '%s.%s' is write-only and should be a method instead\n",
                       interfaceName.c_str(), propertyName.c_str());
            }

            // RULE-27: Strive to use read-only properties.
            if (access == "readwrite") {
                g_info++;
                printf("INFO-27: property '%s.%s' is readwrite, only appropriate if independent of all other properties\n",
                       interfaceName.c_str(), propertyName.c_str());
            }

            // RULE-32: Properties should have descriptions.
            descriptionElement = propertyElement->GetChild("description");
            if (descriptionElement == nullptr) {
                g_warnings++;
                printf("WARNING-32: property '%s.%s' missing description element\n",
                       interfaceName.c_str(), propertyName.c_str());
            }
        }

        if ((interfaceName.compare(0, 12, "org.alljoyn.") == 0) &&
            (interfaceName.compare(0, 20, "org.alljoyn.example.") != 0)) {
            if (versionElement == nullptr) {
                // RULE-42: Every standardized Interface must include a uint16 (signature ''q'') property
                // ''Version'' that indicates the implemented version of the Interface.
                g_warnings++;
                printf("WARNING-42: interface '%s' missing Version property\n", interfaceName.c_str());
            } else {
                // RULE-44: Version property should be uint16.
                const qcc::String type = versionElement->GetAttribute("type");
                if (type != "q") {
                    g_warnings++;
                    printf("WARNING-44: '%s.Version' should be a uint16 (signature 'q') property\n", interfaceName.c_str());
                }
            }
        } else {
            if (versionElement == nullptr) {
                // RULE-43: Other faces should also have a Version property.
                g_info++;
                printf("INFO-43: consider adding a uint16 Version property to interface '%s'\n", interfaceName.c_str());
            }
        }
    }

    printf("============================================================================\n");
    printf("%d errors, %d warnings, %d informational messages\n", g_errors, g_warnings, g_info);
    (void) qcc::Shutdown();
    return 0;
}
