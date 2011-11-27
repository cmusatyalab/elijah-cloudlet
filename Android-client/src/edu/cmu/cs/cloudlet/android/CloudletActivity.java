package edu.cmu.cs.cloudlet.android;


import org.teleal.cling.android.AndroidUpnpServiceImpl;

import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.upnp.DeviceDisplay;
import edu.cmu.cs.cloudlet.android.upnp.UPnPDiscovery;
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
	/** Called when the activity is first created. */
	protected Button startConnectionButton;
	protected CloudletConnector connector;
	private UPnPDiscovery serviceDiscovery;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.cloudlet);

		// upnp service binding
		this.serviceDiscovery = new UPnPDiscovery(this, CloudletActivity.this, discoveryHandler);
		this.connector = new CloudletConnector(this, CloudletActivity.this);
		getApplicationContext().bindService(new Intent(this, AndroidUpnpServiceImpl.class), this.serviceDiscovery.serviceConnection, Context.BIND_AUTO_CREATE);

		// show upnp discovery dialog 
		serviceDiscovery.showDialogSelectOption();
		
		// set button action
        Button mSendButton = (Button) findViewById(R.id.connectButton);
        mSendButton.setOnClickListener(clickListener);

		// Launching Application
		Intent intent = new Intent(CloudletActivity.this, CloudletCameraActivity.class);
		intent.putExtra("address", "desk.krha.kr");
		CloudletActivity.this.startActivityForResult(intent, 0);
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
			
				// Launching Application
				Intent intent = new Intent(CloudletActivity.this, CloudletCameraActivity.class);
				intent.putExtra("address", "desk.krha.kr");
				CloudletActivity.this.startActivityForResult(intent, 0);
				
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
			serviceDiscovery.showDialogSelectOption();
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