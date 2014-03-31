/* -*- Mode: C++; c-file-style: "1tbs"; indent-tabs-mode:nil; -*- */
/**
 * @file
 * Data structures used for generalized callbacks
 */

/******************************************************************************
 * Copyright (c) 2010-2012, 2014, AllSeen Alliance. All rights reserved.
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
#ifndef _CALLBACK_H
#define _CALLBACK_H

#ifndef __cplusplus
#error Only include Callback.h in C++ code.
#endif

namespace ajn {

/**
 * Empty class used for a simplified but powerful callback mechanism.
 *
 * This is a simplified, but powerful callback mechanism that allows for separation
 * of implementation between two objects that exchange callbacks.  The following
 * classes are in the spirit of the Generalized Functors of Andrei Alexandrescu.
 *
 * Callbacks are split into two halves.  There is a generic piece that is used when
 * invoking, and there is a specific piece that is used when creating the Callback.
 *
 * The basic idea is actually quite simple.  First you declare a generic version
 * of the Callback that knows nothing about the specific implementation.  This is
 * the side of the callback that will be invoked.
   @code
   template <typename R>
   class GenericCallback {
   public:
       virtual R operator() (void) = 0;
   };
   @endcode
 *
 * Then you create a specific implementation that inherits from the generic
 * version.  This specific implementation knows about the details of the class
 * that contains the method to be called.
 *
   @code
   template <typename T, typename R>
   class SpecificCallback : public GenericCallback<R> {
   public:
      SpecificCallback(T *object, void(T::*member)(void)) : m_object(object), m_member(member) {}

      R operator() (void)
      {
          return (*m_object.*m_member)();
      }

   private:
      T *m_object;
      void (T::*m_member)(void);
   };
   @endcode
 *
 * You can create a class/object and method to be called, with the method appropriate
 * to the signature we just declared/defined:  void (*)(void)
 *
   @code
   class Test {
   public:
      void Method(void)
      {
          printf("void Method(void) called\n");
      }
   };
   @endcode
 *
 * When you want to create a specific Callback in the place that has knowledge
 * about the called class, you create one and initialize it using:
 *
   @code
      SpecificCallback<Test, void> specificCallback(&test, &Test::Method);
   @endcode
 * The declaration instantiates a Callback class that knows how to call into a
 * Test class to a method that takes no parameters and returns void -- thus
 * the <tt><Test, void></tt> template arguments.  The constructor provides the
 * "this" pointer and the function pointer offset of the method -- thus the
 * <tt>(&test, &Test::Method)</tt> parameters.
 *
 * You then take a specific callback pointer and assign it to a generic callback
 * pointer.  This casts the specific callback to a generic callback with a
 * defined <tt>operator()</tt>.
   @code
      GenericCallback<void> *pCallback = &specificCallback;
   @endcode
 * It is this generic Callback pointer that you give to the entity that you
 * want to call you back.  The calling entity does not have to know anything
 * about the internals of your class at all.  All it has to do is to invoke the
 * call operator.
 *
   @code
      (*pCallback)();
   @endcode
 * At its core, this is is a fancy templated version of the handle-body (also
 * known as pimpl) idiom designed to hide implementation details.
 *
 * Making this facility more generic and easier to use makes the templates
 * more complicated, but the basic idea remains.
 */
class empty { };

/**
 * @defgroup callback_usage Callback example
 * @{
 * These things are sometimes quite incomprehensible, so a short example of a
 * real use may be helpful here.
 *
 * Let's say you need to put together two classes that are hooked together
 * by a callback.  This callback takes two int parameters and returns a
 * QStatus:
 *
 * The first thing to do is to create a class with a method that will be the
 * target of the callback.  Call the target method, "Method":
 *
 *     class Callee {
 *     public:
 *         QStatus Method(int a1, int a2)
 *         {
 *             printf("Method(%d, %d) called\n", a1, a2);
 *             return ER_OK;
 *         }
 *     };
 *
 *
 * Now you need a class that will invoke the callback.  This class need some
 * way to have the callback set, and some member variable to hold the actual
 * callback.
 *
 *     class Caller {
 *     public:
 *          void SetCallback(Callback<QStatus, int, int> *cb) {m_cb = cb;}
 *     private:
 *          QStatus FireCallback(int a1, int a2) {return (*m_cb)(a1, a2);}
 *          Callback<QStatus, int, int> *m_cb;
 *     };
 *
 * Instantiate the classes somewhere.
 *
 *     Callee callee;
 *     Caller caller;
 *
 * You make a new CallbackImpl and give it to the caller:
 *
 *     caller.SetCallback(new CallbackImpl<Callee, QStatus, int, int>(&callee, &Callee::Method));
 *
 * The caller can then do its own thing for a while and then fire the callback
 * providing two integers and saving the returned QStatus.
 *
 *     QStatus status = FireCallback(1, 2);
 *
 * This executes the callback in the Callee.  Notice that the Caller class
 * has no idea about the implementation details of the Callee class, even
 * though it is making a method call into that class.
 * @}
 */

