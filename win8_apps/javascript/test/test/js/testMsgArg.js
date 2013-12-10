/*
 * Copyright (c) 2012, AllSeen Alliance. All rights reserved.
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
 */

(function () {
    AllJoyn.MsgArg.setTypeCoercionMode("weak");

    // Array of length 0
    {
        var msgArgs = new AllJoyn.MsgArg('as', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('ah', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('ao', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('as', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('ag', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('ab', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('ad', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('ai', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('an', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('aq', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('at', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('au', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('ax', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('ay', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('a{si}', new Array(new Array()));
        var msgArgs = new AllJoyn.MsgArg('a(si)', new Array(new Array()));
    }
    // Hex string to numeric value
    {
        var args = new Array();
        args[0] = "0x35";  // ==> 0x35
        var msgArg0 = AllJoyn.MsgArg("n", args);
        var msgArg1 = AllJoyn.MsgArg("i", args);
        var msgArg2 = AllJoyn.MsgArg("d", args);
        var msgArg3 = AllJoyn.MsgArg("h", args);
        var msgArg4 = AllJoyn.MsgArg("q", args);
        var msgArg5 = AllJoyn.MsgArg("u", args);
        var msgArg6 = AllJoyn.MsgArg("x", args);
        var msgArg7 = AllJoyn.MsgArg("y", args);
        var msgArg8 = AllJoyn.MsgArg("t", args);
    }

    {
        var args = new Array();
        args[0] = "011";  // ==> 9
        var msgArg0 = AllJoyn.MsgArg("n", args);
        var msgArg1 = AllJoyn.MsgArg("i", args);
        var msgArg2 = AllJoyn.MsgArg("d", args);
        var msgArg3 = AllJoyn.MsgArg("h", args);
        var msgArg4 = AllJoyn.MsgArg("q", args);
        var msgArg5 = AllJoyn.MsgArg("u", args);
        var msgArg6 = AllJoyn.MsgArg("x", args);
        var msgArg7 = AllJoyn.MsgArg("y", args);
        var msgArg8 = AllJoyn.MsgArg("t", args);
    }

    {
        var args = new Array();
        args[0] = "42.2";
        var msgArg0 = AllJoyn.MsgArg("n", args);
        var msgArg1 = AllJoyn.MsgArg("i", args);
        var msgArg2 = AllJoyn.MsgArg("d", args);
        var msgArg3 = AllJoyn.MsgArg("h", args);
        var msgArg4 = AllJoyn.MsgArg("q", args);
        var msgArg5 = AllJoyn.MsgArg("u", args);
        var msgArg6 = AllJoyn.MsgArg("x", args);
        var msgArg7 = AllJoyn.MsgArg("y", args);
        var msgArg8 = AllJoyn.MsgArg("t", args);
    }

    // Boolean
    {
        var args = new Array();
        args[0] = "false";
        var msgArg = AllJoyn.MsgArg("b", args);
    }

    // ALLJOYN_BOOLEAN_ARRAY
    {
        var arr = new Array();
        arr[0] = "False";        // ==> false
        arr[1] = "tRUE";         // ==> true
        arr[2] = "1";            // ==> true
        arr[3] = "0";            // ==> false
        arr[4] = 1;              // ==> true
        arr[5] = 0;              // ==> false
        arr[6] = true;           // ==> true
        arr[7] = false;          // ==> false
        var msgArg = new AllJoyn.MsgArg("ab", new Array(arr));
    }

    // ALLJOYN_DOUBLE_ARRAY
    {
        var arr = new Array();
        arr[0] = -.36254e+2;
        arr[1] = "556.32e-11";  // ==> 36.254
        arr[2] = "0x781";       // ==> 0x781
        arr[3] = ".22";         // ==> 0.22
        arr[4] = "0.45e2";      // ==> 45
        arr[5] = "-1e-4";       // ==> -0.0001
        var msgArg = new AllJoyn.MsgArg("ad", new Array(arr));
    }

    // ALLJOYN_INT32_ARRAY
    {
        var arr = new Array();
        arr[0] = 21;
        arr[1] = -0x9985622;
        arr[2] = 1.35e1;     // ==> 13
        arr[3] = "0x781";    // ==> 781
        arr[4] = "1.35e+2";  // ==> 135
        arr[5] = "-2";       // ==> -2
        arr[6] = "-011";     // ==> -9
        arr[7] = "+011";     // ==> 9
        var msgArg = new AllJoyn.MsgArg("ai", new Array(arr));
    }

    // ALLJOYN_UINT32_ARRAY
    {
        var arr = new Array();
        arr[0] = 21;
        arr[1] = 22;
        var msgArg = new AllJoyn.MsgArg("au", new Array(arr));
    }

    // ALLJOYN_INT16_ARRAY
    {
        var arr = new Array();
        arr[0] = 21.25;        // ==> 21
        arr[1] = "-22.23e2";   // ==> -2223
        var msgArg = new AllJoyn.MsgArg("an", new Array(arr));
    }

    // ALLJOYN_UINT16_ARRAY
    {
        var arr = new Array();
        arr[0] = 21;
        arr[1] = "22";  // ==> 22
        var msgArg = new AllJoyn.MsgArg("aq", new Array(arr));
    }

    // ALLJOYN_UINT64_ARRAY
    {
        var arr = new Array();
        arr[0] = 21;
        arr[1] = "22";  // ==> 22
        var msgArg = new AllJoyn.MsgArg("at", new Array(arr));
    }

    // ALLJOYN_INT64_ARRAY
    {
        var arr = new Array();
        arr[0] = 21;
        arr[1] = "22";  // ==> 22
        var msgArg = new AllJoyn.MsgArg("ax", new Array(arr));
    }

    // ALLJOYN_BYTE_ARRAY
    {
        var arr = new Array();
        arr[0] = 21;
        arr[1] = "22";  // ==> 22
        var msgArg = new AllJoyn.MsgArg("ay", new Array(arr));
    }

    // AllJoyn String
    {
        var args = new Array();
        args[0] = 12;  // ==> "12"
        var msgArg = new AllJoyn.MsgArg("s", args);
    }

    // AllJoyn String Array
    {
        var arr = new Array();
        arr[0] = "hello";
        arr[1] = 12;        // ==> "12"
        arr[2] = false;     // ==> "0"
        arr[3] = -32.25;    // ==> "-32.25"
        var msgArg = new AllJoyn.MsgArg("as", new Array(arr));
    }

    // struct
    {
        var struct_args = new Array();
        struct_args[0] = "hello";
        struct_args[1] = 42;
        var struct = new AllJoyn.MsgArg("(si)", struct_args);
    }

    // struct 2
    {
        var arg1 = new AllJoyn.MsgArg("y", [42]);
        var arg2 = new AllJoyn.MsgArg("b", [false]);
        var args = new Array(arg1, arg2);
        var struct = new AllJoyn.MsgArg("r", new Array(args));
    }

    // Dict Entry
    {
        var val = new AllJoyn.MsgArg("y", [42]);
        var key = new AllJoyn.MsgArg("b", new Array(true));
        var dictE = new AllJoyn.MsgArg("e", new Array(key, val));
    }

    // Dict Entry 2
    {
        var args = new Array();
        args[0] = 42;
        args[1] = false;
        var dictE = new AllJoyn.MsgArg("{(y)(b)}", args);
    }

    // ALLJOYN_ARRAY
    {
        var struct_args = new Array();
        struct_args[0] = "hello";
        struct_args[1] = 42;  // ==> "42"
        var struct1 = new AllJoyn.MsgArg("(si)", struct_args);

        var struct_args2 = new Array();
        struct_args2[0] = "world";
        struct_args2[1] = 43; // ==> "43"
        var struct2 = new AllJoyn.MsgArg("(si)", struct_args2);

        ajstruct_args = new Array(struct1, struct2);
        var ajstruct = new AllJoyn.MsgArg("a(si)", new Array(ajstruct_args));
    }

})();
