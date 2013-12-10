//-----------------------------------------------------------------------
// <copyright file="StressManager.cs" company="AllSeen Alliance.">
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
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using AllJoyn;

    /// <summary>
    /// This class generates the tasks which run the stress tests using the input 
    /// given by the user
    /// </summary>
    public class StressManager
    {
        /// <summary>
        /// Mutex used to allow mutual exclusive when accessing TaskCount
        /// </summary>
        private static Mutex taskMutex = new Mutex();

        /// <summary>
        /// Current count of all stress operations that have been created
        /// </summary>
        private static uint taskCount = 0;

        /// <summary>
        /// Array of all the current tasks running the stress operation execution
        /// </summary>
        private Task[] tasks;

        /// <summary>
        /// Reference to the main page for this application
        /// </summary>
        private MainPage mainPage;

        /// <summary>
        /// Initializes a new instance of the <see cref="StressManager"/> class.
        /// </summary>
        /// <param name="page">Reference to the main page of the app</param>
        public StressManager(MainPage page)
        {
            this.mainPage = page;
        }

        /// <summary>
        /// Gets a static count of all the stress operations that have been created
        /// </summary>
        public static uint TaskCount
        {
            get
            {
                taskMutex.WaitOne();
                uint old = taskCount;
                taskCount++;
                taskMutex.ReleaseMutex();
                return old;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the application is currently running 
        /// stress tests or not
        /// </summary>
        public bool CurrentlyRunning { get; set; }

        /// <summary>
        /// Output message to the UI output
        /// </summary>
        /// <param name="msg">Message to output to user</param>
        public void Output(string msg)
        {
            this.mainPage.Output(msg);
        }

        /// <summary>
        /// Create the specified number of threads which will execute the specified stress
        /// type for a specified number of iterations
        /// </summary>
        /// <param name="args">Arguments obtained from the user which configure the execution 
        /// of the app</param>
        public void RunStressOperations(ArgumentObject args)
        {
            this.DebugPrint("//////////////////////////////////////////////////////////////////////////");
            this.DebugPrint("//// Starting the stress operation");
            this.DebugPrint("//////////////////////////////////////////////////////////////////////////");

            for (uint iters = 0; iters < args.NumOfIterations; iters++)
            {
                this.tasks = new Task[args.NumOfTasks];
                for (uint taskNum = 0; taskNum < args.NumOfTasks; taskNum++)
                {
                    Task t = new Task(
                        () =>
                        {
                            StressOperation stressOp = new StressOperation(TaskCount);
                            stressOp.Start(args.StressOperation, this, args.IsMultipoint);
                        });
                    t.Start();
                    this.tasks[taskNum] = t;
                }

                // Wait for all threads to finish execution
                if (args.StopThreadBeforeJoin)
                {
                    // TODO: Kill the tasks before join (Haven't found a way to accomplish this)
                }

                // Wait on all threads to finish execution
                Task.WaitAll(this.tasks, 15000);
                this.tasks = null;
            }

            this.CurrentlyRunning = false;

            this.DebugPrint("//////////////////////////////////////////////////////////////////////////");
            this.DebugPrint("//// The stress operation has finished");
            this.DebugPrint("//////////////////////////////////////////////////////////////////////////");
        }

        /// <summary>
        /// Print to the output window of the debugger
        /// </summary>
        /// <param name="msg">Message to print</param>
        public void DebugPrint(string msg)
        {
            System.Diagnostics.Debug.WriteLine(msg);
        }
    }
}
