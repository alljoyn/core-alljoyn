/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 */
#ifndef _NATIVEOBJECT_H
#define _NATIVEOBJECT_H

#include "Plugin.h"

class NativeObject {
  public:
    /**
     * Retains a reference to an existing NPObject*.
     */
    NativeObject(Plugin& plugin, NPObject* objectValue);
    /**
     * Creates a new NPObject* by calling "new Object();".
     */
    NativeObject(Plugin& plugin);
    virtual ~NativeObject();

    virtual void Invalidate();
    bool operator==(const NativeObject& that) const;

    Plugin plugin;
    NPObject* objectValue;
};

#endif // _NATIVEOBJECT_H