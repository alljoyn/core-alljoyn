/******************************************************************************
 *    Copyright (c) Open Connectivity Foundation (OCF), AllJoyn Open Source
 *    Project (AJOSP) Contributors and others.
 *
 *    SPDX-License-Identifier: Apache-2.0
 *
 *    All rights reserved. This program and the accompanying materials are
 *    made available under the terms of the Apache License, Version 2.0
 *    which accompanies this distribution, and is available at
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Copyright (c) Open Connectivity Foundation and Contributors to AllSeen
 *    Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for
 *    any purpose with or without fee is hereby granted, provided that the
 *    above copyright notice and this permission notice appear in all
 *    copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 *    WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 *    WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 *    AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 *    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 *    PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 *    TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 *    PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

package org.alljoyn.config.samples.configservice.storage;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.StringWriter;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.UUID;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.alljoyn.bus.AboutKeys;
import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;
import org.xmlpull.v1.XmlSerializer;

import android.util.Xml;

/**
 * A utility that reads and writes a {@link AboutDataImpl} into an xml file
 */
public class PropertyParser 
{
	/** Parsing semantics */
	private static final String VALUE_TRUE = "y";
	private static final String VALUE_FALSE = "n";
	private static final String TAG_CONFIG = "config";
	private static final String TAG_KEYS = "keys";
	private static final String TAG_KEY = "key";
	private static final String TAG_VALUE = "value";
	private static final String ATTR_LANGUAGE = "language";
	private static final String ATTR_PUBLIC = "public";
	private static final String ATTR_ANNOUNCE = "announce";
	private static final String ATTR_WRITABLE = "writable";
	private static final String ATTR_NAME = "name";

	/**
	 * Read a {@link Property} map from an XML InputStream
	 * @param xmlis the XML InputStream
	 * @return a name->property map
	 * @throws ParserConfigurationException
	 * @throws SAXException
	 * @throws IOException
	 */
	public static Map<String, Property> readFromXML(InputStream xmlis)
			throws ParserConfigurationException, SAXException, IOException {
		Map<String, Property> properties = new HashMap<String, Property>(20);

		DocumentBuilderFactory docfactory=DocumentBuilderFactory.newInstance(); 
		docfactory.setIgnoringElementContentWhitespace(true);
		DocumentBuilder docBuilder = docfactory.newDocumentBuilder();
		Document document = docBuilder.parse(xmlis);
		NodeList keys = document.getElementsByTagName(TAG_KEY);
		for (int i=0; i<keys.getLength(); i++) {
			Node key = keys.item(i);
			NamedNodeMap keyAttrs = key.getAttributes();
			String keyName = keyAttrs.getNamedItem(ATTR_NAME).getTextContent();
			boolean isWritable = getBooleanAttribute(keyAttrs, ATTR_WRITABLE);
			boolean isAnnounced = getBooleanAttribute(keyAttrs, ATTR_ANNOUNCE);
			boolean isPublic = getBooleanAttribute(keyAttrs, ATTR_PUBLIC);

			Property property = new Property(keyName, isWritable, isAnnounced, isPublic);

			NodeList values = key.getChildNodes();
			for (int j=0; j<values.getLength();j++) {
				Node value = values.item(j);
				if (TAG_VALUE.equalsIgnoreCase(value.getNodeName())) {
					NamedNodeMap valueAttrs = value.getAttributes();
					Node languageNode = valueAttrs.getNamedItem(ATTR_LANGUAGE);
					String language = Property.NO_LANGUAGE;
					if (languageNode != null) {
						language = languageNode.getNodeValue();
					}
					String text = value.getTextContent();
					if (AboutKeys.ABOUT_APP_ID.equalsIgnoreCase(keyName)) {
						UUID uuid = UUID.fromString(text);
						property.setValue(language, uuid);
					} else if (AboutKeys.ABOUT_SUPPORTED_LANGUAGES.equalsIgnoreCase(keyName)) {
						text = text.substring(1, text.length()-1);
						String[] split = text.split(",");
						String[] trimArray = new String[split.length];
						for(int k = 0; k < split.length; k++) {
							trimArray[k] = split[k].trim();
						}
						Set<String> set = new HashSet<String>(Arrays.asList(trimArray));
						property.setValue(language, set);
					} else {
						property.setValue(language, text);
					}
				}
			}

			properties.put(keyName, property);
		}

		return properties;
	}

	/**
	 * Writes a map of {@link Property} into an XML OutputStream
	 * @param xmlos the XML OutputStream
	 * @param properties the map of properties.
	 * @throws IllegalArgumentException
	 * @throws IllegalStateException
	 * @throws IOException
	 */
	public static void writeToXML(OutputStream xmlos, Map<String, Property> properties)
			throws IllegalArgumentException, IllegalStateException, IOException {
	    XmlSerializer serializer = Xml.newSerializer();
	    StringWriter stringWriter = new StringWriter();
	    serializer.setOutput(stringWriter);
	    serializer.startDocument("UTF-8", true);
	    serializer.startTag("", TAG_CONFIG);
	    serializer.startTag("", TAG_KEYS);
	    for (String key: properties.keySet()) {
	    	Property acProperty = properties.get(key);
	    	serializer.startTag("", TAG_KEY);
	    	serializer.attribute("", ATTR_NAME, acProperty.getName());
	    	serializer.attribute("", ATTR_WRITABLE, acProperty.isWritable() ? VALUE_TRUE : VALUE_FALSE);
	    	serializer.attribute("", ATTR_ANNOUNCE, acProperty.isAnnounced() ? VALUE_TRUE : VALUE_FALSE);
	    	serializer.attribute("", ATTR_PUBLIC, acProperty.isPublic() ? VALUE_TRUE : VALUE_FALSE);
	    	Set<String> languages = acProperty.getLanguages();
	    	for (String language: languages) {
	    		serializer.startTag("", TAG_VALUE);
	    		if (!Property.NO_LANGUAGE.equals(language)) {
	    			serializer.attribute("", ATTR_LANGUAGE, language);
	    		}
	    		
	    		Object value = acProperty.getValue(language, Property.NO_LANGUAGE);
	    		serializer.text(String.valueOf(value));
	    		serializer.endTag("", TAG_VALUE);
	    	}
	    	serializer.endTag("", TAG_KEY);
	    }
	    serializer.endTag("", TAG_KEYS);
	    serializer.endTag("", TAG_CONFIG);
	    serializer.endDocument();
	    OutputStreamWriter osWriter = new OutputStreamWriter(xmlos);
	    osWriter.write(stringWriter.toString());
	    osWriter.close();
	}

	private static boolean getBooleanAttribute(NamedNodeMap attributes, String attributeName) {
		Node attrNode = attributes.getNamedItem(attributeName);
		if (attrNode != null) {
			String attrValue = attrNode.getTextContent();
			return VALUE_TRUE.equalsIgnoreCase(attrValue);
		} else {
			return false;
		}
	}
}
