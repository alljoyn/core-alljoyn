//------------------------------------------------------------------------
// <copyright file="BasicServiceGlobals.cs" company="AllSeen Alliance.">
//     Copyright (c) 2016 Open Connectivity Foundation (OCF) and AllJoyn Open
//        Source Project (AJOSP) Contributors and others.
//
//        SPDX-License-Identifier: Apache-2.0
//
//        All rights reserved. This program and the accompanying materials are
//        made available under the terms of the Apache License, Version 2.0
//        which accompanies this distribution, and is available at
//        http://www.apache.org/licenses/LICENSE-2.0
//
//        Copyright 2016 Open Connectivity Foundation and Contributors to
//        AllSeen Alliance. All rights reserved.
//
//        Permission to use, copy, modify, and/or distribute this software for
//        any purpose with or without fee is hereby granted, provided that the
//        above copyright notice and this permission notice appear in all
//        copies.
//
//         THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//         WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
//         WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
//         AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
//         DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
//         PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
//         TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
//         PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

namespace Service_Globals
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;
    using AllJoyn;

    /// <summary>
    /// Provides all constants for the naming conventions of the service-client application
    /// </summary>
    public static class BasicServiceGlobals
    {
        /// <summary>
        /// tcp and port specifications
        /// </summary>
        public const string ConnectSpec = "tcp:addr=127.0.0.1,port=9956";

        /// <summary>
        /// advertised well-known service name for client to find 
        /// </summary>
        public const string WellKnownServiceName = "org.alljoyn.Bus.sample";

        /// <summary>
        /// relative path of 'cat' method
        /// </summary>
        public const string ServicePath = "/sample";

        /// <summary>
        /// Name of the interface implementing 'cat' method
        /// </summary>
        public const string InterfaceName = "org.alljoyn.Bus.sample";

        /// <summary>
        /// absolute path to the interface implementing 'cat' method
        /// </summary>
        public const string ServiceName = WellKnownServiceName + ServicePath;

        /// <summary>
        /// A container for the configuration of the session create for the application
        /// </summary>
        public static class SessionProps
        {
            /// <summary>
            /// session port number b/t client/service
            /// </summary>
            public const ushort SessionPort = 25;

            /// <summary>
            /// True if there are multiple point connections b/t client/service
            /// </summary>
            public const bool IsMultiPoint = false;

            /// <summary>
            /// Type of traffic communication b/t client/service
            /// </summary>
            public const TrafficType TrType = TrafficType.TRAFFIC_MESSAGES;

            /// <summary>
            /// proximity type b/t client/service
            /// </summary>
            public const ProximityType PrType = ProximityType.PROXIMITY_ANY;

            /// <summary>
            /// transport mask b/t client/service
            /// </summary>
            public const TransportMaskType TmType = TransportMaskType.TRANSPORT_ANY;
        }
    }
}