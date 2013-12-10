//-----------------------------------------------------------------------
// <copyright file="VisualTreeHelperEx.cs" company="AllSeen Alliance.">
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

namespace Secure.Common
{
    using System.Collections.Generic;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Media;

    /// <summary>
    /// This class is used to get the first descendant of a given type from a given object.
    /// </summary>
    public static class VisualTreeHelperEx
    {
        /// <summary>
        /// Do the search for the first descendant of the given time starting at the object 'start'.
        /// This is particularly useful for finding the ScrollViewer from a TextBox.
        /// </summary>
        /// <typeparam name="T">The type we are looking for.</typeparam>
        /// <param name="start">The object to start searching from.</param>
        /// <returns>The first descendant found of type T or null if none found.</returns>
        public static T GetFirstDescendantOfType<T>(this DependencyObject start) where T : DependencyObject
        {
            T returnValue = null;
            Queue<DependencyObject> queue = new Queue<DependencyObject>();
            int count = VisualTreeHelper.GetChildrenCount(start);

            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(start, i);

                if (child is T)
                {
                    returnValue = (T)child;
                    break;
                }

                queue.Enqueue(child);
            }

            while (returnValue == null && queue.Count > 0)
            {
                var parent = queue.Dequeue();
                var count2 = VisualTreeHelper.GetChildrenCount(parent);

                for (int i = 0; i < count2; i++)
                {
                    var child = VisualTreeHelper.GetChild(parent, i);

                    if (child is T)
                    {
                        returnValue = (T)child;
                        break;
                    }
                }
            }

            return returnValue;
        }
    }
}
