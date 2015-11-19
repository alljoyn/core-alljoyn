/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
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
/*
 * The following code was developed with the documentation. Please don't modify
 * this code unless fixing errors in the code or documentation.
 */
/*
 * This sample demonstrates how to use MsgArg.
 * The sample code shows how to create containers of basic AllJoyn types and
 * AllJoyn container types.
 *
 * This sample also includes short examples on using MsgArg::GetElement and
 * MsgArg::Stabilize.
 */
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <utility>

#include <alljoyn/MsgArg.h>
#include <alljoyn/version.h>
#include <Status.h>

using namespace std;
using namespace ajn;

int CDECL_CALL main()
{
    /*
     * Brackets are used here for scooping purposes.  The values defined in
     * brackets should only exist while in the brackets.
     */
    /*
     * Code showing how to use MsgArgs for all basic types.
     */
    {
        /// [msgarg_basic_types]
        /* BYTE */
        uint8_t y = 0;
        /* BOOLEAN */
        bool b = true;
        /* INT16 */
        int16_t n = 42;
        /* UINT16 */
        uint16_t q = 0xBEBE;
        /* DOUBLE */
        double d = 3.14159265L;
        /* INT32 */
        int32_t i = -9999;
        /* UINT32 */
        uint32_t u = 0x32323232;
        /* INT64 */
        int64_t x = -1LL;
        /* UINT64 */
        uint64_t t = 0x6464646464646464ULL;
        /* STRING */
        const char* s = "this is a string";
        /* OBJECT_PATH */
        const char* o = "/org/foo/bar";
        /* SIGNATURE */
        const char* g = "a{is}d(siiux)";
        /// [msgarg_basic_types]

        /*
         * creating MsgArgs of basic types
         */
        /// [create_basic_types]
        MsgArg arg_y("y", y);         /* BYTE */
        MsgArg arg_b("b", b);         /* BOOLEAN */
        MsgArg arg_n("n", n);         /* INT16 */
        MsgArg arg_q("q", q);         /* UINT16 */
        MsgArg arg_d("d", d);         /* DOUBLE */
        MsgArg arg_i("i", i);         /* INT32 */
        MsgArg arg_u("u", u);         /* UINT32 */
        MsgArg arg_x("x", x);         /* INT64 */
        MsgArg arg_t("t", t);         /* UINT64 */
        MsgArg arg_s("s", s);         /* STRING */
        MsgArg arg_o("o", o);         /* OBJECT_PATH */
        MsgArg arg_g("g", g);         /* SIGNATURE */
        /// [create_basic_types]
        /*
         * creating MsgArgs for basic types using MsgArg.Set() method
         */
        /// [set_basic_types]
        MsgArg arg1;
        arg1.Set("i", i);         /* INT32 */
        MsgArg arg2;
        arg2.Set("s", s);         /* STRING */
        /// [set_basic_types]
        /*
         * pulling values out of a MsgArg using the MsgArg.Get() method
         */
        /// [get_basic_types]
        uint8_t my_y;             /* BYTE */
        arg_y.Get("y", &my_y);
        bool my_b;                /* BOOLEAN */
        arg_b.Get("b", &my_b);
        int16_t my_n;             /* INT16 */
        arg_n.Get("n", &my_n);
        uint16_t my_q;            /* UINT16 */
        arg_q.Get("q", &my_q);
        double my_d;              /* DOUBLE */
        arg_d.Get("d", &my_d);
        int32_t my_i;             /* INT32 */
        arg_i.Get("i", &my_i);
        uint32_t my_u;            /* UINT32 */
        arg_u.Get("u", &my_u);
        int64_t my_x;             /* INT64 */
        arg_x.Get("x", &my_x);
        uint64_t my_t;            /* UINT64 */
        arg_t.Get("t", &my_t);
        char* my_s;               /* STRING */
        arg_s.Get("s", &my_s);
        char* my_o;               /* OBJECT_PATH */
        arg_o.Get("o", &my_o);
        char* my_g;               /* SIGNATURE */
        arg_g.Get("g", &my_g);
        /// [get_basic_types]

        /*
         * checking the QStatus return value for the MsgArg.Set() and MsgArg.Get
         * method.
         */
        {
            /// [basic_get_set_with_error_checking]
            /* INT32 */
            int32_t i = -9999;
            MsgArg arg;
            QStatus status = ER_OK;
            status = arg.Set("i", i);
            if (status != ER_OK) {
                exit(status);
            }
            int32_t my_i;
            status = arg.Get("i", &my_i);
            if (status != ER_OK) {
                exit(status);
            }
            /// [basic_get_set_with_error_checking]
        }
    } //end basic types

    /*
     * Code showing how to use MsgArgs for arrays of basic types.
     */
    {
        /// [array_container_types]
        /* Array of BYTE */
        static uint8_t ay[] = { 9, 19, 29, 39, 49 };
        /* Array of INT16 */
        static int16_t an[] = { -9, -99, 999, 9999 };
        /* Array of INT32 */
        static int32_t ai[] = { -8, -88, 888, 8888 };
        /* Array of INT64 */
        static int64_t ax[] = { -8, -88, 888, 8888 };
        /* Array of DOUBLE */
        static double ad[] = { 0.001, 0.01, 0.1, 1.0, 10.0, 100.0 };
        /* Array of STRING */
        static const char* as[] = { "one", "two", "three", "four" };
        /* Array of OBJECT_PATH */
        static const char* ao[] = { "/org/one", "/org/two", "/org/three", "/org/four" };
        /* Array of SIGNATURE */
        static const char* ag[] = { "s", "sss", "as", "a(iiiiuu)" };

        /*
         * Showing how to create a MsgArg array for most AllJoyn data types.
         * Note: the MsgArg.Set() method returns a QStatus that indicates
         * success or failure when trying to create a new MsgArg the code shown
         * bellow does not check the return status.
         */
        MsgArg arg_ay;
        arg_ay.Set("ay", 5, ay);
        MsgArg arg_an;
        arg_an.Set("an", 4, an);
        MsgArg arg_ai;
        arg_ai.Set("ai", 4, ai);
        MsgArg arg_ax;
        arg_ax.Set("ax", 4, ax);
        MsgArg arg_ad;
        arg_ad.Set("ad", 6, ad);
        MsgArg arg_as;
        arg_as.Set("as", 4, as);
        MsgArg arg_ag;
        arg_ag.Set("ag", 4, ag);
        MsgArg arg_ao;
        arg_ao.Set("ao", 4, ao);
        /// [array_container_types]

        /*
         * Obtaining values from a MsgArg array for scalar types.
         * scalar types are indicated by the following AllJoyn type signatures:
         * 'y' (byte), 'b' (boolean), 'n' (int16), 'q' (uint16), 'd' (double),
         * 'i' (int32), 'u' (uint32), 'x' (int64), and 't' (uint64).
         * all other types are non-scalar types.
         */
        /// [get_arrays_of_scalars]
        /*
         * Arrays of scalars
         */
        QStatus status = ER_OK;
        uint8_t* pay;
        size_t lay;
        status = arg_ay.Get("ay", &lay, &pay);

        if (status == ER_OK) {
            uint16_t* pan;
            size_t lan;
            status = arg_an.Get("an", &lan, &pan);
        }
        if (status == ER_OK) {
            int32_t* pai;
            size_t lai;
            status = arg_ai.Get("ai", &lai, &pai);
        }
        if (status == ER_OK) {
            int64_t* pax;
            size_t lax;
            status = arg_ax.Get("ax", &lax, &pax);
        }
        if (status == ER_OK) {
            double* pad;
            size_t lad;
            status = arg_ad.Get("ad", &lad, &pad);
        }
        if (status != ER_OK) {
            printf("Error pulling value from a scalar array.\n");
            exit(status);
        }
        /// [get_arrays_of_scalars]
        /// [get_arrays_of_non_scalars]
        /*
         * Obtaining values from non-scalar arrays of basic AllJoyn types.
         */
        MsgArg* asArray;
        char** pas;         //pointer to a char* i.e. an array of strings
        size_t las;
        status = arg_as.Get("as", &las, &asArray);
        pas = new char*[las];         //allocate a char* for each string
        if (status == ER_OK) {
            for (size_t i = 0; i < las; ++i) {
                status = asArray[i].Get("s", &pas[i]);
                if (status != ER_OK) {
                    break;
                }
            }
        }

        MsgArg* aoArray;
        char** pao;         //pointer to a char* i.e. an array of strings
        size_t lao;
        status = arg_ao.Get("ao", &lao, &aoArray);
        pao = new char*[lao];         //allocate a char* for each string
        if (status == ER_OK) {
            for (size_t i = 0; i < lao; ++i) {
                status = aoArray[i].Get("s", &pao[i]);
                if (status != ER_OK) {
                    break;
                }
            }
        }

        MsgArg* agArray;
        char** pag;         //pointer to a char* i.e. an array of strings
        size_t lag;
        status = arg_ag.Get("ag", &lag, &agArray);
        pag = new char*[lag];         //allocate a char* for each string
        if (status == ER_OK) {
            for (size_t i = 0; i < lag; ++i) {
                status = agArray[i].Get("g", &pas[i]);
                if (status != ER_OK) {
                    break;
                }
            }
        }
        /// [get_arrays_of_non_scalars]
        QCC_UNUSED(pag);
    } //end arrays of basic types

    /*
     * Code showing how to use MsgArgs for struct data types
     */
    {
        /// [set_get_structs]
        QStatus status = ER_OK;
        MsgArg structArg;
        status = structArg.Set("(siii)", "hello", 1, 2, 3);

        struct {
            char* my_str;
            int my_int1;
            int my_int2;
            int my_int3;
        } my_struct;

        if (status == ER_OK) {
            structArg.Get("(siii)", &my_struct.my_str, &my_struct.my_int1, &my_struct.my_int2, &my_struct.my_int3);
        }

        /* BYTE */
        uint8_t y = 0;
        /* BOOLEAN */
        bool b = true;
        /* INT16 */
        int16_t n = 42;
        /* UINT16 */
        uint16_t q = 0xBEBE;
        /* DOUBLE */
        double d = 3.14159265L;
        /* INT32 */
        int32_t i = -9999;
        /* UINT32 */
        uint32_t u = 0x32323232;
        /* INT64 */
        int64_t x = -1LL;
        /* UINT64 */
        uint64_t t = 0x6464646464646464ULL;
        /* STRING */
        const char* s = "this is a string";
        /* OBJECT_PATH */
        const char* o = "/org/foo/bar";
        /* SIGNATURE */
        const char* g = "a{is}d(siiux)";

        struct basic_types {
            uint8_t y;
            bool b;
            int16_t n;
            uint16_t q;
            double d;
            int32_t i;
            uint32_t u;
            int64_t x;
            uint64_t t;
            char* s;
            char* o;
            char* g;
        };

        /*
         * creating a struct from basic AllJoyn types.  We are building the
         * struct from individual varaibles we could have just as easily passed the
         * values in from a struct.
         */
        MsgArg argList;
        status = argList.Set("(ybnqdiuxtsog)", y, b, n, q, d, i, u, x, t, s, o, g);

        /*
         * pulling the values out of the MsgArg into a 'basic_types' struct
         */
        struct basic_types sOut;
        if (status == ER_OK) {

            status = argList.Get("(ybnqdiuxtsog)", &sOut.y, &sOut.b, &sOut.n,
                                 &sOut.q, &sOut.d, &sOut.i, &sOut.u, &sOut.x,
                                 &sOut.t, &sOut.s, &sOut.o, &sOut.g);
        }
        /// [set_get_structs]
    } //end basic struct

    /*
     * Code showing how to create dictionary container types
     */
    {
        /// [set_get_dictionary]
        /*
         * Sample dictionary mapping the number of the month to its string
         * representation
         */
        size_t numberMonths = 12;
        MsgArg* months = new MsgArg[numberMonths];

        months[0].Set("{us}", 1, "January");
        months[1].Set("{us}", 2, "February");
        months[2].Set("{us}", 3, "March");
        months[3].Set("{us}", 4, "April");
        months[4].Set("{us}", 5, "May");
        months[5].Set("{us}", 6, "June");
        months[6].Set("{us}", 7, "July");
        months[7].Set("{us}", 8, "August");
        months[8].Set("{us}", 9, "September");
        months[9].Set("{us}", 10, "October");
        months[10].Set("{us}", 11, "November");
        months[11].Set("{us}", 12, "December");

        MsgArg monthDictionary;
        monthDictionary.Set("a{us}", numberMonths, months);

        /*
         * pulling the dictionary values out of the MsgArg
         */
        MsgArg* entries;
        size_t num;
        QStatus status;
        status = monthDictionary.Get("a{us}", &num, &entries);

        pair<int, char*>*monthsOut;
        monthsOut = new pair<int, char*>[num];

        if (status == ER_OK) {
            for (size_t i = 0; i < num; ++i) {
                uint32_t key;
                char* value;
                status = entries[i].Get("{us}", &key, &value);
                if (status == ER_OK) {
                    monthsOut[i] = make_pair(key, value);
                }
            }
        }
        /// [set_get_dictionary]
        /// [dictionary_getelement]
        /*
         * using MsgArg::GetElement method to read a single value given its key.
         * This will read the month with the key 10 from the monthDictionary.
         */
        char* currentMonth;
        monthDictionary.GetElement("{us}", 10, &currentMonth);
        /// [dictionary_getelement]
    } //end dictionary

    /*
     * Code showing how to use set and read Variant data types
     */
    {
        /// [get_set_variant]
        QStatus status;
        for (uint8_t n = 0; n < 3; ++n) {
            MsgArg arg;

            /*
             * fill the MsgArg 'arg' with with a different value depending on
             * what the case we are in in the switch statement.
             */
            switch (n) {
            case 0:
                arg.Set("v", new MsgArg("i", 42));
                break;

            case 1:
                arg.Set("v", new MsgArg("d", 3.14159));
                break;

            case 2:
                arg.Set("v", new MsgArg("s", "hello world"));
                break;
            }

            /*
             * Read the value from the MsgArg.  We don't know what value it
             * contains but we know it is one of three values int32, double, or
             * a string.  We will try to read each value from the variant till
             * we are successful.
             */
            int32_t my_i;
            double my_d;
            char* my_s;
            status = arg.Get("i", &my_i);
            if (status == ER_BUS_SIGNATURE_MISMATCH) {
                status = arg.Get("s", &my_s);
                if (status == ER_BUS_SIGNATURE_MISMATCH) {
                    status = arg.Get("d", &my_d);
                }
            }
            if (status != ER_OK) {
                printf("Unable to read value from variant.\n");
                break;
            }
        }
        /// [get_set_variant]
    } //end Variant

    /*
     * Nested containers.  i.e. container of containers
     */
    {
        /// [nested_structs]
        /*
         * Nested structs
         */
        QStatus status = ER_OK;
        MsgArg struct1;
        status = struct1.Set("((dub)(i(ss)))", 3.14159, 42, false, 128, "hello", "world");
        if (status == ER_OK) {
            /*
             * creating a C++ struct that means the same thing as the signature
             * "((dub)(i(ss)))"
             * Here nested structures are being used when in typical code the
             * inner structs would be pointers to some data structure that has
             * actual meaning.
             */
            struct {
                struct {
                    double d;
                    uint32_t u;
                    bool b;
                } innerA;
                struct {
                    int32_t i;
                    struct {
                        char* str1;
                        char* str2;
                    } innerA;
                } innerB;
            } s1_out;
            status = struct1.Get("((dub)(i(ss)))", &s1_out.innerA.d, &s1_out.innerA.u,
                                 &s1_out.innerA.b, &s1_out.innerB.i,
                                 &s1_out.innerB.innerA.str1, &s1_out.innerB.innerA.str2);
        }
        /// [nested_structs]
        /// [nested_structs_of_arrays]
        /*
         * Nested structs with a nested array.
         */
        MsgArg struct2;
        int32_t au[] = { 0, 1, 1, 2, 3, 5, 8 };
        status = struct2.Set("((iuiu)au)", 17, 58, -1, 42, sizeof(au) / sizeof(au[0]), au);
        if (status == ER_OK) {
            /*
             * Creating a C++ strut that means the same thing as the signature
             * "((iuiu)au)"
             * Note the array requires two items in the struct.  An array pointer
             * and an size_t array length element
             */
            struct {
                struct {
                    int32_t i1;
                    uint32_t u1;
                    int32_t i2;
                    uint32_t u2;
                } innerA;
                uint32_t* pau;
                size_t pau_length;
            } s2_out;

            status = struct2.Get("((iuiu)(yd)at)", &s2_out.innerA.i1, &s2_out.innerA.i2,
                                 &s2_out.innerA.u1,
                                 &s2_out.innerA.u2, &s2_out.pau, &s2_out.pau_length);
        }
        /// [nested_structs_of_arrays]
        /// [nested_arrays]
        /*
         * Multi-dimensional arrays or arrays of arrays "aai"
         * we want to make an array of arrays that contains the value represented
         * here.
         * {{0, 1, 1, 2, 3, 5, 8},
         *  {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61,
         *   67, 71, 73, 79, 83, 89, 97}}
         * (First 7 numbers of the Fibonacci sequence, and primes 0 to 100)
         * C++ does not have a simple to use container for ragged arrays. A
         * ragged array must be stored using pointer manipulation or a container
         * like std::vector. There are many solutions for ragged arrays in other
         * libraries.  For this example we are interested on how to build the
         * MsgArg that holds the multi-dimensional array and how to pull the values
         * out of the MsgArg.  Deciding how to represent the multi-dimensional
         * array is represented in the code is up to the programmer. For this
         * example the MsgArg will be build from individual arrays and read from
         * the MsgArg into a non-ragged array of size [2][25].
         */
        int32_t inner1[] = { 0, 1, 1, 2, 3, 5, 8 };
        int32_t inner2[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
                             47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97 };

        status = ER_OK;
        MsgArg* int_array = new MsgArg[2];
        status = int_array[0].Set("ai", sizeof(inner1) / sizeof(inner1[0]), inner1);
        if (status == ER_OK) {
            int_array[1].Set("ai", sizeof(inner2) / sizeof(inner2[0]), inner2);
        }

        MsgArg aai;
        if (status == ER_OK) {
            status = aai.Set("aai", 2, int_array);
        }

        if (status != ER_OK) {
            printf("Error packaging MsgArg.\n");
        }

        int32_t aai_out[2][25];
        size_t aai_out_len[2];

        MsgArg*argout;
        size_t argout_len;

        aai.Get("aai", &argout_len, &argout);
        if (argout_len > 2) {
            printf("MsgArg aai contains a larger array than expected.\n");
        }

        for (size_t i = 0; i < argout_len; ++i) {
            if (status == ER_OK) {
                int32_t* pai;
                status = argout[i].Get("ai", &aai_out_len[i], &pai);
                for (size_t j = 0; j < aai_out_len[i]; ++j) {
                    aai_out[i][j] = pai[j];
                }
            }
        }

        if (status != ER_OK) {
            printf("Error reading value out of MsgArg.\n");
        }
        /// [nested_arrays]
        QCC_UNUSED(aai_out);
        /// [array_of_structs]
        /*
         * An array of another container.  a(ss)
         * When building an array of another conainer you simply make an array
         * of MsgArgs that each hold the container then place them in the array.
         */
        MsgArg struct_elements[2];
        status = struct_elements[0].Set("(ss)", "Hello ", "world!");
        if (status == ER_OK) {
            struct_elements[1].Set("(ss)", "The Eagle ", "has landed.");
        }
        MsgArg array_struct;
        if (status == ER_OK) {
            array_struct.Set("a(ss)", 2, struct_elements);
        }

        struct MyStruct {
            char* str1;
            char* str2;
        };

        MyStruct myStruct[2];
        MsgArg*struct_elements_out;
        size_t struct_elements_len;

        array_struct.Get("a(ss)", &struct_elements_len, &struct_elements_out);
        for (size_t i = 0; i < struct_elements_len; ++i) {
            struct_elements_out[i].Get("(ss)", &myStruct[i].str1, &myStruct[i].str2);
        }
        /// [array_of_structs]
        /// [variant_of_struct]
        /*
         * a variant that is a container v == (ss)
         */
        MsgArg variant_container;
        variant_container.Set("v", new MsgArg("(ss)", "Hello ", "world"));

        MyStruct out;
        status = variant_container.Get("(ss)", &out.str1, &out.str2);
        /*
         * Should always check the return value when using variants.
         * The most common return other than ER_OK is ER_BUS_SIGNATURE_MISMATCH
         * This indicates that the signature you are trying to read is not the
         * signature that is held by the variant value.
         */
        if (status != ER_OK) {
            printf("Error reading value from variant container.\n");
        }
        /// [variant_of_struct]
    } //end nested containers

    /*
     * Example Showing the use of MsgArg::Stabilize
     */
    {
        /// [str_pointer_not_stabilized]
        MsgArg arg;
        {
            const char* str_pointer = "Hello";
            arg.Set("s", str_pointer);
        }
        /*
         * the message 'arg' is no longer valid because the value 'str_pointer'
         * has gone out of scope and is no longer a valid pointer. Trying to use
         * 'arg' after this point would result in undefined behavior.
         */
        /// [str_pointer_not_stabilized]
        /// [str_pointer_stabilized]
        {
            const char* str_pointer = "GoodBye";
            arg.Set("s", str_pointer);
            arg.Stabilize();
            str_pointer = NULL;
        }
        /*
         * Since MsgArg.Stabilize was called on the MsgArg before the string
         * pointed to went out of scope the contents of str_pointer were copied
         * into MsgArg.  Using the Stabilize method creates a copy of anything
         * MsgArg is pointing to. Stabilize can be an expensive operation both
         * in time and the amount of memory that is used.
         */
        char* out_str;
        arg.Get("s", &out_str);
        /// [str_pointer_stabilized]

    } //end MsgArg::Stabilize
    return 0;
}
