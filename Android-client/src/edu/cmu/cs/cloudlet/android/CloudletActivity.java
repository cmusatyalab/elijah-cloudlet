package edu.cmu.cs.cloudlet.android;

import java.io.File;

import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.network.ServiceDiscovery;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
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
	private ServiceDiscovery serviceDiscovery;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);

		this.serviceDiscovery = new ServiceDiscovery();
		this.serviceDiscovery.setHandler(discoveryHandler);
		this.connector = new CloudletConnector(CloudletActivity.this);
		
		// set button action
		Button mSendButton = (Button) findViewById(R.id.connectButton);
		mSendButton.setOnClickListener(clickListener);
	}

	/*
	 * Service Discovery Handler
	 */
	Handler discoveryHandler = new Handler() {
		public void handleMessage(Message msg) {
			if(msg.what == ServiceDiscovery.SERVICE_FOUND){
				Bundle data = msg.getData();
				String ipAddress = data.getString(ServiceDiscovery.KEY_SERVICE_IP_ADDRESS);
				int port = data.getInt(ServiceDiscovery.KEY_SERVICE_PORT);
				connector.startConnection(ipAddress, port);
			}
		}
	};

	View.OnClickListener clickListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			connector.startConnection("desk.krha.kr", -1);
		}
	};


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
		super.onDestroy();
	}
}