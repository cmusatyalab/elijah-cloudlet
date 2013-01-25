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
package edu.cmu.cs.cloudlet.android;
import java.util.ArrayList;

import org.teleal.cling.android.AndroidUpnpServiceImpl;

import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.application.face.batch.FaceAndroidBatchClientActivity;
import edu.cmu.cs.cloudlet.android.application.face.batch.FacePreferenceActivity;
import edu.cmu.cs.cloudlet.android.application.graphics.GraphicsClientActivity;
import edu.cmu.cs.cloudlet.android.application.speech.SpeechAndroidBatchClientActivity;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.upnp.DeviceDisplay;
import edu.cmu.cs.cloudlet.android.upnp.UPnPDiscovery;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;
import edu.cmu.cs.cloudlet.android.util.CloudletPreferenceActivity;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;

public class CloudletActivity extends Activity {
	public static String SYNTHESIS_SERVER_IP = "cloudlet.krha.kr"; // Cloudlet
	public static int SYNTHESIS_SERVER_PORT = 8021; // Cloudlet port for
													// VM Synthesis

	public static final String[] applications = { "MOPED", "GRAPHICS", "FACE", "Speech", "NULL" };
	public static final int TEST_CLOUDLET_APP_MOPED_PORT = 9092; // 19092
	public static final int TEST_CLOUDLET_APP_GRAPHICS_PORT = 9093;
	public static final int TEST_CLOUDLET_APP_FACE_PORT = 9876;
	private static final int TEST_CLOUDLET_APP_SPEECH_PORT = 10191;

	private static final int SYNTHESIS_MENU_ID_SETTINGS = 11123;
	private static final int SYNTHESIS_MENU_ID_CLEAR = 12311;

	protected Button startConnectionButton;
	protected CloudletConnector connector;
	private UPnPDiscovery serviceDiscovery;
	protected int selectedOveralyIndex;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		loadPreferneces();

		// Initiate Environment Settings
		CloudletEnv.instance();

		// upnp service binding and show dialog
		this.serviceDiscovery = new UPnPDiscovery(this, CloudletActivity.this, discoveryHandler);
		getApplicationContext().bindService(new Intent(this, AndroidUpnpServiceImpl.class),
				this.serviceDiscovery.serviceConnection, Context.BIND_AUTO_CREATE);
		serviceDiscovery.showDialogSelectOption();

