package edu.cmu.cs.cloudlet.android;

import java.io.File;

import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.network.ServiceDiscovery;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
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
        
//		connector.startConnection("128.2.212.207", 9090);
	}

	/*
	 * Service Discovery Handler
	 */
	Handler discoveryHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == ServiceDiscovery.SERVICE_FOUND) {
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
			Intent intent = new Intent(CloudletActivity.this, CloudletCameraActivity.class);
			intent.putExtra("address", "desk.krha.kr");
		    startActivityForResult(intent, 0);
		}
	};

	private void DialogSelectOption() {
		final String items[] = { "item1", "item2", "item3" };
		AlertDialog.Builder ab = new AlertDialog.Builder(this);
		ab.setTitle("Title");
		ab.setIcon(R.drawable.ic_launcher);
		ab.setSingleChoiceItems(items, 0, new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
			}
		}).setPositiveButton("Ok", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
			}
		});
		ab.show();
	}	

	/*
	 * Callback function after VM Synthesis
	 */
	public static DialogInterface.OnClickListener launchApplication = new DialogInterface.OnClickListener() {
		@Override
		public void onClick(DialogInterface dialog, int which) {
			// TODO Auto-generated method stub
			
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
		connector.close();
		super.onDestroy();
	}
}