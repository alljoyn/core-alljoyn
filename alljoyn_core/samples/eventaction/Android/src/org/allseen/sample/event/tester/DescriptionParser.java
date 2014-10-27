/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
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

package org.allseen.sample.event.tester;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.StringReader;
import java.util.LinkedList;
import java.util.List;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.xml.sax.Attributes;
import org.xml.sax.EntityResolver;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.XMLReader;
import org.xml.sax.helpers.DefaultHandler;

import android.util.Log;

public class DescriptionParser {

	//=============================================//

	class IntrospectionParser extends DefaultHandler {

		private XMLReader xmlReader = null;
		private SAXParser saxParser = null;

		private DescriptionParser currentNode = null;
		private String iface;
		private boolean sawRootNode = false;
		private Description currInfo = null;
		private boolean ignoreArgDescriptions = false;
		private String inbetween = "";

		public IntrospectionParser() throws IOException, ParserConfigurationException, SAXException {
			SAXParserFactory parserFactory = SAXParserFactory.newInstance();
			parserFactory.setNamespaceAware(false);

			saxParser = parserFactory.newSAXParser();

			xmlReader = saxParser.getXMLReader();
			xmlReader.setContentHandler(this);
			xmlReader.setEntityResolver(new EntityResolver() {
				public InputSource resolveEntity(String publicId, String systemId) 
						throws SAXException, java.io.IOException {
					return new InputSource(new ByteArrayInputStream("".getBytes()));
				}
			});
		}

		public void parse(DescriptionParser node, String xml) throws SAXException {
			this.currentNode = node;
			sawRootNode = false;
			try{
				xmlReader.parse(new InputSource(new StringReader(xml)));
			} catch(IOException cantReallyHappen) {
				Log.e("IntrospectionNode", "Failed to read the XML: '" + cantReallyHappen.getMessage() + "', ", cantReallyHappen);
			} catch(Exception shouldNotHappen) {
				Log.e("IntrospectionNode", "Failed to read the XML: '" + shouldNotHappen.getMessage() + "', ", shouldNotHappen);
			}
			this.currentNode = null;
		}

		public void startElement(String namespaceURI, String localName, 
				String qName, Attributes attrs) throws SAXException {
			if(qName.equals("node")) {
				currInfo = null;
				if(!sawRootNode) {
					sawRootNode = true;
					return;
				}
				currentNode.addChild(getNameAttr(attrs));
			}else if(qName.equals("interface")){
				if(null == currentNode) throw new SAXException("interface not in node");
				currInfo = null;
				currentNode.interfaces.add(getNameAttr(attrs));
				iface = getNameAttr(attrs);
			}
			else if(qName.equals("method")){
				if(null == currentNode) throw new SAXException("method not in node");
				ActionDescription temp = new ActionDescription();
				temp.setIface(iface);
				temp.setPath(currentNode.path);
				temp.setMemberName(getNameAttr(attrs));
				currInfo = temp;
			}
			else if(qName.equals("signal")){
				if(null == currentNode) throw new SAXException("signal not in node");
				try{
					EventDescription temp = new EventDescription();
					temp.setIface(iface);
					temp.setPath(currentNode.path);
					temp.setMemberName(getNameAttr(attrs));
					currInfo = temp;
				}catch(Exception e) {
					e.printStackTrace();
				}
				try{
					((EventDescription)currInfo).setSessionless(getIfSessionless(attrs));
				} catch(Exception e) {
					/** Do nothing with this since the isSessionless attribute was not found */
				}
			}
			else if(qName.equals("arg")) {
				String arg = getSignatureAttr(attrs);
				if(currInfo instanceof EventDescription)
					((EventDescription)currInfo).addSignature(arg);
				//TODO: parse arg description, for now ignore them
				ignoreArgDescriptions = true;
			}
		}

		public void endElement(String namespaceURI, String localName, 
				String qName) throws SAXException {
			if(qName.equals("method") || qName.equals("signal") || qName.equals("property") || qName.equals("node")){
				currInfo = null;
				ignoreArgDescriptions = false;
			} else if(ignoreArgDescriptions) {
				if(qName.equals("arg")) {
					ignoreArgDescriptions = false;
				}
			}else if(qName.equals("description")){
				if(currInfo != null) {
					if(currInfo instanceof EventDescription) {
						((EventDescription)currInfo).setDescription(inbetween);
						currentNode.events.add(((EventDescription)currInfo));
					}
					if(currInfo instanceof ActionDescription) {
						((ActionDescription)currInfo).setDescription(inbetween);
						currentNode.actions.add(((ActionDescription)currInfo));
					}
				}
			}
		}

		@Override
		public void characters(char[] ac, int i, int j) throws SAXException {
			inbetween = new String(ac, i, j);
		}

		private boolean getIfSessionless(Attributes attrs) throws SAXException {
			int i = attrs.getIndex("sessionless");
			if(-1 == i) throw new SAXException("inner node without a name");
			return attrs.getValue(i).equals("true");
		}

		private String getNameAttr(Attributes attrs) throws SAXException {
			int i = attrs.getIndex("name");
			if(-1 == i) throw new SAXException("inner node without a name");
			return attrs.getValue(i);
		}
		
		private String getSignatureAttr(Attributes attrs) throws SAXException {
			int i = attrs.getIndex("type");
			if(-1 == i) throw new SAXException("inner node without a name");
			return attrs.getValue(i);
		}
	}

	//================================================//
	//                END OF NESTED CLASSES           //
	//================================================//

	private String path = null;
	private IntrospectionParser parser = null;

	private List<DescriptionParser> children = new LinkedList<DescriptionParser>();
	private List<String> interfaces = new LinkedList<String>();
	private List<Description> events = new LinkedList<Description>();
	private List<Description> actions = new LinkedList<Description>();

	public DescriptionParser(String path) throws ParserConfigurationException, IOException, SAXException { 
		this.path = path;
		this.parser = new IntrospectionParser();
	}

	private DescriptionParser(String path, IntrospectionParser parser){
		this.path = path;
		this.parser = parser;
	}

	protected void addChild(String name) {
		StringBuilder sb = new StringBuilder(path);
		if(!name.endsWith("/") && !path.equals("/"))
			sb.append('/');
		sb.append(name);
		children.add(new DescriptionParser(sb.toString(), parser));
	}

	public String getPath() {
		return path;
	}

	public void parse(String xml) throws SAXException {
		parser.parse(this, xml);
	}

	public List<DescriptionParser> getChidren() {
		return children;
	}

	public List<String> getInterfaces() {
		return interfaces;
	}

	public List<Description> getEvents() {
		return events;
	}

	public List<Description> getActions() {
		return actions;
	}
}