/**
 * Define the generic side of Callback class with a return type and some total
 * number of arguments we want to support.  The default value of the arguments
 * is set to the type " * ".  This type is actually never used, but acts as
 * a placeholder to allow us to construct what appears to be variable argument
 * lists.
 *
 * For example, if you want to call a function that returns void and takes no
 * arguments, you would declare Callback<void>.  Since the template has twelve
 * default arguments, what you are really asking for is:
 *
   @code
      Callback<void, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty,>
   @endcode
 * We provide a number of partially specified Callback templates below.  One
 * of them will have the right number of non-empty arguments in its signature.
 * In the case of the example above, The found template class will define an
 * operator:
 *
   @code
      virtual R operator() () = 0;
   @endcode
 *
 * which, with the substitution of void for R due to template instantiation,
 * is the signature of the callback you wanted.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 * @tparam T9 type for the ninth parameter in callback member function
 * @tparam T10 type for the tenth parameter in callback member function
 * @tparam T11 type for the eleventh parameter in callback member function
 * @tparam T12 type for the twelfth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R,
          typename T1 = empty, typename T2 = empty, typename T3 = empty, typename T4 = empty,
          typename T5 = empty, typename T6 = empty, typename T7 = empty, typename T8 = empty,
          typename T9 = empty, typename T10 = empty, typename T11 = empty, typename T12 = empty>
class Callback {
  public:
    /**
     * virtual destructor for the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * twelve parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) = 0;
  private:
    /**
     * Declare copy constructor private without implementation to prevent usage.
     */
    Callback(const Callback& other);
    /**
     * Declare assignment operator private without implementation to prevent usage.
     */
    Callback& operator=(const Callback& other);
};

/**
 * Define a partial specialization for a generic Callback with no arguments.
 * The "empty" template arguments are where you would more typically put a type
 * such as int or float.  These are called, perhaps not too surprisingly,
 * non-type template arguments.  This is going to define an Abstract Data Type
 * that will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 *
 * @ingroup callback_usage
 */
template <typename R>
class Callback<R, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * zero parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()() = 0;
};

/**
 * Define partial specialization for generic Callbacks with one non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1>
class Callback<R, T1, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * one parameter
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1) = 0;
};

/**
 * Define partial specialization for generic Callbacks with two non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2>
class Callback<R, T1, T2, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * two parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2) = 0;
};

/**
 * Define partial specialization for generic Callbacks with three non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3>
class Callback<R, T1, T2, T3, empty, empty, empty, empty, empty, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * three parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3) = 0;
};

/**
 * Define partial specialization for generic Callbacks with four non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3, typename T4>
class Callback<R, T1, T2, T3, T4, empty, empty, empty, empty, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * four parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4) = 0;
};

/**
 * Define partial specialization for generic Callbacks with five non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
class Callback<R, T1, T2, T3, T4, T5, empty, empty, empty, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * five parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4, T5) = 0;
};

/**
 * Define partial specialization for generic Callbacks with six non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class Callback<R, T1, T2, T3, T4, T5, T6, empty, empty, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * six parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4, T5, T6) = 0;
};

/**
 * Define partial specialization for generic Callbacks with seven non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class Callback<R, T1, T2, T3, T4, T5, T6, T7, empty, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * seven parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4, T5, T6, T7) = 0;
};

/**
 * Define partial specialization for generic Callbacks with eight non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class Callback<R, T1, T2, T3, T4, T5, T6, T7, T8, empty, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * eight parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4, T5, T6, T7, T8) = 0;
};

/**
 * Define partial specialization for generic Callbacks with nine non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 * @tparam T9 type for the ninth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class Callback<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, empty, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * nine parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4, T5, T6, T7, T8, T9) = 0;
};

/**
 * Define partial specialization for generic Callbacks with ten non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 * @tparam T9 type for the ninth parameter in callback member function
 * @tparam T10 type for the tenth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class Callback<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, empty, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * ten parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) = 0;
};

/**
 * Define partial specialization for generic Callbacks with eleven non-empty arguments.
 *
 * The none empty arguments are where a types such as int or
 * float would be placed. This is going to define an Abstract Data Type that
 * will be used by callers of the generic side of the Callback.
 *
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 * @tparam T9 type for the ninth parameter in callback member function
 * @tparam T10 type for the tenth parameter in callback member function
 * @tparam T11 type for the eleventh parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class Callback<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, empty> {
  public:
    /**
     * virtual destructor for the partial specialization of the Callback class
     */
    virtual ~Callback() { }

    /**
     * The pure virtual operator that used to define a callback method that takes
     * eleven parameters
     * @see empty
     *
     * @return return the value specified in the template R type
     */
    virtual R operator()(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) = 0;
};

