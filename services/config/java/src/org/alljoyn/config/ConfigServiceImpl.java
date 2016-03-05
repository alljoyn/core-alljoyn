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

package org.alljoyn.config;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.BusObject;
import org.alljoyn.bus.ErrorReplyBusException;
import org.alljoyn.bus.Status;
import org.alljoyn.bus.Variant;
import org.alljoyn.config.client.ConfigClient;
import org.alljoyn.config.client.ConfigClientImpl;
import org.alljoyn.config.server.ConfigChangeListener;
import org.alljoyn.config.server.FactoryResetHandler;
import org.alljoyn.config.server.PassphraseChangedListener;
import org.alljoyn.config.server.RestartHandler;
import org.alljoyn.config.server.SetPasswordHandler;
import org.alljoyn.config.transport.ConfigTransport;
import org.alljoyn.services.common.BusObjectDescription;
import org.alljoyn.services.common.LanguageNotSupportedException;
import org.alljoyn.services.common.PropertyStore;
import org.alljoyn.services.common.PropertyStore.Filter;
import org.alljoyn.services.common.PropertyStoreException;
import org.alljoyn.services.common.ServiceAvailabilityListener;
import org.alljoyn.services.common.ServiceCommonImpl;
import org.alljoyn.services.common.utils.TransportUtil;

/**
 * An implementation of the ConfigService interface
 */
public class ConfigServiceImpl extends ServiceCommonImpl implements ConfigService {

    /********* General *********/

    private static Object m_instance;

    /********* Sender *********/

    private PropertyStore m_propertyStore;
    private ConfigInterface m_configInterface;
    private RestartHandler m_restartHandler;
    private FactoryResetHandler m_factoryResetHandler;
    private PassphraseChangedListener m_passphraseChangeListener;
    private ConfigChangeListener m_configChangeListener;

    private SetPasswordHandler m_setPasswordHandler;

    /******** Receiver ********/

    /**
     * A singleton ConfigService instance
     *
     * @return {@link ConfigService} instance
     */
    public static ConfigService getInstance() {
        if (m_instance == null)
            m_instance = new ConfigServiceImpl();
        return (ConfigService) m_instance;
    }

    private ConfigServiceImpl() {
        super();
        TAG = "ioe" + ConfigClientImpl.class.getSimpleName();
    }

    /**
     * **************** Server **************
     */
    @Override
    public void startConfigServer(ConfigDataStore configDataStore, ConfigChangeListener configChangeListener, RestartHandler restartHandler, FactoryResetHandler factoryResetHandler,
            PassphraseChangedListener passphraseChangeListener, BusAttachment bus) throws Exception {
        startConfigServer((PropertyStore)configDataStore, configChangeListener, restartHandler, factoryResetHandler, passphraseChangeListener, bus);
    }

    @Override
    public void startConfigServer(PropertyStore propertyStore, ConfigChangeListener configChangeListener, RestartHandler restartHandler, FactoryResetHandler factoryResetHandler,
            PassphraseChangedListener passphraseChangeListener, BusAttachment bus) throws Exception {
        setBus(bus);
        super.startServer();
        m_propertyStore = propertyStore;
        m_restartHandler = restartHandler;
        m_factoryResetHandler = factoryResetHandler;
        m_passphraseChangeListener = passphraseChangeListener;
        m_configChangeListener = configChangeListener;

        registerConfigInterface();
    }

    private void registerConfigInterface() {
        m_configInterface = new ConfigInterface();
        Status status = getBus().registerBusObject(m_configInterface, ConfigTransport.OBJ_PATH);
        getLogger().debug(TAG, String.format("BusAttachment.registerBusObject(m_aboutConfigInterface): %s", status));
        if (status != Status.OK) {
            return;
        }
    }

    /**
     * Returns the ConfigInterface field
     *
     * @return the ConfigInterface field
     */
    public ConfigInterface getConfigInterface() {
        if (m_configInterface == null)
            m_configInterface = new ConfigInterface();
        return m_configInterface;
    }

    /**
     * *************** Client *************
     */
    @Override
    public void startConfigClient(BusAttachment bus) throws Exception {
        super.startClient();
        setBus(bus);
    }

