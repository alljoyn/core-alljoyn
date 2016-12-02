/*
 *  * 
 *    Copyright (c) 2016 Open Connectivity Foundation and AllJoyn Open
 *    Source Project Contributors and others.
 *    
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0

 *
 */
package org.alljoyn.bus.samples.contacts_client;

import org.alljoyn.bus.annotation.Position;
/*
 * see Contact.java for description of Position annotation
 */
public class NameId{
    @Position(0)
    public String displayName;
    @Position(1)
    public int userId;
}