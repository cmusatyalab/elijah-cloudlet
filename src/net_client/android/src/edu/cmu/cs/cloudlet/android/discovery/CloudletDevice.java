package edu.cmu.cs.cloudlet.android.discovery;

import java.net.URL;

import org.json.JSONException;
import org.json.JSONObject;
import org.teleal.cling.model.meta.Device;
import org.teleal.cling.model.meta.RemoteDevice;

import edu.cmu.cs.cloudlet.android.util.KLog;

public class CloudletDevice {

	private String ipAddress;
	private boolean isUPnP;

	public CloudletDevice(Device device) {
		URL address = ((RemoteDevice) device).getIdentity().getDescriptorURL();
		this.ipAddress = address.getHost();
		this.isUPnP = true;
	}
	
	public CloudletDevice(JSONObject device){
		this.isUPnP = false;

		try {
			this.ipAddress = device.getString("ip_address");
			double latitude = device.getDouble("latitude");
			double longitude = device.getDouble("longitude");
		} catch (JSONException e) {
			e.printStackTrace();
		}
	}

	@Override
	public boolean equals(Object o) {
		if (this == o)
			return true;
		if (o == null || getClass() != o.getClass())
			return false;
		
		CloudletDevice that = (CloudletDevice) o;
		if (this.ipAddress == that.ipAddress)
			return true;
		else
			return false;
	}


	@Override
	public String toString() {
		
		return "Cloudlet at " + this.ipAddress; 
		
		// For UPnP
		// return device.isFullyHydrated() ? device.getDisplayString() : device.getDisplayString() + " *";
	}

	public String getIPAddress() {
		return this.ipAddress;
	}

}