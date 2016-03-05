/**
 * @file ConfigHelper is a facility to make it easier to construct internal
 * router configuration XML without having to create and use external XML files
 * or edit and rebuild routing nodes.  Useful for making small changes to the
 * default configuration.
 */

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
#ifndef _CONFIG_HELPER_H
#define _CONFIG_HELPER_H

#include <list>

#include <alljoyn/Init.h>
#include <alljoyn/Status.h>
#include <qcc/String.h>

namespace ajn {

class ConfigEntry {
  public:
    ConfigEntry(qcc::String kind, qcc::String name, qcc::String value) : m_kind(kind), m_name(name), m_value(value) { }

    qcc::String Generate()
    {
        /*
         * Listen is different from the other kinds.  Here name is the transport
         * name (e.g., "tcp"), we add the ":" for free and value is the rest of
         * the key/value pairs.  For example, to get
         *
         *     <listen>tcp:iface=*,port=9955</listen>
         *
         * into the config, use
         *
         *     Set("listen", "tcp", "iface=*,port=9955");
         */
        if (m_kind == "listen") {
            qcc::String str("<listen>" + m_name + ":" + m_value + "</listen>");
            return str;
        } else {
            qcc::String str("<" + m_kind + " name=\"" + m_name + "\">" + m_value + "</" + m_kind + ">");
            return str;
        }
    }

    qcc::String m_kind;
    qcc::String m_name;
    qcc::String m_value;
};

class ConfigHelper {
  public:
    /**
     * Construct a ConfigHelper.
     */
    ConfigHelper() : m_pretty(false) { }

    /**
     * Clear any existing entries and Load some reasonable defaults suitable for
     * the platform
     */
    void Clear()
    {
        m_entries.clear();
    }

    /**
     * Clear out any existing entries and Load some reasonable defaults suitable
     * for the platform
     */
    void PlatformDefaults()
    {
        Set("limit", "auth_timeout", "20000");
        Set("limit", "max_incomplete_connections", "48");
        Set("limit", "max_completed_connections", "64");
        Set("limit", "max_remote_clients_tcp", "48");
        Set("limit", "max_remote_clients_udp", "48");
        Set("property", "router_power_source", "Battery powered and chargeable");
        Set("property", "router_mobility", "Intermediate mobility");
        Set("property", "router_availability", "3-6 hr");
        Set("property", "router_node_connection", "Wireless");
        Set("flag", "restrict_untrusted_clients", "false");
        Set("listen", "tcp", "iface=*,port=9955");
        Set("listen", "udp", "iface=*,port=9955");
#ifdef QCC_OS_GROUP_POSIX
        Set("listen", "unix", "abstract=alljoyn");
#endif
#if defined(QCC_OS_DARWIN)
        Set("listen", "launchd", "env=DBUS_LAUNCHD_SESSION_BUS_SOCKET");
#endif
    }

    /**
     * Enable a prettified version of the configuration XML suitable for
     * human reading.
     */
    void Pretty() { m_pretty = true; }

    /**
     * Enable a compact version of the configuration XML suitable for
     * internal use.
     */
    void Normal() { m_pretty = false; }

    /**
     * Set or overwrite an entry in the configuration.
     */
    void Set(qcc::String kind, qcc::String name, qcc::String value)
    {
        /*
         * There can be multiple listen specs for the same transport,
         * Otherwise we overwrite same kind and name entries.
         */
        if (kind != "listen") {
            std::list<ConfigEntry>::iterator it = m_entries.begin();
            while (it != m_entries.end()) {
                if (it->m_kind == kind && it->m_name == name) {
                    it = m_entries.erase(it);
                } else {
                    ++it;
                }
            }
        }
        ConfigEntry entry(kind, name, value);
        m_entries.push_back(entry);
    }

