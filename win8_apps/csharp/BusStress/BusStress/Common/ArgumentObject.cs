//-----------------------------------------------------------------------
// <copyright file="ArgumentObject.cs" company="AllSeen Alliance.">
//     Copyright (c) 2012, AllSeen Alliance. All rights reserved.
//
//        Permission to use, copy, modify, and/or distribute this software for any
//        purpose with or without fee is hereby granted, provided that the above
//        copyright notice and this permission notice appear in all copies.
//
//        THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//        WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//        MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//        ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//        WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//        ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//        OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
// </copyright>
//-----------------------------------------------------------------------

namespace BusStress.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Enumeration of the different stress types that can be specified for this app
    /// </summary>
    public enum StressType 
    { 
        /// <summary>
        /// Default stress the setup and tear down of the bus attachment
        /// </summary>
        Default, 

        /// <summary>
        /// Service tests the service interaction of a bus by implementing interface
        /// for 'cat' method
        /// </summary>
        Service, 

        /// <summary>
        /// Client tests the client interaction of a bus by introspecting and calling 
        /// service method.
        /// </summary>
        Client 
    }

    /// <summary>
    /// This object encapsulates the user's input for configuring the BusStress app
    /// </summary>
    public class ArgumentObject
    {
        /// <summary>
        /// Gets or sets the number of threads to use when running Stress Operations
        /// </summary>
        public uint NumOfTasks { get; set; }

        /// <summary>
        /// Gets or sets number of iterations of execution for the threads
        /// </summary>
        public uint NumOfIterations { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the threads are to be stopped before 
        /// they're joined
        /// </summary>
        public bool StopThreadBeforeJoin { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the bus attachments are to be deleted 
        /// after the Stress Operation has finished executing
        /// </summary>
        public bool DeleteBusAttachments { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the client-service session is multipoint
        /// </summary>
        public bool IsMultipoint { get; set; }

        /// <summary>
        /// Gets or sets the the type of Stress Operation to run in this app
        /// </summary>
        public StressType StressOperation { get; set; }
    }
}
