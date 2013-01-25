package edu.cmu.cs.cloudlet.android.upnp;

import java.net.URL;

import org.teleal.cling.model.meta.Device;
import org.teleal.cling.model.meta.RemoteDevice;

import edu.cmu.cs.cloudlet.android.util.KLog;

public class DeviceDisplay {
    Device device;

    public DeviceDisplay(Device device) {
        this.device = device;
    }

    public Device getDevice() {
        return device;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        DeviceDisplay that = (DeviceDisplay) o;
        return device.equals(that.device);
    }

    @Override
    public int hashCode() {
        return device.hashCode();
    }

    @Override
    public String toString() {
        // Display a little star while the device is being loaded
        return device.isFullyHydrated() ? device.getDisplayString() : device.getDisplayString() + " *";
//    	URL address = ((RemoteDevice) device).getIdentity().getDescriptorURL();
//    	return address.toString();
    }
    
    public String getIPAddress(){
    	URL address = ((RemoteDevice) device).getIdentity().getDescriptorURL();  
    	KLog.println(device.toString());
    	return address.getHost();
    }

	public int getPort() {
    	URL address = ((RemoteDevice) device).getIdentity().getDescriptorURL();    	
    	return address.getPort();
	}
}