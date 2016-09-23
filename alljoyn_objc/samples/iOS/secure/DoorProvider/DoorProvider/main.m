//
//  main.m
//  DoorProvider
//
//  Created by Jorge Martinez on 9/21/16.
//  Copyright © 2016 AllseenAlliance. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#import "AJNInit.h"

int main(int argc, char * argv[]) {
    @autoreleasepool {
        if ([AJNInit alljoynInit] != ER_OK) {
            return 1;
        }
        if ([AJNInit alljoynRouterInit] != ER_OK) {
            [AJNInit alljoynShutdown];
            return 1;
        }
        int ret = UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
        [AJNInit alljoynRouterShutdown];
        [AJNInit alljoynShutdown];
        return ret;
    }
}
