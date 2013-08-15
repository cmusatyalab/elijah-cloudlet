package edu.cmu.cs.cloudlet.android.discovery;

import org.teleal.cling.android.AndroidUpnpService;
import org.teleal.cling.registry.DefaultRegistryListener;
import org.teleal.cling.registry.Registry;
import org.teleal.cling.registry.RegistryListener;

import android.app.Activity;
import android.content.ComponentName;
import android.content.ServiceConnection;
import android.os.IBinder;
import android.widget.ArrayAdapter;

import org.teleal.cling.model.meta.Device;
import org.teleal.cling.model.meta.LocalDevice;
import org.teleal.cling.model.meta.RemoteDevice;

import edu.cmu.cs.cloudlet.android.util.KLog;

public class UPnPDiscovery {
	private AndroidUpnpService upnpService;

	private ArrayAdapter<CloudletDevice> listAdapter = null;
	private RegistryListener registryListener = new BrowseRegistryListener();
	private Activity activity = null;

	public UPnPDiscovery(Activity activity, ArrayAdapter<CloudletDevice> listAdapter) {
		this.activity = activity;
		this.listAdapter = listAdapter;
	}

	public void close() {
		if (upnpService != null) {
			upnpService.getRegistry().removeListener(registryListener);
		}
	}

	public ServiceConnection serviceConnection = new ServiceConnection() {

		public void onServiceConnected(ComponentName className, IBinder service) {
			upnpService = (AndroidUpnpService) service;

			// Refresh the list with all known devices
//			listAdapter.clear();
			for (Device device : upnpService.getRegistry().getDevices()) {
				((BrowseRegistryListener) registryListener).deviceAdded(device);
			}

			// Getting ready for future device advertisements
			upnpService.getRegistry().addListener(registryListener);

			// Search asynchronously for all devices
			upnpService.getControlPoint().search();
		}

		public void onServiceDisconnected(ComponentName className) {
			upnpService = null;
		}
	};

	class BrowseRegistryListener extends DefaultRegistryListener {
		@Override
		public void remoteDeviceDiscoveryStarted(Registry registry, RemoteDevice device) {
			deviceAdded(device);
		}

		@Override
		public void remoteDeviceDiscoveryFailed(Registry registry, final RemoteDevice device, final Exception ex) {
			activity.runOnUiThread(new Runnable() {
				public void run() {
					KLog.println("UPNP Discovery failed of '" + device.getDisplayString() + "': "
							+ (ex != null ? ex.toString() : "Couldn't retrieve device/service descriptors"));
					/*
					Toast.makeText(
							context,
							"Discovery failed of '" + device.getDisplayString() + "': "
									+ (ex != null ? ex.toString() : "Couldn't retrieve device/service descriptors"),
							Toast.LENGTH_LONG).show();
					*/
				}
			});
			deviceRemoved(device);
		}

		@Override
		public void remoteDeviceAdded(Registry registry, RemoteDevice device) {
			deviceAdded(device);
		}

		@Override
		public void remoteDeviceRemoved(Registry registry, RemoteDevice device) {
			deviceRemoved(device);
		}

		@Override
		public void localDeviceAdded(Registry registry, LocalDevice device) {
			deviceAdded(device);
		}

		@Override
		public void localDeviceRemoved(Registry registry, LocalDevice device) {
			deviceRemoved(device);
		}

		public void deviceAdded(final Device device) {
			activity.runOnUiThread(new Runnable() {
				public void run() {
					// Check cloudlet service
					// TODO: Search only Cloudlet service from the start
					String deviceString = device.getDisplayString().toLowerCase();
					if (deviceString.contains("cloudlet") == false){
						KLog.println("Skip found UPnP device since it's not Cloudlet\n");
						return;
					}
					
					CloudletDevice d = new CloudletDevice(device);
					int position = listAdapter.getPosition(d);
					if (position >= 0) {
						// Device already in the list, re-set new value at same
						// position
						listAdapter.remove(d);
						listAdapter.insert(d, position);
					} else {
						listAdapter.add(d);
					}
				}
			});
		}

		public void deviceRemoved(final Device device) {
			activity.runOnUiThread(new Runnable() {
				public void run() {
					listAdapter.remove(new CloudletDevice(device));
				}
			});
		}
	}

}