    /**
     * Clear an entry in the configuration (remove one of the defaults).  Note
     * that removing a listen spec for a transport name will remove all listen
     * specs for that transport.
     */
    void Unset(qcc::String kind, qcc::String name)
    {
        std::list<ConfigEntry>::iterator it = m_entries.begin();
        while (it != m_entries.end()) {
            if (it->m_kind == kind && it->m_name == name) {
                it = m_entries.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * Generate the actual XML for the provided set of items.
     */
    qcc::String Generate()
    {
        qcc::String generated("<busconfig>");
        if (m_pretty) {
            generated.append("\n");
        }

        if (m_pretty) {
            generated.append("    ");
        }
        generated.append("<type>alljoyn</type>");
        if (m_pretty) {
            generated.append("\n");
        }

        for (std::list<ConfigEntry>::iterator it = m_entries.begin(); it != m_entries.end(); ++it) {
            if (m_pretty) {
                generated.append("    ");
            }

            generated.append(it->Generate());

            if (m_pretty) {
                generated.append("\n");
            }
        }

        generated.append("</busconfig>");
        if (m_pretty) {
            generated.append("\n");
        }

        return generated;
    }

    /**
     * Helper to parse command line arguments in a simple language used
     * to create custom configurations.
     *
     * --custom
     * --flag name value       (e.g. "--flag restrict_untrusted_clients true")
     * --limit name value      (e.g. "--limit max_completed_connections 32")
     * --property name value   (e.g. "--property router_node_connection Wireless")
     * --listen transport spec (e.g. "--listen tcp iface=*,port=9954")
     * --clear                 (Clear any existing entries)
     * --defaults              (Set the platform defaults in the configuration)
     * --end                   (End the configuration-by-arguments process and return)
     */
    int ParseArgs(int start, int argc, char** argv)
    {
        enum Mode {READY, ACCUMULATE};
        Mode mode = Mode::READY;

        enum State {NONE, NAME};
        State state = State::NONE;

        qcc::String kind;
        qcc::String name;

        int ret = start;
        qcc::String arg(argv[start]);

        /*
         * The first item of a custom configuration section must be "--custom"
         */
        if (arg.compare("--custom") != 0) {
            return ret;
        }

        for (int i = start + 1; i < argc; ++i) {
            ret = i;
            arg = argv[i];
            switch (mode) {
            case Mode::READY:
                if (arg.compare("--flag") == 0) {
                    kind = "flag";
                    mode = Mode::ACCUMULATE;
                    state = State::NONE;
                } else if (arg.compare("--limit") == 0) {
                    kind = "limit";
                    mode = Mode::ACCUMULATE;
                    state = State::NONE;
                } else if (arg.compare("--property") == 0) {
                    kind = "property";
                    mode = Mode::ACCUMULATE;
                    state = State::NONE;
                } else if (arg.compare("--listen") == 0) {
                    kind = "listen";
                    mode = Mode::ACCUMULATE;
                    state = State::NONE;
                } else if (arg.compare("--defaults") == 0) {
                    PlatformDefaults();
                    break;
                } else if (arg.compare("--clear") == 0) {
                    Clear();
                    break;
                } else if (arg.compare("--end") == 0) {
                    break;
                } else {
                    printf("found junk %s at index %d\n", arg.c_str(), ret);
                    return ret;
                }
                break;

            case Mode::ACCUMULATE:
                /*
                 * In one of the defined modes.  We have either accumulated
                 * nothing and we are pointing at the name (state NONE), or we
                 * have accumulated the name and are pointing at the value
                 * (state NAME).
                 */
                switch (state) {
                case State::NONE:
                    /*
                     * Accumulate name and change state accordingly.
                     */
                    name = arg;
                    state = State::NAME;
                    break;

                case State::NAME:
                    /*
                     * Have name.  Must be pointing at value.  We have
                     * everything we need for this config entry, so create it
                     * with one exception: if kind is "listen" and arg is "DEL"
                     * then delete instead of add to allow replacing default
                     * listen specs (remember multiple listen specs on the same
                     * transport is legal).
                     */
                    if (kind.compare("listen") == 0 && arg.compare("DEL") == 0) {
                        Unset(kind, name);
                    } else {
                        Set(kind, name, arg);
                    }
                    mode = Mode::READY;
                    state = State::NONE;
                    break;
                }

            default:
                /*
                 * Something wrong with the input.
                 */
                break;
            }
        }

        /*
         * Return the last item parsed.  We expect to be called from an argv
         * parse loop as in
         *
         *     i = configHelper.ParseArgs(i, argc, argv);
         *
         * So if everything went smoothly, argv[i] will be "--end" otherwise it will
         * point to some unexpected token.
         */
        return ret;
    }

  private:
    std::list<ConfigEntry> m_entries;
    bool m_pretty;
};

}

#endif
