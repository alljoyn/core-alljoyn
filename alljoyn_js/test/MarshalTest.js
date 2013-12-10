/*
 * Copyright (c) 2011, 2013, AllSeen Alliance. All rights reserved.
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
AsyncTestCase("MarshalTest", {
    _setUp: ondeviceready(function(callback) {
        otherBus = undefined;
        bus = new org.alljoyn.bus.BusAttachment();
        bus.create(false, callback);
    }),
    tearDown: function() {
        otherBus && otherBus.destroy();
        bus.destroy();
    },

    testBasic: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                bus.createInterface({ name: 'interface.b', method: [ { name: 'Methodb', signature: 'b', returnSignature: 'b' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.d', method: [ { name: 'Methodd', signature: 'd', returnSignature: 'd' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.g', method: [ { name: 'Methodg', signature: 'g', returnSignature: 'g' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.i', method: [ { name: 'Methodi', signature: 'i', returnSignature: 'i' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.n', method: [ { name: 'Methodn', signature: 'n', returnSignature: 'n' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.o', method: [ { name: 'Methodo', signature: 'o', returnSignature: 'o' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.q', method: [ { name: 'Methodq', signature: 'q', returnSignature: 'q' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.s', method: [ { name: 'Methods', signature: 's', returnSignature: 's' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.t', method: [ { name: 'Methodt', signature: 't', returnSignature: 't' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.u', method: [ { name: 'Methodu', signature: 'u', returnSignature: 'u' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.x', method: [ { name: 'Methodx', signature: 'x', returnSignature: 'x' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.y', method: [ { name: 'Methody', signature: 'y', returnSignature: 'y' } ] }, callbacks.add(registerBusObject))
                }))}))}))}))}))}))}))}))}))}))}))};
            var registerBusObject = function(err) {
                bus.registerBusObject('/testObject',
                                      {
                                          'interface.b': { Methodb: function(context, arg) { context.reply(arg); } },
                                          'interface.d': { Methodd: function(context, arg) { context.reply(arg); } },
                                          'interface.g': { Methodg: function(context, arg) { context.reply(arg); } },
                                          'interface.i': { Methodi: function(context, arg) { context.reply(arg); } },
                                          'interface.n': { Methodn: function(context, arg) { context.reply(arg); } },
                                          'interface.o': { Methodo: function(context, arg) { context.reply(arg); } },
                                          'interface.q': { Methodq: function(context, arg) { context.reply(arg); } },
                                          'interface.s': { Methods: function(context, arg) { context.reply(arg); } },
                                          'interface.t': { Methodt: function(context, arg) { context.reply(arg); } },
                                          'interface.u': { Methodu: function(context, arg) { context.reply(arg); } },
                                          'interface.x': { Methodx: function(context, arg) { context.reply(arg); } },
                                          'interface.y': { Methody: function(context, arg) { context.reply(arg); } }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + '/testObject', callbacks.add(methodb));
            };
            var proxy;
            var methodb = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                proxy.methodCall('interface.b', 'Methodb', true, callbacks.add(onReplyb));
            };
            var onReplyb = function(err, context, argb) {
                assertFalsy(err);
                assertEquals(true, argb);
                proxy.methodCall('interface.d', 'Methodd', 1.234, callbacks.add(onReplyd));
            };
            var onReplyd = function(err, context, argd) {
                assertFalsy(err);
                assertEquals(1.234, argd);
                proxy.methodCall('interface.g', 'Methodg', "sig", callbacks.add(onReplyg));
            };
            var onReplyg = function(err, context, argg) {
                assertFalsy(err);
                assertEquals("sig", argg);
                proxy.methodCall('interface.i', 'Methodi', -1, callbacks.add(onReplyi));
            };
            var onReplyi = function(err, context, argi) {
                assertFalsy(err);
                assertEquals(-1, argi);
                proxy.methodCall('interface.n', 'Methodn', -2, callbacks.add(onReplyn));
            };
            var onReplyn = function(err, context, argn) {
                assertFalsy(err);
                assertEquals(-2, argn);
                proxy.methodCall('interface.o', 'Methodo', "/path", callbacks.add(onReplyo));
            };
            var onReplyo = function(err, context, argo) {
                assertFalsy(err);
                assertEquals("/path", argo);
                proxy.methodCall('interface.q', 'Methodq', 3, callbacks.add(onReplyq));
            };
            var onReplyq = function(err, context, argq) {
                assertFalsy(err);
                assertEquals(3, argq);
                proxy.methodCall('interface.s', 'Methods', "string", callbacks.add(onReplys));
            };
            var onReplys = function(err, context, args) {
                assertFalsy(err);
                assertEquals("string", args);
                proxy.methodCall('interface.t', 'Methodt', 4, callbacks.add(onReplyt));
            };
            var onReplyt = function(err, context, argt) {
                assertFalsy(err);
                assertEquals(4, argt);
                proxy.methodCall('interface.u', 'Methodu', 5, callbacks.add(onReplyu));
            };
            var onReplyu = function(err, context, argu) {
                assertFalsy(err);
                assertEquals(5, argu);
                proxy.methodCall('interface.x', 'Methodx', -6, callbacks.add(onReplyx));
            };
            var onReplyx = function(err, context, argx) {
                assertFalsy(err);
                assertEquals(-6, argx);
                proxy.methodCall('interface.y', 'Methody', 7, callbacks.add(onReplyy));
            };
            var onReplyy = function(err, context, argy) {
                assertFalsy(err);
                assertEquals(7, argy);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testArray: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                bus.createInterface({ name: 'interface.ab', method: [ { name: 'Methodab', signature: 'ab', returnSignature: 'ab' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ad', method: [ { name: 'Methodad', signature: 'ad', returnSignature: 'ad' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ag', method: [ { name: 'Methodag', signature: 'ag', returnSignature: 'ag' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ai', method: [ { name: 'Methodai', signature: 'ai', returnSignature: 'ai' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.an', method: [ { name: 'Methodan', signature: 'an', returnSignature: 'an' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ao', method: [ { name: 'Methodao', signature: 'ao', returnSignature: 'ao' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.aq', method: [ { name: 'Methodaq', signature: 'aq', returnSignature: 'aq' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.as', method: [ { name: 'Methodas', signature: 'as', returnSignature: 'as' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.at', method: [ { name: 'Methodat', signature: 'at', returnSignature: 'at' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.au', method: [ { name: 'Methodau', signature: 'au', returnSignature: 'au' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ax', method: [ { name: 'Methodax', signature: 'ax', returnSignature: 'ax' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ay', method: [ { name: 'Methoday', signature: 'ay', returnSignature: 'ay' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.aas', method: [ { name: 'Methodaas', signature: 'aas', returnSignature: 'aas' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ae', method: [ { name: 'Methodae', signature: 'aa{ss}', returnSignature: 'aa{ss}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ar', method: [ { name: 'Methodar', signature: 'a(s)', returnSignature: 'a(s)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.av', method: [ { name: 'Methodav', signature: 'av', returnSignature: 'av' } ] }, callbacks.add(registerBusObject))
                }))}))}))}))}))}))}))}))}))}))}))}))}))}))}))};
            var registerBusObject = function(err) {
                bus.registerBusObject('/testObject',
                                      {
                                          'interface.ab': { Methodab: function(context, arg) { context.reply(arg); } },
                                          'interface.ad': { Methodad: function(context, arg) { context.reply(arg); } },
                                          'interface.ag': { Methodag: function(context, arg) { context.reply(arg); } },
                                          'interface.ai': { Methodai: function(context, arg) { context.reply(arg); } },
                                          'interface.an': { Methodan: function(context, arg) { context.reply(arg); } },
                                          'interface.ao': { Methodao: function(context, arg) { context.reply(arg); } },
                                          'interface.aq': { Methodaq: function(context, arg) { context.reply(arg); } },
                                          'interface.as': { Methodas: function(context, arg) { context.reply(arg); } },
                                          'interface.at': { Methodat: function(context, arg) { context.reply(arg); } },
                                          'interface.au': { Methodau: function(context, arg) { context.reply(arg); } },
                                          'interface.ax': { Methodax: function(context, arg) { context.reply(arg); } },
                                          'interface.ay': { Methoday: function(context, arg) { context.reply(arg); } },
                                          'interface.aas': { Methodaas: function(context, arg) { context.reply(arg); } },
                                          'interface.ae': { Methodae: function(context, arg) { context.reply(arg); } },
                                          'interface.ar': { Methodar: function(context, arg) { context.reply(arg); } },
                                          'interface.av': {
                                              Methodav: function(context, arg) {
                                                  var args = [];
                                                  for (var i = 0; i < arg.length; ++i) {
                                                      args[i] = { 's': arg[i] };
                                                  }
                                                  context.reply(args);
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + '/testObject', callbacks.add(methodab));
            };
            var proxy;
            var methodab = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                proxy.methodCall('interface.ab', 'Methodab', [ true, true ], callbacks.add(onReplyab));
            };
            var onReplyab = function(err, context, argab) {
                err && console.log('err.name = ' + err.name + '   err.code = ' + err.code);
                assertFalsy(err);
                assertEquals([ true, true ], argab);
                proxy.methodCall('interface.ad', 'Methodad', [ 1.234, 1.234 ], callbacks.add(onReplyad));
            };
            var onReplyad = function(err, context, argad) {
                assertFalsy(err);
                assertEquals([ 1.234, 1.234 ], argad);
                proxy.methodCall('interface.ag', 'Methodag', [ "sig", "sig" ], callbacks.add(onReplyag));
            };
            var onReplyag = function(err, context, argag) {
                assertFalsy(err);
                assertEquals([ "sig", "sig" ], argag);
                proxy.methodCall('interface.ai', 'Methodai', [ -1, -1 ], callbacks.add(onReplyai));
            };
            var onReplyai = function(err, context, argai) {
                assertFalsy(err);
                assertEquals([ -1, -1 ], argai);
                proxy.methodCall('interface.an', 'Methodan', [ -2, -2 ], callbacks.add(onReplyan));
            };
            var onReplyan = function(err, context, argan) {
                assertFalsy(err);
                assertEquals([ -2, -2 ], argan);
                proxy.methodCall('interface.ao', 'Methodao', [ "/path", "/path" ], callbacks.add(onReplyao));
            };
            var onReplyao = function(err, context, argao) {
                assertFalsy(err);
                assertEquals([ "/path", "/path" ], argao);
                proxy.methodCall('interface.aq', 'Methodaq', [ 3, 3 ], callbacks.add(onReplyaq));
            };
            var onReplyaq = function(err, context, argaq) {
                assertFalsy(err);
                assertEquals([ 3, 3 ], argaq);
                proxy.methodCall('interface.as', 'Methodas', [ "string", "string" ], callbacks.add(onReplyas));
            };
            var onReplyas = function(err, context, argas) {
                assertFalsy(err);
                assertEquals([ "string", "string" ], argas);
                proxy.methodCall('interface.at', 'Methodat', [ 4, 4 ], callbacks.add(onReplyat));
            };
            var onReplyat = function(err, context, argat) {
                assertFalsy(err);
                argat[0] = parseInt(argat[0]);
                argat[1] = parseInt(argat[1]);
                assertEquals([ 4, 4 ], argat);
                proxy.methodCall('interface.au', 'Methodau', [ 5, 5 ], callbacks.add(onReplyau));
            };
            var onReplyau = function(err, context, argau) {
                assertFalsy(err);
                assertEquals([ 5, 5 ], argau);
                proxy.methodCall('interface.ax', 'Methodax', [ -6, -6 ], callbacks.add(onReplyax));
            };
            var onReplyax = function(err, context, argax) {
                assertFalsy(err);
                argax[0] = parseInt(argax[0]);
                argax[1] = parseInt(argax[1]);
                assertEquals([ -6, -6 ], argax);
                proxy.methodCall('interface.ay', 'Methoday', [ 7, 7 ], callbacks.add(onReplyay));
            };
            var onReplyay = function(err, context, argay) {
                assertFalsy(err);
                assertEquals([ 7, 7 ], argay);
                proxy.methodCall('interface.aas', 'Methodaas', [ ["s0", "s1"], ["s0", "s1"] ], callbacks.add(onReplyaas));
            };
            var onReplyaas = function(err, context, argaas) {
                assertFalsy(err);
                assertEquals([ ["s0", "s1"], ["s0", "s1"] ], argaas);
                proxy.methodCall('interface.ae', 'Methodae', [ { key0: "value0", key1: "value1" }, { key0: "value0", key1: "value1" } ], callbacks.add(onReplyae));
            };
            var onReplyae = function(err, context, argae) {
                assertFalsy(err);
                assertEquals([ { key0: "value0", key1: "value1" }, { key0: "value0", key1: "value1" } ], argae);
                proxy.methodCall('interface.ar', 'Methodar', [ ["string"], ["string"] ], callbacks.add(onReplyar));
            };
            var onReplyar = function(err, context, argar) {
                assertFalsy(err);
                assertEquals([ ["string"], ["string"] ], argar);
                proxy.methodCall('interface.av', 'Methodav', [ { s: "string" }, { s: "string" } ], callbacks.add(onReplyav));
            };
            var onReplyav = function(err, context, argav) {
                err && console.log('err.name = ' + err.name + '   err.code = ' + err.code);
                assertFalsy(err);
                assertEquals([ "string", "string" ], argav);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testDictionary: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                bus.createInterface({ name: 'interface.ebb', method: [ { name: 'Methodebb', signature: 'a{bb}', returnSignature: 'a{bb}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebd', method: [ { name: 'Methodebd', signature: 'a{bd}', returnSignature: 'a{bd}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebg', method: [ { name: 'Methodebg', signature: 'a{bg}', returnSignature: 'a{bg}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebi', method: [ { name: 'Methodebi', signature: 'a{bi}', returnSignature: 'a{bi}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebn', method: [ { name: 'Methodebn', signature: 'a{bn}', returnSignature: 'a{bn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebo', method: [ { name: 'Methodebo', signature: 'a{bo}', returnSignature: 'a{bo}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebq', method: [ { name: 'Methodebq', signature: 'a{bq}', returnSignature: 'a{bq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebs', method: [ { name: 'Methodebs', signature: 'a{bs}', returnSignature: 'a{bs}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebt', method: [ { name: 'Methodebt', signature: 'a{bt}', returnSignature: 'a{bt}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebu', method: [ { name: 'Methodebu', signature: 'a{bu}', returnSignature: 'a{bu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebx', method: [ { name: 'Methodebx', signature: 'a{bx}', returnSignature: 'a{bx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eby', method: [ { name: 'Methodeby', signature: 'a{by}', returnSignature: 'a{by}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebas', method: [ { name: 'Methodebas', signature: 'a{bas}', returnSignature: 'a{bas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebe', method: [ { name: 'Methodebe', signature: 'a{ba{ss}}', returnSignature: 'a{ba{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebr', method: [ { name: 'Methodebr', signature: 'a{b(s)}', returnSignature: 'a{b(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ebv', method: [ { name: 'Methodebv', signature: 'a{bv}', returnSignature: 'a{bv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edb', method: [ { name: 'Methodedb', signature: 'a{db}', returnSignature: 'a{db}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edd', method: [ { name: 'Methodedd', signature: 'a{dd}', returnSignature: 'a{dd}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edg', method: [ { name: 'Methodedg', signature: 'a{dg}', returnSignature: 'a{dg}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edi', method: [ { name: 'Methodedi', signature: 'a{di}', returnSignature: 'a{di}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edn', method: [ { name: 'Methodedn', signature: 'a{dn}', returnSignature: 'a{dn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edo', method: [ { name: 'Methodedo', signature: 'a{do}', returnSignature: 'a{do}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edq', method: [ { name: 'Methodedq', signature: 'a{dq}', returnSignature: 'a{dq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eds', method: [ { name: 'Methodeds', signature: 'a{ds}', returnSignature: 'a{ds}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edt', method: [ { name: 'Methodedt', signature: 'a{dt}', returnSignature: 'a{dt}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edu', method: [ { name: 'Methodedu', signature: 'a{du}', returnSignature: 'a{du}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edx', method: [ { name: 'Methodedx', signature: 'a{dx}', returnSignature: 'a{dx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edy', method: [ { name: 'Methodedy', signature: 'a{dy}', returnSignature: 'a{dy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edas', method: [ { name: 'Methodedas', signature: 'a{das}', returnSignature: 'a{das}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ede', method: [ { name: 'Methodede', signature: 'a{da{ss}}', returnSignature: 'a{da{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edr', method: [ { name: 'Methodedr', signature: 'a{d(s)}', returnSignature: 'a{d(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.edv', method: [ { name: 'Methodedv', signature: 'a{dv}', returnSignature: 'a{dv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egb', method: [ { name: 'Methodegb', signature: 'a{gb}', returnSignature: 'a{gb}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egd', method: [ { name: 'Methodegd', signature: 'a{gd}', returnSignature: 'a{gd}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egg', method: [ { name: 'Methodegg', signature: 'a{gg}', returnSignature: 'a{gg}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egi', method: [ { name: 'Methodegi', signature: 'a{gi}', returnSignature: 'a{gi}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egn', method: [ { name: 'Methodegn', signature: 'a{gn}', returnSignature: 'a{gn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ego', method: [ { name: 'Methodego', signature: 'a{go}', returnSignature: 'a{go}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egq', method: [ { name: 'Methodegq', signature: 'a{gq}', returnSignature: 'a{gq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egs', method: [ { name: 'Methodegs', signature: 'a{gs}', returnSignature: 'a{gs}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egt', method: [ { name: 'Methodegt', signature: 'a{gt}', returnSignature: 'a{gt}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egu', method: [ { name: 'Methodegu', signature: 'a{gu}', returnSignature: 'a{gu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egx', method: [ { name: 'Methodegx', signature: 'a{gx}', returnSignature: 'a{gx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egy', method: [ { name: 'Methodegy', signature: 'a{gy}', returnSignature: 'a{gy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egas', method: [ { name: 'Methodegas', signature: 'a{gas}', returnSignature: 'a{gas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ege', method: [ { name: 'Methodege', signature: 'a{ga{ss}}', returnSignature: 'a{ga{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egr', method: [ { name: 'Methodegr', signature: 'a{g(s)}', returnSignature: 'a{g(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.egv', method: [ { name: 'Methodegv', signature: 'a{gv}', returnSignature: 'a{gv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eib', method: [ { name: 'Methodeib', signature: 'a{ib}', returnSignature: 'a{ib}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eid', method: [ { name: 'Methodeid', signature: 'a{id}', returnSignature: 'a{id}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eig', method: [ { name: 'Methodeig', signature: 'a{ig}', returnSignature: 'a{ig}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eii', method: [ { name: 'Methodeii', signature: 'a{ii}', returnSignature: 'a{ii}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ein', method: [ { name: 'Methodein', signature: 'a{in}', returnSignature: 'a{in}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eio', method: [ { name: 'Methodeio', signature: 'a{io}', returnSignature: 'a{io}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eiq', method: [ { name: 'Methodeiq', signature: 'a{iq}', returnSignature: 'a{iq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eis', method: [ { name: 'Methodeis', signature: 'a{is}', returnSignature: 'a{is}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eit', method: [ { name: 'Methodeit', signature: 'a{it}', returnSignature: 'a{it}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eiu', method: [ { name: 'Methodeiu', signature: 'a{iu}', returnSignature: 'a{iu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eix', method: [ { name: 'Methodeix', signature: 'a{ix}', returnSignature: 'a{ix}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eiy', method: [ { name: 'Methodeiy', signature: 'a{iy}', returnSignature: 'a{iy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eias', method: [ { name: 'Methodeias', signature: 'a{ias}', returnSignature: 'a{ias}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eie', method: [ { name: 'Methodeie', signature: 'a{ia{ss}}', returnSignature: 'a{ia{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eir', method: [ { name: 'Methodeir', signature: 'a{i(s)}', returnSignature: 'a{i(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eiv', method: [ { name: 'Methodeiv', signature: 'a{iv}', returnSignature: 'a{iv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.enb', method: [ { name: 'Methodenb', signature: 'a{nb}', returnSignature: 'a{nb}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.end', method: [ { name: 'Methodend', signature: 'a{nd}', returnSignature: 'a{nd}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eng', method: [ { name: 'Methodeng', signature: 'a{ng}', returnSignature: 'a{ng}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eni', method: [ { name: 'Methodeni', signature: 'a{ni}', returnSignature: 'a{ni}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.enn', method: [ { name: 'Methodenn', signature: 'a{nn}', returnSignature: 'a{nn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eno', method: [ { name: 'Methodeno', signature: 'a{no}', returnSignature: 'a{no}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.enq', method: [ { name: 'Methodenq', signature: 'a{nq}', returnSignature: 'a{nq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ens', method: [ { name: 'Methodens', signature: 'a{ns}', returnSignature: 'a{ns}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ent', method: [ { name: 'Methodent', signature: 'a{nt}', returnSignature: 'a{nt}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.enu', method: [ { name: 'Methodenu', signature: 'a{nu}', returnSignature: 'a{nu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.enx', method: [ { name: 'Methodenx', signature: 'a{nx}', returnSignature: 'a{nx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eny', method: [ { name: 'Methodeny', signature: 'a{ny}', returnSignature: 'a{ny}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.enas', method: [ { name: 'Methodenas', signature: 'a{nas}', returnSignature: 'a{nas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ene', method: [ { name: 'Methodene', signature: 'a{na{ss}}', returnSignature: 'a{na{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.enr', method: [ { name: 'Methodenr', signature: 'a{n(s)}', returnSignature: 'a{n(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.env', method: [ { name: 'Methodenv', signature: 'a{nv}', returnSignature: 'a{nv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eob', method: [ { name: 'Methodeob', signature: 'a{ob}', returnSignature: 'a{ob}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eod', method: [ { name: 'Methodeod', signature: 'a{od}', returnSignature: 'a{od}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eog', method: [ { name: 'Methodeog', signature: 'a{og}', returnSignature: 'a{og}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eoi', method: [ { name: 'Methodeoi', signature: 'a{oi}', returnSignature: 'a{oi}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eon', method: [ { name: 'Methodeon', signature: 'a{on}', returnSignature: 'a{on}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eoo', method: [ { name: 'Methodeoo', signature: 'a{oo}', returnSignature: 'a{oo}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eoq', method: [ { name: 'Methodeoq', signature: 'a{oq}', returnSignature: 'a{oq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eos', method: [ { name: 'Methodeos', signature: 'a{os}', returnSignature: 'a{os}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eot', method: [ { name: 'Methodeot', signature: 'a{ot}', returnSignature: 'a{ot}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eou', method: [ { name: 'Methodeou', signature: 'a{ou}', returnSignature: 'a{ou}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eox', method: [ { name: 'Methodeox', signature: 'a{ox}', returnSignature: 'a{ox}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eoy', method: [ { name: 'Methodeoy', signature: 'a{oy}', returnSignature: 'a{oy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eoas', method: [ { name: 'Methodeoas', signature: 'a{oas}', returnSignature: 'a{oas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eoe', method: [ { name: 'Methodeoe', signature: 'a{oa{ss}}', returnSignature: 'a{oa{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eor', method: [ { name: 'Methodeor', signature: 'a{o(s)}', returnSignature: 'a{o(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eov', method: [ { name: 'Methodeov', signature: 'a{ov}', returnSignature: 'a{ov}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqb', method: [ { name: 'Methodeqb', signature: 'a{qb}', returnSignature: 'a{qb}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqd', method: [ { name: 'Methodeqd', signature: 'a{qd}', returnSignature: 'a{qd}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqg', method: [ { name: 'Methodeqg', signature: 'a{qg}', returnSignature: 'a{qg}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqi', method: [ { name: 'Methodeqi', signature: 'a{qi}', returnSignature: 'a{qi}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqn', method: [ { name: 'Methodeqn', signature: 'a{qn}', returnSignature: 'a{qn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqo', method: [ { name: 'Methodeqo', signature: 'a{qo}', returnSignature: 'a{qo}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqq', method: [ { name: 'Methodeqq', signature: 'a{qq}', returnSignature: 'a{qq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqs', method: [ { name: 'Methodeqs', signature: 'a{qs}', returnSignature: 'a{qs}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqt', method: [ { name: 'Methodeqt', signature: 'a{qt}', returnSignature: 'a{qt}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.equ', method: [ { name: 'Methodequ', signature: 'a{qu}', returnSignature: 'a{qu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqx', method: [ { name: 'Methodeqx', signature: 'a{qx}', returnSignature: 'a{qx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqy', method: [ { name: 'Methodeqy', signature: 'a{qy}', returnSignature: 'a{qy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqas', method: [ { name: 'Methodeqas', signature: 'a{qas}', returnSignature: 'a{qas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqe', method: [ { name: 'Methodeqe', signature: 'a{qa{ss}}', returnSignature: 'a{qa{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqr', method: [ { name: 'Methodeqr', signature: 'a{q(s)}', returnSignature: 'a{q(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eqv', method: [ { name: 'Methodeqv', signature: 'a{qv}', returnSignature: 'a{qv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esb', method: [ { name: 'Methodesb', signature: 'a{sb}', returnSignature: 'a{sb}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esd', method: [ { name: 'Methodesd', signature: 'a{sd}', returnSignature: 'a{sd}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esg', method: [ { name: 'Methodesg', signature: 'a{sg}', returnSignature: 'a{sg}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esi', method: [ { name: 'Methodesi', signature: 'a{si}', returnSignature: 'a{si}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esn', method: [ { name: 'Methodesn', signature: 'a{sn}', returnSignature: 'a{sn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eso', method: [ { name: 'Methodeso', signature: 'a{so}', returnSignature: 'a{so}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esq', method: [ { name: 'Methodesq', signature: 'a{sq}', returnSignature: 'a{sq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ess', method: [ { name: 'Methodess', signature: 'a{ss}', returnSignature: 'a{ss}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.est', method: [ { name: 'Methodest', signature: 'a{st}', returnSignature: 'a{st}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esu', method: [ { name: 'Methodesu', signature: 'a{su}', returnSignature: 'a{su}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esx', method: [ { name: 'Methodesx', signature: 'a{sx}', returnSignature: 'a{sx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esy', method: [ { name: 'Methodesy', signature: 'a{sy}', returnSignature: 'a{sy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esas', method: [ { name: 'Methodesas', signature: 'a{sas}', returnSignature: 'a{sas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ese', method: [ { name: 'Methodese', signature: 'a{sa{ss}}', returnSignature: 'a{sa{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esr', method: [ { name: 'Methodesr', signature: 'a{s(s)}', returnSignature: 'a{s(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.esv', method: [ { name: 'Methodesv', signature: 'a{sv}', returnSignature: 'a{sv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etb', method: [ { name: 'Methodetb', signature: 'a{tb}', returnSignature: 'a{tb}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etd', method: [ { name: 'Methodetd', signature: 'a{td}', returnSignature: 'a{td}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etg', method: [ { name: 'Methodetg', signature: 'a{tg}', returnSignature: 'a{tg}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eti', method: [ { name: 'Methodeti', signature: 'a{ti}', returnSignature: 'a{ti}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etn', method: [ { name: 'Methodetn', signature: 'a{tn}', returnSignature: 'a{tn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eto', method: [ { name: 'Methodeto', signature: 'a{to}', returnSignature: 'a{to}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etq', method: [ { name: 'Methodetq', signature: 'a{tq}', returnSignature: 'a{tq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ets', method: [ { name: 'Methodets', signature: 'a{ts}', returnSignature: 'a{ts}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ett', method: [ { name: 'Methodett', signature: 'a{tt}', returnSignature: 'a{tt}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etu', method: [ { name: 'Methodetu', signature: 'a{tu}', returnSignature: 'a{tu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etx', method: [ { name: 'Methodetx', signature: 'a{tx}', returnSignature: 'a{tx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ety', method: [ { name: 'Methodety', signature: 'a{ty}', returnSignature: 'a{ty}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etas', method: [ { name: 'Methodetas', signature: 'a{tas}', returnSignature: 'a{tas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ete', method: [ { name: 'Methodete', signature: 'a{ta{ss}}', returnSignature: 'a{ta{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etr', method: [ { name: 'Methodetr', signature: 'a{t(s)}', returnSignature: 'a{t(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.etv', method: [ { name: 'Methodetv', signature: 'a{tv}', returnSignature: 'a{tv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eub', method: [ { name: 'Methodeub', signature: 'a{ub}', returnSignature: 'a{ub}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eud', method: [ { name: 'Methodeud', signature: 'a{ud}', returnSignature: 'a{ud}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eug', method: [ { name: 'Methodeug', signature: 'a{ug}', returnSignature: 'a{ug}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eui', method: [ { name: 'Methodeui', signature: 'a{ui}', returnSignature: 'a{ui}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eun', method: [ { name: 'Methodeun', signature: 'a{un}', returnSignature: 'a{un}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.euo', method: [ { name: 'Methodeuo', signature: 'a{uo}', returnSignature: 'a{uo}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.euq', method: [ { name: 'Methodeuq', signature: 'a{uq}', returnSignature: 'a{uq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eus', method: [ { name: 'Methodeus', signature: 'a{us}', returnSignature: 'a{us}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eut', method: [ { name: 'Methodeut', signature: 'a{ut}', returnSignature: 'a{ut}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.euu', method: [ { name: 'Methodeuu', signature: 'a{uu}', returnSignature: 'a{uu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eux', method: [ { name: 'Methodeux', signature: 'a{ux}', returnSignature: 'a{ux}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.euy', method: [ { name: 'Methodeuy', signature: 'a{uy}', returnSignature: 'a{uy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.euas', method: [ { name: 'Methodeuas', signature: 'a{uas}', returnSignature: 'a{uas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eue', method: [ { name: 'Methodeue', signature: 'a{ua{ss}}', returnSignature: 'a{ua{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eur', method: [ { name: 'Methodeur', signature: 'a{u(s)}', returnSignature: 'a{u(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.euv', method: [ { name: 'Methodeuv', signature: 'a{uv}', returnSignature: 'a{uv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exb', method: [ { name: 'Methodexb', signature: 'a{xb}', returnSignature: 'a{xb}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exd', method: [ { name: 'Methodexd', signature: 'a{xd}', returnSignature: 'a{xd}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exg', method: [ { name: 'Methodexg', signature: 'a{xg}', returnSignature: 'a{xg}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exi', method: [ { name: 'Methodexi', signature: 'a{xi}', returnSignature: 'a{xi}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exn', method: [ { name: 'Methodexn', signature: 'a{xn}', returnSignature: 'a{xn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exo', method: [ { name: 'Methodexo', signature: 'a{xo}', returnSignature: 'a{xo}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exq', method: [ { name: 'Methodexq', signature: 'a{xq}', returnSignature: 'a{xq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exs', method: [ { name: 'Methodexs', signature: 'a{xs}', returnSignature: 'a{xs}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ext', method: [ { name: 'Methodext', signature: 'a{xt}', returnSignature: 'a{xt}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exu', method: [ { name: 'Methodexu', signature: 'a{xu}', returnSignature: 'a{xu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exx', method: [ { name: 'Methodexx', signature: 'a{xx}', returnSignature: 'a{xx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exy', method: [ { name: 'Methodexy', signature: 'a{xy}', returnSignature: 'a{xy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exas', method: [ { name: 'Methodexas', signature: 'a{xas}', returnSignature: 'a{xas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exe', method: [ { name: 'Methodexe', signature: 'a{xa{ss}}', returnSignature: 'a{xa{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exr', method: [ { name: 'Methodexr', signature: 'a{x(s)}', returnSignature: 'a{x(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.exv', method: [ { name: 'Methodexv', signature: 'a{xv}', returnSignature: 'a{xv}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyb', method: [ { name: 'Methodeyb', signature: 'a{yb}', returnSignature: 'a{yb}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyd', method: [ { name: 'Methodeyd', signature: 'a{yd}', returnSignature: 'a{yd}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyg', method: [ { name: 'Methodeyg', signature: 'a{yg}', returnSignature: 'a{yg}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyi', method: [ { name: 'Methodeyi', signature: 'a{yi}', returnSignature: 'a{yi}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyn', method: [ { name: 'Methodeyn', signature: 'a{yn}', returnSignature: 'a{yn}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyo', method: [ { name: 'Methodeyo', signature: 'a{yo}', returnSignature: 'a{yo}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyq', method: [ { name: 'Methodeyq', signature: 'a{yq}', returnSignature: 'a{yq}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eys', method: [ { name: 'Methodeys', signature: 'a{ys}', returnSignature: 'a{ys}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyt', method: [ { name: 'Methodeyt', signature: 'a{yt}', returnSignature: 'a{yt}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyu', method: [ { name: 'Methodeyu', signature: 'a{yu}', returnSignature: 'a{yu}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyx', method: [ { name: 'Methodeyx', signature: 'a{yx}', returnSignature: 'a{yx}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyy', method: [ { name: 'Methodeyy', signature: 'a{yy}', returnSignature: 'a{yy}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyas', method: [ { name: 'Methodeyas', signature: 'a{yas}', returnSignature: 'a{yas}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eye', method: [ { name: 'Methodeye', signature: 'a{ya{ss}}', returnSignature: 'a{ya{ss}}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyr', method: [ { name: 'Methodeyr', signature: 'a{y(s)}', returnSignature: 'a{y(s)}' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.eyv', method: [ { name: 'Methodeyv', signature: 'a{yv}', returnSignature: 'a{yv}' } ] }, callbacks.add(registerBusObject))
                }))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))};
            var registerBusObject = function(err) {
                bus.registerBusObject('/testObject',
                                      {
                                          'interface.ebb': { Methodebb: function(context, arg) { context.reply(arg); } },
                                          'interface.ebd': { Methodebd: function(context, arg) { context.reply(arg); } },
                                          'interface.ebg': { Methodebg: function(context, arg) { context.reply(arg); } },
                                          'interface.ebi': { Methodebi: function(context, arg) { context.reply(arg); } },
                                          'interface.ebn': { Methodebn: function(context, arg) { context.reply(arg); } },
                                          'interface.ebo': { Methodebo: function(context, arg) { context.reply(arg); } },
                                          'interface.ebq': { Methodebq: function(context, arg) { context.reply(arg); } },
                                          'interface.ebs': { Methodebs: function(context, arg) { context.reply(arg); } },
                                          'interface.ebt': { Methodebt: function(context, arg) { context.reply(arg); } },
                                          'interface.ebu': { Methodebu: function(context, arg) { context.reply(arg); } },
                                          'interface.ebx': { Methodebx: function(context, arg) { context.reply(arg); } },
                                          'interface.eby': { Methodeby: function(context, arg) { context.reply(arg); } },
                                          'interface.ebas': { Methodebas: function(context, arg) { context.reply(arg); } },
                                          'interface.ebe': { Methodebe: function(context, arg) { context.reply(arg); } },
                                          'interface.ebr': { Methodebr: function(context, arg) { context.reply(arg); } },
                                          'interface.ebv': {
                                              Methodebv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.edb': { Methodedb: function(context, arg) { context.reply(arg); } },
                                          'interface.edd': { Methodedd: function(context, arg) { context.reply(arg); } },
                                          'interface.edg': { Methodedg: function(context, arg) { context.reply(arg); } },
                                          'interface.edi': { Methodedi: function(context, arg) { context.reply(arg); } },
                                          'interface.edn': { Methodedn: function(context, arg) { context.reply(arg); } },
                                          'interface.edo': { Methodedo: function(context, arg) { context.reply(arg); } },
                                          'interface.edq': { Methodedq: function(context, arg) { context.reply(arg); } },
                                          'interface.eds': { Methodeds: function(context, arg) { context.reply(arg); } },
                                          'interface.edt': { Methodedt: function(context, arg) { context.reply(arg); } },
                                          'interface.edu': { Methodedu: function(context, arg) { context.reply(arg); } },
                                          'interface.edx': { Methodedx: function(context, arg) { context.reply(arg); } },
                                          'interface.edy': { Methodedy: function(context, arg) { context.reply(arg); } },
                                          'interface.edas': { Methodedas: function(context, arg) { context.reply(arg); } },
                                          'interface.ede': { Methodede: function(context, arg) { context.reply(arg); } },
                                          'interface.edr': { Methodedr: function(context, arg) { context.reply(arg); } },
                                          'interface.edv': {
                                              Methodedv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.egb': { Methodegb: function(context, arg) { context.reply(arg); } },
                                          'interface.egd': { Methodegd: function(context, arg) { context.reply(arg); } },
                                          'interface.egg': { Methodegg: function(context, arg) { context.reply(arg); } },
                                          'interface.egi': { Methodegi: function(context, arg) { context.reply(arg); } },
                                          'interface.egn': { Methodegn: function(context, arg) { context.reply(arg); } },
                                          'interface.ego': { Methodego: function(context, arg) { context.reply(arg); } },
                                          'interface.egq': { Methodegq: function(context, arg) { context.reply(arg); } },
                                          'interface.egs': { Methodegs: function(context, arg) { context.reply(arg); } },
                                          'interface.egt': { Methodegt: function(context, arg) { context.reply(arg); } },
                                          'interface.egu': { Methodegu: function(context, arg) { context.reply(arg); } },
                                          'interface.egx': { Methodegx: function(context, arg) { context.reply(arg); } },
                                          'interface.egy': { Methodegy: function(context, arg) { context.reply(arg); } },
                                          'interface.egas': { Methodegas: function(context, arg) { context.reply(arg); } },
                                          'interface.ege': { Methodege: function(context, arg) { context.reply(arg); } },
                                          'interface.egr': { Methodegr: function(context, arg) { context.reply(arg); } },
                                          'interface.egv': {
                                              Methodegv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.eib': { Methodeib: function(context, arg) { context.reply(arg); } },
                                          'interface.eid': { Methodeid: function(context, arg) { context.reply(arg); } },
                                          'interface.eig': { Methodeig: function(context, arg) { context.reply(arg); } },
                                          'interface.eii': { Methodeii: function(context, arg) { context.reply(arg); } },
                                          'interface.ein': { Methodein: function(context, arg) { context.reply(arg); } },
                                          'interface.eio': { Methodeio: function(context, arg) { context.reply(arg); } },
                                          'interface.eiq': { Methodeiq: function(context, arg) { context.reply(arg); } },
                                          'interface.eis': { Methodeis: function(context, arg) { context.reply(arg); } },
                                          'interface.eit': { Methodeit: function(context, arg) { context.reply(arg); } },
                                          'interface.eiu': { Methodeiu: function(context, arg) { context.reply(arg); } },
                                          'interface.eix': { Methodeix: function(context, arg) { context.reply(arg); } },
                                          'interface.eiy': { Methodeiy: function(context, arg) { context.reply(arg); } },
                                          'interface.eias': { Methodeias: function(context, arg) { context.reply(arg); } },
                                          'interface.eie': { Methodeie: function(context, arg) { context.reply(arg); } },
                                          'interface.eir': { Methodeir: function(context, arg) { context.reply(arg); } },
                                          'interface.eiv': {
                                              Methodeiv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.enb': { Methodenb: function(context, arg) { context.reply(arg); } },
                                          'interface.end': { Methodend: function(context, arg) { context.reply(arg); } },
                                          'interface.eng': { Methodeng: function(context, arg) { context.reply(arg); } },
                                          'interface.eni': { Methodeni: function(context, arg) { context.reply(arg); } },
                                          'interface.enn': { Methodenn: function(context, arg) { context.reply(arg); } },
                                          'interface.eno': { Methodeno: function(context, arg) { context.reply(arg); } },
                                          'interface.enq': { Methodenq: function(context, arg) { context.reply(arg); } },
                                          'interface.ens': { Methodens: function(context, arg) { context.reply(arg); } },
                                          'interface.ent': { Methodent: function(context, arg) { context.reply(arg); } },
                                          'interface.enu': { Methodenu: function(context, arg) { context.reply(arg); } },
                                          'interface.enx': { Methodenx: function(context, arg) { context.reply(arg); } },
                                          'interface.eny': { Methodeny: function(context, arg) { context.reply(arg); } },
                                          'interface.enas': { Methodenas: function(context, arg) { context.reply(arg); } },
                                          'interface.ene': { Methodene: function(context, arg) { context.reply(arg); } },
                                          'interface.enr': { Methodenr: function(context, arg) { context.reply(arg); } },
                                          'interface.env': {
                                              Methodenv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.eob': { Methodeob: function(context, arg) { context.reply(arg); } },
                                          'interface.eod': { Methodeod: function(context, arg) { context.reply(arg); } },
                                          'interface.eog': { Methodeog: function(context, arg) { context.reply(arg); } },
                                          'interface.eoi': { Methodeoi: function(context, arg) { context.reply(arg); } },
                                          'interface.eon': { Methodeon: function(context, arg) { context.reply(arg); } },
                                          'interface.eoo': { Methodeoo: function(context, arg) { context.reply(arg); } },
                                          'interface.eoq': { Methodeoq: function(context, arg) { context.reply(arg); } },
                                          'interface.eos': { Methodeos: function(context, arg) { context.reply(arg); } },
                                          'interface.eot': { Methodeot: function(context, arg) { context.reply(arg); } },
                                          'interface.eou': { Methodeou: function(context, arg) { context.reply(arg); } },
                                          'interface.eox': { Methodeox: function(context, arg) { context.reply(arg); } },
                                          'interface.eoy': { Methodeoy: function(context, arg) { context.reply(arg); } },
                                          'interface.eoas': { Methodeoas: function(context, arg) { context.reply(arg); } },
                                          'interface.eoe': { Methodeoe: function(context, arg) { context.reply(arg); } },
                                          'interface.eor': { Methodeor: function(context, arg) { context.reply(arg); } },
                                          'interface.eov': {
                                              Methodeov: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.eqb': { Methodeqb: function(context, arg) { context.reply(arg); } },
                                          'interface.eqd': { Methodeqd: function(context, arg) { context.reply(arg); } },
                                          'interface.eqg': { Methodeqg: function(context, arg) { context.reply(arg); } },
                                          'interface.eqi': { Methodeqi: function(context, arg) { context.reply(arg); } },
                                          'interface.eqn': { Methodeqn: function(context, arg) { context.reply(arg); } },
                                          'interface.eqo': { Methodeqo: function(context, arg) { context.reply(arg); } },
                                          'interface.eqq': { Methodeqq: function(context, arg) { context.reply(arg); } },
                                          'interface.eqs': { Methodeqs: function(context, arg) { context.reply(arg); } },
                                          'interface.eqt': { Methodeqt: function(context, arg) { context.reply(arg); } },
                                          'interface.equ': { Methodequ: function(context, arg) { context.reply(arg); } },
                                          'interface.eqx': { Methodeqx: function(context, arg) { context.reply(arg); } },
                                          'interface.eqy': { Methodeqy: function(context, arg) { context.reply(arg); } },
                                          'interface.eqas': { Methodeqas: function(context, arg) { context.reply(arg); } },
                                          'interface.eqe': { Methodeqe: function(context, arg) { context.reply(arg); } },
                                          'interface.eqr': { Methodeqr: function(context, arg) { context.reply(arg); } },
                                          'interface.eqv': {
                                              Methodeqv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.esb': { Methodesb: function(context, arg) { context.reply(arg); } },
                                          'interface.esd': { Methodesd: function(context, arg) { context.reply(arg); } },
                                          'interface.esg': { Methodesg: function(context, arg) { context.reply(arg); } },
                                          'interface.esi': { Methodesi: function(context, arg) { context.reply(arg); } },
                                          'interface.esn': { Methodesn: function(context, arg) { context.reply(arg); } },
                                          'interface.eso': { Methodeso: function(context, arg) { context.reply(arg); } },
                                          'interface.esq': { Methodesq: function(context, arg) { context.reply(arg); } },
                                          'interface.ess': { Methodess: function(context, arg) { context.reply(arg); } },
                                          'interface.est': { Methodest: function(context, arg) { context.reply(arg); } },
                                          'interface.esu': { Methodesu: function(context, arg) { context.reply(arg); } },
                                          'interface.esx': { Methodesx: function(context, arg) { context.reply(arg); } },
                                          'interface.esy': { Methodesy: function(context, arg) { context.reply(arg); } },
                                          'interface.esas': { Methodesas: function(context, arg) { context.reply(arg); } },
                                          'interface.ese': { Methodese: function(context, arg) { context.reply(arg); } },
                                          'interface.esr': { Methodesr: function(context, arg) { context.reply(arg); } },
                                          'interface.esv': {
                                              Methodesv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.etb': { Methodetb: function(context, arg) { context.reply(arg); } },
                                          'interface.etd': { Methodetd: function(context, arg) { context.reply(arg); } },
                                          'interface.etg': { Methodetg: function(context, arg) { context.reply(arg); } },
                                          'interface.eti': { Methodeti: function(context, arg) { context.reply(arg); } },
                                          'interface.etn': { Methodetn: function(context, arg) { context.reply(arg); } },
                                          'interface.eto': { Methodeto: function(context, arg) { context.reply(arg); } },
                                          'interface.etq': { Methodetq: function(context, arg) { context.reply(arg); } },
                                          'interface.ets': { Methodets: function(context, arg) { context.reply(arg); } },
                                          'interface.ett': { Methodett: function(context, arg) { context.reply(arg); } },
                                          'interface.etu': { Methodetu: function(context, arg) { context.reply(arg); } },
                                          'interface.etx': { Methodetx: function(context, arg) { context.reply(arg); } },
                                          'interface.ety': { Methodety: function(context, arg) { context.reply(arg); } },
                                          'interface.etas': { Methodetas: function(context, arg) { context.reply(arg); } },
                                          'interface.ete': { Methodete: function(context, arg) { context.reply(arg); } },
                                          'interface.etr': { Methodetr: function(context, arg) { context.reply(arg); } },
                                          'interface.etv': {
                                              Methodetv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.eub': { Methodeub: function(context, arg) { context.reply(arg); } },
                                          'interface.eud': { Methodeud: function(context, arg) { context.reply(arg); } },
                                          'interface.eug': { Methodeug: function(context, arg) { context.reply(arg); } },
                                          'interface.eui': { Methodeui: function(context, arg) { context.reply(arg); } },
                                          'interface.eun': { Methodeun: function(context, arg) { context.reply(arg); } },
                                          'interface.euo': { Methodeuo: function(context, arg) { context.reply(arg); } },
                                          'interface.euq': { Methodeuq: function(context, arg) { context.reply(arg); } },
                                          'interface.eus': { Methodeus: function(context, arg) { context.reply(arg); } },
                                          'interface.eut': { Methodeut: function(context, arg) { context.reply(arg); } },
                                          'interface.euu': { Methodeuu: function(context, arg) { context.reply(arg); } },
                                          'interface.eux': { Methodeux: function(context, arg) { context.reply(arg); } },
                                          'interface.euy': { Methodeuy: function(context, arg) { context.reply(arg); } },
                                          'interface.euas': { Methodeuas: function(context, arg) { context.reply(arg); } },
                                          'interface.eue': { Methodeue: function(context, arg) { context.reply(arg); } },
                                          'interface.eur': { Methodeur: function(context, arg) { context.reply(arg); } },
                                          'interface.euv': {
                                              Methodeuv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.exb': { Methodexb: function(context, arg) { context.reply(arg); } },
                                          'interface.exd': { Methodexd: function(context, arg) { context.reply(arg); } },
                                          'interface.exg': { Methodexg: function(context, arg) { context.reply(arg); } },
                                          'interface.exi': { Methodexi: function(context, arg) { context.reply(arg); } },
                                          'interface.exn': { Methodexn: function(context, arg) { context.reply(arg); } },
                                          'interface.exo': { Methodexo: function(context, arg) { context.reply(arg); } },
                                          'interface.exq': { Methodexq: function(context, arg) { context.reply(arg); } },
                                          'interface.exs': { Methodexs: function(context, arg) { context.reply(arg); } },
                                          'interface.ext': { Methodext: function(context, arg) { context.reply(arg); } },
                                          'interface.exu': { Methodexu: function(context, arg) { context.reply(arg); } },
                                          'interface.exx': { Methodexx: function(context, arg) { context.reply(arg); } },
                                          'interface.exy': { Methodexy: function(context, arg) { context.reply(arg); } },
                                          'interface.exas': { Methodexas: function(context, arg) { context.reply(arg); } },
                                          'interface.exe': { Methodexe: function(context, arg) { context.reply(arg); } },
                                          'interface.exr': { Methodexr: function(context, arg) { context.reply(arg); } },
                                          'interface.exv': {
                                              Methodexv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          },
                                          'interface.eyb': { Methodeyb: function(context, arg) { context.reply(arg); } },
                                          'interface.eyd': { Methodeyd: function(context, arg) { context.reply(arg); } },
                                          'interface.eyg': { Methodeyg: function(context, arg) { context.reply(arg); } },
                                          'interface.eyi': { Methodeyi: function(context, arg) { context.reply(arg); } },
                                          'interface.eyn': { Methodeyn: function(context, arg) { context.reply(arg); } },
                                          'interface.eyo': { Methodeyo: function(context, arg) { context.reply(arg); } },
                                          'interface.eyq': { Methodeyq: function(context, arg) { context.reply(arg); } },
                                          'interface.eys': { Methodeys: function(context, arg) { context.reply(arg); } },
                                          'interface.eyt': { Methodeyt: function(context, arg) { context.reply(arg); } },
                                          'interface.eyu': { Methodeyu: function(context, arg) { context.reply(arg); } },
                                          'interface.eyx': { Methodeyx: function(context, arg) { context.reply(arg); } },
                                          'interface.eyy': { Methodeyy: function(context, arg) { context.reply(arg); } },
                                          'interface.eyas': { Methodeyas: function(context, arg) { context.reply(arg); } },
                                          'interface.eye': { Methodeye: function(context, arg) { context.reply(arg); } },
                                          'interface.eyr': { Methodeyr: function(context, arg) { context.reply(arg); } },
                                          'interface.eyv': {
                                              Methodeyv: function(context, arg) {
                                                  var args = {};
                                                  for (var name in arg) {
                                                      args[name] = { 's': arg[name] };
                                                  }
                                                  context.reply(args);
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + '/testObject', callbacks.add(methodebb));
            };
            var proxy;
            var methodebb = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                proxy.methodCall('interface.ebb', 'Methodebb', { "true": true }, callbacks.add(onReplyebb));
            };
            var onReplyebb = function(err, context, argebb) {
                err && console.log('err.name = ' + err.name + '   err.code = ' + err.code);
                assertFalsy(err);
                assertEquals({ "true": true }, argebb);
                proxy.methodCall('interface.ebd', 'Methodebd', { "true": 1.234 }, callbacks.add(onReplyebd));
            };
            var onReplyebd = function(err, context, argebd) {
                assertFalsy(err);
                assertEquals({ "true": 1.234 }, argebd);
                proxy.methodCall('interface.ebg', 'Methodebg', { "true": "sig" }, callbacks.add(onReplyebg));
            };
            var onReplyebg = function(err, context, argebg) {
                assertFalsy(err);
                assertEquals({ "true": "sig" }, argebg);
                proxy.methodCall('interface.ebi', 'Methodebi', { "true": -1 }, callbacks.add(onReplyebi));
            };
            var onReplyebi = function(err, context, argebi) {
                assertFalsy(err);
                assertEquals({ "true": -1 }, argebi);
                proxy.methodCall('interface.ebn', 'Methodebn', { "true": -2 }, callbacks.add(onReplyebn));
            };
            var onReplyebn = function(err, context, argebn) {
                assertFalsy(err);
                assertEquals({ "true": -2 }, argebn);
                proxy.methodCall('interface.ebo', 'Methodebo', { "true": "/path" }, callbacks.add(onReplyebo));
            };
            var onReplyebo = function(err, context, argebo) {
                assertFalsy(err);
                assertEquals({ "true": "/path" }, argebo);
                proxy.methodCall('interface.ebq', 'Methodebq', { "true": 3 }, callbacks.add(onReplyebq));
            };
            var onReplyebq = function(err, context, argebq) {
                assertFalsy(err);
                assertEquals({ "true": 3 }, argebq);
                proxy.methodCall('interface.ebs', 'Methodebs', { "true": "string" }, callbacks.add(onReplyebs));
            };
            var onReplyebs = function(err, context, argebs) {
                assertFalsy(err);
                assertEquals({ "true": "string" }, argebs);
                proxy.methodCall('interface.ebt', 'Methodebt', { "true": 4 }, callbacks.add(onReplyebt));
            };
            var onReplyebt = function(err, context, argebt) {
                assertFalsy(err);
                argebt["true"] = parseInt(argebt["true"]);
                assertEquals({ "true": 4 }, argebt);
                proxy.methodCall('interface.ebu', 'Methodebu', { "true": 5 }, callbacks.add(onReplyebu));
            };
            var onReplyebu = function(err, context, argebu) {
                assertFalsy(err);
                assertEquals({ "true": 5 }, argebu);
                proxy.methodCall('interface.ebx', 'Methodebx', { "true": -6 }, callbacks.add(onReplyebx));
            };
            var onReplyebx = function(err, context, argebx) {
                assertFalsy(err);
                argebx["true"] = parseInt(argebx["true"]);
                assertEquals({ "true": -6 }, argebx);
                proxy.methodCall('interface.eby', 'Methodeby', { "true": 7 }, callbacks.add(onReplyeby));
            };
            var onReplyeby = function(err, context, argeby) {
                assertFalsy(err);
                assertEquals({ "true": 7 }, argeby);
                proxy.methodCall('interface.ebas', 'Methodebas', { "true": ["s0", "s1"] }, callbacks.add(onReplyebas));
            };
            var onReplyebas = function(err, context, argebas) {
                assertFalsy(err);
                assertEquals({ "true": ["s0", "s1"] }, argebas);
                proxy.methodCall('interface.ebe', 'Methodebe', { "true": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyebe));
            };
            var onReplyebe = function(err, context, argebe) {
                assertFalsy(err);
                assertEquals({ "true": { key0: "value0", key1: "value1" } }, argebe);
                proxy.methodCall('interface.ebr', 'Methodebr', { "true": ["string"] }, callbacks.add(onReplyebr));
            };
            var onReplyebr = function(err, context, argebr) {
                assertFalsy(err);
                assertEquals({ "true": ["string"] }, argebr);
                proxy.methodCall('interface.ebv', 'Methodebv', { "true": { s: "string" } }, callbacks.add(onReplyebv));
            };
            var onReplyebv = function(err, context, argebv) {
                assertFalsy(err);
                assertEquals({ "true": "string" }, argebv);
                proxy.methodCall('interface.edb', 'Methodedb', { "1.234": true }, callbacks.add(onReplyedb));
            };
            var onReplyedb = function(err, context, argedb) {
                assertFalsy(err);
                assertEquals({ "1.234": true }, argedb);
                proxy.methodCall('interface.edd', 'Methodedd', { "1.234": 1.234 }, callbacks.add(onReplyedd));
            };
            var onReplyedd = function(err, context, argedd) {
                assertFalsy(err);
                assertEquals({ "1.234": 1.234 }, argedd);
                proxy.methodCall('interface.edg', 'Methodedg', { "1.234": "sig" }, callbacks.add(onReplyedg));
            };
            var onReplyedg = function(err, context, argedg) {
                assertFalsy(err);
                assertEquals({ "1.234": "sig" }, argedg);
                proxy.methodCall('interface.edi', 'Methodedi', { "1.234": -1 }, callbacks.add(onReplyedi));
            };
            var onReplyedi = function(err, context, argedi) {
                assertFalsy(err);
                assertEquals({ "1.234": -1 }, argedi);
                proxy.methodCall('interface.edn', 'Methodedn', { "1.234": -2 }, callbacks.add(onReplyedn));
            };
            var onReplyedn = function(err, context, argedn) {
                assertFalsy(err);
                assertEquals({ "1.234": -2 }, argedn);
                proxy.methodCall('interface.edo', 'Methodedo', { "1.234": "/path" }, callbacks.add(onReplyedo));
            };
            var onReplyedo = function(err, context, argedo) {
                assertFalsy(err);
                assertEquals({ "1.234": "/path" }, argedo);
                proxy.methodCall('interface.edq', 'Methodedq', { "1.234": 3 }, callbacks.add(onReplyedq));
            };
            var onReplyedq = function(err, context, argedq) {
                assertFalsy(err);
                assertEquals({ "1.234": 3 }, argedq);
                proxy.methodCall('interface.eds', 'Methodeds', { "1.234": "string" }, callbacks.add(onReplyeds));
            };
            var onReplyeds = function(err, context, argeds) {
                assertFalsy(err);
                assertEquals({ "1.234": "string" }, argeds);
                proxy.methodCall('interface.edt', 'Methodedt', { "1.234": 4 }, callbacks.add(onReplyedt));
            };
            var onReplyedt = function(err, context, argedt) {
                assertFalsy(err);
                argedt["1.234"] = parseInt(argedt["1.234"]);
                assertEquals({ "1.234": 4 }, argedt);
                proxy.methodCall('interface.edu', 'Methodedu', { "1.234": 5 }, callbacks.add(onReplyedu));
            };
            var onReplyedu = function(err, context, argedu) {
                assertFalsy(err);
                assertEquals({ "1.234": 5 }, argedu);
                proxy.methodCall('interface.edx', 'Methodedx', { "1.234": -6 }, callbacks.add(onReplyedx));
            };
            var onReplyedx = function(err, context, argedx) {
                assertFalsy(err);
                argedx["1.234"] = parseInt(argedx["1.234"]);
                assertEquals({ "1.234": -6 }, argedx);
                proxy.methodCall('interface.edy', 'Methodedy', { "1.234": 7 }, callbacks.add(onReplyedy));
            };
            var onReplyedy = function(err, context, argedy) {
                assertFalsy(err);
                assertEquals({ "1.234": 7 }, argedy);
                proxy.methodCall('interface.edas', 'Methodedas', { "1.234": ["s0", "s1"] }, callbacks.add(onReplyedas));
            };
            var onReplyedas = function(err, context, argedas) {
                assertFalsy(err);
                assertEquals({ "1.234": ["s0", "s1"] }, argedas);
                proxy.methodCall('interface.ede', 'Methodede', { "1.234": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyede));
            };
            var onReplyede = function(err, context, argede) {
                assertFalsy(err);
                assertEquals({ "1.234": { key0: "value0", key1: "value1" } }, argede);
                proxy.methodCall('interface.edr', 'Methodedr', { "1.234": ["string"] }, callbacks.add(onReplyedr));
            };
            var onReplyedr = function(err, context, argedr) {
                assertFalsy(err);
                assertEquals({ "1.234": ["string"] }, argedr);
                proxy.methodCall('interface.edv', 'Methodedv', { "1.234": { s: "string" } }, callbacks.add(onReplyedv));
            };
            var onReplyedv = function(err, context, argedv) {
                assertFalsy(err);
                assertEquals({ "1.234": "string" }, argedv);
                proxy.methodCall('interface.egb', 'Methodegb', { "sig": true }, callbacks.add(onReplyegb));
            };
            var onReplyegb = function(err, context, argegb) {
                assertFalsy(err);
                assertEquals({ "sig": true }, argegb);
                proxy.methodCall('interface.egd', 'Methodegd', { "sig": 1.234 }, callbacks.add(onReplyegd));
            };
            var onReplyegd = function(err, context, argegd) {
                assertFalsy(err);
                assertEquals({ "sig": 1.234 }, argegd);
                proxy.methodCall('interface.egg', 'Methodegg', { "sig": "sig" }, callbacks.add(onReplyegg));
            };
            var onReplyegg = function(err, context, argegg) {
                assertFalsy(err);
                assertEquals({ "sig": "sig" }, argegg);
                proxy.methodCall('interface.egi', 'Methodegi', { "sig": -1 }, callbacks.add(onReplyegi));
            };
            var onReplyegi = function(err, context, argegi) {
                assertFalsy(err);
                assertEquals({ "sig": -1 }, argegi);
                proxy.methodCall('interface.egn', 'Methodegn', { "sig": -2 }, callbacks.add(onReplyegn));
            };
            var onReplyegn = function(err, context, argegn) {
                assertFalsy(err);
                assertEquals({ "sig": -2 }, argegn);
                proxy.methodCall('interface.ego', 'Methodego', { "sig": "/path" }, callbacks.add(onReplyego));
            };
            var onReplyego = function(err, context, argego) {
                assertFalsy(err);
                assertEquals({ "sig": "/path" }, argego);
                proxy.methodCall('interface.egq', 'Methodegq', { "sig": 3 }, callbacks.add(onReplyegq));
            };
            var onReplyegq = function(err, context, argegq) {
                assertFalsy(err);
                assertEquals({ "sig": 3 }, argegq);
                proxy.methodCall('interface.egs', 'Methodegs', { "sig": "string" }, callbacks.add(onReplyegs));
            };
            var onReplyegs = function(err, context, argegs) {
                assertFalsy(err);
                assertEquals({ "sig": "string" }, argegs);
                proxy.methodCall('interface.egt', 'Methodegt', { "sig": 4 }, callbacks.add(onReplyegt));
            };
            var onReplyegt = function(err, context, argegt) {
                assertFalsy(err);
                argegt["sig"] = parseInt(argegt["sig"]);
                assertEquals({ "sig": 4 }, argegt);
                proxy.methodCall('interface.egu', 'Methodegu', { "sig": 5 }, callbacks.add(onReplyegu));
            };
            var onReplyegu = function(err, context, argegu) {
                assertFalsy(err);
                assertEquals({ "sig": 5 }, argegu);
                proxy.methodCall('interface.egx', 'Methodegx', { "sig": -6 }, callbacks.add(onReplyegx));
            };
            var onReplyegx = function(err, context, argegx) {
                assertFalsy(err);
                argegx["sig"] = parseInt(argegx["sig"]);
                assertEquals({ "sig": -6 }, argegx);
                proxy.methodCall('interface.egy', 'Methodegy', { "sig": 7 }, callbacks.add(onReplyegy));
            };
            var onReplyegy = function(err, context, argegy) {
                assertFalsy(err);
                assertEquals({ "sig": 7 }, argegy);
                proxy.methodCall('interface.egas', 'Methodegas', { "sig": ["s0", "s1"] }, callbacks.add(onReplyegas));
            };
            var onReplyegas = function(err, context, argegas) {
                assertFalsy(err);
                assertEquals({ "sig": ["s0", "s1"] }, argegas);
                proxy.methodCall('interface.ege', 'Methodege', { "sig": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyege));
            };
            var onReplyege = function(err, context, argege) {
                assertFalsy(err);
                assertEquals({ "sig": { key0: "value0", key1: "value1" } }, argege);
                proxy.methodCall('interface.egr', 'Methodegr', { "sig": ["string"] }, callbacks.add(onReplyegr));
            };
            var onReplyegr = function(err, context, argegr) {
                assertFalsy(err);
                assertEquals({ "sig": ["string"] }, argegr);
                proxy.methodCall('interface.egv', 'Methodegv', { "sig": { s: "string" } }, callbacks.add(onReplyegv));
            };
            var onReplyegv = function(err, context, argegv) {
                assertFalsy(err);
                assertEquals({ "sig": "string" }, argegv);
                proxy.methodCall('interface.eib', 'Methodeib', { "-1": true }, callbacks.add(onReplyeib));
            };
            var onReplyeib = function(err, context, argeib) {
                assertFalsy(err);
                assertEquals({ "-1": true }, argeib);
                proxy.methodCall('interface.eid', 'Methodeid', { "-1": 1.234 }, callbacks.add(onReplyeid));
            };
            var onReplyeid = function(err, context, argeid) {
                assertFalsy(err);
                assertEquals({ "-1": 1.234 }, argeid);
                proxy.methodCall('interface.eig', 'Methodeig', { "-1": "sig" }, callbacks.add(onReplyeig));
            };
            var onReplyeig = function(err, context, argeig) {
                assertFalsy(err);
                assertEquals({ "-1": "sig" }, argeig);
                proxy.methodCall('interface.eii', 'Methodeii', { "-1": -1 }, callbacks.add(onReplyeii));
            };
            var onReplyeii = function(err, context, argeii) {
                assertFalsy(err);
                assertEquals({ "-1": -1 }, argeii);
                proxy.methodCall('interface.ein', 'Methodein', { "-1": -2 }, callbacks.add(onReplyein));
            };
            var onReplyein = function(err, context, argein) {
                assertFalsy(err);
                assertEquals({ "-1": -2 }, argein);
                proxy.methodCall('interface.eio', 'Methodeio', { "-1": "/path" }, callbacks.add(onReplyeio));
            };
            var onReplyeio = function(err, context, argeio) {
                assertFalsy(err);
                assertEquals({ "-1": "/path" }, argeio);
                proxy.methodCall('interface.eiq', 'Methodeiq', { "-1": 3 }, callbacks.add(onReplyeiq));
            };
            var onReplyeiq = function(err, context, argeiq) {
                assertFalsy(err);
                assertEquals({ "-1": 3 }, argeiq);
                proxy.methodCall('interface.eis', 'Methodeis', { "-1": "string" }, callbacks.add(onReplyeis));
            };
            var onReplyeis = function(err, context, argeis) {
                assertFalsy(err);
                assertEquals({ "-1": "string" }, argeis);
                proxy.methodCall('interface.eit', 'Methodeit', { "-1": 4 }, callbacks.add(onReplyeit));
            };
            var onReplyeit = function(err, context, argeit) {
                assertFalsy(err);
                argeit["-1"] = parseInt(argeit["-1"]);
                assertEquals({ "-1": 4 }, argeit);
                proxy.methodCall('interface.eiu', 'Methodeiu', { "-1": 5 }, callbacks.add(onReplyeiu));
            };
            var onReplyeiu = function(err, context, argeiu) {
                assertFalsy(err);
                assertEquals({ "-1": 5 }, argeiu);
                proxy.methodCall('interface.eix', 'Methodeix', { "-1": -6 }, callbacks.add(onReplyeix));
            };
            var onReplyeix = function(err, context, argeix) {
                assertFalsy(err);
                argeix["-1"] = parseInt(argeix["-1"]);
                assertEquals({ "-1": -6 }, argeix);
                proxy.methodCall('interface.eiy', 'Methodeiy', { "-1": 7 }, callbacks.add(onReplyeiy));
            };
            var onReplyeiy = function(err, context, argeiy) {
                assertFalsy(err);
                assertEquals({ "-1": 7 }, argeiy);
                proxy.methodCall('interface.eias', 'Methodeias', { "-1": ["s0", "s1"] }, callbacks.add(onReplyeias));
            };
            var onReplyeias = function(err, context, argeias) {
                assertFalsy(err);
                assertEquals({ "-1": ["s0", "s1"] }, argeias);
                proxy.methodCall('interface.eie', 'Methodeie', { "-1": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyeie));
            };
            var onReplyeie = function(err, context, argeie) {
                assertFalsy(err);
                assertEquals({ "-1": { key0: "value0", key1: "value1" } }, argeie);
                proxy.methodCall('interface.eir', 'Methodeir', { "-1": ["string"] }, callbacks.add(onReplyeir));
            };
            var onReplyeir = function(err, context, argeir) {
                assertFalsy(err);
                assertEquals({ "-1": ["string"] }, argeir);
                proxy.methodCall('interface.eiv', 'Methodeiv', { "-1": { s: "string" } }, callbacks.add(onReplyeiv));
            };
            var onReplyeiv = function(err, context, argeiv) {
                assertFalsy(err);
                assertEquals({ "-1": "string" }, argeiv);
                proxy.methodCall('interface.enb', 'Methodenb', { "-2": true }, callbacks.add(onReplyenb));
            };
            var onReplyenb = function(err, context, argenb) {
                assertFalsy(err);
                assertEquals({ "-2": true }, argenb);
                proxy.methodCall('interface.end', 'Methodend', { "-2": 1.234 }, callbacks.add(onReplyend));
            };
            var onReplyend = function(err, context, argend) {
                assertFalsy(err);
                assertEquals({ "-2": 1.234 }, argend);
                proxy.methodCall('interface.eng', 'Methodeng', { "-2": "sig" }, callbacks.add(onReplyeng));
            };
            var onReplyeng = function(err, context, argeng) {
                assertFalsy(err);
                assertEquals({ "-2": "sig" }, argeng);
                proxy.methodCall('interface.eni', 'Methodeni', { "-2": -1 }, callbacks.add(onReplyeni));
            };
            var onReplyeni = function(err, context, argeni) {
                assertFalsy(err);
                assertEquals({ "-2": -1 }, argeni);
                proxy.methodCall('interface.enn', 'Methodenn', { "-2": -2 }, callbacks.add(onReplyenn));
            };
            var onReplyenn = function(err, context, argenn) {
                assertFalsy(err);
                assertEquals({ "-2": -2 }, argenn);
                proxy.methodCall('interface.eno', 'Methodeno', { "-2": "/path" }, callbacks.add(onReplyeno));
            };
            var onReplyeno = function(err, context, argeno) {
                assertFalsy(err);
                assertEquals({ "-2": "/path" }, argeno);
                proxy.methodCall('interface.enq', 'Methodenq', { "-2": 3 }, callbacks.add(onReplyenq));
            };
            var onReplyenq = function(err, context, argenq) {
                assertFalsy(err);
                assertEquals({ "-2": 3 }, argenq);
                proxy.methodCall('interface.ens', 'Methodens', { "-2": "string" }, callbacks.add(onReplyens));
            };
            var onReplyens = function(err, context, argens) {
                assertFalsy(err);
                assertEquals({ "-2": "string" }, argens);
                proxy.methodCall('interface.ent', 'Methodent', { "-2": 4 }, callbacks.add(onReplyent));
            };
            var onReplyent = function(err, context, argent) {
                assertFalsy(err);
                argent["-2"] = parseInt(argent["-2"]);
                assertEquals({ "-2": 4 }, argent);
                proxy.methodCall('interface.enu', 'Methodenu', { "-2": 5 }, callbacks.add(onReplyenu));
            };
            var onReplyenu = function(err, context, argenu) {
                assertFalsy(err);
                assertEquals({ "-2": 5 }, argenu);
                proxy.methodCall('interface.enx', 'Methodenx', { "-2": -6 }, callbacks.add(onReplyenx));
            };
            var onReplyenx = function(err, context, argenx) {
                assertFalsy(err);
                argenx["-2"] = parseInt(argenx["-2"]);
                assertEquals({ "-2": -6 }, argenx);
                proxy.methodCall('interface.eny', 'Methodeny', { "-2": 7 }, callbacks.add(onReplyeny));
            };
            var onReplyeny = function(err, context, argeny) {
                assertFalsy(err);
                assertEquals({ "-2": 7 }, argeny);
                proxy.methodCall('interface.enas', 'Methodenas', { "-2": ["s0", "s1"] }, callbacks.add(onReplyenas));
            };
            var onReplyenas = function(err, context, argenas) {
                assertFalsy(err);
                assertEquals({ "-2": ["s0", "s1"] }, argenas);
                proxy.methodCall('interface.ene', 'Methodene', { "-2": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyene));
            };
            var onReplyene = function(err, context, argene) {
                assertFalsy(err);
                assertEquals({ "-2": { key0: "value0", key1: "value1" } }, argene);
                proxy.methodCall('interface.enr', 'Methodenr', { "-2": ["string"] }, callbacks.add(onReplyenr));
            };
            var onReplyenr = function(err, context, argenr) {
                assertFalsy(err);
                assertEquals({ "-2": ["string"] }, argenr);
                proxy.methodCall('interface.env', 'Methodenv', { "-2": { s: "string" } }, callbacks.add(onReplyenv));
            };
            var onReplyenv = function(err, context, argenv) {
                assertFalsy(err);
                assertEquals({ "-2": "string" }, argenv);
                proxy.methodCall('interface.eob', 'Methodeob', { "/path": true }, callbacks.add(onReplyeob));
            };
            var onReplyeob = function(err, context, argeob) {
                assertFalsy(err);
                assertEquals({ "/path": true }, argeob);
                proxy.methodCall('interface.eod', 'Methodeod', { "/path": 1.234 }, callbacks.add(onReplyeod));
            };
            var onReplyeod = function(err, context, argeod) {
                assertFalsy(err);
                assertEquals({ "/path": 1.234 }, argeod);
                proxy.methodCall('interface.eog', 'Methodeog', { "/path": "sig" }, callbacks.add(onReplyeog));
            };
            var onReplyeog = function(err, context, argeog) {
                assertFalsy(err);
                assertEquals({ "/path": "sig" }, argeog);
                proxy.methodCall('interface.eoi', 'Methodeoi', { "/path": -1 }, callbacks.add(onReplyeoi));
            };
            var onReplyeoi = function(err, context, argeoi) {
                assertFalsy(err);
                assertEquals({ "/path": -1 }, argeoi);
                proxy.methodCall('interface.eon', 'Methodeon', { "/path": -2 }, callbacks.add(onReplyeon));
            };
            var onReplyeon = function(err, context, argeon) {
                assertFalsy(err);
                assertEquals({ "/path": -2 }, argeon);
                proxy.methodCall('interface.eoo', 'Methodeoo', { "/path": "/path" }, callbacks.add(onReplyeoo));
            };
            var onReplyeoo = function(err, context, argeoo) {
                assertFalsy(err);
                assertEquals({ "/path": "/path" }, argeoo);
                proxy.methodCall('interface.eoq', 'Methodeoq', { "/path": 3 }, callbacks.add(onReplyeoq));
            };
            var onReplyeoq = function(err, context, argeoq) {
                assertFalsy(err);
                assertEquals({ "/path": 3 }, argeoq);
                proxy.methodCall('interface.eos', 'Methodeos', { "/path": "string" }, callbacks.add(onReplyeos));
            };
            var onReplyeos = function(err, context, argeos) {
                assertFalsy(err);
                assertEquals({ "/path": "string" }, argeos);
                proxy.methodCall('interface.eot', 'Methodeot', { "/path": 4 }, callbacks.add(onReplyeot));
            };
            var onReplyeot = function(err, context, argeot) {
                assertFalsy(err);
                argeot["/path"] = parseInt(argeot["/path"]);
                assertEquals({ "/path": 4 }, argeot);
                proxy.methodCall('interface.eou', 'Methodeou', { "/path": 5 }, callbacks.add(onReplyeou));
            };
            var onReplyeou = function(err, context, argeou) {
                assertFalsy(err);
                assertEquals({ "/path": 5 }, argeou);
                proxy.methodCall('interface.eox', 'Methodeox', { "/path": -6 }, callbacks.add(onReplyeox));
            };
            var onReplyeox = function(err, context, argeox) {
                assertFalsy(err);
                argeox["/path"] = parseInt(argeox["/path"]);
                assertEquals({ "/path": -6 }, argeox);
                proxy.methodCall('interface.eoy', 'Methodeoy', { "/path": 7 }, callbacks.add(onReplyeoy));
            };
            var onReplyeoy = function(err, context, argeoy) {
                assertFalsy(err);
                assertEquals({ "/path": 7 }, argeoy);
                proxy.methodCall('interface.eoas', 'Methodeoas', { "/path": ["s0", "s1"] }, callbacks.add(onReplyeoas));
            };
            var onReplyeoas = function(err, context, argeoas) {
                assertFalsy(err);
                assertEquals({ "/path": ["s0", "s1"] }, argeoas);
                proxy.methodCall('interface.eoe', 'Methodeoe', { "/path": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyeoe));
            };
            var onReplyeoe = function(err, context, argeoe) {
                assertFalsy(err);
                assertEquals({ "/path": { key0: "value0", key1: "value1" } }, argeoe);
                proxy.methodCall('interface.eor', 'Methodeor', { "/path": ["string"] }, callbacks.add(onReplyeor));
            };
            var onReplyeor = function(err, context, argeor) {
                assertFalsy(err);
                assertEquals({ "/path": ["string"] }, argeor);
                proxy.methodCall('interface.eov', 'Methodeov', { "/path": { s: "string" } }, callbacks.add(onReplyeov));
            };
            var onReplyeov = function(err, context, argeov) {
                assertFalsy(err);
                assertEquals({ "/path": "string" }, argeov);
                proxy.methodCall('interface.eqb', 'Methodeqb', { "3": true }, callbacks.add(onReplyeqb));
            };
            var onReplyeqb = function(err, context, argeqb) {
                assertFalsy(err);
                assertEquals({ "3": true }, argeqb);
                proxy.methodCall('interface.eqd', 'Methodeqd', { "3": 1.234 }, callbacks.add(onReplyeqd));
            };
            var onReplyeqd = function(err, context, argeqd) {
                assertFalsy(err);
                assertEquals({ "3": 1.234 }, argeqd);
                proxy.methodCall('interface.eqg', 'Methodeqg', { "3": "sig" }, callbacks.add(onReplyeqg));
            };
            var onReplyeqg = function(err, context, argeqg) {
                assertFalsy(err);
                assertEquals({ "3": "sig" }, argeqg);
                proxy.methodCall('interface.eqi', 'Methodeqi', { "3": -1 }, callbacks.add(onReplyeqi));
            };
            var onReplyeqi = function(err, context, argeqi) {
                assertFalsy(err);
                assertEquals({ "3": -1 }, argeqi);
                proxy.methodCall('interface.eqn', 'Methodeqn', { "3": -2 }, callbacks.add(onReplyeqn));
            };
            var onReplyeqn = function(err, context, argeqn) {
                assertFalsy(err);
                assertEquals({ "3": -2 }, argeqn);
                proxy.methodCall('interface.eqo', 'Methodeqo', { "3": "/path" }, callbacks.add(onReplyeqo));
            };
            var onReplyeqo = function(err, context, argeqo) {
                assertFalsy(err);
                assertEquals({ "3": "/path" }, argeqo);
                proxy.methodCall('interface.eqq', 'Methodeqq', { "3": 3 }, callbacks.add(onReplyeqq));
            };
            var onReplyeqq = function(err, context, argeqq) {
                assertFalsy(err);
                assertEquals({ "3": 3 }, argeqq);
                proxy.methodCall('interface.eqs', 'Methodeqs', { "3": "string" }, callbacks.add(onReplyeqs));
            };
            var onReplyeqs = function(err, context, argeqs) {
                assertFalsy(err);
                assertEquals({ "3": "string" }, argeqs);
                proxy.methodCall('interface.eqt', 'Methodeqt', { "3": 4 }, callbacks.add(onReplyeqt));
            };
            var onReplyeqt = function(err, context, argeqt) {
                assertFalsy(err);
                argeqt["3"] = parseInt(argeqt["3"]);
                assertEquals({ "3": 4 }, argeqt);
                proxy.methodCall('interface.equ', 'Methodequ', { "3": 5 }, callbacks.add(onReplyequ));
            };
            var onReplyequ = function(err, context, argequ) {
                assertFalsy(err);
                assertEquals({ "3": 5 }, argequ);
                proxy.methodCall('interface.eqx', 'Methodeqx', { "3": -6 }, callbacks.add(onReplyeqx));
            };
            var onReplyeqx = function(err, context, argeqx) {
                assertFalsy(err);
                argeqx["3"] = parseInt(argeqx["3"]);
                assertEquals({ "3": -6 }, argeqx);
                proxy.methodCall('interface.eqy', 'Methodeqy', { "3": 7 }, callbacks.add(onReplyeqy));
            };
            var onReplyeqy = function(err, context, argeqy) {
                assertFalsy(err);
                assertEquals({ "3": 7 }, argeqy);
                proxy.methodCall('interface.eqas', 'Methodeqas', { "3": ["s0", "s1"] }, callbacks.add(onReplyeqas));
            };
            var onReplyeqas = function(err, context, argeqas) {
                assertFalsy(err);
                assertEquals({ "3": ["s0", "s1"] }, argeqas);
                proxy.methodCall('interface.eqe', 'Methodeqe', { "3": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyeqe));
            };
            var onReplyeqe = function(err, context, argeqe) {
                assertFalsy(err);
                assertEquals({ "3": { key0: "value0", key1: "value1" } }, argeqe);
                proxy.methodCall('interface.eqr', 'Methodeqr', { "3": ["string"] }, callbacks.add(onReplyeqr));
            };
            var onReplyeqr = function(err, context, argeqr) {
                assertFalsy(err);
                assertEquals({ "3": ["string"] }, argeqr);
                proxy.methodCall('interface.eqv', 'Methodeqv', { "3": { s: "string" } }, callbacks.add(onReplyeqv));
            };
            var onReplyeqv = function(err, context, argeqv) {
                assertFalsy(err);
                assertEquals({ "3": "string" }, argeqv);
                proxy.methodCall('interface.esb', 'Methodesb', { "string": true }, callbacks.add(onReplyesb));
            };
            var onReplyesb = function(err, context, argesb) {
                assertFalsy(err);
                assertEquals({ "string": true }, argesb);
                proxy.methodCall('interface.esd', 'Methodesd', { "string": 1.234 }, callbacks.add(onReplyesd));
            };
            var onReplyesd = function(err, context, argesd) {
                assertFalsy(err);
                assertEquals({ "string": 1.234 }, argesd);
                proxy.methodCall('interface.esg', 'Methodesg', { "string": "sig" }, callbacks.add(onReplyesg));
            };
            var onReplyesg = function(err, context, argesg) {
                assertFalsy(err);
                assertEquals({ "string": "sig" }, argesg);
                proxy.methodCall('interface.esi', 'Methodesi', { "string": -1 }, callbacks.add(onReplyesi));
            };
            var onReplyesi = function(err, context, argesi) {
                assertFalsy(err);
                assertEquals({ "string": -1 }, argesi);
                proxy.methodCall('interface.esn', 'Methodesn', { "string": -2 }, callbacks.add(onReplyesn));
            };
            var onReplyesn = function(err, context, argesn) {
                assertFalsy(err);
                assertEquals({ "string": -2 }, argesn);
                proxy.methodCall('interface.eso', 'Methodeso', { "string": "/path" }, callbacks.add(onReplyeso));
            };
            var onReplyeso = function(err, context, argeso) {
                assertFalsy(err);
                assertEquals({ "string": "/path" }, argeso);
                proxy.methodCall('interface.esq', 'Methodesq', { "string": 3 }, callbacks.add(onReplyesq));
            };
            var onReplyesq = function(err, context, argesq) {
                assertFalsy(err);
                assertEquals({ "string": 3 }, argesq);
                proxy.methodCall('interface.ess', 'Methodess', { "string": "string" }, callbacks.add(onReplyess));
            };
            var onReplyess = function(err, context, argess) {
                assertFalsy(err);
                assertEquals({ "string": "string" }, argess);
                proxy.methodCall('interface.est', 'Methodest', { "string": 4 }, callbacks.add(onReplyest));
            };
            var onReplyest = function(err, context, argest) {
                assertFalsy(err);
                argest["string"] = parseInt(argest["string"]);
                assertEquals({ "string": 4 }, argest);
                proxy.methodCall('interface.esu', 'Methodesu', { "string": 5 }, callbacks.add(onReplyesu));
            };
            var onReplyesu = function(err, context, argesu) {
                assertFalsy(err);
                assertEquals({ "string": 5 }, argesu);
                proxy.methodCall('interface.esx', 'Methodesx', { "string": -6 }, callbacks.add(onReplyesx));
            };
            var onReplyesx = function(err, context, argesx) {
                assertFalsy(err);
                argesx["string"] = parseInt(argesx["string"]);
                assertEquals({ "string": -6 }, argesx);
                proxy.methodCall('interface.esy', 'Methodesy', { "string": 7 }, callbacks.add(onReplyesy));
            };
            var onReplyesy = function(err, context, argesy) {
                assertFalsy(err);
                assertEquals({ "string": 7 }, argesy);
                proxy.methodCall('interface.esas', 'Methodesas', { "string": ["s0", "s1"] }, callbacks.add(onReplyesas));
            };
            var onReplyesas = function(err, context, argesas) {
                assertFalsy(err);
                assertEquals({ "string": ["s0", "s1"] }, argesas);
                proxy.methodCall('interface.ese', 'Methodese', { "string": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyese));
            };
            var onReplyese = function(err, context, argese) {
                assertFalsy(err);
                assertEquals({ "string": { key0: "value0", key1: "value1" } }, argese);
                proxy.methodCall('interface.esr', 'Methodesr', { "string": ["string"] }, callbacks.add(onReplyesr));
            };
            var onReplyesr = function(err, context, argesr) {
                assertFalsy(err);
                assertEquals({ "string": ["string"] }, argesr);
                proxy.methodCall('interface.esv', 'Methodesv', { "string": { s: "string" } }, callbacks.add(onReplyesv));
            };
            var onReplyesv = function(err, context, argesv) {
                assertFalsy(err);
                assertEquals({ "string": "string" }, argesv);
                proxy.methodCall('interface.etb', 'Methodetb', { "4": true }, callbacks.add(onReplyetb));
            };
            var onReplyetb = function(err, context, argetb) {
                assertFalsy(err);
                assertEquals({ "4": true }, argetb);
                proxy.methodCall('interface.etd', 'Methodetd', { "4": 1.234 }, callbacks.add(onReplyetd));
            };
            var onReplyetd = function(err, context, argetd) {
                assertFalsy(err);
                assertEquals({ "4": 1.234 }, argetd);
                proxy.methodCall('interface.etg', 'Methodetg', { "4": "sig" }, callbacks.add(onReplyetg));
            };
            var onReplyetg = function(err, context, argetg) {
                assertFalsy(err);
                assertEquals({ "4": "sig" }, argetg);
                proxy.methodCall('interface.eti', 'Methodeti', { "4": -1 }, callbacks.add(onReplyeti));
            };
            var onReplyeti = function(err, context, argeti) {
                assertFalsy(err);
                assertEquals({ "4": -1 }, argeti);
                proxy.methodCall('interface.etn', 'Methodetn', { "4": -2 }, callbacks.add(onReplyetn));
            };
            var onReplyetn = function(err, context, argetn) {
                assertFalsy(err);
                assertEquals({ "4": -2 }, argetn);
                proxy.methodCall('interface.eto', 'Methodeto', { "4": "/path" }, callbacks.add(onReplyeto));
            };
            var onReplyeto = function(err, context, argeto) {
                assertFalsy(err);
                assertEquals({ "4": "/path" }, argeto);
                proxy.methodCall('interface.etq', 'Methodetq', { "4": 3 }, callbacks.add(onReplyetq));
            };
            var onReplyetq = function(err, context, argetq) {
                assertFalsy(err);
                assertEquals({ "4": 3 }, argetq);
                proxy.methodCall('interface.ets', 'Methodets', { "4": "string" }, callbacks.add(onReplyets));
            };
            var onReplyets = function(err, context, argets) {
                assertFalsy(err);
                assertEquals({ "4": "string" }, argets);
                proxy.methodCall('interface.ett', 'Methodett', { "4": 4 }, callbacks.add(onReplyett));
            };
            var onReplyett = function(err, context, argett) {
                assertFalsy(err);
                argett["4"] = parseInt(argett["4"]);
                assertEquals({ "4": 4 }, argett);
                proxy.methodCall('interface.etu', 'Methodetu', { "4": 5 }, callbacks.add(onReplyetu));
            };
            var onReplyetu = function(err, context, argetu) {
                assertFalsy(err);
                assertEquals({ "4": 5 }, argetu);
                proxy.methodCall('interface.etx', 'Methodetx', { "4": -6 }, callbacks.add(onReplyetx));
            };
            var onReplyetx = function(err, context, argetx) {
                assertFalsy(err);
                argetx["4"] = parseInt(argetx["4"]);
                assertEquals({ "4": -6 }, argetx);
                proxy.methodCall('interface.ety', 'Methodety', { "4": 7 }, callbacks.add(onReplyety));
            };
            var onReplyety = function(err, context, argety) {
                assertFalsy(err);
                assertEquals({ "4": 7 }, argety);
                proxy.methodCall('interface.etas', 'Methodetas', { "4": ["s0", "s1"] }, callbacks.add(onReplyetas));
            };
            var onReplyetas = function(err, context, argetas) {
                assertFalsy(err);
                assertEquals({ "4": ["s0", "s1"] }, argetas);
                proxy.methodCall('interface.ete', 'Methodete', { "4": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyete));
            };
            var onReplyete = function(err, context, argete) {
                assertFalsy(err);
                assertEquals({ "4": { key0: "value0", key1: "value1" } }, argete);
                proxy.methodCall('interface.etr', 'Methodetr', { "4": ["string"] }, callbacks.add(onReplyetr));
            };
            var onReplyetr = function(err, context, argetr) {
                assertFalsy(err);
                assertEquals({ "4": ["string"] }, argetr);
                proxy.methodCall('interface.etv', 'Methodetv', { "4": { s: "string" } }, callbacks.add(onReplyetv));
            };
            var onReplyetv = function(err, context, argetv) {
                assertFalsy(err);
                assertEquals({ "4": "string" }, argetv);
                proxy.methodCall('interface.eub', 'Methodeub', { "5": true }, callbacks.add(onReplyeub));
            };
            var onReplyeub = function(err, context, argeub) {
                assertFalsy(err);
                assertEquals({ "5": true }, argeub);
                proxy.methodCall('interface.eud', 'Methodeud', { "5": 1.234 }, callbacks.add(onReplyeud));
            };
            var onReplyeud = function(err, context, argeud) {
                assertFalsy(err);
                assertEquals({ "5": 1.234 }, argeud);
                proxy.methodCall('interface.eug', 'Methodeug', { "5": "sig" }, callbacks.add(onReplyeug));
            };
            var onReplyeug = function(err, context, argeug) {
                assertFalsy(err);
                assertEquals({ "5": "sig" }, argeug);
                proxy.methodCall('interface.eui', 'Methodeui', { "5": -1 }, callbacks.add(onReplyeui));
            };
            var onReplyeui = function(err, context, argeui) {
                assertFalsy(err);
                assertEquals({ "5": -1 }, argeui);
                proxy.methodCall('interface.eun', 'Methodeun', { "5": -2 }, callbacks.add(onReplyeun));
            };
            var onReplyeun = function(err, context, argeun) {
                assertFalsy(err);
                assertEquals({ "5": -2 }, argeun);
                proxy.methodCall('interface.euo', 'Methodeuo', { "5": "/path" }, callbacks.add(onReplyeuo));
            };
            var onReplyeuo = function(err, context, argeuo) {
                assertFalsy(err);
                assertEquals({ "5": "/path" }, argeuo);
                proxy.methodCall('interface.euq', 'Methodeuq', { "5": 3 }, callbacks.add(onReplyeuq));
            };
            var onReplyeuq = function(err, context, argeuq) {
                assertFalsy(err);
                assertEquals({ "5": 3 }, argeuq);
                proxy.methodCall('interface.eus', 'Methodeus', { "5": "string" }, callbacks.add(onReplyeus));
            };
            var onReplyeus = function(err, context, argeus) {
                assertFalsy(err);
                assertEquals({ "5": "string" }, argeus);
                proxy.methodCall('interface.eut', 'Methodeut', { "5": 4 }, callbacks.add(onReplyeut));
            };
            var onReplyeut = function(err, context, argeut) {
                assertFalsy(err);
                argeut["5"] = parseInt(argeut["5"]);
                assertEquals({ "5": 4 }, argeut);
                proxy.methodCall('interface.euu', 'Methodeuu', { "5": 5 }, callbacks.add(onReplyeuu));
            };
            var onReplyeuu = function(err, context, argeuu) {
                assertFalsy(err);
                assertEquals({ "5": 5 }, argeuu);
                proxy.methodCall('interface.eux', 'Methodeux', { "5": -6 }, callbacks.add(onReplyeux));
            };
            var onReplyeux = function(err, context, argeux) {
                assertFalsy(err);
                argeux["5"] = parseInt(argeux["5"]);
                assertEquals({ "5": -6 }, argeux);
                proxy.methodCall('interface.euy', 'Methodeuy', { "5": 7 }, callbacks.add(onReplyeuy));
            };
            var onReplyeuy = function(err, context, argeuy) {
                assertFalsy(err);
                assertEquals({ "5": 7 }, argeuy);
                proxy.methodCall('interface.euas', 'Methodeuas', { "5": ["s0", "s1"] }, callbacks.add(onReplyeuas));
            };
            var onReplyeuas = function(err, context, argeuas) {
                assertFalsy(err);
                assertEquals({ "5": ["s0", "s1"] }, argeuas);
                proxy.methodCall('interface.eue', 'Methodeue', { "5": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyeue));
            };
            var onReplyeue = function(err, context, argeue) {
                assertFalsy(err);
                assertEquals({ "5": { key0: "value0", key1: "value1" } }, argeue);
                proxy.methodCall('interface.eur', 'Methodeur', { "5": ["string"] }, callbacks.add(onReplyeur));
            };
            var onReplyeur = function(err, context, argeur) {
                assertFalsy(err);
                assertEquals({ "5": ["string"] }, argeur);
                proxy.methodCall('interface.euv', 'Methodeuv', { "5": { s: "string" } }, callbacks.add(onReplyeuv));
            };
            var onReplyeuv = function(err, context, argeuv) {
                assertFalsy(err);
                assertEquals({ "5": "string" }, argeuv);
                proxy.methodCall('interface.exb', 'Methodexb', { "-6": true }, callbacks.add(onReplyexb));
            };
            var onReplyexb = function(err, context, argexb) {
                assertFalsy(err);
                assertEquals({ "-6": true }, argexb);
                proxy.methodCall('interface.exd', 'Methodexd', { "-6": 1.234 }, callbacks.add(onReplyexd));
            };
            var onReplyexd = function(err, context, argexd) {
                assertFalsy(err);
                assertEquals({ "-6": 1.234 }, argexd);
                proxy.methodCall('interface.exg', 'Methodexg', { "-6": "sig" }, callbacks.add(onReplyexg));
            };
            var onReplyexg = function(err, context, argexg) {
                assertFalsy(err);
                assertEquals({ "-6": "sig" }, argexg);
                proxy.methodCall('interface.exi', 'Methodexi', { "-6": -1 }, callbacks.add(onReplyexi));
            };
            var onReplyexi = function(err, context, argexi) {
                assertFalsy(err);
                assertEquals({ "-6": -1 }, argexi);
                proxy.methodCall('interface.exn', 'Methodexn', { "-6": -2 }, callbacks.add(onReplyexn));
            };
            var onReplyexn = function(err, context, argexn) {
                assertFalsy(err);
                assertEquals({ "-6": -2 }, argexn);
                proxy.methodCall('interface.exo', 'Methodexo', { "-6": "/path" }, callbacks.add(onReplyexo));
            };
            var onReplyexo = function(err, context, argexo) {
                assertFalsy(err);
                assertEquals({ "-6": "/path" }, argexo);
                proxy.methodCall('interface.exq', 'Methodexq', { "-6": 3 }, callbacks.add(onReplyexq));
            };
            var onReplyexq = function(err, context, argexq) {
                assertFalsy(err);
                assertEquals({ "-6": 3 }, argexq);
                proxy.methodCall('interface.exs', 'Methodexs', { "-6": "string" }, callbacks.add(onReplyexs));
            };
            var onReplyexs = function(err, context, argexs) {
                assertFalsy(err);
                assertEquals({ "-6": "string" }, argexs);
                proxy.methodCall('interface.ext', 'Methodext', { "-6": 4 }, callbacks.add(onReplyext));
            };
            var onReplyext = function(err, context, argext) {
                assertFalsy(err);
                argext["-6"] = parseInt(argext["-6"]);
                assertEquals({ "-6": 4 }, argext);
                proxy.methodCall('interface.exu', 'Methodexu', { "-6": 5 }, callbacks.add(onReplyexu));
            };
            var onReplyexu = function(err, context, argexu) {
                assertFalsy(err);
                assertEquals({ "-6": 5 }, argexu);
                proxy.methodCall('interface.exx', 'Methodexx', { "-6": -6 }, callbacks.add(onReplyexx));
            };
            var onReplyexx = function(err, context, argexx) {
                assertFalsy(err);
                argexx["-6"] = parseInt(argexx["-6"]);
                assertEquals({ "-6": -6 }, argexx);
                proxy.methodCall('interface.exy', 'Methodexy', { "-6": 7 }, callbacks.add(onReplyexy));
            };
            var onReplyexy = function(err, context, argexy) {
                assertFalsy(err);
                assertEquals({ "-6": 7 }, argexy);
                proxy.methodCall('interface.exas', 'Methodexas', { "-6": ["s0", "s1"] }, callbacks.add(onReplyexas));
            };
            var onReplyexas = function(err, context, argexas) {
                assertFalsy(err);
                assertEquals({ "-6": ["s0", "s1"] }, argexas);
                proxy.methodCall('interface.exe', 'Methodexe', { "-6": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyexe));
            };
            var onReplyexe = function(err, context, argexe) {
                assertFalsy(err);
                assertEquals({ "-6": { key0: "value0", key1: "value1" } }, argexe);
                proxy.methodCall('interface.exr', 'Methodexr', { "-6": ["string"] }, callbacks.add(onReplyexr));
            };
            var onReplyexr = function(err, context, argexr) {
                assertFalsy(err);
                assertEquals({ "-6": ["string"] }, argexr);
                proxy.methodCall('interface.exv', 'Methodexv', { "-6": { s: "string" } }, callbacks.add(onReplyexv));
            };
            var onReplyexv = function(err, context, argexv) {
                assertFalsy(err);
                assertEquals({ "-6": "string" }, argexv);
                proxy.methodCall('interface.eyb', 'Methodeyb', { "7": true }, callbacks.add(onReplyeyb));
            };
            var onReplyeyb = function(err, context, argeyb) {
                assertFalsy(err);
                assertEquals({ "7": true }, argeyb);
                proxy.methodCall('interface.eyd', 'Methodeyd', { "7": 1.234 }, callbacks.add(onReplyeyd));
            };
            var onReplyeyd = function(err, context, argeyd) {
                assertFalsy(err);
                assertEquals({ "7": 1.234 }, argeyd);
                proxy.methodCall('interface.eyg', 'Methodeyg', { "7": "sig" }, callbacks.add(onReplyeyg));
            };
            var onReplyeyg = function(err, context, argeyg) {
                assertFalsy(err);
                assertEquals({ "7": "sig" }, argeyg);
                proxy.methodCall('interface.eyi', 'Methodeyi', { "7": -1 }, callbacks.add(onReplyeyi));
            };
            var onReplyeyi = function(err, context, argeyi) {
                assertFalsy(err);
                assertEquals({ "7": -1 }, argeyi);
                proxy.methodCall('interface.eyn', 'Methodeyn', { "7": -2 }, callbacks.add(onReplyeyn));
            };
            var onReplyeyn = function(err, context, argeyn) {
                assertFalsy(err);
                assertEquals({ "7": -2 }, argeyn);
                proxy.methodCall('interface.eyo', 'Methodeyo', { "7": "/path" }, callbacks.add(onReplyeyo));
            };
            var onReplyeyo = function(err, context, argeyo) {
                assertFalsy(err);
                assertEquals({ "7": "/path" }, argeyo);
                proxy.methodCall('interface.eyq', 'Methodeyq', { "7": 3 }, callbacks.add(onReplyeyq));
            };
            var onReplyeyq = function(err, context, argeyq) {
                assertFalsy(err);
                assertEquals({ "7": 3 }, argeyq);
                proxy.methodCall('interface.eys', 'Methodeys', { "7": "string" }, callbacks.add(onReplyeys));
            };
            var onReplyeys = function(err, context, argeys) {
                assertFalsy(err);
                assertEquals({ "7": "string" }, argeys);
                proxy.methodCall('interface.eyt', 'Methodeyt', { "7": 4 }, callbacks.add(onReplyeyt));
            };
            var onReplyeyt = function(err, context, argeyt) {
                assertFalsy(err);
                argeyt["7"] = parseInt(argeyt["7"]);
                assertEquals({ "7": 4 }, argeyt);
                proxy.methodCall('interface.eyu', 'Methodeyu', { "7": 5 }, callbacks.add(onReplyeyu));
            };
            var onReplyeyu = function(err, context, argeyu) {
                assertFalsy(err);
                assertEquals({ "7": 5 }, argeyu);
                proxy.methodCall('interface.eyx', 'Methodeyx', { "7": -6 }, callbacks.add(onReplyeyx));
            };
            var onReplyeyx = function(err, context, argeyx) {
                assertFalsy(err);
                argeyx["7"] = parseInt(argeyx["7"]);
                assertEquals({ "7": -6 }, argeyx);
                proxy.methodCall('interface.eyy', 'Methodeyy', { "7": 7 }, callbacks.add(onReplyeyy));
            };
            var onReplyeyy = function(err, context, argeyy) {
                assertFalsy(err);
                assertEquals({ "7": 7 }, argeyy);
                proxy.methodCall('interface.eyas', 'Methodeyas', { "7": ["s0", "s1"] }, callbacks.add(onReplyeyas));
            };
            var onReplyeyas = function(err, context, argeyas) {
                assertFalsy(err);
                assertEquals({ "7": ["s0", "s1"] }, argeyas);
                proxy.methodCall('interface.eye', 'Methodeye', { "7": { key0: "value0", key1: "value1" } }, callbacks.add(onReplyeye));
            };
            var onReplyeye = function(err, context, argeye) {
                assertFalsy(err);
                assertEquals({ "7": { key0: "value0", key1: "value1" } }, argeye);
                proxy.methodCall('interface.eyr', 'Methodeyr', { "7": ["string"] }, callbacks.add(onReplyeyr));
            };
            var onReplyeyr = function(err, context, argeyr) {
                assertFalsy(err);
                assertEquals({ "7": ["string"] }, argeyr);
                proxy.methodCall('interface.eyv', 'Methodeyv', { "7": { s: "string" } }, callbacks.add(onReplyeyv));
            };
            var onReplyeyv = function(err, context, argeyv) {
                assertFalsy(err);
                assertEquals({ "7": "string" }, argeyv);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testStruct: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                bus.createInterface({ name: 'interface.rb', method: [ { name: 'Methodrb', signature: '(b)', returnSignature: '(b)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rd', method: [ { name: 'Methodrd', signature: '(d)', returnSignature: '(d)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rg', method: [ { name: 'Methodrg', signature: '(g)', returnSignature: '(g)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ri', method: [ { name: 'Methodri', signature: '(i)', returnSignature: '(i)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rn', method: [ { name: 'Methodrn', signature: '(n)', returnSignature: '(n)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ro', method: [ { name: 'Methodro', signature: '(o)', returnSignature: '(o)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rq', method: [ { name: 'Methodrq', signature: '(q)', returnSignature: '(q)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rs', method: [ { name: 'Methodrs', signature: '(s)', returnSignature: '(s)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rt', method: [ { name: 'Methodrt', signature: '(t)', returnSignature: '(t)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ru', method: [ { name: 'Methodru', signature: '(u)', returnSignature: '(u)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rx', method: [ { name: 'Methodrx', signature: '(x)', returnSignature: '(x)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ry', method: [ { name: 'Methodry', signature: '(y)', returnSignature: '(y)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ras', method: [ { name: 'Methodras', signature: '(as)', returnSignature: '(as)' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.re', method: [ { name: 'Methodre', signature: '(a{ss})', returnSignature: '(a{ss})' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rr', method: [ { name: 'Methodrr', signature: '((s))', returnSignature: '((s))' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.rv', method: [ { name: 'Methodrv', signature: '(v)', returnSignature: '(v)' } ] }, callbacks.add(registerBusObject))
                }))}))}))}))}))}))}))}))}))}))}))}))}))}))}))};
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject('/testObject',
                                      {
                                          'interface.rb': { Methodrb: function(context, arg) { context.reply(arg); } },
                                          'interface.rd': { Methodrd: function(context, arg) { context.reply(arg); } },
                                          'interface.rg': { Methodrg: function(context, arg) { context.reply(arg); } },
                                          'interface.ri': { Methodri: function(context, arg) { context.reply(arg); } },
                                          'interface.rn': { Methodrn: function(context, arg) { context.reply(arg); } },
                                          'interface.ro': { Methodro: function(context, arg) { context.reply(arg); } },
                                          'interface.rq': { Methodrq: function(context, arg) { context.reply(arg); } },
                                          'interface.rs': { Methodrs: function(context, arg) { context.reply(arg); } },
                                          'interface.rt': { Methodrt: function(context, arg) { context.reply(arg); } },
                                          'interface.ru': { Methodru: function(context, arg) { context.reply(arg); } },
                                          'interface.rx': { Methodrx: function(context, arg) { context.reply(arg); } },
                                          'interface.ry': { Methodry: function(context, arg) { context.reply(arg); } },
                                          'interface.ras': { Methodras: function(context, arg) { context.reply(arg); } },
                                          'interface.re': { Methodre: function(context, arg) { context.reply(arg); } },
                                          'interface.rr': { Methodrr: function(context, arg) { context.reply(arg); } },
                                          'interface.rv': {
                                              Methodrv: function(context, arg) {
                                                  var args = [];
                                                  for (var i = 0; i < arg.length; ++i) {
                                                      args[i] = { 's': arg[i] };
                                                  }
                                                  context.reply(args);
                                              }
                                          }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + '/testObject', callbacks.add(methodrb));
            };
            var proxy;
            var methodrb = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                proxy.methodCall('interface.rb', 'Methodrb', [ true ], callbacks.add(onReplyrb));
            };
            var onReplyrb = function(err, context, argrb) {
                assertFalsy(err);
                assertEquals([ true ], argrb);
                proxy.methodCall('interface.rd', 'Methodrd', [ 1.234 ], callbacks.add(onReplyrd));
            };
            var onReplyrd = function(err, context, argrd) {
                assertFalsy(err);
                assertEquals([ 1.234 ], argrd);
                proxy.methodCall('interface.rg', 'Methodrg', [ "sig" ], callbacks.add(onReplyrg));
            };
            var onReplyrg = function(err, context, argrg) {
                assertFalsy(err);
                assertEquals([ "sig" ], argrg);
                proxy.methodCall('interface.ri', 'Methodri', [ -1 ], callbacks.add(onReplyri));
            };
            var onReplyri = function(err, context, argri) {
                assertFalsy(err);
                assertEquals([ -1 ], argri);
                proxy.methodCall('interface.rn', 'Methodrn', [ -2 ], callbacks.add(onReplyrn));
            };
            var onReplyrn = function(err, context, argrn) {
                assertFalsy(err);
                assertEquals([ -2 ], argrn);
                proxy.methodCall('interface.ro', 'Methodro', [ "/path" ], callbacks.add(onReplyro));
            };
            var onReplyro = function(err, context, argro) {
                assertFalsy(err);
                assertEquals([ "/path" ], argro);
                proxy.methodCall('interface.rq', 'Methodrq', [ 3 ], callbacks.add(onReplyrq));
            };
            var onReplyrq = function(err, context, argrq) {
                assertFalsy(err);
                assertEquals([ 3 ], argrq);
                proxy.methodCall('interface.rs', 'Methodrs', [ "string" ], callbacks.add(onReplyrs));
            };
            var onReplyrs = function(err, context, argrs) {
                assertFalsy(err);
                assertEquals([ "string" ], argrs);
                proxy.methodCall('interface.rt', 'Methodrt', [ 4 ], callbacks.add(onReplyrt));
            };
            var onReplyrt = function(err, context, argrt) {
                assertFalsy(err);
                argrt[0] = parseInt(argrt[0]);
                assertEquals([ 4 ], argrt);
                proxy.methodCall('interface.ru', 'Methodru', [ 5 ], callbacks.add(onReplyru));
            };
            var onReplyru = function(err, context, argru) {
                assertFalsy(err);
                assertEquals([ 5 ], argru);
                proxy.methodCall('interface.rx', 'Methodrx', [ -6 ], callbacks.add(onReplyrx));
            };
            var onReplyrx = function(err, context, argrx) {
                assertFalsy(err);
                argrx[0] = parseInt(argrx[0]);
                assertEquals([ -6 ], argrx);
                proxy.methodCall('interface.ry', 'Methodry', [ 7 ], callbacks.add(onReplyry));
            };
            var onReplyry = function(err, context, argry) {
                assertFalsy(err);
                assertEquals([ 7 ], argry);
                proxy.methodCall('interface.ras', 'Methodras', [ ["s0", "s1"] ], callbacks.add(onReplyras));
            };
            var onReplyras = function(err, context, argras) {
                assertFalsy(err);
                assertEquals([ ["s0", "s1"] ], argras);
                proxy.methodCall('interface.re', 'Methodre', [ { key0: "value0", key1: "value1" } ], callbacks.add(onReplyre));
            };
            var onReplyre = function(err, context, argre) {
                assertFalsy(err);
                assertEquals([ { key0: "value0", key1: "value1" } ], argre);
                proxy.methodCall('interface.rr', 'Methodrr', [ ["string"] ], callbacks.add(onReplyrr));
            };
            var onReplyrr = function(err, context, argrr) {
                assertFalsy(err);
                assertEquals([ ["string"] ], argrr);
                proxy.methodCall('interface.rv', 'Methodrv', [ { s: "string" } ], callbacks.add(onReplyrv));
            };
            var onReplyrv = function(err, context, argrv) {
                assertFalsy(err);
                assertEquals([ "string" ], argrv);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testVariant: function(queue) {
        queue.call(function(callbacks) {
            var connect = function(err) {
                assertFalsy(err);
                bus.connect(callbacks.add(createInterface));
            };
            var createInterface = function(err) {
                bus.createInterface({ name: 'interface.vb', method: [ { name: 'Methodvb', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vd', method: [ { name: 'Methodvd', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vg', method: [ { name: 'Methodvg', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vi', method: [ { name: 'Methodvi', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vn', method: [ { name: 'Methodvn', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vo', method: [ { name: 'Methodvo', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vq', method: [ { name: 'Methodvq', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vs', method: [ { name: 'Methodvs', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vt', method: [ { name: 'Methodvt', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vu', method: [ { name: 'Methodvu', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vx', method: [ { name: 'Methodvx', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vy', method: [ { name: 'Methodvy', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vas', method: [ { name: 'Methodvas', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.ve', method: [ { name: 'Methodve', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vr', method: [ { name: 'Methodvr', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(function(err) {
                bus.createInterface({ name: 'interface.vv', method: [ { name: 'Methodvv', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(registerBusObject))
                }))}))}))}))}))}))}))}))}))}))}))}))}))}))}))};
            var registerBusObject = function(err) {
                assertFalsy(err);
                bus.registerBusObject('/testObject',
                                      {
                                          'interface.vb': { Methodvb: function(context, arg) { context.reply({ 'b': arg }); } },
                                          'interface.vd': { Methodvd: function(context, arg) { context.reply({ 'd': arg }); } },
                                          'interface.vg': { Methodvg: function(context, arg) { context.reply({ 'g': arg }); } },
                                          'interface.vi': { Methodvi: function(context, arg) { context.reply({ 'i': arg }); } },
                                          'interface.vn': { Methodvn: function(context, arg) { context.reply({ 'n': arg }); } },
                                          'interface.vo': { Methodvo: function(context, arg) { context.reply({ 'o': arg }); } },
                                          'interface.vq': { Methodvq: function(context, arg) { context.reply({ 'q': arg }); } },
                                          'interface.vs': { Methodvs: function(context, arg) { context.reply({ 's': arg }); } },
                                          'interface.vt': { Methodvt: function(context, arg) { context.reply({ 't': arg }); } },
                                          'interface.vu': { Methodvu: function(context, arg) { context.reply({ 'u': arg }); } },
                                          'interface.vx': { Methodvx: function(context, arg) { context.reply({ 'x': arg }); } },
                                          'interface.vy': { Methodvy: function(context, arg) { context.reply({ 'y': arg }); } },
                                          'interface.vas': { Methodvas: function(context, arg) { context.reply({ 'as': arg }); } },
                                          'interface.ve': { Methodve: function(context, arg) { context.reply({ 'a{ss}': arg }); } },
                                          'interface.vr': { Methodvr: function(context, arg) { context.reply({ '(s)': arg }); } },
                                          'interface.vv': { Methodvv: function(context, arg) { context.reply({ 'v': arg }); } }
                                      },
                                      false,
                                      callbacks.add(getProxyObj));
            };
            var getProxyObj = function(err) {
                assertFalsy(err);
                bus.getProxyBusObject(bus.uniqueName + '/testObject', callbacks.add(methodvb));
            };
            var proxy;
            var methodvb = function(err, proxyObj) {
                assertFalsy(err);
                proxy = proxyObj;
                proxy.methodCall('interface.vb', 'Methodvb', { 'b': true }, callbacks.add(onReplyvb));
            };
            var onReplyvb = function(err, context, argvb) {
                assertFalsy(err);
                assertEquals(true, argvb);
                proxy.methodCall('interface.vd', 'Methodvd', { 'd': 1.234 }, callbacks.add(onReplyvd));
            };
            var onReplyvd = function(err, context, argvd) {
                assertFalsy(err);
                assertEquals(1.234, argvd);
                proxy.methodCall('interface.vg', 'Methodvg', { 'g': "sig" }, callbacks.add(onReplyvg));
            };
            var onReplyvg = function(err, context, argvg) {
                assertFalsy(err);
                assertEquals("sig", argvg);
                proxy.methodCall('interface.vi', 'Methodvi', { 'i': -1 }, callbacks.add(onReplyvi));
            };
            var onReplyvi = function(err, context, argvi) {
                assertFalsy(err);
                assertEquals(-1, argvi);
                proxy.methodCall('interface.vn', 'Methodvn', { 'n': -2 }, callbacks.add(onReplyvn));
            };
            var onReplyvn = function(err, context, argvn) {
                assertFalsy(err);
                assertEquals(-2, argvn);
                proxy.methodCall('interface.vo', 'Methodvo', { 'o': "/path" }, callbacks.add(onReplyvo));
            };
            var onReplyvo = function(err, context, argvo) {
                assertFalsy(err);
                assertEquals("/path", argvo);
                proxy.methodCall('interface.vq', 'Methodvq', { 'q': 3 }, callbacks.add(onReplyvq));
            };
            var onReplyvq = function(err, context, argvq) {
                assertFalsy(err);
                assertEquals(3, argvq);
                proxy.methodCall('interface.vs', 'Methodvs', { 's': "string" }, callbacks.add(onReplyvs));
            };
            var onReplyvs = function(err, context, argvs) {
                assertFalsy(err);
                assertEquals("string", argvs);
                proxy.methodCall('interface.vt', 'Methodvt', { 't': 4 }, callbacks.add(onReplyvt));
            };
            var onReplyvt = function(err, context, argvt) {
                assertFalsy(err);
                assertEquals(4, argvt);
                proxy.methodCall('interface.vu', 'Methodvu', { 'u': 5 }, callbacks.add(onReplyvu));
            };
            var onReplyvu = function(err, context, argvu) {
                assertFalsy(err);
                assertEquals(5, argvu);
                proxy.methodCall('interface.vx', 'Methodvx', { 'x': -6 }, callbacks.add(onReplyvx));
            };
            var onReplyvx = function(err, context, argvx) {
                assertFalsy(err);
                assertEquals(-6, argvx);
                proxy.methodCall('interface.vy', 'Methodvy', { 'y': 7 }, callbacks.add(onReplyvy));
            };
            var onReplyvy = function(err, context, argvy) {
                assertFalsy(err);
                assertEquals(7, argvy);
                proxy.methodCall('interface.vas', 'Methodvas', { 'as': ["s0", "s1"] }, callbacks.add(onReplyvas));
            };
            var onReplyvas = function(err, context, argvas) {
                assertFalsy(err);
                assertEquals(["s0", "s1"], argvas);
                proxy.methodCall('interface.ve', 'Methodve', { 'a{ss}': { key0: "value0", key1: "value1" } }, callbacks.add(onReplyve));
            };
            var onReplyve = function(err, context, argve) {
                assertFalsy(err);
                assertEquals({ key0: "value0", key1: "value1" }, argve);
                proxy.methodCall('interface.vr', 'Methodvr', { '(s)': ["string"] }, callbacks.add(onReplyvr));;
            };
            var onReplyvr = function(err, context, argvr) {
                assertFalsy(err);
                assertEquals(["string"], argvr);
                proxy.methodCall('interface.vv', 'Methodvv', { 'v': { 's': "string" } }, callbacks.add(onReplyvv));
            };
            var onReplyvv = function(err, context, argvv) {
                assertFalsy(err);
                assertEquals({ 's': "string" }, argvv);
            };
            this._setUp(callbacks.add(connect));
        });
    },

    testHandle: function(queue) {
        queue.call(function(callbacks) {
            var fds = {};
            var SESSION_PORT = 111;
            var startSession = function(err) {
                assertFalsy(err);
                var bindSessionPort = function(err) {
                    assertFalsy(err);
                    bus.bindSessionPort({
                        port: SESSION_PORT,
                        traffic: org.alljoyn.bus.SessionOpts.TRAFFIC_RAW_RELIABLE,
                        transport: org.alljoyn.bus.SessionOpts.TRANSPORT_LOCAL,
                        onAccept: function(port, joiner, opts) {
                            return true;
                        },
                        onJoined: function(port, id, joiner) {
                            var onFd = function(err, fd) {
                                assertFalsy(err);
                                fds.server = fd;
                                if (fds.server && fds.client) {
                                    test();
                                }
                            };
                            bus.getSessionFd(id, callbacks.add(onFd));
                        }
                    }, callbacks.add(joinSession));
                };
                bus.connect(callbacks.add(bindSessionPort));
            };
            var joinSession = function(err) {
                assertFalsy(err);
                var connect = function(err) {
                    assertFalsy(err);
                    otherBus.connect(callbacks.add(joinSession));
                };
                var joinSession = function(err) {
                    assertFalsy(err);
                    otherBus.joinSession({
                        host: bus.uniqueName,
                        port: SESSION_PORT,
                        traffic: org.alljoyn.bus.SessionOpts.TRAFFIC_RAW_RELIABLE
                    }, callbacks.add(onJoinSession));
                };
                var onJoinSession = function(err, id, opts) {
                    assertFalsy(err);
                    var onFd = function(err, fd) {
                        assertFalsy(err);
                        fds.client = fd;
                        if (fds.server && fds.client) {
                            test();
                        }
                    };
                    otherBus.getSessionFd(id, callbacks.add(onFd));
                };
                otherBus = new org.alljoyn.bus.BusAttachment();
                otherBus.create(false, callbacks.add(connect));
            };

            var test = function() {
                var createInterface = function(err) {
                    bus.createInterface({ name: 'interface.h', method: [ { name: 'Methodh', signature: 'h', returnSignature: 'h' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ah', method: [ { name: 'Methodah', signature: 'ah', returnSignature: 'ah' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehb', method: [ { name: 'Methodehb', signature: 'a{hb}', returnSignature: 'a{hb}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehd', method: [ { name: 'Methodehd', signature: 'a{hd}', returnSignature: 'a{hd}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehg', method: [ { name: 'Methodehg', signature: 'a{hg}', returnSignature: 'a{hg}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehi', method: [ { name: 'Methodehi', signature: 'a{hi}', returnSignature: 'a{hi}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehn', method: [ { name: 'Methodehn', signature: 'a{hn}', returnSignature: 'a{hn}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.eho', method: [ { name: 'Methodeho', signature: 'a{ho}', returnSignature: 'a{ho}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehq', method: [ { name: 'Methodehq', signature: 'a{hq}', returnSignature: 'a{hq}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehs', method: [ { name: 'Methodehs', signature: 'a{hs}', returnSignature: 'a{hs}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.eht', method: [ { name: 'Methodeht', signature: 'a{ht}', returnSignature: 'a{ht}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehu', method: [ { name: 'Methodehu', signature: 'a{hu}', returnSignature: 'a{hu}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehx', method: [ { name: 'Methodehx', signature: 'a{hx}', returnSignature: 'a{hx}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehy', method: [ { name: 'Methodehy', signature: 'a{hy}', returnSignature: 'a{hy}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehas', method: [ { name: 'Methodehas', signature: 'a{has}', returnSignature: 'a{has}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehe', method: [ { name: 'Methodehe', signature: 'a{ha{ss}}', returnSignature: 'a{ha{ss}}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehr', method: [ { name: 'Methodehr', signature: 'a{h(s)}', returnSignature: 'a{h(s)}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.ehv', method: [ { name: 'Methodehv', signature: 'a{hv}', returnSignature: 'a{hv}' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.rh', method: [ { name: 'Methodrh', signature: '(h)', returnSignature: '(h)' } ] }, callbacks.add(function(err) {
                    bus.createInterface({ name: 'interface.vh', method: [ { name: 'Methodvh', signature: 'v', returnSignature: 'v' } ] }, callbacks.add(registerBusObject))
                    }))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))}))};
                var registerBusObject = function(err) {
                    bus.registerBusObject('/testObject',
                                          {
                                              'interface.h': { Methodh: function(context, arg) { context.reply(arg); } },
                                              'interface.ah': { Methodah: function(context, arg) { context.reply(arg); } },
                                              'interface.ehb': { Methodehb: function(context, arg) { context.reply(arg); } },
                                              'interface.ehd': { Methodehd: function(context, arg) { context.reply(arg); } },
                                              'interface.ehg': { Methodehg: function(context, arg) { context.reply(arg); } },
                                              'interface.ehi': { Methodehi: function(context, arg) { context.reply(arg); } },
                                              'interface.ehn': { Methodehn: function(context, arg) { context.reply(arg); } },
                                              'interface.eho': { Methodeho: function(context, arg) { context.reply(arg); } },
                                              'interface.ehq': { Methodehq: function(context, arg) { context.reply(arg); } },
                                              'interface.ehs': { Methodehs: function(context, arg) { context.reply(arg); } },
                                              'interface.eht': { Methodeht: function(context, arg) { context.reply(arg); } },
                                              'interface.ehu': { Methodehu: function(context, arg) { context.reply(arg); } },
                                              'interface.ehx': { Methodehx: function(context, arg) { context.reply(arg); } },
                                              'interface.ehy': { Methodehy: function(context, arg) { context.reply(arg); } },
                                              'interface.ehas': { Methodehas: function(context, arg) { context.reply(arg); } },
                                              'interface.ehe': { Methodehe: function(context, arg) { context.reply(arg); } },
                                              'interface.ehr': { Methodehr: function(context, arg) { context.reply(arg); } },
                                              'interface.ehv': {
                                                  Methodehv: function(context, arg) {
                                                      var args = {};
                                                      for (var name in arg) {
                                                          args[name] = { 's': arg[name] };
                                                      }
                                                      context.reply(args);
                                                  }
                                              },
                                              'interface.rh': { Methodrh: function(context, arg) { context.reply(arg); } },
                                              'interface.vh': { Methodvh: function(context, arg) { context.reply({ 'h': arg }); } }
                                          },
                                          false,
                                          callbacks.add(getProxyObj));
                };
                var getProxyObj = function(err) {
                    assertFalsy(err);
                    bus.getProxyBusObject(bus.uniqueName + '/testObject', callbacks.add(methodh));
                };
                var proxy;
                var methodh = function(err, proxyObj) {
                    assertFalsy(err);
                    proxy = proxyObj;
                    proxy.methodCall('interface.h', 'Methodh', fds.server, callbacks.add(onReplyh));
                };
                var onReplyh = function(err, context, argh) {
                    assertFalsy(err);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    argh.recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    proxy.methodCall('interface.ah', 'Methodah', [ fds.server, fds.server ], callbacks.add(onReplyah))
                };
                var onReplyah = function(err, context, argah) {
                    assertFalsy(err);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    argah[1].recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehb = {};
                    ehb[fds.server.fd] = true;
                    proxy.methodCall('interface.ehb', 'Methodehb', ehb, callbacks.add(onReplyehb));
                };
                var onReplyehb = function(err, context, argehb) {
                    assertFalsy(err);
                    for (var fd in argehb) { break; }
                    assertEquals(true, argehb[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehd = {};
                    ehd[fds.server.fd] = 1.234;
                    proxy.methodCall('interface.ehd', 'Methodehd', ehd, callbacks.add(onReplyehd));
                };
                var onReplyehd = function(err, context, argehd) {
                    assertFalsy(err);
                    for (var fd in argehd) { break; }
                    assertEquals(1.234, argehd[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehg = {};
                    ehg[fds.server.fd] = "sig";
                    proxy.methodCall('interface.ehg', 'Methodehg', ehg, callbacks.add(onReplyehg));
                };
                var onReplyehg = function(err, context, argehg) {
                    assertFalsy(err);
                    for (var fd in argehg) { break; }
                    assertEquals("sig", argehg[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehi = {};
                    ehi[fds.server.fd] = -1;
                    proxy.methodCall('interface.ehi', 'Methodehi', ehi, callbacks.add(onReplyehi));
                };
                var onReplyehi = function(err, context, argehi) {
                    assertFalsy(err);
                    for (var fd in argehi) { break; }
                    assertEquals(-1, argehi[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehn = {};
                    ehn[fds.server.fd] = -2;
                    proxy.methodCall('interface.ehn', 'Methodehn', ehn, callbacks.add(onReplyehn));
                };
                var onReplyehn = function(err, context, argehn) {
                    assertFalsy(err);
                    for (var fd in argehn) { break; }
                    assertEquals(-2, argehn[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var eho = {};
                    eho[fds.server.fd] = "/path";
                    proxy.methodCall('interface.eho', 'Methodeho', eho, callbacks.add(onReplyeho));
                };
                var onReplyeho = function(err, context, argeho) {
                    assertFalsy(err);
                    for (var fd in argeho) { break; }
                    assertEquals("/path", argeho[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehq = {};
                    ehq[fds.server.fd] = 3;
                    proxy.methodCall('interface.ehq', 'Methodehq', ehq, callbacks.add(onReplyehq));
                };
                var onReplyehq = function(err, context, argehq) {
                    assertFalsy(err);
                    for (var fd in argehq) { break; }
                    assertEquals(3, argehq[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehs = {};
                    ehs[fds.server.fd] = "string";
                    proxy.methodCall('interface.ehs', 'Methodehs', ehs, callbacks.add(onReplyehs));
                };
                var onReplyehs = function(err, context, argehs) {
                    assertFalsy(err);
                    for (var fd in argehs) { break; }
                    assertEquals("string", argehs[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var eht = {};
                    eht[fds.server.fd] = 4;
                    proxy.methodCall('interface.eht', 'Methodeht', eht, callbacks.add(onReplyeht));
                };
                var onReplyeht = function(err, context, argeht) {
                    assertFalsy(err);
                    for (var fd in argeht) { break; }
                    assertEquals(4, argeht[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehu = {};
                    ehu[fds.server.fd] = 5;
                    proxy.methodCall('interface.ehu', 'Methodehu', ehu, callbacks.add(onReplyehu));
                };
                var onReplyehu = function(err, context, argehu) {
                    assertFalsy(err);
                    for (var fd in argehu) { break; }
                    assertEquals(5, argehu[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehx = {};
                    ehx[fds.server.fd] = -6;
                    proxy.methodCall('interface.ehx', 'Methodehx', ehx, callbacks.add(onReplyehx));
                };
                var onReplyehx = function(err, context, argehx) {
                    assertFalsy(err);
                    for (var fd in argehx) { break; }
                    assertEquals(-6, argehx[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehy = {};
                    ehy[fds.server.fd] = 7;
                    proxy.methodCall('interface.ehy', 'Methodehy', ehy, callbacks.add(onReplyehy));
                };
                var onReplyehy = function(err, context, argehy) {
                    assertFalsy(err);
                    for (var fd in argehy) { break; }
                    assertEquals(7, argehy[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehas = {};
                    ehas[fds.server.fd] = ["s0", "s1"];
                    proxy.methodCall('interface.ehas', 'Methodehas', ehas, callbacks.add(onReplyehas));
                };
                var onReplyehas = function(err, context, argehas) {
                    assertFalsy(err);
                    for (var fd in argehas) { break; }
                    assertEquals(["s0", "s1"], argehas[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehe = {};
                    ehe[fds.server.fd] = { key0: "value0", key1: "value1" };
                    proxy.methodCall('interface.ehe', 'Methodehe', ehe, callbacks.add(onReplyehe));
                };
                var onReplyehe = function(err, context, argehe) {
                    assertFalsy(err);
                    for (var fd in argehe) { break; }
                    assertEquals({ key0: "value0", key1: "value1" }, argehe[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehr = {};
                    ehr[fds.server.fd] = ["string"];
                    proxy.methodCall('interface.ehr', 'Methodehr', ehr, callbacks.add(onReplyehr));
                };
                var onReplyehr = function(err, context, argehr) {
                    assertFalsy(err);
                    for (var fd in argehr) { break; }
                    assertEquals(["string"], argehr[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    var ehv = {};
                    ehv[fds.server.fd] = { "s": "string" };
                    proxy.methodCall('interface.ehv', 'Methodehv', ehv, callbacks.add(onReplyehv));
                };
                var onReplyehv = function(err, context, argehv) {
                    assertFalsy(err);
                    for (var fd in argehv) { break; }
                    assertEquals("string", argehv[fd]);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    new org.alljoyn.bus.SocketFd(fd).recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    proxy.methodCall('interface.rh', 'Methodrh', [ fds.server ], callbacks.add(onReplyrh));
                };
                var onReplyrh = function(err, context, argrh) {
                    assertFalsy(err);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    argrh[0].recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);

                    proxy.methodCall('interface.vh', 'Methodvh', { 'h': fds.server }, callbacks.add(onReplyvh));
                };
                var onReplyvh = function(err, context, argvh) {
                    assertFalsy(err);
                    assertEquals(4, fds.client.send([1, 2, 3, 4]));
                    var buf = new Array(4);
                    argvh.recv(buf);
                    assertEquals(buf, [1, 2, 3, 4]);
                };
            };

            this._setUp(callbacks.add(startSession));
        });
    },
});
