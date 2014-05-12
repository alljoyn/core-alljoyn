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

#include "RulePersister.h"

#define FILE_PATH "/usr/lib/rule_engine_sample/rules.conf"

RulePersister::RulePersister() : ruleFile(FILE_PATH, std::fstream::in | std::fstream::out | std::fstream::app)
{

}

RulePersister::~RulePersister() {
    ruleFile.close();
}

void RulePersister::saveRule(Rule* rule)
{
    if (ruleFile.is_open()) {
        ruleFile << rule->toString().c_str() << std::endl;
    }
}

void RulePersister::loadRules()
{

}

void RulePersister::clearRules()
{
    ruleFile.close();
    ruleFile.open(FILE_PATH, std::fstream::in | std::fstream::out | std::fstream::truc);
}
