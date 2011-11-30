package edu.cmu.cs.cloudlet.android;


import java.security.acl.LastOwnerException;
import java.util.ArrayList;

import org.teleal.cling.android.AndroidUpnpServiceImpl;

import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.application.face.ui.FaceRecClientCameraPreview;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.upnp.DeviceDisplay;
import edu.cmu.cs.cloudlet.android.upnp.UPnPDiscovery;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;

public class CloudletActivity extends Activity {
	public static final String TEST_CLOUDLET_SERVER_IP = "cage.coda.cs.cmu.edu";
	public static final int TEST_CLOUDLET_SERVER_PORT = 9090;
	public static final int TEST_CLOUDLET_FACE_PORT = 9876;
	protected Button startConnectionButton;
	protected CloudletConnector connector;
	private UPnPDiscovery serviceDiscovery;
	protected int selectedOveralyIndex;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		// upnp service binding
		this.serviceDiscovery = new UPnPDiscovery(this, CloudletActivity.this, discoveryHandler);
		this.connector = new CloudletConnector(this, CloudletActivity.this);
		getApplicationContext().bindService(new Intent(this, AndroidUpnpServiceImpl.class), this.serviceDiscovery.serviceConnection, Context.BIND_AUTO_CREATE);		

		// show upnp discovery dialog 
//		serviceDiscovery.showDialogSelectOption();

		// Connect Directly
		findViewById(R.id.testSynthesis).setOnClickListener(clickListener);
		findViewById(R.id.runMOPEDApp).setOnClickListener(clickListener);
		findViewById(R.id.runFACEApp).setOnClickListener(clickListener);
        
	}

	private void upadteVMList() {
		ArrayList<VMInfo> vmList = CloudletEnv.instance().getOverlayDirectoryInfo();
		VMInfo selectedVM = null;
		if(selectedOveralyIndex >= 0){
			selectedVM = vmList.get(selectedOveralyIndex);
			vmList.clear();
			vmList.add(selectedVM);			
		}

		connector.startConnection(TEST_CLOUDLET_SERVER_IP, TEST_CLOUDLET_SERVER_PORT);
	}
	
	private void showDialogSelectOverlay(ArrayList<VMInfo> vmList) {
		String[] nameList = new String[vmList.size()];
		for(int i = 0; i < nameList.length; i++){
			nameList[i] = new String(vmList.get(i).getInfo(VMInfo.JSON_KEY_NAME));			
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
				if(position >= 0){
					selectedOveralyIndex = position;
				}
				
				upadteVMList();
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				return;
			}
		});
		ab.show();
	}

	/*
	 * Service Discovery Handler
	 */
	Handler discoveryHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == UPnPDiscovery.DEVICE_SELECTED) {
				DeviceDisplay device = (DeviceDisplay) msg.obj;
				String ipAddress = device.getIPAddress();
				int port = device.getPort();
				KLog.println("ip : " + ipAddress + ", port : " + port);
				
//				connector.startConnection(ipAddress, 9090);
//				connector.startConnection("128.2.212.207", 9090);
				
			}else if(msg.what == UPnPDiscovery.USER_CANCELED){
				new AlertDialog.Builder(CloudletActivity.this).setTitle("Info")
				.setMessage("Select UPnP Server for Cloudlet Service")
				.setIcon(R.drawable.ic_launcher)
				.setNegativeButton("Confirm", null)
				.show();
			}
		}
	};

	View.OnClickListener clickListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
//			serviceDiscovery.showDialogSelectOption();			
			if(v.getId() == R.id.testSynthesis){
				// Find All overlay and let user select one of them.
				CloudletEnv.instance().resetOverlayList();
				ArrayList<VMInfo> vmList = CloudletEnv.instance().getOverlayDirectoryInfo();
				if(vmList.size() > 0){
					showDialogSelectOverlay(vmList);			
				}else{
					new AlertDialog.Builder(CloudletActivity.this).setTitle("Error")
					.setIcon(R.drawable.ic_launcher)
					.setMessage("We found No Overlay")
					.setNegativeButton("Confirm", null)
					.show();
				}				
			}else if(v.getId() == R.id.runMOPEDApp){
				Intent intent = new Intent(CloudletActivity.this, CloudletCameraActivity.class);
				intent.putExtra("address", TEST_CLOUDLET_SERVER_IP);
				startActivityForResult(intent, 0);				
			}else if(v.getId() == R.id.runFACEApp){
				Intent intent = new Intent(CloudletActivity.this, FaceRecClientCameraPreview.class);
				intent.putExtra("address", TEST_CLOUDLET_SERVER_IP);
				intent.putExtra("port", TEST_CLOUDLET_FACE_PORT);
				startActivityForResult(intent, 0);
			}
		}
	};


	@Override 
    protected void onActivityResult(int requestCode, int resultCode, Intent data) { 
        super.onActivityResult(requestCode, resultCode, data); 
        if (resultCode == RESULT_OK) { 
            if (requestCode == 0) {  
                String ret = data.getExtras().getString("message"); 
            } 
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
	public void onDestroy() {		
		getApplicationContext().unbindService(this.serviceDiscovery.serviceConnection);
		this.serviceDiscovery.close();
		this.connector.close();
		super.onDestroy();
	}
}