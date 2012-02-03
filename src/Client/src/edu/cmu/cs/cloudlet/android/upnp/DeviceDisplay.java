//
// Copyright (C) 2011-2012 Carnegie Mellon University
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of version 2 of the GNU General Public License as published
// by the Free Software Foundation.  A copy of the GNU General Public License
// should have been distributed along with this program in the file
// LICENSE.GPL.

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
package edu.cmu.cs.cloudlet.android.upnp;

import java.net.URL;

import org.teleal.cling.model.meta.Device;
import org.teleal.cling.model.meta.RemoteDevice;

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
    	return address.getHost();
    }

	public int getPort() {
    	URL address = ((RemoteDevice) device).getIdentity().getDescriptorURL();    	
    	return address.getPort();
	}
}
