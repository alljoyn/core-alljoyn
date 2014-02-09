/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

#ifndef AJNAnnounceHandlerAdapter_H
#define AJNAnnounceHandlerAdapter_H

#import "AnnounceHandler.h"
#import "AJNAnnouncementListener.h"

/**
 AnnounceHandlerAdapter enable bind the C++ AnnounceHandler API with an objective-c announcement listener
 */
class AJNAnnounceHandlerAdapter : public ajn::services::AnnounceHandler {
public:
    // Handle to the objective-c announcement listener
    id<AJNAnnouncementListener> AJNAnnouncementListener;
    
    /**
     Constructor
     */
    AJNAnnounceHandlerAdapter(id<AJNAnnouncementListener> announcementListener);
    
    /**
     Destructor
     */
    ~AJNAnnounceHandlerAdapter();
    
    /**
     Called when a new announcement is received.
     @param version The version of the AboutService.
     @param port The port used by the AboutService.
     @param busName Unique or well-known name of AllJoyn bus.
     @param objectDescs Map of ObjectDescriptions using qcc::String as key std::vector<qcc::String>   as value, describing interfaces
     @param aboutData Map of AboutData using qcc::String as key and ajn::MsgArg as value
     */
    void Announce(uint16_t version, uint16_t port, const char* busName, const ObjectDescriptions& objectDescs, const AboutData& aboutData);
    
};
#endif