package edu.cmu.cs.cloudlet.android.test;

import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.application.graphics.GraphicsClientActivity;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.discovery.CloudletDiscovery;
import edu.cmu.cs.cloudlet.android.network.CloudletConnector;
import edu.cmu.cs.cloudlet.android.util.CloudletEnv;

public class BatchSynthesisTest extends Activity {

	private static String ADDRESS = "128.2.210.197";
	private static final int PORT = 8021;

	private ArrayList<VMInfo> overlayVMList;
	private CloudletConnector connector;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.test_main);

		// Initiate Environment Settings
		CloudletEnv.instance();
		findViewById(R.id.startBatchTest).setOnClickListener(clickListener);
	}

	private void startBatchSynthesis() {			
		for(VMInfo overlayVM : this.overlayVMList){
			this.connector = new CloudletConnector(this, synthesisHandler);
			this.connector.setConnection(this.ADDRESS, this.PORT, overlayVM);
		}

	}
	
	/*
	 * Synthesis callback handler
	 */
	Handler synthesisHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (msg.what == CloudletConnector.SYNTHESIS_SUCCESS) {
				String appName = (String)msg.obj;
				boolean isSuccess = runStandAlone(appName);
				if (isSuccess == false) {
					// If we cannot find matching application,
					// then ask user to close VM
					String message = "VM synthesis is successfully finished, but cannot find matching Android application named ("
							+ appName + ")\n\nClose VM at Cloudlet?";
					new AlertDialog.Builder(BatchSynthesisTest.this).setTitle("Info").setMessage(message)
							.setPositiveButton("Yes", new DialogInterface.OnClickListener() {
								public void onClick(DialogInterface dialog, int which) {
									connector.closeRequest();
								}
							}).setNegativeButton("No", null).show();
				}
			} else if (msg.what == CloudletConnector.SYNTHESIS_FAILED) {				
				
			}
		}
	};
	
	public boolean runStandAlone(String application) {
		application = application.trim();

		if (application.equalsIgnoreCase("moped") || application.equalsIgnoreCase("moped_disk")) {
			Intent intent = new Intent(BatchSynthesisTest.this, CloudletCameraActivity.class);
			intent.putExtra("address", ADDRESS);
			intent.putExtra("port", CloudletActivity.TEST_CLOUDLET_APP_MOPED_PORT);
			startActivityForResult(intent, 0);
			return true;
		} else if (application.equalsIgnoreCase("graphics")) {
			Intent intent = new Intent(BatchSynthesisTest.this, GraphicsClientActivity.class);
			intent.putExtra("address", ADDRESS);
			intent.putExtra("port", CloudletActivity.TEST_CLOUDLET_APP_GRAPHICS_PORT);
			startActivityForResult(intent, 0);
			return true;
		} else {
			return false;
		}
	}
	
	View.OnClickListener clickListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			if(v.getId() == R.id.startBatchTest) {
				CloudletEnv.instance().resetOverlayList();
				final ArrayList<VMInfo> vmList = CloudletEnv.instance().getOverlayDirectoryInfo();
				if (vmList.size() == 0) {
					new AlertDialog.Builder(BatchSynthesisTest.this).setTitle("Error").setMessage("Cannot find overlay VM")
							.setIcon(R.drawable.ic_launcher).setNegativeButton("Confirm", null).show();
					return;
				}

				StringBuffer overlayStr = new StringBuffer();
				for(VMInfo overlayItem : vmList){
					overlayStr.append(overlayItem.getAppName());
				}
				
				// Show Dialog
				AlertDialog.Builder ab = new AlertDialog.Builder(BatchSynthesisTest.this);
				ab.setTitle("Application List");
				ab.setIcon(R.drawable.ic_launcher);
				ab.setMessage(overlayStr.toString());
				ab.setPositiveButton("Ok", new DialogInterface.OnClickListener() {
					@Override
					public void onClick(DialogInterface dialog, int position) {
						startBatchSynthesis();
					}
					
				}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
					public void onClick(DialogInterface dialog, int position) {
						return;
					}
				});
				ab.show();

			}
		}
	};

}
