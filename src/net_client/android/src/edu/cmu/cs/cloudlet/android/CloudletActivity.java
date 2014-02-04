/*
 *
 * Cloudlet Infrastructure for Mobile Computing
 *
 *   Author: Kiryong Ha <krha@cmu.edu>
 *
 *   Copyright (C) 2011-2013 Carnegie Mellon University
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

package edu.cmu.cs.cloudlet.android;

import java.io.File;
import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.HashMap;

import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.application.graphics.GraphicsClientActivity;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.discovery.CloudletDiscovery;
import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.network.OpenStackClient;
import edu.cmu.cs.cloudlet.android.discovery.CloudletDevice;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.text.InputType;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

public class CloudletActivity extends Activity {
	public static String CLOUDLET_SYNTHESIS_IP = "cloudlet.krha.kr";
	public static int CLOUDLET_SYNTHESIS_PORT = 8021;

	private static final int SYNTHESIS_MENU_ID_SETTINGS = 11123;
	private static final int SYNTHESIS_MENU_ID_CLEAR = 12311;

	protected Button startConnectionButton;
	protected CloudletConnector connector;
	protected int selectedOveralyIndex;
	private CloudletDiscovery cloudletDiscovery;
	private ProgressDialog progDialog;
	
	//openstack variable
	protected OpenStackClient openStackClient;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		// Initiate Environment Settings
		CloudletEnv.instance();

		// Cloudlet discovery
		this.cloudletDiscovery = new CloudletDiscovery(this, CloudletActivity.this, discoveryHandler);

		// Performance Button
		findViewById(R.id.testSynthesis).setOnClickListener(clickListener);
		findViewById(R.id.synthesisFromOpenStack).setOnClickListener(clickListener);
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
				runConnection(CLOUDLET_SYNTHESIS_IP, CLOUDLET_SYNTHESIS_PORT, overlayVM);
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				return;
			}
		});
		ab.show();
	}

	/*
	 * Synthesis start
	 */
	protected void runConnection(String address, int port, VMInfo overlayVM) {
		if (this.connector != null) {
			this.connector.close();
		}
		this.connector = new CloudletConnector(CloudletActivity.this, synthesisHandler);
		this.connector.setConnection(address, port, overlayVM);
		this.connector.start();

		if (this.progDialog == null) {
			this.progDialog = new ProgressDialog(this);
			this.progDialog.setMessage("Connecting to " + address);
			this.progDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
			this.progDialog.setCancelable(true);
			this.progDialog.setIcon(R.drawable.ic_launcher);
		} else {
			this.progDialog.setMessage("Connecting to " + address);
		}
		this.progDialog.show();
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

	public boolean runStandAlone(String application) {
		application = application.trim();
		return runStandAloneIP(application, CLOUDLET_SYNTHESIS_IP);
	}
	
	public boolean runStandAloneIP(String application, String ip){

		if (application.equalsIgnoreCase("moped") || application.equalsIgnoreCase("object")) {
			Intent intent = new Intent(CloudletActivity.this, CloudletCameraActivity.class);
			intent.putExtra("address", ip);
			intent.putExtra("port", TEST_CLOUDLET_APP_MOPED_PORT);
			startActivityForResult(intent, 0);
			return true;
		} else if (application.equalsIgnoreCase("graphics") || application.equalsIgnoreCase("fluid")) {
			Intent intent = new Intent(CloudletActivity.this, GraphicsClientActivity.class);
			intent.putExtra("address", ip);
			intent.putExtra("port", TEST_CLOUDLET_APP_GRAPHICS_PORT);
			startActivityForResult(intent, 0);
			return true;
		} else {
			return false;
		}
	}

	View.OnClickListener clickListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			if (v.getId() == R.id.testSynthesis) {
				// Find All overlay and let user select one of them.
				CloudletEnv.instance().resetOverlayList();
				ArrayList<VMInfo> vmList = CloudletEnv.instance().getOverlayDirectoryInfo();
				if (vmList.size() > 0) {
					showDialogSelectOverlay(vmList);
				} else {
					File ovelray_root = CloudletEnv.instance().getFilePath(CloudletEnv.OVERLAY_DIR);
					String errMsg = "No Overlay exist\nCreate overlay directory under \""
							+ ovelray_root.getAbsolutePath() + "\"";
					showAlert("Error", errMsg);
				}
			} else if (v.getId() == R.id.synthesisFromOpenStack) {
				String OPENSTACK_RELAY_IP = "cloudlet.krha.kr";
				int OPENSTACK_RELAY_PORT = 8081;
				String applicationName = "moped";
				String overly_url = "http://storage.findcloudlet.org/media/qlw7l9nf13/overlay-moped.zip";

				HashMap<String, String> requestMap = new HashMap<String, String>();
				requestMap.put("overlay_url", overly_url);
				requestMap.put("application_name", applicationName);
				JSONObject requestJson = new JSONObject(requestMap);

				// network request
				if (openStackClient == null){
					openStackClient = new OpenStackClient(CloudletActivity.this, openStackClientHandler,
							OPENSTACK_RELAY_IP, OPENSTACK_RELAY_PORT);
					openStackClient.start();					
				}
				openStackClient.queueRequest(requestJson);

				// start progress Dialog
				if (progDialog == null) {
					progDialog = new ProgressDialog(CloudletActivity.this);
					progDialog.setMessage("Request VM Synthesis to " + OPENSTACK_RELAY_IP + ":" + OPENSTACK_RELAY_PORT);
					progDialog.setCancelable(true);
					progDialog.setIcon(R.drawable.ic_launcher);
				} else {
					progDialog.setMessage("Request VM Synthesis to " + OPENSTACK_RELAY_IP + ":" + OPENSTACK_RELAY_PORT);
				}
				progDialog.show();
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
		if (this.connector != null){
			this.connector.closeRequest();
		}
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

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		menu.add(0, SYNTHESIS_MENU_ID_SETTINGS, 0, getString(R.string.synthesis_config_memu_setting));
		menu.add(0, SYNTHESIS_MENU_ID_CLEAR, 1, getString(R.string.synthesis_config_memu_clear));
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	protected void onResume() {
		super.onResume();
	}

	@Override
	public void onDestroy() {
		if (this.cloudletDiscovery != null) {
			this.cloudletDiscovery.close();
		}
		if (this.connector != null)
			this.connector.close();
		if (this.openStackClient != null)
			this.openStackClient.close();

		super.onDestroy();
	}

	/*
	 * Service Discovery Handler
	 */
	Handler discoveryHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == CloudletDiscovery.DEVICE_SELECTED) {
				CloudletDevice device = (CloudletDevice) msg.obj;
				String ipAddress = device.getIPAddress();
				CLOUDLET_SYNTHESIS_IP = ipAddress;
			} else if (msg.what == CloudletDiscovery.USER_CANCELED) {
				// Ask IP Address
				AlertDialog.Builder ab = new AlertDialog.Builder(CloudletActivity.this);
				final EditText input = new EditText(getApplicationContext());
				input.setText(CLOUDLET_SYNTHESIS_IP);
				input.setInputType(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
				ab.setTitle("Cloudlet Conection");
				ab.setMessage("Enter the IP address of Cloudlet since no result from discovery.\n(Specify default address at CLOUDLET_SYNTHESIS_IP in CloudletAcitivity.java)");
				ab.setView(input);
				ab.setIcon(R.drawable.ic_launcher);
				ab.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int whichButton) {
						String value = input.getText().toString();
						CLOUDLET_SYNTHESIS_IP = value;
					}
				});
				ab.show();
			}
		}
	};

	/*
	 * Synthesis callback handler
	 */
	Handler synthesisHandler = new Handler() {
		private int PROGRESS_NONE = -1;
		private int PROGRESS_FINISH = 100;

		protected void updateMessage(String msg) {
			if ((progDialog != null) && (progDialog.isShowing())) {
				progDialog.setMessage(msg);
			}
		}

		protected void updatePercent(int percent) {
			if ((progDialog != null) && (progDialog.isShowing())) {
				progDialog.setProgress(percent);
			}
		}

		public void handleMessage(Message msg) {
			if (msg.what == CloudletConnector.SYNTHESIS_SUCCESS) {
				String appName = (String) msg.obj;
				this.updateMessage("Synthesis SUCESS");
				this.updatePercent(PROGRESS_FINISH);
				boolean isSuccess = runStandAlone(appName);
				if (isSuccess == false) {
					// If we cannot find matching application,
					// then ask user to close VM
					String message = "VM synthesis is successfully finished, but cannot find Android application matched with "
							+ "directory name ("
							+ appName
							+ ").\nPlease read README file for details.\n\nClose VM at Cloudlet?";
					new AlertDialog.Builder(CloudletActivity.this).setTitle("Info").setMessage(message)
							.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog, int which) {
									connector.closeRequest();
								}
							}).setNegativeButton("No", null).show();
				}
				progDialog.dismiss();
			} else if (msg.what == CloudletConnector.SYNTHESIS_FAILED) {
				String reason = (String) msg.obj;
				if (reason != null)
					this.updateMessage(reason);
				else
					this.updateMessage("Synthesis FAILED");
				progDialog.dismiss();

				AlertDialog.Builder ab = new AlertDialog.Builder(CloudletActivity.this);
				ab.setTitle("VM Synthesis Failed");
				ab.setIcon(R.drawable.ic_launcher);
				ab.setPositiveButton("Ok", null).setNegativeButton("Cancel", null);
				ab.show();
			} else if (msg.what == CloudletConnector.SYNTHESIS_PROGRESS_MESSAGE) {
				String message = (String) msg.obj;
				this.updateMessage(message);
			} else if (msg.what == CloudletConnector.SYNTHESIS_PROGRESS_PERCENT) {
				int percent = (Integer) msg.obj;
				this.updatePercent(percent);
			}
		}
	};

	/*
	 * OpenStack communication handler (for testing purpose)
	 */
	Handler openStackClientHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == OpenStackClient.CALLBACK_SUCCESS) {
				JSONObject recvJson = (JSONObject) msg.obj;
				String serverID = null;
				String serverIP = null;
				String appName = null;
				try {
					serverID = recvJson.getString("vm-id");
					serverIP = recvJson.getString("vm-ip");
					appName = recvJson.getString("application_name");
					if ((appName == null) || (appName.trim().length() == 0)){
						throw new JSONException("no application name is received");
					}					
				} catch (JSONException e) {
					String errmsg = "Server does not return valid json format";
					new AlertDialog.Builder(CloudletActivity.this).setTitle("Error").setMessage(errmsg)
							.setIcon(R.drawable.ic_launcher).setNegativeButton("Confirm", null).show();
				}

				if ((progDialog != null) && (progDialog.isShowing())) {
					progDialog.dismiss();
				}
				runStandAloneIP(appName, serverIP);
			} else {
				String errString = (String) msg.obj;
				// Ask IP Address
				AlertDialog.Builder ab = new AlertDialog.Builder(CloudletActivity.this);
				ab.setTitle("Cloudlet Conection");
				ab.setMessage("Failed to synthesize new VM.\n" + errString);
				ab.setIcon(R.drawable.ic_launcher);
				ab.setPositiveButton("Ok", null);
				ab.show();

				if ((progDialog != null) && (progDialog.isShowing())) {
					progDialog.dismiss();
				}
			}
		}
	};

	// TO BE DELETED (only for test purpose)
	public static final String[] applications = { "MOPED", "GRAPHICS" };
	public static final int TEST_CLOUDLET_APP_MOPED_PORT = 9092; // 19092
	public static final int TEST_CLOUDLET_APP_GRAPHICS_PORT = 9093;
}
