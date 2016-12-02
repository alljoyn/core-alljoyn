/******************************************************************************
 *
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

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

    void SetValue(int value) { this->val = value; }

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