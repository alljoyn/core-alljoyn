/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

#include "AuthorizationData.h"

namespace ajn {
AuthorizationData::AuthorizationData()
{
    version = 1;
}

void AuthorizationData::AddRule(qcc::String& ifn, qcc::String& mbr, Type tp, Action ac)
{
    rules[ifn][mbr] = ac;
}

bool AuthorizationData::IsAllowed(qcc::String& ifn, qcc::String& mbr, Type tp, Action ac) const
{
    bool result = false;

    Rules::const_iterator rit;
    IfnRules::const_iterator irit;

    rit = rules.find(ifn);
    if (rit != rules.end()) {
        irit = rit->second.find(mbr);
        if (irit != rit->second.end()) {
            result = (irit->second == ac);
        }
    }

    return result;
}

QStatus AuthorizationData::Marshal(ajn::MsgArg& msgarg, Action a) const
{
    msgarg.typeId = ajn::ALLJOYN_BYTE;
    msgarg.v_byte = a;
    return ER_OK;
}

QStatus AuthorizationData::Marshal(ajn::MsgArg& msgarg, const qcc::String& s) const
{
    msgarg.typeId = ajn::ALLJOYN_STRING;
    msgarg.v_string.str = s.c_str();
    msgarg.v_string.len = msgarg.v_string.str ? strlen(msgarg.v_string.str) : 0;
    return ER_OK;
}

QStatus AuthorizationData::MarshalDictEntry(ajn::MsgArg& msgarg,
                                            ajn::MsgArg* key,
                                            ajn::MsgArg* val) const
{
    QStatus status = ER_OK;
    msgarg.typeId = ajn::ALLJOYN_DICT_ENTRY;
    msgarg.v_dictEntry.key = key;
    msgarg.v_dictEntry.val = val;
    msgarg.SetOwnershipFlags(ajn::MsgArg::OwnsArgs);
    msgarg.Stabilize();
    return status;
}

QStatus AuthorizationData::MarshalArray(ajn::MsgArg& msgarg,
                                        ajn::MsgArg* elements,
                                        size_t numElements) const
{
    QStatus status = ER_OK;
    msgarg.typeId = ajn::ALLJOYN_ARRAY;
    status = msgarg.v_array.SetElements(elements[0].Signature().c_str(), numElements,
                                        numElements ? elements : NULL);
    if (ER_OK == status) {
        msgarg.SetOwnershipFlags(ajn::MsgArg::OwnsArgs);
        msgarg.Stabilize();
    }
    return status;
}

QStatus AuthorizationData::Marshal(ajn::MsgArg& msgarg, IfnRules rules) const
{
    QStatus status = ER_OK;
    size_t numRules = rules.size();

    ajn::MsgArg* elements;
    ajn::MsgArg* key;
    ajn::MsgArg* value;

    elements = new ajn::MsgArg[numRules];
    size_t i = 0;

    for (IfnRules::const_iterator it = rules.begin(); it != rules.end(); ++it) {
        key = new ajn::MsgArg();
        if (ER_OK != (status = Marshal(*key, it->first))) {
            break;
        }

        value = new ajn::MsgArg();
        if (ER_OK != (status = Marshal(*value, it->second))) {
            break;
        }

        // transfers ownership of key and value
        if (ER_OK != (status = MarshalDictEntry(elements[i], key, value))) {
            break;
        }

        i++;
    }

    // transfers ownership of elements
    status = this->MarshalArray(msgarg, elements, numRules);

    return status;
};

QStatus AuthorizationData::Marshal(ajn::MsgArg& msgarg) const
{
    QStatus status = ER_OK;
    size_t numRules = rules.size();

    ajn::MsgArg* elements;
    ajn::MsgArg* key;
    ajn::MsgArg* value;

    elements = new ajn::MsgArg[numRules];
    size_t i = 0;

    for (Rules::const_iterator it = rules.begin(); it != rules.end(); ++it) {
        key = new ajn::MsgArg();
        if (ER_OK != (status = Marshal(*key, it->first))) {
            break;
        }

        value = new ajn::MsgArg();
        if (ER_OK != (status = Marshal(*value, it->second))) {
            break;
        }

        if (ER_OK != (status = MarshalDictEntry(elements[i], key, value))) {
            break;
        }

        i++;
    }

    status = this->MarshalArray(msgarg, elements, numRules);

    return status;
};

QStatus AuthorizationData::Unmarshal(qcc::String& data, const ajn::MsgArg& m) const
{
    if (ajn::ALLJOYN_STRING != m.typeId) {
        return ER_FAIL;
    } else {
        data.assign(m.v_string.str, m.v_string.len);
        return ER_OK;
    }
}

QStatus AuthorizationData::Unmarshal(Action& data, const ajn::MsgArg& m) const
{
    if (ajn::ALLJOYN_BYTE != m.typeId) {
        return ER_FAIL;
    } else {
        data = static_cast<Action>(m.v_byte);
        return ER_OK;
    }
}

QStatus AuthorizationData::Unmarshal(IfnRules& data, const ajn::MsgArg& m) const
{
    QStatus status = ER_FAIL;

    if ((ajn::ALLJOYN_ARRAY == m.typeId) && (0 == data.size())) {
        size_t numElements = m.v_array.GetNumElements();

        if (numElements > 0) {
            const ajn::MsgArg* elements = m.v_array.GetElements();
            IfnRules tmp;

            for (size_t i = 0; i < numElements; ++i) {
                qcc::String k;
                if (ER_OK != (status = Unmarshal(k, *elements[i].v_dictEntry.key))) {
                    break;
                }
                Action v;
                if (ER_OK != (status = Unmarshal(v, *elements[i].v_dictEntry.val))) {
                    break;
                }
                tmp[k] = v;
            }
            if (ER_OK == status) {
                data.swap(tmp);
            }
        } else {
            status = ER_OK;
        }
    }
    return status;
}

QStatus AuthorizationData::Unmarshal(const ajn::MsgArg& m)
{
    QStatus status = ER_FAIL;

    if ((ajn::ALLJOYN_ARRAY == m.typeId) && (0 == rules.size())) {
        size_t numElements = m.v_array.GetNumElements();

        if (numElements > 0) {
            const ajn::MsgArg* elements = m.v_array.GetElements();
            Rules tmp;

            for (size_t i = 0; i < numElements; ++i) {
                qcc::String k;
                if (ER_OK != (status = Unmarshal(k, *elements[i].v_dictEntry.key))) {
                    break;
                }
                IfnRules v;
                if (ER_OK != (status = Unmarshal(v, *elements[i].v_dictEntry.val))) {
                    break;
                }
                tmp[k] = v;
            }
            if (ER_OK == status) {
                rules.swap(tmp);
            }
        } else {
            status = ER_OK;
        }
    }
    return status;
}

qcc::String AuthorizationData::ToString(Action a) const
{
    qcc::String str;
    switch (a) {
    case DENY:
        str += "D";
        break;

    case PROVIDE:
        str += "P";
        break;

    case OBSERVE:
        str += "O";
        break;

    case MODIFY:
        str += "M";
        break;

    case PROVIDE_OBSERVE:
        str += "PO";
        break;

    case PROVIDE_MODIFY:
        str += "PM";
        break;

    default:
        str += "D";
    }
    return str;
};

qcc::String AuthorizationData::ToString(IfnRules::const_iterator i) const
{
    qcc::String str;
    str += "\"";
    str += i->first;
    str += "\":";
    str += ToString(i->second);
    return str;
}

qcc::String AuthorizationData::ToString(Rules::const_iterator i) const
{
    qcc::String str;
    str += "\"";
    str += i->first;
    str += "\":{";
    for (IfnRules::const_iterator j = i->second.begin(); j != i->second.end(); ++j) {
        str += ToString(j);
    }
    str += "}";
    return str;
}

qcc::String AuthorizationData::ToString() const
{
    qcc::String str;
    str += "{\"version\":1";
    str += ",\"rules\":[";
    for (Rules::const_iterator i = rules.begin(); i != rules.end(); ++i) {
        str += ToString(i);
    }
    str += "]}";
    return str;
}

QStatus AuthorizationData::ActionFromString(qcc::String& str, Action& a) const
{
    QStatus status = ER_FAIL;

    if (str == "D") {
        a = DENY;
        status = ER_OK;
    }
    if (str == "P") {
        a = PROVIDE;
        status = ER_OK;
    }
    if (str == "O") {
        a = OBSERVE;
        status = ER_OK;
    }
    if (str == "M") {
        a = MODIFY;
        status = ER_OK;
    }
    if (str == "PO") {
        a = PROVIDE_OBSERVE;
        status = ER_OK;
    }
    if (str == "PM") {
        a = PROVIDE_MODIFY;
        status = ER_OK;
    }

    return status;
}

QStatus AuthorizationData::IfnRuleFromString(qcc::String& ifn, qcc::String& str)
{
    // TODO: add error checking
    std::size_t pos = str.find_first_of(":");
    qcc::String memberStr = str.substr(1, pos - 2).c_str();
    qcc::String actionStr = str.substr(pos + 1).c_str();
    // std::cout << "memberStr:" << memberStr << " actionStr: " << actionStr << std::endl;
    Action a;
    ActionFromString(actionStr, a);
    rules[ifn][memberStr] = a;
    return ER_OK;
}

QStatus AuthorizationData::RuleFromString(qcc::String& str)
{
    // TODO: add error checking
    std::size_t pos = str.find_first_of(":");
    qcc::String ifnStr = str.substr(1, pos - 2).c_str();
    qcc::String ifnRules = str.substr(pos + 1).c_str();

    // TODO: add loop
    std::size_t p = ifnRules.find_first_of("{");
    std::size_t p2 = ifnRules.find_first_of("}");
    qcc::String ifnRule = ifnRules.substr(p + 1, p2 - 1);
    // std::cout << "ifnStr:" << ifnStr << " ifnRule: " << ifnRule << std::endl;
    IfnRuleFromString(ifnStr, ifnRule);

    return ER_OK;
}

QStatus AuthorizationData::FromString(qcc::String& str)
{
    std::size_t p = str.find("\"rules\":[");
    std::size_t p2 = str.find("]", p);
    qcc::String rules = str.substr(p + 9, p2 - 1);
    // std::cout << "rules: " << rules << std::endl;

    // TODO: add loop
    RuleFromString(rules);

    return ER_OK;
}

QStatus AuthorizationData::Serialize(qcc::String& data) const
{
    data = ToString();
    return ER_OK;
}

QStatus AuthorizationData::Deserialize(qcc::String& data)
{
    return FromString(data);
}
} // namespace qcc
