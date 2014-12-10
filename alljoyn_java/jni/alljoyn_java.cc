/******************************************************************************
 * Copyright (c) 2010 - 2014, AllSeen Alliance. All rights reserved.
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
 *
 ******************************************************************************/

#include <jni.h>
#include <stdio.h>
#include <assert.h>
#include <map>
#include <list>
#include <algorithm>
#include <qcc/Debug.h>
#include <qcc/Log.h>
#include <qcc/ManagedObj.h>
#include <qcc/Mutex.h>
#include <qcc/atomic.h>
#include <qcc/String.h>
#include <qcc/Thread.h>
#include <qcc/ScopedMutexLock.h>
#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/PasswordManager.h>
#include <MsgArgUtils.h>
#include <SignatureUtils.h>
#include "alljoyn_java.h"
#include <alljoyn/BusObject.h>
#include <alljoyn/Translator.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/AboutObj.h>
#include <alljoyn/version.h>

#define QCC_MODULE "ALLJOYN_JAVA"

using namespace std;
using namespace qcc;
using namespace ajn;

/*
 * Basic architectural sidebar
 * ---------------------------
 *
 * The job of the Java bindings is to translate parameters and method calls
 * from Java to C++ and vice versa.  The basic guiding philosophy in how to
 * approach this is driven by maintainability concerns and on the observation
 * that there are three times at which error may present themselves:
 *
 * 1) compile time;
 * 2) link time;
 * 3) run time.
 *
 * We think that these are listed an the order of preference: it is best to
 * have problems manifest at compile time and worst at run time.  Because of
 * this we prefer to bind Java methods to C++ BusAttachment hellper methods
 * directly whenever possible.  We prefer not to rely on AllJoyn messages
 * directly since they only fail at run-time when there's been a change.
 *
 * We believe that the Java bindings should be a very thin adapter layer.
 * Because of this, you will see Mutable parameters passed into the upper level
 * interface of the Java code which are a distinctly non-Java thing.  This is to
 * make the binding as close to the underlying C++ as possible and to maximize
 * the chances that changes to the C++ API will cause easily understandable
 * failures in the Java API as soon as possible during development.
 *
 * When there is a clear and compelling reason to prefer a more abstract
 * API, such as using Java reflection to enable a cleaner message interface,
 * we do use it, however.
 *
 * The typical idiom is to use the C++ helper API.  This is somewhat painful for
 * the programmer doing the bindings Since the JNI code turns out to be lots of
 * brain-dead translation, but then again, that is exactly what we want.  Sorry,
 * bindings developer.
 *
 * The basic idea here is to plumb calls from Java into corresponding C++
 * objects and vice versa.  The operational point of view is C++-centric.  In
 * the JNI code, we use Java reflection to reach into the Java objects to find
 * what we need and change it.  From a memory management point of view, we are
 * Java-centric to accomodate the various models for code use that are popular
 * in the wild.
 *
 * Exception handling sidebar
 * --------------------------
 *
 * When a JNI function returns an error code, it typically has also raised an
 * exception.  Exceptions aren't handled in the bindings, but are expected to
 * be propagated back up to the user code and be handled (or ignored) there.
 * It is a serious programming error to make a call to a JNI function with a
 * pending exception, which results in undefined behavior (there are documented
 * exceptions for resource management, though).  Because of this,
 * the bindings have two basic strategies available: we can stop work and
 * return or we can clear the exeption and try and proceed.  Trying to figure
 * out how to recover is not always possible, so we adopt the "stop and return"
 * approach.  Since it requires a JNI call to construct a JNI object to return
 * a status code, we must return NULL from any binding that expects a return
 * value if we get an internal JNI error.
 *
 * Returning NULL or an error code sounds like a problem since it seems that a
 * client must always check for NULL.  This is not the case.  Since an internal
 * JNI error always has an excpetion raised, we return NULL, but the Java code
 * calling the bindings never sees it.
 *
 * You may see something like the following code repeated
 * over and over again:
 *
 *   if (env->ExceptionCheck()) {
 *     QCC_LogError(ER_FAIL, ("Descriptive text"));
 *     return NULL;
 *   }
 *
 * And wonder why the Java client code never checks for the possibility of NULL
 * as a return value.  It is because the NULL case will never be seen by the
 * client code since it is always the byproduct of an exception.  This, in turn
 * means that the client never sees the NULL return value.
 *
 * Memory management sidebar
 * -------------------------
 *
 * Often, memory management approaches are some of the least obvious and most
 * problematic pieces of code.  Since we have diferent languages each with
 * fundamentally different approaches to memory management meeting at this
 * point, we pause to discuss what is happpening.
 *
 * Java uses garbage collection as a memory management strategy.  There are
 * plenty of articles written on this approach, so we don't go into any detail
 * (and the details of how garbage collection is implemented are not really
 * specified and so may vary from one JVM implementation to another anyway).  In
 * order to understand the scope of the problem, we simply note the following:
 * all Java objects are allocated on the heap.  When allocated, the program gets
 * a reference to the object.  As long as the Java garbage collector decides
 * that the program has a way to get to the object (it is reachable) the object
 * must be live and cannot be deleted.  As soon as the object can no longer be
 * reached (for example if the reference to an object is set to null), the
 * object becomes unnecesary "garbage" and is then a canditate for "garbage
 * collection."  The JVM will eventually eventually be "free" the object and
 * recycle the associated heap memory.
 *
 * C++ uses explicit memory management.  Memory on the heap must be explicitly
 * allocated, and when the memory is no longer useful, it must be explicitly
 * freed.  Because of this, it is very important in a C++ program to assign
 * responsibility for freeing objects.  This becomes especially confusing when
 * an object is allocated in one module and a pointer to the object is passed to
 * another module.  The memory management "responsibility" for the disposition
 * of the object must be explicity passed or other mechanism must be used for
 * correctly ensuring that allocated objects are freed.  We have the additional
 * complexity of objects being passed from language (Java) to another (C++) and
 * vice-versa with the attendant memory model impedance mismatches.
 *
 * Just to keep us on our toes, it turns out that the Java code can be written
 * in two completely different ways.  The first approach is to define a named
 * class, create one and retain a reference to it; which is passed into the C++
 * code by the bindings:
 *
 *   class MySessionPortListener extends SessionPortListener {
 *       public boolean acceptSesionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
 *           return true;
 *       }
 *   }
 *
 *   MySessionPortListener mySessionPortListener = new MySessionPortListener();
 *
 *   mBus.bindSessionPort(contactPortOne, sessionOptsOne, mySessionPortListener);
 *   mBus.bindSessionPort(contactPortTwo, sessionOptsTwo, mySessionPortListener);
 *
 * Notice that when this model is used, one Java object (the
 * SessionPortListener) is shared between two session ports and then used twice
 * by the C++ code.  The java client has one reference to the Java listener
 * object but there needs to be wither two C++ listener objects or two
 * references to a single C++ listener.
 *
 * The second approach possible in Java is to pass an anonymous class reference
 * directly into the Java bindings:
 *
 *   mBus.bindSessionPort(contactPortOne, sessionOptsOne, new SessionPortListener() {
 *       public boolean acceptSesionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
 *           return true;
 *       }
 *   });
 *
 *   mBus.bindSessionPort(contactPortTwo, sessionOptsTwo, new SessionPortListener() {
 *       public boolean acceptSesionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
 *           return true;
 *       }
 *   });
 *
 * Notice that there are two SessionPortListener references and neither is
 * shared.  In the first approach we previously described, the Java client code
 * is remembering the listener object and so Java garbage collection will not
 * free it; however in the anonymous class approach, the Java client code
 * immediately forgets about the callback.  If we don't do anything about it,
 * this will allow the garbage collector to free the reference when it decides
 * to and this will break the "plumbing" from C++ to Java.  This means that the
 * bindings must always acquire a global reference to a provided Java listener to
 * defend itself against the use of the anonymous class idiom.
 *
 * The picture is a little complicated, and perhaps difficult to grok without an
 * illustration, so here you go.  This is the picture from the first approach
 * discussed, where there is an explicit Java listener class created and
 * remembered by the client.
 *
 *   +-- Java Client (1)
 *   |   Strong Global Reference
 *   |
 *   v                       (5)                                 (4)
 *  +---------------+    Java Weak Ref   +--------------+   C++ Object Ref   +----------------+
 *  | Java listener | <----------------- | C++ listener | <----------------- | Session Port M |
 *  |               | -----------------> |              |                    |                |
 *  +---------------+   C++ Object Ref   +--------------+                    +----------------+
 *   ^                       (3)
 *   |
 *   +-- Bindings Strong Global Reference (2)
 *
 * (1) shows that the Java client program retains a strong global reference to
 *     the listener object it provides to the bindings.
 *
 * (2) indicates that the bindings needs to establish a strong global reference
 *     to the listener class just in case the client forgets it (perhaps
 *     intentionally if using the anonymous class idiom).
 *
 * (3) there needs to be a C++ object created to allow the bus attachment to
 *     make callbacks.  We chose to keep a one-to-one relationship between the
 *     Java object and the C++ object.  Since the Java object will always need a
 *     C++ object to do pretty much anything with the bindings, we assign
 *     responsibility for the C++ object to the Java object.  The Java listener
 *     in this example explicitly creates and destroys its associated C++
 *     objects.  This also means that we keep a C++ pointer (non reference
 *     counted) to the C++ object in the Java listener object.
 *
 * (4) when a bindings call is made (to bindSessionPort, for example) is made,
 *     the C++ object reference is passed to the bus attachment.  When the bus
 *     attachment fires a callback, it references the C++ object associated with
 *     the Java listener.
 *
 * (5) The C++ listener implementation plumbs the callbacks into the Java
 *     listener object.  To do so, it requires a reference to the Java object.
 *     This is a weak global reference to the listener because it must not
 *     prevent Java garbage collection from releasing the Java listener if both
 *     the bindings and client references are released.  The controlling
 *     reference that we use to ensure the Java listener is not garbage
 *     collected when our C++ to Java plumbing is in place is a global strong
 *     reference held by the bindings.
 *
 * If the client uses the anonymous class idiom, it will immediately forget its
 * reference to the Java listener as soon as the bindings call is completed:
 *
 *   X No reference (1)
 *   |
 *   v
 *  +---------------+    Java Weak Ref   +--------------+   C++ Object Ref   +----------------+
 *  | Java listener | <----------------- | C++ listener | <----------------- | Session Port M |
 *  |               | -----------------> |              |                    |                |
 *  +---------------+   C++ Object Ref   +--------------+                    +----------------+
 *   ^
 *   |
 *   +-- Bindings Strong Global Reference (2)
 *
 * (1) The client forgets its reference to the provided listener after the
 *     bindings call is made.
 *
 * (2) The Java listener object is kept alive by virtue of the fact that the
 *     bindings keeps a reference to the object.
 *
 * In the case of multiple anonymous listeners or multiple named listeners
 * we just see the above illustration repeateds for each listener instance.
 *
 * The picture for multiple session ports sharing a single listener is subtly
 * different.  Consider a situation where a Java client creates a named session
 * listener object and passes it to bindSessionPort twice -- once for a session
 * port M and once for a different session port N.  This is a perfectly legal
 * and expected use case.  The difference is only in two additional references.
 *
 *   +-- Java Client (1)
 *   |   Strong Global Reference
 *   v
 *  +---------------+    Java Weak Ref    +--------------+   C++ Object Ref   +----------------+
 *  | Java listener | <------------------ | C++ listener | <----------------- | Session Port M |
 *  |               | ------------------> |              | <--------+         +----------------+
 *  +---------------+   C++ Object Ref    +--------------+          |
 *   ^                                                              |         +----------------+
 *   |                                                              +-------- | Session Port N |
 *   +-- Bindings Strong Global Reference                    C++ Object Ref   +----------------+
 *   |                                                             (2)
 *   +-- Bindings Strong Global Reference (1)
 *
 * (1) shows that the bindings acquire a strong global reference to the Java
 *     listener every time through the bindings.
 *
 * (2) shows that the AllJoyn C++ code acquires a C++ pointer reference to the
 *     C++ object every time the listener object is passed into AllJoyn.
 *
 * It might not be obvious at first glance, but what is happening in our
 * implementation is that we are using the Java garbage collector to reference
 * count the C++ listener object.  This deserves a little amplification.
 *
 * As mentioned above, The whole purpose of a C++ listener is to plumb callbacks
 * from C++ to Java.  The Java listener cannot fulfil its purpose in life if it
 * doesn't have a C++ counterpart.  Therefore, the C++ listener object is
 * created when the Java object is instantiated, and destroyed when the Java
 * object is finalized, as mentioned above.  Clearly, in the picture above,
 * AllJoyn has more than one reference to the described C++ object, and it
 * cannot be deleted until all of those references are gone.  The obvious
 * approach is to reference count the C++ object.  A key observation is that we
 * need to keep the Java object around as well.  In the illustration above there
 * are three references to the Java listener object: one held by the client and
 * two held by the bindings.  This shows that the Java listener is also
 * reference counted, albeit in the Java garbage collector.  Note that the two
 * bindings strong global references (1) actually have exactly the same meaning
 * as would a reference count of two in the C++ object references (2).
 *
 * It turns out that if we add global references to support the anonymous class
 * idiom and to protect ourselves generally, we end up reference counting the
 * Java listner object.  This, in turn, means that reference counting the C++
 * object would be completely superfluous since it would just duplicate the
 * reference counting of the Java object.  We can therefore rely on Java to do
 * our work for us.
 *
 * We do have a corner case to deal with when C++ destruction order is
 * important.  This manifests, for example, when a Java bus attachment object is
 * destroyed.  We rely on Java finalizers to drive the process of associated C++
 * object cleanup.  Unfortuantely, Java finalizers may be called at any time and
 * in any order, so we can't assume that just because finalizer Y is called,
 * finalizer X must have been previously called.  C++ is sometimes not happy
 * with this situation.
 *
 * For example, if a Java bus attachment holds references to a number of
 * subsidiary Java bus objects, the bindings code will cause the Java bus object
 * references to be released before the final reference to the bus attachment
 * is.  Behind the scenes, in the garbage collector, it may be the case that the
 * Java bus attachment is actually finalized BEFORE one or more of the Java bus objects
 * since the finalize order in Java is undefined.

 * There are underlying C++ objects, the Java bus objects may expect the C++
 * backing object for the bus attachment to exist when they are executed.  We
 * must therefore keep the bus attachment around until all of the bus objects
 * are destroyed before deleting the it.  This is the perfect use for a
 * reference counted smart pointer.  It is tempting to use qcc::ManagedObj to
 * reference count the underlying bus attachemnt C++ object, but it really isn't
 * a smart pointer.  It is subtly different.  The major complication is that
 * there is no such thing as a NULL Managed object that doesn't point to a live
 * object.
 *
 * Even if we do work around the problems and shoehorn in a qcc::ManagedObj one
 * is always tempted to think of those ManagedObj things as smart pointers and
 * even typedef and them that way.  This can introduce some hard to find and
 * very subtle bugs.  Because of this, we just build an intrusive reference
 * count into our C++ bus attachment object and be done with it.  It's very
 * simple and straightforward that way, and you can think of your reference
 * counted bus attachment as a reference counted bus attachment without worry.
 *
 * Bus objects sidebar
 * -------------------
 *
 * As you may have gathered from the memory management sidbar, Bus Objects are
 * well, just different.  They are different in order to support a simple
 * programming model for simple services, and because Java does not support
 * multiple inheritance.  This leads to some fairly significant complexity, and
 * the underlying reason is not intuitively obvious, so we spend some time
 * discussing them here.
 *
 * In a larger application, one typically thinks of a model that holds the state
 * of the application and a number of what are essentially Views and Controllers
 * of an MVC application to talk to the network.  For example one might consider
 * a high level architecture that looks something like,
 *
 *   +-----------+    +-----------+    +-----------+
 *   | Graphical |    |  Android  |    |  AllJoyn  |
 *   |   User    |    |   Binder  |    |    Bus    |
 *   | Interface |    | Interface |    | Interface |
 *   +-----------+    +-----------+    +-----------+
 *         |                |                |
 *   +---------------------------------------------+
 *   |              Application Model              |
 *   +---------------------------------------------+
 *
 * The "AllJoyn Bus Interface" would be implemented as a class, and it would
 * basically be a single-threaded (applications with GUIs are almost universally
 * single threaded) thing talking to associate AllJoyn objects, one of which
 * would be a BusObject that implements some service.  It might look something
 * like the following illustration.
 *
 *                                           |  (From Application Model)
 *                                           v
 *                                +-----------------------+
 *                                | AllJoyn Bus Interface |
 *                                +-----------------------+
 *                                    |    |   |    |
 *            +-----------------------+    |   |    +--------------------------+
 *            |                   +--------+   +---------+                     |
 *            |                   |                      |                     |
 *   +----------------+    +--------------+    +--------------------+    +------------+
 *   | Bus Attachment |    | Bus Listener |    | SessionPortListeer |    | Bus Object |
 *   +----------------+    +--------------+    +--------------------+    |            | <----> Network
 *                                                                       | Interface  |
 *                                                                       |  Methods   |
 *                                                                       +------------+
 *                                                                             |
 *                                                     (To Application Model)  v
 *                                                     (via Getters/Setters)
 *
 * Notice that in this case, the AllJoyn Bus Interface HASA Bus Object in the
 * object oriented programming sense.  The important observation to make is that the
 * service interfaces are duplicated in the Bus Object and Application Model.  This
 * is an artifact of the centralized Model (in the Model-View-Controller sense).
 *
 * On the other hand, in a very simple service, it makes sense to include the
 * Model in the Bus Object.  One might want to construct an entirely self-
 * contained AllJoyn-only object, which might look something like the following
 * illustration.
 *
 *                      +-----------------------+
 *                      |    Simple Service     |
 *                      |                       |
 *                      | OS Service Interface  |
 *                      | AllJoyn Bus Interface |
 *                      |                       |
 *                      |      Bus Object       |
 *                      |  Interface Methods    | <----> Network
 *                      +-----------------------+
 *                             |    |    |
 *              +--------------+    |    +------------------+
 *              |                   |                       |
 *     +----------------+    +--------------+    +--------------------+
 *     | Bus Attachment |    | Bus Listener |    | SessionPortListeer |
 *     +----------------+    +--------------+    +--------------------+
 *
 * Notice that in this case, the simple service wants to inherit from a concrete
 * platform-dependent service class (for example the Android Service class) so
 * it ISA (in the object-oriented architecture sense) OS Service Interface and
 * it ISA Bus Object.
 *
 * Since Java does not support multiple inheritance, the only way to accomplish
 * this is to make either the Bus Object or the OS Service definition a Java
 * "interface".  Since we are unlikely to convince every OS manufacturer to
 * accomodate us, this means that BusObject needs to be an interface.
 *
 * What this means to us here is that we cannot enforce that clients put a
 * "handle" in implementations.  This means that we must treat Bus Objects
 * differently than objects which we have control over.
 *
 * Another complication is that the Java part of the bindings make it seem like
 * you should be able to register a bus object with multiple bus attachments
 * since there is no reference to a bus attachment visible anywhere from that
 * perspective.  The problem is that there is a hidden reference to the bus
 * attachment way down in the AllJoyn C++ BusObject code.  We need to reference
 * count the bus attachment as desribed in the memory management sidebar; so
 * we need to be able to increment and decrement references as bus objects
 * are created and destroyed.  This means there really is a reference to a
 * single bus attachment in the bus object, and regitering bus objects with
 * more than one bus attachment is not possible.
 *
 * Java Bus Objects do not have an explicit reference, nor is there a way to put
 * them into an interface, so this makes them further different from the other
 * objects in the system since there is no one-to-one relationship with a Bus
 * Attachment in the Java Object.  Signal Emitters are also associated with a
 * Java Object that implements given interfaces, but not with anything else.
 * Since there is no state in the Bus Object, we need to provide enough external
 * scaffolding to make the connection to the C++ Object that backs up the Java
 * Bus Object.
 *
 * This all results in a rather intricate object relationship which deserves an
 * illustration of its own.
 *
 *              +--- Bindings Strong Reference (7)
 *              |
 *              |              (1)                                                  (2) (8)
 *      +-------------+   Java Weak Ref  +--------------+    C++ Object Ref    +----------------+
 *      | Java Object | <--------------- |  C++ Object  | <------------------- |    AllJoyn     |
 *      |             |                  |              | --------+            | Bus Attachment |
 *      |   Extends   |                  |  Implements  |         |            +----------------+
 *      |  Bus Object |                  |  Bus Object  |         |                         ^
 *      |  Implements |                  | Methods from |         |                         |
 *      |  Interface  |                  |   C++ class  |         |                         |
 *      |  Interface  |                  +--------------+         | (3) Pointer to          |
 *      |     ...     |                         ^                 |     refcounted          |
 *      +-------------+                         |                 |     object              |
 *             ^                                |                 |                         |
 *             |                                |                 |                         |
 *     +----------------+                       |                 |                         |
 *     | Signal Emitter | (5)                   |                 |                         |
 *     +----------------+                       |                 |                         | (4) C++ Bus Attachment
 *             |                                |                 |                         |     ISA AllJoyn Bus
 *             v                                |                 |                         |     Attachment
 *     [Java Object, Ref Count, C++ Object] ----+ (6) (7)         |                         |
 *             ^                                                  |                         |
 *             |                                                  |                         |
 *             +--------------------------------------------------)---------------+         |
 *                                                                |               |         |
 *                                           +--------------------+               |         |
 *                                           |                                    |         |
 *                                           v                                    |         |
 *     +---------------------+     +--------------------+                         |         |
 *     | Java Bus Attachment | --> | C++ Bus Attachment | --> [Java Bus Object] --+ (8) (9) |
 *     +---------------------+     +--------------------+     [Java Bus Object]             |
 *                                           |                                              |
 *                                           |                                              |
 *                                           +----------------------------------------------+
 *
 * (1) As usual, there is a one-to-one relationship between the provided Java
 *     object and the associated C++ object, but the relationship is one-way
 *     since there is no bindings state in the Java Object.  The Java reference
 *     in the C++ object is used to plumb the calls from AllJoyn through to the
 *     Java interface implementations.
 *
 * (2) When the bindings RegisterBusObject method is called, a reference to
 *     the C++ method is given to AllJoyn.  Since the Java Object has no
 *     concrete state, the C++ object cannot be created when the Java object
 *     is created, but must be created on-demand in the RegisterBusObject
 *     method.
 *
 * (3) Because of finalizer ordering uncertainty, the AllJoyn Bus Attachment
 *     must remain instantiated until all Bus Objects are completely destroyed.
 *     Because of this, the C++ backing object for the AllJoyn Bus Attachment
 *     is reference counted.  The backing C++ object for the Java Bus Object
 *     holds a reference to the backing C++ Object to the Java Bus Attachment
 *     which is in turn refers to the AllJoyn Bus Attachment.
 *
 * (4) Although the relationship between the C++ Bus Attachment and the AllJoyn
 *     Bus Attachment is illustrated with a pointer, the C++ Bus Attachment
 *     actually inherits from the AllJoyn Bus Attachment.
 *
 * (5) Signal Emitters have a reference to the Java Bus Object with which they
 *     are associated.  In order to actually emit signals, the C++ object
 *     associated with the Java Bus Object must be looked up.  This is done
 *     by looking up the Java Object reference in a global gBusObjectMap.
 *
 * (6) In the normal (not Bus Object) case, we use the Java garbage collector to
 *     reference count our Java objects, and override the finalize() method of
 *     the target object to drive the free of the underlying C++ object.  Since
 *     we have no ability to affect the finalize() method of a Bus Object, we
 *     can no longer rely on the Java GC and have to provide our own reference
 *     count.  The reference count is interpreted as the number of times that
 *     registerBusObject has been called.  This can currently be exactly once.
 *
 * (7) Whenever a Java Bus Object is registered with a Bus Attachment, one JNI
 *     strong global reference is taken to the object.  This ensures that the
 *     Java object is not released while the bindings are using it.  When a Bus
 *     Object is registered for the first time, there will be no entry in the
 *     global Bus Object to C++ Object map.  In this case,a new C++ Object is
 *     created and associated with the Java Object via the global map.  If the
 *     same object is registered more than once (currently not possible), the
 *     reference count in the map entry is incremented.  When a Bus Object is
 *     unregistered, One JNI reference to the object is released.  The mapping
 *     between Java object and C++ object is determined from the global object
 *     map and the reference count there is decremented.  If the reference count
 *     goes to zero, the C++ object is freed and the map entry removed.
 *
 * (8) Whenever a Bus Attachent is destroyed, we want to be able to remove all
 *     of the bindings references to Java Bus Objects and delete any C++ objects
 *     that are no longer necessary.  We must have a list of Java Bus Objects in
 *     each Bus Attachment for cleanup purposes.
 *
 * (9) The detail to be aware of in (8) is that since a BusObject is an
 *     interface we have no way to know when the user is done with a particular
 *     BusObject.  If we had our hands on it, we could know when the user is
 *     done by hooking the finalizer.  We do want to make sure that the resources
 *     allocated to the BusAttachment are not held up by a user forgetting to
 *     Unregister a bus object.  Do enable this, we run through our list of
 *     Java bus objects in the BusAttachment finalizer and unregister all of the
 *     bus objects.  This is okay since the BusAttachment is completely stopped
 *     and we know we'll never call out to the correcponding Java objects again.
 *     This way, a memory leak in a Java BusObject just leaks the user object.
 *
 *     The tricky bit is that the JBusAttachment is reference counted, so it is
 *     only deleted when its reference count is decremented to zero; but each of
 *     the BusObjects hods a reference to the JBusAttachment.  We have to
 *     release the BusObject reference counts in order to get the destructor to
 *     run, so these releases must happen elsewhere.  Elsewhere is in the bus
 *     attachment finalize function out in Java-land, where we call in with a
 *     destroy method that indicates that a final shutdown is happening.
 *
 * To summarize, this is quite a bit of complexity for this particular case,
 * but it supports a required API feature, which is that the BusObject be an
 * interface.
 *
 * One final note is that in all of the classes you will find below, unless
 * there is an explicit need, copy constructors and assignment operators will be
 * declared private with no provided implementation.  This is so that the
 * compiler generates an error instead of generating a most likely incorrect
 * default implementation if someone decides to use one unexpectedly.
 *
 * Multithread safety sidebar
 * --------------------------
 *
 * Threading models, like memory management models come in different flavors.
 * Just as in the memory management situation described above, the bindings are
 * the place where the Java threading model and the Posix threading models meet.
 * We say Posix threading model since there really is no C++ or C threading
 * model, and AllJoyn for Linux-based systems uses Posix threads wrapped by an
 * OS abstraction layer.
 *
 * We assume that AllJoyn is multithread safe since it advertises itself that
 * way.  We assume that multiple threads of execution may come at us from the
 * AllJoyn side since there are multiple ways to get notifications.  In practice
 * notifications may be serialized since they are coming from one endpoint
 * receive thread, but we don't rely on that since it is a behavior that can
 * eisly be changed.
 *
 * We assume that Java is capable of multithreading and will most likely be
 * running hardware threads.  This means that multiple threads of execution may
 * come at us from the Java side as well.  The big picture is illustrated below
 * showing three areas of responsibility.
 *
 *  {Client Responsibility}     {Bindings Responsibility}      {AllJoyn Responsibility}
 *           (1)                           (3)                          (2)
 *
 *              +--------+     +---------+     +---------+     +---------+
 *   Thread --> |        | --> |         | --> |         | --> |         |
 *   Thread --> | Client | --> | Binding | --> | Binding | --> | AllJoyn |
 *              |  Java  | <-- |  Java   | <-- |   C++   | <-- |   C++   | <-- Thread
 *              |        | <-- |         | <-- |         | <-- |         | <-- Thread
 *              +--------+     +---------+     +---------+     +---------+
 *
 * (1) The client is responsibile, at a minimum, of understanding that
 *     notifications will come in on at least one separate thread and may come
 *     in over any number of threads.  The client may spin up any number of
 *     threads and it is its own responsibility for deciding how to manage them
 *     and its own multithreading issues.  This is pointed out numerous times
 *     in the bindings documentation.
 *
 * (2) AllJoyn is responsible for being able to deal with any number of threads
 *     accessing its objects.  In turn, it can send notifications up to the
 *     bindings on any number of threads.  We do not concern ourselves about
 *     how AllJoyn does this, only that it claims to be multithread safe; we
 *     just believe it.
 *
 * (3) The bindings must be able to pass commands from the Java client through
 *     to AllJoyn over any number of threads and be able to pass notifications
 *     back into the client over any number of threads.  We must be able to
 *     guarantee multithread safety in this environment.
 *
 * There is a lot of terminology thrown around which seems to mean different
 * things on different systems, but what we need is the ability to serialize
 * access to objects.  Java likes to use the concept of Monitors and Posix
 * likes to use binary semaphores or mutual exclusion objects which it calls
 * Mutexes.  At a low level, they're really the same thing.
 *
 * If looking in from a Java perspective, one typically thinks of a group of
 * methods in a given object that are automatically associated with a Monitor
 * by using the synchronize keyword.  Only one method is allowed to proceed
 * through the Monitor and begin execution at any given time.  The Java JNI
 * provides access to Monitors via the MonitorEnter(obj) and MonitorExit(obj)
 * functions which take Java Objects as parameters.  The MonitorEnter function
 * allows exactly one thread to pass and blocks the "gate" until that one
 * thread calls MonitorExit.  During the time the "gate" is closed, other
 * threads are suspended and will not execute.
 *
 * If looking in from a Posix perspective, one typically thinks of a mutual
 * exclusion object that one uses to serialize access to a number of related
 * functions.  The most common idiom is to place a mutex object into an object
 * as a member variable.  In the AllJoyn OS abstraction layer the Mutex object
 * has methods lock() and unlock().  The lock method allows exactly one thread
 * to pass and blocks the "gate" until that one thread calls unlock.  During
 * the time the "gate" is closed, other threads are suspended and will not
 * execute.
 *
 * The Java Virtual Machine specifies what thread-related functions must be
 * implemented by a specific Java Runtime.  It is possible that a given Java
 * Runtime can handle all thread-related functions itself.  Sun called these
 * runtimes "green thread" runtimes.  Typically, however, on machines that
 * support native threads, the Java runtime also uses native threads, although
 * this may be configurable.
 *
 * The question immediately arises whether or not Java native threads are
 * compatible with the pthreads that AllJoyn uses.  In the case of Android, each
 * Dalvik thread explicitly maps directly to a native pthread.  This is usually
 * the case with Sun JVMs, but the Solaris JDK is a notable exception.  We will
 * assume here that there are no horrible side effects from using JNI critical
 * sections in code executing under pthreads or vice versa; and that mutual
 * exclusion actually happens when we ask for it.
 *
 * The first choice we have is to use Posix or Java implementation of mutual
 * exclusion since they are functionally equivalent.  We want to keep the
 * multithread aware code as limited in scope as possible.  That is, we don't
 * want to sprinkle synchronized keywords through the Java objects of the
 * bindings since these can be removed without necessarily auditing the JNI
 * code.  In addition, new methods can be added and objects specialized; and
 * unless one is keeping multithreading in mind, it is easy to add that method
 * without the synchronized keyword.  Since synchronized methods compose a
 * monitor on the entire object, the exclusion is on a per-method basis.  We
 * don't want to exclude access, for example, to the bus attachment while a
 * remote method is being called.  This implies a finer granularity than
 * per-method.
 *
 * So we choose the Posix version of mutual exclusion for two reasons: first, we
 * want to keep the scope of the multithread problem contained to one place, and
 * that place is C++, we should use the "natural" method for C++ which is
 * pthreads.  Second, since we want a finer granularity than per-method (we want
 * to protect shared resources rather than just arbitrarily serialize access to
 * an entire object) we should use the mechanism oriented to that approach,
 * which is the pthreads mutex as encapsulated by the AllJoyn OS abstraction
 * layer.
 *
 * The JNI code's purpose for living is to make connections between Java and
 * C++.  To do this, it generally creates C++ objects and ties them to the
 * Java objects as described in the memory management section.  The C++ objects
 * constitute the plumbing between the Java system and the AllJoyn system and
 * serve as the ideal place to centralize the bindings multi-thread safe code.
 *
 * Referring back to the illustration above, there are a lot of implications
 * for the bindings.  The bindings are split roughly in half -- into a set of
 * Java classes and corresponding C++ classes found in this file.  The
 * illustration implies that the Java code found in src/org/alljoyn/bus should
 * be multithread safe (Safe or MT-Safe in the jargon of Linux libraries) and
 * we should be multithread safe here.
 *
 * What this means to us is that there must be zero intances of unprotected
 * read-modify-write patterns in those objects if they are going to rely on the
 * C++ code here for their thread safety.  This includes uses of arrays, lists
 * and collections in general since they read-modify-write the data structures
 * that access the collections.
 *
 * Listener objects are the Java and C++ objects called from AllJoyn on one of
 * its threads.  All of our C++ listener objects are expected to be MT-Safe
 * between construction and destruction.  We have control of that here.  A
 * similar situation should exist in the Java objects, but we lose control of
 * them at the client.  We expect that clients of the bindings will understand
 * that they must be MT-Safe.  We don't try to come up with some one size fits
 * all solution; we expect the users to understand that they may have multiple
 * threads running around in parts of their code.
 *
 * Another important question is whether or not all of the env->function() JNI
 * calls are thread safe.  The answer is that it is implementation dependent and
 * so we can make no such assumption.  There is certainly one env pointer
 * per-thread, and all JNI functions are accessed through the env pointer, but
 * there is no requirement that a given JNI implementation take a lock on an
 * object while manipulating it.  Local references are stashed in thread local
 * storage, are accessible only by the current thread and so are not of concern,
 * but we do have to serialize accesses to global shared objects whenever
 * appropriate.
 */

/** The cached JVM pointer, valid across all contexts. */
static JavaVM* jvm = NULL;

/** java/lang cached items - these are guaranteed to be loaded at all times. */
static jclass CLS_Integer = NULL;
static jclass CLS_Object = NULL;
static jclass CLS_String = NULL;

/** org/alljoyn/bus */
static jclass CLS_BusException = NULL;
static jclass CLS_ErrorReplyBusException = NULL;
static jclass CLS_IntrospectionListener = NULL;
static jclass CLS_IntrospectionWithDescListener = NULL;
static jclass CLS_BusObjectListener = NULL;
static jclass CLS_MessageContext = NULL;
static jclass CLS_MsgArg = NULL;
static jclass CLS_Signature = NULL;
static jclass CLS_Status = NULL;
static jclass CLS_Variant = NULL;
static jclass CLS_BusAttachment = NULL;
static jclass CLS_SessionOpts = NULL;
static jclass CLS_AboutDataListener = NULL;

static jmethodID MID_Integer_intValue = NULL;
static jmethodID MID_Object_equals = NULL;
static jmethodID MID_BusException_log = NULL;
static jmethodID MID_MsgArg_marshal = NULL;
static jmethodID MID_MsgArg_marshal_array = NULL;
static jmethodID MID_MsgArg_unmarshal = NULL;
static jmethodID MID_MsgArg_unmarshal_array = NULL;


// predeclare some methods as necessary
static jobject Unmarshal(const MsgArg* arg, jobject jtype);
static MsgArg* Marshal(const char* signature, jobject jarg, MsgArg* arg);

/**
 * Get a valid JNIEnv pointer.
 *
 * A JNIEnv pointer is only valid in an associated JVM thread.  In a callback
 * function (from C++), there is no associated JVM thread, so we need to obtain
 * a valid JNIEnv.  This is a helper function to make that happen.
 *
 * @return The JNIEnv pointer valid in the calling context.
 */
static JNIEnv* GetEnv(jint* result = 0)
{
    JNIEnv* env;
    jint ret = jvm->GetEnv((void**)&env, JNI_VERSION_1_2);
    if (result) {
        *result = ret;
    }
    if (JNI_EDETACHED == ret) {
#if defined(QCC_OS_ANDROID)
        ret = jvm->AttachCurrentThread(&env, NULL);
#else
        ret = jvm->AttachCurrentThread((void**)&env, NULL);
#endif
    }
    assert(JNI_OK == ret);
    return env;
}

/**
 * Inverse of GetEnv.
 */
static void DeleteEnv(jint result)
{
    if (JNI_EDETACHED == result) {
        jvm->DetachCurrentThread();
    }
}

/*
 * Note that some JNI calls do not set the returned value to NULL when
 * an exception occurs.  In that case we must explicitly set the
 * reference here to NULL to prevent calling DeleteLocalRef on an
 * invalid reference.
 *
 * The list of such functions used in this file is:
 * - CallObjectMethod
 * - CallStaticObjectMethod
 * - GetObjectArrayElement
 */
static jobject CallObjectMethod(JNIEnv* env, jobject obj, jmethodID methodID, ...)
{
    va_list args;
    va_start(args, methodID);
    jobject ret = env->CallObjectMethodV(obj, methodID, args);
    if (env->ExceptionCheck()) {
        ret = NULL;
    }
    va_end(args);
    return ret;
}

static jobject CallStaticObjectMethod(JNIEnv* env, jclass clazz, jmethodID methodID, ...)
{
    va_list args;
    va_start(args, methodID);
    jobject ret = env->CallStaticObjectMethodV(clazz, methodID, args);
    if (env->ExceptionCheck()) {
        ret = NULL;
    }
    va_end(args);
    return ret;
}

static jobject GetObjectArrayElement(JNIEnv* env, jobjectArray array, jsize index)
{
    jobject ret = env->GetObjectArrayElement(array, index);
    if (env->ExceptionCheck()) {
        ret = NULL;
    }
    return ret;
}

/**
 * Implement the load hook for the alljoyn_java native library.
 *
 * The Java VM (JVM) calls JNI_OnLoad when a native library is loaded (as a
 * result, for example, of a System.loadLibrary).  We take this opportunity to
 * Store a pointer to the JavaVM and do as much of the fairly expensive calls
 * into Java reflection as we can.  This is also useful since we may not have
 * access to all of the bits and pieces in all contexts, so it is useful
 * to get at them all where/when we can.
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm,
                                  void* reserved)
{
    QCC_UseOSLogging(true);
    jvm = vm;
    JNIEnv* env;
    if (jvm->GetEnv((void**)&env, JNI_VERSION_1_2)) {
        return JNI_ERR;
    } else {
        jclass clazz;
        clazz = env->FindClass("java/lang/Integer");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_Integer = (jclass)env->NewGlobalRef(clazz);

        MID_Integer_intValue = env->GetMethodID(CLS_Integer, "intValue", "()I");
        if (!MID_Integer_intValue) {
            return JNI_ERR;
        }

        clazz = env->FindClass("java/lang/Object");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_Object = (jclass)env->NewGlobalRef(clazz);

        MID_Object_equals = env->GetMethodID(CLS_Object, "equals", "(Ljava/lang/Object;)Z");
        if (!MID_Object_equals) {
            return JNI_ERR;
        }

        clazz = env->FindClass("java/lang/String");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_String = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/BusException");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_BusException = (jclass)env->NewGlobalRef(clazz);
        MID_BusException_log = env->GetStaticMethodID(CLS_BusException, "log", "(Ljava/lang/Throwable;)V");
        if (!MID_BusException_log) {
            return JNI_ERR;
        }

        clazz = env->FindClass("org/alljoyn/bus/ErrorReplyBusException");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_ErrorReplyBusException = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/IntrospectionListener");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_IntrospectionListener = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/IntrospectionWithDescriptionListener");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_IntrospectionWithDescListener = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/BusObjectListener");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_BusObjectListener = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/AboutDataListener");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_AboutDataListener = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/MsgArg");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_MsgArg = (jclass)env->NewGlobalRef(clazz);
        MID_MsgArg_marshal = env->GetStaticMethodID(CLS_MsgArg, "marshal", "(JLjava/lang/String;Ljava/lang/Object;)V");
        if (!MID_MsgArg_marshal) {
            return JNI_ERR;
        }
        MID_MsgArg_marshal_array = env->GetStaticMethodID(CLS_MsgArg, "marshal", "(JLjava/lang/String;[Ljava/lang/Object;)V");
        if (!MID_MsgArg_marshal_array) {
            return JNI_ERR;
        }
        MID_MsgArg_unmarshal = env->GetStaticMethodID(CLS_MsgArg, "unmarshal", "(JLjava/lang/reflect/Type;)Ljava/lang/Object;");
        if (!MID_MsgArg_unmarshal) {
            return JNI_ERR;
        }
        MID_MsgArg_unmarshal_array = env->GetStaticMethodID(CLS_MsgArg, "unmarshal", "(Ljava/lang/reflect/Method;J)[Ljava/lang/Object;");
        if (!MID_MsgArg_unmarshal_array) {
            return JNI_ERR;
        }

        clazz = env->FindClass("org/alljoyn/bus/MessageContext");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_MessageContext = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/Signature");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_Signature = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/Status");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_Status = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/Variant");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_Variant = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/BusAttachment");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_BusAttachment = (jclass)env->NewGlobalRef(clazz);

        clazz = env->FindClass("org/alljoyn/bus/SessionOpts");
        if (!clazz) {
            return JNI_ERR;
        }
        CLS_SessionOpts = (jclass)env->NewGlobalRef(clazz);

        return JNI_VERSION_1_2;
    }
}

/**
 * A helper class to wrap local references ensuring proper release.
 */
template <class T>
class JLocalRef {
  public:
    JLocalRef() : jobj(NULL) { }
    JLocalRef(const T& obj) : jobj(obj) { }
    ~JLocalRef() { if (jobj) { GetEnv()->DeleteLocalRef(jobj); } }
    JLocalRef& operator=(T obj)
    {
        if (jobj) {
            GetEnv()->DeleteLocalRef(jobj);
        }
        jobj = obj;
        return *this;
    }
    operator T() { return jobj; }
    T move()
    {
        T ret = jobj;
        jobj = NULL;
        return ret;
    }
  private:
    T jobj;
};

/**
 * A scoped JNIEnv pointer to ensure proper release.
 */
class JScopedEnv {
  public:
    JScopedEnv();
    ~JScopedEnv();
    JNIEnv* operator->() { return env; }
    operator JNIEnv*() { return env; }
  private:
    JScopedEnv(const JScopedEnv& other);
    JScopedEnv& operator =(const JScopedEnv& other);

    JNIEnv* env;
    jint detached;
};

/**
 * Construct a scoped JNIEnv pointer.
 */
JScopedEnv::JScopedEnv()
    : env(GetEnv(&detached))
{
}

/**
 * Destroy a scoped JNIEnv pointer.
 */
JScopedEnv::~JScopedEnv()
{
    /* Clear any pending exceptions before detaching. */
    {
        JLocalRef<jthrowable> ex = env->ExceptionOccurred();
        if (ex) {
            env->ExceptionClear();
            env->CallStaticVoidMethod(CLS_BusException, MID_BusException_log, (jthrowable)ex);
        }
    }
    DeleteEnv(detached);
}

/**
 * Helper function to wrap StringUTFChars to ensure proper release of resource.
 *
 * @warning NULL is a valid value, so exceptions must be checked for explicitly
 * by the caller after constructing the JString.
 */
class JString {
  public:
    JString(jstring s);
    ~JString();
    const char* c_str() { return str; }
  private:
    JString(const JString& other);
    JString& operator =(const JString& other);

    jstring jstr;
    const char* str;
};

/**
 * Construct a representation of a string with wraped StringUTFChars.
 *
 * @param s the string to wrap.
 */
JString::JString(jstring s)
    : jstr(s), str(jstr ? GetEnv()->GetStringUTFChars(jstr, NULL) : NULL)
{
}

/**
 * Destroy a string with wraped StringUTFChars.
 */
JString::~JString()
{
    if (str) {
        GetEnv()->ReleaseStringUTFChars(jstr, str);
    }
}

/**
 * Helper function to throw an exception
 */
static void Throw(const char* name, const char* msg)
{
    JNIEnv* env = GetEnv();
    JLocalRef<jclass> clazz = env->FindClass(name);
    if (clazz) {
        env->ThrowNew(clazz, msg);
    }
}

/**
 * Helper function to throw a bus exception
 */
static void ThrowErrorReplyBusException(const char* name, const char* message)
{
    JNIEnv* env = GetEnv();
    JLocalRef<jstring> jname = env->NewStringUTF(name);
    if (!jname) {
        return;
    }
    JLocalRef<jstring> jmessage = env->NewStringUTF(message);
    if (!jmessage) {
        return;
    }
    jmethodID mid = env->GetMethodID(CLS_ErrorReplyBusException, "<init>",
                                     "(Ljava/lang/String;Ljava/lang/String;)V");
    JLocalRef<jthrowable> jexc = (jthrowable)env->NewObject(CLS_ErrorReplyBusException, mid,
                                                            (jstring)jname, (jstring)jmessage);
    if (jexc) {
        env->Throw(jexc);
    }
}

/**
 * Get the native C++ handle of a given Java object.
 *
 * If we have an object that has a native counterpart, we need a way to get at
 * the native object from the Java object.  We do this by storing the native
 * pointer as an opaque handle in a Java field named "handle".  We use Java
 * reflection to pull the field out and return the handle value.
 *
 * Think of this handle as the counterpart to the object reference found in
 * the C++ objects that need to call into Java.  Java objects use the handle to
 * get at the C++ objects, and C++ objects use an object reference to get at
 * the Java objects.
 *
 * @return The handle value as a pointer.  NULL is a valid value.
 *
 * @warning This method makes native calls which may throw exceptions.  In the
 *          usual idiom, exceptions must be checked for explicitly by the caller
 *          after *every* call to GetHandle.  Since NULL is a valid value to
 *          return, validity of the returned pointer must be checked as well.
 */
template <typename T>
static T GetHandle(jobject jobj)
{
    JNIEnv* env = GetEnv();
    if (!jobj) {
        Throw("java/lang/NullPointerException", "failed to get native handle on null object");
        return NULL;
    }
    JLocalRef<jclass> clazz = env->GetObjectClass(jobj);
    jfieldID fid = env->GetFieldID(clazz, "handle", "J");
    void* handle = NULL;
    if (fid) {
        handle = (void*)env->GetLongField(jobj, fid);
    }

    return reinterpret_cast<T>(handle);
}

/**
 * Set the native C++ handle of a given Java object.
 *
 * If we have an object that has a native counterpart, we need a way to get at
 * the native object from the Java object.  We do this by storing the native
 * pointer as an opaque handle in a Java field named "handle".  We use Java
 * reflection to determine the field out and set the handle value.
 *
 * @param jobj The Java object which needs to have its handle set.
 * @param handle The pointer to the C++ object which is the handle value.
 *
 * @warning May throw an exception.
 */
static void SetHandle(jobject jobj, void* handle)
{
    JNIEnv* env = GetEnv();
    if (!jobj) {
        Throw("java/lang/NullPointerException", "failed to set native handle on null object");
        return;
    }
    JLocalRef<jclass> clazz = env->GetObjectClass(jobj);
    jfieldID fid = env->GetFieldID(clazz, "handle", "J");
    if (fid) {
        env->SetLongField(jobj, fid, (jlong)handle);
    }
}

/**
 * Given a Java listener object, return its corresponding C++ object.
 */
template <typename T>
T GetNativeListener(JNIEnv* env, jobject jlistener)
{
    return GetHandle<T>(jlistener);
}

/**
 * Translate a C++ return status code (QStatus) into a Java return status code
 * (JStatus).
 *
 * We have things called QStatus which are integers returned by the C++ side of
 * the bindings.  We need to translate those into a Java version (JStatus) that
 * serves the same purpose.
 *
 * @return A org.alljoyn.bus.Status enum value from the QStatus.
 */
static jobject JStatus(QStatus status)
{
    JNIEnv* env = GetEnv();
    jmethodID mid = env->GetStaticMethodID(CLS_Status, "create", "(I)Lorg/alljoyn/bus/Status;");
    if (!mid) {
        return NULL;
    }
    return CallStaticObjectMethod(env, CLS_Status, mid, status);
}

class JBusAttachment;

/**
 * This is classes primary responsibility is to convert the value returned from
 * the Java AboutDataListener to a C++ values expected for a C++ AboutDataListener
 *
 * This class also implements the C++ AboutObj so that for every Java AboutObj
 * an instance of this AboutDataListener also exists.
 */
class JAboutObject : public AboutObj, public AboutDataListener {
  public:
    JAboutObject(JBusAttachment* bus, AnnounceFlag isAboutIntfAnnounced) :
        AboutObj(*reinterpret_cast<BusAttachment*>(bus), isAboutIntfAnnounced), busPtr(bus) {
        QCC_DbgPrintf(("JAboutObject::JAboutObject"));
        MID_getAboutData = NULL;
        MID_getAnnouncedAboutData = NULL;
        jaboutDataListenerRef = NULL;
        jaboutObjGlobalRef = NULL;
    }

    QStatus announce(JNIEnv* env, jobject thiz, jshort sessionPort, jobject jaboutDataListener) {
        // Make sure the jaboutDataListener is the latest version of the Java AboutDataListener
        if (env->IsInstanceOf(jaboutDataListener, CLS_AboutDataListener)) {
            JLocalRef<jclass> clazz = env->GetObjectClass(jaboutDataListener);

            MID_getAboutData = env->GetMethodID(clazz, "getAboutData", "(Ljava/lang/String;)Ljava/util/Map;");
            if (!MID_getAboutData) {
                return ER_FAIL;
            }
            MID_getAnnouncedAboutData = env->GetMethodID(clazz, "getAnnouncedAboutData", "()Ljava/util/Map;");
            if (!MID_getAnnouncedAboutData) {
                return ER_FAIL;
            }
        } else {
            return ER_FAIL;
        }
        QCC_DbgPrintf(("AboutObj_announce jaboutDataListener is an instance of CLS_AboutDataListener"));


        /*
         * The weak global reference jaboutDataListener cannot be directly used.  We
         * have to get a "hard" reference to it and then use that.  If you try to
         * use a weak reference directly you will crash and burn.
         */
        //The user can change the AboutDataListener between calls check to see
        // we already have a a jaboutDataListenerRef if we do delete that ref
        // and create a new one.
        if (jaboutDataListenerRef != NULL) {
            GetEnv()->DeleteGlobalRef(jaboutDataListenerRef);
            jaboutDataListenerRef = NULL;
        }
        jaboutDataListenerRef = env->NewGlobalRef(jaboutDataListener);
        if (!jaboutDataListenerRef) {
            QCC_LogError(ER_FAIL, ("Can't get new local reference to AboutDataListener"));
            return ER_FAIL;
        }

        return Announce(static_cast<SessionPort>(sessionPort), *this);
    }

    ~JAboutObject() {
        QCC_DbgPrintf(("JAboutObject::~JAboutObject"));
        if (jaboutDataListenerRef != NULL) {
            GetEnv()->DeleteGlobalRef(jaboutDataListenerRef);
            jaboutDataListenerRef = NULL;
        }
    }

    QStatus GetAboutData(MsgArg* msgArg, const char* language)
    {
        QCC_DbgPrintf(("JAboutObject::GetMsgArg"));

        /*
         * JScopedEnv will automagically attach the JVM to the current native
         * thread.
         */
        JScopedEnv env;

        // Note we don't check that if the jlanguage is null because null is an
        // acceptable value for the getAboutData Method call.
        JLocalRef<jstring> jlanguage = env->NewStringUTF(language);

        QStatus status = ER_FAIL;
        if (jaboutDataListenerRef != NULL && MID_getAboutData != NULL) {
            QCC_DbgPrintf(("Calling getAboutData for %s language.", language));
            JLocalRef<jobject> jannounceArg = CallObjectMethod(env, jaboutDataListenerRef, MID_getAboutData, (jstring)jlanguage);
            QCC_DbgPrintf(("JAboutObj::GetMsgArg Made Java Method call getAboutData"));
            // check for ErrorReplyBusException exception
            status = CheckForThrownException(env);
            if (ER_OK == status) {
                // Marshal the returned value
                if (!Marshal("a{sv}", jannounceArg, msgArg)) {
                    QCC_LogError(ER_FAIL, ("JAboutData(): GetMsgArgAnnounce() marshaling error"));
                    return ER_FAIL;
                }
            } else {
                QCC_DbgPrintf(("JAboutObj::GetMsgArg exception with status %s", QCC_StatusText(status)));
                return status;
            }
        }
        return ER_OK;
    }

    QStatus GetAnnouncedAboutData(MsgArg* msgArg)
    {
        QCC_DbgPrintf(("JAboutObject::~GetMsgArgAnnounce"));
        QStatus status = ER_FAIL;
        if (jaboutDataListenerRef != NULL && MID_getAnnouncedAboutData != NULL) {
            QCC_DbgPrintf(("AboutObj_announce obtained jo local ref of jaboutDataListener"));
            /*
             * JScopedEnv will automagically attach the JVM to the current native
             * thread.
             */
            JScopedEnv env;

            JLocalRef<jobject> jannounceArg = CallObjectMethod(env, jaboutDataListenerRef, MID_getAnnouncedAboutData);
            QCC_DbgPrintf(("AboutObj_announce Made Java Method call getAnnouncedAboutData"));
            // check for ErrorReplyBusException exception
            status = CheckForThrownException(env);
            if (ER_OK == status) {
                if (!Marshal("a{sv}", jannounceArg, msgArg)) {
                    QCC_LogError(ER_FAIL, ("JAboutData(): GetMsgArgAnnounce() marshaling error"));
                    return ER_FAIL;
                }
            } else {
                QCC_DbgPrintf(("JAboutObj::GetAnnouncedAboutData exception with status %s", QCC_StatusText(status)));
                return status;
            }
        }
        return status;
    }

    /**
     * This will check if the last method call threw an exception Since we are
     * looking for ErrorReplyBusExceptions we know that the exception thrown
     * correlates to a QStatus that we are trying to get.  If ER_FAIL is returned
     * then we had an issue resolving the java method calls.
     *
     * @return QStatus indicating the status that was thrown from the ErrReplyBusException
     */
    QStatus CheckForThrownException(JScopedEnv& env) {
        JLocalRef<jthrowable> ex = env->ExceptionOccurred();
        if (ex) {
            env->ExceptionClear();
            JLocalRef<jclass> clazz = env->GetObjectClass(ex);
            if (env->IsInstanceOf(ex, CLS_ErrorReplyBusException) && clazz != NULL) {
                jmethodID mid = env->GetMethodID(clazz, "getErrorStatus", "()Lorg/alljoyn/bus/Status;");
                if (!mid) {
                    return ER_FAIL;
                }
                JLocalRef<jobject> jstatus = CallObjectMethod(env, ex, mid);
                if (env->ExceptionCheck()) {
                    return ER_FAIL;
                }
                JLocalRef<jclass> statusClazz = env->GetObjectClass(jstatus);
                mid = env->GetMethodID(statusClazz, "getErrorCode", "()I");
                if (!mid) {
                    return ER_FAIL;
                }
                QStatus errorCode = (QStatus)env->CallIntMethod(jstatus, mid);
                if (env->ExceptionCheck()) {
                    return ER_FAIL;
                }
                return errorCode;
            }
            return ER_FAIL;
        }
        return ER_OK;
    }
    JBusAttachment* busPtr;
    jmethodID MID_getAboutData;
    jmethodID MID_getAnnouncedAboutData;
    jobject jaboutDataListenerRef;
    Mutex jaboutObjGlobalRefLock;
    jobject jaboutObjGlobalRef;
};

class JBusObject;
class JSignalHandler;
class JKeyStoreListener;
class JAuthListener;
class PendingAsyncJoin;
class PendingAsyncPing;


/**
 * The C++ class that backs the Java BusAttachment class and provides the
 * plumbing connection from AllJoyn out to Java-land.
 */
class JBusAttachment : public BusAttachment {
  public:
    JBusAttachment(const char* applicationName, bool allowRemoteMessages, int concurrency);
    QStatus Connect(const char* connectArgs, jobject jkeyStoreListener, const char* authMechanisms,
                    jobject jauthListener, const char* keyStoreFileName, jboolean isShared);
    void Disconnect(const char* connectArgs);
    QStatus EnablePeerSecurity(const char* authMechanisms, jobject jauthListener, const char* keyStoreFileName, jboolean isShared);
    QStatus RegisterBusObject(const char* objPath, jobject jbusObject, jobjectArray jbusInterfaces,
                              jboolean jsecure, jstring jlangTag, jstring jdesc, jobject jtranslator);
    void UnregisterBusObject(jobject jbusObject);

    template <typename T>
    QStatus RegisterSignalHandler(const char* ifaceName, const char* signalName,
                                  jobject jsignalHandler, jobject jmethod, const char* ancillary);
    void UnregisterSignalHandler(jobject jsignalHandler, jobject jmethod);

    bool IsLocalBusObject(jobject jbusObject);
    void ForgetLocalBusObject(jobject jbusObject);

    /**
     * A mutex to serialize access to bus attachment critical sections.  It
     * doesn't seem worthwhile to have any finer granularity than this.  Note
     * that this member is public since we trust that the native binding we
     * wrote will use it correctly.
     */
    Mutex baCommonLock;

    /**
     * A mutex to serialize method call, property, etc., access in any attached
     * ProxyBusObject.  This is a blunt instrument, but support for
     * multi-threading on client and service sides has not been completely
     * implemented, so we simply disallow it for now.
     */
    Mutex baProxyLock;

    /**
     * A vector of all of the C++ "halves" of the signal handler objects
     * associated with this bus attachment.  Note that this member is public
     * since we trust that the native binding we wrote will use it correctly.
     */
    vector<pair<jobject, JSignalHandler*> > signalHandlers;

    /*
     * The single (optionsl) KeyStoreListener associated with this bus
     * attachment.  The KeyStoreListener and AuthListener work together to deal
     * with security exchanges over secure interfaces.  Note that this member is
     * public since we trust that the native binding we wrote will usse it
     * correctly.  When keyStoreListener is set, there must be a corresponding
     * strong reference to the associated Java Object set in
     * jkeyStoreListenerRef.
     */
    JKeyStoreListener* keyStoreListener;

    /**
     * A JNI strong global reference to The single (optional) Java KeyStoreListener
     * that has been associated with this bus attachment.  When jkeystoreListenerRef is
     * set, there must be a corresponding object pointer to an associated
     * C++ backing object set in keyStoreListener.
     */
    jobject jkeyStoreListenerRef;

    /**
     * The single (optional) C++ backing class for a provided AuthListener that
     * has been associated with this bus attachment.  The KeyStoreListener and
     * AuthListener work together to deal with security exchanges over secure
     * interfaces.  Note that this member is public since we trust that the
     * native binding we wrote will use it correctly.  When authListener is
     * set, there must be a corresponding strong reference to the associated
     * Java Object set in jauthListenerRef.
     */
    JAuthListener* authListener;

    /**
     * The single (optional) C++ backing class for JAboutObject. The aboutObj
     * contain a global ref jaboutObjGlobalRef that must be cleared when the
     * BusAttachment is disconnected.
     */
    JAboutObject* aboutObj;

    /**
     * A JNI strong global reference to The single (optional) Java AuthListener
     * that has been associated with this bus attachment.  When jauthListenerRef is
     * set, there must be a corresponding object pointer to an associated
     * C++ backing object set in authListener.
     */
    jobject jauthListenerRef;

    /**
     * A dedicated mutex to serialize access to the authListener,
     * authListenerRef, keyStoreListener and keyStoreListenerRef.  This is
     * required since we can't hold the common lock during callouts to Alljoyn
     * that may result in callins.  This describes the authentication process.
     * In order to prevent users from calling in right in the middle of an
     * authentication session and changing the authentication listeners out
     * from under us, we dedicate a lock that must be taken in order to make
     * a change.  This lock is held during the authentication process and during
     * the change process.
     */
    Mutex baAuthenticationChangeLock;

    /**
     * A list of strong references to Java bus listener objects.
     *
     * If clients use the unnamed parameter / unnamed class idiom to provide bus
     * listeners to registerBusListener, they can forget that the listeners
     * exist after the register call and never explicitly call unregister.
     *
     * Since we need these Java objects around, we need to hold a strong
     * reference to them to keep them from being garbage collected.
     *
     * Note that this member is public since we trust that the native binding we
     * wrote will use it correctly.
     */
    list<jobject> busListeners;

    /**
     * A list of strong references to Java translator objects.
     *
     * If clients use the unnamed parameter / unnamed class idiom to provide bus
     * listeners to setDescriptionTranslator, they can forget that the listeners
     * exist after the register call and never explicitly call unregister.
     *
     * Since we need these Java objects around, we need to hold a strong
     * reference to them to keep them from being garbage collected.
     *
     * Note that this member is public since we trust that the native binding we
     * wrote will usse it correctly.
     */
    list<jobject> translators;

    /**
     * A list of strong references to Java Bus Objects we use to indicate that
     * we have a part ownership in a given object.  Used during destruction.
     *
     */
    list<jobject> busObjects;

    /**
     * A map from session ports to their associated Java session port listeners.
     *
     * This mapping must be on a per-bus attachment basis since the scope of the
     * uniqueness of a session port is per-bus attachment
     *
     * Note that this member is public since we trust that the native binding we
     * wrote will usse it correctly.
     */
    map<SessionPort, jobject> sessionPortListenerMap;

    typedef struct {
        jobject jhostedListener;
        jobject jjoinedListener;
        jobject jListener;
    }BusAttachmentSessionListeners;

    /**
     * A map from sessions to their associated Java session listeners.
     *
     * This mapping must be on a per-bus attachment basis since the uniqueness of a
     * session is per-bus attachment.
     *
     * Note that this member is public since we trust that the native binding we
     * wrote will usse it correctly.
     */

    map<SessionId, BusAttachmentSessionListeners> sessionListenerMap;

    /**
     * A List of pending asynchronous join operation informations.  We store
     * Java object references here while AllJoyn mulls over what it can do about
     * the operation. Note that this member is public since we trust that the
     * native binding we wrote will use it correctly.
     */
    list<PendingAsyncJoin*> pendingAsyncJoins;

    /**
     * A List of pending asynchronous ping operation informations.  We store
     * Java object references here while AllJoyn mulls over what it can do about
     * the operation. Note that this member is public since we trust that the
     * native binding we wrote will use it correctly.
     */
    list<PendingAsyncPing*> pendingAsyncPings;

    int32_t IncRef(void)
    {
        return IncrementAndFetch(&refCount);
    }

    int32_t DecRef(void)
    {
        uint32_t refs = DecrementAndFetch(&refCount);
        if (refs == 0) {
            delete this;
        }
        return refs;
    }

    int32_t GetRef(void)
    {
        return refCount;
    }

  private:
    JBusAttachment(const JBusAttachment& other);
    JBusAttachment& operator =(const JBusAttachment& other);

    /*
     * An intrusive reference count
     */
    int32_t refCount;

    /*
     * Destructor is marked private since it should only be called from DecRef.
     */
    virtual ~JBusAttachment();

};

/**
 * The C++ class that implements the BusListener functionality.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't create a reference cycle -- we must allow the listener to be garbage
 * collected if the client and binding both drop refrences.  See the member
 * variable jbusListener for this reference.
 *
 * Think of the object reference here as the counterpart to the handle pointer
 * found in the Java objects that need to call into C++.  Java objects use the
 * handle to get at the C++ objects, and C++ objects use an object reference to
 * get at the Java objects.
 *
 * This object translates C++ callbacks from the BusListener to its Java
 * counterpart.  Because of this, the constructor performs reflection on the
 * provided Java object to determine the methods that need to be called.  When
 * The callback from C++ is executed, we make corresponding Java calls using the
 * reference to the java object and the reflection information we discovered in
 * the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 */
class JBusListener : public BusListener {
  public:
    JBusListener(jobject jlistener);
    ~JBusListener();

    void Setup(jobject jbusAttachment);

    void ListenerRegistered(BusAttachment* bus);
    void ListenerUnregistered();
    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);
    void LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix);
    void NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner);
    void BusStopping();
    void BusDisconnected();

  private:
    JBusListener(const JBusListener& other);
    JBusListener& operator =(const JBusListener& other);

    jweak jbusListener;
    jmethodID MID_listenerRegistered;
    jmethodID MID_listenerUnregistered;
    jmethodID MID_foundAdvertisedName;
    jmethodID MID_lostAdvertisedName;
    jmethodID MID_nameOwnerChanged;
    jmethodID MID_busStopping;
    jmethodID MID_busDisconnected;
    jweak jbusAttachment;
};

/**
 * The C++ class that implements the SessionListener functionality.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.
 *
 * Think of the object reference here as the counterpart to the handle pointer
 * found in the Java objects that need to call into C++.  Java objects use the
 * handle to get at the C++ objects, and C++ objects use a weak reference to
 * get at the Java objects.
 *
 * This object translates C++ callbacks from the SessionListener to its Java
 * counterpart.  Because of this, the constructor performs reflection on the
 * provided Java object to determine the methods that need to be called.  When
 * The callback from C++ is executed, we make corresponding Java calls using the
 * reference to the java object and the reflection information we discovered in
 * the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 */
class JSessionListener : public SessionListener {
  public:
    JSessionListener(jobject jsessionListener);
    ~JSessionListener();

    void SessionLost(SessionId sessionId);

    void SessionLost(SessionId sessionId, SessionListener::SessionLostReason reason);

    void SessionMemberAdded(SessionId sessionId, const char* uniqueName);

    void SessionMemberRemoved(SessionId sessionId, const char* uniqueName);

  private:
    JSessionListener(const JSessionListener& other);
    JSessionListener& operator =(const JSessionListener& other);

    jweak jsessionListener;
    jmethodID MID_sessionLost;
    jmethodID MID_sessionLostWithReason;
    jmethodID MID_sessionMemberAdded;
    jmethodID MID_sessionMemberRemoved;
};

/**
 * The C++ class that imlements the SessionPortListener functionality.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.
 *
 * Think of the object reference here as the counterpart to the handle pointer
 * found in the Java objects that need to call into C++.  Java objects use the
 * handle to get at the C++ objects, and C++ objects use a weak reference to
 * get at the Java objects.
 *
 * This object translates C++ callbacks from the SessionPortListener to its Java
 * counterpart.  Because of this, the constructor performs reflection on the
 * provided Java object to determine the methods that need to be called.  When
 * The callback from C++ is executed, we make corresponding Java calls using the
 * reference to the java object and the reflection information we discovered in
 * the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 */
class JSessionPortListener : public SessionPortListener {
  public:
    JSessionPortListener(jobject jsessionPortListener);
    ~JSessionPortListener();

    bool AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts);
    void SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner);

  private:
    JSessionPortListener(const JSessionPortListener& other);
    JSessionPortListener& operator =(const JSessionPortListener& other);

    jweak jsessionPortListener;
    jmethodID MID_acceptSessionJoiner;
    jmethodID MID_sessionJoined;
};

/**
 * The C++ class that imlements the KeyStoreListener functionality.
 *
 * For historical reasons, the KeyStoreListener follows a different pattern than
 * most of the listeners found in the bindings. Typically there is a one-to-one
 * correspondence between the methods of the C++ listener objects and the Java
 * listener objects.  That is not the case here.
 *
 * The C++ object has two methods, LoadRequest and StoreRequest, which take a
 * reference to a C++ KeyStore object.  The Java bindings break these requests
 * out into more primitive operations.  The upside is that this approach is
 * thought to correspond more closely to the "Java Way."  The downsides are that
 * Java clients work differently than other clients, and by breaking the operations
 * up into more primitive calls, we have created possible consistency problems.
 *
 * A LoadRequest callback to the C++ object is implemented as the following call
 * sequence:
 *
 * - Call into the Java client KeyStoreListener.getKeys() to get the keys from
 *   a local KeyStore, typically a filesystem operation.
 * - Call into the Java client KeyStoreListener.getPassword() to get the
 *   password used to encrypt the keys.  This is remembered somehow, probably
 *   needing a filesystem operation to recall.
 * - Call into the Bindings' BusAttachment.encode() to encode the keys byte
 *   array as UTF-8 characters.  This is a quick local operation.
 * - Call into the C++ KeyStoreListener::PutKeys() to give the encoded keys
 *   and password back to AllJoyn which passes them on to the authentication
 *   engine.
 *
 * The KeyStore and KeyStoreListener are responsible for ensuring the
 * consistency of the information, in what might be a farily complicated
 * way.  Here in the bindings we don't attempt this, but trust that what we
 * get will make sense.
 *
 * A StoreRequest callback to the C++ object is implemented as one call into
 * the client Java object, but the keys are provided as a byte array instead
 * of as a reference to a key store object, and the method name called is
 * changed from the C++ version.
 *
 * - Call into C++ KeyStoreListener::GetKeys to get the newly updated keys
 *   from AllJoyn.
 * - Call into the Java client KeyStoreListener.putKeys() to save the keys
 *   into the local KeyStore, probably using a filesystem operation.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.
 *
 * Think of the weak reference as the counterpart to the handle pointer found in
 * the Java objects that need to call into C++.  Java objects use the handle to
 * get at the C++ objects, and C++ objects use a weak object reference to get at
 * the Java objects.
 *
 * This object translates C++ callbacks from the KeyStoreListener to its Java
 * counterpart.  Because of this, the constructor performs reflection on the
 * provided Java object to determine the methods that need to be called.  When
 * The callback from C++ is executed, we make corresponding Java calls using
 * the weak reference to the java object and the reflection information we
 * discovered in the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 */
class JKeyStoreListener : public KeyStoreListener {
  public:
    JKeyStoreListener(jobject jlistener);
    ~JKeyStoreListener();
    QStatus LoadRequest(KeyStore& keyStore);
    QStatus StoreRequest(KeyStore& keyStore);
  private:
    JKeyStoreListener(const JKeyStoreListener& other);
    JKeyStoreListener& operator =(const JKeyStoreListener& other);
    jweak jkeyStoreListener;
    jmethodID MID_getKeys;
    jmethodID MID_getPassword;
    jmethodID MID_putKeys;
    jmethodID MID_encode;
};

/**
 * The C++ class that imlements the AuthListener functionality.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.  See the member variable
 * jbusListener for this reference.  The bindings hold separate strong references
 * to prevent the listener from being garbage collected in the presence of the
 * anonymous class idiom.
 *
 * Think of the weak reference as the counterpart to the handle pointer found in
 * the Java objects that need to call into C++.  Java objects use the handle to
 * get at the C++ objects, and C++ objects use a weak object reference to get at
 * the Java objects.
 *
 * This object translates C++ callbacks from the AuthListener to its Java
 * counterpart.  Because of this, the constructor performs reflection on the
 * provided Java object to determine the methods that need to be called.  When
 * The callback from C++ is executed, we make corresponding Java calls using the
 * weak reference to the java object and the reflection information we
 * discovered in the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 */
class JAuthListener : public AuthListener {
  public:
    JAuthListener(JBusAttachment* ba, jobject jlistener);
    ~JAuthListener();
    bool RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount,
                            const char* userName, uint16_t credMask, Credentials& credentials);
    bool VerifyCredentials(const char* authMechanism, const char* peerName, const Credentials& credentials);
    void SecurityViolation(QStatus status, const Message& msg);
    void AuthenticationComplete(const char* authMechanism, const char* peerName, bool success);
  private:
    JAuthListener(const JAuthListener& other);
    JAuthListener& operator =(const JAuthListener& other);

    JBusAttachment* busPtr;
    jweak jauthListener;
    jmethodID MID_requestCredentials;
    jmethodID MID_verifyCredentials;
    jmethodID MID_securityViolation;
    jmethodID MID_authenticationComplete;
};

/**
 * A C++ class to hold the Java object references required for an asynchronous
 * join operation while AllJoyn mulls over what it can do about the operation.
 *
 * An instance of this class is given to the C++ JoinSessionAsync method as the
 * context object.  Note well that the context object passed around in the C++
 * side of things is *not* the same as the Java context object passed into
 * joinSessionAsync.
 *
 * Another thing to keep in mind is that since the Java objects have been taken
 * into the JNI fold, they are referenced by JNI global references to the
 * objects provided by Java which may be different than the references seen by
 * the Java code.  Compare using JNI IsSameObject() to see if they are really
 * referencing the same object..
 */
class PendingAsyncJoin {
  public:
    PendingAsyncJoin(jobject jsessionListener, jobject jonJoinSessionListener, jobject jcontext) {
        this->jsessionListener = jsessionListener;
        this->jonJoinSessionListener = jonJoinSessionListener;
        this->jcontext = jcontext;
    }
    jobject jsessionListener;
    jobject jonJoinSessionListener;
    jobject jcontext;
  private:
    PendingAsyncJoin(const PendingAsyncJoin& other);
    PendingAsyncJoin& operator =(const PendingAsyncJoin& other);
};

/**
 * The C++ class that imlements the OnJoinSessionListener functionality.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.  See the member variable
 * jbusListener for this reference.  The bindings hold separate strong references
 * to prevent the listener from being garbage collected in the presence of the
 * anonymous class idiom.
 *
 * Think of the weak reference as the counterpart to the handle pointer found in
 * the Java objects that need to call into C++.  Java objects use the handle to
 * get at the C++ objects, and C++ objects use a weak object reference to get at
 * the Java objects.
 *
 * This object translates C++ callbacks from the OnJoinSessionListener to its
 * Java counterpart.  Because of this, the constructor performs reflection on
 * the provided Java object to determine the methods that need to be called.
 * When The callback from C++ is executed, we make corresponding Java calls
 * using the weak reference to the java object and the reflection information we
 * discovered in the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 *
 * One minor abberation here is that the bus attachment pointer can't be a
 * managed object since we don't have it when the listener is created, it is
 * passed in later.
 */
class JOnJoinSessionListener : public BusAttachment::JoinSessionAsyncCB {
  public:
    JOnJoinSessionListener(jobject jonJoinSessionListener);
    ~JOnJoinSessionListener();

    void Setup(JBusAttachment* jbap);
    void JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& sessionOpts, void* context);

  private:
    JOnJoinSessionListener(const JOnJoinSessionListener& other);
    JOnJoinSessionListener& operator =(const JOnJoinSessionListener& other);

    jmethodID MID_onJoinSession;
    JBusAttachment* busPtr;
};

/**
 * A C++ class to hold the Java object references required for an asynchronous
 * ping operation while AllJoyn mulls over what it can do about the operation.
 *
 * An instance of this class is given to the C++ PingAsync method as the context
 * object.  Note well that the context object passed around in the C++ side of
 * things is **not** the same as the Java context object passed into pingAsync.
 *
 * Another thing to keep in mind is that since the Java objects have been taken
 * into the JNI fold, they are referenced by JNI global references to the
 * objects provided by Java which may be different than the references seen by
 * the Java code.  Compare using JNI IsSameObject() to see if they are really
 * referencing the same object..
 */
class PendingAsyncPing {
  public:
    PendingAsyncPing(jobject jonPingListener, jobject jcontext) {
        this->jonPingListener = jonPingListener;
        this->jcontext = jcontext;
    }
    jobject jonPingListener;
    jobject jcontext;
  private:
    /**
     * private copy constructor this object can not be copied or assigned
     */
    PendingAsyncPing(const PendingAsyncPing& other);
    /**
     * private assignment operator this object can not be copied or assigned
     */
    PendingAsyncPing& operator =(const PendingAsyncPing& other);
};

/**
 * The C++ class that imlements the OnPingListener functionality.
 *
 * The standard idiom here is that whenever we have a C++ object in the AllJoyn
 * API, it has a corresponding Java object.  If the objects serve as callback
 * handlers, the C++ object needs to call into the Java object as a result of
 * an invocation by the AllJoyn code.
 *
 * As mentioned in the memory management sidebar (at the start of this file) we
 * have an idiom in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.  See the member variable
 * jbusListener for this reference.  The bindings hold separate strong references
 * to prevent the listener from being garbage collected in the presence of the
 * anonymous class idiom.
 *
 * Think of the weak reference as the counterpart to the handle pointer found in
 * the Java objects that need to call into C++.  Java objects use the handle to
 * get at the C++ objects, and C++ objects use a weak object reference to get at
 * the Java objects.
 *
 * This object translates C++ callbacks from the OnPingListener to its Java
 * counterpart.  Because of this, the constructor performs reflection on the
 * provided Java object to determine the methods that need to be called.  When
 * The callback from C++ is executed, we make corresponding Java calls using the
 * weak reference to the java object and the reflection information we
 * discovered in the constructor.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 *
 * One minor abberation here is that the bus attachment pointer can't be a
 * managed object since we don't have it when the listener is created, it is
 * passed in later.
 */
class JOnPingListener : public BusAttachment::PingAsyncCB {
  public:
    JOnPingListener(jobject jonPingListener);
    ~JOnPingListener();

    void Setup(JBusAttachment* jbap);
    void PingCB(QStatus status, void* context);

  private:
    /**
     * private copy constructor this object can not be copied or assigned
     */
    JOnPingListener(const JOnPingListener& other);
    /**
     * private assignment operator this object can not be copied or assigned
     */
    JOnPingListener& operator =(const JOnPingListener& other);

    jmethodID MID_onPing;
    JBusAttachment* busPtr;
};

/**
 * The C++ class that imlements the BusObject functionality.
 *
 * As mentioned in the Bus Object sidebar (at the start of this file) we
 * have a situation in which the C++ object is allocated and holds a reference to
 * the corresponding Java object.  This reference is a weak reference so we
 * don't interfere with Java garbage collection.  See the member variable
 * jbusObj for this reference.  The bindings hold separate strong references
 * to prevent the Java Bus Object from being garbage collected in case the client
 * forgets it (perhaps intentionally in the presence of the anonymous class idiom).
 *
 * This object translates C++ callbacks from the BusObject to its Java
 * counterpart.  This is a somewhat more dynamic situation than most of the
 * other C++ backing objects, so required reflection on the provided Java Object
 * is made in the callbacks themselves.  Once the required method has been
 * determined, we make corresponding Java calls using the weak reference into
 * the java object.
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 */
class JBusObject : public BusObject {
  public:
    JBusObject(JBusAttachment* jbap, const char* path, jobject jobj);
    ~JBusObject();
    QStatus AddInterfaces(jobjectArray jbusInterfaces);
    void MethodHandler(const InterfaceDescription::Member* member, Message& msg);
    QStatus MethodReply(const InterfaceDescription::Member* member, Message& msg, QStatus status);
    QStatus MethodReply(const InterfaceDescription::Member* member, const Message& msg, const char* error, const char* errorMessage = NULL);
    QStatus MethodReply(const InterfaceDescription::Member* member, Message& msg, jobject jreply);
    QStatus Signal(const char* destination, SessionId sessionId, const char* ifaceName, const char* signalName,
                   const MsgArg* args, size_t numArgs, uint32_t timeToLive, uint8_t flags, Message& msg);
    QStatus Get(const char* ifcName, const char* propName, MsgArg& val);
    QStatus Set(const char* ifcName, const char* propName, MsgArg& val);
    String GenerateIntrospection(const char* languageTag, bool deep = false, size_t indent = 0) const;
    String GenerateIntrospection(bool deep = false, size_t indent = 0) const;
    void ObjectRegistered();
    void ObjectUnregistered();
    void SetDescriptions(jstring jlangTag, jstring jdescription, jobject jtranslator);
  private:
    JBusObject(const JBusObject& other);
    JBusObject& operator =(const JBusObject& other);

    struct Property {
        String signature;
        jobject jget;
        jobject jset;
    };
    typedef map<String, jobject> JMethod;
    typedef map<String, Property> JProperty;
    jobject jbusObj;
    jmethodID MID_generateIntrospection;
    jmethodID MID_generateIntrospectionWithDesc;
    jmethodID MID_registered;
    jmethodID MID_unregistered;

    JMethod methods;
    JProperty properties;
    Mutex mapLock;

    JBusAttachment* busPtr;

    jobject jtranslatorRef;
};

/**
 * A map of Java Objects to JBusObjects.
 *
 * When we register a bus object, we are registering a plain old Java Object
 * that the client is claiming can act as a BusObject and has whatever it
 * takes to deal with the claimed interfaces.
 *
 * In order to make implementing bus objects easier (see the sidebar on Bus
 * Objects at the start of the file), what the client does is implement an
 * empty interface called BusObject (a Java marker interface).
 *
 * Since we have no super-powers to let us influence what goes into the
 * object that implements the empty interface, we have to provide some
 * scaffolding outside of the object to allow us to locate the C++ object
 * associated with the Java object and to reference count that C++ object.
 *
 * This is a different use case than a smart pointer, so once again, instead of
 * (mis) using the ManagedObj in another strange way, we just provide a
 * non-intrusive reference count here.
 */
map<jobject, pair<uint32_t, JBusObject*> > gBusObjectMap;
Mutex gBusObjectMapLock;

/**
 * This function takes a Java Bus Object and a newly created C++ backing
 * object and creates an entry in a global structure to establish the
 * relationship between the two.
 *
 * Whevever a Java Bus Listener is registered with a Bus Attachment, a
 * corresponding C++ object must be created to form the plumbing between
 * the AllJoyn system and the Java client.  Since a Java Bus Object may be
 * registered multiple times with multiple bus attachments, the C++ object
 * must be reference counted.  This function sets that reference count to
 * one, indicating a newly established relationship.
 *
 * This function transfers ownership of a JBusObject* from the caller to the
 * underlying map.  The caller must not free a provided JBusObject* unless
 * responsibility is explicitly transferred back by a non-zero return from
 * the function DecRefBackingObject.
 *
 * Since a reference to a Java Object is stored in the underlying map, we
 * insist that the caller must have taken a strong global reference to that
 * Java object prior to calling this function.
 *
 * Whenever a registerBusObject call is made, we expect the caller to check
 * to see if a relationship between the provided Java Bus Object and a C++
 * backing object exists, and if not create a backing object and call this
 * function to establish the relationship.
 *
 * Note that the lock on the underlying map is not taken in this and other
 * associated functions.  This is because, in most cases, atomicity must be
 * ensured across several calls that access the underlying map.  Therefore it is
 * the responsibility of the calling code to acquire the lock (i.e. call
 * gBusObjectMapLock.Lock) before calling any of the functions which access the
 * gBusObjectMap.
 */
void NewRefBackingObject(jobject javaObject, JBusObject* cppObject)
{
    QCC_DbgPrintf(("NewRefBackingObject(%p, %p)", javaObject, cppObject));

    map<jobject, pair<uint32_t, JBusObject*> >::iterator i = gBusObjectMap.find(javaObject);
    if (i != gBusObjectMap.end()) {
        QCC_LogError(ER_FAIL, ("NewRefBackingObject(): Mapping already established for Bus Object %p", javaObject));
        return;
    }

    gBusObjectMap[javaObject] = make_pair((uint32_t)1, (JBusObject*)cppObject);
}

/**
 * This function takes a Java Bus Object and increments the reference count to a
 * C++ backing object that must already exist.
 *
 * Whevever a Java Bus Listener is registered with a Bus Attachment, a
 * corresponding C++ object must be created to form the plumbing between the
 * AllJoyn system and the Java client.  Since a Java Bus Object may be
 * registered multiple times with multiple bus attachments, the C++ object must
 * be reference counted.  This function increments that reference count
 * indicating the given Java Object is referred to indirectly through an AllJoyn
 * Bus Attachment.
 *
 * Since a reference to a Java Object is stored in the underlying map, we insist
 * that the caller must have taken another strong global reference to the
 * provided Java object prior to calling this function.  That is, when
 * registering an Java Object with a Bus Attachment the caller is expected to
 * take a new reference to the Java object using a JNI call, and then take a new
 * reference to the C++ object by making this call.
 *
 * Whenever a registerBusObject call is made, we expect the caller to check
 * to see if a relationship between the provided Java Bus Object and a C++
 * backing object exists, and if so call this function to increment the reference
 * count on the C++ object.
 *
 * Note that the lock on the underlying map is not taken in this and other
 * associated functions.  This is because, in most cases, atomicity must be
 * ensured across several calls that access the underlying map.  Therefore it is
 * the responsibility of the calling code to acquire the lock (i.e. call
 * gBusObjectMapLock.Lock) before calling any of the functions which access the
 * gBusObjectMap.
 */
void IncRefBackingObject(jobject javaObject)
{
    QCC_DbgPrintf(("IncRefBackingObject()"));

    JNIEnv* env = GetEnv();

    for (map<jobject, pair<uint32_t, JBusObject*> >::iterator i = gBusObjectMap.begin(); i != gBusObjectMap.end(); ++i) {
        if (env->IsSameObject(javaObject, i->first)) {
            QCC_DbgPrintf(("IncRefBackingObject(): Found mapping for Java Bus Object %p.", javaObject));
            uint32_t refCount = i->second.first + 1;
            JBusObject* cppObject = i->second.second;
            gBusObjectMap[javaObject] = make_pair((uint32_t)refCount, (JBusObject*) cppObject);
            return;
        }
    }

    QCC_LogError(ER_FAIL, ("IncRefBackingObject(): No mapping exists for Java Bus Object %p", javaObject));
}

/**
 * This function takes a Java Bus Object and decrements the reference count to a
 * C++ backing object that must already exist.
 *
 * Whevever a Java Bus Listener is registered with a Bus Attachment, a
 * corresponding C++ object must be created to form the plumbing between the
 * AllJoyn system and the Java client.  Since a Java Bus Object may be
 * registered multiple times with multiple bus attachments, the C++ object must
 * be reference counted.  This function decrements that reference count
 * indicating the given Java Object is no longer referred to indirectly through
 * an AllJoyn Bus Attachment.
 *
 * This function transfers ownership of a JBusObject* to the caller if the
 * reference count is decremented to zero.  That is, if NULL is returned, there
 * is no change of responsibility, but if a non-zero pointer to a JBusObject*
 * is returned, the caller is expected to do whatever it takes to tear down the
 * object and free it.
 *
 * Since a reference to a Java Object is stored in the underlying map, we
 * previously insisted that the caller must have taken a strong global reference
 * to that Java object prior to calling this function.
 *
 * Whenever an unregisterBusObject call is made, we expect the caller to release
 * the Java global reference to the Java Bus Object and decrement the reference
 * count to the corresponding C++ object by calling this function.  If we return
 * a non-zero pointer, the caller must delete the JBusObject returned.
 *
 * Note that the lock on the underlying map is not taken in this and other
 * associated functions.  This is because, in most cases, atomicity must be
 * ensured across several calls that access the underlying map.  Therefore it is
 * the responsibility of the calling code to acquire the lock (i.e. call
 * gBusObjectMapLock.Lock) before calling any of the functions which access the
 * gBusObjectMap.
 */
JBusObject* DecRefBackingObject(jobject javaObject)
{
    QCC_DbgPrintf(("DecRefBackingObject(%p)", javaObject));

    JNIEnv* env = GetEnv();

    for (map<jobject, pair<uint32_t, JBusObject*> >::iterator i = gBusObjectMap.begin(); i != gBusObjectMap.end(); ++i) {
        QCC_DbgPrintf(("DecRefBackingObject(%p): trying %p", javaObject, i->first));
        if (env->IsSameObject(javaObject, i->first)) {
            QCC_DbgPrintf(("IncRefBackingObject(): Found mapping for Java Bus Object %p.", javaObject));
            JBusObject* cppObject = i->second.second;
            uint32_t refCount = i->second.first - 1;
            if (refCount) {
                QCC_DbgPrintf(("DecRefBackingObject(): More references to %p.", javaObject));
                gBusObjectMap[javaObject] = make_pair((uint32_t)refCount, (JBusObject*)cppObject);
                cppObject = NULL;
            } else {
                QCC_DbgPrintf(("DecRefBackingObject(): Last reference to %p.", javaObject));
                gBusObjectMap.erase(i);
            }
            return cppObject;
        }
    }

    QCC_LogError(ER_FAIL, ("DecRefBackingObject(): No mapping exists for Java Bus Object %p", javaObject));
    return NULL;
}

/**
 * Given a Java object that someone is claiming has been registered as a bus
 * object with a bus attachment; return the corresponding C++ object that
 * hooks it to the AllJoyn system.
 *
 * Note that the lock on the underlying map is not taken in this and other
 * associated functions.  This is because, in most cases, atomicity must be
 * ensured across several calls that access the underlying map.  Therefore it is
 * the responsibility of the calling code to acquire the lock (i.e. call
 * gBusObjectMapLock.Lock) before calling any of the functions which access the
 * gBusObjectMap.
 */
JBusObject* GetBackingObject(jobject jbusObject)
{
    QCC_DbgPrintf(("GetBackingObject(%p)", jbusObject));

    JNIEnv* env = GetEnv();

    for (map<jobject, pair<uint32_t, JBusObject*> >::iterator i = gBusObjectMap.begin(); i != gBusObjectMap.end(); ++i) {
        if (env->IsSameObject(jbusObject, i->first)) {
            QCC_DbgPrintf(("GetBackingObject(): Found mapping for Java Bus Object %p.", jbusObject));
            return i->second.second;
        }
    }

    QCC_DbgPrintf(("GetBackingObject(): No mapping exists for Java Bus Object %p.", jbusObject));
    return NULL;
}

/**
 * Given a Java object that someone is claiming has been registered as a bus
 * object with a bus attachment; return the corresponding strong reference to it
 * that we must have saved.
 */
jobject GetGlobalRefForObject(jobject jbusObject)
{
    QCC_DbgPrintf(("GetGlobalRefForObject(%p)", jbusObject));

    JNIEnv* env = GetEnv();

    for (map<jobject, pair<uint32_t, JBusObject*> >::iterator i = gBusObjectMap.begin(); i != gBusObjectMap.end(); ++i) {
        if (env->IsSameObject(jbusObject, i->first)) {
            QCC_DbgPrintf(("GetBackingObject(): Found global reference for Java Bus Object %p.", jbusObject));
            return i->first;
        }
    }

    QCC_DbgPrintf(("GetBackingObject(): No mapping exists for Java Bus Object %p.", jbusObject));
    return NULL;
}

/**
 * The C++ backing class corresponding to a Java ProxyBusObject.
 */
class JProxyBusObject : public ProxyBusObject {
  public:
    JProxyBusObject(jobject pbo, JBusAttachment* jbap, const char* endpoint, const char* path, SessionId sessionId, bool secure);
    ~JProxyBusObject();
    JBusAttachment* busPtr;
    jweak jpbo;
  private:
    JProxyBusObject(const JProxyBusObject& other);
    JProxyBusObject& operator =(const JProxyBusObject& other);
};

class JPropertiesChangedListener : public ProxyBusObject::PropertiesChangedListener {
  public:
    JPropertiesChangedListener(jobject jlistener, jobject changed, jobject invalidated);
    ~JPropertiesChangedListener();
    void PropertiesChanged(ProxyBusObject& obj, const char* ifaceName, const MsgArg& changed, const MsgArg& invalidated, void* context);

    jweak jlistener;

  private:
    JPropertiesChangedListener();
    JPropertiesChangedListener(const JPropertiesChangedListener& other);
    JPropertiesChangedListener& operator =(const JPropertiesChangedListener& other);

    jobject jchangedType;
    jobject jinvalidatedType;
};


class JSignalHandler : public MessageReceiver {
  public:
    JSignalHandler(jobject jobj, jobject jmethod);
    virtual ~JSignalHandler();
    bool IsSameObject(jobject jobj, jobject jmethod);
    virtual QStatus Register(BusAttachment& bus, const char* ifaceName, const char* signalName, const char* ancillary);
    virtual void Unregister(BusAttachment& bus) = 0;
    void SignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);
  protected:
    jweak jsignalHandler;
    jobject jmethod;
    const InterfaceDescription::Member* member;
    String ancillary_data; /* can be both source or matchRule; */

  private:
    JSignalHandler(const JSignalHandler& other);
    JSignalHandler& operator =(const JSignalHandler& other);

};

class JSignalHandlerWithSrc : public JSignalHandler {

  public:
    JSignalHandlerWithSrc(jobject jobj, jobject jmethod) : JSignalHandler(jobj, jmethod) { }
    QStatus Register(BusAttachment& bus, const char* ifaceName, const char* signalName, const char* ancillary);
  private:
    JSignalHandlerWithSrc(const JSignalHandlerWithSrc& other);
    JSignalHandlerWithSrc& operator =(const JSignalHandlerWithSrc& other);
    void Unregister(BusAttachment& bus);

};

class JSignalHandlerWithRule : public JSignalHandler {

  public:
    JSignalHandlerWithRule(jobject jobj, jobject jmethod) : JSignalHandler(jobj, jmethod) { }
    QStatus Register(BusAttachment& bus, const char* ifaceName, const char* signalName, const char* ancillary);
  private:
    JSignalHandlerWithRule(const JSignalHandlerWithRule& other);
    JSignalHandlerWithRule& operator =(const JSignalHandlerWithRule& other);
    void Unregister(BusAttachment& bus);

};

class JTranslator : public Translator {
  public:
    JTranslator(jobject jsessionListener);
    ~JTranslator();

    virtual size_t NumTargetLanguages();
    virtual void GetTargetLanguage(size_t index, qcc::String& ret);
    virtual const char* Translate(const char* sourceLanguage,
                                  const char* targetLanguage, const char* source, qcc::String& buffer);

  private:
    JTranslator(const JTranslator& other);
    JTranslator& operator =(const JTranslator& other);

    jweak jtranslator;
    jmethodID MID_numTargetLanguages;
    jmethodID MID_getTargetLanguage;
    jmethodID MID_translate;
};

/**
 * A MessageContext is an object that provides access to underlying AllJoyn
 * Message information without having to plumb the Message out into the Java
 * clients.  This results in cleaner client code since they only have to deal
 * with the signatures they expect in the 99% case.  It does mean we have to do
 * some gyrations here to keep the Message info straight, and we do have some
 * additional API with respect to the C++ version.
 *
 * TODO:
 * Message map is a global.  Why?
 */
class MessageContext {
  public:
    static Message GetMessage();
    MessageContext(const Message& msg);
    ~MessageContext();
  private:
    MessageContext(const MessageContext& other);
    MessageContext& operator =(const MessageContext& other);
};

map<Thread*, Message> gMessageMap;
Mutex gMessageMapLock;

Message MessageContext::GetMessage()
{
    QCC_DbgPrintf(("MessageContext::GetMessage()"));
    gMessageMapLock.Lock();
    map<Thread*, Message>::iterator it = gMessageMap.find(Thread::GetThread());
    assert(gMessageMap.end() != it);
    Message m = it->second;
    gMessageMapLock.Unlock();
    return m;
}

MessageContext::MessageContext(const Message& msg)
{
    QCC_DbgPrintf(("MessageContext::MessageContext()"));
    gMessageMapLock.Lock();
    gMessageMap.insert(pair<Thread*, Message>(Thread::GetThread(), msg));
    gMessageMapLock.Unlock();
}

MessageContext::~MessageContext()
{
    QCC_DbgPrintf(("MessageContext::~MessageContext()"));
    gMessageMapLock.Lock();
    map<Thread*, Message>::iterator it = gMessageMap.find(Thread::GetThread());
    gMessageMap.erase(it);
    gMessageMapLock.Unlock();
}

/**
 * Construct a JKeyStoreListener C++ object by arranging the correspondence
 * between the C++ object being constructed and the provided Java object.
 *
 * Since the purpose of the KeyStoreListener is to allow a client to recieve
 * callbacks from the AllJoyn system, we need to connect the C++ methods to
 * the java methods.  We do that using Java reflection.  In the constructor
 * we do the expensive work of finding the Java method IDs (MID_xxx below)
 * which will be invoked when the callbacks happen.
 *
 * We also save the required weak reference to the provided Java object (see
 * the sidebar on memory management at the start of this file).
 *
 * @param jlistener The corresponding java object.
 */
JKeyStoreListener::JKeyStoreListener(jobject jlistener)
    : jkeyStoreListener(NULL)
{
    QCC_DbgPrintf(("JKeyStoreListener::JKeyStoreListener()"));

    JNIEnv* env = GetEnv();
    jkeyStoreListener = env->NewWeakGlobalRef(jlistener);
    if (!jkeyStoreListener) {
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jlistener);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("JKeyStoreListener::JKeyStoreListener(): Can't GetObjectClass() for KeyStoreListener"));
        return;
    }

    MID_getKeys = env->GetMethodID(clazz, "getKeys", "()[B");
    if (!MID_getKeys) {
        QCC_DbgPrintf(("JKeyStoreListener::JKeystoreListener(): Can't find getKeys() in KeyStoreListener"));
        return;
    }

    MID_getPassword = env->GetMethodID(clazz, "getPassword", "()[C");
    if (!MID_getPassword) {
        QCC_DbgPrintf(("JKeyStoreListener::JKeystoreListener(): Can't find getPassword() in KeyStoreListener"));
        return;
    }

    MID_putKeys = env->GetMethodID(clazz, "putKeys", "([B)V");
    if (!MID_putKeys) {
        QCC_DbgPrintf(("JKeyStoreListener::JKeystoreListener(): Can't find putKeys() in KeyStoreListener"));
        return;
    }

    MID_encode = env->GetStaticMethodID(CLS_BusAttachment, "encode", "([C)[B");
    if (!MID_encode) {
        QCC_DbgPrintf(("JKeyStoreListener::JKeystoreListener(): Can't find endode() in KeyStoreListener"));
        return;
    }
}

/**
 * Destroy a JKeyStoreListener C++ object.
 *
 * We remove the weak reference to the associated Java object when the C++
 * object goes away.
 */
JKeyStoreListener::~JKeyStoreListener()
{
    QCC_DbgPrintf(("JKeyStoreListener::~JKeyStoreListener()"));
    if (jkeyStoreListener) {
        GetEnv()->DeleteWeakGlobalRef(jkeyStoreListener);
        jkeyStoreListener = NULL;
    }
}

/**
 * Handle the C++ LoadRequest callback from the AllJoyn system.
 */
QStatus JKeyStoreListener::LoadRequest(KeyStore& keyStore)
{
    QCC_DbgPrintf(("JKeyStoreListener::LoadRequest()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * The weak global reference jkeyStoreListener cannot be directly used.  We
     * have to get a "hard" reference to it and then use that.  If you try to
     * use a weak reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jkeyStoreListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JKeystoreListener::LoadRequest(): Can't get new local reference to SessionListener"));
        return ER_FAIL;
    }

    /*
     * Since the LoadRequest is broken up into three separate calls into two
     * fundamentally different Java objects, synchronization is hard.  For this
     * reason, we defer the multithread safety issue to the KeyStoreListener and
     * consistency to the combination of KeyStore and KeyStoreListner.
     *
     * We expect to get back references to arrays which are immutable and so we
     * can make these calls with the assurance that data will not change out
     * from under us.  It is not absolutely safe to make a copy of the returned
     * arrays, since the client can change the object "at the same time" as we
     * are copying it; so we rely on the client to return us a reference that it
     * promises not to change.  We assume that an answer from getKeys will be
     * consistent with a following answer from getPassword, and that interference
     * from anohter putKeys executing in another thread will not affect us.
     *
     * Of course, if the KeyStoreListener doesn't play by our ground rules, it
     * might result in a Bad Thing (TM) happening.  The choice of API makes this
     * very difficult for us to deal with.
     *
     * The result is that we may have a KeyStoreListener object which may be accessible
     * globally by any number of threads; and that listener is responsible for being
     * MT-Safe.
     */
    JLocalRef<jbyteArray> jarray = (jbyteArray)CallObjectMethod(env, jo, MID_getKeys);
    if (env->ExceptionCheck()) {
        return ER_FAIL;
    }

    /*
     * By contract with the KeyStoreListener, jarray will not be changed by the
     * client as long as we can possibly access it.  We can now do our several
     * operations on the array without (much) fear.
     */
    String source;
    if (jarray) {
        jsize len = env->GetArrayLength(jarray);
        jbyte* jelements = env->GetByteArrayElements(jarray, NULL);
        if (!jelements) {
            return ER_FAIL;
        }
        source = String((const char*)jelements, len);
        env->ReleaseByteArrayElements(jarray, jelements, JNI_ABORT);
    }

    /*
     * Get the password from the Java listener and load the keys.  The same
     * caveats apply to this char[] as do to the byte[] we got which contains
     * the keys.
     */
    JLocalRef<jcharArray> jpasswordChar = (jcharArray)CallObjectMethod(env, jo, MID_getPassword);
    if (env->ExceptionCheck() || !jpasswordChar) {
        return ER_FAIL;
    }

    /*
     * By contract with the KeyStoreListener, jpassword will not be changed by
     * the client as long as we can possibly access it.  We can now call out to
     * the bus attachment to encode the array without (much) fear of the client
     * interfering.  This call out to the bus attachment in a listener callback
     * implies that the encode method must be MT-Safe.
     */
    JLocalRef<jbyteArray> jpassword = (jbyteArray)CallStaticObjectMethod(env, CLS_BusAttachment, MID_encode, (jcharArray)jpasswordChar);
    if (env->ExceptionCheck()) {
        return ER_FAIL;
    }

    /*
     * Some care here is taken to ensure that we erase any in-memory copies of
     * the password as soon as possible after use to minimize attack exposure.
     * The password came in as the char[] jpasswordChar and was converted to
     * UTF-8 and stored in the byte[] jpassword.  We clear the bytes of the
     * password that we got from the user.
     */
    jchar* passwordChar = env->GetCharArrayElements(jpasswordChar, NULL);
    if (env->ExceptionCheck()) {
        return ER_FAIL;
    }
    memset(passwordChar, 0, env->GetArrayLength(jpasswordChar) * sizeof(jchar));
    env->ReleaseCharArrayElements(jpasswordChar, passwordChar, 0);
    if (!jpassword) {
        return ER_FAIL;
    }

    /*
     * Now, we get the bytes in the UTF-8 encoded password we made for ourselves
     * and call out AllJoyn, providing the keys and password bytes.  After we're
     * done with the UTF-8 encoded password, we zero that out to cover our
     * tracks.
     */
    jbyte* password = env->GetByteArrayElements(jpassword, NULL);
    if (env->ExceptionCheck()) {
        return ER_FAIL;
    }
    QStatus status = PutKeys(keyStore, source, String((const char*)password, env->GetArrayLength(jpassword)));
    memset(password, 0, env->GetArrayLength(jpassword) * sizeof(jbyte));
    env->ReleaseByteArrayElements(jpassword, password, 0);

    return status;
}

/**
 * Handle the C++ StoreRequest callback from the AllJoyn system.
 */
QStatus JKeyStoreListener::StoreRequest(KeyStore& keyStore)
{
    QCC_DbgPrintf(("JKeyStoreListener::StoreRequest()"));

    String sink;

    QStatus status = GetKeys(keyStore, sink);
    if (ER_OK != status) {
        return status;
    }

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    JLocalRef<jbyteArray> jarray = env->NewByteArray(sink.size());
    if (!jarray) {
        return ER_FAIL;
    }

    env->SetByteArrayRegion(jarray, 0, sink.size(), (jbyte*)sink.data());
    if (env->ExceptionCheck()) {
        return ER_FAIL;
    }

    /*
     * The weak global reference jkeyStoreListener cannot be directly used.  We
     * have to get a "hard" reference to it and then use that.  If you try to
     * use a weak reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jkeyStoreListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JKeystoreListener::StoreRequest(): Can't get new local reference to SessionListener"));
        return ER_FAIL;
    }

    /*
     * This call out to the listener means that the putKeys method must be
     * MT-Safe.  This is implied by the definition of the listener.  The
     * implementation of the KeyStoreListener is expected to ensure that
     * its results are consistent since getKeys, getPassword and putKeys
     * requests may come in from multiple threads.
     */
    env->CallVoidMethod(jo, MID_putKeys, (jbyteArray)jarray);
    if (env->ExceptionCheck()) {
        return ER_FAIL;
    }

    return ER_OK;
}

/**
 * Construct a JBusListener C++ object by arranging the correspondence
 * between the C++ object being constructed and the provided Java object.
 *
 * Since the purpose of the BusListener is to allow a client to recieve
 * callbacks from the AllJoyn system, we need to connect the C++ methods to
 * the java methods.  We do that using Java reflection.  In the constructor
 * we do the expensive work of finding the Java method IDs (MID_xxx below)
 * which will be invoked when the callbacks happen.
 *
 * We also save the required weak reference to the provided Java object (see
 * the sidebar on memory management at the start of this file).
 *
 * @param jlistener The corresponding java object.
 */
JBusListener::JBusListener(jobject jlistener)
    : jbusListener(NULL)
    , jbusAttachment(NULL)
{
    QCC_DbgPrintf(("JBusListener::JBusListener()"));

    JNIEnv* env = GetEnv();

    /*
     * Be careful when using a weak global reference.  They can only be
     * passed to NewLocalRef, NewGlobalRef and DeleteWeakGlobalRef.
     */
    QCC_DbgPrintf(("JBusListener::JBusListener(): Taking weak global reference to BusListener %p", jlistener));
    jbusListener = env->NewWeakGlobalRef(jlistener);
    if (!jbusListener) {
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jlistener);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("JBusListener::JBusListener(): Can't GetObjectClass() for KeyStoreListener"));
        return;
    }

    MID_listenerRegistered = env->GetMethodID(clazz, "listenerRegistered", "(Lorg/alljoyn/bus/BusAttachment;)V");
    if (!MID_listenerRegistered) {
        QCC_DbgPrintf(("JBusListener::JBusListener(): Can't find listenerRegistered() in jbusListener"));
    }

    MID_listenerUnregistered = env->GetMethodID(clazz, "listenerUnregistered", "()V");
    if (!MID_listenerUnregistered) {
        QCC_DbgPrintf(("JBusListener::JBusListener(): Can't find listenerUnregistered() in jbusListener"));
    }

    MID_foundAdvertisedName = env->GetMethodID(clazz, "foundAdvertisedName", "(Ljava/lang/String;SLjava/lang/String;)V");
    if (!MID_foundAdvertisedName) {
        QCC_DbgPrintf(("JBusListener::JBusListener(): Can't find foundAdvertisedName() in jbusListener"));
    }

    MID_lostAdvertisedName = env->GetMethodID(clazz, "lostAdvertisedName", "(Ljava/lang/String;SLjava/lang/String;)V");
    if (!MID_lostAdvertisedName) {
        QCC_DbgPrintf(("JBusListener::JBusListener(): Can't find lostAdvertisedName() in jbusListener"));
    }

    MID_nameOwnerChanged = env->GetMethodID(clazz, "nameOwnerChanged", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    if (!MID_nameOwnerChanged) {
        QCC_DbgPrintf(("JBusListener::JBusListener(): Can't find nameOwnerChanged() in jbusListener"));
    }

    MID_busStopping = env->GetMethodID(clazz, "busStopping", "()V");
    if (!MID_busStopping) {
        QCC_DbgPrintf(("JBusListener::JBusListener(): Can't find busStopping() in jbusListener"));
    }

    MID_busDisconnected = env->GetMethodID(clazz, "busDisconnected", "()V");
    if (!MID_busDisconnected) {
        QCC_DbgPrintf(("JBusListener::JBusListener(): Can't find busDisconnected() in jbusListener"));
    }

}

/**
 * Destroy a JBusListener C++ object.
 *
 * We remove the reference to the associated Java object when the C++
 * object goes away.  Since the C++ callback is gone, we can no longer call
 * the corresponding Java object, and it is garbage.
 *
 */
JBusListener::~JBusListener()
{
    QCC_DbgPrintf(("JBusListener::~JBusListener()"));

    JNIEnv* env = GetEnv();
    if (jbusAttachment) {
        QCC_DbgPrintf(("JBusListener::~JBusListener(): Releasing weak global reference to BusAttachment %p", jbusAttachment));
        env->DeleteWeakGlobalRef(jbusAttachment);
        jbusAttachment = NULL;
    }
    if (jbusListener) {
        QCC_DbgPrintf(("JBusListener::~JBusListener(): Releasing weak global reference to BusListener %p", jbusListener));
        env->DeleteWeakGlobalRef(jbusListener);
        jbusListener = NULL;
    }
}

void JBusListener::Setup(jobject jbusAttachment)
{
    QCC_DbgPrintf(("JBusListener::Setup()"));

    /*
     * We need to be able to get back at the bus attachment in the ListenerRegistered callback.
     */
    QCC_DbgPrintf(("JBusListener::Setup(): Taking weak global reference to BusAttachment %p", jbusAttachment));
    this->jbusAttachment = GetEnv()->NewWeakGlobalRef(jbusAttachment);
}

void JBusListener::ListenerRegistered(BusAttachment* bus)
{
    QCC_DbgPrintf(("JBusListener::ListenerRegistered()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    jobject jba = env->NewLocalRef(jbusAttachment);
    if (!jba) {
        QCC_LogError(ER_FAIL, ("JBusListener::ListenerRegistered(): Can't get new local reference to BusAttachment"));
        return;
    }
    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jba);
    if (env->ExceptionCheck() || busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("JBusListener::ListenerRegistered(): Exception or NULL bus pointer"));
        return;
    }
    assert(bus == busPtr);

    /*
     * The weak global reference jbusListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JBusListener::ListenerRegistered(): Can't get new local reference to BusListener"));
        return;
    }

    /*
     * This call out to listenerRegistered implies that the Java method must be
     * MT-safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JBusListener::ListenerRegistered(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_listenerRegistered, jba);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::ListenerRegistered(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JBusListener::ListenerRegistered(): Return"));
}

void JBusListener::ListenerUnregistered()
{
    QCC_DbgPrintf(("JBusListener::ListenerUnregistered()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * The weak global reference jbusListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JBusListener::ListenerUnregistered(): Can't get new local reference to BusListener"));
        return;
    }

    /*
     * This call out to listenerUnregistered implies that the Java method must be
     * MT-safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JBusListener::ListenerUnregistered(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_listenerUnregistered);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::ListenerUnregistered(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JBusListener::ListenerUnregistered(): Return"));
}

/**
 * Handle the C++ FoundAdvertisedName callback from the AllJoyn system.
 *
 * Called by the bus when an external bus is discovered that is advertising a
 * well-known name that this attachment has registered interest in via a DBus
 * call to org.alljoyn.Bus.FindAdvertisedName
 *
 * This is a callback returning void, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallVoidMethod().
 *
 * @param name         A well known name that the remote bus is advertising.
 * @param transport    Transport that received the advertisment.
 * @param namePrefix   The well-known name prefix used in call to
 *                     FindAdvertisedName that triggered this callback.
 */
void JBusListener::FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
{
    QCC_DbgPrintf(("JBusListener::FoundAdvertisedName()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    JLocalRef<jstring> jname = env->NewStringUTF(name);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::FoundAdvertisedName(): Exception"));
        return;
    }

    jshort jtransport = transport;

    JLocalRef<jstring> jnamePrefix = env->NewStringUTF(namePrefix);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::FoundAdvertisedName(): Exception"));
        return;
    }

    /*
     * The weak global reference jbusListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JBusListener::FoundAdvertisedName(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to foundAdvertisedName implies that the Java method must be
     * MT-safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JBusListener::FoundAdvertisedName(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_foundAdvertisedName, (jstring)jname, jtransport, (jstring)jnamePrefix);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::FoundAdvertisedName(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JBusListener::FoundAdvertisedName(): Return"));
}

/**
 * Handle the C++ LostAdvertisedName callback from the AllJoyn system.
 *
 * Called by the bus when an advertisement previously reported through FoundName
 * has become unavailable.
 *
 * This is a callback returning void, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallVoidMethod().
 *
 * @param name         A well known name that the remote bus is advertising.
 * @param transport    Transport that received the advertisment.
 * @param namePrefix   The well-known name prefix used in call to
 *                     FindAdvertisedName that triggered this callback.
 */
void JBusListener::LostAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
{
    QCC_DbgPrintf(("JBusListener::LostAdvertisedName()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    JLocalRef<jstring> jname = env->NewStringUTF(name);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::LostAdvertisedName(): Exception"));
        return;
    }

    jshort jtransport = transport;

    JLocalRef<jstring> jnamePrefix = env->NewStringUTF(namePrefix);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::LostAdvertisedName(): Exception"));
        return;
    }

    /*
     * The weak global reference jbusListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JBusListener::LostAdvertisedName(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to LostAdvertisedName implies that the Java method must be
     * MT-safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JBusListener::LostAdvertisedName(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_lostAdvertisedName, (jstring)jname, jtransport, (jstring)jnamePrefix);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::LostAdvertisedName(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JBusListener::LostAdvertisedName(): Return"));
}

/**
 * Handle the C++ NameOwnerChanged callback from the AllJoyn system.
 *
 * Called by the bus when the ownership of any well-known name changes.
 *
 * This is a callback returning void, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallVoidMethod().
 *
 * @param busName        The well-known name that has changed.
 * @param previousOwner  The unique name that previously owned the name or NULL if there was no previous owner.
 * @param newOwner       The unique name that now owns the name or NULL if the there is no new owner.
 */
void JBusListener::NameOwnerChanged(const char* busName, const char* previousOwner, const char* newOwner)
{
    QCC_DbgPrintf(("JBusListener::NameOwnerChanged()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    JLocalRef<jstring> jbusName = env->NewStringUTF(busName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::NameOwnerChanged(): Exception"));
        return;
    }

    JLocalRef<jstring> jpreviousOwner = env->NewStringUTF(previousOwner);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::NameOwnerChanged(): Exception"));
        return;
    }

    JLocalRef<jstring> jnewOwner = env->NewStringUTF(newOwner);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::NameOwnerChanged(): Exception"));
        return;
    }

    /*
     * The weak global reference jbusListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JBusListener::NameOwnerChanged(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to NameOwnerChanged implies that the Java method must be
     * MT-safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JBusListener::NameOwnerChanged(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_nameOwnerChanged, (jstring)jbusName, (jstring)jpreviousOwner, (jstring)jnewOwner);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::NameOwnerChanged(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JBusListener::NameOwnerChanged(): Return"));
}


/**
 * Handle the C++ BusStopping callback from the AllJoyn system.
 *
 * Called when a bus this listener is registered with is stopping.
 *
 * This is a callback returning void, with no formal parameters, so we just
 * call the corresponding Java method in the listener object.
 */
void JBusListener::BusStopping(void)
{
    QCC_DbgPrintf(("JBusListener::BusStopping()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * The weak global reference jbusListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JBusListener::BusStopping(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to BusStopping implies that the Java method must be
     * MT-safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JBusListener::BusStopping(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_busStopping);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::BusStopping(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JBusListener::BusStopping(): Return"));
}

/**
 * Handle the C++ BusDisconnected callback from the AllJoyn system.
 *
 * Called when a BusAttachment this listener is registered with is has become disconnected from
 * the bus
 *
 * This is a callback returning void, with no formal parameters, so we just
 * call the corresponding Java method in the listener object.
 */
void JBusListener::BusDisconnected(void)
{
    QCC_DbgPrintf(("JBusListener::BusDisconnected()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * The weak global reference jbusListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JBusListener::BusDisconnected(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to BusDisconnected implies that the Java method must be
     * MT-safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JBusListener::BusDisconnected(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_busDisconnected);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusListener::busDisconnected(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JBusListener::BusDisconnected(): Return"));
}

/**
 * Construct a JSessionListener C++ object by arranging the correspondence
 * between the C++ object being constructed and the provided Java object.
 *
 * Since the purpose of the SessionListener is to allow a client to recieve
 * callbacks from the AllJoyn system, we need to connect the C++ methods to
 * the java methods.  We do that using Java reflection.  In the constructor
 * we do the expensive work of finding the Java method IDs (MID_xxx below)
 * which will be invoked when the callbacks happen.
 *
 * We also save the required reference to the provided Java object (see the
 * sidebar on memory management at the start of this file).
 *
 * @param jlistener The corresponding java object.
 */
JSessionListener::JSessionListener(jobject jlistener)
    : jsessionListener(NULL)
{
    QCC_DbgPrintf(("JSessionListener::JSessionListener()"));

    JNIEnv* env = GetEnv();

    QCC_DbgPrintf(("JSessionListener::JSessionListener(): Taking weak global reference to SessionListener %p", jlistener));
    jsessionListener = env->NewWeakGlobalRef(jlistener);
    if (!jsessionListener) {
        QCC_LogError(ER_FAIL, ("JSessionListener::JSessionListener(): Can't create new weak global reference to SessionListener"));
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jlistener);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("JSessionListener::JSessionListener(): Can't GetObjectClass() for SessionListener"));
        return;
    }

    MID_sessionLost = env->GetMethodID(clazz, "sessionLost", "(I)V");
    if (!MID_sessionLost) {
        QCC_LogError(ER_FAIL, ("JSessionListener::JSessionListener(): Can't find sessionLost(I) in SessionListener"));
    }

    MID_sessionLostWithReason = env->GetMethodID(clazz, "sessionLost", "(II)V");
    if (!MID_sessionLostWithReason) {
        QCC_LogError(ER_FAIL, ("JSessionListener::JSessionListener(): Can't find sessionLost(II) in SessionListener"));
    }

    MID_sessionMemberAdded = env->GetMethodID(clazz, "sessionMemberAdded", "(ILjava/lang/String;)V");
    if (!MID_sessionMemberAdded) {
        QCC_LogError(ER_FAIL, ("JSessionListener::JSessionListener(): Can't find sessionMemberAdded() in SessionListener"));
    }

    MID_sessionMemberRemoved = env->GetMethodID(clazz, "sessionMemberRemoved", "(ILjava/lang/String;)V");
    if (!MID_sessionMemberRemoved) {
        QCC_LogError(ER_FAIL, ("JSessionListener::JSessionListener(): Can't find sessionMemberRemoved() in SessionListener"));
    }
}

/**
 * Destroy a JSessionListener C++ object.
 *
 * We remove the reference to the associated Java object when the C++ object
 * goes away.
 */
JSessionListener::~JSessionListener()
{
    QCC_DbgPrintf(("JSessionListener::~JSessionListener()"));

    if (jsessionListener) {
        QCC_DbgPrintf(("JSessionListener::~JSessionListener(): Releasing weak global reference to SessionListener %p", jsessionListener));
        GetEnv()->DeleteWeakGlobalRef(jsessionListener);
        jsessionListener = NULL;
    }
}

/**
 * Handle the C++ SessionLost callback from the AllJoyn system.
 *
 * Called by the bus when an existing session becomes disconnected.
 *
 * This is a callback returning void, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallVoidMethod().
 *
 * @param sessionId     Id of session that was lost.
 */
void JSessionListener::SessionLost(SessionId sessionId)
{
    QCC_DbgPrintf(("JSessionListener::SessionLost(%u)", sessionId));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    jint jsessionId = sessionId;

    /*
     * The weak global reference jsessionListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jsessionListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JSessionListener::SessionLost(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to the listener means that the sessionLost method must
     * be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JSessionListener::SessionLost(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_sessionLost, jsessionId);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JSessionListener::SessionLost(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JSessionListener::SessionLost(): Return"));
}
/**
 * Handle the C++ SessionLost callback from the AllJoyn system.
 *
 * Called by the bus when an existing session becomes disconnected.
 *
 * This is a callback returning void, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallVoidMethod().
 *
 * @param sessionId     Id of session that was lost.
 * @param reason        Reason for the session being lost.
 */
void JSessionListener::SessionLost(SessionId sessionId, SessionListener::SessionLostReason reason)
{
    QCC_DbgPrintf(("JSessionListener::SessionLost(%u, %u)", sessionId, reason));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    jint jsessionId = sessionId;

    jint jreason = reason;

    /*
     * The weak global reference jsessionListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jsessionListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JSessionListener::SessionLost(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to the listener means that the sessionLost method must
     * be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JSessionListener::SessionLost(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_sessionLostWithReason, jsessionId, jreason);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JSessionListener::SessionLost(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JSessionListener::SessionLost(): Return"));
}

/**
 * Handle the C++ SessionMemberAdded callback from the AllJoyn system.
 *
 * Called by the bus when a new member joins an existing multipoint session.
 *
 * This is a callback returning void, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallVoidMethod().
 *
 * @param sessionId     Id of session that whose members changed.
 * @param uniqueName    Unique name that joined the multipoint session.
 */
void JSessionListener::SessionMemberAdded(SessionId sessionId, const char* uniqueName)
{
    QCC_DbgPrintf(("JSessionListener::SessionMemberAdded()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    jint jsessionId = sessionId;
    JLocalRef<jstring> juniqueName = env->NewStringUTF(uniqueName);


    /*
     * The weak global reference jsessionListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jsessionListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JSessionListener::SessionMemberAdded(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to the listener means that the sessionMemberAdded method must
     * be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JSessionListener::SessionMemberAdded(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_sessionMemberAdded, jsessionId, (jstring)juniqueName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JSessionListener::SessionMemberAdded(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JSessionListener::SessionMemberAdded(): Return"));
}

/**
 * Handle the C++ SessionMemberRemoved callback from the AllJoyn system.
 *
 * Called by the bus when an existing member leaves a multipoint session.
 *
 * This is a callback returning void, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallVoidMethod().
 *
 * @param sessionId     Id of session that whose members changed.
 * @param uniqueName    Unique name that left the multipoint session.
 */
void JSessionListener::SessionMemberRemoved(SessionId sessionId, const char* uniqueName)
{
    QCC_DbgPrintf(("JSessionListener::SessionMemberRemoved()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    jint jsessionId = sessionId;
    JLocalRef<jstring> juniqueName = env->NewStringUTF(uniqueName);

    /*
     * The weak global reference jsessionListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jsessionListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JSessionListener::SessionMemberRemoved(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to the listener means that the sessionMemberRemoved method must
     * be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JSessionListener::SessionMemberRemoved(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_sessionMemberRemoved, jsessionId, (jstring)juniqueName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JSessionListener::SessionMemberRemoved(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JSessionListener::SessionMemberRemoved(): Return"));
}

/**
 * Construct a JSessionPortListener C++ object by arranging the correspondence
 * between the C++ object being constructed and the provided Java object.
 *
 * Since the purpose of the SessionListener is to allow a client to recieve
 * callbacks from the AllJoyn system, we need to connect the C++ methods to
 * the java methods.  We do that using Java reflection.  In the constructor
 * we do the expensive work of finding the Java method IDs (MID_xxx below)
 * which will be invoked when the callbacks happen.
 *
 * We also save the required reference to the provided Java object (see the
 * sidebar on memory management at the start of this file).
 *
 * @param jlistener The corresponding java object.
 */
JSessionPortListener::JSessionPortListener(jobject jlistener)
    : jsessionPortListener(NULL)
{
    QCC_DbgPrintf(("JSessionPortListener::JSessionPortListener()"));

    JNIEnv* env = GetEnv();

    QCC_DbgPrintf(("JSessionPortListener::JSessionPortListener(): Taking weak global reference to SessionPortListener %p", jlistener));
    jsessionPortListener = env->NewWeakGlobalRef(jlistener);
    if (!jsessionPortListener) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::JSessionPortListener(): Can't create new weak global reference to SessionPortListener"));
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jlistener);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::JSessionPortListener(): Can't GetObjectClass() for SessionPortListener"));
        return;
    }

    MID_acceptSessionJoiner = env->GetMethodID(clazz, "acceptSessionJoiner", "(SLjava/lang/String;Lorg/alljoyn/bus/SessionOpts;)Z");
    if (!MID_acceptSessionJoiner) {
        QCC_DbgPrintf(("JSessionPortListener::JSessionPortListener(): Can't find acceptSessionJoiner() in SessionPortListener"));
    }

    MID_sessionJoined = env->GetMethodID(clazz, "sessionJoined", "(SILjava/lang/String;)V");
    if (!MID_sessionJoined) {
        QCC_DbgPrintf(("JSessionPortListener::JSessionPortListener(): Can't find sessionJoined() in SessionPortListener"));
    }
}

/**
 * Destroy a JSessionPortListener C++ object.
 *
 * We remove the reference to the associated Java object when the C++
 * object goes away.
 */
JSessionPortListener::~JSessionPortListener()
{
    QCC_DbgPrintf(("JSessionPortListener::~JSessionPortListener()"));
    if (jsessionPortListener) {
        QCC_DbgPrintf(("JSessionPortListener::~JSessionPortListener(): Releasing weak global reference to SessionPortListener %p", jsessionPortListener));
        GetEnv()->DeleteWeakGlobalRef(jsessionPortListener);
        jsessionPortListener = NULL;
    }
}

/**
 * Handle the C++ AcceptSessionJoiner callback from the AllJoyn system.  Accept
 * or reject an incoming JoinSession request. The session does not exist until
 * this after this function returns.
 *
 * This callback is only used by session creators. Therefore it is only called
 * on listeners passed to BusAttachment::BindSessionPort.
 *
 * This is a callback returning bool, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallBoolMethod().
 *
 * @param sessionPort    Session port that was joined.
 * @param joiner         Unique name of potential joiner.
 * @param opts           Session options requested by the joiner.
 * @return   Return true if JoinSession request is accepted. false if rejected.
 */
bool JSessionPortListener::AcceptSessionJoiner(SessionPort sessionPort, const char* joiner, const SessionOpts& opts)
{
    QCC_DbgPrintf(("JSessionPortListener::AcceptSessionJoiner()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    JLocalRef<jstring> jjoiner = env->NewStringUTF(joiner);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::AcceptSessionJoiner(): Exception"));
        return false;
    }

    jmethodID mid = env->GetMethodID(CLS_SessionOpts, "<init>", "()V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::AcceptSessionJoiner(): Can't find SessionOpts constructor"));
        return false;
    }

    QCC_DbgPrintf(("JSessionPortListener::AcceptSessionJoiner(): Create new SessionOpts"));
    JLocalRef<jobject> jsessionopts = env->NewObject(CLS_SessionOpts, mid);
    if (!jsessionopts) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::AcceptSessionJoiner(): Cannot create SessionOpts"));
    }

    QCC_DbgPrintf(("JSessionPortListener::AcceptSessionJoiner(): Load SessionOpts"));
    jfieldID fid = env->GetFieldID(CLS_SessionOpts, "traffic", "B");
    env->SetByteField(jsessionopts, fid, opts.traffic);

    fid = env->GetFieldID(CLS_SessionOpts, "isMultipoint", "Z");
    env->SetBooleanField(jsessionopts, fid, opts.isMultipoint);

    fid = env->GetFieldID(CLS_SessionOpts, "proximity", "B");
    env->SetByteField(jsessionopts, fid, opts.proximity);

    fid = env->GetFieldID(CLS_SessionOpts, "transports", "S");
    env->SetShortField(jsessionopts, fid, opts.transports);

    /*
     * The weak global reference jsessionPortListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jsessionPortListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::AcceptSessionJoiner(): Can't get new local reference to SessionListener"));
        return false;
    }

    /*
     * This call out to the listener means that the acceptSessionJoiner method
     * must be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JSessionPortListener::AcceptSessionJoiner(): Call out to listener object and method"));
    bool result = env->CallBooleanMethod(jo, MID_acceptSessionJoiner, sessionPort, (jstring)jjoiner, (jobject)jsessionopts);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::AcceptSessionJoiner(): Exception"));
        return false;
    }

    QCC_DbgPrintf(("JSessionPortListener::AcceptSessionJoiner(): Return result %d", result));
    return result;
}

/**
 * Handle the C++ SessionJoined callback from the AllJoyn system.
 *
 * Called by the bus when a session has been successfully joined. The session is
 * now fully up.
 *
 * This callback is only used by session creators. Therefore it is only called
 * on listeners passed to BusAttachment::BindSessionPort.
 *
 * This is a callback returning void, so we just need to translate the C++
 * formal parameters we got from AllJoyn into their Java counterparts; call the
 * corresponding Java method in the listener object using the helper method
 * env->CallVoidMethod().
 *
 * @param sessionPort    Session port that was joined.
 * @param id             Id of session.
 * @param joiner         Unique name of the joiner.
 */
void JSessionPortListener::SessionJoined(SessionPort sessionPort, SessionId id, const char* joiner)
{
    QCC_DbgPrintf(("JSessionPortListener::SessionJoined()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    JLocalRef<jstring> jjoiner = env->NewStringUTF(joiner);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::SessionJoined(): Exception"));
    }

    /*
     * The weak global reference jsessionPortListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jsessionPortListener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::SessionJoined(): Can't get new local reference to SessionListener"));
        return;
    }

    /*
     * This call out to the listener means that the sessionJoined method
     * must be MT-Safe.  This is implied by the definition of the listener.
     */
    QCC_DbgPrintf(("JSessionPortListener::SessionJoined(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_sessionJoined, sessionPort, id, (jstring)jjoiner);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JSessionPortListener::SessionJoined(): Exception"));
        return;
    }

    QCC_DbgPrintf(("JSessionPortListener::SessionJoined(): Return"));
}

/**
 * Construct a JAuthListener C++ object by arranging the correspondence between
 * the C++ object being constructed and the provided Java object.
 *
 * Since the purpose of the AuthListener is to allow a client to recieve
 * callbacks from the AllJoyn system, we need to connect the C++ methods to
 * the java methods.  We do that using Java reflection.  In the constructor
 * we do the expensive work of finding the Java method IDs (MID_xxx below)
 * which will be invoked when the callbacks happen.
 *
 * We also save the required weak reference to the provided Java object (see
 * the sidebar on memory management at the start of this file).
 *
 * Objects of this class are expected to be MT-Safe between construction and
 * destruction.
 *
 * @param jlistener The corresponding java object.
 */
JAuthListener::JAuthListener(JBusAttachment* ba, jobject jlistener)
    : busPtr(ba), jauthListener(NULL)
{
    QCC_DbgPrintf(("JAuthListener::JAuthListener()"));

    /*
     * We have a reference to the underlying bus attachment, so we have to
     * increment its reference count.
     */
    QCC_DbgPrintf(("JAuthListener::JAuthListener(): Refcount on busPtr before is %d", busPtr->GetRef()));
    busPtr->IncRef();
    QCC_DbgPrintf(("JAuthListener::JAuthListener(): Refcount on busPtr after %d", busPtr->GetRef()));

    JNIEnv* env = GetEnv();

    QCC_DbgPrintf(("JAuthListener::JAuthListener(): Taking weak global reference to AuthListener %p", jlistener));
    jauthListener = env->NewWeakGlobalRef(jlistener);
    if (!jauthListener) {
        QCC_LogError(ER_FAIL, ("JAuthListener::JAuthListener(): Can't create new weak global reference to AuthListener"));
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jlistener);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("JAuthListener::JAuthListener(): Can't GetObjectClass() for AuthListener"));
        return;
    }

    MID_requestCredentials = env->GetMethodID(clazz, "requestCredentials", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;I)Lorg/alljoyn/bus/AuthListener$Credentials;");
    if (!MID_requestCredentials) {
        QCC_LogError(ER_FAIL, ("JAuthListener::JAuthListener(): Can't find requestCredentials() in AuthListener"));
        return;
    }

    MID_verifyCredentials = env->GetMethodID(clazz, "verifyCredentials", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z");
    if (!MID_verifyCredentials) {
        QCC_LogError(ER_FAIL, ("JAuthListener::JAuthListener(): Can't find verifyCredentials() in jListener"));
        return;
    }

    MID_securityViolation = env->GetMethodID(clazz, "securityViolation", "(Lorg/alljoyn/bus/Status;)V");
    if (!MID_securityViolation) {
        QCC_LogError(ER_FAIL, ("JAuthListener::JAuthListener(): Can't find securityViolation() in jListener"));
        return;
    }

    MID_authenticationComplete = env->GetMethodID(clazz, "authenticationComplete", "(Ljava/lang/String;Ljava/lang/String;Z)V");
    if (!MID_authenticationComplete) {
        QCC_LogError(ER_FAIL, ("JAuthListener::JAuthListener(): Can't find authenticationComplete() in jListener"));
        return;
    }
}

/**
 * Destroy a JAuthListener C++ object.
 *
 * We remove the weak reference to the associated Java object when the C++
 * object goes away.
 */
JAuthListener::~JAuthListener()
{
    QCC_DbgPrintf(("JAuthListener::~JAuthListener()"));

    /*
     * We have a reference to the underlying bus attachment, so we have to
     * decrement its reference count.  Once we decrement it, the object can
     * go away at any time, so we must immediately forget it.
     */
    QCC_DbgPrintf(("JAuthListener::~JAuthListener(): Refcount on busPtr before decrement is %d", busPtr->GetRef()));
    busPtr->DecRef();
    busPtr = NULL;

    if (jauthListener) {
        QCC_DbgPrintf(("JAuthListener::~JAuthListener(): Releasing weak global reference to AuthListener %p", jauthListener));
        GetEnv()->DeleteWeakGlobalRef(jauthListener);
        jauthListener = NULL;
    }
}

/**
 * Handle the C++ RequestCredentials callback from the AllJoyn system.
 *
 * This method is called when the authentication mechanism requests user
 * credentials. If the user name is not an empty string the request is for
 * credentials for that specific user. A count allows the listener to decide
 * whether to allow or reject multiple authentication attempts to the same peer.
 *
 * @param authMechanism  The name of the authentication mechanism issuing the request.
 * @param authPeer       The name of the remote peer being authenticated.  On the initiating
 *                       side this will be a well-known-name for the remote peer. On the
 *                       accepting side this will be the unique bus name for the remote peer.
 * @param authCount      Count (starting at 1) of the number of authentication request attempts made.
 * @param userName       The user name for the credentials being requested.
 * @param credMask       A bit mask identifying the credentials being requested. The application
 *                       may return none, some or all of the requested credentials.
 * @param[out] credentials    The credentials returned.
 *
 * @return  The caller should return true if the request is being accepted or false if the
 *          requests is being rejected. If the request is rejected the authentication is
 *          complete.
 */
bool JAuthListener::RequestCredentials(const char* authMechanism, const char* authPeer, uint16_t authCount,
                                       const char* userName, uint16_t credMask, Credentials& credentials)
{
    QCC_DbgPrintf(("JAuthListener::RequestCredentials()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    JLocalRef<jstring> jauthMechanism = env->NewStringUTF(authMechanism);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Can't get new UTF string"));
        return false;
    }

    JLocalRef<jstring> jauthPeer = env->NewStringUTF(authPeer);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Can't get new UTF string"));
        return false;
    }

    JLocalRef<jstring> juserName = env->NewStringUTF(userName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Can't get new UTF string"));
        return false;
    }

    /*
     * Take the authentication changed lock to prevent clients from changing the
     * authListener out from under us while we are calling out into it.
     */
    busPtr->baAuthenticationChangeLock.Lock();

    /*
     * The weak global reference jauthListener cannot be directly used.  We have
     * to get a "hard" reference to it and then use that.  If you try to use a
     * weak reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jauthListener);
    if (!jo) {
        busPtr->baAuthenticationChangeLock.Unlock();
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Can't get new local reference to AuthListener"));
        return false;
    }

    /*
     * This call out to the listener means that the requestCredentials method must
     * be MT-Safe.  This is implied by the definition of the listener.
     */
    JLocalRef<jobject> jcredentials = CallObjectMethod(env, jo, MID_requestCredentials,
                                                       (jstring)jauthMechanism,
                                                       (jstring)jauthPeer,
                                                       authCount,
                                                       (jstring)juserName,
                                                       credMask);
    /*
     * Once we have made our call, the client can go ahead and make any changes
     * to the authListener it sees fit.
     */
    busPtr->baAuthenticationChangeLock.Unlock();

    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Exception calling out to Java method"));
        return false;
    }

    if (!jcredentials) {
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Null return from Java method"));
        return false;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jcredentials);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Can't GetObjectClass() for Credentials"));
        return false;
    }

    jfieldID fid = env->GetFieldID(clazz, "password", "[B");
    if (!fid) {
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Can't find password field in Credentials"));
        return false;
    }

    JLocalRef<jbyteArray> jpassword = (jbyteArray)env->GetObjectField(jcredentials, fid);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Can't get password byte array from Credentials"));
        return false;
    }

    if (jpassword) {
        jbyte* password = env->GetByteArrayElements(jpassword, NULL);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("JAuthListener::RequestCredentials(): Can't get password bytes"));
            return false;
        }
        credentials.SetPassword(String((const char*)password, env->GetArrayLength(jpassword)));
        memset(password, 0, env->GetArrayLength(jpassword) * sizeof(jbyte));
        env->ReleaseByteArrayElements(jpassword, password, 0);
    }

    fid = env->GetFieldID(clazz, "userName", "Ljava/lang/String;");
    if (!fid) {
        return false;
    }

    juserName = (jstring)env->GetObjectField(jcredentials, fid);
    if (env->ExceptionCheck()) {
        return false;
    }

    if (juserName) {
        JString userName(juserName);
        credentials.SetUserName(userName.c_str());
    }

    fid = env->GetFieldID(clazz, "certificateChain", "Ljava/lang/String;");
    if (!fid) {
        return false;
    }

    JLocalRef<jstring> jcertificate = (jstring)env->GetObjectField(jcredentials, fid);
    if (env->ExceptionCheck()) {
        return false;
    }

    if (jcertificate) {
        JString certificate(jcertificate);
        credentials.SetCertChain(certificate.c_str());
    }

    fid = env->GetFieldID(clazz, "privateKey", "Ljava/lang/String;");
    if (!fid) {
        return false;
    }

    JLocalRef<jstring> jprivateKey = (jstring)env->GetObjectField(jcredentials, fid);
    if (env->ExceptionCheck()) {
        return false;
    }

    if (jprivateKey) {
        JString privateKey(jprivateKey);
        credentials.SetPrivateKey(privateKey.c_str());
    }

    fid = env->GetFieldID(clazz, "logonEntry", "[B");
    if (!fid) {
        return false;
    }

    JLocalRef<jbyteArray> jlogonEntry = (jbyteArray)env->GetObjectField(jcredentials, fid);
    if (env->ExceptionCheck()) {
        return false;
    }

    if (jlogonEntry) {
        jbyte* logonEntry = env->GetByteArrayElements(jlogonEntry, NULL);
        if (env->ExceptionCheck()) {
            return false;
        }
        credentials.SetLogonEntry(String((const char*)logonEntry, env->GetArrayLength(jlogonEntry)));
        memset(logonEntry, 0, env->GetArrayLength(jlogonEntry) * sizeof(jbyte));
        env->ReleaseByteArrayElements(jlogonEntry, logonEntry, 0);
    }

    fid = env->GetFieldID(clazz, "expiration", "Ljava/lang/Integer;");
    if (!fid) {
        return false;
    }

    JLocalRef<jobject> jexpiration = (jobject)env->GetObjectField(jcredentials, fid);
    if (env->ExceptionCheck()) {
        return false;
    }

    if (jexpiration) {
        jint seconds = env->CallIntMethod(jexpiration, MID_Integer_intValue);
        if (env->ExceptionCheck()) {
            return false;
        }
        credentials.SetExpiration(seconds);
    }

    if (env->ExceptionCheck()) {
        return false;
    }
    return true;
}

/**
 * Handle the C++ VerifyCredentials callback from the AllJoyn system.
 *
 * This method is called when the authentication mechanism requests verification
 * of credentials from a remote peer.
 *
 * @param authMechanism  The name of the authentication mechanism issuing the request.
 * @param peerName       The name of the remote peer being authenticated.  On the initiating
 *                       side this will be a well-known-name for the remote peer. On the
 *                       accepting side this will be the unique bus name for the remote peer.
 * @param credentials    The credentials to be verified.
 *
 * @return  The listener should return true if the credentials are acceptable or false if the
 *          credentials are being rejected.
 */
bool JAuthListener::VerifyCredentials(const char* authMechanism, const char* authPeer, const Credentials& credentials)
{
    QCC_DbgPrintf(("JAuthListener::VerifyCredentials()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    JLocalRef<jstring> jauthMechanism = env->NewStringUTF(authMechanism);
    if (env->ExceptionCheck()) {
        return false;
    }

    JLocalRef<jstring> jauthPeer = env->NewStringUTF(authPeer);
    if (env->ExceptionCheck()) {
        return false;
    }

    JLocalRef<jstring> juserName = credentials.IsSet(AuthListener::CRED_USER_NAME) ?
                                   env->NewStringUTF(credentials.GetUserName().c_str()) : NULL;
    if (env->ExceptionCheck()) {
        return false;
    }

    JLocalRef<jstring> jcert = credentials.IsSet(AuthListener::CRED_CERT_CHAIN) ?
                               env->NewStringUTF(credentials.GetCertChain().c_str()) : NULL;
    if (env->ExceptionCheck()) {
        return false;
    }

    /*
     * Take the authentication changed lock to prevent clients from changing the
     * authListener out from under us while we are calling out into it.
     */
    busPtr->baAuthenticationChangeLock.Lock();

    /*
     * The weak global reference jauthListener cannot be directly used.  We have
     * to get a "hard" reference to it and then use that.  If you try to use a
     * weak reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jauthListener);
    if (!jo) {
        busPtr->baAuthenticationChangeLock.Unlock();
        QCC_LogError(ER_FAIL, ("JAuthListener::Verifyredentials(): Can't get new local reference to AuthListener"));
        return false;
    }

    jboolean acceptable = env->CallBooleanMethod(jo, MID_verifyCredentials, (jstring)jauthMechanism,
                                                 (jstring)jauthPeer, (jstring)juserName, (jstring)jcert);

    /*
     * Once we have made our call, the client can go ahead and make any changes
     * to the authListener it sees fit.
     */
    busPtr->baAuthenticationChangeLock.Unlock();

    if (env->ExceptionCheck()) {
        return false;
    }
    return acceptable;
}

/**
 * Handle the C++ SecurityViolation callback from the AllJoyn system.
 *
 * This is an optional callback that, if implemented, allows an application to
 * monitor security violations. This function is called when an attempt to
 * decrypt an encrypted messages failed or when an unencrypted message was
 * received on an interface that requires encryption. The message contains only
 * header information.
 *
 * @param status  A status code indicating the type of security violation.
 * @param msg     The message that cause the security violation.
 */
void JAuthListener::SecurityViolation(QStatus status, const Message& msg)
{
    QCC_DbgPrintf(("JAuthListener::SecurityViolation()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    MessageContext context(msg);
    JLocalRef<jobject> jstatus = JStatus(status);
    if (env->ExceptionCheck()) {
        return;
    }

    /*
     * Take the authentication changed lock to prevent clients from changing the
     * authListener out from under us while we are calling out into it.
     */
    busPtr->baAuthenticationChangeLock.Lock();

    /*
     * The weak global reference jauthListener cannot be directly used.  We have
     * to get a "hard" reference to it and then use that.  If you try to use a
     * weak reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jauthListener);
    if (!jo) {
        busPtr->baAuthenticationChangeLock.Unlock();
        QCC_LogError(ER_FAIL, ("JAuthListener::SecurityViolation(): Can't get new local reference to AuthListener"));
        return;
    }

    env->CallVoidMethod(jo, MID_securityViolation, (jobject)jstatus);

    /*
     * Once we have made our call, the client can go ahead and make any changes
     * to the authListener it sees fit.
     */
    busPtr->baAuthenticationChangeLock.Unlock();

}

/**
 * Handle the C++ AuthenticationComplete callback from the AllJoyn system.
 *
 * Reports successful or unsuccessful completion of authentication.
 *
 * @param authMechanism  The name of the authentication mechanism that was used or an empty
 *                       string if the authentication failed.
 * @param peerName       The name of the remote peer being authenticated.  On the initiating
 *                       side this will be a well-known-name for the remote peer. On the
 *                       accepting side this will be the unique bus name for the remote peer.
 * @param success        true if the authentication was successful, otherwise false.
 */
void JAuthListener::AuthenticationComplete(const char* authMechanism, const char* authPeer,  bool success)
{
    QCC_DbgPrintf(("JAuthListener::AuthenticationComplete()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    JLocalRef<jstring> jauthMechanism = env->NewStringUTF(authMechanism);
    if (env->ExceptionCheck()) {
        return;
    }

    JLocalRef<jstring> jauthPeer = env->NewStringUTF(authPeer);
    if (env->ExceptionCheck()) {
        return;
    }

    /*
     * Take the authentication changed lock to prevent clients from changing the
     * authListener out from under us while we are calling out into it.
     */
    busPtr->baAuthenticationChangeLock.Lock();

    /*
     * The weak global reference jauthListener cannot be directly used.  We have
     * to get a "hard" reference to it and then use that.  If you try to use a
     * weak reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jauthListener);
    if (!jo) {
        busPtr->baAuthenticationChangeLock.Unlock();
        QCC_LogError(ER_FAIL, ("JAuthListener::AuthenticationComplete(): Can't get new local reference to AuthListener"));
        return;
    }

    env->CallVoidMethod(jo, MID_authenticationComplete, (jstring)jauthMechanism, (jstring)jauthPeer, success);

    /*
     * Once we have made our call, the client can go ahead and make any changes
     * to the authListener it sees fit.
     */
    busPtr->baAuthenticationChangeLock.Unlock();
}

/**
 * Create a new C++ backing object for the Java Bus Attachment object.  This is
 * an intrusively reference counted object so the destructor should never be
 * called directly.
 */
JBusAttachment::JBusAttachment(const char* applicationName, bool allowRemoteMessages, int concurrency)
    : BusAttachment(applicationName, allowRemoteMessages, concurrency),
    keyStoreListener(NULL),
    jkeyStoreListenerRef(NULL),
    authListener(NULL),
    aboutObj(NULL),
    jauthListenerRef(NULL),
    refCount(1)
{
    QCC_DbgPrintf(("JBusAttachment::JBusAttachment()"));
}

/**
 * Destroy the C++ backing object for the Java Bus Attachment object.  This is
 * an intrusively reference counted object so the destructor should never be
 * called directly.
 */
JBusAttachment::~JBusAttachment()
{
    QCC_DbgPrintf(("JBusAttachment::~JBusAttachment()"));

    /*
     * Note that the Bus Objects for this Bus Attachment are assumed to have
     * previously been released, since they will have held references to the
     * bus attachment that would have prevented its reference count from
     * going to zero and thus kept the bus attachment alive.
     */
    assert(busObjects.size() == 0);
}

/**
 * Connect the bus attachment to the underlying daemon.
 */
QStatus JBusAttachment::Connect(const char* connectArgs, jobject jkeyStoreListener, const char* authMechanisms,
                                jobject jauthListener, const char* keyStoreFileName, jboolean isShared)
{
    QCC_DbgPrintf(("JBusAttachment::Connect()"));

    JNIEnv* env = GetEnv();

    QCC_DbgPrintf(("JBusAttachment::Connect(): Starting the underlying bus attachment"));
    QStatus status = Start();
    if (ER_OK != status) {
        return status;
    }

    /*
     * The higher level Java BusAttachment constructor creates a default
     * AuthListener for us.  A KeyStoreListener can be set by explicit call,
     * which may or may not have been done.  We provide separate
     * registerAuthListener and registerKeyStoreListener functions in the Java
     * Bus Attachment as well.
     *
     * It is a bit confusing, but calling the Java version of
     * registerAuthListener results in a call out to AllJoyn, providing a new
     * AuthListener to us through enablePeerSecurity.  Calling the Java version
     * of registerKeyStoreListener just sets the Java member variable containing
     * the listener reference.  The only chance you get to change the key store
     * listener is here in connect.
     *
     * We take the authentication change lock here just in case someone is doing
     * something completely strange and bizarre as calling registerAuthListener
     * at the same time as she is calling connect() on a different thread.
     */
    QCC_DbgPrintf(("JBusAttachment::Connect(): Taking Bus Attachment authentication listener change lock"));
    baAuthenticationChangeLock.Lock();

    if (jkeyStoreListener) {
        QCC_DbgPrintf(("JBusAttachment::Connect(): Taking strong global reference to KeyStoreListener %p", jkeyStoreListener));
        jkeyStoreListenerRef = env->NewGlobalRef(jkeyStoreListener);

        keyStoreListener = new JKeyStoreListener(jkeyStoreListener);
        if (!keyStoreListener) {
            Throw("java/lang/OutOfMemoryError", NULL);
        }
        if (env->ExceptionCheck()) {
            status = ER_FAIL;
            goto exit;
        }

        RegisterKeyStoreListener(*keyStoreListener);
    }

    status = EnablePeerSecurity(authMechanisms, jauthListener, keyStoreFileName, isShared);
    if (ER_OK != status) {
        goto exit;
    }

    status = BusAttachment::Connect(connectArgs);

    exit :
    if (ER_OK != status) {
        Disconnect(connectArgs);

        QCC_DbgPrintf(("JBusAttachment::Connect(): Forgetting jkeyStoreListenerRef"));
        env->DeleteGlobalRef(jkeyStoreListenerRef);
        jkeyStoreListenerRef = NULL;

        QCC_DbgPrintf(("JBusAttachment::Connect(): Deleting keyStoreListener"));
        delete keyStoreListener;
        keyStoreListener = NULL;
    }

    QCC_DbgPrintf(("JBusAttachment::Connect(): Releasing Bus Attachment authentication listener change lock"));
    baAuthenticationChangeLock.Unlock();
    return status;
}

void JBusAttachment::Disconnect(const char* connectArgs)
{
    QCC_DbgPrintf(("JBusAttachment::Disconnect()"));

    if (IsConnected()) {
        QCC_DbgPrintf(("JBusAttachment::Disconnect(): calling BusAttachment::Disconnect()"));
        QStatus status = BusAttachment::Disconnect(connectArgs);
        if (ER_OK != status) {
            QCC_LogError(status, ("Disconnect failed"));
        }
    }

    // TODO: DisablePeerSecurity
    // TODO: UnregisterKeyStoreListener
    if (IsStarted()) {
        QCC_DbgPrintf(("JBusAttachment::Disconnect(): calling Stop()"));
        QStatus status = Stop();
        if (ER_OK != status) {
            QCC_LogError(status, ("Stop failed"));
        }

        QCC_DbgPrintf(("JBusAttachment::Disconnect(): calling Join()"));
        status = Join();
        if (ER_OK != status) {
            QCC_LogError(status, ("Join failed"));
        }
    }

    /*
     * Whenever we arrange a callback path from AllJoyn to Java, there is a
     * Java object and a C++ object involved.  Typically, the Java object "owns"
     * the C++ object.  In some cases, especially with the Java anonymous class
     * idiom, we have to hold a global strong reference to the Java object to
     * ensure that it is not garbage collected, which would result in the C++
     * object being freed before AllJoyn is notified that it should no longer
     * call back into the C++ object.  Since we hold those references, we have
     * to release them.
     *
     * As soon as we disconnected from the bus, we are guaranteed that we will
     * no longer receive callbacks, so we can now release all of the listener
     * resources we may have accumulated.  If we are holding the last reference
     * to the listener object, its finalize() method will be called, which will
     * cause its native resources to be released.  If the client still holds a
     * reference, the release will be delayed until the client releases the
     * reference.  The exception is user context objects which we are just
     * passing through uninterpreted.  We hold a reference to them, but there
     * is no corresponding native resource.
     */
    JNIEnv* env = GetEnv();

    /*
     * We need to be able to access objects in both the global bus object map
     * and the bus attachment in a critical section.  Since we have multiple
     * threads accessing multiple critical sections, lock order is important.
     * We always acquire the global lock first and then the bus attachment lock
     * and we always release the bus attachment lock first and then the global
     * object lock.  This must be done wherever these two lock objects are used
     * to avoid deadlock.
     */
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Taking global Bus Object map lock"));
    gBusObjectMapLock.Lock();

    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Taking Bus Attachment common lock"));
    baCommonLock.Lock();

    /*
     * Release any strong references we may hold to Java bus listener objects.
     */
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing BusListeners"));
    for (list<jobject>::iterator i = busListeners.begin(); i != busListeners.end(); ++i) {
        JBusListener* listener = GetNativeListener<JBusListener*>(env, *i);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("JBusAttachment::Disconnect(): Exception"));
            baCommonLock.Unlock();
            return;
        }
        QCC_DbgPrintf(("JBusAttachment::Disconnect(): Call UnregisterBusListener()"));
        UnregisterBusListener(*listener);
        QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing strong global reference to BusListener %p", *i));
        env->DeleteGlobalRef(*i);
    }
    busListeners.clear();

    /*
     * Release any strong references we may hold to Java translator objects.
     */
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing Translators"));
    for (list<jobject>::iterator i = translators.begin(); i != translators.end(); ++i) {
        QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing strong global reference to Translator %p", *i));
        env->DeleteGlobalRef(*i);
    }
    translators.clear();

    /*
     * Release any strong references we may hold to objects passed in through an
     * async join.  We assume that since we have done a disconnect/stop/join, there
     * will never be a callback firing that expects to call out into one of these.
     */
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing PendingAsyncJoins"));
    for (list<PendingAsyncJoin*>::iterator i = pendingAsyncJoins.begin(); i != pendingAsyncJoins.end(); ++i) {
        QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing strong global reference to SessionListener %p", (*i)->jsessionListener));
        env->DeleteGlobalRef((*i)->jsessionListener);
        QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing strong global reference to OnJoinSessionListener %p", (*i)->jonJoinSessionListener));
        env->DeleteGlobalRef((*i)->jonJoinSessionListener);
        if ((*i)->jcontext) {
            QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing strong global reference to context Object %p", (*i)->jcontext));
            env->DeleteGlobalRef((*i)->jcontext);
        }
    }
    pendingAsyncJoins.clear();

    /*
     * Release any strong references we may hold to objects passed in through an
     * async ping.  We assume that since we have done a disconnect/stop/join, there
     * will never be a callback firing that expects to call out into one of these.
     */
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing PendingAsyncPings"));
    for (list<PendingAsyncPing*>::iterator i = pendingAsyncPings.begin(); i != pendingAsyncPings.end(); ++i) {
        QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing strong global reference to OnPingListener %p", (*i)->jonPingListener));
        env->DeleteGlobalRef((*i)->jonPingListener);
        if ((*i)->jcontext) {
            QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing strong global reference to context Object %p", (*i)->jcontext));
            env->DeleteGlobalRef((*i)->jcontext);
        }
    }
    pendingAsyncPings.clear();

    /*
     * Release any strong references we may hold to objects passed in through a
     * bind.
     */
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing SessionPortListeners"));
    for (map<SessionPort, jobject>::iterator i = sessionPortListenerMap.begin(); i != sessionPortListenerMap.end(); ++i) {
        if (i->second) {
            QCC_DbgPrintf(("JBusAttachment::Disconnect(): Call UnbindSessionPort(%d)", i->first));
            UnbindSessionPort(i->first);
            QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing strong global reference to SessionPortListener %p", i->second));
            env->DeleteGlobalRef(i->second);
        }
    }
    sessionPortListenerMap.clear();

    /*
     * Release any strong references we may hold to objects passed in through a
     * join session.
     */
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing SessionListeners"));
    for (map<SessionId, BusAttachmentSessionListeners>::iterator i = sessionListenerMap.begin(); i != sessionListenerMap.end(); ++i) {
        if (i->second.jhostedListener) {
            QCC_DbgPrintf(("JBusAttachment::Disconnect(): Call SetHostedSessionListener(%d, %p)", i->first, 0));
            SetHostedSessionListener(i->first, 0);
            env->DeleteGlobalRef(i->second.jhostedListener);
        }
        if (i->second.jjoinedListener) {
            QCC_DbgPrintf(("JBusAttachment::Disconnect(): Call SetJoinedSessionListener(%d, %p)", i->first, 0));
            SetJoinedSessionListener(i->first, 0);
            env->DeleteGlobalRef(i->second.jjoinedListener);
        }
        if (i->second.jListener) {
            QCC_DbgPrintf(("JBusAttachment::Disconnect(): Call SetSessionListener(%d, %p)", i->first, 0));
            SetSessionListener(i->first, 0);
            env->DeleteGlobalRef(i->second.jListener);
        }
    }
    sessionListenerMap.clear();

    /*
     * Release any strong references we may hold to objects passed in through a
     * security API.
     */
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing AuthListener"));
    if (authListener) {
        EnablePeerSecurity(0, 0, 0, true);
        delete authListener;
    }
    authListener = NULL;
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Forgetting jauthListenerRef %p", jauthListenerRef));
    env->DeleteGlobalRef(jauthListenerRef);

    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing KeyStoreListener"));
    delete keyStoreListener;
    keyStoreListener = NULL;
    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Forgetting jkeyStoreListenerRef"));
    env->DeleteGlobalRef(jkeyStoreListenerRef);

    if (aboutObj != NULL) {
        aboutObj->jaboutObjGlobalRefLock.Lock();
        if (aboutObj->jaboutObjGlobalRef != NULL) {
            env->DeleteGlobalRef(aboutObj->jaboutObjGlobalRef);
            aboutObj->jaboutObjGlobalRef = NULL;
        }
        aboutObj->jaboutObjGlobalRefLock.Unlock();
    }

    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing Bus Attachment common lock"));
    baCommonLock.Unlock();

    QCC_DbgPrintf(("JBusAttachment::Disconnect(): Releasing global Bus Object map lock"));
    gBusObjectMapLock.Unlock();
}

QStatus JBusAttachment::EnablePeerSecurity(const char* authMechanisms, jobject jauthListener, const char* keyStoreFileName, jboolean isShared)
{
    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity()"));

    JNIEnv* env = GetEnv();
    if (!authMechanisms || !IsStarted()) {
        return ER_OK;
    }

    /*
     * We are going to release the common lock when calling out to AllJoyn since
     * it may call back in during processing of EnablePeerSecurity.  We
     * therefore need to take the authentication change lock to prevent a user
     * from sneaking in and changing the authentication listeners out from under
     * us when we do the call out.
     */
    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Taking Bus Attachment authentication listener change lock"));
    baAuthenticationChangeLock.Lock();

    /*
     * Since we are playing with multiple objects that need to be kept consistent, we
     * need to take the bus attachment lock while doing so.
     *
     * Since there are now multiple locks involved, we need to pay attention to
     * lock order. Since we need to release the common lock during the callout, we
     * take the common lock second and release it first.

     */
    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Taking Bus Attachment common lock"));
    baCommonLock.Lock();

    /*
     * Since we are going to associate a C++ backing object to the provided listener,
     * and this will plumb AllJoyn callbacks into the Java listener object, we need
     * to take a strong global reference to the listener to ensure the object stays
     * around until we are done with it.
     */
    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Taking strong global reference to AuthListener %p", jauthListener));
    jauthListenerRef = env->NewGlobalRef(jauthListener);
    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Remembering %p", jauthListenerRef));
    if (!jauthListenerRef) {
        QCC_LogError(ER_FAIL, ("JBusAttachment::EnablePeerSecurity(): Unable to take strong global reference to AuthListener %p", jauthListener));

        QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Releasing Bus Attachment common lock"));
        baCommonLock.Unlock();

        QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Releasing Bus Attachment authentication listener change lock"));
        baAuthenticationChangeLock.Unlock();
        return ER_FAIL;
    }

    /*
     * Whenever a new listener is provided, we need to associate a new C++ listener with
     * it.  The listener needs to get back to the bus attachment to access the MT locks.
     */
    delete authListener;
    authListener = new JAuthListener(this, jauthListener);
    if (!authListener) {
        QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Forgetting jauthListenerRef %p", jauthListenerRef));
        env->DeleteGlobalRef(jauthListenerRef);
        jauthListenerRef = NULL;

        Throw("java/lang/OutOfMemoryError", NULL);
    }

    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JBusAttachment::EnablePeerSecurity(): Exception"));

        QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Releasing Bus Attachment common lock"));
        baCommonLock.Unlock();

        QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Releasing Bus Attachment authentication listener change lock"));
        baAuthenticationChangeLock.Unlock();
        return ER_FAIL;
    }

    /*
     * Who knows what the AllJoyn version of EnablePeerSecurity is actually
     * going to do.  It may decide that it needs to call back out into the
     * bindings, so we need to be careful to release the BusAttachment common
     * lock to avoid deadlock.  We continue to hold the authentication listener
     * change lock to prevent another call on another thread changing our
     * listeners out from under us.
     */
    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Releasing Bus Attachment common lock"));
    baCommonLock.Unlock();

    QStatus status = BusAttachment::EnablePeerSecurity(authMechanisms, authListener, keyStoreFileName, isShared);

    /*
     * We're back, and depending on what has happened out from under us we
     * we will need to tweak the listeners.  So we need to take back the
     * common lock.
     */
    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Taking Bus Attachment common lock"));
    baCommonLock.Lock();

    /*
     * If we got an error, we don't need to keep a reference to the Java Object and we don't need the
     * C++ object, so we get rid of them here.
     */
    if (ER_OK != status) {
        delete authListener;
        authListener = NULL;

        QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Forgetting jauthListenerRef %p", jauthListenerRef));
        env->DeleteGlobalRef(jauthListenerRef);
        jauthListenerRef = NULL;
    }

    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Releasing Bus Attachment common lock"));
    baCommonLock.Unlock();

    QCC_DbgPrintf(("JBusAttachment::EnablePeerSecurity(): Releasing Bus Attachment authentication listener change lock"));
    baAuthenticationChangeLock.Unlock();

    return status;
}

bool JBusAttachment::IsLocalBusObject(jobject jbusObject)
{
    QCC_DbgPrintf(("JBusAttachment::IsLocalBusObject(%p)", jbusObject));

    JNIEnv* env = GetEnv();

    for (list<jobject>::iterator i = busObjects.begin(); i != busObjects.end(); ++i) {
        if (env->IsSameObject(jbusObject, *i)) {
            QCC_DbgPrintf(("JBusAttachment::IsLocalBusObject(): yes"));
            return true;
        }
    }

    QCC_DbgPrintf(("JBusAttachment::IsLocalBusObject(): no"));
    return false;
}

void JBusAttachment::ForgetLocalBusObject(jobject jbusObject)
{
    QCC_DbgPrintf(("JBusAttachment::ForgetLocalBusObject(%p)", jbusObject));

    JNIEnv* env = GetEnv();

    for (list<jobject>::iterator i = busObjects.begin(); i != busObjects.end(); ++i) {
        if (env->IsSameObject(jbusObject, *i)) {
            busObjects.erase(i);
            return;
        }
    }
}

QStatus JBusAttachment::RegisterBusObject(const char* objPath, jobject jbusObject,
                                          jobjectArray jbusInterfaces, jboolean jsecure,
                                          jstring jlangTag, jstring jdesc, jobject jtranslator)
{
    QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(%p)", jbusObject));

    /*
     * We need to be able to access objects in both the global bus object map
     * and the bus attachment in a critical section.  Since we have multiple
     * threads accessing multiple critical sections, lock order is important.
     * We always acquire the global lock first and then the bus attachment lock
     * and we always release the bus attachment lock first and then the global
     * object lock.  This must be done wherever these two lock objects are used
     * to avoid deadlock.
     */
    QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Taking global Bus Object map lock"));
    gBusObjectMapLock.Lock();

    QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Taking Bus Attachment common lock"));
    baCommonLock.Lock();

    /*
     * It is a programming error to register any bus object with a given bus
     * attachment multiple times.
     */
    if (IsLocalBusObject(jbusObject)) {
        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing Bus Attachment common lock"));
        baCommonLock.Unlock();

        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        return ER_BUS_OBJ_ALREADY_EXISTS;
    }

    JNIEnv* env = GetEnv();

    /*
     * We always take a global strong reference to a Java Bus Object that
     * we are going to use in any way.
     */
    QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Taking strong global reference to BusObject %p", jbusObject));
    jobject jglobalref = env->NewGlobalRef(jbusObject);
    if (!jglobalref) {
        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing Bus Attachment common lock"));
        baCommonLock.Unlock();

        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        return ER_FAIL;
    }

    /*
     * We need to remember that we have a hold on this bus object so we can
     * release it if we destruct without the user calling UnregisterBusObject
     */
    QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Remembering strong global reference to BusObject %p", jglobalref));
    busObjects.push_back(jglobalref);

    /*
     * It is a programming error to register the same Java Bus Object with
     * multiple bus attachments.  It looks like it should be possible from
     * the top, but that is not the case.
     */
    JBusObject* busObject = GetBackingObject(jglobalref);
    if (busObject) {
        /*
         * If AllJoyn doesn't get a hold on the Java Bus Object, we shouldn't
         * correspondingly have a hold on it.
         */
        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Forgetting jglobalref"));
        env->DeleteGlobalRef(jglobalref);

        /*
         * Release our hold on the shared resources, remembering to reverse the
         * lock order.
         */
        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing Bus Attachment common lock"));
        baCommonLock.Unlock();

        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        return ER_BUS_OBJ_ALREADY_EXISTS;
    } else {
        busObject = new JBusObject(this, objPath, jglobalref);
        busObject->AddInterfaces(jbusInterfaces);
        busObject->SetDescriptions(jlangTag, jdesc, jtranslator);
        if (env->ExceptionCheck()) {
            delete busObject;
            QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing Bus Attachment common lock"));
            baCommonLock.Unlock();

            QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing global Bus Object map lock"));
            gBusObjectMapLock.Unlock();
            return ER_FAIL;
        }

        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Taking hold of Bus Object %p", jbusObject));
        NewRefBackingObject(jglobalref, busObject);
    }

    /*
     * After we enter this call, AllJoyn has its hands on the bus object and
     * calls in can start flowing.
     */
    QStatus status = BusAttachment::RegisterBusObject(*busObject, jsecure);
    if (status != ER_OK) {
        /*
         * AllJoyn balked at us for some reason.  As a result we really don't
         * need to have a hold on any of the objects we've acquired references
         * to or created.  If we created the C++ backing object, we'll get
         * responsibility for its disposition from DecRefBackingObject.
         * release our global reference to that as well.
         */
        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): RegisterBusObject fails.  DecRefBackingObject on %p", jbusObject));
        JBusObject* cppObject = DecRefBackingObject(jglobalref);
        if (cppObject) {
            delete cppObject;
            cppObject = NULL;
        }

        /*
         * If AllJoyn doesn't have a hold on the Java Bus Object, we shouldn't
         * correspondingly have a hold on it.
         */
        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Forgetting jglobalref"));
        env->DeleteGlobalRef(jglobalref);
    }

    /*
     * We've successfully arranged for our AllJoyn Bus Attachment to use the
     * provided Bus Object.  Release our hold on the shared resources,
     * remembering to reverse the lock order.
     */
    QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing Bus Attachment common lock"));
    baCommonLock.Unlock();

    QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Releasing global Bus Object map lock"));
    gBusObjectMapLock.Unlock();
    return ER_OK;
}

void JBusAttachment::UnregisterBusObject(jobject jbusObject)
{
    QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(%p)", jbusObject));

    /*
     * We need to be able to access objects in both the global bus object map
     * and the bus attachment in a critical section.  Since we have multiple
     * threads accessing multiple critical sections, lock order is important.
     * We always acquire the global lock first and then the bus attachment lock
     * and we always release the bus attachment lock first and then the global
     * object lock.  This must be done wherever these two lock objects are used
     * to avoid deadlock.
     */
    QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Taking global Bus Object map lock"));
    gBusObjectMapLock.Lock();

    QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Releasing Bus Attachment common lock"));
    baCommonLock.Lock();

    /*
     * It is a programming error to 1) register a Bus Object on one Bus
     * Attachment and unregister it on another; 2) unregister a Bus Object that
     * has never been regsitered with the given Bus Attachment; and 3)
     * unregister a Bus Object multiple times on a given Bus Attachment.  All of
     * these cases are caught by making sure the provided Java Bus Object is
     * currently in the list of Java Objects associated with this bus
     * Attachment.
     */
    if (!IsLocalBusObject(jbusObject)) {
        QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Releasing Bus Attachment common lock"));
        baCommonLock.Unlock();

        QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        QCC_LogError(ER_BUS_OBJ_NOT_FOUND, ("JBusAttachment::UnregisterBusObject(): No existing Java Bus Object"));
        return;
    }

    JBusObject* cppObject = GetBackingObject(jbusObject);
    if (cppObject == NULL) {
        QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Releasing Bus Attachment common lock"));
        baCommonLock.Unlock();

        QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        QCC_LogError(ER_BUS_OBJ_NOT_FOUND, ("JBusAttachment::UnregisterBusObject(): No existing Backing Object"));
        return;
    }

    /*
     * As soon as this call completes, AllJoyn will not make any further
     * calls into the object, so we can safely get rid of it, and we can
     * release our hold on the corresponding Java object and allow it to
     * be garbage collected.
     */
    BusAttachment::UnregisterBusObject(*cppObject);

    /*
     * AllJoyn doesn't have its grubby little hands on the C++ Object any
     * more.  As a result we shouldn't have a hold on any of the objects
     * we've acquired to support the plumbing.
     *
     * Just because we don't need the C++ object doesn't mean that other
     * bus attachments don't need it, so we need to pay attention to the
     * reference counting mechanism.  If DecRefBackingObject returns a
     * pointer to the object, we
     */
    QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Getting global ref for jbusObject %p", jbusObject));
    jobject jo = GetGlobalRefForObject(jbusObject);

    QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): DecRefBackingObject on %p", jbusObject));
    JBusObject* cppObjectToDelete = DecRefBackingObject(jo);
    if (cppObjectToDelete) {
        /*
         * The object we delete had better be the object we just told AllJoyn
         * about.
         */
        assert(cppObjectToDelete == cppObject);
        delete cppObject;
        cppObject = NULL;
    }

    /*
     * AllJoyn shouldn't be remembering the Java Bus Object as a bus
     * object associated with this bus attachment.  We've now changed
     * the structure of the busObjects list so the iterator is
     * invalid, so mark it as such.
     */
    ForgetLocalBusObject(jo);

    /*
     * And we shouldn't correspondingly have a hold on the Java
     * reference.
     */
    QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Deleting global reference to  %p", jo));
    GetEnv()->DeleteGlobalRef(jo);

    /*
     * We've successfully arranged for our AllJoyn Bus Attachment to stop using
     * the provided Bus Object.  Release our hold on the shared resources,
     * remembering to reverse the lock order.
     */
    QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Releasing Bus Attachment common lock"));
    baCommonLock.Unlock();

    QCC_DbgPrintf(("JBusAttachment::UnregisterBusObject(): Releasing global Bus Object map lock"));
    gBusObjectMapLock.Unlock();
    return;
}

template <typename T>
QStatus JBusAttachment::RegisterSignalHandler(const char* ifaceName, const char* signalName,
                                              jobject jsignalHandler, jobject jmethod, const char* ancillary)
{
    QCC_DbgPrintf(("JBusAttachment::RegisterSignalHandler(): Taking Bus Attachment common lock"));
    baCommonLock.Lock();

    /*
     * Whenever we get an object from the outside world that we are going to
     * wire together with a C++ object, we take a strong global reference to it.
     * We also get a method here but we assume that since the method refers to
     * an annotation of a method in the provided object it will stay around if
     * we put a hold on the object.
     */
    JNIEnv* env = GetEnv();
    jobject jglobalref = env->NewGlobalRef(jsignalHandler);
    if (!jglobalref) {
        QCC_DbgPrintf(("JBusAttachment::RegisterSignalHandler(): Releasing Bus Attachment common lock"));
        baCommonLock.Unlock();
        return ER_FAIL;
    }

    /*
     * Create the C++ object that backs the Java signal handler object.
     */
    JSignalHandler* signalHandler = new T(jsignalHandler, jmethod);
    if (signalHandler == NULL) {
        Throw("java/lang/OutOfMemoryError", NULL);
        baCommonLock.Unlock();
        return ER_FAIL;
    }

    /*
     * Wire the C++ signal handler to the Java signal handler and if the
     * operation was successful, remember both the Java object and the C++
     * object.  If it didn't work then we might as well forget them both.
     */
    QStatus status = signalHandler->Register(*this, ifaceName, signalName, ancillary);
    if (ER_OK == status) {
        signalHandlers.push_back(make_pair(jglobalref, signalHandler));
    } else {
        delete signalHandler;
        QCC_DbgPrintf(("JBusAttachment::RegisterBusObject(): Forgetting jglobalref"));
        env->DeleteGlobalRef(jglobalref);
    }

    QCC_DbgPrintf(("JBusAttachment::RegisterSignalHandler(): Releasing Bus Attachment common lock"));
    baCommonLock.Unlock();

    return status;
}

void JBusAttachment::UnregisterSignalHandler(jobject jsignalHandler, jobject jmethod)
{
    QCC_DbgPrintf(("JBusAttachment::UnregisterSignalHandler(): Taking Bus Attachment common lock"));
    baCommonLock.Lock();

    JNIEnv* env = GetEnv();

    for (vector<pair<jobject, JSignalHandler*> >::iterator i = signalHandlers.begin(); i != signalHandlers.end(); ++i) {
        if (i->second->IsSameObject(jsignalHandler, jmethod)) {
            i->second->Unregister(*this);
            delete (i->second);
            QCC_DbgPrintf(("JBusAttachment::UnregisterSignalHandler(): Forgetting %p", i->first));
            env->DeleteGlobalRef(i->first);
            signalHandlers.erase(i);
            break;
        }
    }

    QCC_DbgPrintf(("JBusAttachment::UnregisterSignalHandler(): Releasing Bus Attachment common lock"));
    baCommonLock.Unlock();
}

/**
 * The native C++ implementation of the Java class BusAttachment.create method
 * found in src/org/alljoyn/bus/BusAttachment.java
 *
 * This method allocates any C++ resources that may be associated with the Java
 * BusAttachment; and is expected to be called from the BusAttachment constructor.
 *
 * The picture to keep in mind is that there is a Java BusAttachment object which
 * is presented to the Java user.  As described in the sidebars at the start of
 * this file, the Java BusAttachment has a corresponding C++ class.  In order to
 * simplify lifetime issues for the C++ class, it is accesed through a ManagedObj
 * which is a reference counting mechanism.  Recall from the sidebars that Java
 * objects use an opaque handle to get at the C++ objects, and C++ objects use a
 * weak object reference to get at the Java objects.  The opaque handle in this
 * case is that managed object pointer.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param japplicationName A name to give the application.  Used primarily in
 *                         authentication.
 * @param allowRemoteMessages If true allow communication with attachments
 *                            on physically remote attachments.
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_create(JNIEnv* env, jobject thiz, jstring japplicationName, jboolean allowRemoteMessages, jint concurrency)
{
    // Temporary debugging use
    // QCC_UseOSLogging(true);
    // QCC_SetDebugLevel("ALLJOYN_JAVA", 7);

    QCC_DbgPrintf(("BusAttachment_create()"));

    JString applicationName(japplicationName);
    if (env->ExceptionCheck()) {
        return;
    }

    const char* name = applicationName.c_str();

    /*
     * Create a new C++ backing object for the Java BusAttachment.  This is
     * an intrusively reference counted object.
     */
    JBusAttachment* busPtr = new JBusAttachment(name, allowRemoteMessages, concurrency);
    if (!busPtr) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    QCC_DbgPrintf(("BusAttachment_create(): Refcount on busPtr is %d", busPtr->GetRef()));
    QCC_DbgPrintf(("BusAttachment_create(): Remembering busPtr as %p", busPtr));
    SetHandle(thiz, busPtr);
    if (env->ExceptionCheck()) {
        /*
         * can't directly delete the JBusAttachment since it is refcounted.
         */
        QCC_DbgPrintf(("BusAttachment_create(): Refcount on busPtr before decrement is %d", busPtr->GetRef()));
        busPtr->DecRef();
        busPtr = NULL;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_emitChangedSignal(
    JNIEnv* env, jobject thiz, jobject jbusObject, jstring jifaceName, jstring jpropName, jobject jpropValue, jint sessionId)
{
    QCC_DbgPrintf(("BusAttachment_emitChangedSignal()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck() || busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_emitChangedSignal(): Exception or NULL bus pointer"));
        return;
    }

    JString ifaceName(jifaceName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_emitChangedSignal(): Exception"));
        return;
    }

    JString propName(jpropName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_emitChangedSignal(): Exception"));
        return;
    }

    //ScopedMutexLock guard(gBusObjectMapLock);
    gBusObjectMapLock.Lock();
    JBusObject* busObject = GetBackingObject(jbusObject);

    if (!busObject) {
        QCC_DbgPrintf(("BusAttachment_emitChangedSignal(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        QCC_LogError(ER_FAIL, ("BusAttachment_emitChangedSignal(): Exception"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_BUS_NO_SUCH_OBJECT));
        return;
    }

    MsgArg* arg = NULL;
    MsgArg value;

    if (jpropValue) {
        const BusAttachment& bus = busObject->GetBusAttachment();
        const InterfaceDescription* iface = bus.GetInterface(ifaceName.c_str());
        assert(iface);
        const InterfaceDescription::Property* prop = iface->GetProperty(propName.c_str());
        assert(prop);
        arg = Marshal(prop->signature.c_str(), jpropValue, &value);
    }

    if (busObject) {
        busObject->EmitPropChanged(ifaceName.c_str(), propName.c_str(), (arg ? *arg : value), sessionId);
    }

    gBusObjectMapLock.Unlock();
    QCC_DbgPrintf(("BusAttachment_emitChangedSignal(): Releasing global Bus Object map lock"));
}

/**
 * The native C++ implementation of the Java class BusAttachment.destroy method
 * found in src/org/alljoyn/bus/BusAttachment.java
 *
 * This method releasess any C++ resources that may be associated with the Java
 * BusAttachment; and is expected to be called from the BusAttachment finalizer
 * method.
 *
 * The picture to keep in mind is that there is a Java BusAttachment object which
 * is presented to the Java user.  As described in the sidebars at the start of
 * this file, the Java BusAttachment has a corresponding C++ class.
 *
 * The Java object lifetime is managed by the JVM garbage collector, but the
 * C++ object needs to be explicitly managed.  In order to accomodate this, we
 * hook the finalize() method in the Java BusAttachment finalize() method, which
 * will be called when the Java object has been determined to be garbage.
 *
 * This method is called in BusAttachment.finalize() in order to do the explicit
 * release of the associated C++ object.  Recall that the reference to the C++
 * object is stored in the "handle" field of the BusAttachment.  We get at it
 * using GetHandle.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_destroy(JNIEnv* env,
                                                                  jobject thiz)
{
    QCC_DbgPrintf(("BusAttachment_destroy()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_destroy(): Exception"));
        return;
    }

    if (busPtr == NULL) {
        QCC_DbgPrintf(("BusAttachment_destroy(): Already destroyed. Returning."));
        return;
    }

    /*
     * We want to allow users to forget the BusAttachent in Java by setting a
     * reference to null.  We want to reclaim all of our resources, including
     * those held by BusObjects which hold references to our bus attachment.  We
     * don't want to force our user to explicitly unregister those bus objects,
     * which is the only way we can get an indication that the BusObject is
     * going away.  This is becuase BusObjects are interfaces and we have no way
     * to hook the finalize on those objects and drive release of the underlying
     * resources.
     *
     * So we want to and can use this method (destroy) to drive the release of
     * all of the Java bus object C++ backing objects.  Since the garbage
     * collector has run on the bus attachment (we are running here) we know
     * there is no way for a user to access the bus attachment.  We assume that
     * the BusAttachment release() and or finalize() methods have ensured that
     * the BusAttachment is disconnected and stopped, so it will never call out
     * to any of its associated objects.
     *
     * So, we release references to the Bus Objects that this particular Bus
     * Attachment holds now.  The theory is that nothing else can be accessing
     * the bus attachment or the bus obejcts, so we don't need to take the
     * multithread locks any more than the bus attachment destructor will.
     */
    QCC_DbgPrintf(("BusAttachment_destroy(): Releasing BusObjects"));
    for (list<jobject>::iterator i = busPtr->busObjects.begin(); i != busPtr->busObjects.end(); ++i) {
        /*
         * If we are the last BusAttachment to use this bus Object, we acquire
         * the memory management responsibility for the associated C++ object.
         * This is a vestige of an obsolete idea, but we still need to do it.
         * We expect we will always have the memory management responsibility.
         */
        QCC_DbgPrintf(("BusAttachment_destroy(): DecRefBackingObject on %p", *i));
        JBusObject* cppObject = DecRefBackingObject(*i);
        if (cppObject) {
            QCC_DbgPrintf(("BusAttachment_destroy(): deleting cppObject %p", cppObject));
            delete cppObject;
            cppObject = NULL;
        }

        QCC_DbgPrintf(("BusAttachment_destroy(): Releasing strong global reference to Bus Object %p", *i));
        env->DeleteGlobalRef(*i);
    }
    busPtr->busObjects.clear();

    /*
     * We don't want to directly delete a reference counted object, we want to
     * decrement the refererence count.  As soon as this refcount goes to zero
     * the object on the heap will be deallocated via a delete this, so we must
     * forget it now and forever.  Since we just released all of the bus object
     * references, we assume that the bus attachment actually goes away now.
     */
    QCC_DbgPrintf(("BusAttachment_destroy(): Refcount on busPtr is %d before decrement", busPtr->GetRef()));
    busPtr->DecRef();
    SetHandle(thiz, NULL);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_whoImplements(JNIEnv* env, jobject thiz, jobjectArray jinterfaces) {
    QStatus status = ER_OK;

    QCC_DbgPrintf(("BusAttachment_whoImplements()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck() || busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_whoImplements(): Exception or NULL bus pointer"));
        return JStatus(ER_FAIL);
    }
    QCC_DbgPrintf(("BusAttachment_whoImplements(): Refcount on busPtr is %d", busPtr->GetRef()));

    int len = (jinterfaces != NULL) ? env->GetArrayLength(jinterfaces) : 0;
    if (0 == len) {
        // both null and size zero interfaces are used the same.
        status = busPtr->WhoImplements(NULL, 0);
    } else {
        const char** rawIntfString = new const char*[len];
        memset(rawIntfString, 0, len * sizeof(const char*));
        jstring* jintfs = new jstring[len];
        memset(jintfs, 0, len * sizeof(jstring));
        for (int i = 0; i < len; ++i) {
            jintfs[i] = (jstring) GetObjectArrayElement(env, jinterfaces, i);
            if (env->ExceptionCheck() || NULL == jintfs[i]) {
                QCC_LogError(ER_FAIL, ("BusAttachment_whoImplements(): Exception"));
                status = ER_BAD_ARG_1;
                goto cleanup;
            }

            rawIntfString[i] = env->GetStringUTFChars(jintfs[i], NULL);
            if (NULL == rawIntfString[i]) {
                status = ER_BAD_ARG_1;
                goto cleanup;
            }
        }
        status = busPtr->WhoImplements(rawIntfString, len);
    cleanup:
        for (int i = 0; i < len; ++i) {
            if (jintfs[i] && rawIntfString[i]) {
                env->ReleaseStringUTFChars(jintfs[i], rawIntfString[i]);
            }
        }
        delete [] jintfs;
        jintfs = NULL;
        delete [] rawIntfString;
        rawIntfString = NULL;
    }
    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_cancelWhoImplements(JNIEnv* env, jobject thiz, jobjectArray jinterfaces) {
    QStatus status = ER_OK;

    QCC_DbgPrintf(("BusAttachment_cancelWhoImplements()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck() || busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelWhoImplements(): Exception or NULL bus pointer"));
        return JStatus(ER_FAIL);
    }
    QCC_DbgPrintf(("BusAttachment_cancelWhoImplements(): Refcount on busPtr is %d", busPtr->GetRef()));

    int len = (jinterfaces != NULL) ? env->GetArrayLength(jinterfaces) : 0;
    if (0 == len) {
        // both null and size zero interfaces are used the same.
        status = busPtr->CancelWhoImplements(NULL, 0);
    } else {
        const char** rawIntfString = new const char*[len];
        memset(rawIntfString, 0, len * sizeof(const char*));
        jstring* jintfs = new jstring[len];
        memset(jintfs, 0, len * sizeof(jstring));
        for (int i = 0; i < len; ++i) {
            jintfs[i] = (jstring) GetObjectArrayElement(env, jinterfaces, i);
            if (env->ExceptionCheck() || NULL == jintfs[i]) {
                QCC_LogError(ER_FAIL, ("BusAttachment_whoImplements(): Exception"));
                status = ER_BAD_ARG_1;
                goto cleanup;
            }

            rawIntfString[i] = env->GetStringUTFChars(jintfs[i], NULL);
            if (NULL == rawIntfString[i]) {
                status = ER_BAD_ARG_1;
                goto cleanup;
            }
        }
        status = busPtr->CancelWhoImplements(rawIntfString, len);
    cleanup:
        for (int i = 0; i < len; ++i) {
            if (jintfs[i] && rawIntfString[i]) {
                env->ReleaseStringUTFChars(jintfs[i], rawIntfString[i]);
            }
        }
        delete [] jintfs;
        jintfs = NULL;
        delete [] rawIntfString;
        rawIntfString = NULL;
    }
    return JStatus(status);
}

/**
 * Register an object that will receive bus event notifications.  In this
 * context, registering a listener should be thought of as "adding another
 * listener."  You may register zero (if you are not interested in receiving
 * notifications) or more listeners.
 *
 * The bus attachment is the way that Java clients and services talk to the bus,
 * but the bus needs a way to notify clients and services of events happening on
 * the bus.  This is done via an object with a number of methods corresponding
 * to callback functions that are invoked when bus events occur.
 *
 * The listener passed in as the "jobject jlistener" will be a reference to, or
 * perhaps descendent of, the java class BusListener.  As usual we need to create
 * an instance of a C++ object to receive the actual callbacks.  The responsibility
 * of this C++ object is just to call the corresponding methods in the Java object.
 *
 * As mentioned in the sidebar at the start of this file, the C++ object gets to the
 * Java object via a jobject reference back to the Java object which we pass in the
 * C++ object constructor.
 *
 * If the jobject needs to get back to the C++ object it does so via the "handle"
 * field and the helpers SetHandle() and GetHandle().
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param listener  Object instance that will receive bus event notifications.
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_registerBusListener(JNIEnv* env, jobject thiz, jobject jlistener)
{
    QCC_DbgPrintf(("BusAttachment_registerBusListener()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck() || busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_registerBusListener(): Exception or NULL bus pointer"));
        return;
    }
    QCC_DbgPrintf(("BusAttachment_registerBusListener(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * We always take a strong global reference to the listener object.
     */
    QCC_DbgPrintf(("BusAttachment_registerBusListener(): Taking strong global reference to BusListener %p", jlistener));
    jobject jglobalref = env->NewGlobalRef(jlistener);
    if (!jglobalref) {
        return;
    }

    QCC_DbgPrintf(("BusAttachment_registerBusListener(): Taking Bus Attachment common lock"));
    busPtr->baCommonLock.Lock();

    busPtr->busListeners.push_back(jglobalref);

    QCC_DbgPrintf(("BusAttachment_registerBusListener(): Releasing Bus Attachment common lock"));
    busPtr->baCommonLock.Unlock();

    /*
     * Get the C++ object that must be there backing the Java object
     */
    JBusListener* listener = GetNativeListener<JBusListener*>(env, jlistener);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_registerBusListener(): Exception"));
        return;
    }

    assert(listener);
    listener->Setup(thiz);

    /*
     * Make the call into AllJoyn.
     */
    QCC_DbgPrintf(("BusAttachment_registerBusListener(): Call RegisterBusListener()"));
    busPtr->RegisterBusListener(*listener);
}

/**
 * Unregister an object to prevent it from receiving further bus event
 * notifications.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param listener  Object instance that will receive bus event notifications.
 */
JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_unregisterBusListener(JNIEnv* env, jobject thiz, jobject jlistener)
{
    QCC_DbgPrintf(("BusAttachment_unregisterBusListener()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck() || busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_unregisterBusListener(): Exception or NULL bus pointer"));
        return;
    }
    QCC_DbgPrintf(("BusAttachment_unregisterBusListener(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Get the C++ object that must be there backing the Java object
     */
    JBusListener* listener = GetNativeListener<JBusListener*>(env, jlistener);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_unregisterBusListener(): Exception"));
        return;
    }

    assert(listener);

    /*
     * Make the call into AllJoyn.
     */
    QCC_DbgPrintf(("BusAttachment_unregisterBusListener(): Call UnregisterBusListener()"));
    busPtr->UnregisterBusListener(*listener);

    /*
     * We always take a reference to the Java listener in registerBusListener,
     * so we always release a reference here.
     */
    QCC_DbgPrintf(("BusAttachment_unregisterBusListener(): Taking Bus Attachment common lock"));
    busPtr->baCommonLock.Lock();

    for (list<jobject>::iterator i = busPtr->busListeners.begin(); i != busPtr->busListeners.end(); ++i) {
        if (env->IsSameObject(*i, jlistener)) {
            QCC_DbgPrintf(("BusAttachment_unregisterBusListener(): Releasing strong global reference to BusListener %p", jlistener));
            env->DeleteGlobalRef(*i);
            busPtr->busListeners.erase(i);
            break;
        }
    }

    QCC_DbgPrintf(("BusAttachment_unregisterBusListener(): Releasing Bus Attachment common lock"));
    busPtr->baCommonLock.Unlock();
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusListener_create(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("BusListener_create()"));

    assert(GetHandle<JBusListener*>(thiz) == NULL);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_create(): Exception"));
        return;
    }

    JBusListener* jbl = new JBusListener(thiz);
    if (jbl == NULL) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    SetHandle(thiz, jbl);
    if (env->ExceptionCheck()) {
        delete jbl;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusListener_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("BusListener_destroy()"));

    JBusListener* jbl = GetHandle<JBusListener*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusListener_destroy(): Exception"));
        return;
    }

    assert(jbl);
    delete jbl;

    SetHandle(thiz, NULL);
    return;
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_requestName(JNIEnv*env, jobject thiz,
                                                                         jstring jname, jint jflags)
{
    QCC_DbgPrintf(("BusAttachment_requestName()"));

    /*
     * Load the C++ well-known name with the Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_requestName(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_requestName(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_requestName(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_requestName(): Call RequestName(%s, 0x%08x)", name.c_str(), jflags));

    QStatus status = busPtr->RequestName(name.c_str(), jflags);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_requestName(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_requestName(): RequestName() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_releaseName(JNIEnv*env, jobject thiz,
                                                                         jstring jname)
{
    QCC_DbgPrintf(("BusAttachment_releaseName()"));

    /*
     * Load the C++ well-known name with the Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_releaseName(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_releaseName(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_releaseName(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */

    QCC_DbgPrintf(("BusAttachment_releaseName(): Call ReleaseName(%s)", name.c_str()));

    QStatus status = busPtr->ReleaseName(name.c_str());
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_releaseName(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_releaseName(): ReleaseName() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_addMatch(JNIEnv*env, jobject thiz,
                                                                      jstring jrule)
{
    QCC_DbgPrintf(("BusAttachment_addMatch()"));

    /*
     * Load the C++ well-known name with the Java well-known name.
     */
    JString rule(jrule);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_addMatch(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_addMatch(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_addMatch(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_addMatch(): Call AddMatch(%s)", rule.c_str()));

    QStatus status = busPtr->AddMatch(rule.c_str());
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_addMatch(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_addMatch(): AddMatch() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_removeMatch(JNIEnv*env, jobject thiz,
                                                                         jstring jrule)
{
    QCC_DbgPrintf(("BusAttachment_removeMatch()"));

    /*
     * Load the C++ well-known name with the Java well-known name.
     */
    JString rule(jrule);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_removeMatch(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_removeMatch(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_removeMatch(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_removeMatch(): Call RemoveMatch(%s)", rule.c_str()));

    QStatus status = busPtr->RemoveMatch(rule.c_str());
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_removeMatch(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_removeMatch(): RemoveMatch() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_advertiseName(JNIEnv* env, jobject thiz,
                                                                           jstring jname, jshort jtransports)
{
    QCC_DbgPrintf(("BusAttachment_advertiseName()"));

    /*
     * Load the C++ well-known name with the Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_advertiseName(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_advertiseName(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_advertiseName(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_advertiseName(): Call AdvertiseName(%s, 0x%04x)", name.c_str(), jtransports));

    QStatus status = busPtr->AdvertiseName(name.c_str(), jtransports);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_advertiseName(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_advertiseName(): AdvertiseName() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_cancelAdvertiseName(JNIEnv* env, jobject thiz,
                                                                                 jstring jname, jshort jtransports)
{
    QCC_DbgPrintf(("BusAttachment_cancelAdvertiseName()"));

    /*
     * Load the C++ well-known name Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelAdvertiseName(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelAdvertiseName(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_cancelAdvertiseName(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_cancelAdvertiseName(): Call CancelAdvertiseName(%s, 0x%04x)", name.c_str()));

    QStatus status = busPtr->CancelAdvertiseName(name.c_str(), jtransports);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelAdvertiseName(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_cancelAdvertiseName(): CancelAdvertiseName() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_findAdvertisedName(JNIEnv* env, jobject thiz,
                                                                                jstring jname)
{
    QCC_DbgPrintf(("BusAttachment_findAdvertisedName()"));

    /*
     * Load the C++ well-known name Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_findAdvertisedName(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_findAdvertisedName(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_findAdvertisedName(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_findAdvertisedName(): Call FindAdvertisedName(%s)", name.c_str()));

    QStatus status = busPtr->FindAdvertisedName(name.c_str());
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_findAdvertisedName(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_findAdvertisedName(): FindAdvertisedName() fails"));
    }

    return JStatus(status);
}
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_findAdvertisedNameByTransport(JNIEnv*env, jobject thiz,
                                                                                           jstring jname, jshort jtransports)
{
    QCC_DbgPrintf(("BusAttachment_findAdvertisedNameByTransport()"));

    /*
     * Load the C++ well-known name Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_findAdvertisedNameByTransport(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_findAdvertisedNameByTransport(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_findAdvertisedNameByTransport(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_findAdvertisedNameByTransport(): Call FindAdvertisedNameByTransport(%s, %d)", name.c_str(), jtransports));

    QStatus status = busPtr->FindAdvertisedNameByTransport(name.c_str(), jtransports);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_findAdvertisedNameByTransport(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_findAdvertisedNameByTransport(): FindAdvertisedNameByTransport() fails"));
    }

    return JStatus(status);
}
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_cancelFindAdvertisedName(JNIEnv* env, jobject thiz,
                                                                                      jstring jname)
{
    QCC_DbgPrintf(("BusAttachment_cancelFindAdvertisedName()"));

    /*
     * Load the C++ well-known name Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelFindAdvertisedName(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelFindAdvertisedName(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_cancelFindAdvertisedName(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_cancelFindAdvertisedName(): Call CancelFindAdvertisedName(%s)", name.c_str()));

    QStatus status = busPtr->CancelFindAdvertisedName(name.c_str());
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelFindAdvertisedName(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_cancelfindAdvertisedName(): CancelFindAdvertisedName() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_cancelFindAdvertisedNameByTransport(JNIEnv*env, jobject thiz,
                                                                                                 jstring jname, jshort jtransports)
{
    QCC_DbgPrintf(("BusAttachment_cancelFindAdvertisedNameByTransport()"));

    /*
     * Load the C++ well-known name Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelFindAdvertisedNameByTransport(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelFindAdvertisedNameByTransport(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_cancelFindAdvertisedNameByTransport(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_cancelFindAdvertisedNameByTransport(): Call CancelFindAdvertisedNameByTransport(%s, %d)", name.c_str(), jtransports));

    QStatus status = busPtr->CancelFindAdvertisedNameByTransport(name.c_str(), jtransports);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_cancelFindAdvertisedNameByTransport(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_cancelFindAdvertisedNameByTransport(): CancelFindAdvertisedNameByTransport() fails"));
    }

    return JStatus(status);
}
/**
 * Bind a session port with the BusAttachment.  This makes a SessionPort
 * available for external BusAttachments to join, and enables callbacks to the
 * associated listener.
 *
 * Each BusAttachment binds its own set of SessionPorts. Session joiners use the
 * bound session port along with the name of the attachement to create a
 * persistent logical connection (called a Session) with the original
 * BusAttachment.  A SessionPort and bus name form a unique identifier that
 * BusAttachments use internally as a "half-association" when joining a session.
 *
 * SessionPort values can be pre-arranged between AllJoyn services and their
 * clients (well-known SessionPorts) in much the same way as a well-known IP
 * port number, although SessionPorts have scope local to the associated
 * BusAttachment and not the local host.
 *
 * Once a session is joined using one of the service's well-known SessionPorts,
 * the service may bind additional SessionPorts (dynamically) and share these
 * SessionPorts with the joiner over the original session. The joiner can then
 * create additional sessions with the service by calling JoinSession with these
 * dynamic SessionPort ids.
 *
 * The bus will return events related to the management of sessions related to
 * the given session port through a listener object.  This listener object is
 * expected to inherit from class SessionPortListener and specialize the callback
 * methods in which a user is interested.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionPort The SessionPort value to bind or SESSION_PORT_ANY to allow
 *                     this method to choose an available port. On successful
 *                     return, this value contains the chosen SessionPort.
 * @param jsessionOpts Session options that joiners must agree to in order to
 *                     successfully join the session.
 * @param jsessionPortListener  Called by the bus when session related events occur.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_bindSessionPort(JNIEnv* env, jobject thiz,
                                                                             jobject jsessionPort, jobject jsessionOpts,
                                                                             jobject jlistener)
{
    QCC_DbgPrintf(("BusAttachment_bindSessionPort()"));

    /*
     * Load the C++ session port from the Java session port.
     */
    SessionPort sessionPort;
    JLocalRef<jclass> clazz = env->GetObjectClass(jsessionPort);
    jfieldID spValueFid = env->GetFieldID(clazz, "value", "S");
    assert(spValueFid);
    sessionPort = env->GetShortField(jsessionPort, spValueFid);

    /*
     * Load the C++ session options from the Java session options.
     */
    SessionOpts sessionOpts;
    clazz = env->GetObjectClass(jsessionOpts);
    jfieldID fid = env->GetFieldID(clazz, "traffic", "B");
    assert(fid);
    sessionOpts.traffic = static_cast<SessionOpts::TrafficType>(env->GetByteField(jsessionOpts, fid));

    fid = env->GetFieldID(clazz, "isMultipoint", "Z");
    assert(fid);
    sessionOpts.isMultipoint = env->GetBooleanField(jsessionOpts, fid);

    fid = env->GetFieldID(clazz, "proximity", "B");
    assert(fid);
    sessionOpts.proximity = env->GetByteField(jsessionOpts, fid);

    fid = env->GetFieldID(clazz, "transports", "S");
    assert(fid);
    sessionOpts.transports = env->GetShortField(jsessionOpts, fid);

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_bindSessionPort(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_bindSessionPort(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * We always take a strong global reference to the listener object.  We
     * can't just store the reference in one common place with all of the other
     * references since we have to be able to find it on the "un" call which
     * doesn't provide them.  We have a map of session port to listener object
     * for that reason.  In this case, the global reference does double duty,
     * allowing the search and holding on to the listener.  We take the
     * reference here since callbacks may start flowing the instant the AllJoyn
     * connection is made and we will have no time to ponder what may have
     * happened after we get back from the AllJoyn call.  By holding the
     * reference, we ensure that the corresponding C++ object is live.  We store
     * it (or not) below in a more convenient place where we can make the
     * decision.
     *
     * If we can't acquire the reference, then we are in an exception state and
     * returning NULL is okay.
     */
    QCC_DbgPrintf(("BusAttachment_bindSessionPort(): Taking strong global reference to SessionPortListener %p", jlistener));
    jobject jglobalref = env->NewGlobalRef(jlistener);
    if (!jglobalref) {
        return NULL;
    }

    /*
     * Get the C++ object that must be there backing the Java listener object
     */
    JSessionPortListener* listener = GetNativeListener<JSessionPortListener*>(env, jlistener);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_bindSessionPort(): Exception"));
        return NULL;
    }

    assert(listener);

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_bindSessionPort(): Call BindSessionPort(%d, <0x%02x, %d, 0x%02x, 0x%04x>, %p)",
                   sessionPort, sessionOpts.traffic, sessionOpts.isMultipoint, sessionOpts.proximity, sessionOpts.transports, listener));

    QStatus status = busPtr->BindSessionPort(sessionPort, sessionOpts, *listener);

    QCC_DbgPrintf(("BusAttachment_bindSessionPort(): Back from BindSessionPort(%d, <0x%02x, %d, 0x%02x, 0x%04x>, %p)",
                   sessionPort, sessionOpts.traffic, sessionOpts.isMultipoint, sessionOpts.proximity, sessionOpts.transports, listener));

    /*
     * If we get an exception down in the AllJoyn code, it's hard to know what
     * to do.  The good part is that the C++ code down in AllJoyn hasn't got a
     * clue that we're up here and won't throw any Java exceptions, so we should
     * be in good shape and never see this.  Famous last words, I know.  To be
     * safe, we'll keep the global reference(s) in place (leaking temporarily),
     * log the exception and let it propagate on up the stack to the client.
     */
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_bindSessionPort(): Exception"));
        return NULL;
    }

    /*
     * If we get an error from the AllJoyn code, we know from code inspection
     * that the C++ listener object will not be used.  In this case, the semantics
     * of the "un" operation are that it is not required.  Since the C++ listener
     * object will not be used, and the Java listener global reference is not
     * required, we can just forget about the provided listener.
     *
     * If we get a successful completion, we need to save the provided listener
     * in our session port map.  Since there can only be one successful bind to
     * any given session port we are guaranteed that there is a one-to-one map
     * between session port and active Java listener reference.
     */
    if (status == ER_OK) {
        QCC_DbgPrintf(("BusAttachment_bindSessionPort(): Success"));

        QCC_DbgPrintf(("BusAttachment_bindSessionPort(): Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();

        busPtr->sessionPortListenerMap[sessionPort] = jglobalref;

        QCC_DbgPrintf(("BusAttachment_bindSessionPort(): Releasing Bus Attachment common lock"));
        busPtr->baCommonLock.Unlock();
    } else {
        QCC_LogError(status, ("BusAttachment_bindSessionPort(): Error.  Forgetting jglobalref"));
        env->DeleteGlobalRef(jglobalref);
        return JStatus(status);
    }

    /*
     * Store the actual session port back in the session port out parameter
     */
    env->SetShortField(jsessionPort, spValueFid, sessionPort);

    return JStatus(status);
}

/**
 * Unbind (cancel) a session port with the BusAttachment.  This makes a
 * SessionPort unavailable for external BusAttachments to join, and disables
 * callbacks to the associated listener.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionPort The SessionPort value to bind or SESSION_PORT_ANY to allow
 *                     this method to choose an available port. On successful
 *                     return, this value contains the chosen SessionPort.
 * @param jsessionOpts Session options that joiners must agree to in order to
 *                     successfully join the session.
 * @param jsessionPortListener  Called by the bus when session related events occur.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_unbindSessionPort(JNIEnv* env, jobject thiz, jshort jsessionPort)
{
    QCC_DbgPrintf(("BusAttachment_unbindSessionPort()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_unbindSessionPort(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_unbindSessionPort(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_unbindSessionPort(): Call UnbindSessionPort(%d)", jsessionPort));

    QStatus status = busPtr->UnbindSessionPort(jsessionPort);

    /*
     * If we get an exception down in the AllJoyn code, it's hard to know what
     * to do.  The good part is that the C++ code down in AllJoyn hasn't got a
     * clue that we're up here and won't throw any Java exceptions, so we should
     * be in good shape and never see this.  Famous last words, I know.  To be
     * safe, we'll keep the global reference(s) in place (leaking temporarily),
     * log the exception and let it propagate on up the stack to the client.
     */
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_unbindSessionPort(): Exception"));
        return NULL;
    }

    /*
     * We did the call to unbind the session port but we have to ask ourselves
     * about the disposition of the C++ session port listener that was
     * associated with the possibly deceased session port.  Because we can
     * inspect the code we know that the only time the AllJoyn bus attachement
     * actually forgets the listener is when UnbindSessionPort returns status
     * ER_OK.
     *
     * This means that if there was any kind of error, AllJoyn can still call
     * back into our C++ listener.  Therefore we must keep it around.  Since the
     * C++ object is around, the Java object must be kept around to receive
     * translated callbacks from the C++ object.  This is a rather Byzantine
     * error and we are not going to try to harden the Java bindings against an
     * error of this type that is propagated up.
     *
     * It may be surprising to some that a failure to unbind a session port means
     * they might continue receiving notifications, but it may not be surprising
     * to others.  We'll just leave it at that.
     */
    if (status == ER_OK) {
        QCC_DbgPrintf(("BusAttachment_unbindSessionPort(): Success"));

        /*
         * We know that AllJoyn has released its hold on C++ listener object
         * referred to by our Java listener object.  We can now release our hold on
         * the Java listener object.
         */
        QCC_DbgPrintf(("BusAttachment_unbindSessionPort(): Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();

        jobject jglobalref = busPtr->sessionPortListenerMap[jsessionPort];
        busPtr->sessionPortListenerMap[jsessionPort] = 0;

        QCC_DbgPrintf(("BusAttachment_unbindSessionPort(): Releasing Bus Attachment common lock"));
        busPtr->baCommonLock.Unlock();

        QCC_DbgPrintf(("BusAttachment_bindSessionPort(): Releasing strong global reference to SessionPortListener %p", jglobalref));
        env->DeleteGlobalRef(jglobalref);
    } else {
        QCC_LogError(status, ("BusAttachment_bindSessionPort(): Error"));
    }

    return JStatus(status);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_SessionPortListener_create(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("SessionPortListener_create()"));

    assert(GetHandle<JSessionPortListener*>(thiz) == NULL);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("SessionPortListener_create(): Exception"));
        return;
    }

    JSessionPortListener* jspl = new JSessionPortListener(thiz);
    if (jspl == NULL) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    SetHandle(thiz, jspl);
    if (env->ExceptionCheck()) {
        delete jspl;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_SessionPortListener_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("SessionPortListener_destroy()"));

    JSessionPortListener* jspl = GetHandle<JSessionPortListener*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("SessionPortListener_destroy(): Exception"));
        return;
    }

    assert(jspl);
    delete jspl;

    SetHandle(thiz, NULL);
    return;
}

/**
 * Join a session bound to a given contact session port.
 *
 * Each BusAttachment binds its own set of SessionPorts. Session joiners use the
 * bound session port along with the name of the attachement to create a
 * persistent logical connection (called a Session) with the original
 * BusAttachment.  A SessionPort and bus name form a unique identifier that
 * BusAttachments use internally as a "half-association" when joining a session.
 *
 * SessionPort values can be pre-arranged between AllJoyn services and their
 * clients (well-known SessionPorts) in much the same way as a well-known IP
 * port number, although SessionPorts have scope local to the associated
 * BusAttachment and not the local host.
 *
 * The bus will return events related to the session through a listener object.
 * This listener object is expected to inherit from class SessionListener and
 * specialize the callback methods in which a user is interested.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionPort The SessionPort value representing the contact port.
 * @param jsessionId Set to the resulting SessionID value if the call succeeds.
 * @param jsessionOpts Session options that services must agree to in order to
 *                     successfully join the session.
 * @param jlistener Java listener object called when session related events occur.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_joinSession(JNIEnv* env, jobject thiz,
                                                                         jstring jsessionHost,
                                                                         jshort jsessionPort,
                                                                         jobject jsessionId,
                                                                         jobject jsessionOpts,
                                                                         jobject jlistener)
{
    QCC_DbgPrintf(("BusAttachment_joinSession()"));

    /*
     * Load the C++ session host string from the java parameter
     */
    JString sessionHost(jsessionHost);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSession(): Exception"));
        return NULL;
    }

    /*
     * Load the C++ session options from the Java session options.
     */
    SessionOpts sessionOpts;
    JLocalRef<jclass> clazz = env->GetObjectClass(jsessionOpts);
    jfieldID fid = env->GetFieldID(clazz, "traffic", "B");
    assert(fid);
    sessionOpts.traffic = static_cast<SessionOpts::TrafficType>(env->GetByteField(jsessionOpts, fid));

    fid = env->GetFieldID(clazz, "isMultipoint", "Z");
    assert(fid);
    sessionOpts.isMultipoint = env->GetBooleanField(jsessionOpts, fid);

    fid = env->GetFieldID(clazz, "proximity", "B");
    assert(fid);
    sessionOpts.proximity = env->GetByteField(jsessionOpts, fid);

    fid = env->GetFieldID(clazz, "transports", "S");
    assert(fid);
    sessionOpts.transports = env->GetShortField(jsessionOpts, fid);

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_bindSessionPort(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_joinSession(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * We always take a strong global reference to the listener object and hold
     * it as long as we can possibly get callbacks that use it.  If we can't
     * acquire the reference, then we are in an exception state and returning
     * NULL is okay.
     */
    QCC_DbgPrintf(("BusAttachment_joinSession(): Taking strong global reference to SessionListener %p", jlistener));
    jobject jglobalref = env->NewGlobalRef(jlistener);
    if (!jglobalref) {
        return NULL;
    }

    /*
     * Get the C++ object that must be there backing the Java listener object
     */
    JSessionListener* listener = GetNativeListener<JSessionListener*>(env, jlistener);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSession(): Exception"));
        return NULL;
    }

    assert(listener);

    /*
     * Make the AllJoyn call.
     */
    SessionId sessionId = 0;

    QCC_DbgPrintf(("BusAttachment_joinSession(): Call JoinSession(%s, %d, %p, %d,  <0x%02x, %d, 0x%02x, 0x%04x>)",
                   sessionHost.c_str(), jsessionPort, listener, sessionId, sessionOpts.traffic, sessionOpts.isMultipoint,
                   sessionOpts.proximity, sessionOpts.transports));

    QStatus status = busPtr->JoinSession(sessionHost.c_str(), jsessionPort, listener, sessionId, sessionOpts);

    QCC_DbgPrintf(("BusAttachment_joinSession(): Back from JoinSession(%s, %d, %p, %d,  <0x%02x, %d, 0x%02x, 0x%04x>)",
                   sessionHost.c_str(), jsessionPort, listener, sessionId, sessionOpts.traffic, sessionOpts.isMultipoint,
                   sessionOpts.proximity, sessionOpts.transports));

    /*
     * If we get an exception down in the AllJoyn code, it's hard to know what
     * to do.  The good part is that the C++ code down in AllJoyn hasn't got a
     * clue that we're up here and won't throw any Java exceptions, so we should
     * be in good shape and never see this.  Famous last words, I know.  To be
     * safe, we'll keep the global reference(s) in place (leaking temporarily),
     * log the exception and let it propagate on up the stack to the client.
     */
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSession(): Exception"));
        return NULL;
    }

    /*
     * If we get an error from the AllJoyn code, we know from code inspection
     * that the C++ listener object will not be used.  Since the C++ listener
     * object will not be used, and the Java listener global reference is not
     * required, we can just forget about the provided listener.
     *
     * If we get a successful completion, we need to save the provided listener
     * in our session listener map.  Since there can only be one successful join
     * to any given session we are guaranteed that there is a one-to-one map
     * between session ID and active Java listener reference.
     */
    if (status == ER_OK) {
        QCC_DbgPrintf(("BusAttachment_joinSession(): Success"));

        QCC_DbgPrintf(("BusAttachment_joinSession(): Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();

        busPtr->sessionListenerMap[sessionId].jListener = jglobalref;

        QCC_DbgPrintf(("BusAttachment_joinSession(): Releasing Bus Attachment common lock"));
        busPtr->baCommonLock.Unlock();
    } else {
        QCC_LogError(status, ("BusAttachment_joinSession(): Error.  Forgetting jglobalref"));
        env->DeleteGlobalRef(jglobalref);
        return JStatus(status);
    }

    /*
     * Store the session ID back in its out parameter.
     */
    clazz = env->GetObjectClass(jsessionId);
    fid = env->GetFieldID(clazz, "value", "I");
    assert(fid);
    env->SetIntField(jsessionId, fid, sessionId);

    /*
     * Store the Java session options from the returned [out] C++ session options.
     */
    clazz = env->GetObjectClass(jsessionOpts);

    fid = env->GetFieldID(clazz, "traffic", "B");
    assert(fid);
    env->SetByteField(jsessionOpts, fid, sessionOpts.traffic);

    fid = env->GetFieldID(clazz, "isMultipoint", "Z");
    assert(fid);
    env->SetBooleanField(jsessionOpts, fid, sessionOpts.isMultipoint);

    fid = env->GetFieldID(clazz, "proximity", "B");
    assert(fid);
    env->SetByteField(jsessionOpts, fid, sessionOpts.proximity);

    fid = env->GetFieldID(clazz, "transports", "S");
    assert(fid);
    env->SetShortField(jsessionOpts, fid, sessionOpts.transports);

    return JStatus(status);
}

typedef enum {
    BA_HSL,     // BusAttachment hosted session listener index
    BA_JSL,     // BusAttachment joined session listener index
    BA_SL,     // BusAttachment session listener index
    BA_LAST     // indicates the size of the enum
} BusAttachmentSessionListenerIndex;

static jobject leaveGenericSession(JNIEnv* env, jobject thiz,
                                   jint jsessionId, BusAttachmentSessionListenerIndex index)
{
    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QStatus status = ER_OK;
    jobject* jsessionListener = NULL;

    switch (index) {
    case BA_HSL:
        QCC_DbgPrintf(("Call LeaveHostedSession(%d)", jsessionId));
        status = busPtr->LeaveHostedSession(jsessionId);
        jsessionListener = &busPtr->sessionListenerMap[jsessionId].jhostedListener;
        break;

    case BA_JSL:
        QCC_DbgPrintf(("Call LeaveJoinedSession(%d)", jsessionId));
        status = busPtr->LeaveJoinedSession(jsessionId);
        jsessionListener = &busPtr->sessionListenerMap[jsessionId].jjoinedListener;
        break;

    case BA_SL:
        QCC_DbgPrintf(("Call LeaveSession(%d)\r\n", jsessionId));
        status = busPtr->LeaveSession(jsessionId);
        if (ER_OK == status) {
            jsessionListener = &busPtr->sessionListenerMap[jsessionId].jListener;
        }
        break;

    default:
        QCC_LogError(ER_FAIL, ("Exception unknown BusAttachmentSessionListenerIndex %d", index));
        assert(0);
    }

    /*
     * If we get an exception down in the AllJoyn code, it's hard to know what
     * to do.  The good part is that the C++ code down in AllJoyn hasn't got a
     * clue that we're up here and won't throw any Java exceptions, so we should
     * be in good shape and never see this.  Famous last words, I know.  To be
     * safe, we'll keep the global reference(s) in place (leaking temporarily),
     * log the exception and let it propagate on up the stack to the client.
     */
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("Exception"));
        return NULL;
    }

    /*
     * We did the call to leave the session but we have to ask ourselves about
     * the disposition of the C++ session listener that was associated with the
     * possibly deceased session.  Because we can inspect the code we know that
     * the only time the AllJoyn bus attachement actually forgets the listener
     * is when LeaveSession returns status ER_OK.
     *
     * This means that if there was any kind of error, AllJoyn can still call
     * back into our C++ listener.  Therefore we must keep it around.  Since the
     * C++ object is around, the Java object must be kept around to receive
     * translated callbacks from the C++ object.  This is a rather Byzantine
     * error and we are not going to try to harden the Java bindings against an
     * error of this type that is propagated up.
     *
     * It may be surprising to some that a failure to leave a session means they
     * might continue receiving notifications, but it may not be surprising to
     * others.  We'll just leave it at that.
     */
    if (status == ER_OK) {
        QCC_DbgPrintf(("Success"));

        /*
         * We know that AllJoyn has released its hold on C++ listener object
         * referred to by our Java listener object.  We can now release our hold on
         * the Java listener object.
         */
        QCC_DbgPrintf(("Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();
        if (jsessionListener) {
            jobject jglobalref = *jsessionListener;
            *jsessionListener = 0;

            QCC_DbgPrintf(("Releasing Bus Attachment common lock"));
            busPtr->baCommonLock.Unlock();

            QCC_DbgPrintf(("Releasing strong global reference to SessionListener %p", jglobalref));
            env->DeleteGlobalRef(jglobalref);
        } else {
            busPtr->baCommonLock.Unlock();
        }
    } else {
        QCC_LogError(status, ("Error"));
    }

    return JStatus(status);
}

/**
 * Leave (cancel) a session.  This releases the resources allocated for the
 * session, notifies the other side that we have left, and disables callbacks
 * to the associated listener.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionId The SessionId value of the session to end.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_leaveSession(JNIEnv* env, jobject thiz,
                                                                          jint jsessionId)
{
    QCC_DbgPrintf(("BusAttachment_leaveSession()"));
    return leaveGenericSession(env, thiz, jsessionId, BA_SL);
}

/**
 * Leave (cancel) a hosted session.  This releases the resources allocated for the
 * session, notifies the other side that we have left, and disables callbacks
 * to the associated listener.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionId The SessionId value of the session to end.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_leaveHostedSession(JNIEnv* env, jobject thiz,
                                                                                jint jsessionId)
{
    QCC_DbgPrintf(("BusAttachment_leaveHostedSession()"));
    return leaveGenericSession(env, thiz, jsessionId, BA_HSL);
}

/**
 * Leave (cancel) a joined session.  This releases the resources allocated for the
 * session, notifies the other side that we have left, and disables callbacks
 * to the associated listener.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionId The SessionId value of the session to end.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_leaveJoinedSession(JNIEnv* env, jobject thiz,
                                                                                jint jsessionId)
{
    QCC_DbgPrintf(("BusAttachment_leaveJoinedSession()"));
    return leaveGenericSession(env, thiz, jsessionId, BA_JSL);
}

/**
 * Remove a session member from an existing multipoint session.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionId The SessionId value of the session to remove the member from.
 * @param jsessionMemberName The session member to remove.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_removeSessionMember(JNIEnv* env, jobject thiz,
                                                                                 jint jsessionId, jstring jsessionMemberName)
{
    QCC_DbgPrintf(("BusAttachment_removeSessionMember()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_removeSessionMember(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    /*
     * Load the C++ session host string from the java parameter
     */
    JString sessionMemberName(jsessionMemberName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_removeSessionMember(): Exception"));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_removeSessionMember(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_removeSessionMember(): Call RemoveSessionMember(%d, %s)", jsessionId, sessionMemberName.c_str()));

    QStatus status = busPtr->RemoveSessionMember(jsessionId, sessionMemberName.c_str());

    /*
     * If we get an exception down in the AllJoyn code, it's hard to know what
     * to do.  The good part is that the C++ code down in AllJoyn hasn't got a
     * clue that we're up here and won't throw any Java exceptions, so we should
     * be in good shape and never see this.  Famous last words, I know.  To be
     * safe, we'll keep the global reference(s) in place (leaking temporarily),
     * log the exception and let it propagate on up the stack to the client.
     */
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_removeSessionMember(): Exception"));
        return NULL;
    }

    return JStatus(status);
}

static jobject setGenericSessionListener(JNIEnv* env, jobject thiz,
                                         jint jsessionId, jobject jlistener,
                                         BusAttachmentSessionListenerIndex index)
{
    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * We always take a strong global reference to the listener object and hold
     * it as long as we can possibly get callbacks that use it.  If we can't
     * acquire the reference, then we are in an exception state and returning
     * NULL is okay.
     */
    jobject jglobalref = NULL;
    JSessionListener* listener = NULL;
    if (jlistener) {
        QCC_DbgPrintf(("Taking strong global reference to SessionListener %p", jlistener));
        jglobalref = env->NewGlobalRef(jlistener);
        if (!jglobalref) {
            return NULL;
        }

        /*
         * Get the C++ object that must be there backing the Java listener object
         */
        listener = GetNativeListener<JSessionListener*>(env, jlistener);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("Exception"));
            jthrowable exception = env->ExceptionOccurred();
            env->ExceptionClear();
            env->DeleteGlobalRef(jglobalref);
            env->Throw(exception);
            return NULL;
        }

        assert(listener);
    }

    /*
     * Make the AllJoyn call.
     */

    QStatus status = ER_OK;
    jobject* jsessionListener = NULL;
    switch (index) {
    case BA_HSL:
        QCC_DbgPrintf(("Call SetHostedSessionListener(%d, %p)", jsessionId, listener));
        status = busPtr->SetHostedSessionListener(jsessionId, listener);
        jsessionListener = &busPtr->sessionListenerMap[jsessionId].jhostedListener;
        break;

    case BA_JSL:
        QCC_DbgPrintf(("Call SetJoinedSessionListener(%d, %p)", jsessionId, listener));
        status = busPtr->SetJoinedSessionListener(jsessionId, listener);
        jsessionListener = &busPtr->sessionListenerMap[jsessionId].jjoinedListener;
        break;

    case BA_SL:
        QCC_DbgPrintf(("Call SetSessionListener(%d, %p)", jsessionId, listener));
        status = busPtr->SetSessionListener(jsessionId, listener);
        jsessionListener = &busPtr->sessionListenerMap[jsessionId].jListener;
        break;

    default:
        QCC_LogError(ER_FAIL, ("Exception unknown BusAttachmentSessionListenerIndex %d", index));
        assert(0);
    }

    /*
     * We did the call to set the session listner, but we have to ask ourselves
     * two questions: did the new session listener actually get accepted, and
     * what happened to a C++ session listener that may or may not have
     * previously existed.  By inspecting the code, we know that if no error
     * is returned, the session listener has been set.  We don't have any way
     * of directly inferring that there was a previous listener that will no
     * longer be used.  That is left to us.
     *
     * This means that if there was any kind of error, AllJoyn can still call
     * back into a possibly pre-existing C++ listener.  Therefore we must keep
     * it around.  But since the new listener was not accepted, we don't have to
     * keep it around.  This is a rather Byzantine error and we are not going to
     * try to harden the Java bindings against an error of this type that is
     * propagated up.
     *
     * It may be surprising to some that a failure to set a session listener
     * means they might continue receiving notifications on a previously set
     * session listener, but it may not be surprising to others.  We'll just
     * leave it at that.
     */
    if (status == ER_OK) {
        QCC_DbgPrintf(("Success"));

        /*
         * We know that AllJoyn has released its hold on any pre-existing C++
         * listener object referred to by a pre-existing Java listener object.
         * We can now release our hold on that Java listener object.
         */
        QCC_DbgPrintf(("Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();
        if (jsessionListener) {
            jobject joldglobalref = *jsessionListener;
            *jsessionListener = 0;

            QCC_DbgPrintf(("Releasing strong global reference to SessionListener %p", joldglobalref));
            env->DeleteGlobalRef(joldglobalref);
            /*
             * We also know that AllJoyn has a hold on the C++ listener object that
             * we just used.  We have got to keep a hold on the corresponding Java
             * object.
             */
            if (jglobalref) {
                *jsessionListener = jglobalref;
            }
        }

        QCC_DbgPrintf(("Releasing Bus Attachment common lock"));
        busPtr->baCommonLock.Unlock();
    } else {
        QCC_LogError(status, ("Error"));

        /*
         * We know that the C++ listener corresponding to the Java listener we
         * got passed into this method was not accepted by AllJoyn.  This means
         * that we don't need to keep a hold of the reference we took above.  It
         * does mean that if an existing object is there, it may still be used
         * to receive callbacks.  That is, if there is an existing listener on a
         * session, and a subsequent setSessionListener fails, the existing
         * listener remains.
         */
        if (jglobalref) {
            QCC_DbgPrintf(("Releasing strong global reference to SessionListener %p", jglobalref));
            env->DeleteGlobalRef(jglobalref);
        }
    }

    return JStatus(status);
}

/**
 * Explicitly set a session listener for a given session ID.
 *
 * Clients provide session listeners when they join sessions since it makes
 * sense to associate the provided listener with the expected session ID.
 * Services, on the other hand, do not join sessions, they are notified when
 * clients join the sessions they are exporting.  So there is no easy way to
 * make the session ID to session joiner association.  Because of this, it is
 * expected that a service will make that association explicitly in its
 * session joined callback by calling this method.
 *
 * Although this is intended to be used by services, there is no rule that
 * states that this method may only be used in that context.  As such, any
 * call to this method will overwrite an existing listener, disconnecting it
 * from its callbacks.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionId Set to the resulting SessionID value if the call succeeds.
 * @param jlistener Called by the bus when session related events occur.
 *                  May be NULL to clear previous listener.
 */

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_setSessionListener(JNIEnv* env, jobject thiz,
                                                                                jint jsessionId, jobject jlistener)
{
    QCC_DbgPrintf(("BusAttachment_setSessionListener()"));
    return setGenericSessionListener(env, thiz, jsessionId, jlistener, BA_SL);
}

/**
 * Explicitly set a joined session listener for a given session ID.
 *
 * Clients provide session listeners when they join sessions since it makes
 * sense to associate the provided listener with the expected session ID.
 * Services, on the other hand, do not join sessions, they are notified when
 * clients join the sessions they are exporting.  So there is no easy way to
 * make the session ID to session joiner association.  Because of this, it is
 * expected that a service will make that association explicitly in its
 * session joined callback by calling this method.
 *
 * Although this is intended to be used by services, there is no rule that
 * states that this method may only be used in that context.  As such, any
 * call to this method will overwrite an existing listener, disconnecting it
 * from its callbacks.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionId Set to the resulting SessionID value if the call succeeds.
 * @param jlistener Called by the bus when session related events occur.
 *                  May be NULL to clear previous listener.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_setJoinedSessionListener(JNIEnv* env, jobject thiz,
                                                                                      jint jsessionId, jobject jlistener)
{
    QCC_DbgPrintf(("BusAttachment_setJoinedSessionListener()"));
    return setGenericSessionListener(env, thiz, jsessionId, jlistener, BA_JSL);
}

/**
 * Explicitly set a hosted session listener for a given session ID.
 *
 * Clients provide session listeners when they join sessions since it makes
 * sense to associate the provided listener with the expected session ID.
 * Services, on the other hand, do not join sessions, they are notified when
 * clients join the sessions they are exporting.  So there is no easy way to
 * make the session ID to session joiner association.  Because of this, it is
 * expected that a service will make that association explicitly in its
 * session joined callback by calling this method.
 *
 * Although this is intended to be used by services, there is no rule that
 * states that this method may only be used in that context.  As such, any
 * call to this method will overwrite an existing listener, disconnecting it
 * from its callbacks.
 *
 * @param env  The environment pointer used to get access to the JNI helper
 *             functions.
 * @param thiz The Java object reference back to the BusAttachment.  Like a
 *             "this" pointer in C++.
 * @param jsessionId Set to the resulting SessionID value if the call succeeds.
 * @param jlistener Called by the bus when session related events occur.
 *                  May be NULL to clear previous listener.
 */
JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_setHostedSessionListener(JNIEnv* env, jobject thiz,
                                                                                      jint jsessionId, jobject jlistener)
{
    QCC_DbgPrintf(("BusAttachment_setHostedSessionListener()"));
    return setGenericSessionListener(env, thiz, jsessionId, jlistener, BA_HSL);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_SessionListener_create(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("SessionListener_create()"));

    assert(GetHandle<JSessionListener*>(thiz) == NULL);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("SessionListener_create(): Exception"));
        return;
    }

    JSessionListener* jsl = new JSessionListener(thiz);
    if (jsl == NULL) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    SetHandle(thiz, jsl);
    if (env->ExceptionCheck()) {
        delete jsl;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_SessionListener_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("SessionListener_destroy()"));

    JSessionListener* jsl = GetHandle<JSessionListener*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("SessionListener_destroy(): Exception"));
        return;
    }

    assert(jsl);
    delete jsl;

    SetHandle(thiz, NULL);
    return;
}

JOnJoinSessionListener::JOnJoinSessionListener(jobject jonJoinSessionListener)
    : busPtr(NULL)
{
    QCC_DbgPrintf(("JOnJoinSessionListener::JOnJoinSessionListener()"));

    JNIEnv* env = GetEnv();
    JLocalRef<jclass> clazz = env->GetObjectClass(jonJoinSessionListener);

    MID_onJoinSession = env->GetMethodID(clazz, "onJoinSession", "(Lorg/alljoyn/bus/Status;ILorg/alljoyn/bus/SessionOpts;Ljava/lang/Object;)V");
    if (!MID_onJoinSession) {
        QCC_DbgPrintf(("JOnJoinSessionListener::JOnJoinSessionListener(): Can't find onJoinSession() in OnJoinSessionListener"));
    }
}

JOnJoinSessionListener::~JOnJoinSessionListener()
{
    QCC_DbgPrintf(("JOnJoinSessionListener::~JOnJoinSessionListener()"));

    /*
     * In our Setup method we are passed a pointer to the reference counted bus
     * attachment.  We don't want to delete the object directly so we need to
     * DecRef() it.  Once we do this the underlying object can be deleted at any
     * time, so we need to forget about this pointer immediately.
     */
    if (busPtr) {
        QCC_DbgPrintf(("JOnJoinSessionListener::~JOnJoinSessionListener(): Refcount on busPtr before decrement is %d", busPtr->GetRef()));
        busPtr->DecRef();
        busPtr = NULL;
    }
}

void JOnJoinSessionListener::Setup(JBusAttachment* jbap)
{
    QCC_DbgPrintf(("JOnJoinSessionListener::Setup(0x%p)", jbap));

    /*
     * We need to be able to get back at the bus attachment in the callback to
     * release and/or reassign resources.  We are going to keep a pointer to the
     * reference counted bus attachment, so we need to IncRef() it.
     */
    busPtr = jbap;
    QCC_DbgPrintf(("JOnJoinSessionListener::Setup(): Refcount on busPtr before is %d", busPtr->GetRef()));
    busPtr->IncRef();
    QCC_DbgPrintf(("JOnJoinSessionListener::Setup(): Refcount on busPtr after %d", busPtr->GetRef()));
}

void JOnJoinSessionListener::JoinSessionCB(QStatus status, SessionId sessionId, const SessionOpts& opts, void* context)
{
    QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(%s, %d,  <0x%02x, %d, 0x%02x, 0x%04x>, %p)",
                   QCC_StatusText(status), sessionId, opts.traffic, opts.isMultipoint,
                   opts.proximity, opts.transports, context));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    JLocalRef<jobject> jstatus;
    jint jsessionId;
    jmethodID mid;
    JLocalRef<jobject> jopts;
    jfieldID fid;
    jobject jo;

    /*
     * The context parameter we get here is not the same thing as the context
     * parameter we gave to Java in joinSessionAsync.  Here it is a pointer to a
     * PendingAsyncJoin object which holds the references to the three Java
     * objects involved in the transaction.  This must be there.
     */
    PendingAsyncJoin* paj = static_cast<PendingAsyncJoin*>(context);
    assert(paj);

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    jstatus = JStatus(status);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JOnJoinSessionListener::JoinSessionCB(): Exception"));
        goto exit;
    }

    jsessionId = sessionId;

    mid = env->GetMethodID(CLS_SessionOpts, "<init>", "()V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("JOnJoinSessionListener::JoinSessionCB(): Can't find SessionOpts constructor"));
        goto exit;
    }

    QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Create new SessionOpts"));
    jopts = env->NewObject(CLS_SessionOpts, mid);
    if (!jopts) {
        QCC_LogError(ER_FAIL, ("JOnJoinSessionListener::JoinSessionCB(): Cannot create SessionOpts"));
        goto exit;
    }

    QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Load SessionOpts"));
    fid = env->GetFieldID(CLS_SessionOpts, "traffic", "B");
    env->SetByteField(jopts, fid, opts.traffic);

    fid = env->GetFieldID(CLS_SessionOpts, "isMultipoint", "Z");
    env->SetBooleanField(jopts, fid, opts.isMultipoint);

    fid = env->GetFieldID(CLS_SessionOpts, "proximity", "B");
    env->SetByteField(jopts, fid, opts.proximity);

    fid = env->GetFieldID(CLS_SessionOpts, "transports", "S");
    env->SetShortField(jopts, fid, opts.transports);

    /*
     * The references provided in the PendingAsyncJoin are strong global references
     * so they can be used as-is (we need the on join session listener and the context).
     */
    jo = paj->jonJoinSessionListener;

    QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Call out to listener object and method"));
    env->CallVoidMethod(jo, MID_onJoinSession, (jobject)jstatus, jsessionId, (jobject)jopts, (jobject)paj->jcontext);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JOnJoinSessionListener::JoinSessionCB(): Exception"));
        goto exit;
    }

exit:
    QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Release Resources"));

    QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Taking Bus Attachment common lock"));
    busPtr->baCommonLock.Lock();

    /*
     * We stored an object containing instances of the Java objects provided in the
     * original call to the async join that drove this process in case the call
     * got lost in a disconnect -- we don't want to leak them.  So we need to find
     * the matching object and delete it.
     */
    for (list<PendingAsyncJoin*>::iterator i = busPtr->pendingAsyncJoins.begin(); i != busPtr->pendingAsyncJoins.end(); ++i) {
        /*
         * If the pointer to the PendingAsyncJoin in the bus attachment is equal
         * to the one passed in from the C++ async join callback, then we are
         * talkiing about the same async join instance.
         */
        if (*i == context) {
            /*
             * Double check that the pointers are consistent and nothing got
             * changed out from underneath us.  That would be bad (TM).
             */
            assert((*i)->jonJoinSessionListener == paj->jonJoinSessionListener);
            assert((*i)->jsessionListener == paj->jsessionListener);
            assert((*i)->jcontext == paj->jcontext);

            /*
             * If the join succeeded, we need to keep on holding the session
             * listener in case something happens to the now "up" session.  This
             * reference must go in the sessionListenerMap and we are delegating
             * responsibility for cleaning up to that map.  If the async call
             * failed, we are done with the session listener as well and we need
             * to release our hold on it since no callback will be made on a
             * failed session.
             */
            if (status == ER_OK) {
                busPtr->sessionListenerMap[sessionId].jListener = (*i)->jsessionListener;
                (*i)->jsessionListener = NULL;
            } else {
                QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Release strong global reference to SessionListener %p", (*i)->jsessionListener));
                env->DeleteGlobalRef((*i)->jsessionListener);
            }

            /*
             * We always release our hold on the user context object
             * irrespective of the outcome of the call since it will no longer
             * be used by this asynchronous join instance.
             */
            if ((*i)->jcontext) {
                QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Release strong global reference to context Object %p", (*i)->jcontext));
                env->DeleteGlobalRef((*i)->jcontext);
                (*i)->jcontext = NULL;
            }

            /*
             * We always release our hold on the OnJoinSessionListener object
             * and the user context object irrespective of the outcome of the
             * call since it will no longer be used by this asynchronous join
             * instance.
             *
             * Releasing the Java OnJoinSessionListener is effectively a "delete
             * this" since the global reference to the Java object controls the
             * lifetime of its corresponding C++ object, which is what we are
             * executing in here.  We have got to make sure to do that last.
             */
            assert((*i)->jonJoinSessionListener);
            jobject jcallback = (*i)->jonJoinSessionListener;
            (*i)->jonJoinSessionListener = NULL;
            busPtr->pendingAsyncJoins.erase(i);

            QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Release strong global reference to OnJoinSessionListener %p", jcallback));
            env->DeleteGlobalRef(jcallback);

            QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Releasing Bus Attachment common lock"));
            busPtr->baCommonLock.Unlock();
            return;
        }
    }

    QCC_DbgPrintf(("JOnJoinSessionListener::JoinSessionCB(): Releasing Bus Attachment common lock"));
    busPtr->baCommonLock.Unlock();

    QCC_LogError(ER_FAIL, ("JOnJoinSessionListener::JoinSessionCB(): Unable to match context"));
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_joinSessionAsync(JNIEnv* env,
                                                                              jobject thiz,
                                                                              jstring jsessionHost,
                                                                              jshort jsessionPort,
                                                                              jobject jsessionOpts,
                                                                              jobject jsessionListener,
                                                                              jobject jonJoinSessionListener,
                                                                              jobject jcontext)
{
    /*
     * This method is unusual in that there are three objects passed which have
     * a lifetime past the duration of the method: The SessionListener needs to
     * be kept around to hear about the joined session being lost for as long as
     * the session is up; the OnJoinSessionListner needs to be kept around until
     * the asynchronous join completes; and the user-defined context object has
     * the same lifetime (from our perspective) as the OnJoinSessionListener.
     *
     * We handle the two "AllJoyn" objects (the session listener and the on join
     * session listener) the same way we do all other long-lived Java objects.
     * We expect them to create their own corresponding C++ object when their Java
     * constructor is run, and we expect them to delete the C++ object when they
     * are finalized.  Our memory management responsibility, then, is then to add
     * a strong global reference to the objects to keep them alive though the two
     * lifetime scopes mentioned above.  The Context object is just a vanilla
     * Java object (for example, Integer) and so we can assume no C++ backing
     * object.
     *
     * One of the challenges we face is because we have to work with the
     * anonymous class idiom of Java and the underlying C++ functions don't
     * plumb all three objects through all calls.  For example, the C++ callback
     * JoinSessionAsyncCB gets a pointer to the JOnJoinSessionListener in its
     * this pointer, gets a pointer to the Java context in its context parameter
     * but doesn't get a pointer to the session listener.  This is not a problem
     * in C++ since the language doesn't support anonymous classes, but in Java
     * we need to be able to discover that pointer.
     *
     * Since different combinations of the same or different three objects can
     * be used in overlapping calls to JoinSessionAsync, we have to keep track
     * of which instances of which objects need to be freed when a callback is
     * fired.  This may not be intuitively obvious, so consider the following.
     *
     *   The user instantiates a SessionListener SL, an OnJoinSessionListener
     *   OJSL and a context object O; and starts an async join.
     *
     *   The user decides to reuse the OnJoinSessionListener but provide a new
     *   SessionListener SL'; and starts an async join to a session.
     *
     * In this case, the first async join would take strong global references to
     * the three objects and save weak references to them into the C++ backing
     * object of the OnJoinSessionListener.  The second async join would take
     * three more references to the provided three objects, and write them into
     * the backing object of the provided OnJoinSessionListener.  This would
     * overwrite the value of the provided session listener of the first join
     * (SL) with that of the second join (SL') and create a memory leak.
     *
     * What we need is a way to have the C++ code pass us all three instances so
     * we can keep track of them.  The C++ code does plumb through a context
     * value, but the problem is that we want the Java code to plumb through a
     * context value as well.  The answer is to change the meaning of the context
     * value in the C++ code to be a special object that includes the references
     * to the three Java objects we need.
     *
     * It's a bit counter-intuitive, but the C++ context object in this code path
     * does not map one-to-one with the Java context object.  The Java context
     * is stored in a special C++ context -- the two are not at all the same.
     */
    QCC_DbgPrintf(("BusAttachment_joinSessionAsync()"));

    /*
     * Load the C++ session host string from the java parameter
     */
    JString sessionHost(jsessionHost);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSessionAsync(): Exception"));
        return NULL;
    }

    /*
     * Load the C++ session options from the Java session options.
     */
    SessionOpts sessionOpts;
    JLocalRef<jclass> clazz = env->GetObjectClass(jsessionOpts);
    jfieldID fid = env->GetFieldID(clazz, "traffic", "B");
    assert(fid);
    sessionOpts.traffic = static_cast<SessionOpts::TrafficType>(env->GetByteField(jsessionOpts, fid));

    fid = env->GetFieldID(clazz, "isMultipoint", "Z");
    assert(fid);
    sessionOpts.isMultipoint = env->GetBooleanField(jsessionOpts, fid);

    fid = env->GetFieldID(clazz, "proximity", "B");
    assert(fid);
    sessionOpts.proximity = env->GetByteField(jsessionOpts, fid);

    fid = env->GetFieldID(clazz, "transports", "S");
    assert(fid);
    sessionOpts.transports = env->GetShortField(jsessionOpts, fid);

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSessionAsync(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSessionAsync(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Refcount on busPtr is %d", busPtr->GetRef()));

    QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Taking strong global reference to SessionListener %p", jsessionListener));
    jobject jglobalListenerRef = env->NewGlobalRef(jsessionListener);
    if (!jglobalListenerRef) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSessionAsync(): Unable to take strong global reference"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Taking strong global reference to OnJoinSessionListener %p", jonJoinSessionListener));
    jobject jglobalCallbackRef = env->NewGlobalRef(jonJoinSessionListener);
    if (!jglobalCallbackRef) {
        QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Forgetting jglobalListenerRef"));
        env->DeleteGlobalRef(jglobalListenerRef);

        QCC_LogError(ER_FAIL, ("BusAttachment_joinSessionAsync(): Unable to take strong global reference"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    /*
     * The user context is optional.
     */
    jobject jglobalContextRef = NULL;
    if (jcontext) {
        QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Taking strong global reference to context Object %p", jcontext));
        jglobalContextRef = env->NewGlobalRef(jcontext);
        if (!jglobalContextRef) {
            QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Forgetting jglobalListenerRef"));
            env->DeleteGlobalRef(jglobalListenerRef);
            QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Forgetting jglobalCallbackRef"));
            env->DeleteGlobalRef(jglobalCallbackRef);
            return NULL;
        }
    }

    /*
     * Get the C++ object that must be there backing the Java listener object
     */
    JSessionListener* listener = GetNativeListener<JSessionListener*>(env, jsessionListener);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSessionAsync(): Exception"));
        return NULL;
    }

    assert(listener);

    /*
     * Get the C++ object that must be there backing the Java callback object
     */
    JOnJoinSessionListener* callback = GetNativeListener<JOnJoinSessionListener*>(env, jonJoinSessionListener);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSessionAsync(): Exception"));
        return NULL;
    }

    assert(callback);

    /*
     * There is no C++ object backing the Java context object.  This is just an
     * object reference that is plumbed through AllJoyn which will pop out the
     * other side un-molested.  It is not interpreted by AllJoyn so we can just
     * use our Java global reference to the provided Java object.  We pass the
     * reference nack to the user when the callback fires.  N.B. this is not
     * going to be passed into the context parameter of the C++ AsyncJoin method
     * as described above and below.
     *
     * We have three objects now that are closely associated: we have an
     * OnJoinSessionListener that we need to keep a strong reference to until
     * the C++ async join completes; we have a SessionListener that we need to
     * continue to hold a strong reference to past the async join completion if
     * the join is successful, but release if it is not; and we have a user
     * context object we need to hold a strong reference to until the async call
     * is finished.  We tie them all together as weak references in the C++
     * listener object corresponding to the OnJoinSessionListener.
     *
     * We have taken the required references above, but we need to assoicate
     * those references with an instance of a call to async join.  We do this
     * by allocating an object that contains the instance information and by
     * commandeering the C++ async join context to plumb it through.
     */
    PendingAsyncJoin* paj = new PendingAsyncJoin(jglobalListenerRef, jglobalCallbackRef, jglobalContextRef);

    /*
     * We need to provide a pointer to the bus attachment to the on join session
     * listener.  This will bump the underlying reference count.
     */
    callback->Setup(busPtr);

    /*
     * Make the actual call into the C++ JoinSessionAsync method.  Not to beat
     * a dead horse, but note that the context parameter is not the same as the
     * Java context parameter passed into this method.
     */
    QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Call JoinSessionAsync(%s, %d, %p, <0x%02x, %d, 0x%02x, 0x%04x>, %p, %p)",
                   sessionHost.c_str(), jsessionPort, listener, sessionOpts.traffic, sessionOpts.isMultipoint,
                   sessionOpts.proximity, sessionOpts.transports, callback, paj));
    QStatus status = busPtr->JoinSessionAsync(sessionHost.c_str(), jsessionPort, listener, sessionOpts, callback, paj);

    /*
     * If we get an exception down in the AllJoyn code, it's hard to know what
     * to do.  The good part is that the C++ code down in AllJoyn hasn't got a
     * clue that we're up here and won't throw any Java exceptions, so we should
     * be in good shape and never see this.  Famous last words, I know.  To be
     * safe, we'll keep the global reference(s) in place (leaking temporarily),
     * log the exception and let it propagate on up the stack to the client.
     */
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_joinSessionAsync(): Exception"));
        return NULL;
    }

    /*
     * This is an async join method, so getting a successful completion only
     * means that AllJoyn was able to send off a message requesting the join.
     * This means we have a special case code to "pend" the Java objects we are
     * holding until we get a status from AllJoyn.  We will release the callback
     * and the context unconditionally when the callback fires, but what we do
     * with the session listener will depend on the completion status.
     *
     * If we get an error from the AllJoyn code now it means that the send of
     * the join session message to the daemon failed, and nothing has worked.
     * We know from code inspection that neither the C++ listener object, the
     * C++ callback nor the C++ context will be remembered by AllJoyn if an
     * error happens now.  Since the C++ objects will not be used, the Java
     * objects will never be used and our saved global references are not
     * required -- we can just forget about them.
     *
     * Pick up the async join code path in JOnJoinSessionListener::JoinSessionCB
     */
    if (status == ER_OK) {
        QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Success"));

        QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();

        busPtr->pendingAsyncJoins.push_back(paj);

        QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Releasing Bus Attachment common lock"));
        busPtr->baCommonLock.Unlock();
    } else {
        QCC_LogError(status, ("BusAttachment_joinSessionAsync(): Error"));

        QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Releasing strong global reference to SessionListener %p", jglobalListenerRef));
        env->DeleteGlobalRef(jglobalListenerRef);

        QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Releasing strong global reference to OnJoinSessionListener %p", jglobalCallbackRef));
        env->DeleteGlobalRef(jglobalCallbackRef);

        if (jglobalContextRef) {
            QCC_DbgPrintf(("BusAttachment_joinSessionAsync(): Releasing strong global reference to context Object %p", jcontext));
            env->DeleteGlobalRef(jglobalContextRef);
        }
    }
    return JStatus(status);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_OnJoinSessionListener_create(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("OnJoinSessionListener_create()"));

    assert(GetHandle<JOnJoinSessionListener*>(thiz) == NULL);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("OnJoinSessionListener_create(): Exception"));
        return;
    }

    QCC_DbgPrintf(("OnJoinSessionListener_create(): Create backing object"));
    JOnJoinSessionListener* jojsl = new JOnJoinSessionListener(thiz);
    if (jojsl == NULL) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    QCC_DbgPrintf(("OnJoinSessionListener_create(): Set handle to %p", jojsl));
    SetHandle(thiz, jojsl);
    if (env->ExceptionCheck()) {
        delete jojsl;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_OnJoinSessionListener_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("OnJoinSessionListener_destroy()"));

    JOnJoinSessionListener* jojsl = GetHandle<JOnJoinSessionListener*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("OnJoinSessionListener_destroy(): Exception"));
        return;
    }

    assert(jojsl);
    delete jojsl;

    SetHandle(thiz, NULL);
    return;
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_getSessionFd(JNIEnv* env, jobject thiz,
                                                                          jint jsessionId,
                                                                          jobject jsockfd)
{
    QCC_DbgPrintf(("BusAttachment_getSessionFd()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getSessionFd(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getSessionFd(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_getSessionFd(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    qcc::SocketFd sockfd = -1;

    QCC_DbgPrintf(("BusAttachment_getSessionFd(): Call GetSessionFd(%d, %d)", jsessionId, sockfd));

    QStatus status = busPtr->GetSessionFd(jsessionId, sockfd);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getSessionFd(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_getSessionFd(): GetSessionFd() fails"));
    }

    /*
     * Store the sockFd in its corresponding out parameter.
     */
    JLocalRef<jclass> clazz = env->GetObjectClass(jsockfd);
    jfieldID fid = env->GetFieldID(clazz, "value", "I");
    assert(fid);
    env->SetIntField(jsockfd, fid, sockfd);

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_setLinkTimeout(JNIEnv* env, jobject thiz,
                                                                            jint jsessionId,
                                                                            jobject jLinkTimeout)
{
    QCC_DbgPrintf(("BusAttachment_setLinkTimeout()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setLinkTimeout(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setLinkTimeout(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_setLinkTimeout(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    JLocalRef<jclass> clazz = env->GetObjectClass(jLinkTimeout);
    jfieldID fid = env->GetFieldID(clazz, "value", "I");
    assert(fid);
    uint32_t linkTimeout = env->GetIntField(jLinkTimeout, fid);
    QCC_DbgPrintf(("BusAttachment_setLinkTimeout(): Call SetLinkTimeout(%d, %d)", jsessionId, linkTimeout));

    QStatus status = busPtr->SetLinkTimeout(jsessionId, linkTimeout);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setLinkTimeout(): Exception"));
        return NULL;
    }

    /*
     * Store the linkTimeout in its corresponding out parameter.
     */
    if (status == ER_OK) {
        env->SetIntField(jLinkTimeout, fid, linkTimeout);
    } else {
        QCC_LogError(status, ("BusAttachment_setLinkTimeout(): SetLinkTimeout() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_getPeerGUID(JNIEnv* env,
                                                                         jobject thiz,
                                                                         jstring jname,
                                                                         jobject jguid)
{
    QCC_DbgPrintf(("BusAttachment::getPeerGUID()"));

    /*
     * Load the C++ name string from the java parameter.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getPeerGUID(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getPeerGUID(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getPeerGUID(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_getPeerGUID(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    qcc::String guidstr;
    QCC_DbgPrintf(("BusAttachment_getPeerGUID(): Call GetPeerGUID(%s, %s)", name.c_str(), guidstr.c_str()));

    QStatus status = busPtr->GetPeerGUID(name.c_str(), guidstr);

    QCC_DbgPrintf(("BusAttachment_getPeerGUID(): Back from GetPeerGUID(%s, %s)", name.c_str(), guidstr.c_str()));

    /*
     * Locate the C++ GUID string.  Note that the reference to the string is
     * passed in as an [out] parameter using a mutable object, so we are really
     * finding the field which we will write our found GUID string reference into.
     */
    JLocalRef<jclass> clazz = env->GetObjectClass(jguid);
    jfieldID guidValueFid = env->GetFieldID(clazz, "value", "Ljava/lang/String;");
    assert(guidValueFid);

    /*
     * We provided an empty C++ string to AllJoyn, and it has put the GUID in
     * that string if it succeeded.  We need to create a Java string with the
     * returned bytes and put it into the StringValue object's "value" field
     * which we just located.
     */
    jstring jstr = env->NewStringUTF(guidstr.c_str());
    env->SetObjectField(jguid, guidValueFid, jstr);

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_getPeerGUID(): GetPeerGUID() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_ping(JNIEnv* env,
                                                                  jobject thiz,
                                                                  jstring jname,
                                                                  jint jtimeout)
{
    QCC_DbgPrintf(("BusAttachment_ping()"));

    /*
     * Load the C++ well-known name Java well-known name.
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_ping(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_ping(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_ping(): Call Ping(%s)", name.c_str()));

    QStatus status = busPtr->Ping(name.c_str(), jtimeout);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_ping(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_ping(): Ping() fails"));
    }

    return JStatus(status);
}

JOnPingListener::JOnPingListener(jobject jonPingListener)
    : busPtr(NULL)
{
    QCC_DbgPrintf(("JOnPingListener::JOnPingListener()"));

    JNIEnv* env = GetEnv();
    JLocalRef<jclass> clazz = env->GetObjectClass(jonPingListener);

    MID_onPing = env->GetMethodID(clazz, "onPing", "(Lorg/alljoyn/bus/Status;Ljava/lang/Object;)V");
    if (!MID_onPing) {
        QCC_DbgPrintf(("JOnPingListener::JOnPingListener(): Can't find onPing() in OnPingListener"));
    }
}

JOnPingListener::~JOnPingListener()
{
    QCC_DbgPrintf(("JOnPingListener::~JOnPingListener()"));

    /*
     * In our Setup method we are passed a pointer to the reference counted bus
     * attachment.  We don't want to delete the object directly so we need to
     * DecRef() it.  Once we do this the underlying object can be deleted at any
     * time, so we need to forget about this pointer immediately.
     */
    if (busPtr) {
        QCC_DbgPrintf(("JOnPingListener::~JOnPingListener(): Refcount on busPtr before decrement is %d", busPtr->GetRef()));
        busPtr->DecRef();
        busPtr = NULL;
    }
}

void JOnPingListener::Setup(JBusAttachment* jbap)
{
    QCC_DbgPrintf(("JOnPingListener::Setup(0x%p)", jbap));

    /*
     * We need to be able to get back at the bus attachment in the callback to
     * release and/or reassign resources.  We are going to keep a pointer to the
     * reference counted bus attachment, so we need to IncRef() it.
     */
    busPtr = jbap;
    QCC_DbgPrintf(("JOnPingListener::Setup(): Refcount on busPtr before is %d", busPtr->GetRef()));
    busPtr->IncRef();
    QCC_DbgPrintf(("JOnPingListener::Setup(): Refcount on busPtr after %d", busPtr->GetRef()));
}

void JOnPingListener::PingCB(QStatus status, void* context)
{
    QCC_DbgPrintf(("JOnPingListener::PingCB(%s, %p)", QCC_StatusText(status), context));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;
    // the ping object corresponding to the java OnPingListener class
    jobject po;
    JLocalRef<jobject> jstatus;


    /*
     * The context parameter we get here is not the same thing as the context
     * parameter we gave to Java in joinPingAsync.  Here it is a pointer to a
     * PendingAsyncPing object which holds the references to the two Java
     * objects involved in the transaction.
     */
    PendingAsyncPing* pap = static_cast<PendingAsyncPing*>(context);
    assert(pap);

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    jstatus = JStatus(status);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JOnPingListener::PingCB(): Exception"));
        goto exit;
    }

    /*
     * The references provided in the PendingAsyncPing are strong global references
     * so they can be used as-is (we need the on ping listener and the context).
     */
    po = pap->jonPingListener;

    QCC_DbgPrintf(("JOnPingListener::PingCB(): Call out to listener object and method"));
    env->CallVoidMethod(po, MID_onPing, (jobject)jstatus, (jobject)pap->jcontext);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JOnPingListener::PingCB(): Exception"));
        goto exit;
    }

exit:
    QCC_DbgPrintf(("JOnPingListener::PingCB(): Release Resources"));

    QCC_DbgPrintf(("JOnPingListener::PingCB(): Taking Bus Attachment common lock"));
    busPtr->baCommonLock.Lock();

    /*
     * We stored an object containing instances of the Java objects provided in the
     * original call to the async ping that drove this process in case the call
     * got lost in a disconnect -- we don't want to leak them.  So we need to find
     * the matching object and delete it.
     */
    for (list<PendingAsyncPing*>::iterator i = busPtr->pendingAsyncPings.begin(); i != busPtr->pendingAsyncPings.end(); ++i) {
        /*
         * If the pointer to the PendingAsyncPing in the bus attachment is equal
         * to the one passed in from the C++ async ping callback, then we are
         * talkiing about the same async ping instance.
         */
        if (*i == context) {
            /*
             * Double check that the pointers are consistent and nothing got
             * changed out from underneath us.  That would be bad (TM).
             */
            assert((*i)->jonPingListener == pap->jonPingListener);
            assert((*i)->jcontext == pap->jcontext);

            /*
             * We always release our hold on the user context object
             * irrespective of the outcome of the call since it will no longer
             * be used by this asynchronous ping instance.
             */
            if ((*i)->jcontext) {
                QCC_DbgPrintf(("JOnPingListener::PingCB(): Release strong global reference to context Object %p", (*i)->jcontext));
                env->DeleteGlobalRef((*i)->jcontext);
                (*i)->jcontext = NULL;
            }

            /*
             * We always release our hold on the OnPingListener object
             * and the user context object irrespective of the outcome of the
             * call since it will no longer be used by this asynchronous ping
             * instance.
             *
             * Releasing the Java OnPingListener is effectively a "delete
             * this" since the global reference to the Java object controls the
             * lifetime of its corresponding C++ object, which is what we are
             * executing in here.  We have got to make sure to do that last.
             */
            assert((*i)->jonPingListener);
            jobject jcallback = (*i)->jonPingListener;
            (*i)->jonPingListener = NULL;
            busPtr->pendingAsyncPings.erase(i);

            QCC_DbgPrintf(("JOnPingListener::PingCB(): Release strong global reference to OnPingListener %p", jcallback));
            env->DeleteGlobalRef(jcallback);

            QCC_DbgPrintf(("JOnPingListener::PingCB(): Releasing Bus Attachment common lock"));
            busPtr->baCommonLock.Unlock();
            return;
        }
    }

    QCC_DbgPrintf(("JOnPingListener::PingCB(): Releasing Bus Attachment common lock"));
    busPtr->baCommonLock.Unlock();

    QCC_LogError(ER_FAIL, ("JOnPingListener::PingCB(): Unable to match context"));
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_pingAsync(JNIEnv* env,
                                                                       jobject thiz,
                                                                       jstring jname,
                                                                       jint jtimeout,
                                                                       jobject jonPingListener,
                                                                       jobject jcontext)
{
    /*
     * This method is unusual in that there are two objects passed which have
     * a lifetime past the duration of the method: the OnPingListner needs to be
     * kept around until the asynchronous ping completes; and the user-defined
     * context object has the same lifetime (from our perspective) as the
     * OnPingListener.
     *
     * We handle the "AllJoyn" object (the on ping listener) the same way we do
     * all other long-lived Java objects. We expect them to create their own
     * corresponding C++ object when their Java constructor is run, and we
     * expect them to delete the C++ object when they are finalized.  Our memory
     * management responsibility, then, is then to add a strong global reference
     * to the objects to keep them alive though the lifetime scopes mentioned above.
     * The Context object is just a vanilla Java object (for example, Integer)
     * and so we can assume no C++ backing object.
     *
     * One of the challenges we face is because we have to work with the
     * anonymous class idiom of Java and the underlying C++ functions don't
     * plumb all the objects through all calls.  For example, the C++ callback
     * PingAsyncCB gets a pointer to the JOnPingListener this pointer, gets a
     * pointer to the Java context in its context parameter but doesn't get a
     * pointer to the ping listener.  This is not a problem in C++ since the
     * language doesn't support anonymous classes, but in Java we need to be
     * able to discover that pointer.
     *
     * Since different combinations of the same or different three objects can
     * be used in overlapping calls to PingAsync, we have to keep track of which
     * instances of which objects need to be freed when a callback is fired.
     * This may not be intuitively obvious, so consider the following.
     *
     *   The user instantiates an OnPingListener OPL and a context object O; and
     *   starts an async ping.
     *
     *   The user decides to reuse the OnPingListener and starts an async ping.
     *
     * In this case, the first async ping would take strong global references to
     * the two objects and save weak references to them into the C++ backing
     * object of the OnPingListener.  The second async ping would take two more
     * references to the provided  objects, and write them into the backing
     * object of the provided OnPingListener.
     *
     * What we need is a way to have the C++ code pass us all three instances so
     * we can keep track of them.  The C++ code does plumb through a context
     * value, but the problem is that we want the Java code to plumb through a
     * context value as well.  The answer is to change the meaning of the context
     * value in the C++ code to be a special object that includes the references
     * to the two Java objects we need.
     *
     * It's a bit counter-intuitive, but the C++ context object in this code path
     * does not map one-to-one with the Java context object.  The Java context
     * is stored in a special C++ context -- the two are not at all the same.
     */
    QCC_DbgPrintf(("BusAttachment_pingAsync()"));

    /*
     * Load the C++ session host string from the java parameter
     */
    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_pingAsync(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_pingAsync(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_pingAsync(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_pingAsync(): Refcount on busPtr is %d", busPtr->GetRef()));

    QCC_DbgPrintf(("BusAttachment_pingAsync(): Taking strong global reference to OnPingListener %p", jonPingListener));
    jobject jglobalCallbackRef = env->NewGlobalRef(jonPingListener);
    if (!jglobalCallbackRef) {
        QCC_LogError(ER_FAIL, ("BusAttachment_pingAsync(): Unable to take strong global reference"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    /*
     * The user context is optional.
     */
    jobject jglobalContextRef = NULL;
    if (jcontext) {
        QCC_DbgPrintf(("BusAttachment_pingAsync(): Taking strong global reference to context Object %p", jcontext));
        jglobalContextRef = env->NewGlobalRef(jcontext);
        if (!jglobalContextRef) {
            QCC_DbgPrintf(("BusAttachment_pingAsync(): Forgetting jglobalCallbackRef"));
            env->DeleteGlobalRef(jglobalCallbackRef);
            return NULL;
        }
    }

    /*
     * Get the C++ object that must be there backing the Java callback object
     */
    JOnPingListener* callback = GetNativeListener<JOnPingListener*>(env, jonPingListener);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_pingAsync(): Exception"));
        return NULL;
    }

    assert(callback);

    /*
     * We need to provide a pointer to the bus attachment to the on ping
     * listener.  This will bump the underlying reference count.
     */
    callback->Setup(busPtr);

    /*
     * There is no C++ object backing the Java context object.  This is just an
     * object reference that is plumbed through AllJoyn which will pop out the
     * other side un-molested.  It is not interpreted by AllJoyn so we can just
     * use our Java global reference to the provided Java object.  We pass the
     * reference back to the user when the callback fires.  N.B. this is not
     * going to be passed into the context parameter of the C++ PingAsync method
     * as described above and below.
     *
     * We have two objects now that are closely associated: we have an
     * OnPingListener that we need to keep a strong reference to until
     * the C++ async ping completes; and we have a user context object we need
     * to hold a strong reference to until the async call is finished.  We tie
     * them together as weak references in the C++ listener object corresponding
     * to the OnPingListener.
     *
     * We have taken the required references above, but we need to assoicate
     * those references with an instance of a call to async ping.  We do this
     * by allocating an object that contains the instance information and by
     * commandeering the C++ async ping context to plumb it through.
     */
    PendingAsyncPing* pap = new PendingAsyncPing(jglobalCallbackRef, jglobalContextRef);

    /*
     * Make the actual call into the C++ PingAsync method.  Not to beat
     * a dead horse, but note that the context parameter is not the same as the
     * Java context parameter passed into this method.
     */
    QCC_DbgPrintf(("BusAttachment_pingAsync(): Call PingAsync(%s, %d, %p, %p)",
                   name.c_str(), jtimeout, callback, pap));
    QStatus status = busPtr->PingAsync(name.c_str(), jtimeout, callback, pap);

    /*
     * If we get an exception down in the AllJoyn code, it's hard to know what
     * to do.  The good part is that the C++ code down in AllJoyn hasn't got a
     * clue that we're up here and won't throw any Java exceptions, so we should
     * be in good shape and never see this.  Famous last words, I know.  To be
     * safe, we'll keep the global reference(s) in place (leaking temporarily),
     * log the exception and let it propagate on up the stack to the client.
     */
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_pingAsync(): Exception"));
        return NULL;
    }

    /*
     * This is an async ping method, so getting a successful completion only
     * means that AllJoyn was able to send off a message requesting the ping.
     * This means we have a special case code to "pend" the Java objects we are
     * holding until we get a status from AllJoyn.  We will release the callback
     * and the context unconditionally when the callback fires, but what we do
     * with the session listener will depend on the completion status.
     *
     * If we get an error from the AllJoyn code now it means that the send of
     * the ping message to the daemon failed, and nothing has worked.
     * We know from code inspection that neither the C++ listener object, nor
     * the C++ context will be remembered by AllJoyn if an error happens now.
     * Since the C++ objects will not be used, the Java objects will never be
     * used and our saved global references are not required -- we can just
     * forget about them.
     *
     * Pick up the async ping code path in JOnPingListener::PingCB
     */
    if (status == ER_OK) {
        QCC_DbgPrintf(("BusAttachment_pingAsync(): Success"));

        QCC_DbgPrintf(("BusAttachment_pingAsync(): Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();

        busPtr->pendingAsyncPings.push_back(pap);

        QCC_DbgPrintf(("BusAttachment_pingAsync(): Releasing Bus Attachment common lock"));
        busPtr->baCommonLock.Unlock();
    } else {
        QCC_LogError(status, ("BusAttachment_pingAsync(): Error"));

        QCC_DbgPrintf(("BusAttachment_pingAsync(): Releasing strong global reference to OnPingListener %p", jglobalCallbackRef));
        env->DeleteGlobalRef(jglobalCallbackRef);

        if (jglobalContextRef) {
            QCC_DbgPrintf(("BusAttachment_pingAsync(): Releasing strong global reference to context Object %p", jcontext));
            env->DeleteGlobalRef(jglobalContextRef);
        }
    }
    return JStatus(status);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_OnPingListener_create(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("OnPingListener_create()"));

    assert(GetHandle<JOnPingListener*>(thiz) == NULL);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("OnPingListener_create(): Exception"));
        return;
    }

    QCC_DbgPrintf(("OnPingListener_create(): Create backing object"));
    JOnPingListener* jopl = new JOnPingListener(thiz);
    if (jopl == NULL) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    QCC_DbgPrintf(("OnPingListener_create(): Set handle to %p", jopl));
    SetHandle(thiz, jopl);
    if (env->ExceptionCheck()) {
        QCC_DbgPrintf(("OnPingListener_create(): Set handle Exception"));
        delete jopl;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_OnPingListener_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("OnPingListener_destroy()"));

    JOnPingListener* jopl = GetHandle<JOnPingListener*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("OnPingListener_destroy(): Exception"));
        return;
    }

    assert(jopl);
    delete jopl;

    SetHandle(thiz, NULL);
    return;
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_setDaemonDebug(JNIEnv*env,
                                                                            jobject thiz,
                                                                            jstring jmodule,
                                                                            jint jlevel)
{
    QCC_DbgPrintf(("BusAttachment_setDaemonDebug()"));

    /*
     * Load the C++ module name with the Java module name.
     */
    JString module(jmodule);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setDaemonDebug(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setDaemonDebug(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setDaemonDebug(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_setDaemonDebug(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("BusAttachment_setDaemonDebug(): Call SetDaemonDebug(%s, %d)", module.c_str(), jlevel));

    QStatus status = busPtr->SetDaemonDebug(module.c_str(), jlevel);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setDaemonDebug(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_setDaemonDebug(): SetDaemonDebug() fails"));
    }

    return JStatus(status);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_setLogLevels(JNIEnv*env, jobject thiz, jstring jlogEnv)
{
    QCC_DbgPrintf(("BusAttachment_setLogLevels()"));

    /*
     * Load the C++ environment string with the Java environment string.
     */
    JString logEnv(jlogEnv);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setLogLevels(): Exception"));
        return;
    }

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("QCC_SetLogLevels(%s)", logEnv.c_str()));
    QCC_SetLogLevels(logEnv.c_str());
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_setDebugLevel(JNIEnv*env, jobject thiz, jstring jmodule, jint jlevel)
{
    QCC_DbgPrintf(("BusAttachment_setDebugLevel()"));

    /*
     * Load the C++ module string with the Java module string.
     */
    JString module(jmodule);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setDebugLevel(): Exception"));
        return;
    }

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("QCC_SetDebugLevel(%s, %d)", module.c_str(), jlevel));
    QCC_SetDebugLevel(module.c_str(), jlevel);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_useOSLogging(JNIEnv*env, jobject thiz, jboolean juseOSLog)
{
    QCC_DbgPrintf(("BusAttachment_useOSLogging()"));

    /*
     * Make the AllJoyn call.
     */
    QCC_DbgPrintf(("QCC_UseOSLogging(%d)", juseOSLog));
    QCC_UseOSLogging(juseOSLog);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_setAnnounceFlag(JNIEnv* env, jobject thiz, jobject jbusObject, jstring jifaceName, jboolean jisAnnounced)
{
    QCC_DbgPrintf(("BusAttachment_setAnnounceFlag()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck() || busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setAnnounceFlag(): Exception or NULL bus pointer"));
        return JStatus(ER_FAIL);
    }

    JString ifaceName(jifaceName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setAnnounceFlag(): Exception"));
        return JStatus(ER_FAIL);
    }


    gBusObjectMapLock.Lock();
    JBusObject* busObject = GetBackingObject(jbusObject);

    if (!busObject) {
        QCC_DbgPrintf(("BusAttachment_setAnnounceFlag(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        QCC_LogError(ER_BUS_NO_SUCH_OBJECT, ("BusAttachment_setAnnounceFlag(): BusObject not found"));
        return JStatus(ER_BUS_NO_SUCH_OBJECT);
    }

    QStatus status = ER_OK;
    const InterfaceDescription* iface = busPtr->GetInterface(ifaceName.c_str());
    if (!iface) {
        gBusObjectMapLock.Unlock();
        return JStatus(ER_BUS_OBJECT_NO_SUCH_INTERFACE);
    }
    if (jisAnnounced) {
        QCC_DbgPrintf(("BusAttachment_setAnnounceFlag(): ANNOUNCED"));
        status = busObject->SetAnnounceFlag(iface, BusObject::ANNOUNCED);
    } else {
        QCC_DbgPrintf(("BusAttachment_setAnnounceFlag(): UNANNOUNCED"));
        status = busObject->SetAnnounceFlag(iface, BusObject::UNANNOUNCED);
    }

    gBusObjectMapLock.Unlock();
    QCC_DbgPrintf(("BusAttachment_setAnnounceFlag(): Releasing global Bus Object map lock"));
    return JStatus(status);
}


JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_connect(JNIEnv* env,
                                                                     jobject thiz,
                                                                     jstring jconnectArgs,
                                                                     jobject jkeyStoreListener,
                                                                     jstring jauthMechanisms,
                                                                     jobject jauthListener,
                                                                     jstring jkeyStoreFileName,
                                                                     jboolean isShared)
{
    QCC_DbgPrintf(("BusAttachment_connect()"));

    JString connectArgs(jconnectArgs);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_connect(): Exception"));
        return NULL;
    }

    JString authMechanisms(jauthMechanisms);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_connect(): Exception"));
        return NULL;
    }

    JString keyStoreFileName(jkeyStoreFileName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_connect(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_connect(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_connect(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_connect(): Refcount on busPtr is %d", busPtr->GetRef()));

    QStatus status = busPtr->Connect(connectArgs.c_str(), jkeyStoreListener, authMechanisms.c_str(),
                                     jauthListener, keyStoreFileName.c_str(), isShared);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_connect(): Exception"));
        return NULL;
    } else {
        return JStatus(status);
    }
}

JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_BusAttachment_isConnected(JNIEnv*env, jobject thiz)
{
    QCC_DbgPrintf(("BusAttachment_isConnected()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_isConnected(): Exception"));
        return false;
    }

    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_isConnected(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return false;
    }
    return busPtr->IsConnected();
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_disconnect(JNIEnv* env, jobject thiz, jstring jconnectArgs)
{
    QCC_DbgPrintf(("BusAttachment_disconnect()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_disconnect(): Exception"));
        return;
    }

    /*
     * It is possible that having a NULL busPtr at this point is perfectly legal.
     * This would happen if the client explitly called release() before giving
     * up its Java pointer.  In this case, by definition, the underlying C++
     * object has been released and our busPtr will be NULL.  We print a debug
     * message in case this is unexpected, but do not produce an error.
     */
    if (busPtr == NULL) {
        QCC_DbgPrintf(("BusAttachment_disconnect(): NULL bus pointer"));
        return;
    }

    JString connectArgs(jconnectArgs);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_disconnect(): Exception"));
        return;
    }

    QCC_DbgPrintf(("BusAttachment_disconnect(): Refcount on busPtr is %d", busPtr->GetRef()));

    busPtr->Disconnect(connectArgs.c_str());
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_enablePeerSecurity(JNIEnv* env, jobject thiz, jstring jauthMechanisms, jobject jauthListener,
                                                                                jstring jkeyStoreFileName, jboolean isShared)
{
    QCC_DbgPrintf(("BusAttachment_enablePeerSecurity()"));

    JString authMechanisms(jauthMechanisms);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    JString keyStoreFileName(jkeyStoreFileName);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_enablePeerSecurity(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_enablePeerSecurity(): Refcount on busPtr is %d", busPtr->GetRef()));

    QStatus status = busPtr->EnablePeerSecurity(authMechanisms.c_str(), jauthListener, keyStoreFileName.c_str(), isShared);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_enablePeerSecurity(): Exception"));
        return NULL;
    } else {
        return JStatus(status);
    }
}

/**
 * Create a new JBusObject to serve as the C++ half of a Java BusObject and C++
 * JBusObject pair.
 */
JBusObject::JBusObject(JBusAttachment* jbap, const char* path, jobject jobj)
    : BusObject(path), jbusObj(NULL), MID_generateIntrospection(NULL), MID_generateIntrospectionWithDesc(NULL),
    MID_registered(NULL), MID_unregistered(NULL), jtranslatorRef(NULL)
{
    QCC_DbgPrintf(("JBusObject::JBusObject()"));

    /*
     * Note the sneaky case here where we get a JBusAttachmentPtr* and we give a
     * reference to the underlying BusAttachment to the constructing BusObject.
     * Since the uderlying BusObject takes a reference to the provided
     * JBusAttachment, we must take a reference to the bus attachment even
     * though we never actually use it.  Because we take a reference here, we
     * need to give one in the destructor, which means that we have to save a
     * a copy of the JBusAttachment* in the JBusObject and therefore a given
     * bus object cannot be shared among bus attachments.
     */
    busPtr = jbap;
    QCC_DbgPrintf(("JBusObject::JBusObject(): Refcount on busPtr before is %d", busPtr->GetRef()));
    busPtr->IncRef();
    QCC_DbgPrintf(("JBusObject::JBusObject(): Refcount on busPtr after is %d", busPtr->GetRef()));

    JNIEnv* env = GetEnv();

    /*
     * take a weak global reference back to the Java object.  We expect the bus
     * attachment to have a strong reference to keep it from being garbage
     * collected, but we need to get back to it without interfering with GC.
     */
    QCC_DbgPrintf(("JBusObject::JBusObject():  Taking new weak global reference to BusObject %p", jobj));
    jbusObj = env->NewWeakGlobalRef(jobj);
    if (!jbusObj) {
        return;
    }
    QCC_DbgPrintf(("JBusObject::JBusObject():  Remembering weak global reference %p", jbusObj));

    if (env->IsInstanceOf(jobj, CLS_IntrospectionListener)) {
        JLocalRef<jclass> clazz = env->GetObjectClass(jobj);

        MID_generateIntrospection = env->GetMethodID(clazz, "generateIntrospection", "(ZI)Ljava/lang/String;");
        if (!MID_generateIntrospection) {
            return;
        }
    }

    if (env->IsInstanceOf(jobj, CLS_IntrospectionWithDescListener)) {
        JLocalRef<jclass> clazz = env->GetObjectClass(jobj);

        MID_generateIntrospectionWithDesc = env->GetMethodID(clazz, "generateIntrospection",
                                                             "(Ljava/lang/String;ZI)Ljava/lang/String;");
        if (!MID_generateIntrospectionWithDesc) {
            return;
        }
    }

    if (env->IsInstanceOf(jobj, CLS_BusObjectListener)) {
        JLocalRef<jclass> clazz = env->GetObjectClass(jobj);

        MID_registered = env->GetMethodID(clazz, "registered", "()V");
        if (!MID_registered) {
            return;
        }

        MID_unregistered = env->GetMethodID(clazz, "unregistered", "()V");
        if (!MID_unregistered) {
            return;
        }
    }
}

JBusObject::~JBusObject()
{
    QCC_DbgPrintf(("JBusObject::~JBusObject()"));

    JNIEnv* env = GetEnv();

    mapLock.Lock();

    QCC_DbgPrintf(("JBusObject::~JBusObject(): Deleting methods"));
    for (JMethod::const_iterator method = methods.begin(); method != methods.end(); ++method) {
        QCC_DbgPrintf(("JBusObject::~JBusObject(): Deleting method %p", method->second));
        env->DeleteGlobalRef(method->second);
    }

    QCC_DbgPrintf(("JBusObject::~JBusObject(): Deleting properties"));
    for (JProperty::const_iterator property = properties.begin(); property != properties.end(); ++property) {
        QCC_DbgPrintf(("JBusObject::~JBusObject(): Deleting property getter %p", property->second.jget));
        env->DeleteGlobalRef(property->second.jget);

        QCC_DbgPrintf(("JBusObject::~JBusObject(): Deleting property setter %p", property->second.jset));
        env->DeleteGlobalRef(property->second.jset);
    }

    mapLock.Unlock();

    if (jbusObj) {
        QCC_DbgPrintf(("JBusObject::~JBusObject(): Deleting weak global reference to BusObject %p", jbusObj));
        env->DeleteWeakGlobalRef(jbusObj);
        jbusObj = NULL;
    }

    QCC_DbgPrintf(("JBusObject::~JBusObject(): Releasing strong global reference to Translator %p", jtranslatorRef));
    env->DeleteGlobalRef(jtranslatorRef);
    jtranslatorRef = NULL;

    QCC_DbgPrintf(("JBusObject::~JBusObject(): Refcount on busPtr before decrement is %d", busPtr->GetRef()));
    busPtr->DecRef();
    busPtr = NULL;
}

QStatus JBusObject::AddInterfaces(jobjectArray jbusInterfaces)
{
    QCC_DbgPrintf(("JBusObject::AddInterfaces()"));

    QStatus status;

    JNIEnv* env = GetEnv();
    jsize len = env->GetArrayLength(jbusInterfaces);

    for (jsize i = 0; i < len; ++i) {
        JLocalRef<jobject> jbusInterface = GetObjectArrayElement(env, jbusInterfaces, i);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("JBusObject::AddInterfaces(): Exception"));
            return ER_FAIL;
        }
        assert(jbusInterface);

        const InterfaceDescription* intf = GetHandle<const InterfaceDescription*>(jbusInterface);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("JBusObject::AddInterfaces(): Exception"));
            return ER_FAIL;
        }
        assert(intf);

        JLocalRef<jclass> clazz = env->GetObjectClass(jbusInterface);
        jmethodID announced_mid = env->GetMethodID(clazz, "isAnnounced", "()Z");
        if (!announced_mid) {
            QCC_DbgPrintf(("JBusObject::AddInterfaces() failed to call isAnnounced"));
            status = ER_FAIL;
            break;
        }
        jboolean isAnnounced = env->CallBooleanMethod(jbusInterface, announced_mid);

        if (isAnnounced == JNI_TRUE) {
            QCC_DbgPrintf(("JBusObject::AddInterfaces() isAnnounced returned true"));
            status = AddInterface(*intf, ANNOUNCED);
        } else {
            QCC_DbgPrintf(("JBusObject::AddInterfaces() isAnnounced returned false"));
            status = AddInterface(*intf);

        }
        if (ER_OK != status) {
            return status;
        }

        size_t numMembs = intf->GetMembers(NULL);
        const InterfaceDescription::Member** membs = new const InterfaceDescription::Member *[numMembs];
        if (!membs) {
            return ER_OUT_OF_MEMORY;
        }

        intf->GetMembers(membs, numMembs);
        for (size_t m = 0; m < numMembs; ++m) {
            if (MESSAGE_METHOD_CALL == membs[m]->memberType) {
                status = AddMethodHandler(membs[m], static_cast<MessageReceiver::MethodHandler>(&JBusObject::MethodHandler));
                if (ER_OK != status) {
                    break;
                }

                JLocalRef<jstring> jname = env->NewStringUTF(membs[m]->name.c_str());
                if (!jname) {
                    status = ER_FAIL;
                    break;
                }

                JLocalRef<jclass> clazz = env->GetObjectClass(jbusInterface);
                jmethodID mid = env->GetMethodID(clazz, "getMember", "(Ljava/lang/String;)Ljava/lang/reflect/Method;");
                if (!mid) {
                    status = ER_FAIL;
                    break;
                }

                JLocalRef<jobject> jmethod = CallObjectMethod(env, jbusInterface, mid, (jstring)jname);
                if (env->ExceptionCheck()) {
                    status = ER_FAIL;
                    break;
                }
                if (!jmethod) {
                    status = ER_BUS_INTERFACE_NO_SUCH_MEMBER;
                    break;
                }

                jobject jref = env->NewGlobalRef(jmethod);
                if (!jref) {
                    status = ER_FAIL;
                    break;
                }

                String key = intf->GetName() + membs[m]->name;
                methods.insert(pair<String, jobject>(key, jref));
            }
        }

        delete [] membs;
        membs = NULL;
        if (ER_OK != status) {
            return status;
        }

        size_t numProps = intf->GetProperties(NULL);
        const InterfaceDescription::Property** props = new const InterfaceDescription::Property *[numProps];
        if (!props) {
            return ER_OUT_OF_MEMORY;
        }

        intf->GetProperties(props, numProps);

        for (size_t p = 0; p < numProps; ++p) {
            Property property;
            property.signature = props[p]->signature;

            JLocalRef<jstring> jname = env->NewStringUTF(props[p]->name.c_str());
            if (!jname) {
                status = ER_FAIL;
                break;
            }

            JLocalRef<jclass> clazz = env->GetObjectClass(jbusInterface);
            jmethodID mid = env->GetMethodID(clazz, "getProperty",
                                             "(Ljava/lang/String;)[Ljava/lang/reflect/Method;");
            if (!mid) {
                status = ER_FAIL;
                break;
            }

            JLocalRef<jobjectArray> jmethods = (jobjectArray)CallObjectMethod(env, jbusInterface, mid, (jstring)jname);
            if (env->ExceptionCheck()) {
                status = ER_FAIL;
                break;
            }
            if (!jmethods) {
                status = ER_BUS_NO_SUCH_PROPERTY;
                break;
            }

            JLocalRef<jobject> jget = GetObjectArrayElement(env, jmethods, 0);
            if (env->ExceptionCheck()) {
                status = ER_FAIL;
                break;
            }
            if (jget) {
                property.jget = env->NewGlobalRef(jget);
                if (!property.jget) {
                    status = ER_FAIL;
                    break;
                }
            } else {
                property.jget = NULL;
            }

            JLocalRef<jobject> jset = GetObjectArrayElement(env, jmethods, 1);
            if (env->ExceptionCheck()) {
                status = ER_FAIL;
                break;
            }

            if (jset) {
                property.jset = env->NewGlobalRef(jset);
                if (!property.jset) {
                    status = ER_FAIL;
                    break;
                }
            } else {
                property.jset = NULL;
            }

            String key = intf->GetName() + props[p]->name;
            properties.insert(pair<String, Property>(key, property));
        }
        delete [] props;
        props = NULL;
        if (ER_OK != status) {
            return status;
        }
    }

    return ER_OK;
}

/**
 * Marshal an Object into a MsgArg.
 *
 * @param[in] signature the signature of the Object
 * @param[in] jarg the Object
 * @param[in] arg the MsgArg to marshal into
 * @return the marshalled MsgArg or NULL if the marshalling failed.  This will
 *         be the same as @param arg if marshalling succeeded.
 */
static MsgArg* Marshal(const char* signature, jobject jarg, MsgArg* arg)
{
    JNIEnv* env = GetEnv();
    JLocalRef<jstring> jsignature = env->NewStringUTF(signature);
    if (!jsignature) {
        return NULL;
    }
    env->CallStaticVoidMethod(CLS_MsgArg, MID_MsgArg_marshal, (jlong)arg, (jstring)jsignature, jarg);
    if (env->ExceptionCheck()) {
        return NULL;
    }
    return arg;
}

/**
 * Marshal an Object[] into MsgArgs.  The arguments are marshalled into an
 * ALLJOYN_STRUCT with the members set to the marshalled Object[] elements.
 *
 * @param[in] signature the signature of the Object[]
 * @param[in] jargs the Object[]
 * @param[in] arg the MsgArg to marshal into
 * @return an ALLJOYN_STRUCT containing the marshalled MsgArgs or NULL if the
 *         marshalling failed.  This will be the same as @param arg if
 *         marshalling succeeded.
 */
static MsgArg* Marshal(const char* signature, jobjectArray jargs, MsgArg* arg)
{
    JNIEnv* env = GetEnv();
    JLocalRef<jstring> jsignature = env->NewStringUTF(signature);
    if (!jsignature) {
        return NULL;
    }
    env->CallStaticVoidMethod(CLS_MsgArg, MID_MsgArg_marshal_array, (jlong)arg, (jstring)jsignature, jargs);
    if (env->ExceptionCheck()) {
        return NULL;
    }
    return arg;
}

/**
 * Unmarshal a single MsgArg into an Object.
 *
 * @param[in] arg the MsgArg
 * @param[in] jtype the Type of the Object to unmarshal into
 * @return the unmarshalled Java Object
 */
static jobject Unmarshal(const MsgArg* arg, jobject jtype)
{
    JNIEnv* env = GetEnv();
    jobject jarg = CallStaticObjectMethod(env, CLS_MsgArg, MID_MsgArg_unmarshal, (jlong)arg, jtype);
    if (env->ExceptionCheck()) {
        return NULL;
    }
    return jarg;
}

/**
 * Unmarshal MsgArgs into an Object[].
 *
 * @param[in] args the MsgArgs
 * @param[in] numArgs the number of MsgArgs
 * @param[in] jmethod the Method that will be invoked with the returned Object[]
 * @param[out] junmarshalled the unmarshalled Java Object[]
 */
static QStatus Unmarshal(const MsgArg* args, size_t numArgs, jobject jmethod,
                         JLocalRef<jobjectArray>& junmarshalled)
{
    MsgArg arg(ALLJOYN_STRUCT);
    arg.v_struct.members = (MsgArg*)args;
    arg.v_struct.numMembers = numArgs;
    JNIEnv* env = GetEnv();
    junmarshalled = (jobjectArray)CallStaticObjectMethod(env, CLS_MsgArg, MID_MsgArg_unmarshal_array,
                                                         jmethod, (jlong) & arg);
    if (env->ExceptionCheck()) {
        return ER_FAIL;
    }
    return ER_OK;
}

/**
 * Unmarshal an AllJoyn message into an Object[].
 *
 * @param[in] msg the AllJoyn message received
 * @param[in] jmethod the Method that will be invoked with the returned Object[]
 * @param[out] junmarshalled the unmarshalled Java Objects
 */
static QStatus Unmarshal(Message& msg, jobject jmethod, JLocalRef<jobjectArray>& junmarshalled)
{
    const MsgArg* args;
    size_t numArgs;
    msg->GetArgs(numArgs, args);
    return Unmarshal(args, numArgs, jmethod, junmarshalled);
}

void JBusObject::MethodHandler(const InterfaceDescription::Member* member, Message& msg)
{
    QCC_DbgPrintf(("JBusObject::MethodHandler()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    MessageContext context(msg);
    /*
     * The Java method is called via invoke() on the
     * java.lang.reflect.Method object.  This allows us to package up
     * all the message args into an Object[], saving us from having to
     * figure out the signature of each method to lookup.
     */
    String key = member->iface->GetName() + member->name;

    /*
     * We're going to wander into a list of methods and pick one.  Lock the
     * mutex that protects this list for the entire time we'll be using the
     * list and the found method.
     */
    mapLock.Lock();

    JMethod::const_iterator method = methods.find(key);
    if (methods.end() == method) {
        mapLock.Unlock();
        MethodReply(member, msg, ER_BUS_OBJECT_NO_SUCH_MEMBER);
        return;
    }

    JLocalRef<jobjectArray> jargs;
    QStatus status = Unmarshal(msg, method->second, jargs);
    if (ER_OK != status) {
        mapLock.Unlock();
        MethodReply(member, msg, status);
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(method->second);
    jmethodID mid = env->GetMethodID(clazz, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    if (!mid) {
        mapLock.Unlock();
        MethodReply(member, msg, ER_FAIL);
        return;
    }

    /*
     * The weak global reference jbusObj cannot be directly used.  We have to
     * get a "hard" reference to it and then use that.  If you try to use a weak
     * reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusObj);
    if (!jo) {
        mapLock.Unlock();
        QCC_LogError(ER_FAIL, ("JBusObject::MethodHandler(): Can't get new local reference to BusObject"));
        return;
    }

    mapLock.Unlock();

    JLocalRef<jobject> jreply = CallObjectMethod(env, method->second, mid, jo, (jobjectArray)jargs);
    JLocalRef<jthrowable> ex = env->ExceptionOccurred();
    if (ex) {
        env->ExceptionClear();
        JLocalRef<jclass> clazz = env->GetObjectClass(ex);
        jmethodID mid = env->GetMethodID(clazz, "getCause", "()Ljava/lang/Throwable;");
        if (!mid) {
            MethodReply(member, msg, ER_FAIL);
            return;
        }
        ex = (jthrowable)CallObjectMethod(env, ex, mid);
        if (env->ExceptionCheck()) {
            MethodReply(member, msg, ER_FAIL);
            return;
        }

        clazz = env->GetObjectClass(ex);
        if (env->IsInstanceOf(ex, CLS_ErrorReplyBusException)) {
            mid = env->GetMethodID(clazz, "getErrorStatus", "()Lorg/alljoyn/bus/Status;");
            if (!mid) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }
            JLocalRef<jobject> jstatus = CallObjectMethod(env, ex, mid);
            if (env->ExceptionCheck()) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }
            JLocalRef<jclass> statusClazz = env->GetObjectClass(jstatus);
            mid = env->GetMethodID(statusClazz, "getErrorCode", "()I");
            if (!mid) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }
            QStatus errorCode = (QStatus)env->CallIntMethod(jstatus, mid);
            if (env->ExceptionCheck()) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }

            mid = env->GetMethodID(clazz, "getErrorName", "()Ljava/lang/String;");
            if (!mid) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }
            JLocalRef<jstring> jerrorName = (jstring)CallObjectMethod(env, ex, mid);
            if (env->ExceptionCheck()) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }
            JString errorName(jerrorName);
            if (env->ExceptionCheck()) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }

            mid = env->GetMethodID(clazz, "getErrorMessage", "()Ljava/lang/String;");
            if (!mid) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }
            JLocalRef<jstring> jerrorMessage = (jstring)CallObjectMethod(env, ex, mid);
            if (env->ExceptionCheck()) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }
            JString errorMessage(jerrorMessage);
            if (env->ExceptionCheck()) {
                MethodReply(member, msg, ER_FAIL);
                return;
            }

            if (errorName.c_str()) {
                MethodReply(member, msg, errorName.c_str(), errorMessage.c_str());
            } else {
                MethodReply(member, msg, errorCode);
            }
        } else {
            MethodReply(member, msg, ER_FAIL);
        }
        return;
    }

    MethodReply(member, msg, jreply);
}

QStatus JBusObject::MethodReply(const InterfaceDescription::Member* member, Message& msg, QStatus status)
{
    QCC_DbgPrintf(("JBusObject::MethodReply()"));

    qcc::String val;
    if (member->GetAnnotation(org::freedesktop::DBus::AnnotateNoReply, val) && val == "true") {
        return ER_OK;
    } else {
        return BusObject::MethodReply(msg, status);
    }
}

QStatus JBusObject::MethodReply(const InterfaceDescription::Member* member, const Message& msg, const char* error, const char* errorMessage)
{
    QCC_DbgPrintf(("JBusObject::MethodReply()"));

    qcc::String val;
    if (member->GetAnnotation(org::freedesktop::DBus::AnnotateNoReply, val) && val == "true") {
        return ER_OK;
    } else {
        return BusObject::MethodReply(msg, error, errorMessage);
    }
}

QStatus JBusObject::MethodReply(const InterfaceDescription::Member* member, Message& msg, jobject jreply)
{
    QCC_DbgPrintf(("JBusObject::MethodReply()"));

    qcc::String val;
    if (member->GetAnnotation(org::freedesktop::DBus::AnnotateNoReply, val) && val == "true") {
        if (!jreply) {
            return ER_OK;
        } else {
            QCC_LogError(ER_BUS_BAD_HDR_FLAGS,
                         ("Method %s is annotated as 'no reply' but value returned, replying anyway",
                          member->name.c_str()));
        }
    }
    JNIEnv* env = GetEnv();
    MsgArg replyArgs;
    QStatus status;
    uint8_t completeTypes = SignatureUtils::CountCompleteTypes(member->returnSignature.c_str());
    if (jreply) {
        JLocalRef<jobjectArray> jreplyArgs;
        if (completeTypes > 1) {
            jmethodID mid = env->GetStaticMethodID(CLS_Signature, "structArgs",
                                                   "(Ljava/lang/Object;)[Ljava/lang/Object;");
            if (!mid) {
                return MethodReply(member, msg, ER_FAIL);
            }
            jreplyArgs = (jobjectArray)CallStaticObjectMethod(env, CLS_Signature, mid, (jobject)jreply);
            if (env->ExceptionCheck()) {
                return MethodReply(member, msg, ER_FAIL);
            }
        } else {
            /*
             * Create Object[] out of the invoke() return value to reuse
             * marshalling code in Marshal() for the reply message.
             */
            jreplyArgs = env->NewObjectArray(1, CLS_Object, NULL);
            if (!jreplyArgs) {
                return MethodReply(member, msg, ER_FAIL);
            }
            env->SetObjectArrayElement(jreplyArgs, 0, jreply);
            if (env->ExceptionCheck()) {
                return MethodReply(member, msg, ER_FAIL);
            }
        }
        if (!Marshal(member->returnSignature.c_str(), jreplyArgs, &replyArgs)) {
            return MethodReply(member, msg, ER_FAIL);
        }
        status = BusObject::MethodReply(msg, replyArgs.v_struct.members, replyArgs.v_struct.numMembers);
    } else if (completeTypes) {
        String errorMessage(member->iface->GetName());
        errorMessage += "." + member->name + " returned null";
        QCC_LogError(ER_BUS_BAD_VALUE, (errorMessage.c_str()));
        status = BusObject::MethodReply(msg, "org.alljoyn.bus.BusException", errorMessage.c_str());
    } else {
        status = BusObject::MethodReply(msg, (MsgArg*)NULL, 0);
    }
    if (ER_OK != status) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
    return status;
}

QStatus JBusObject::Signal(const char* destination, SessionId sessionId, const char* ifaceName, const char* signalName,
                           const MsgArg* args, size_t numArgs, uint32_t timeToLive, uint8_t flags, Message& msg)
{
    QCC_DbgPrintf(("JBusObject::Signal()"));

    const InterfaceDescription* intf = bus->GetInterface(ifaceName);
    if (!intf) {
        return ER_BUS_OBJECT_NO_SUCH_INTERFACE;
    }
    const InterfaceDescription::Member* signal = intf->GetMember(signalName);
    if (!signal) {
        return ER_BUS_OBJECT_NO_SUCH_MEMBER;
    }
    return BusObject::Signal(destination, sessionId, *signal, args, numArgs, timeToLive, flags, &msg);
}

QStatus JBusObject::Get(const char* ifcName, const char* propName, MsgArg& val)
{
    QCC_DbgPrintf(("JBusObject::Get()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    String key = String(ifcName) + propName;

    /*
     * We're going to wander into a list of properties and pick one.  Lock the
     * mutex that protects this list for the entire time we'll be using the list
     * and the found method.
     */
    mapLock.Lock();

    JProperty::const_iterator property = properties.find(key);
    if (properties.end() == property) {
        mapLock.Unlock();
        return ER_BUS_NO_SUCH_PROPERTY;
    }
    if (!property->second.jget) {
        mapLock.Unlock();
        return ER_BUS_PROPERTY_ACCESS_DENIED;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(property->second.jget);
    jmethodID mid = env->GetMethodID(clazz, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    if (!mid) {
        mapLock.Unlock();
        return ER_FAIL;
    }

    /*
     * The weak global reference jbusObj cannot be directly used.  We have to
     * get a "hard" reference to it and then use that.  If you try to use a weak
     * reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusObj);
    if (!jo) {
        mapLock.Unlock();
        QCC_LogError(ER_FAIL, ("JBusObject::Get(): Can't get new local reference to BusObject"));
        return ER_FAIL;
    }

    JLocalRef<jobject> jvalue = CallObjectMethod(env, property->second.jget, mid, jo, NULL);
    if (env->ExceptionCheck()) {
        mapLock.Unlock();
        return ER_FAIL;
    }

    if (!Marshal(property->second.signature.c_str(), (jobject)jvalue, &val)) {
        mapLock.Unlock();
        return ER_FAIL;
    }

    mapLock.Unlock();
    return ER_OK;
}

QStatus JBusObject::Set(const char* ifcName, const char* propName, MsgArg& val)
{
    QCC_DbgPrintf(("JBusObject::Set()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    String key = String(ifcName) + propName;

    /*
     * We're going to wander into a list of properties and pick one.  Lock the
     * mutex that protects this list for the entire time we'll be using the list
     * and the found method.
     */
    mapLock.Lock();

    JProperty::const_iterator property = properties.find(key);
    if (properties.end() == property) {
        mapLock.Unlock();
        return ER_BUS_NO_SUCH_PROPERTY;
    }
    if (!property->second.jset) {
        mapLock.Unlock();
        return ER_BUS_PROPERTY_ACCESS_DENIED;
    }

    JLocalRef<jobjectArray> jvalue;
    QStatus status = Unmarshal(&val, 1, property->second.jset, jvalue);
    if (ER_OK != status) {
        mapLock.Unlock();
        return status;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(property->second.jset);
    jmethodID mid = env->GetMethodID(clazz, "invoke", "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    if (!mid) {
        mapLock.Unlock();
        return ER_FAIL;
    }

    /*
     * The weak global reference jbusObj cannot be directly used.  We have to
     * get a "hard" reference to it and then use that.  If you try to use a weak
     * reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jbusObj);
    if (!jo) {
        mapLock.Unlock();
        QCC_LogError(ER_FAIL, ("JBusObject::Set(): Can't get new local reference to BusObject"));
        return ER_FAIL;
    }

    CallObjectMethod(env, property->second.jset, mid, jo, (jobjectArray)jvalue);
    if (env->ExceptionCheck()) {
        mapLock.Unlock();
        return ER_FAIL;
    }

    mapLock.Unlock();
    return ER_OK;
}

String JBusObject::GenerateIntrospection(const char* languageTag, bool deep, size_t indent) const
{
    QCC_DbgPrintf(("JBusObject::GenerateIntrospection()"));

    if (!languageTag) {
        return JBusObject::GenerateIntrospection(deep, indent);
    }

    if (MID_generateIntrospectionWithDesc) {
        /*
         * JScopedEnv will automagically attach the JVM to the current native
         * thread.
         */
        JScopedEnv env;

        /*
         * The weak global reference jbusObj cannot be directly used.  We have to
         * get a "hard" reference to it and then use that.  If you try to use a weak
         * reference directly you will crash and burn.
         */
        jobject jo = env->NewLocalRef(jbusObj);
        if (!jo) {
            QCC_LogError(ER_FAIL, ("JBusObject::GenerateIntrospection(): Can't get new local reference to BusObject"));
            return "";
        }

        JLocalRef<jstring> jlang = env->NewStringUTF(languageTag);
        JLocalRef<jstring> jintrospection = (jstring)CallObjectMethod(env,
                                                                      jo, MID_generateIntrospectionWithDesc, (jstring)jlang, deep, indent);
        if (env->ExceptionCheck()) {
            return BusObject::GenerateIntrospection(languageTag, deep, indent);
        }

        JString introspection(jintrospection);
        if (env->ExceptionCheck()) {
            return BusObject::GenerateIntrospection(languageTag, deep, indent);
        }

        return String(introspection.c_str());
    }

    return BusObject::GenerateIntrospection(languageTag, deep, indent);
}

String JBusObject::GenerateIntrospection(bool deep, size_t indent) const
{
    QCC_DbgPrintf(("JBusObject::GenerateIntrospection()"));


    //Use either interface but prefer IntrospectionListener since it's more specific
    if (MID_generateIntrospectionWithDesc || MID_generateIntrospection) {
        /*
         * JScopedEnv will automagically attach the JVM to the current native
         * thread.
         */
        JScopedEnv env;

        /*
         * The weak global reference jbusObj cannot be directly used.  We have to
         * get a "hard" reference to it and then use that.  If you try to use a weak
         * reference directly you will crash and burn.
         */
        jobject jo = env->NewLocalRef(jbusObj);
        if (!jo) {
            QCC_LogError(ER_FAIL, ("JBusObject::GenerateIntrospection(): Can't get new local reference to BusObject"));
            return "";
        }

        JLocalRef<jstring> jintrospection;
        if (MID_generateIntrospection) {
            jintrospection = (jstring)CallObjectMethod(env, jo, MID_generateIntrospection, deep, indent);
        } else {
            jintrospection = (jstring)CallObjectMethod(env, jo, MID_generateIntrospectionWithDesc, deep, indent, NULL);
        }

        if (env->ExceptionCheck()) {
            return BusObject::GenerateIntrospection(deep, indent);
        }

        JString introspection(jintrospection);
        if (env->ExceptionCheck()) {
            return BusObject::GenerateIntrospection(deep, indent);
        }

        return String(introspection.c_str());
    }

    return BusObject::GenerateIntrospection(deep, indent);
}

void JBusObject::ObjectRegistered()
{
    QCC_DbgPrintf(("JBusObject::ObjectRegistered()"));

    BusObject::ObjectRegistered();
    if (NULL != MID_registered) {
        /*
         * JScopedEnv will automagically attach the JVM to the current native
         * thread.
         */
        JScopedEnv env;

        /*
         * The weak global reference jbusObj cannot be directly used.  We have to
         * get a "hard" reference to it and then use that.  If you try to use a weak
         * reference directly you will crash and burn.
         */
        jobject jo = env->NewLocalRef(jbusObj);
        if (!jo) {
            QCC_LogError(ER_FAIL, ("JBusObject::ObjectRegistered(): Can't get new local reference to BusObject"));
            return;
        }

        env->CallVoidMethod(jo, MID_registered);
    }
}

void JBusObject::ObjectUnregistered()
{
    QCC_DbgPrintf(("JBusObject::ObjectUnregistered()"));

    BusObject::ObjectUnregistered();
    if (NULL != MID_registered) {
        /*
         * JScopedEnv will automagically attach the JVM to the current native
         * thread.
         */
        JScopedEnv env;

        /*
         * The weak global reference jbusObj cannot be directly used.  We have to
         * get a "hard" reference to it and then use that.  If you try to use a weak
         * reference directly you will crash and burn.
         */
        jobject jo = env->NewLocalRef(jbusObj);
        if (!jo) {
            QCC_LogError(ER_FAIL, ("JBusObject::ObjectUnregistered(): Can't get new local reference to BusObject"));
            return;
        }

        env->CallVoidMethod(jo, MID_unregistered);
    }
}

void JBusObject::SetDescriptions(jstring jlangTag, jstring jdescription, jobject jtranslator)
{
    QCC_DbgPrintf(("JBusObject::SetDescriptions()"));
    JNIEnv* env = GetEnv();

    JString langTag(jlangTag);
    JString description(jdescription);

    if (langTag.c_str() && description.c_str()) {
        SetDescription(langTag.c_str(), description.c_str());
    }

    if (jtranslator) {
        jobject jglobalref = env->NewGlobalRef(jtranslator);
        if (!jglobalref) {
            return;
        }
        jtranslatorRef = jglobalref;
        JTranslator* translator = GetHandle<JTranslator*>(jtranslator);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("BusAttachment_setDescriptionTranslator(): Exception"));
            return;
        }
        assert(translator);
        SetDescriptionTranslator(translator);
    }
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_registerBusObject(JNIEnv* env, jobject thiz, jstring jobjPath,
                                                                               jobject jbusObject, jobjectArray jbusInterfaces,
                                                                               jboolean jsecure, jstring jlangTag, jstring jdesc,
                                                                               jobject jtranslator)
{
    QCC_DbgPrintf(("BusAttachment_registerBusObject()"));

    JString objPath(jobjPath);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_registerBusObject(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_registerBusObject(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_registerBusObject(): Refcount on busPtr is %d", busPtr->GetRef()));

    QStatus status = busPtr->RegisterBusObject(objPath.c_str(), jbusObject, jbusInterfaces, jsecure, jlangTag, jdesc, jtranslator);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_registerBusObject(): Exception"));
        return NULL;
    }

    return JStatus(status);
}

JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_BusAttachment_isSecureBusObject(JNIEnv* env, jobject thiz, jobject jbusObject)
{
    QCC_DbgPrintf(("BusAttachment_isSecureBusObjectt()"));
    gBusObjectMapLock.Lock();
    JBusObject* busObject = GetBackingObject(jbusObject);

    if (!busObject) {
        QCC_DbgPrintf(("BusAttachment_isSecureBusObject(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        QCC_LogError(ER_FAIL, ("BusAttachment_isSecureBusObject(): Exception"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_BUS_NO_SUCH_OBJECT));
        return false;
    }
    bool result = busObject->IsSecure();
    gBusObjectMapLock.Unlock();
    return result;
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_unregisterBusObject(JNIEnv* env, jobject thiz, jobject jbusObject)
{
    QCC_DbgPrintf(("BusAttachment_unregisterBusObject()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_unregisterBusObject(): Exception"));
        return;
    }

    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_unregisterBusObject(): NULL bus pointer"));
        return;
    }

    QCC_DbgPrintf(("BusAttachment_unregisterBusObject(): Refcount on busPtr is %d", busPtr->GetRef()));

    busPtr->UnregisterBusObject(jbusObject);
}

JSignalHandler::JSignalHandler(jobject jobj, jobject jmeth)
    : jsignalHandler(NULL), jmethod(NULL), member(NULL)
{
    JNIEnv* env = GetEnv();
    jsignalHandler = env->NewWeakGlobalRef(jobj);
    jmethod = env->NewGlobalRef(jmeth);
}

JSignalHandler::~JSignalHandler()
{
    JNIEnv* env = GetEnv();
    if (jmethod) {
        QCC_DbgPrintf(("JSignalHandler::~JSignalHandler(): Forgetting jmethod"));
        env->DeleteGlobalRef(jmethod);
        jmethod = NULL;
    }
    if (jsignalHandler) {
        QCC_DbgPrintf(("JSignalHandler::~JSignalHandler(): Forgetting jsignalHandler"));
        env->DeleteWeakGlobalRef(jsignalHandler);
        jsignalHandler = NULL;
    }
}

bool JSignalHandler::IsSameObject(jobject jobj, jobject jmeth)
{
    JNIEnv* env = GetEnv();
    /*
     * The weak global reference jsignalHandler cannot be directly used.  We
     * have to get a "hard" reference to it and then use that.  If you try to
     * use a weak reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jsignalHandler);
    if (!jo) {
        return false;
    }

    return env->IsSameObject(jo, jobj) && env->CallBooleanMethod(jmethod, MID_Object_equals, jmeth);
}

QStatus JSignalHandler::Register(BusAttachment& bus, const char* ifaceName, const char* signalName,
                                 const char* ancillary) {

    if (bus.IsConnected() == false) {
        return ER_BUS_NOT_CONNECTED;
    }
    const InterfaceDescription* intf = bus.GetInterface(ifaceName);
    if (!intf) {
        return ER_BUS_NO_SUCH_INTERFACE;
    }
    member = intf->GetMember(signalName);
    if (!member) {
        return ER_BUS_INTERFACE_NO_SUCH_MEMBER;
    }
    ancillary_data = ancillary;
    return ER_OK;
}

void JSignalHandler::SignalHandler(const InterfaceDescription::Member* member,
                                   const char* sourcePath,
                                   Message& msg)
{
    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    MessageContext context(msg);

    JLocalRef<jobjectArray> jargs;
    QStatus status = Unmarshal(msg, jmethod, jargs);
    if (ER_OK != status) {
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jmethod);
    jmethodID mid = env->GetMethodID(clazz, "invoke",
                                     "(Ljava/lang/Object;[Ljava/lang/Object;)Ljava/lang/Object;");
    if (!mid) {
        return;
    }

    /*
     * The weak global reference jsignalHandler cannot be directly used.  We
     * have to get a "hard" reference to it and then use that.  If you try to
     * use a weak reference directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jsignalHandler);
    if (!jo) {
        return;
    }
    CallObjectMethod(env, jmethod, mid, jo, (jobjectArray)jargs);
}

QStatus JSignalHandlerWithSrc::Register(BusAttachment& bus, const char* ifaceName, const char* signalName,
                                        const char* ancillary) {

    QStatus status = JSignalHandler::Register(bus, ifaceName, signalName, ancillary);
    if (status != ER_OK) {
        return status;
    }

    return bus.RegisterSignalHandler(this,
                                     static_cast<MessageReceiver::SignalHandler>(&JSignalHandler::SignalHandler),
                                     member,
                                     ancillary_data.c_str());

}

void JSignalHandlerWithSrc::Unregister(BusAttachment& bus)
{
    if (bus.IsConnected() == false) {
        return;
    }

    if (member) {
        bus.UnregisterSignalHandler(this,
                                    static_cast<MessageReceiver::SignalHandler>(&JSignalHandler::SignalHandler),
                                    member,
                                    ancillary_data.c_str());
    }
}


QStatus JSignalHandlerWithRule::Register(BusAttachment& bus, const char* ifaceName, const char* signalName,
                                         const char* ancillary) {

    QStatus status = JSignalHandler::Register(bus, ifaceName, signalName, ancillary);
    if (status != ER_OK) {
        return status;
    }

    return bus.RegisterSignalHandlerWithRule(this,
                                             static_cast<MessageReceiver::SignalHandler>(&JSignalHandler::SignalHandler),
                                             member,
                                             ancillary_data.c_str());

}

void JSignalHandlerWithRule::Unregister(BusAttachment& bus)
{
    if (bus.IsConnected() == false) {
        return;
    }

    if (member) {
        bus.UnregisterSignalHandlerWithRule(this,
                                            static_cast<MessageReceiver::SignalHandler>(&JSignalHandler::SignalHandler),
                                            member,
                                            ancillary_data.c_str());
    }
}

JTranslator::JTranslator(jobject jobj)
{
    QCC_DbgPrintf(("JTranslator::JTranslator()"));

    JNIEnv* env = GetEnv();

    QCC_DbgPrintf(("JTranslator::JTranslator(): Taking weak global reference to DescriptionListener %p", jobj));
    jtranslator = env->NewWeakGlobalRef(jobj);
    if (!jtranslator) {
        QCC_LogError(ER_FAIL, ("JTranslator::JTranslator(): Can't create new weak global reference to Translator"));
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jobj);
    if (!clazz) {
        QCC_LogError(ER_FAIL, ("JTranslator::JTranslator(): Can't GetObjectClass() for Translator"));
        return;
    }

    MID_numTargetLanguages = env->GetMethodID(clazz, "numTargetLanguages", "()I");
    if (!MID_numTargetLanguages) {
        QCC_LogError(ER_FAIL, ("JTranslator::JTranslator(): Can't find numTargetLanguages() in Translator"));
    }

    MID_getTargetLanguage = env->GetMethodID(clazz, "getTargetLanguage", "(I)Ljava/lang/String;");
    if (!MID_getTargetLanguage) {
        QCC_LogError(ER_FAIL, ("JTranslator::JTranslator(): Can't find getTargetLanguage() in Translator"));
    }

    MID_translate = env->GetMethodID(clazz, "translate", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;");
    if (!MID_translate) {
        QCC_LogError(ER_FAIL, ("JTranslator::JTranslator(): Can't find translate() in Translator"));
    }

}

JTranslator::~JTranslator()
{
    QCC_DbgPrintf(("JTranslator::~JTranslator()"));

    if (jtranslator) {
        QCC_DbgPrintf(("JTranslator::~JTranslator(): Releasing weak global reference to Translator %p",
                       jtranslator));
        GetEnv()->DeleteWeakGlobalRef(jtranslator);
        jtranslator = NULL;
    }
}

size_t JTranslator::NumTargetLanguages()
{
    QCC_DbgPrintf(("JTranslator::NumTargetLanguages()"));
    JScopedEnv env;
    jobject jo = env->NewLocalRef(jtranslator);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JTranslator::NumTargetLanguages(): Can't get new local reference to Translator"));
        return 0;
    }

    size_t ret = (size_t)env->CallIntMethod(jo, MID_numTargetLanguages);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JTranslator::NumTargetLanguages(): Exception"));
        return 0;
    }

    return ret;
}

void JTranslator::GetTargetLanguage(size_t index, qcc::String& ret)
{
    QCC_DbgPrintf(("JTranslator::GetTargetLanguage()"));
    JScopedEnv env;

    jobject jo = env->NewLocalRef(jtranslator);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JTranslator::GetTargetLanguage(): Can't get new local reference to Translator"));
        return;
    }

    JLocalRef<jstring> jres = (jstring)CallObjectMethod(env, jo, MID_getTargetLanguage, (jint)index);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JTranslator::GetTargetLanguage(): Can't get new local reference to Translator"));
        return;
    }

    if (NULL == jres) {
        return;
    }

    const char* chars = env->GetStringUTFChars(jres, NULL);
    ret.assign(chars);
    env->ReleaseStringUTFChars(jres, chars);
}

const char* JTranslator::Translate(const char* sourceLanguage,
                                   const char* targetLanguage, const char* source, qcc::String& buffer)
{
    QCC_DbgPrintf(("JTranslator::Translate()"));
    JScopedEnv env;

    JLocalRef<jstring> jsourceLang = env->NewStringUTF(sourceLanguage);
    JLocalRef<jstring> jtargLang = env->NewStringUTF(targetLanguage);
    JLocalRef<jstring> jsource = env->NewStringUTF(source);

    jobject jo = env->NewLocalRef(jtranslator);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JTranslator::Translate(): Can't get new local reference to Translator"));
        return NULL;
    }

    QCC_DbgPrintf(("JTranslator::Translate(): Call out"));
    JLocalRef<jstring> jres = (jstring)CallObjectMethod(env, jo, MID_translate, (jstring)jsourceLang, (jstring)jtargLang, (jstring)jsource);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JTranslator::Translate(): Exception"));
        return NULL;
    }

    QCC_DbgPrintf(("JTranslator::Translate(): Return"));

    if (NULL == jres) {
        return NULL;
    }

    const char* chars = env->GetStringUTFChars(jres, NULL);
    buffer.assign(chars);
    env->ReleaseStringUTFChars(jres, chars);

    return buffer.c_str();
}

template <typename T>
static jobject registerNativeSignalHandler(JNIEnv* env, jobject thiz, jstring jifaceName,
                                           jstring jsignalName, jobject jsignalHandler,
                                           jobject jmethod, jstring jancillary)
{
    QCC_DbgPrintf(("BusAttachment_registerNativeSignalHandler()"));

    JString ifaceName(jifaceName);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    JString signalName(jsignalName);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    JString ancillary(jancillary);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    const char* ancillarystr = NULL;
    if (ancillary.c_str() && ancillary.c_str()[0]) {
        ancillarystr = ancillary.c_str();
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_registerNativeSignalHandler(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_registerNativeSignalHandler(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_registerNativeSignalHandler(): Refcount on busPtr is %d", busPtr->GetRef()));

    QStatus status = busPtr->RegisterSignalHandler<T>(ifaceName.c_str(), signalName.c_str(), jsignalHandler, jmethod, ancillarystr);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_registerBusObject(): Exception"));
        return NULL;
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_registerNativeSignalHandlerWithSrcPath(JNIEnv* env, jobject thiz, jstring jifaceName,
                                                                                                    jstring jsignalName, jobject jsignalHandler,
                                                                                                    jobject jmethod, jstring jsource) {
    return registerNativeSignalHandler<JSignalHandlerWithSrc>(env, thiz, jifaceName, jsignalName, jsignalHandler, jmethod, jsource);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_registerNativeSignalHandlerWithRule(JNIEnv* env, jobject thiz, jstring jifaceName,
                                                                                                 jstring jsignalName, jobject jsignalHandler,
                                                                                                 jobject jmethod, jstring jsource) {
    return registerNativeSignalHandler<JSignalHandlerWithRule>(env, thiz, jifaceName, jsignalName, jsignalHandler, jmethod, jsource);
}


JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_unregisterSignalHandler(JNIEnv* env, jobject thiz, jobject jsignalHandler, jobject jmethod)
{
    QCC_DbgPrintf(("BusAttachment_unregisterSignalHandler()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_unregisterNativeSignalHandler(): Exception"));
        return;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_unregisterNativeSignalHandler(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return;
    }

    QCC_DbgPrintf(("BusAttachment_unregisterNativeSignalHandler(): Refcount on busPtr is %d", busPtr->GetRef()));

    busPtr->UnregisterSignalHandler(jsignalHandler, jmethod);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_getUniqueName(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("BusAttachment_getUniqueName()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getUniqueName(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getUniqueName(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_getUniqueName(): Refcount on busPtr is %d", busPtr->GetRef()));

    return env->NewStringUTF(busPtr->GetUniqueName().c_str());
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_getGlobalGUIDString(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("BusAttachment_getGlobalGUIDString()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getGlobalGUIDString(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getGlobalGUIDString(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_getGlobalGUIDString(): Refcount on busPtr is %d", busPtr->GetRef()));

    return env->NewStringUTF(busPtr->GetGlobalGUIDString().c_str());
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_clearKeyStore(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("BusAttachment_clearKeyStore()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_clearKeyStore(): Exception"));
        return;
    }

    if (busPtr == NULL) {
        return;
    }

    QCC_DbgPrintf(("BusAttachment_clearKeyStore(): Refcount on busPtr is %d", busPtr->GetRef()));

    busPtr->ClearKeyStore();
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_clearKeys(JNIEnv* env, jobject thiz, jstring jguid)
{
    QCC_DbgPrintf(("BusAttachment::clearKeys()"));

    /*
     * Load the C++ guid string from the java parameter
     */
    JString guid(jguid);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_clearKeys(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_clearKeys(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_clearKeys(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_clearKeys(): Refcount on busPtr is %d", busPtr->GetRef()));

    QCC_DbgPrintf(("BusAttachment_clearKeys(): Call ClearKeys(%s)", guid.c_str()));

    QStatus status = busPtr->ClearKeys(guid.c_str());

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_clearKeys(): ClearKeys() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_setKeyExpiration(JNIEnv* env, jobject thiz, jstring jguid, jint jtimeout)
{
    QCC_DbgPrintf(("BusAttachment::setKeyExpiration()"));

    /*
     * Load the C++ guid string from the java parameter
     */
    JString guid(jguid);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setKeyExpiration(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setKeyExpiration(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setKeyExpiration(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_setKeyExpiration(): Refcount on busPtr is %d", busPtr->GetRef()));

    QCC_DbgPrintf(("BusAttachment_setKeyExpiration(): Call SetKeyExpiration(%s, %d)", guid.c_str(), jtimeout));

    QStatus status = busPtr->SetKeyExpiration(guid.c_str(), jtimeout);

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_setKeyExpiration(): SetKeyExpiration() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_getKeyExpiration(JNIEnv* env, jobject thiz, jstring jguid, jobject jtimeout)
{
    QCC_DbgPrintf(("BusAttachment::getKeyExpiration()"));

    /*
     * Load the C++ guid string from the java parameter.
     */
    JString guid(jguid);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getKeyExpiration(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getKeyExpiration(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_getKeyExpiration(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_getKeyExpiration(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Make the AllJoyn call.
     */
    uint32_t timeout = 0;
    QCC_DbgPrintf(("BusAttachment_getKeyExpiration(): Call GetKeyExpiration(%s)", guid.c_str()));

    QStatus status = busPtr->GetKeyExpiration(guid.c_str(), timeout);

    QCC_DbgPrintf(("BusAttachment_getKeyExpiration(): Back from GetKeyExpiration(%s, %u)", guid.c_str(), timeout));

    /*
     * Locate the C++ timeout.  Note that the reference to the timeout is
     * passed in as an [out] parameter using a mutable object, so we are really
     * finding the field which we will write our found timeout reference into.
     */
    JLocalRef<jclass> clazz = env->GetObjectClass(jtimeout);
    jfieldID timeoutValueFid = env->GetFieldID(clazz, "value", "I");
    assert(timeoutValueFid);

    env->SetIntField(jtimeout, timeoutValueFid, timeout);

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_getKeyExpiration(): GetKeyExpiration() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_reloadKeyStore(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("BusAttachment_reloadKeyStore()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_reloadKeyStore(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_reloadKeyStore(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("BusAttachment_reloadKeyStore(): Refcount on busPtr is %d", busPtr->GetRef()));

    QCC_DbgPrintf(("BusAttachment_reloadKeyStore(): Call ReloadKeyStore()"));

    QStatus status = busPtr->ReloadKeyStore();

    if (status != ER_OK) {
        QCC_LogError(status, ("BusAttachment_reloadKeyStore(): ReloadKeyStore() fails"));
    }

    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_BusAttachment_getMessageContext(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("BusAttachment_getMessageContext()"));

    Message msg = MessageContext::GetMessage();

    JLocalRef<jstring> jobjectPath = env->NewStringUTF(msg->GetObjectPath());
    if (!jobjectPath) {
        return NULL;
    }

    JLocalRef<jstring> jinterfaceName = env->NewStringUTF(msg->GetInterface());
    if (!jinterfaceName) {
        return NULL;
    }

    JLocalRef<jstring> jmemberName = env->NewStringUTF(msg->GetMemberName());
    if (!jmemberName) {
        return NULL;
    }

    JLocalRef<jstring> jdestination = env->NewStringUTF(msg->GetDestination());
    if (!jdestination) {
        return NULL;
    }

    JLocalRef<jstring> jsender = env->NewStringUTF(msg->GetSender());
    if (!jsender) {
        return NULL;
    }

    JLocalRef<jstring> jsignature = env->NewStringUTF(msg->GetSignature());
    if (!jsignature) {
        return NULL;
    }

    JLocalRef<jstring> jauthMechanism = env->NewStringUTF(msg->GetAuthMechanism().c_str());
    if (!jauthMechanism) {
        return NULL;
    }

    SessionId sessionId = msg->GetSessionId();
    uint32_t serial = msg->GetCallSerial();

    jmethodID mid = env->GetMethodID(CLS_MessageContext, "<init>", "(ZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;I)V");
    if (!mid) {
        return NULL;
    }

    return env->NewObject(CLS_MessageContext, mid, msg->IsUnreliable(), (jstring)jobjectPath,
                          (jstring)jinterfaceName, (jstring)jmemberName, (jstring)jdestination,
                          (jstring)jsender, sessionId, (jstring)jsignature, (jstring)jauthMechanism,
                          serial);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_enableConcurrentCallbacks(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("BusAttachment_enableConcurrency()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_enableConcurrency(): Exception"));
        return;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_enableConcurrency(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return;
    }

    QCC_DbgPrintf(("BusAttachment_enableConcurrency(): Refcount on busPtr is %d", busPtr->GetRef()));

    busPtr->EnableConcurrentCallbacks();

    return;
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusAttachment_setDescriptionTranslator(
    JNIEnv*env, jobject thiz, jobject jtranslator)
{
    QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator()"));

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("BusAttachment_setDescriptionTranslator(): Exception"));
        return;
    }
    assert(busPtr);

    JTranslator* translator = NULL;
    if (jtranslator) {
        /*
         * We always take a strong global reference to the translator object.
         */
        QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator(): Taking strong global reference to Translator %p", jtranslator));
        jobject jglobalref = env->NewGlobalRef(jtranslator);
        if (!jglobalref) {
            return;
        }

        QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator(): Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();

        busPtr->translators.push_back(jglobalref);

        QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator(): Releasing Bus Attachment common lock"));
        busPtr->baCommonLock.Unlock();

        /*
         * Get the C++ object that must be there backing the Java object
         */
        translator = GetHandle<JTranslator*>(jtranslator);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("BusAttachment_setDescriptionTranslator(): Exception"));
            return;
        }

        assert(translator);
    }
    /*
     * Make the call into AllJoyn.
     */
    QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator(): Call SetDescriptionTranslator()"));
    busPtr->SetDescriptionTranslator(translator);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_InterfaceDescription_create(JNIEnv* env, jobject thiz, jobject jbus, jstring jname,
                                                                           jint securePolicy, jint numProps, jint numMembers)
{
    QCC_DbgPrintf(("InterfaceDescription_create()"));

    JString name(jname);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_create(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_create(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("InterfaceDescription_create(): Refcount on busPtr is %d", busPtr->GetRef()));

    InterfaceDescription* intf;
    QStatus status = busPtr->CreateInterface(name.c_str(), intf, static_cast<InterfaceSecurityPolicy>(securePolicy));
    if (ER_BUS_IFACE_ALREADY_EXISTS == status) {
        /*
         * We know that an interface exists with the same name, but it may differ in other parameters,
         * so confirm that other parameters are the same too before returning ER_OK.
         *
         * Note that we're not checking members or properties here, that check will be done later in
         * addMember and addProperty.
         */
        intf = (InterfaceDescription*)busPtr->GetInterface(name.c_str());
        assert(intf);
        if ((intf->GetSecurityPolicy() == static_cast<InterfaceSecurityPolicy>(securePolicy)) &&
            (intf->GetProperties() == (size_t)numProps) &&
            (intf->GetMembers() == (size_t)numMembers)) {
            status = ER_OK;
        }
        /*
         * When using org.freedesktop.DBus interfaces, we treat them as a special
         * case to remain backwards compatible. It cannot add the 'off' security
         * annotation. However, to work properly with object security, it must
         * still report its interface security as 'off'.
         */
        bool isDBusStandardIfac;
        if (name.c_str() == NULL) {     // passing NULL into strcmp is undefined behavior.
            isDBusStandardIfac = false;
        } else {
            isDBusStandardIfac = (strcmp(org::freedesktop::DBus::Introspectable::InterfaceName, name.c_str()) == 0) ||
                                 (strcmp(org::freedesktop::DBus::Peer::InterfaceName, name.c_str()) == 0) ||
                                 (strcmp(org::freedesktop::DBus::Properties::InterfaceName, name.c_str()) == 0) ||
                                 (strcmp(org::allseen::Introspectable::InterfaceName, name.c_str()) == 0);
        }
        if ((status != ER_OK) &&
            isDBusStandardIfac &&
            (intf->GetSecurityPolicy() == static_cast<InterfaceSecurityPolicy>(org_alljoyn_bus_InterfaceDescription_AJ_IFC_SECURITY_OFF))) {
            status = ER_OK;
        }
    }
    if (ER_OK == status) {
        SetHandle(thiz, intf);
    }

    if (env->ExceptionCheck()) {
        return NULL;
    } else {
        return JStatus(status);
    }
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_InterfaceDescription_addAnnotation(JNIEnv* env, jobject thiz, jstring jannotation, jstring jvalue)
{
    QCC_DbgPrintf(("InterfaceDescription_AddAnnotation()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_AddAnnotation(): Exception"));
        return NULL;
    }
    assert(intf);

    JString annotation(jannotation);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_AddAnnotation(): Exception"));
        return NULL;
    }

    JString value(jvalue);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_AddAnnotation(): Exception"));
        return NULL;
    }

    QStatus status = intf->AddAnnotation(annotation.c_str(), value.c_str());
    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_InterfaceDescription_addMember(JNIEnv* env, jobject thiz, jint type, jstring jname,
                                                                              jstring jinputSig, jstring joutSig, jint annotation, jstring jaccessPerm)
{
    QCC_DbgPrintf(("InterfaceDescription_addMember()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMember(): Exception"));
        return NULL;
    }
    assert(intf);

    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMember(): Exception"));
        return NULL;
    }

    JString inputSig(jinputSig);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMember(): Exception"));
        return NULL;
    }

    JString outSig(joutSig);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMember(): Exception"));
        return NULL;
    }

    JString accessPerm(jaccessPerm);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMember(): Exception"));
        return NULL;
    }

    QStatus status = intf->AddMember((AllJoynMessageType)type, name.c_str(), inputSig.c_str(), outSig.c_str(), NULL, annotation, accessPerm.c_str());
    if (ER_BUS_MEMBER_ALREADY_EXISTS == status || ER_BUS_INTERFACE_ACTIVATED == status) {
        /*
         * We know that a member exists with the same name, but check that the other parameters
         * are identical as well before returning ER_OK.  See also the comment in create above.
         */
        const InterfaceDescription::Member* member = intf->GetMember(name.c_str());
        if (member &&
            (member->memberType == (AllJoynMessageType)type) &&
            (name.c_str() && member->name == name.c_str()) &&
            (inputSig.c_str() && member->signature == inputSig.c_str()) &&
            (outSig.c_str() && member->returnSignature == outSig.c_str())) {

            // for reverse compatibility:
            // two annotations can be represented in the int variable 'annotation': DEPRECATED and NOREPLY
            // make sure these int values matches with what's in the full annotations map
            bool annotations_match = true;
            if (annotation & MEMBER_ANNOTATE_DEPRECATED) {
                qcc::String val;
                if (!member->GetAnnotation(org::freedesktop::DBus::AnnotateDeprecated, val) || val != "true") {
                    annotations_match = false;
                }
            }

            if (annotation & MEMBER_ANNOTATE_NO_REPLY) {
                qcc::String val;
                if (!member->GetAnnotation(org::freedesktop::DBus::AnnotateNoReply, val) || val != "true") {
                    annotations_match = false;
                }
            }

            if (annotations_match) {
                status = ER_OK;
            }
        }
    }
    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_InterfaceDescription_addMemberAnnotation(JNIEnv* env, jobject thiz,
                                                                                        jstring member, jstring annotation, jstring value)
{
    QCC_DbgPrintf(("InterfaceDescription_addMemberAnnotation()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMemberAnnotation(): Exception"));
        return NULL;
    }
    assert(intf);

    JString jName(member);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMemberAnnotation(): Exception"));
        return NULL;
    }

    JString jAnnotation(annotation);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMemberAnnotation(): Exception"));
        return NULL;
    }

    JString jValue(value);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addMemberAnnotation(): Exception"));
        return NULL;
    }

    QStatus status = intf->AddMemberAnnotation(jName.c_str(), jAnnotation.c_str(), jValue.c_str());
    return JStatus(status);
}


JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_InterfaceDescription_addProperty(JNIEnv* env, jobject thiz, jstring jname,
                                                                                jstring jsignature, jint access, jint annotation)
{
    QCC_DbgPrintf(("InterfaceDescription_addProperty()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addProperty(): Exception"));
        return NULL;
    }
    assert(intf);

    JString name(jname);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addProperty(): Exception"));
        return NULL;
    }

    JString signature(jsignature);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addProperty(): Exception"));
        return NULL;
    }

    QStatus status = intf->AddProperty(name.c_str(), signature.c_str(), access);
    if (ER_BUS_PROPERTY_ALREADY_EXISTS == status || ER_BUS_INTERFACE_ACTIVATED == status) {
        /*
         * We know that a property exists with the same name, but check that the other parameters
         * are identical as well before returning ER_OK.  See also the comment in create above.
         */
        const InterfaceDescription::Property* prop = intf->GetProperty(name.c_str());
        if (prop &&
            (name.c_str() && prop->name == name.c_str()) &&
            (signature.c_str() && prop->signature == signature.c_str()) &&
            (prop->access == access)) {

            // for reverse compatibility:
            // two annotations can be represented in the int variable 'annotation': EMIT_CHANGED_SIGNAL and EMIT_CHANGED_SIGNAL_INVALIDATES
            // make sure these int values matches with what's in the full annotations map
            bool annotations_match = true;
            if (annotation & PROP_ANNOTATE_EMIT_CHANGED_SIGNAL) {
                qcc::String val;
                if (!prop->GetAnnotation(org::freedesktop::DBus::AnnotateEmitsChanged, val) || val != "true") {
                    annotations_match = false;
                }
            }

            if (annotation & PROP_ANNOTATE_EMIT_CHANGED_SIGNAL_INVALIDATES) {
                qcc::String val;
                if (!prop->GetAnnotation(org::freedesktop::DBus::AnnotateEmitsChanged, val) || val != "invalidates") {
                    annotations_match = false;
                }
            }

            if (annotations_match) {
                status = ER_OK;
            }
        }
    }
    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_InterfaceDescription_addPropertyAnnotation(JNIEnv* env, jobject thiz,
                                                                                          jstring property, jstring annotation, jstring value)
{
    QCC_DbgPrintf(("InterfaceDescription_addPropertyAnnotation()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addPropertyAnnotation(): Exception"));
        return NULL;
    }
    assert(intf);

    JString jName(property);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addPropertyAnnotation(): Exception"));
        return NULL;
    }

    JString jAnnotation(annotation);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addPropertyAnnotation(): Exception"));
        return NULL;
    }

    JString jValue(value);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_addPropertyAnnotation(): Exception"));
        return NULL;
    }

    QStatus status = intf->AddPropertyAnnotation(jName.c_str(), jAnnotation.c_str(), jValue.c_str());
    return JStatus(status);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_InterfaceDescription_setDescriptionLanguage(
    JNIEnv*env, jobject thiz, jstring language)
{
    QCC_DbgPrintf(("InterfaceDescription_setDescriptionLanguage()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setDescriptionLanguage(): Exception"));
        return;
    }
    assert(intf);

    JString jlanguage(language);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setDescriptionLanguage(): Exception"));
        return;
    }

    intf->SetDescriptionLanguage(jlanguage.c_str());
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_InterfaceDescription_setDescription(
    JNIEnv*env, jobject thiz, jstring description)
{
    QCC_DbgPrintf(("InterfaceDescription_setDescsription()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setDescription(): Exception"));
        return;
    }
    assert(intf);

    JString jdescription(description);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setDescription(): Exception"));
        return;
    }

    intf->SetDescription(jdescription.c_str());
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_InterfaceDescription_setDescriptionTranslator(
    JNIEnv*env, jobject thiz, jobject jbus, jobject jtranslator)
{
    QCC_DbgPrintf(("InterfaceDescription_setDescriptionTranslator()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setDescriptionTranslator(): Exception"));
        return;
    }
    assert(intf);

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setDescriptionTranslator(): Exception"));
        return;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setDescriptionTranslator(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return;
    }

    JTranslator* translator = NULL;
    if (jtranslator) {
        /*
         * We always take a strong global reference to the translator object.
         */
        QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator(): Taking strong global reference to Translator %p", jtranslator));
        jobject jglobalref = env->NewGlobalRef(jtranslator);
        if (!jglobalref) {
            return;
        }

        QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator(): Taking Bus Attachment common lock"));
        busPtr->baCommonLock.Lock();

        busPtr->translators.push_back(jglobalref);

        QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator(): Releasing Bus Attachment common lock"));
        busPtr->baCommonLock.Unlock();

        /*
         * Get the C++ object that must be there backing the Java object
         */
        translator = GetHandle<JTranslator*>(jtranslator);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("BusAttachment_setDescriptionTranslator(): Exception"));
            return;
        }

        assert(translator);
    }
    /*
     * Make the call into AllJoyn.
     */
    QCC_DbgPrintf(("BusAttachment_setDescriptionTranslator(): Call SetDescriptionTranslator()"));
    intf->SetDescriptionTranslator(translator);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_InterfaceDescription_setMemberDescription(JNIEnv*env, jobject thiz,
                                                                                         jstring jmember, jstring jdesc, jboolean isSessionless)
{
    QCC_DbgPrintf(("InterfaceDescription_setMemberDescription()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setMemberDescription(): Exception"));
        return NULL;
    }
    assert(intf);

    JString member(jmember);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setMemberDescription(): Exception"));
        return NULL;
    }

    JString desc(jdesc);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setMemberDescription(): Exception"));
        return NULL;
    }

    QStatus status = intf->SetMemberDescription(member.c_str(), desc.c_str(), isSessionless);
    return JStatus(status);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_InterfaceDescription_setPropertyDescription(
    JNIEnv*env, jobject thiz, jstring jpropName, jstring jdesc)
{
    QCC_DbgPrintf(("InterfaceDescription_setPropertyDescription()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setPropertyDescription(): Exception"));
        return NULL;
    }
    assert(intf);

    JString propName(jpropName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setPropertyDescription(): Exception"));
        return NULL;
    }

    JString desc(jdesc);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_setPropertyDescription(): Exception"));
        return NULL;
    }

    QStatus status = intf->SetPropertyDescription(propName.c_str(), desc.c_str());
    return JStatus(status);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_InterfaceDescription_activate(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("InterfaceDescription_activate()"));

    InterfaceDescription* intf = GetHandle<InterfaceDescription*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("InterfaceDescription_activate(): Exception"));
        return;
    }

    assert(intf);

    intf->Activate();
}

static QStatus AddInterfaceStatus(jobject thiz, JBusAttachment* busPtr, jstring jinterfaceName)
{
    JNIEnv* env = GetEnv();

    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("AddInterface(): Exception"));
        return ER_FAIL;
    }

    assert(proxyBusObj);

    JString interfaceName(jinterfaceName);
    if (env->ExceptionCheck()) {
        return ER_FAIL;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(thiz);
    jmethodID mid = env->GetMethodID(clazz, "addInterface", "(Ljava/lang/String;)I");
    if (!mid) {
        return ER_FAIL;
    }

    QStatus status = (QStatus)env->CallIntMethod(thiz, mid, jinterfaceName);
    if (env->ExceptionCheck()) {
        /* AnnotationBusException */
        QCC_LogError(ER_FAIL, ("AddInterface(): Exception"));
        return ER_FAIL;
    }

    if (ER_OK != status) {
        QCC_LogError(status, ("AddInterface(): Exception"));
        return status;
    }

    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("AddInterface(): NULL bus pointer"));
        return ER_FAIL;
    }

    QCC_DbgPrintf(("AddInterface(): Refcount on busPtr is %d", busPtr->GetRef()));

    const InterfaceDescription* intf = busPtr->GetInterface(interfaceName.c_str());
    assert(intf);

    status = proxyBusObj->AddInterface(*intf);
    return status;
}

static void AddInterface(jobject thiz, JBusAttachment* busPtr, jstring jinterfaceName)
{
    JNIEnv* env = GetEnv();

    QStatus status = AddInterfaceStatus(thiz, busPtr, jinterfaceName);
    if (env->ExceptionCheck()) {
        return;
    }
    if (status != ER_OK) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}


JProxyBusObject::JProxyBusObject(jobject pbo, JBusAttachment* jbap, const char* endpoint, const char* path, SessionId sessionId, bool secure)
    : ProxyBusObject(*jbap, endpoint, path, sessionId, secure), jpbo(NULL)
{
    QCC_DbgPrintf(("JProxyBusObject::JProxyBusObject()"));

    /*
     * We need to ensure that the underlying Bus Attachment is alive as long
     * as we are.  We do this by bumping the reference count there.
     */
    busPtr = jbap;
    assert(busPtr);
    QCC_DbgPrintf(("JProxyBusObject::JProxyBusObject(): Refcount on busPtr before is %d", busPtr->GetRef()));
    busPtr->IncRef();
    QCC_DbgPrintf(("JProxyBusObject::JProxyBusObject(): Refcount on busPtr after is %d", busPtr->GetRef()));

    JNIEnv* env = GetEnv();
    jpbo = env->NewWeakGlobalRef(pbo);
}

JProxyBusObject::~JProxyBusObject()
{
    QCC_DbgPrintf(("JProxyBusObject::~JProxyBusObject()"));

    /*
     * We have a hold on the underlying Bus Attachment, but we need it until the BusObject destructor
     * has been run.  We inherit from it, so it will run after our destructor.  This means we can't
     * drop the reference count ourselves, but we have to rely on the object that called delete on
     * us.
     */
    assert(busPtr);
    QCC_DbgPrintf(("JProxyBusObject::~JProxyBusObject(): Refcount on busPtr at destruction is %d", busPtr->GetRef()));

    JNIEnv* env = GetEnv();
    env->DeleteWeakGlobalRef(jpbo);
}

JPropertiesChangedListener::JPropertiesChangedListener(jobject jobj, jobject jch, jobject jinval)
    : jlistener(NULL), jchangedType(NULL), jinvalidatedType(NULL)
{
    JNIEnv* env = GetEnv();
    jlistener = env->NewWeakGlobalRef(jobj);
    jchangedType = env->NewGlobalRef(jch);
    jinvalidatedType = env->NewGlobalRef(jinval);
}

JPropertiesChangedListener::~JPropertiesChangedListener()
{
    JNIEnv* env = GetEnv();
    env->DeleteWeakGlobalRef(jlistener);
    env->DeleteGlobalRef(jchangedType);
    env->DeleteGlobalRef(jinvalidatedType);
}

void JPropertiesChangedListener::PropertiesChanged(ProxyBusObject& obj, const char* ifaceName, const MsgArg& changed, const MsgArg& invalidated, void* context)
{
    QCC_DbgPrintf(("JPropertiesChangedListener::PropertiesChanged()"));

    /*
     * JScopedEnv will automagically attach the JVM to the current native
     * thread.
     */
    JScopedEnv env;

    /*
     * Translate the C++ formal parameters into their JNI counterparts.
     */
    JLocalRef<jstring> jifaceName = env->NewStringUTF(ifaceName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JPropertiesChangedListener::PropertiesChanged(): Exception extracting interface"));
        return;
    }

    JLocalRef<jobjectArray> jchanged = (jobjectArray)Unmarshal(&changed, jchangedType);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JPropertiesChangedListener::PropertiesChanged(): Exception extracting changed dictionary"));
        return;
    }

    JLocalRef<jobjectArray> jinvalidated = (jobjectArray)Unmarshal(&invalidated, jinvalidatedType);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("JPropertiesChangedListener::PropertiesChanged(): Exception extracting invalidated list"));
        return;
    }

    /*
     * The weak global reference jbusListener cannot be directly used.  We have to get
     * a "hard" reference to it and then use that.  If you try to use a weak reference
     * directly you will crash and burn.
     */
    jobject jo = env->NewLocalRef(jlistener);
    if (!jo) {
        QCC_LogError(ER_FAIL, ("JPropertiesChangedListener::PropertiesChanged(): Can't get new local reference to ProxyBusObjectListener"));
        return;
    }

    JLocalRef<jclass> clazz = env->GetObjectClass(jo);
    jmethodID mid = env->GetMethodID(clazz, "propertiesChanged",
                                     "(Lorg/alljoyn/bus/ProxyBusObject;Ljava/lang/String;Ljava/util/Map;[Ljava/lang/String;)V");
    if (!mid) {
        QCC_LogError(ER_FAIL, ("JPropertiesChangedListener::PropertiesChanged(): Can't get new local reference to ProxyBusObjectListener property changed handler method"));
        return;
    }

    /*
     * This call out to the property changed handler implies that the Java method must be
     * MT-safe.  This is implied by the definition of the listener.
     */
    jobject pbo = env->NewLocalRef(((JProxyBusObject &)obj).jpbo);

    if (pbo) {
        QCC_DbgPrintf(("JPropertiesChangedListener::PropertiesChanged(): Call out to listener object and method"));
        env->CallVoidMethod(jo, mid, pbo, (jstring)jifaceName, (jobjectArray)jchanged, (jobjectArray)jinvalidated);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("JPropertiesChangedListener::PropertiesChanged(): Exception"));
            return;
        }
    }
    QCC_DbgPrintf(("JPropertiesChangedListener::PropertiesChanged(): Return"));
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_ProxyBusObject_create(JNIEnv* env, jobject thiz, jobject jbus,
                                                                  jstring jbusName, jstring jobjPath,
                                                                  jint sessionId, jboolean secure)
{
    QCC_DbgPrintf(("ProxyBusObject_create()"));

    JString busName(jbusName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObject_create(): Exception"));
        return;
    }

    JString objPath(jobjPath);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObject_create(): Exception"));
        return;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObject_create(): Exception"));
        return;
    }

    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("ProxyBusObject_create(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return;
    }

    QCC_DbgPrintf(("ProxyBusObject_create(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * Create a C++ proxy bus object to back the Java bus object and stash the
     * pointer to it in our "handle"  Note that we are giving the busPtr to the
     * new JProxyBusObject, so it must bump the reference count
     */
    JProxyBusObject* jpbo = new JProxyBusObject(thiz, busPtr, busName.c_str(), objPath.c_str(), sessionId, secure);
    if (!jpbo) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }
    QCC_DbgPrintf(("ProxyBusObject_create(): Refcount on busPtr now %d", busPtr->GetRef()));

    SetHandle(thiz, jpbo);
    if (env->ExceptionCheck()) {
        delete jpbo;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_ProxyBusObject_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("ProxyBusObject_destroy()"));

    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (!proxyBusObj) {
        QCC_DbgPrintf(("ProxyBusObject_destroy(): Already destroyed. Returning."));
        return;
    }

    if (proxyBusObj == NULL) {
        QCC_LogError(ER_FAIL, ("ProxyBusObject_destroy(): NULL bus object pointer"));
        return;
    }

    QCC_DbgPrintf(("ProxyBusObject_destroy(): Refcount on busPtr now %d", proxyBusObj->busPtr->GetRef()));

    /*
     * We need to delete the JProxyBusObject.  It is holding pointer to the
     * reference counted bus attachment so one would think that when it is
     * destroyed, the destructor should call DecRef() on it.  The problem is
     * taht it is a base class that is actually using the reference to the bus
     * attachment, so if we delete it in the destructor, the base class crashes
     * when it doesn't have it.  So we have to help the JProxyBusObject
     * destructor out and delete what should be its reference for it after the
     * base class (BusObject) finishes its destruction process.
     */
    JBusAttachment* busPtr = proxyBusObj->busPtr;
    delete proxyBusObj;
    QCC_DbgPrintf(("ProxyBusObject_destroy(): Refcount on busPtr before decrement is %d", busPtr->GetRef()));
    busPtr->DecRef();
    SetHandle(thiz, NULL);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_ProxyBusObject_registerPropertiesChangedListener(JNIEnv* env,
                                                                                                jobject thiz,
                                                                                                jstring jifaceName,
                                                                                                jobjectArray jproperties,
                                                                                                jobject jpropertiesChangedListener)
{
    QCC_DbgPrintf(("ProxyBusObject_registerPropertiesChangedListener()"));

    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (env->ExceptionCheck() || !proxyBusObj) {
        return NULL;
    }

    JString ifaceName(jifaceName);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    size_t numProps = env->GetArrayLength(jproperties);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    JPropertiesChangedListener* listener = GetHandle<JPropertiesChangedListener*>(jpropertiesChangedListener);
    if (env->ExceptionCheck() || !listener) {
        return NULL;
    }

    QStatus status;
    jobject jstatus = NULL;

    if (!proxyBusObj->ImplementsInterface(ifaceName.c_str())) {
        status = AddInterfaceStatus(thiz, proxyBusObj->busPtr, jifaceName);
        if (env->ExceptionCheck()) {
            QCC_LogError(ER_FAIL, ("ProxyBusObject_registerPropertiesChangedListener(): Exception"));
            return NULL;
        }
        if (status != ER_OK) {
            jstatus = JStatus(status);
            return jstatus;
        }
    }

    const char** props = new const char*[numProps];
    jstring* jprops = new jstring[numProps];
    memset(props, 0, numProps * sizeof(props[0]));
    memset(jprops, 0, numProps * sizeof(jprops[0]));

    for (size_t i = 0; i < numProps; ++i) {
        jprops[i] = (jstring)GetObjectArrayElement(env, jproperties, i);
        if (env->ExceptionCheck()) {
            goto exit;
        }
        props[i] = env->GetStringUTFChars(jprops[i], NULL);
        if (env->ExceptionCheck()) {
            goto exit;
        }
    }

    status = proxyBusObj->RegisterPropertiesChangedListener(ifaceName.c_str(), props, numProps, *listener, NULL);
    jstatus = JStatus(status);

exit:
    for (size_t i = 0; i < numProps; ++i) {
        if (props[i]) {
            env->ReleaseStringUTFChars(jprops[i], props[i]);
        }
    }
    delete [] props;
    delete [] jprops;
    return jstatus;
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_ProxyBusObject_unregisterPropertiesChangedListener(JNIEnv* env,
                                                                                                  jobject thiz,
                                                                                                  jstring jifaceName,
                                                                                                  jobject jpropertiesChangedListener)
{
    QCC_DbgPrintf(("ProxyBusObject_unregisterPropertiesChangedListener()"));

    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (env->ExceptionCheck() || !proxyBusObj) {
        return NULL;
    }

    JString ifaceName(jifaceName);
    if (env->ExceptionCheck()) {
        return NULL;
    }

    JPropertiesChangedListener* listener = GetHandle<JPropertiesChangedListener*>(jpropertiesChangedListener);
    if (env->ExceptionCheck() || !listener) {
        return NULL;
    }

    QStatus status = proxyBusObj->UnregisterPropertiesChangedListener(ifaceName.c_str(), *listener);

    return JStatus(status);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_PropertiesChangedListener_create(JNIEnv* env,
                                                                             jobject thiz,
                                                                             jobject jchanged,
                                                                             jobject jinvalidated)
{
    QCC_DbgPrintf(("PropertiesChangedListener_create()"));

    assert(GetHandle<JPropertiesChangedListener*>(thiz) == NULL);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("PropertiesChangedListener_create(): Exception"));
        return;
    }

    QCC_DbgPrintf(("PropertiesChangedListener_create(): Create backing object"));
    JPropertiesChangedListener* jojcl = new JPropertiesChangedListener(thiz, jchanged, jinvalidated);
    if (jojcl == NULL) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    QCC_DbgPrintf(("PropertiesChangedListener_create(): Set handle to %p", jojcl));
    SetHandle(thiz, jojcl);
    if (env->ExceptionCheck()) {
        delete jojcl;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_PropertiesChangedListener_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("PropertiesChangedListener_destroy()"));

    JPropertiesChangedListener* jojcl = GetHandle<JPropertiesChangedListener*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("PropertiesChangedListener_destroy(): Exception"));
        return;
    }

    assert(jojcl);
    delete jojcl;

    SetHandle(thiz, NULL);
    return;
}

/*
 * if the interface security policy is Required return true,
 * if the interface security policy is off return false
 * otherwise return the object security.
 */
static inline bool SecurityApplies(const JProxyBusObject* obj, const InterfaceDescription* ifc)
{
    InterfaceSecurityPolicy ifcSec = ifc->GetSecurityPolicy();
    if (ifcSec == AJ_IFC_SECURITY_REQUIRED) {
        return true;
    } else if (ifcSec == AJ_IFC_SECURITY_OFF) {
        return false;
    } else {
        return obj->IsSecure();
    }
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_ProxyBusObject_methodCall(JNIEnv* env,
                                                                         jobject thiz,
                                                                         jobject jbus,
                                                                         jstring jinterfaceName,
                                                                         jstring jmethodName,
                                                                         jstring jinputSig,
                                                                         jobject joutType,
                                                                         jobjectArray jargs,
                                                                         jint replyTimeoutMsecs,
                                                                         jint flags)
{
    QCC_DbgPrintf(("ProxyBusObject_methodCall()"));

    JString interfaceName(jinterfaceName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_methodCall(): Exception"));
        return NULL;
    }

    JString methodName(jmethodName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_methodCall(): Exception"));
        return NULL;
    }

    JString inputSig(jinputSig);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_methodCall(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_methodCall(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_methodCall(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("ProxybusObject_methodCall(): Refcount on busPtr is %d", busPtr->GetRef()));

    /*
     * This part of the binding and on down lower is fundamentally single
     * threaded.  We want to eventually support multiple overlapping synchronous
     * calls, but we do not support this now.
     *
     * It might sound reasonable for a user of the bindings to get around this
     * limitation by spinning up a bunch of threads to make overlapping
     * synchronous method calls.  Since these calls will be coming in here to be
     * dispatched, We have to actively prevent this from happening for now.
     *
     * It's a bit of a blunt instrument, but we acquire a common method call lock
     * in the underlying bus attachment before allowing any method call on a
     * proxy bus object to proceed.
     */
    busPtr->baProxyLock.Lock();

    Message replyMsg(*busPtr);

    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (env->ExceptionCheck()) {
        busPtr->baProxyLock.Unlock();
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_methodCall(): Exception"));
        return NULL;
    }

    assert(proxyBusObj);

    const InterfaceDescription::Member* member = NULL;

    const InterfaceDescription* intf = proxyBusObj->GetInterface(interfaceName.c_str());
    if (!intf) {
        AddInterface(thiz, busPtr, jinterfaceName);
        if (env->ExceptionCheck()) {
            busPtr->baProxyLock.Unlock();
            QCC_LogError(ER_FAIL, ("ProxyBusObjexct_methodCall(): Exception"));
            return NULL;
        }
        intf = proxyBusObj->GetInterface(interfaceName.c_str());
        assert(intf);
    }

    member = intf->GetMember(methodName.c_str());
    if (!member) {
        busPtr->baProxyLock.Unlock();
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_BUS_INTERFACE_NO_SUCH_MEMBER));
        return NULL;
    }

    busPtr->baProxyLock.Unlock();

    MsgArg args;
    QStatus status;
    const MsgArg* replyArgs;
    size_t numReplyArgs;
    jobject jreplyArg = NULL;

    if (!Marshal(inputSig.c_str(), jargs, &args)) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_methodCall(): Marshal failure"));
        return jreplyArg;
    }

    /*
     * If we call any method on the org.freedesktop.DBus.Properties interface
     *    - org.freedesktop.DBus.Properties.Get(ssv)
     *    - org.freedesktop.DBus.Properties.Set(ssv)
     *    - org.freedesktop.DBus.Properties.GetAll(sa{sv})
     * If the properties are part of an encrypted interface then the they must
     * also be encrypted.  The first parameter of Get, Set, and GetAll is the
     * interface name that the property belongs to.
     *    - this code reads the interface name from the Properties method call
     *    - tries to Get the InterfaceDescription from the proxyBusObj based on
     *      the interface name
     *    - Checks the InterfaceDescription to see if it has Security Annotation
     *      or object security
     *    - if security is set change the security flag to for the property
     *      method so the properties are encrypted.
     *    - if it is unable to get the InterfaceDescription it will check the
     *      security of the ProxyObject.
     *    - Failure to find a security indication will result the properties
     *      methods being used without encryption.
     */
    if (interfaceName.c_str() != NULL) {     //if interfaceName.c_str() is null strcmp is undefined behavior
        if (strcmp(interfaceName.c_str(), org::freedesktop::DBus::Properties::InterfaceName) == 0) {
            char* interface_name;
            /* the fist member of the struct is the interface name*/
            args.v_struct.members[0].Get("s", &interface_name);
            const InterfaceDescription* ifac_with_property = proxyBusObj->GetInterface(interface_name);
            /*
             * If the object or the property interface is secure method call
             * must be encrypted.
             */
            if (ifac_with_property == NULL) {
                if (proxyBusObj->IsSecure()) {
                    flags |= ALLJOYN_FLAG_ENCRYPTED;
                }
            } else
            if (SecurityApplies(proxyBusObj, ifac_with_property)) {
                flags |= ALLJOYN_FLAG_ENCRYPTED;
            }
        }
    }
    qcc::String val;
    if (member->GetAnnotation(org::freedesktop::DBus::AnnotateNoReply, val) && val == "true") {
        status = proxyBusObj->MethodCallAsync(*member, NULL, NULL, args.v_struct.members,
                                              args.v_struct.numMembers, NULL, replyTimeoutMsecs, flags);
        if (ER_OK != status) {
            env->ThrowNew(CLS_BusException, QCC_StatusText(status));
        }
    } else {
        status = proxyBusObj->MethodCall(*member, args.v_struct.members, args.v_struct.numMembers,
                                         replyMsg, replyTimeoutMsecs, flags);
        if (ER_OK == status) {
            replyMsg->GetArgs(numReplyArgs, replyArgs);
            if (numReplyArgs > 1) {
                MsgArg structArg(ALLJOYN_STRUCT);
                structArg.v_struct.numMembers = numReplyArgs;
                structArg.v_struct.members = new MsgArg[numReplyArgs];
                for (size_t i = 0; i < numReplyArgs; ++i) {
                    structArg.v_struct.members[i] = replyArgs[i];
                }
                structArg.SetOwnershipFlags(MsgArg::OwnsArgs);
                jreplyArg = Unmarshal(&structArg, joutType);
            } else if (numReplyArgs > 0) {
                jreplyArg = Unmarshal(&replyArgs[0], joutType);
            }
        } else if (ER_BUS_REPLY_IS_ERROR_MESSAGE == status) {
            String errorMessage;
            const char* errorName = replyMsg->GetErrorName(&errorMessage);
            if (errorName) {
                if (!strcmp("org.alljoyn.bus.BusException", errorName)) {
                    env->ThrowNew(CLS_BusException, errorMessage.c_str());
                } else {
                    ThrowErrorReplyBusException(errorName, errorMessage.c_str());
                }
            } else {
                env->ThrowNew(CLS_BusException, QCC_StatusText(status));
            }
        } else {
            env->ThrowNew(CLS_BusException, QCC_StatusText(status));
        }
    }

    if (env->ExceptionCheck()) {
        return NULL;
    } else {
        return jreplyArg;
    }
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_ProxyBusObject_getProperty(JNIEnv* env, jobject thiz, jobject jbus,
                                                                          jstring jinterfaceName, jstring jpropertyName)
{
    QCC_DbgPrintf(("ProxyBusObject_getProperty()"));

    JString interfaceName(jinterfaceName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getProperty(): Exception"));
        return NULL;
    }

    JString propertyName(jpropertyName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getProperty(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getProperty(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getProperty(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("ProxybusObject_getproperty(): Refcount on busPtr is %d\n", busPtr->GetRef()));

    /*
     * This part of the binding and on down lower is fundamentally single
     * threaded.  We want to eventually support multiple overlapping synchronous
     * calls, but we do not support this now.
     *
     * It might sound reasonable for a user of the bindings to get around this
     * limitation by spinning up a bunch of threads to make overlapping get
     * property calls.  Since these calls will be coming in here to be
     * dispatched, We have to actively prevent this from happening for now.
     *
     * It's a bit of a blunt instrument, but we acquire a common method call lock
     * in the underlying bus attachment before allowing any method call on a
     * proxy bus object to proceed.
     */
    busPtr->baProxyLock.Lock();

    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (env->ExceptionCheck()) {
        busPtr->baProxyLock.Unlock();
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getProperty(): Exception"));
        return NULL;
    }

    assert(proxyBusObj);

    if (!proxyBusObj->ImplementsInterface(interfaceName.c_str())) {
        AddInterface(thiz, busPtr, jinterfaceName);
        if (env->ExceptionCheck()) {
            busPtr->baProxyLock.Unlock();
            QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getProperty(): Exception"));
            return NULL;
        }
    }

    MsgArg value;
    QStatus status = proxyBusObj->GetProperty(interfaceName.c_str(), propertyName.c_str(), value);
    if (ER_OK == status) {
        jobject obj = Unmarshal(&value, CLS_Variant);
        busPtr->baProxyLock.Unlock();
        return obj;
    } else {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getProperty(): Exception"));
        busPtr->baProxyLock.Unlock();
        env->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_ProxyBusObject_getAllProperties(JNIEnv* env, jobject thiz, jobject jbus,
                                                                               jobject joutType, jstring jinterfaceName)
{
    QCC_DbgPrintf(("ProxyBusObject_getAllProperties()"));

    JString interfaceName(jinterfaceName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getAllProperties(): Exception"));
        return NULL;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getAllProperties(): Exception"));
        return NULL;
    }

    /*
     * We don't want to force the user to constantly check for NULL return
     * codes, so if we have a problem, we throw an exception.
     */
    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getAllProperties(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return NULL;
    }

    QCC_DbgPrintf(("ProxybusObject_getproperty(): Refcount on busPtr is %d\n", busPtr->GetRef()));

    /*
     * This part of the binding and on down lower is fundamentally single
     * threaded.  We want to eventually support multiple overlapping synchronous
     * calls, but we do not support this now.
     *
     * It might sound reasonable for a user of the bindings to get around this
     * limitation by spinning up a bunch of threads to make overlapping get
     * property calls.  Since these calls will be coming in here to be
     * dispatched, We have to actively prevent this from happening for now.
     *
     * It's a bit of a blunt instrument, but we acquire a common method call lock
     * in the underlying bus attachment before allowing any method call on a
     * proxy bus object to proceed.
     */
    busPtr->baProxyLock.Lock();

    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (env->ExceptionCheck()) {
        busPtr->baProxyLock.Unlock();
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getAllProperties(): Exception"));
        return NULL;
    }

    assert(proxyBusObj);

    if (!proxyBusObj->ImplementsInterface(interfaceName.c_str())) {
        AddInterface(thiz, busPtr, jinterfaceName);
        if (env->ExceptionCheck()) {
            busPtr->baProxyLock.Unlock();
            QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getAllProperties(): Exception"));
            return NULL;
        }
    }

    MsgArg value;
    QStatus status = proxyBusObj->GetAllProperties(interfaceName.c_str(), value);
    if (ER_OK == status) {
        jobject obj = Unmarshal(&value, joutType);
        busPtr->baProxyLock.Unlock();
        return obj;
    } else {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getAllProperties(): Exception"));
        busPtr->baProxyLock.Unlock();
        env->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_ProxyBusObject_setProperty(JNIEnv* env,
                                                                       jobject thiz,
                                                                       jobject jbus,
                                                                       jstring jinterfaceName,
                                                                       jstring jpropertyName,
                                                                       jstring jsignature,
                                                                       jobject jvalue)
{
    QCC_DbgPrintf(("ProxyBusObject_setProperty()"));

    JString interfaceName(jinterfaceName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_setProperty(): Exception"));
        return;
    }

    JString propertyName(jpropertyName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_setProperty(): Exception"));
        return;
    }

    JString signature(jsignature);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_setProperty(): Exception"));
        return;
    }

    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_getProperty(): Exception"));
        return;
    }

    if (busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_setProperty(): NULL bus pointer"));
        return;
    }

    QCC_DbgPrintf(("ProxybusObject_setproperty(): Refcount on busPtr is %d\n", busPtr->GetRef()));

    /*
     * This part of the binding and on down lower is fundamentally single
     * threaded.  We want to eventually support multiple overlapping synchronous
     * calls, but we do not support this now.
     *
     * It might sound reasonable for a user of the bindings to get around this
     * limitation by spinning up a bunch of threads to make overlapping set
     * property calls.  Since these calls will be coming in here to be
     * dispatched, We have to actively prevent this from happening for now.
     *
     * It's a bit of a blunt instrument, but we acquire a common method call lock
     * in the underlying bus attachment before allowing any method call on a
     * proxy bus object to proceed.
     */
    busPtr->baProxyLock.Lock();

    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (env->ExceptionCheck()) {
        busPtr->baProxyLock.Unlock();
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_setProperty(): Exception"));
        return;
    }

    assert(proxyBusObj);

    if (!proxyBusObj->ImplementsInterface(interfaceName.c_str())) {
        AddInterface(thiz, busPtr, jinterfaceName);
        if (env->ExceptionCheck()) {
            busPtr->baProxyLock.Unlock();
            QCC_LogError(ER_FAIL, ("ProxyBusObjexct_setProperty(): Exception"));
            return;
        }
    }

    MsgArg value;
    QStatus status;
    if (Marshal(signature.c_str(), jvalue, &value)) {
        status = proxyBusObj->SetProperty(interfaceName.c_str(), propertyName.c_str(), value);
    } else {
        status = ER_FAIL;
    }
    if (ER_OK != status) {
        QCC_LogError(ER_FAIL, ("ProxyBusObjexct_setProperty(): Exception"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
    busPtr->baProxyLock.Unlock();
}

JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_ProxyBusObject_isProxyBusObjectSecure(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("ProxyBusObject_isSecure()"));
    JProxyBusObject* proxyBusObj = GetHandle<JProxyBusObject*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("ProxyBusObject_isSecure(): Exception"));
        return false;
    }

    if (proxyBusObj == NULL) {
        QCC_LogError(ER_FAIL, ("ProxyBusObject_isSecure(): NULL bus pointer"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_FAIL));
        return false;
    }
    return proxyBusObj->IsSecure();
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_SignalEmitter_signal(JNIEnv* env, jobject thiz, jobject jbusObject, jstring jdestination,
                                                                 jint sessionId, jstring jifaceName, jstring jsignalName,
                                                                 jstring jinputSig, jobjectArray jargs, jint timeToLive, jint flags,
                                                                 jobject jmsgContext)
{
    QCC_DbgPrintf(("SignalEmitter_signal()"));

    JString destination(jdestination);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("SignalEmitter_signal(): Exception"));
        return;
    }

    JString ifaceName(jifaceName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("SignalEmitter_signal(): Exception"));
        return;
    }

    JString signalName(jsignalName);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("SignalEmitter_signal(): Exception"));
        return;
    }

    JString inputSig(jinputSig);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("SignalEmitter_signal(): Exception"));
        return;
    }

    MsgArg args;
    if (!Marshal(inputSig.c_str(), jargs, &args)) {
        QCC_LogError(ER_FAIL, ("SignalEmitter_signal(): Marshal() error"));
        return;
    }

    /*
     * We have to find the C++ object that backs our Java Bus Object.  Since we are
     * provided a Java Bus Object reference here, there should be a corresponding
     * C++ backing object stored in the global map of such things.  Just because we
     * think it should be there doesn't mean that the client is playing games that
     * would mess us up.  For example, she could call signal on one thread and also
     * "simultaneously" call UnregisterBusObject on another, which could cause the
     * C++ backing object to be deleted out from under us.  To prevent such scenarios
     * we take the global bus object map lock during the entire signal processing
     * time.  This does mean that if the Signal function causes the execution of
     * something that needs to come back in and manage the global bus objects, we
     * will deadlock.
     */
    QCC_DbgPrintf(("SignalEmitter_signal(): Taking global Bus Object map lock"));
    gBusObjectMapLock.Lock();
    JBusObject* busObject = GetBackingObject(jbusObject);
    if (!busObject) {
        QCC_DbgPrintf(("SignalEmitter_signal(): Releasing global Bus Object map lock"));
        gBusObjectMapLock.Unlock();
        QCC_LogError(ER_FAIL, ("SignalEmitter_signal(): Exception"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_BUS_NO_SUCH_OBJECT));
        return;
    }

    BusAttachment& bus = const_cast<BusAttachment&>(busObject->GetBusAttachment());
    Message msg(bus);
    QStatus status = busObject->Signal(destination.c_str(), sessionId, ifaceName.c_str(), signalName.c_str(),
                                       args.v_struct.members, args.v_struct.numMembers, timeToLive, flags, msg);

    QCC_DbgPrintf(("SignalEmitter_signal(): Releasing global Bus Object map lock"));
    gBusObjectMapLock.Unlock();

    if (ER_OK == status) {
        /* Update MessageContext */
        jclass msgCtxClass = env->FindClass("org/alljoyn/bus/MessageContext");
        jfieldID fid = env->GetFieldID(msgCtxClass, "isUnreliable", "Z");
        env->SetBooleanField(jmsgContext, fid, msg->IsUnreliable());
        fid = env->GetFieldID(msgCtxClass, "objectPath", "Ljava/lang/String;");
        env->SetObjectField(jmsgContext, fid, env->NewStringUTF(msg->GetObjectPath()));
        fid = env->GetFieldID(msgCtxClass, "interfaceName", "Ljava/lang/String;");
        env->SetObjectField(jmsgContext, fid, env->NewStringUTF(msg->GetInterface()));
        fid = env->GetFieldID(msgCtxClass, "memberName", "Ljava/lang/String;");
        env->SetObjectField(jmsgContext, fid, env->NewStringUTF(msg->GetMemberName()));
        fid = env->GetFieldID(msgCtxClass, "destination", "Ljava/lang/String;");
        env->SetObjectField(jmsgContext, fid, env->NewStringUTF(msg->GetDestination()));
        fid = env->GetFieldID(msgCtxClass, "sender", "Ljava/lang/String;");
        env->SetObjectField(jmsgContext, fid, env->NewStringUTF(msg->GetSender()));
        fid = env->GetFieldID(msgCtxClass, "sessionId", "I");
        env->SetIntField(jmsgContext, fid, msg->GetSessionId());
        fid = env->GetFieldID(msgCtxClass, "serial", "I");
        env->SetIntField(jmsgContext, fid, msg->GetCallSerial());
        fid = env->GetFieldID(msgCtxClass, "signature", "Ljava/lang/String;");
        env->SetObjectField(jmsgContext, fid, env->NewStringUTF(msg->GetSignature()));
        fid = env->GetFieldID(msgCtxClass, "authMechanism", "Ljava/lang/String;");
        env->SetObjectField(jmsgContext, fid, env->NewStringUTF(msg->GetAuthMechanism().c_str()));
    }

    if (ER_OK != status) {
        QCC_LogError(ER_FAIL, ("SignalEmitter_signal(): Exception"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(status));
    }
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_SignalEmitter_cancelSessionlessSignal(JNIEnv* env, jobject thiz, jobject jbusObject, jint serialNum)
{
    QCC_DbgPrintf(("SignalEmitter_cancelSessionlessSignal()"));

    gBusObjectMapLock.Lock();
    JBusObject* busObject = GetBackingObject(jbusObject);
    if (!busObject) {
        gBusObjectMapLock.Unlock();
        QCC_LogError(ER_FAIL, ("SignalEmitter_cancelSessionlessSignal(): Exception"));
        env->ThrowNew(CLS_BusException, QCC_StatusText(ER_BUS_NO_SUCH_OBJECT));
        return NULL;
    }

    QStatus status = busObject->CancelSessionlessMessage(serialNum);

    gBusObjectMapLock.Unlock();

    return JStatus(status);
}

JNIEXPORT jobjectArray JNICALL Java_org_alljoyn_bus_Signature_split(JNIEnv* env, jclass clazz, jstring jsignature)
{
    // QCC_DbgPrintf(("Signature_split()"));

    JString signature(jsignature);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("Signature_split(): Exception"));
        return NULL;
    }
    const char* next = signature.c_str();
    if (next) {
        uint8_t count = SignatureUtils::CountCompleteTypes(next);
        JLocalRef<jobjectArray> jsignatures = env->NewObjectArray(count, CLS_String, NULL);
        if (!jsignatures) {
            return NULL;
        }
        const char* prev = next;
        for (jsize i = 0; *next; ++i, prev = next) {
            QStatus status = SignatureUtils::ParseCompleteType(next);
            if (ER_OK != status) {
                return NULL;
            }
            assert(i < count);

            ptrdiff_t len = next - prev;
            String type(prev, len);

            JLocalRef<jstring> jtype = env->NewStringUTF(type.c_str());
            if (!jtype) {
                return NULL;
            }
            env->SetObjectArrayElement(jsignatures, i, jtype);
            if (env->ExceptionCheck()) {
                return NULL;
            }
        }
        return jsignatures.move();
    } else {
        return NULL;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_Variant_destroy(JNIEnv* env, jobject thiz)
{
    // QCC_DbgPrintf(("Variant_destroy()"));

    MsgArg* arg = GetHandle<MsgArg*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("Variant_destroy(): Exception"));
        return;
    }

    if (!arg) {
        return;
    }
    delete arg;
    SetHandle(thiz, NULL);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_Variant_setMsgArg(JNIEnv* env, jobject thiz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("Variant_setMsgArg()"));

    MsgArg* arg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_VARIANT == arg->typeId);
    MsgArg* argCopy = new MsgArg(*arg->v_variant.val);
    if (!argCopy) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }
    SetHandle(thiz, argCopy);
    if (env->ExceptionCheck()) {
        delete argCopy;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_BusException_logln(JNIEnv* env, jclass clazz, jstring jline)
{
    JString line(jline);
    if (env->ExceptionCheck()) {
        return;
    }
    _QCC_DbgPrint(DBG_LOCAL_ERROR, ("%s", line.c_str()));
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_MsgArg_getNumElements(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getNumElements()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_ARRAY == msgArg->typeId);
    return msgArg->v_array.GetNumElements();
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_getElement(JNIEnv* env, jclass clazz, jlong jmsgArg, jint index)
{
    // QCC_DbgPrintf(("MsgArg_getElement()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_ARRAY == msgArg->typeId);
    assert(index < (jint)msgArg->v_array.GetNumElements());
    return (jlong) & msgArg->v_array.GetElements()[index];
}

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_MsgArg_getElemSig(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getElementSig()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_ARRAY == msgArg->typeId);
    return env->NewStringUTF(msgArg->v_array.GetElemSig());
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_getVal(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getVal()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    switch (msgArg->typeId) {
    case ALLJOYN_VARIANT:
        return (jlong)msgArg->v_variant.val;

    case ALLJOYN_DICT_ENTRY:
        return (jlong)msgArg->v_dictEntry.val;

    default:
        assert(0);
        return 0;
    }
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_MsgArg_getNumMembers(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getNumMembers()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_STRUCT == msgArg->typeId);
    return msgArg->v_struct.numMembers;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_getMember(JNIEnv* env, jclass clazz, jlong jmsgArg, jint index)
{
    // QCC_DbgPrintf(("MsgArg_getMember()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_STRUCT == msgArg->typeId);
    assert(index < (jint)msgArg->v_struct.numMembers);
    return (jlong) & msgArg->v_struct.members[index];
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_getKey(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getKey()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_DICT_ENTRY == msgArg->typeId);
    return (jlong)msgArg->v_dictEntry.key;
}

JNIEXPORT jbyteArray JNICALL Java_org_alljoyn_bus_MsgArg_getByteArray(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getKey()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_BYTE_ARRAY == msgArg->typeId);
    jbyteArray jarray = env->NewByteArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }
    jbyte* jelements = env->GetByteArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_byte[i];
    }
    env->ReleaseByteArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jshortArray JNICALL Java_org_alljoyn_bus_MsgArg_getInt16Array(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getInt16Array()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_INT16_ARRAY == msgArg->typeId);

    jshortArray jarray = env->NewShortArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }

    jshort* jelements = env->GetShortArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_int16[i];
    }

    env->ReleaseShortArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jshortArray JNICALL Java_org_alljoyn_bus_MsgArg_getUint16Array(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getUint16Array()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_UINT16_ARRAY == msgArg->typeId);

    jshortArray jarray = env->NewShortArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }

    jshort* jelements = env->GetShortArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_uint16[i];
    }

    env->ReleaseShortArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jbooleanArray JNICALL Java_org_alljoyn_bus_MsgArg_getBoolArray(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getBoolArray()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_BOOLEAN_ARRAY == msgArg->typeId);

    jbooleanArray jarray = env->NewBooleanArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }

    jboolean* jelements = env->GetBooleanArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_bool[i];
    }

    env->ReleaseBooleanArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jintArray JNICALL Java_org_alljoyn_bus_MsgArg_getUint32Array(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getUint32Array()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_UINT32_ARRAY == msgArg->typeId);

    jintArray jarray = env->NewIntArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }

    jint* jelements = env->GetIntArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_uint32[i];
    }

    env->ReleaseIntArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jintArray JNICALL Java_org_alljoyn_bus_MsgArg_getInt32Array(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getUint32Array()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_INT32_ARRAY == msgArg->typeId);

    jintArray jarray = env->NewIntArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }

    jint* jelements = env->GetIntArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_int32[i];
    }

    env->ReleaseIntArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jlongArray JNICALL Java_org_alljoyn_bus_MsgArg_getInt64Array(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getInt64Array()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_INT64_ARRAY == msgArg->typeId);

    jlongArray jarray = env->NewLongArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }

    jlong* jelements = env->GetLongArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_int64[i];
    }

    env->ReleaseLongArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jlongArray JNICALL Java_org_alljoyn_bus_MsgArg_getUint64Array(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getUint64Array()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_UINT64_ARRAY == msgArg->typeId);

    jlongArray jarray = env->NewLongArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }

    jlong* jelements = env->GetLongArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_uint64[i];
    }

    env->ReleaseLongArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jdoubleArray JNICALL Java_org_alljoyn_bus_MsgArg_getDoubleArray(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getDoubleArray()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_DOUBLE_ARRAY == msgArg->typeId);

    jdoubleArray jarray = env->NewDoubleArray(msgArg->v_scalarArray.numElements);
    if (!jarray) {
        return NULL;
    }

    jdouble* jelements = env->GetDoubleArrayElements(jarray, NULL);
    for (size_t i = 0; i < msgArg->v_scalarArray.numElements; ++i) {
        jelements[i] = msgArg->v_scalarArray.v_double[i];
    }

    env->ReleaseDoubleArrayElements(jarray, jelements, 0);
    return jarray;
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_MsgArg_getTypeId(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getTypeId()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    return msgArg->typeId;
}

JNIEXPORT jbyte JNICALL Java_org_alljoyn_bus_MsgArg_getByte(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getByte()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_BYTE == msgArg->typeId);
    return msgArg->v_byte;
}

JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_MsgArg_getInt16(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getInt16()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_INT16 == msgArg->typeId);
    return msgArg->v_int16;
}

JNIEXPORT jshort JNICALL Java_org_alljoyn_bus_MsgArg_getUint16(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getUint16()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_UINT16 == msgArg->typeId);
    return msgArg->v_uint16;
}

JNIEXPORT jboolean JNICALL Java_org_alljoyn_bus_MsgArg_getBool(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getBool()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_BOOLEAN == msgArg->typeId);
    return msgArg->v_bool;
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_MsgArg_getUint32(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getUint32()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_UINT32 == msgArg->typeId);
    return msgArg->v_uint32;
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_MsgArg_getInt32(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getInt32()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_INT32 == msgArg->typeId);
    return msgArg->v_int32;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_getInt64(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getInt64()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_INT64 == msgArg->typeId);
    return msgArg->v_int64;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_getUint64(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getUint64()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_UINT64 == msgArg->typeId);
    return msgArg->v_uint64;
}

JNIEXPORT jdouble JNICALL Java_org_alljoyn_bus_MsgArg_getDouble(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getDouble()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_DOUBLE == msgArg->typeId);
    return msgArg->v_double;
}

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_MsgArg_getString(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getString()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_STRING == msgArg->typeId);

    char* str = new char[msgArg->v_string.len + 1];
    if (!str) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return NULL;
    }

    memcpy(str, msgArg->v_string.str, msgArg->v_string.len);
    str[msgArg->v_string.len] = 0;

    jstring jstr = env->NewStringUTF(str);
    delete [] str;
    return jstr;
}

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_MsgArg_getObjPath(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getObjPath()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_OBJECT_PATH == msgArg->typeId);

    char* str = new char[msgArg->v_objPath.len + 1];
    if (!str) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return NULL;
    }

    memcpy(str, msgArg->v_objPath.str, msgArg->v_objPath.len);
    str[msgArg->v_objPath.len] = 0;

    jstring jstr = env->NewStringUTF(str);
    delete [] str;
    return jstr;
}

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_MsgArg_getSignature__J(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_getsignature__J()"));

    MsgArg* msgArg = (MsgArg*)jmsgArg;
    assert(ALLJOYN_SIGNATURE == msgArg->typeId);

    char* str = new char[msgArg->v_signature.len + 1];
    if (!str) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return NULL;
    }

    memcpy(str, msgArg->v_signature.sig, msgArg->v_signature.len);
    str[msgArg->v_signature.len] = 0;

    jstring jstr = env->NewStringUTF(str);
    delete [] str;
    return jstr;
}

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_MsgArg_getSignature___3J(JNIEnv* env, jclass clazz, jlongArray jarray)
{
    // QCC_DbgPrintf(("MsgArg_getsignature___3J()"));

    MsgArg* values = NULL;
    size_t numValues = jarray ? env->GetArrayLength(jarray) : 0;
    if (numValues) {
        values = new MsgArg[numValues];
        if (!values) {
            Throw("java/lang/OutOfMemoryError", NULL);
            return NULL;
        }

        jlong* jvalues = env->GetLongArrayElements(jarray, NULL);
        for (size_t i = 0; i < numValues; ++i) {
            values[i] = *(MsgArg*)(jvalues[i]);
        }

        env->ReleaseLongArrayElements(jarray, jvalues, JNI_ABORT);
    }

    jstring signature = env->NewStringUTF(MsgArg::Signature(values, numValues).c_str());
    delete [] values;
    return signature;
}

/**
 * Calls MsgArgUtils::SetV() to set the values of a MsgArg.
 *
 * @param arg the arg to set
 * @param jsignature the signature of the arg
 * @param ... the values to set
 * @return the @param arg passed in or NULL if an error occurred
 * @throws BusException if an error occurs
 */
static MsgArg* Set(JNIEnv* env, MsgArg* arg, jstring jsignature, ...)
{
    // QCC_DbgPrintf(("Set()"));

    JString signature(jsignature);
    if (env->ExceptionCheck()) {
        return NULL;
    }
    va_list argp;
    va_start(argp, jsignature);
    size_t one = 1;
    QStatus status = MsgArgUtils::SetV(arg, one, signature.c_str(), &argp);
    va_end(argp);
    if (ER_OK != status) {
        env->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return NULL;
    }
    return arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2B(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jbyte value)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2B()"));
    return (jlong)Set(env, (MsgArg*)jmsgArg, jsignature, value);
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2Z(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jboolean value)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2Z()"));
    return (jlong)Set(env, (MsgArg*)jmsgArg, jsignature, value);
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2S(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jshort value)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2S()"));
    return (jlong)Set(env, (MsgArg*)jmsgArg, jsignature, value);
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2I(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jint value)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2I()"));
    return (jlong)Set(env, (MsgArg*)jmsgArg, jsignature, value);
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2J(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jlong value)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2J()"));
    return (jlong)Set(env, (MsgArg*)jmsgArg, jsignature, value);
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2D(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jdouble value)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2D()"));
    return (jlong)Set(env, (MsgArg*)jmsgArg, jsignature, value);
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2Ljava_lang_String_2(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jstring jvalue)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2Ljava_lang_String_2"));

    JString value(jvalue);
    if (env->ExceptionCheck()) {
        return 0;
    }

    MsgArg* arg = Set(env, (MsgArg*)jmsgArg, jsignature, value.c_str());
    if (arg) {
        arg->Stabilize();
    }

    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2_3B(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jbyteArray jarray)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2_3B"));

    jbyte* jelements = env->GetByteArrayElements(jarray, NULL);

    MsgArg* arg = Set(env, (MsgArg*)jmsgArg, jsignature, env->GetArrayLength(jarray), jelements);
    if (arg) {
        arg->Stabilize();
    }

    env->ReleaseByteArrayElements(jarray, jelements, JNI_ABORT);
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2_3Z(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jbooleanArray jarray)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2_3Z"));

    /* Booleans are different sizes in Java and MsgArg, so can't just do a straight copy. */
    jboolean* jelements = env->GetBooleanArrayElements(jarray, NULL);
    size_t numElements = env->GetArrayLength(jarray);
    bool* v_bool = new bool[numElements];
    if (!v_bool) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return 0;
    }

    for (size_t i = 0; i < numElements; ++i) {
        v_bool[i] = jelements[i];
    }

    MsgArg* arg = Set(env, (MsgArg*)jmsgArg, jsignature, numElements, v_bool);
    if (arg) {
        arg->SetOwnershipFlags(MsgArg::OwnsData);
    } else {
        delete [] v_bool;
    }

    env->ReleaseBooleanArrayElements(jarray, jelements, JNI_ABORT);
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2_3S(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jshortArray jarray)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2_3S"));

    jshort* jelements = env->GetShortArrayElements(jarray, NULL);

    MsgArg* arg = Set(env, (MsgArg*)jmsgArg, jsignature, env->GetArrayLength(jarray), jelements);
    if (arg) {
        arg->Stabilize();
    }

    env->ReleaseShortArrayElements(jarray, jelements, JNI_ABORT);
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2_3I(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jintArray jarray)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2_3I"));

    jint* jelements = env->GetIntArrayElements(jarray, NULL);

    MsgArg* arg = Set(env, (MsgArg*)jmsgArg, jsignature, env->GetArrayLength(jarray), jelements);
    if (arg) {
        arg->Stabilize();
    }

    env->ReleaseIntArrayElements(jarray, jelements, JNI_ABORT);
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2_3J(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jlongArray jarray)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2_3J"));

    jlong* jelements = env->GetLongArrayElements(jarray, NULL);

    MsgArg* arg = Set(env, (MsgArg*)jmsgArg, jsignature, env->GetArrayLength(jarray), jelements);
    if (arg) {
        arg->Stabilize();
    }

    env->ReleaseLongArrayElements(jarray, jelements, JNI_ABORT);
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_set__JLjava_lang_String_2_3D(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jdoubleArray jarray)
{
    // QCC_DbgPrintf(("MsgArg_set__JLjava_lang_String_2_3D"));

    jdouble* jelements = env->GetDoubleArrayElements(jarray, NULL);

    MsgArg* arg = Set(env, (MsgArg*)jmsgArg, jsignature, env->GetArrayLength(jarray), jelements);
    if (arg) {
        arg->Stabilize();
    }

    env->ReleaseDoubleArrayElements(jarray, jelements, JNI_ABORT);
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_setArray(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jelemSig, jint numElements)
{
    QCC_DbgPrintf(("MsgArg_setArray"));

    JString elemSig(jelemSig);
    if (env->ExceptionCheck()) {
        return 0;
    }

    MsgArg* arg = (MsgArg*)jmsgArg;

    MsgArg* elements = new MsgArg[numElements];
    if (!elements) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return 0;
    }

    QCC_DbgPrintf(("MsgArg_setArray calling SetElements: %s, %d, %p", elemSig.c_str(), numElements, elements));
    QStatus status = arg->v_array.SetElements(elemSig.c_str(), numElements, elements);
    if (ER_OK != status) {
        QCC_DbgPrintf(("MsgArg_setArray calling SetElements: failed"));
        delete [] elements;
        env->ThrowNew(CLS_BusException, QCC_StatusText(status));
        return 0;
    }
    QCC_DbgPrintf(("MsgArg_setArray calling SetElements: successful"));
    arg->SetOwnershipFlags(MsgArg::OwnsArgs);
    arg->typeId = ALLJOYN_ARRAY;
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_setStruct(JNIEnv* env, jclass clazz, jlong jmsgArg, jint numMembers)
{
    // QCC_DbgPrintf(("MsgArg_setStruct"));

    MsgArg* arg = (MsgArg*)jmsgArg;

    MsgArg* members = new MsgArg[numMembers];
    if (!members) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return 0;
    }

    arg->v_struct.numMembers = numMembers;
    arg->v_struct.members = members;
    arg->SetOwnershipFlags(MsgArg::OwnsArgs);
    arg->typeId = ALLJOYN_STRUCT;
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_setDictEntry(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_setDictEntry"));

    MsgArg* arg = (MsgArg*)jmsgArg;
    MsgArg* key = new MsgArg;
    MsgArg* val = new MsgArg;
    if (!key || !val) {
        delete val;
        delete key;
        Throw("java/lang/OutOfMemoryError", NULL);
        return 0;
    }
    arg->v_dictEntry.key = key;
    arg->v_dictEntry.val = val;
    arg->SetOwnershipFlags(MsgArg::OwnsArgs);
    arg->typeId = ALLJOYN_DICT_ENTRY;
    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_setVariant__JLjava_lang_String_2J(JNIEnv* env, jclass clazz, jlong jmsgArg, jstring jsignature, jlong jvalue)
{
    // QCC_DbgPrintf(("MsgArg_setVariant__JLjava_lang_String_2J"));

    MsgArg* value = new MsgArg(*(MsgArg*)jvalue);
    if (!value) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return 0;
    }

    MsgArg* arg = Set(env, (MsgArg*)jmsgArg, jsignature, value);
    if (arg) {
        arg->SetOwnershipFlags(MsgArg::OwnsArgs);
    }

    return (jlong)arg;
}

JNIEXPORT jlong JNICALL Java_org_alljoyn_bus_MsgArg_setVariant__J(JNIEnv* env, jclass clazz, jlong jmsgArg)
{
    // QCC_DbgPrintf(("MsgArg_setVariant__J"));

    MsgArg* arg = (MsgArg*)jmsgArg;

    MsgArg* val = new MsgArg;
    if (!val) {
        delete val;
        Throw("java/lang/OutOfMemoryError", NULL);
        return 0;
    }

    arg->v_variant.val = val;
    arg->SetOwnershipFlags(MsgArg::OwnsArgs);
    arg->typeId = ALLJOYN_VARIANT;
    return (jlong)arg;
}


JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_PasswordManager_setCredentials(JNIEnv*env, jobject,
                                                                              jstring authMechanism, jstring password)
{
    /*
     * Load the C++ authMechanism Java authMechanism.
     */
    JString jauthMechanism(authMechanism);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("PasswordManager_setCredentials(): Exception"));
        return NULL;
    }

    /*
     * Load the C++ password Java password.
     */
    JString jpassword(password);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("PasswordManager_setCredentials(): Exception"));
        return NULL;
    }

    QStatus status = PasswordManager::SetCredentials(jauthMechanism.c_str(), jpassword.c_str());
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("PasswordManager_setCredentials(): Exception"));
        return NULL;
    }

    if (status != ER_OK) {
        QCC_LogError(status, ("PasswordManager_setCredentials: SetCredentials() fails"));
    }

    return JStatus(status);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_Translator_create(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("Translator_create()"));

    assert(GetHandle<JTranslator*>(thiz) == NULL);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("Translator_create(): Exception"));
        return;
    }

    JTranslator* jdt = new JTranslator(thiz);
    if (jdt == NULL) {
        Throw("java/lang/OutOfMemoryError", NULL);
        return;
    }

    SetHandle(thiz, jdt);
    if (env->ExceptionCheck()) {
        delete jdt;
    }
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_Translator_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("Translator_destroy()"));

    JTranslator* jdt = GetHandle<JTranslator*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("Translator_destroy(): Exception"));
        return;
    }

    assert(jdt);
    delete jdt;

    SetHandle(thiz, NULL);
    return;
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_AboutObj_create(JNIEnv* env, jobject thiz, jobject jbus, jboolean isAboutAnnounced)
{
    JBusAttachment* busPtr = GetHandle<JBusAttachment*>(jbus);
    if (env->ExceptionCheck() || busPtr == NULL) {
        QCC_LogError(ER_FAIL, ("BusAttachment_create(): Exception or NULL bus pointer"));
        return;
    }
    QCC_DbgPrintf(("BusAttachment_unregisterBusListener(): Refcount on busPtr is %d", busPtr->GetRef()));

    JAboutObject* aboutObj;
    if (isAboutAnnounced == JNI_TRUE) {
        aboutObj = new JAboutObject(busPtr, BusObject::ANNOUNCED);
    } else {
        aboutObj = new JAboutObject(busPtr, BusObject::UNANNOUNCED);
    }
    // Make the JAboutObj Accessable to the BusAttachment so it can be used
    // by the BusAttachment to Release the global ref contained in the JAboutObject
    // when the BusAttachment shuts down.
    aboutObj->busPtr->aboutObj = aboutObj;
    // Incrament the ref so the BusAttachment will not be deleted before the About
    // Object.
    aboutObj->busPtr->IncRef();

    SetHandle(thiz, aboutObj);
}

JNIEXPORT void JNICALL Java_org_alljoyn_bus_AboutObj_destroy(JNIEnv* env, jobject thiz)
{
    QCC_DbgPrintf(("AboutObj_destroy()"));

    JAboutObject* aboutObj = GetHandle<JAboutObject*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("AboutObj_destroy(): Exception"));
        return;
    }

    if (aboutObj == NULL) {
        QCC_DbgPrintf(("AboutObj_destroy(): Already destroyed. Returning."));
        return;
    }

    JBusAttachment* busPtr = aboutObj->busPtr;

    //Remove the BusAttachments pointer to the JAboutObject
    busPtr->aboutObj = NULL;

    delete aboutObj;
    aboutObj = NULL;

    // Decrament the ref pointer so the BusAttachment can be released.
    busPtr->DecRef();

    SetHandle(thiz, NULL);
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_AboutObj_announce(JNIEnv* env, jobject thiz, jshort sessionPort, jobject jaboutDataListener)
{
    QCC_DbgPrintf(("AboutObj_announce"));

    QStatus status = ER_FAIL;
    JAboutObject* aboutObj = GetHandle<JAboutObject*>(thiz);
    if (env->ExceptionCheck() || !aboutObj) {
        QCC_LogError(ER_FAIL, ("AboutObj_announce(): Exception"));
        return JStatus(status);
    }
    // if we don't already have a GlobalRef obtain a GlobalRef
    aboutObj->jaboutObjGlobalRefLock.Lock();
    if (aboutObj->jaboutObjGlobalRef == NULL) {
        aboutObj->jaboutObjGlobalRef = env->NewGlobalRef(thiz);
    }
    aboutObj->jaboutObjGlobalRefLock.Unlock();
    return JStatus(aboutObj->announce(env, thiz, sessionPort, jaboutDataListener));
}

JNIEXPORT jobject JNICALL Java_org_alljoyn_bus_AboutObj_unannounce(JNIEnv* env, jobject thiz) {
    JAboutObject* aboutObj = GetHandle<JAboutObject*>(thiz);
    if (env->ExceptionCheck()) {
        QCC_LogError(ER_FAIL, ("AboutObj_unannounce(): Exception"));
        return JStatus(ER_FAIL);
    } else if (aboutObj == NULL) {
        QCC_LogError(ER_FAIL, ("AboutObj_cancelAnnouncement(): NULL AboutObj"));
        return JStatus(ER_FAIL);
    }
    // Release the GlobalRef it will be re-obtained if announce is called again
    aboutObj->jaboutObjGlobalRefLock.Lock();
    if (aboutObj->jaboutObjGlobalRef != NULL) {
        env->DeleteGlobalRef(aboutObj->jaboutObjGlobalRef);
        aboutObj->jaboutObjGlobalRef = NULL;
    }
    aboutObj->jaboutObjGlobalRefLock.Unlock();
    return JStatus(aboutObj->Unannounce());
}

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_Version_get(JNIEnv* env, jclass clazz) {

    return env->NewStringUTF(ajn::GetVersion());
}

JNIEXPORT jstring JNICALL Java_org_alljoyn_bus_Version_getBuildInfo(JNIEnv* env, jclass clazz) {
    return env->NewStringUTF(ajn::GetBuildInfo());
}

JNIEXPORT jint JNICALL Java_org_alljoyn_bus_Version_getNumeric(JNIEnv* env, jclass clazz) {
    return ajn::GetNumericVersion();
}
