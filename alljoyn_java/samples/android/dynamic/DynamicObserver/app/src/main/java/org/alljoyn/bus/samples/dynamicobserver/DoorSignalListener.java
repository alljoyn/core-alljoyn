package org.alljoyn.bus.samples.dynamicobserver;

import org.alljoyn.bus.AbstractDynamicBusObject;
import org.alljoyn.bus.BusAttachment;
import org.alljoyn.bus.BusException;
import org.alljoyn.bus.MessageContext;
import org.alljoyn.bus.defs.BusObjectInfo;

import java.util.Arrays;

/**
 * The DoorSignalListener (consumer-side DynamicBusObject) handles receipt of bus signals from
 * a remote Door interface provider.
 */
public class DoorSignalListener extends AbstractDynamicBusObject {

    private final BusHandler mBushandler;

    /**
     * Create door listener used to handle received bus signals.
     *
     * @param bus the bus attachment with which this listener is registered.
     * @param objectDef the bus object's interface(s) definition.
     * @param handler used to relay notifications regarding received signals.
     */
    public DoorSignalListener(BusAttachment bus, BusObjectInfo objectDef, BusHandler handler) {
        super(bus, objectDef);
        mBushandler = handler;
    }

    /**
     * Override AbstractDynamicBusObject.signalReceiver to provide a generic callback handler that
     * will receive incoming Door interface bus signals (namely the PersonPassedThrough signal).
     */
    @Override
    public void signalReceived(Object... args) throws BusException {
        MessageContext msgCtx = getBus().getMessageContext();
        String argString = Arrays.toString(args).replaceAll("^.|.$", "");

        String message = String.format("%s(%s) event received from %s", msgCtx.memberName, argString, msgCtx.sender);
        mBushandler.sendUIMessageOnSignal(message);
    }

}
