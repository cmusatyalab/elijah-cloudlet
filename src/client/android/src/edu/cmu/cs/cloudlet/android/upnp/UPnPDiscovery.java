package edu.cmu.cs.cloudlet.android.upnp;

import java.security.acl.LastOwnerException;
import java.util.Iterator;
import java.util.Vector;

import org.teleal.cling.android.AndroidUpnpService;
import org.teleal.cling.android.AndroidUpnpServiceImpl;
import org.teleal.cling.registry.DefaultRegistryListener;
import org.teleal.cling.registry.Registry;
import org.teleal.cling.registry.RegistryListener;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.SimpleCursorAdapter;
import android.widget.Toast;

import org.teleal.cling.model.meta.Device;
import org.teleal.cling.model.meta.LocalDevice;
import org.teleal.cling.model.meta.RemoteDevice;

import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.network.CloudletConnector;


public class UPnPDiscovery{
	public static final int DEVICE_SELECTED = 1;
	public static final int USER_CANCELED = 2;
	
	private ArrayAdapter<DeviceDisplay> listAdapter;
	private AndroidUpnpService upnpService;
	private int lastSelectedIndex = -1;
	
	private RegistryListener registryListener = new BrowseRegistryListener();
	private Activity activity;
	private Context context;
	private Handler handler;

	public UPnPDiscovery(Activity activity, Context context, Handler handler){
		this.activity = activity;
		this.handler = handler;
		this.context = context;
		listAdapter = new ArrayAdapter(this.context, R.layout.dialog_list_item);
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
			listAdapter.clear();
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
	
	public void showDialogSelectOption() {		
		AlertDialog.Builder ab = new AlertDialog.Builder(this.context);
		ab.setTitle("Cloudlet Discovery");
		ab.setIcon(R.drawable.ic_launcher);
		ab.setSingleChoiceItems(listAdapter, 0, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				lastSelectedIndex = position;
			}
		}).setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				Message msg = Message.obtain();
				msg.what = UPnPDiscovery.DEVICE_SELECTED;
				if(position >= 0){
					msg.obj = listAdapter.getItem(position);					
				}else if(lastSelectedIndex != -1){
					msg.obj = listAdapter.getItem(lastSelectedIndex);
				}else if(listAdapter.getCount() > 0){
					msg.obj = listAdapter.getItem(0);
				}
				handler.sendMessage(msg);
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				Message msg = Message.obtain();
				msg.what = UPnPDiscovery.USER_CANCELED;
				handler.sendMessage(msg);
			}
		});
		ab.show();
	}
	

	class BrowseRegistryListener extends DefaultRegistryListener {
		@Override
		public void remoteDeviceDiscoveryStarted(Registry registry, RemoteDevice device) {
			deviceAdded(device);
		}

		@Override
		public void remoteDeviceDiscoveryFailed(Registry registry, final RemoteDevice device, final Exception ex) {
			activity.runOnUiThread(new Runnable() {
				public void run() {
					Toast.makeText(
							context,
							"Discovery failed of '" + device.getDisplayString() + "': "
									+ (ex != null ? ex.toString() : "Couldn't retrieve device/service descriptors"), Toast.LENGTH_LONG).show();
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
					DeviceDisplay d = new DeviceDisplay(device);
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
					listAdapter.remove(new DeviceDisplay(device));
				}
			});
		}
	}

}