    @Override
    public void stopConfigServer() throws Exception {
        if (m_configInterface != null) {
            getBus().unregisterBusObject(m_configInterface);
        }
        super.stopServer();
    }

    @Override
    public void stopConfigClient() throws Exception {
        super.stopClient();
    }

    @Override
    public ConfigClient createFeatureConfigClient(String deviceName, ServiceAvailabilityListener serviceAvailabilityListener, short port) throws Exception {
        return new ConfigClientImpl(deviceName, getBus(), serviceAvailabilityListener, port);
    }

    /**
     * The AllJoyn BusObject that exposes the Config Interface of this device
     * over the Bus. One Bus Object represents 2 interfaces.
     */
    private class ConfigInterface implements BusObject, ConfigTransport {

        @Override
        public Map<String, Variant> GetConfigurations(String languageTag) throws BusException {
            Map<String, Object> persistedConfiguration = new HashMap<String, Object>();

            try {
                m_propertyStore.readAll(languageTag, Filter.WRITE, persistedConfiguration);
            } catch (PropertyStoreException e) {
                if (e.getReason() == PropertyStoreException.UNSUPPORTED_LANGUAGE) {
                    throw new LanguageNotSupportedException();
                } else {
                    e.printStackTrace();
                }
            }

            Map<String, Variant> configuration = TransportUtil.toVariantMap(persistedConfiguration);
            return configuration;
        }

        @Override
        public void ResetConfigurations(String language, String[] fieldsToRemove) throws BusException {
            for (String key : fieldsToRemove) {
                try {
                    m_propertyStore.reset(key, language);
                } catch (PropertyStoreException e) {
                    if (e.getReason() == PropertyStoreException.UNSUPPORTED_LANGUAGE) {
                        throw new LanguageNotSupportedException();
                    } else {
                        e.printStackTrace();
                    }
                }
            }

            m_configChangeListener.onResetConfiguration(language, fieldsToRemove);
        }

        @Override
        public void UpdateConfigurations(String languageTag, Map<String, Variant> configuration) throws BusException {

            Map<String, Object> toObjectMap = TransportUtil.fromVariantMap(configuration);

            // notify application
            if (m_propertyStore != null) {
                for (Entry<String, Object> entry : toObjectMap.entrySet()) {
                    try {
                        m_propertyStore.update(entry.getKey(), languageTag, entry.getValue());
                    } catch (PropertyStoreException e) {
                        if (e.getReason() == PropertyStoreException.UNSUPPORTED_LANGUAGE) {
                            throw new LanguageNotSupportedException();
                        } else if (e.getReason() == PropertyStoreException.INVALID_VALUE) {
                            throw new ErrorReplyBusException("org.alljoyn.Error.InvalidValue", "Invalid value");
                        } else {
                            e.printStackTrace();
                        }
                    }
                }

                if (m_configChangeListener != null) {
                    m_configChangeListener.onConfigChanged(configuration, languageTag);
                }
            }
        }

        @Override
        public void SetPasscode(String daemonRealm, byte[] passphrase) throws BusException {
            if (m_setPasswordHandler != null) {
                m_setPasswordHandler.setPassword(daemonRealm, TransportUtil.toCharArray(passphrase));
            }

            if (m_passphraseChangeListener != null) {
                m_passphraseChangeListener.onPassphraseChanged(passphrase);
            }
        }

        @Override
        public void FactoryReset() throws BusException {
            try {
                m_propertyStore.resetAll();
            } catch (PropertyStoreException e) {
                e.printStackTrace();
            }

            if (m_factoryResetHandler != null) {
                m_factoryResetHandler.doFactoryReset();
            }
        }

        @Override
        public void Restart() throws BusException {
            if (m_restartHandler != null) {
                m_restartHandler.restart();
            } else {
                throw new ErrorReplyBusException("org.alljoyn.Error.FeatureNotAvailable", "Feature not available");
            }
        }

        @Override
        public short getVersion() throws BusException {
            return PROTOCOL_VERSION;
        }
    }

    @Override
    public void setSetPasswordHandler(SetPasswordHandler setPasswordHandler) {
        m_setPasswordHandler = setPasswordHandler;
    }

    @Override
    public List<BusObjectDescription> getBusObjectDescriptions() {
        return null;
    }

}// AboutConfigServiceImpl
