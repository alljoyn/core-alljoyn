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

#ifndef AUTHORIZATIONDATA_H_
#define AUTHORIZATIONDATA_H_

#include <map>
#include <qcc/String.h>
#include <alljoyn/Status.h>
#include <alljoyn/MsgArg.h>
#include <iostream>

namespace ajn {
enum Action {
    DENY,
    PROVIDE,
    OBSERVE,
    MODIFY,
    PROVIDE_OBSERVE,
    PROVIDE_MODIFY
};

enum Type {
    METHOD,
    SIGNAL,
    PROPERTY
};

typedef std::map<qcc::String, Action> IfnRules;
typedef std::map<qcc::String, IfnRules> Rules;

class AuthorizationData {
  private:
    uint version;
    Rules rules;

    QStatus Marshal(ajn::MsgArg& msgarg,
                    Action a) const;

    QStatus Marshal(ajn::MsgArg& msgarg,
                    const qcc::String& s) const;

    QStatus MarshalDictEntry(ajn::MsgArg& msgarg,
                             ajn::MsgArg* key,
                             ajn::MsgArg* val) const;

    QStatus MarshalArray(ajn::MsgArg& msgarg,
                         ajn::MsgArg* elements,
                         size_t numElements) const;

    QStatus Marshal(ajn::MsgArg& msgarg,
                    IfnRules rules) const;

    const ajn::MsgArg& MsgArgDereference(const ajn::MsgArg& msgarg) const;

    QStatus Unmarshal(Action& data,
                      const ajn::MsgArg& msgarg) const;

    QStatus Unmarshal(qcc::String& data,
                      const ajn::MsgArg& msgarg) const;

    QStatus Unmarshal(IfnRules& data,
                      const ajn::MsgArg& msgarg) const;

    qcc::String ToString(Action a) const;

    qcc::String ToString(IfnRules::const_iterator i) const;

    qcc::String ToString(Rules::const_iterator i) const;

    QStatus ActionFromString(qcc::String& str, Action& a) const;

    QStatus IfnRuleFromString(qcc::String& ifn, qcc::String& str);

    QStatus RuleFromString(qcc::String& str);

  public:
    AuthorizationData();

    void AddRule(qcc::String& ifn,
                 qcc::String& mbr,
                 Type tp,
                 Action ac);

    void RemoveRule(qcc::String& ifn,
                    qcc::String& mbr,
                    Type tp,
                    Action ac);

    bool IsAllowed(qcc::String& ifn,
                   qcc::String& mbr,
                   Type tp,
                   Action ac) const;

    QStatus Marshal(ajn::MsgArg& msgarg) const;

    QStatus Unmarshal(const ajn::MsgArg& msgarg);

    QStatus Serialize(qcc::String& data) const;

    qcc::String ToString() const;

    QStatus FromString(qcc::String& str);

    QStatus Deserialize(qcc::String& data);
};
} // namespace ajn

#endif /* AUTHORIZATIONDATA_H_ */
