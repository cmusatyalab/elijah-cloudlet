package edu.cmu.cs.cloudlet.android.network;

import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Handler;
import android.os.Message;
import android.util.TypedValue;
import android.view.View;
import android.widget.TextView;

public class CloudletConnector {
	public static final int CONNECTION_ERROR	 	= 1;
	public static final int PROGRESS_MESSAGE		= 2;
	public static final int FINISH_MESSAGE			= 3;
	
	protected Context mContext;
	protected NetworkClientSender networkClient;

	protected ProgressDialog mDialog;
	protected StringBuffer messageBuffer;

	public CloudletConnector(Context context) {
		this.mContext = context;
		networkClient = new NetworkClientSender(context, eventHandler);
		messageBuffer = new StringBuffer();
	}
	
	public void startConnection(String ipAddress, int port) {
		if(mDialog == null){						 
			mDialog = ProgressDialog.show(this.mContext, "Info", "Connecting to " + ipAddress , true);
			mDialog.setIcon(R.drawable.ic_launcher);
		}else{
			mDialog.setMessage("Connecting to " + ipAddress);
		}
		mDialog.show();
		
		networkClient.setConnection(ipAddress, port); 
		networkClient.start();		
	}
	
	public View.OnClickListener protocolTestClickListener = new View.OnClickListener() {
		public void onClick(View v) {
			NetworkMsg networkMsg = null;
			
			switch(v.getId()){
			case R.id.protocol_test1:
				// Send REQ VM List
				networkMsg = NetworkMsg.MSG_OverlayList();
				break;
			case R.id.protocol_test2:
				// Send REQ Transfer_Start
				VMInfo vm2 = new VMInfo().getTestVMInfo();
				networkMsg = NetworkMsg.MSG_SelectedVM(vm2);
				break;
			case R.id.protocol_test3:				
				// REQ Launch
				VMInfo vm3 = new VMInfo().getTestVMInfo();
				networkMsg = NetworkMsg.MSG_LaunchVM(vm3, 4, 512);
				break;
			case R.id.protocol_test4:
				// REQ Stop
				VMInfo vm4 = new VMInfo().getTestVMInfo();
				networkMsg = NetworkMsg.MSG_StopVM(vm4);
				break;
			case R.id.protocol_test5:
			break;
			}
			
			networkMsg.printJSON();
			networkClient.requestCommand(networkMsg);
		}
	};

	public void close() {
		if(this.networkClient != null)
			this.networkClient.close();
	}
	

	/*
	 * Server Message Handler
	 */
	Handler eventHandler = new Handler() {
		public void handleMessage(Message msg) {
			if(msg.what == CloudletConnector.CONNECTION_ERROR){
				if(mDialog != null && mDialog.isShowing() == true){
					mDialog.dismiss();
				}
				
				messageBuffer.append(msg.getData().getString("message") + "\n");
				new AlertDialog.Builder(mContext).setTitle("Error")
				.setMessage(messageBuffer.toString())
				.setNegativeButton("Confirm", null)
				.show();
			}else if(msg.what == CloudletConnector.PROGRESS_MESSAGE){
				messageBuffer.append(msg.getData().getString("message") + "\n");
				if(mDialog == null){
					mDialog = ProgressDialog.show(mContext, "Info", messageBuffer.toString(), true);					
				}
				mDialog.setMessage(messageBuffer.toString());
				if(mDialog.isShowing() == false){
					mDialog.show();
				}

				KLog.println(messageBuffer.toString());
			}
		}
	};
	
	private void DialogSelectOption() {
		final String items[] = { "item1", "item2", "item3" };
		AlertDialog.Builder ab = new AlertDialog.Builder(this.mContext);
		ab.setTitle("Title");
		ab.setIcon(R.drawable.ic_launcher);
		ab.setSingleChoiceItems(items, 0,
			new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
			}
			}).setPositiveButton("Ok",
			new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
			}
			}).setNegativeButton("Cancel",
			new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int whichButton) {
			}
			});
		ab.show();
	}

}
