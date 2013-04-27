package edu.cmu.cs.cloudlet.android.test;

import java.io.File;
import java.util.ArrayList;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
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
	private ProgressDialog progDialog;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.test_main);

		// Initiate Environment Settings
		CloudletEnv.instance();
		this.overlayVMList = CloudletEnv.instance()
				.getOverlayDirectoryInfo();
		if (this.overlayVMList.size() == 0) {
			new AlertDialog.Builder(BatchSynthesisTest.this).setTitle("Error")
					.setMessage("Cannot find overlay VM")
					.setIcon(R.drawable.ic_launcher)
					.setNegativeButton("Confirm", null).show();
		} else {			
			// update application to launch application correctly
			for (VMInfo overlayVM : this.overlayVMList){
				String appName = overlayVM.getAppName();
				appName = appName.split("_")[0];
				overlayVM.setAppName(appName);
			}
		}

		findViewById(R.id.startBatchTest).setOnClickListener(clickListener);
	}

	/*
	 * Synthesis start
	 */
	protected void startBatchSynthesis(String address, int port,
			VMInfo overlayVM) {
		if (this.connector != null) {
			this.connector.close();
		}
		
		this.connector = new CloudletConnector(BatchSynthesisTest.this,
				synthesisHandler);
		this.connector.setConnection(address, port, overlayVM);
		this.connector.start();

		if (this.progDialog == null) {
			this.progDialog = ProgressDialog.show(this, "Info",
					"Connecting to " + address, true);
			this.progDialog.setIcon(R.drawable.ic_launcher);
		} else {
			this.progDialog.setMessage("Connecting to " + address);
		}
		this.progDialog.show();
	}

	/*
	 * Synthesis callback handler
	 */
	Handler synthesisHandler = new Handler() {

		protected void updateMessage(String msg) {
			if ((progDialog != null) && (progDialog.isShowing())) {
				progDialog.setMessage(msg);
			}
		}

		public void handleMessage(Message msg) {
			if (msg.what == CloudletConnector.SYNTHESIS_SUCCESS) {
				String appName = (String) msg.obj;
				this.updateMessage("Synthesis SUCESS");
				connector.closeRequest();
				
				// start next VM Synthesis
				if (overlayVMList.size() > 0){
					final VMInfo overlayVM = overlayVMList.remove(0);
					startBatchSynthesis(ADDRESS, PORT, overlayVM);
				}else{
					// finish
					progDialog.dismiss();					
				}
			} else if (msg.what == CloudletConnector.SYNTHESIS_FAILED) {
				String reason = (String) msg.obj;
				if (reason != null)
					this.updateMessage(reason);
				else
					this.updateMessage("Synthesis FAILED");
				progDialog.dismiss();

				AlertDialog.Builder ab = new AlertDialog.Builder(
						BatchSynthesisTest.this);
				ab.setTitle("VM Synthesis Failed");
				ab.setIcon(R.drawable.ic_launcher);
				ab.setPositiveButton("Ok", null).setNegativeButton("Cancel",
						null);
				ab.show();
			} else if (msg.what == CloudletConnector.SYNTHESIS_PROGRESS_PERCENT) {
				String message = (String) msg.obj;
				this.updateMessage(message);
			}
		}
	};

	public boolean runStandAlone(String application) {
		application = application.trim();

		if (application.equalsIgnoreCase("moped")
				|| application.equalsIgnoreCase("moped_disk")) {
			Intent intent = new Intent(BatchSynthesisTest.this,
					CloudletCameraActivity.class);
			intent.putExtra("address", ADDRESS);
			intent.putExtra("port",
					CloudletActivity.TEST_CLOUDLET_APP_MOPED_PORT);
			startActivityForResult(intent, 0);
			return true;
		} else if (application.equalsIgnoreCase("graphics")) {
			Intent intent = new Intent(BatchSynthesisTest.this,
					GraphicsClientActivity.class);
			intent.putExtra("address", ADDRESS);
			intent.putExtra("port",
					CloudletActivity.TEST_CLOUDLET_APP_GRAPHICS_PORT);
			startActivityForResult(intent, 0);
			return true;
		} else {
			return false;
		}
	}

	View.OnClickListener clickListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			if (v.getId() == R.id.startBatchTest) {
				if (overlayVMList.size() <= 0)
					return;
				
				final VMInfo overlayVM = overlayVMList.remove(0);

				// Show Dialog
				AlertDialog.Builder ab = new AlertDialog.Builder(
						BatchSynthesisTest.this);
				ab.setTitle("Overlay VM list");
				ab.setIcon(R.drawable.ic_launcher);
				ab.setMessage(overlayVM.toString());
				ab.setPositiveButton("Ok",
						new DialogInterface.OnClickListener() {
							@Override
							public void onClick(DialogInterface dialog,
									int position) {
								startBatchSynthesis(ADDRESS, PORT, overlayVM);
							}

						}).setNegativeButton("Cancel", null);
				ab.show();

			}
		}
	};

}