/**
 * The specific side of the Callbacks follows the same prescription as the
 * generic side.  Instead of calling them specific, we call the templates
 * the implementations (Impl) of the generic callbacks called simply
 * Callback.
 *
 * Each of the implementations inherits from a corresponding generic class
 * to get the ADT of the operator it must implement.  This is so the all-
 * important cast can be made from the specific pointer to the generic
 * pointer.
 *
 * For each of the specializations, we have to save the "this" pointer
 * and the member pointer offset, which is done via the constructor.
 * We also provide an implementation of the call operator, which is going
 * to be the implementation of the pure virtual call operator specified
 * in the generic Callback.
 *
 * The prescription here follows that of the generic case.  We provide
 * partial specializations with all of the various numbers of non-empty
 * arguments.
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 * @tparam T9 type for the ninth parameter in callback member function
 * @tparam T10 type for the tenth parameter in callback member function
 * @tparam T11 type for the eleventh parameter in callback member function
 * @tparam T12 type for the twelfth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1 = empty, typename T2 = empty, typename T3 = empty, typename T4 = empty,
          typename T5 = empty, typename T6 = empty, typename T7 = empty, typename T8 = empty,
          typename T9 = empty, typename T10 = empty, typename T11 = empty, typename T12 = empty>
class CallbackImpl
    : public Callback<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12> {
  public:
    /**
     * The callback implementation
     * @param object an object of type T used to save the this pointer for the callback implementation
     * @param member an implementation of the call operator
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12))
        : m_object(object), m_member(member) { }

    /**
     * used to call the callee from the caller of a CallbackImpl
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     * @param a5 fifth templated input parameter for the callback implementation
     * @param a6 sixth templated input parameter for the callback implementation
     * @param a7 seventh templated input parameter for the callback implementation
     * @param a8 eight templated input parameter for the callback implementation
     * @param a9 ninth templated input parameter for the callback implementation
     * @param a10 tenth templated input parameter for the callback implementation
     * @param a11 eleventh templated input parameter for the callback implementation
     * @param a12 twelfth templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8, T9 a9, T10 a10, T11 a11, T12 a12)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with twelve parameters
     */
    R (T::* m_member)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 *
 * @ingroup callback_usage
 */
template <typename T, typename R>
class CallbackImpl<T, R, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty>
    : public Callback<R> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)())
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @return return the value specified in the template R type
     */
    R operator()()
    {
        return (*m_object.*m_member)();
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with zero parameters
     */
    R (T::* m_member)();
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1>
class CallbackImpl<T, R, T1, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty>
    : public Callback<R, T1> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1)
    {
        return (*m_object.*m_member)(a1);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with one parameter
     */
    R (T::* m_member)(T1);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2>
class CallbackImpl<T, R, T1, T2, empty, empty, empty, empty, empty, empty, empty, empty, empty, empty>
    : public Callback<R, T1, T2> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2)
    {
        return (*m_object.*m_member)(a1, a2);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with two parameters
     */
    R (T::* m_member)(T1, T2);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3>
class CallbackImpl<T, R, T1, T2, T3, empty, empty, empty, empty, empty, empty, empty, empty, empty>
    : public Callback<R, T1, T2, T3> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3)
    {
        return (*m_object.*m_member)(a1, a2, a3);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with three parameters
     */
    R (T::* m_member)(T1, T2, T3);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3, typename T4>
class CallbackImpl<T, R, T1, T2, T3, T4, empty, empty, empty, empty, empty, empty, empty, empty>
    : public Callback<R, T1, T2, T3, T4> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with four parameters
     */
    R (T::* m_member)(T1, T2, T3, T4);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3, typename T4, typename T5>
class CallbackImpl<T, R, T1, T2, T3, T4, T5, empty, empty, empty, empty, empty, empty, empty>
    : public Callback<R, T1, T2, T3, T4, T5> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4, T5))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     * @param a5 fifth templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4, a5);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with five parameters
     */
    R (T::* m_member)(T1, T2, T3, T4, T5);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
class CallbackImpl<T, R, T1, T2, T3, T4, T5, T6, empty, empty, empty, empty, empty, empty>
    : public Callback<R, T1, T2, T3, T4, T5, T6> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4, T5, T6))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     * @param a5 fifth templated input parameter for the callback implementation
     * @param a6 sixth templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4, a5, a6);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with six parameters
     */
    R (T::* m_member)(T1, T2, T3, T4, T5, T6);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