		// Performance Button
		findViewById(R.id.testSynthesis).setOnClickListener(clickListener);
	}

	private void showDialogSelectOverlay(final ArrayList<VMInfo> vmList) {
		String[] nameList = new String[vmList.size()];
		for (int i = 0; i < nameList.length; i++) {
			nameList[i] = new String(vmList.get(i).getAppName());
		}

		AlertDialog.Builder ab = new AlertDialog.Builder(this);
		ab.setTitle("Overlay List");
		ab.setIcon(R.drawable.ic_launcher);
		ab.setSingleChoiceItems(nameList, 0, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				selectedOveralyIndex = position;
			}
		}).setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				if (position >= 0) {
					selectedOveralyIndex = position;
				}
				VMInfo overlayVM = vmList.get(selectedOveralyIndex);
				runConnection(SYNTHESIS_SERVER_IP, SYNTHESIS_SERVER_PORT, overlayVM);
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				return;
			}
		});
		ab.show();
	}

	/*
	 * Synthesis initiation through HTTP Post
	 */
	protected void runConnection(String address, int port, VMInfo overlayVM) {
		if (this.connector != null) {
			this.connector.close();
		}
		this.connector = new CloudletConnector(this, CloudletActivity.this);
		this.connector.startConnection(address, port, overlayVM);
	}


	/*
	 * Launch Application as a Standalone
	 */
	private void showDialogSelectApp(final String[] applications) {
		// Show Dialog
		AlertDialog.Builder ab = new AlertDialog.Builder(this);
		ab.setTitle("Application List");
		ab.setIcon(R.drawable.ic_launcher);
		ab.setSingleChoiceItems(applications, 0, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				selectedOveralyIndex = position;
			}
		}).setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				if (position >= 0) {
					selectedOveralyIndex = position;
				} else if (applications.length > 0 && selectedOveralyIndex == -1) {
					selectedOveralyIndex = 0;
				}
				String application = applications[selectedOveralyIndex];
				runStandAlone(application);
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				return;
			}
		});
		ab.show();
	}

	public void runStandAlone(String application) {
		application = application.trim();

		if (application.equalsIgnoreCase("moped") || application.equalsIgnoreCase("moped_disk")) {
			Intent intent = new Intent(CloudletActivity.this, CloudletCameraActivity.class);
			intent.putExtra("address", SYNTHESIS_SERVER_IP);
			intent.putExtra("port", TEST_CLOUDLET_APP_MOPED_PORT);
			startActivityForResult(intent, 0);
		} else if (application.equalsIgnoreCase("graphics")) {
			Intent intent = new Intent(CloudletActivity.this, GraphicsClientActivity.class);
			intent.putExtra("address", SYNTHESIS_SERVER_IP);
			intent.putExtra("port", TEST_CLOUDLET_APP_GRAPHICS_PORT);
			startActivityForResult(intent, 0);
		} else if (application.equalsIgnoreCase("face")) {
			Intent intent = new Intent(CloudletActivity.this, FaceAndroidBatchClientActivity.class);
			intent.putExtra("address", SYNTHESIS_SERVER_IP);
			intent.putExtra("port", TEST_CLOUDLET_APP_FACE_PORT);
			startActivityForResult(intent, 0);
		} else if (application.equalsIgnoreCase("speech")) {
			Intent intent = new Intent(CloudletActivity.this, SpeechAndroidBatchClientActivity.class);
			intent.putExtra("address", SYNTHESIS_SERVER_IP);
			intent.putExtra("port", TEST_CLOUDLET_APP_SPEECH_PORT);
			startActivityForResult(intent, 0);
		} else if (application.equalsIgnoreCase("null")) {
			showAlert("Info", "NUll has no UI");
		} else {
			showAlert("Error", "NO such Application : " + application);
		}
	}

	View.OnClickListener clickListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			// show
			// serviceDiscovery.showDialogSelectOption();

			// This buttons are for MobiSys test
			if (v.getId() == R.id.testSynthesis) {
				// Find All overlay and let user select one of them.
				CloudletEnv.instance().resetOverlayList();
				ArrayList<VMInfo> vmList = CloudletEnv.instance().getOverlayDirectoryInfo();
				if (vmList.size() > 0) {
					showDialogSelectOverlay(vmList);
				} else {
					showAlert("Error", "We found No Overlay");
				}
			}
		}
	};

	public void showAlert(String type, String message) {
		new AlertDialog.Builder(CloudletActivity.this).setTitle(type).setMessage(message)
				.setIcon(R.drawable.ic_launcher).setNegativeButton("Confirm", null).show();
	}

	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		super.onActivityResult(requestCode, resultCode, data);
		if (resultCode == RESULT_OK) {
			if (requestCode == 0) {
				String ret = data.getExtras().getString("message");
			}
		}

		// send close VM message
		this.connector.closeRequest();
	}

	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			new AlertDialog.Builder(CloudletActivity.this).setTitle("Exit").setMessage("Finish Application")
					.setPositiveButton("Confirm", new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int which) {
							moveTaskToBack(true);
							finish();
						}
					}).setNegativeButton("Cancel", null).show();
		}
		return super.onKeyDown(keyCode, event);
	}

	public void loadPreferneces() {
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		CloudletActivity.SYNTHESIS_SERVER_IP = prefs.getString(getString(R.string.synthesis_pref_address),
				getString(R.string.synthesis_default_ip_address));
		CloudletActivity.SYNTHESIS_SERVER_PORT = Integer.parseInt(prefs.getString(getString(R.string.synthesis_pref_port),
				getString(R.string.synthesis_default_port)));
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.add(0, SYNTHESIS_MENU_ID_SETTINGS, 0, getString(R.string.synthesis_config_memu_setting));
		menu.add(0, SYNTHESIS_MENU_ID_CLEAR, 1, getString(R.string.synthesis_config_memu_clear));
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	protected void onResume() {
		loadPreferneces();
		super.onResume();
	}

	@Override
	public void onDestroy() {
		getApplicationContext().unbindService(this.serviceDiscovery.serviceConnection);
		if (this.serviceDiscovery != null)
			this.serviceDiscovery.close();

		this.connector.close();
		super.onDestroy();
	}

	/*
	 * Service Discovery Handler
	 */
	Handler discoveryHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == UPnPDiscovery.DEVICE_SELECTED) {
				DeviceDisplay device = (DeviceDisplay) msg.obj;
				String ipAddress = device.getIPAddress();
				int port = device.getPort(); // port number of upnp server
				SYNTHESIS_SERVER_IP = ipAddress;
			} else if (msg.what == UPnPDiscovery.USER_CANCELED) {
				showAlert("Info", "Select UPnP Server for Cloudlet Service");
			}
		}
	};
}
