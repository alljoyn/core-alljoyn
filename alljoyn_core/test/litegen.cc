/**
 * @file
 *
 */

/******************************************************************************
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
 ******************************************************************************/




#include <qcc/XmlElement.h>
#include <qcc/StringSource.h>
#include <qcc/String.h>
#include <list>

#include <string>
#include <fstream>
#include <streambuf>

using namespace qcc;



void ParseFile(const qcc::String& path, qcc::String& xml)
{
    printf("Opening '%s'\n", path.c_str());
    std::ifstream t(path.c_str());

    if (t.is_open()) {
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        xml = str.c_str();
    } else {
        printf("ERROR!\n");
    }
}


qcc::String ToUpper(const qcc::String& str)
{
    qcc::String s = str;

    for (unsigned i = 0; i < s.size(); ++i) {
        s[i] = toupper(s[i]);
    }

    return s;
}


int main(int argc, char** argv)
{
    String xml;
    ParseFile(argv[1], xml);
    StringSource source(xml);
    XmlParseContext parserContext(source);


    QStatus status = XmlElement::Parse(parserContext);

    if (status != ER_OK) {
        printf("Parser Error: %s\n", QCC_StatusText(status));
        return -1;
    }

    // root == 'node';
    const XmlElement* root = parserContext.GetRoot();
    typedef std::vector<const XmlElement*> ChildVector;
    typedef ChildVector::const_iterator ChildVectorIter;
    const ChildVector children = root->GetChildren("interface");
    const qcc::String objectPath = root->GetAttribute("name");

    struct Member {
        qcc::String name;
        int obj;
        int iface;
        int idx;
    };

    struct Property {
        qcc::String name;
        int obj;
        int iface;
        int idx;
    };

    std::list<Member> members;
    std::list<Property> props;
    std::list<qcc::String> interfaces;

    int i = 0;
    for (ChildVectorIter it = children.begin(); it != children.end(); ++it) {
        const XmlElement* iface = *it;
        const qcc::String name = iface->GetAttribute("name");

        if (name == "org.freedesktop.DBus.Properties" || name == "org.freedesktop.DBus.Introspectable") {
            continue;
        }

        ++i;
        char buf[128];
        snprintf(buf, sizeof(buf), "AJ_Interface_%d", i);
        interfaces.push_back(buf);

        printf("static const char* %s[] = {\n", buf);
        printf("\t\"%s\",\n", name.c_str());


        const ChildVector methods = iface->GetChildren("method");
        int member = 0;
        for (ChildVectorIter cit = methods.begin(); cit != methods.end(); ++cit, ++member) {
            const XmlElement* method = *cit;
            const qcc::String method_name = method->GetAttribute("name");
            printf("\t\"?%s ", method_name.c_str());

            const ChildVector args = method->GetChildren("arg");
            for (ChildVectorIter ait = args.begin(); ait != args.end(); ++ait) {
                const XmlElement* arg = *ait;
                const qcc::String arg_name = arg->GetAttribute("name");
                const qcc::String type = arg->GetAttribute("type");
                const qcc::String direction = arg->GetAttribute("direction");

                const char dir = (direction == "out" ? '>' : '<');
                printf("%s%c%s ", arg_name.c_str(), dir, type.c_str());
            }

            printf("\",\n");

            Member m = { ToUpper(method_name), 0, i, member };
            members.push_back(m);
        }

        const ChildVector signals = iface->GetChildren("signal");
        for (ChildVectorIter cit = signals.begin(); cit != signals.end(); ++cit, ++member) {
            const XmlElement* signal = *cit;
            const qcc::String signal_name = signal->GetAttribute("name");
            printf("\t\"!%s ", signal_name.c_str());

            const ChildVector args = signal->GetChildren("arg");
            for (ChildVectorIter ait = args.begin(); ait != args.end(); ++ait) {
                const XmlElement* arg = *ait;
                const qcc::String name = arg->GetAttribute("name");
                const qcc::String type = arg->GetAttribute("type");
                printf("%s>%s ", name.c_str(), type.c_str());
            }

            printf("\",\n");

            Member m = { ToUpper(signal_name), 0, i, member };
            members.push_back(m);
        }

        const ChildVector properties = iface->GetChildren("property");
        for (ChildVectorIter cit = properties.begin(); cit != properties.end(); ++cit, ++member) {
            const XmlElement* property = *cit;
            const qcc::String name = property->GetAttribute("name");
            const qcc::String type = property->GetAttribute("type");
            const qcc::String readwrite = property->GetAttribute("access");

            char access = '=';
            if (readwrite == "read") {
                access = '<';
            } else if (readwrite == "write") {
                access = '>';
            }

            printf("\t\"@%s%c%s\",\n", name.c_str(), access, type.c_str());

            Property prop = { ToUpper(name), 0, i, member };
            props.push_back(prop);
        }

        printf("\tNULL\n};\n\n");
    }


    const bool has_props = !props.empty();
    const int props_offset = has_props ? 0 : 1;

    printf("\nstatic const AJ_InterfaceDescription interfaces[] = {\n");

    if (has_props) {
        printf("\tAJ_PropertiesIface,\n");
    }

    for (std::list<qcc::String>::const_iterator it = interfaces.begin(); it != interfaces.end(); ++it) {
        printf("\t%s,\n", it->c_str());
    }
    printf("\tNULL\n};\n\n");


    printf("static const AJ_Object AppObjects[] = {\n");
    printf("\t{ \"%s\", interfaces },\n", objectPath.c_str());
    printf("\t{ NULL, NULL }\n");
    printf("};\n\n");




    printf("// Local Objects (service-side)\n");
    if (has_props) {
        printf("#define APP_GET_PROP AJ_APP_MESSAGE_ID(0, 0, AJ_PROP_GET)\n");
        printf("#define APP_SET_PROP AJ_APP_MESSAGE_ID(0, 0, AJ_PROP_SET)\n");
    }

    for (std::list<Member>::const_iterator it = members.begin(); it != members.end(); ++it) {
        Member member = *it;
        printf("#define APP_%s AJ_APP_MESSAGE_ID(%d, %d, %d)\n", member.name.c_str(), member.obj, member.iface - props_offset, member.idx);
    }

    for (std::list<Property>::const_iterator it = props.begin(); it != props.end(); ++it) {
        Property prop = *it;
        printf("#define APP_%s_PROP AJ_APP_PROPERTY_ID(%d, %d, %d)\n", prop.name.c_str(), prop.obj, prop.iface - props_offset, prop.idx);
    }
/*
    printf("\n\n// Proxy Objects (client-side)\n");
    printf("#define PRX_GET_PROP AJ_PRX_MESSAGE_ID(0, 0, AJ_PROP_GET)\n");
    printf("#define PRX_SET_PROP AJ_PRX_MESSAGE_ID(0, 0, AJ_PROP_SET)\n");
    for (std::list<Member>::const_iterator it = members.begin(); it != members.end(); ++it) {
        Member member = *it;
        printf("#define PRX_%s AJ_PRX_MESSAGE_ID(%d, %d, %d)\n", member.name.c_str(), member.obj, member.iface - props_offset, member.idx);
    }

    for (std::list<Property>::const_iterator it = props.begin(); it != props.end(); ++it) {
        Property prop = *it;
        printf("#define PRX_%s_PROP AJ_PRX_PROPERTY_ID(%d, %d, %d)\n", prop.name.c_str(), prop.obj, prop.iface - props_offset, prop.idx);
    }
 */

    return 0;
}
