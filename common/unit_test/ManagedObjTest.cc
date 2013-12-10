/******************************************************************************
 *
 * Copyright (c) 2011, AllSeen Alliance. All rights reserved.
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
#include <qcc/ManagedObj.h>

using namespace qcc;

struct Managed {
    Managed() : val(0) {
        //printf("Created Managed\n");
    }

    ~Managed() {
        //printf("Destroyed Managed\n");
    }

    void SetValue(int val) { this->val = val; }

    int GetValue(void) const { return val; }

  private:
    int val;
};

TEST(ManagedObjTest, ManagedObj) {
    ManagedObj<Managed> foo0;
    EXPECT_EQ(0, (*foo0).GetValue());

    ManagedObj<Managed> foo1;
    foo1->SetValue(1);
    EXPECT_EQ(0, foo0->GetValue());
    EXPECT_EQ(1, foo1->GetValue());

    foo0 = foo1;
    EXPECT_EQ(1, foo0->GetValue());
    EXPECT_EQ(1, foo1->GetValue());

    foo0->SetValue(0);
    EXPECT_EQ(0, foo0->GetValue());
    EXPECT_EQ(0, foo1->GetValue());

}