/******************************************************************************
 * Copyright (c) 2013-2014, AllSeen Alliance. All rights reserved.
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

package org.alljoyn.about.sample;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;

import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;

import org.alljoyn.services.common.PropertyStore;
import org.alljoyn.services.common.PropertyStoreException;
import org.alljoyn.services.common.PropertyStore.Filter;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusListener;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.Mutable;
import org.alljoyn.bus.SessionOpts;
import org.alljoyn.bus.SessionPortListener;
import org.alljoyn.bus.Status;
import org.alljoyn.about.AboutKeys;
import org.alljoyn.about.AboutService;
import org.alljoyn.about.AboutServiceImpl;

public class AboutServerSample {

    static {
        System.loadLibrary("alljoyn_java");
    }

    private static final short CONTACT_PORT=1000;
    static BusAttachment mBus;

    static boolean sessionEstablished = false;
    static int sessionId;


    private static class MyBusListener extends BusListener {
        public void nameOwnerChanged(String busName, String previousOwner, String newOwner) {
            if ("com.my.well.known.name".equals(busName)) {
                System.out.println("BusAttachement.nameOwnerChanged(" + busName + ", " + previousOwner + ", " + newOwner);
            }
        }
    }

    private static  class  PropertyStoreImpl implements PropertyStore{

        public static class Property
        {
            private final boolean m_isLocalized;
            private final boolean m_isWritable;
            private final boolean m_isAnnounced;
            private final boolean m_isPublic;
            private final String m_name;
            private Object m_object=null;
            private Map<String, String> m_localizable_value = null;
            // public.write.announce

            public Property(String m_name,Object value, boolean isPublic,boolean isWritable,boolean isAnnounced)
            {
                super();
                this.m_isLocalized = false;
                this.m_isWritable = isWritable;
                this.m_isAnnounced = isAnnounced;
                this.m_isPublic = isPublic;
                this.m_name = m_name;
                this.m_object=value;
            }

            /*
             * localizable property
             */
            public Property(String m_name, String value, String languageTag, boolean isPublic, boolean isWritable, boolean isAnnounced)
            {
                this.m_isLocalized = true;
                this.m_isWritable = isWritable;
                this.m_isAnnounced = isAnnounced;
                this.m_isPublic = isPublic;
                this.m_name = m_name;
                if(this.m_localizable_value == null) {
                    this.m_localizable_value = new HashMap<String, String>();
                }
                this.m_localizable_value.put(languageTag, value);
            }

            public void addLocalizedValue(String value, String languageTag)
            {
                if (this.m_isLocalized == false) {
                    return;
                }
                if(this.m_localizable_value == null) {
                    this.m_localizable_value = new HashMap<String, String>();
                }
                this.m_localizable_value.put(languageTag, value);
            }

            public boolean isLocalized()
            {
                return m_isLocalized;
            }
            public boolean isWritable()
            {
                return m_isWritable;
            }
            public boolean isAnnounced()
            {
                return m_isAnnounced;
            }
            public boolean isPublic()
            {
                return m_isPublic;
            }
            public String getName()
            {
                return m_name;
            }

            public Object getObject() {
                return m_object;
            }

            public Object getObject(String languageTag) {
                return (Object)m_localizable_value.get(languageTag);
            }
        }


        private Map<String,List<Property>> m_internalMap=null;

        public PropertyStoreImpl(Map<String,List<Property>> dataMap ){
            m_internalMap=dataMap;
        }

        public void readAll(String languageTag, Filter filter, Map<String, Object> dataMap) throws PropertyStoreException{
            if (filter==Filter.ANNOUNCE)
            {
                if (m_internalMap!=null)
                {
                    List<Property> langauge=m_internalMap.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
                    if (langauge!=null)
                    {
                        languageTag=(String)langauge.get(0).getObject();
                    }else{
                        throw new PropertyStoreException(PropertyStoreException.UNSUPPORTED_LANGUAGE);
                    }

                    Set<Map.Entry<String, List<Property>>> entries = m_internalMap.entrySet();
                    for(Map.Entry<String, List<Property>> entry : entries) {
                        String key = entry.getKey();
                        List<Property> properyList = entry.getValue();
                        for (int i=0;i<properyList.size();i++)
                        {
                            Property property=properyList.get(i);
                            if (!property.isAnnounced())
                                continue;
                            if (property.isLocalized()){
                                dataMap.put(key, property.getObject(languageTag));
                            } else {
                                dataMap.put(key, property.getObject());
                            }
                        }
                    }
                }else {
                    throw new  PropertyStoreException(PropertyStoreException.UNSUPPORTED_KEY);
                }
            }
            else if (filter==Filter.READ)
            {
                if (languageTag!=null && languageTag.length()>1)
                {
                    List<Property> supportedLanguages=m_internalMap.get(AboutKeys.ABOUT_SUPPORTED_LANGUAGES);
                    if (supportedLanguages==null)
                        throw new  PropertyStoreException(PropertyStoreException.UNSUPPORTED_KEY);
                    if (!( supportedLanguages.get(0).getObject() instanceof Set<?>)){
                        throw new  PropertyStoreException(PropertyStoreException.UNSUPPORTED_LANGUAGE);
                    }else{
                        @SuppressWarnings("unchecked")
                        Set<String> languages=(Set<String>)supportedLanguages.get(0).getObject();
                        if (!languages.contains(languageTag)){
                            throw new  PropertyStoreException(PropertyStoreException.UNSUPPORTED_LANGUAGE);
                        }
                    }
                }else{

                    List<Property> langauge=m_internalMap.get(AboutKeys.ABOUT_DEFAULT_LANGUAGE);
                    if (langauge!=null)
                    {
                        languageTag=(String)langauge.get(0).getObject();
                    }else{
                        throw new PropertyStoreException(PropertyStoreException.UNSUPPORTED_LANGUAGE);
                    }
                }
                Set<Map.Entry<String, List<Property>>> entries = m_internalMap.entrySet();
                for(Map.Entry<String, List<Property>> entry : entries) {
                    String key = entry.getKey();
                    List<Property> properyList = entry.getValue();
                    for (int i=0;i<properyList.size();i++)
                    {
                        Property property=properyList.get(i);
                        if (!property.isPublic())
                            continue;
                        if (property.isLocalized()){
                            dataMap.put(key, property.getObject(languageTag));
                        } else {
                            dataMap.put(key, property.getObject());
                        }
                    }
                }
            }//end of read.
            else throw new PropertyStoreException(PropertyStoreException.ILLEGAL_ACCESS);
        }

        public void update(String key, String languageTag, Object newValue) throws PropertyStoreException{
        }

        public void reset(String key, String languageTag) throws PropertyStoreException{
        }

        public void resetAll() throws PropertyStoreException{
        }
    }

    public static void main(String[] args) {

        Runtime.getRuntime().addShutdownHook(new Thread() {
            public void run() {
                AboutServiceImpl.getInstance().stopAboutServer();
                mBus.release();
            }
        });

        mBus = new BusAttachment("AppName", BusAttachment.RemoteMessage.Receive);

        BusListener myBusListener = new MyBusListener();
        mBus.registerBusListener(myBusListener);

        Status status = mBus.connect();
        if (status != Status.OK) {
            System.exit(0);
            return;
        }

        Map<String, List<PropertyStoreImpl.Property>> data=new HashMap<String, List<PropertyStoreImpl.Property>>();
        // With this implementation of a propertyStore the Default language must be the first entry.
        data.put(AboutKeys.ABOUT_DEFAULT_LANGUAGE, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_DEFAULT_LANGUAGE,"en",true,true,true))));

        PropertyStoreImpl.Property deviceName = new PropertyStoreImpl.Property(AboutKeys.ABOUT_DEVICE_NAME,"MyDeviceName", "en",true,false,true);
        deviceName.addLocalizedValue("Nombre de mi dispositivo", "es");
        deviceName.addLocalizedValue("Мое устройство Имя", "ru");
        data.put(AboutKeys.ABOUT_DEVICE_NAME, new ArrayList<PropertyStoreImpl.Property>(Arrays.asList(deviceName)));

        data.put(AboutKeys.ABOUT_DEVICE_ID, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_DEVICE_ID,"1231232145667745675477",true,false,true))));

        data.put(AboutKeys.ABOUT_DESCRIPTION, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_DESCRIPTION,"This is car",true,false,false))));


        final UUID uid = UUID.fromString("38400000-8cf0-11bd-b23e-10b96e4ef00d");
        data.put(AboutKeys.ABOUT_APP_ID, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_APP_ID,uid,true,false,true))));

        PropertyStoreImpl.Property appName = new PropertyStoreImpl.Property(AboutKeys.ABOUT_APP_NAME,"AboutConfig", "en" ,true,false,true);
        appName.addLocalizedValue("Acerca Config", "es");
        appName.addLocalizedValue("О Настройка", "ru");
        data.put(AboutKeys.ABOUT_APP_NAME, new ArrayList<PropertyStoreImpl.Property>(Arrays.asList(appName)));

        PropertyStoreImpl.Property manufacture = new PropertyStoreImpl.Property(AboutKeys.ABOUT_MANUFACTURER,"Company", "en", true,false,true);
        manufacture.addLocalizedValue("empresa", "es");
        manufacture.addLocalizedValue("компания", "ru");
        data.put(AboutKeys.ABOUT_MANUFACTURER, new ArrayList<PropertyStoreImpl.Property>(Arrays.asList(manufacture)));

        data.put(AboutKeys.ABOUT_MODEL_NUMBER, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_MODEL_NUMBER,"Wxfy388i",true,false,false))));

        data.put(AboutKeys.ABOUT_SUPPORTED_LANGUAGES, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_SUPPORTED_LANGUAGES,new HashSet <String>(){{add("en");add("es");add("ru");}},true,false,false))));

        PropertyStoreImpl.Property description = new PropertyStoreImpl.Property(AboutKeys.ABOUT_DESCRIPTION,"Wonderful description of the application.", "en", true,false,false);
        description.addLocalizedValue("Maravillosa descripción de la aplicación.", "es");
        description.addLocalizedValue("Замечательный описание приложения.", "ru");
        data.put(AboutKeys.ABOUT_DESCRIPTION, new ArrayList<PropertyStoreImpl.Property>(Arrays.asList(description)));

        data.put(AboutKeys.ABOUT_DATE_OF_MANUFACTURE, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_DATE_OF_MANUFACTURE,"10/1/2199",true,false,false))));

        data.put(AboutKeys.ABOUT_SOFTWARE_VERSION, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_SOFTWARE_VERSION,"12.20.44 build 44454",true,false,false))));

        data.put(AboutKeys.ABOUT_AJ_SOFTWARE_VERSION, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_AJ_SOFTWARE_VERSION,"3.3.2",true,false,false))));

        data.put(AboutKeys.ABOUT_HARDWARE_VERSION, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_HARDWARE_VERSION,"355.499. b",true,false,false))));

        data.put(AboutKeys.ABOUT_SUPPORT_URL, new ArrayList<PropertyStoreImpl.Property>(
                Arrays.asList(new PropertyStoreImpl.Property(AboutKeys.ABOUT_SUPPORT_URL,"http://www.allseenalliance.org",true,false,false))));

        PropertyStore impl=new PropertyStoreImpl(data);

        try{
            AboutServiceImpl.getInstance().startAboutServer(CONTACT_PORT, impl, mBus);
        }catch (Exception e){
            e.printStackTrace();
        }

        try{
            byte aboutIconContent[] = new byte[]  {
                    (byte)0x89, (byte)0x50, (byte)0x4E, (byte)0x47, (byte)0x0D, (byte)0x0A, (byte)0x1A, (byte)0x0A,
                    (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x0D, (byte)0x49, (byte)0x48, (byte)0x44, (byte)0x52,
                    (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x0A, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x0A,
                    (byte)0x08, (byte)0x02, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x02, (byte)0x50, (byte)0x58,
                    (byte)0xEA, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x04, (byte)0x67, (byte)0x41, (byte)0x4D,
                    (byte)0x41, (byte)0x00, (byte)0x00, (byte)0xAF, (byte)0xC8, (byte)0x37, (byte)0x05, (byte)0x8A,
                    (byte)0xE9, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x19, (byte)0x74, (byte)0x45, (byte)0x58,
                    (byte)0x74, (byte)0x53, (byte)0x6F, (byte)0x66, (byte)0x74, (byte)0x77, (byte)0x61, (byte)0x72,
                    (byte)0x65, (byte)0x00, (byte)0x41, (byte)0x64, (byte)0x6F, (byte)0x62, (byte)0x65, (byte)0x20,
                    (byte)0x49, (byte)0x6D, (byte)0x61, (byte)0x67, (byte)0x65, (byte)0x52, (byte)0x65, (byte)0x61,
                    (byte)0x64, (byte)0x79, (byte)0x71, (byte)0xC9, (byte)0x65, (byte)0x3C, (byte)0x00, (byte)0x00,
                    (byte)0x00, (byte)0x18, (byte)0x49, (byte)0x44, (byte)0x41, (byte)0x54, (byte)0x78, (byte)0xDA,
                    (byte)0x62, (byte)0xFC, (byte)0x3F, (byte)0x95, (byte)0x9F, (byte)0x01, (byte)0x37, (byte)0x60,
                    (byte)0x62, (byte)0xC0, (byte)0x0B, (byte)0x46, (byte)0xAA, (byte)0x34, (byte)0x40, (byte)0x80,
                    (byte)0x01, (byte)0x00, (byte)0x06, (byte)0x7C, (byte)0x01, (byte)0xB7, (byte)0xED, (byte)0x4B,
                    (byte)0x53, (byte)0x2C, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x00, (byte)0x49, (byte)0x45,
                    (byte)0x4E, (byte)0x44, (byte)0xAE, (byte)0x42, (byte)0x60, (byte)0x82
            };

            AboutServiceImpl.getInstance().registerIcon("image/png", "http://tinyurl.com/m3ra78j", aboutIconContent);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        Mutable.ShortValue contactPort = new Mutable.ShortValue(CONTACT_PORT);

        SessionOpts sessionOpts = new SessionOpts();
        sessionOpts.traffic = SessionOpts.TRAFFIC_MESSAGES;
        sessionOpts.isMultipoint = false;
        sessionOpts.proximity = SessionOpts.PROXIMITY_ANY;
        sessionOpts.transports = SessionOpts.TRANSPORT_ANY;

        status = mBus.bindSessionPort(contactPort, sessionOpts,
                new SessionPortListener() {
            public boolean acceptSessionJoiner(short sessionPort, String joiner, SessionOpts sessionOpts) {
                System.out.println("SessionPortListener.acceptSessionJoiner called");
                if (sessionPort == CONTACT_PORT) {
                    return true;
                } else {
                    return false;
                }
            }
            public void sessionJoined(short sessionPort, int id, String joiner) {
                System.out.println(String.format("SessionPortListener.sessionJoined(%d, %d, %s)", sessionPort, id, joiner));
                sessionId = id;
                sessionEstablished = true;
            }
        });

        if (mBus.isConnected() && mBus.getUniqueName().length() > 0) {
            status=mBus.advertiseName( mBus.getUniqueName(), SessionOpts.TRANSPORT_ANY);
            if (status != Status.OK) {
                System.out.println("AdvertiseName failed Status = " + status);
                System.exit(0);
                return;
            }
        }

        System.out.println("BusAttachment session established");
        AboutServiceImpl.getInstance().announce();
        while (true) {
            try {
                Thread.sleep(10000);
            } catch (InterruptedException e) {
                System.out.println("Thead Exception caught");
                e.printStackTrace();
            }
        }
    }// end of main
}
