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

package org.alljoyn.services.common.utils;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import java.util.UUID;

import org.alljoyn.about.AboutKeys;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.Variant;

/**
 * A utility class for transforming objects from one format to another
 */
public class TransportUtil
{

    /**
     * A utility method for converting from a String-&gt;Object map to a String-&gt;Variant map
     * @param sToObjectMap input String-&gt;Object map.
     * @return String-&gt;Variant map
     */
    public static Map<String, Variant> toVariantMap(Map<String, Object> sToObjectMap)
    {
        Map<String,Variant> variantMap = new HashMap<String, Variant>(sToObjectMap.size());

        for (String key: sToObjectMap.keySet()) {

            if (AboutKeys.ABOUT_SUPPORTED_LANGUAGES.equalsIgnoreCase(key))
            {

                Object supportLangs = sToObjectMap.get(key);

                if ( supportLangs == null ) {
                    throw new IllegalArgumentException("Failed in creating toVariantMap, the key: 'SupportedLanguages' was found but the value is NULL");
                }

                if ( supportLangs instanceof Set<?> ) {
                    @SuppressWarnings("unchecked")
                    Set<String> value = (Set<String>) supportLangs;
                    String[] toArray  = value.toArray(new String[]{});
                    variantMap.put(key, new Variant(toArray));
                }
                else {
                    variantMap.put(key, new Variant(supportLangs));
                }

            }//if :: ABOUT_SUPPORTED_LANGUAGES
            else
            {
                Object value = sToObjectMap.get(key);

                if ( value == null ) {
                    throw new IllegalArgumentException("Failed in creating toVariantMap, the key: '" + key + "' was found but the value is NULL");
                }

                if (value instanceof UUID)
                {
                    byte [] byteArr = uuidToByteArray((UUID)value);
                    variantMap.put(key, new Variant(byteArr, "ay"));
                }
                else {
                    variantMap.put(key, new Variant(value));
                }
            }
        }//for :: sToObjectMap

        return variantMap;
    }

    /**
     * A utility method for converting from a String-&gt;Variant map to a String-&gt;Object map
     *
     * @param sToVariantMap input String-&gt;Variant map.
     *
     * @throws BusException indicating failure to convert Map from Variant map to Object map
     *
     * @return String-&gt;Object map
     *
     */
    public static Map<String, Object> fromVariantMap(Map<String, Variant> sToVariantMap) throws BusException
    {
        Map<String,Object> objectMap = new HashMap<String, Object>(sToVariantMap.size());
        for (String key: sToVariantMap.keySet()) {

            Variant variant = sToVariantMap.get(key);
            Object object;
            if (AboutKeys.ABOUT_SUPPORTED_LANGUAGES.equalsIgnoreCase(key))
            {
                String[] languages = variant.getObject(String[].class);
                object = languages;
            }
            else if(AboutKeys.ABOUT_APP_ID.equalsIgnoreCase(key)) {
                byte[] b   = variant.getObject(byte[].class);
                UUID appId = byteArrayToUUID(b);
                object = appId;
            }
            else
            {
                object = variant.getObject(Object.class);
            }
            objectMap.put(key, object);
        }

        return objectMap;
    }

    /**
     * A utility method to convert from chars to bytes
     * @param charArray char[]
     * @return byte[]
     */
    public static byte[] toByteArray(char[] charArray)
    {
        if ( charArray == null ) {
            return null;
        }

        byte[] result = new byte[charArray.length];
        for (int i=0; i < charArray.length; i++)
        {
            result[i] = (byte) charArray[i];
        }

        return result;
    }

    /**
     * A utility method to convert from bytes to chars
     * @param byteArray array of bytes to convert to an array of chars
     * @return a char array
     */
    public static char[] toCharArray(byte[] byteArray)
    {
        if ( byteArray == null ) {
            return null;
        }

        char[] result = new char[byteArray.length];
        for (int i=0; i < byteArray.length; i++)
        {
            result[i] = (char) byteArray[i];
        }

        return result;
    }

    /**
     * A utility method to convert from UUID to byte array
     * @param uuid Universally Unique identifier
     * @return the byte representation of that UUID
     */
    public static byte[] uuidToByteArray(UUID uuid)
    {

        if ( uuid == null ) {
            return null;
        }

        long msUuid = uuid.getMostSignificantBits();
        long lsUuid = uuid.getLeastSignificantBits();
        byte[] byteArrayUuid = new byte[16];

        for (int i = 0; i < 8; i++) {
            byteArrayUuid[i] = (byte) (msUuid >>> 8 * (7 - i));
        }
        for (int i = 8; i < 16; i++) {
            byteArrayUuid[i] = (byte) (lsUuid >>> 8 * (7 - i));
        }

        return byteArrayUuid;
    }

    /**
     * A utility method to convert from byte array to UUID
     * @param bAppId byte representation of a UUID
     * @return UUID Returns the created {@link UUID} object or NULL on fail
     */
    public static UUID byteArrayToUUID(byte[] bAppId) {
        long msUuid = 0;
        long lsUuid = 0;

        if ( bAppId == null || bAppId.length != 16 ) {
            return null;
        }

        for (int i = 0; i < 8; i++) {
            msUuid = (msUuid << 8) | (bAppId[i] & 0xff);
        }

        for (int i = 8; i < 16; i++) {
            lsUuid = (lsUuid << 8) | (bAppId[i] & 0xff);
        }

        UUID result = new UUID(msUuid, lsUuid);

        return result;
    }

}