class CallbackImpl<T, R, T1, T2, T3, T4, T5, T6, T7, empty, empty, empty, empty, empty>
    : public Callback<R, T1, T2, T3, T4, T5, T6, T7> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4, T5, T6, T7))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     * @param a5 fifth templated input parameter for the callback implementation
     * @param a6 sixth templated input parameter for the callback implementation
     * @param a7 seventh templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4, a5, a6, a7);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with seven parameters
     */
    R (T::* m_member)(T1, T2, T3, T4, T5, T6, T7);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
class CallbackImpl<T, R, T1, T2, T3, T4, T5, T6, T7, T8, empty, empty, empty, empty>
    : public Callback<R, T1, T2, T3, T4, T5, T6, T7, T8> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4, T5, T6, T7, T8))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     * @param a5 fifth templated input parameter for the callback implementation
     * @param a6 sixth templated input parameter for the callback implementation
     * @param a7 seventh templated input parameter for the callback implementation
     * @param a8 eight templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4, a5, a6, a7, a8);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with eight parameters
     */
    R (T::* m_member)(T1, T2, T3, T4, T5, T6, T7, T8);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 * @tparam T9 type for the ninth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
class CallbackImpl<T, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, empty, empty, empty>
    : public Callback<R, T1, T2, T3, T4, T5, T6, T7, T8, T9> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4, T5, T6, T7, T8, T9))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     * @param a5 fifth templated input parameter for the callback implementation
     * @param a6 sixth templated input parameter for the callback implementation
     * @param a7 seventh templated input parameter for the callback implementation
     * @param a8 eight templated input parameter for the callback implementation
     * @param a9 ninth templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8, T9 a9)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with nine parameters
     */
    R (T::* m_member)(T1, T2, T3, T4, T5, T6, T7, T8, T9);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 * @tparam T9 type for the ninth parameter in callback member function
 * @tparam T10 type for the tenth parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10>
class CallbackImpl<T, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, empty, empty>
    : public Callback<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     * @param a5 fifth templated input parameter for the callback implementation
     * @param a6 sixth templated input parameter for the callback implementation
     * @param a7 seventh templated input parameter for the callback implementation
     * @param a8 eight templated input parameter for the callback implementation
     * @param a9 ninth templated input parameter for the callback implementation
     * @param a10 tenth templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8, T9 a9, T10 a10)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with ten parameters
     */
    R (T::* m_member)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10);
};

/**
 * @copydoc CallbackImpl
 *
 * @tparam T type for the callback object
 * @tparam R return data type
 * @tparam T1 type for the first parameter in callback member function
 * @tparam T2 type for the second parameter in callback member function
 * @tparam T3 type for the third parameter in callback member function
 * @tparam T4 type for the fourth parameter in callback member function
 * @tparam T5 type for the fifth parameter in callback member function
 * @tparam T6 type for the sixth parameter in callback member function
 * @tparam T7 type for the seventh parameter in callback member function
 * @tparam T8 type for the eight parameter in callback member function
 * @tparam T9 type for the ninth parameter in callback member function
 * @tparam T10 type for the tenth parameter in callback member function
 * @tparam T11 type for the eleventh parameter in callback member function
 *
 * @ingroup callback_usage
 */
template <typename T, typename R,
          typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9, typename T10, typename T11>
class CallbackImpl<T, R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, empty>
    : public Callback<R, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11> {
  public:
    /**
     * @copydoc CallbackImpl::CallbackImpl
     */
    CallbackImpl(T* object, R(T::* member)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11))
        : m_object(object), m_member(member) { }

    /**
     * @copydoc CallbackImpl::operator()
     *
     * @param a1 first templated input parameter for callback implementation
     * @param a2 second templated input parameter for callback implementation
     * @param a3 third templated input parameter for the callback implementation
     * @param a4 fourth templated input parameter for the callback implementation
     * @param a5 fifth templated input parameter for the callback implementation
     * @param a6 sixth templated input parameter for the callback implementation
     * @param a7 seventh templated input parameter for the callback implementation
     * @param a8 eight templated input parameter for the callback implementation
     * @param a9 ninth templated input parameter for the callback implementation
     * @param a10 tenth templated input parameter for the callback implementation
     * @param a11 eleventh templated input parameter for the callback implementation
     *
     * @return return the value specified in the template R type
     */
    R operator()(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6, T7 a7, T8 a8, T9 a9, T10 a10, T11 a11)
    {
        return (*m_object.*m_member)(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    }

  private:
    /**
     * A pointer to the "this" pointer for the callee object
     */
    T* m_object;
    /**
     * A member of the callee object with eleven parameters
     */
    R (T::* m_member)(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11);
};
} // namespace ajn

#endif // _CALLBACK_H
