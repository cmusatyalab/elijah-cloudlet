package edu.cmu.cs.cloudlet.android.discovery;

import org.teleal.cling.android.AndroidUpnpServiceImpl;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.widget.ArrayAdapter;
import edu.cmu.cs.cloudlet.android.R;

public class CloudletDiscovery {

	public static final int DEVICE_SELECTED = 1;
	public static final int USER_CANCELED = 2;
	
	private ArrayAdapter<CloudletDevice> listAdapter;
	private int lastSelectedIndex = -1;
	
	private CloudletDirectoryClient globalDiscovery;
	private UPnPDiscovery localDiscovery;
	
	private Activity activity;
	private Context context;
	private Handler handler;
	
	public CloudletDiscovery(Activity activity, Context context, Handler discoveryHandler) {

		this.activity = activity;
		this.handler = discoveryHandler;
		this.context = context;		
		this.listAdapter = new ArrayAdapter<CloudletDevice>(this.context, R.layout.dialog_list_item);
		
		// upnp service binding and show dialog
		this.localDiscovery = new UPnPDiscovery(this.activity, this.listAdapter);
		this.activity.getApplicationContext().bindService(new Intent(this.activity, AndroidUpnpServiceImpl.class),
				this.localDiscovery.serviceConnection, Context.BIND_AUTO_CREATE);		
		this.showDialogSelectOption();

		// Cloudlet discovery client
		this.globalDiscovery = new CloudletDirectoryClient(this.activity, this.listAdapter);		
		globalDiscovery.start();
		
	}

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
				if(position >= 0){
					msg.obj = listAdapter.getItem(position);
					msg.what = CloudletDiscovery.DEVICE_SELECTED;					
				}else if(lastSelectedIndex != -1){
					msg.obj = listAdapter.getItem(lastSelectedIndex);
					msg.what = CloudletDiscovery.DEVICE_SELECTED;
				}else if(listAdapter.getCount() > 0){
					msg.obj = listAdapter.getItem(0);
					msg.what = CloudletDiscovery.DEVICE_SELECTED;
				}else{
					msg.what = CloudletDiscovery.USER_CANCELED;					
				}
				handler.sendMessage(msg);
			}
		}).setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int position) {
				Message msg = Message.obtain();
				msg.what = CloudletDiscovery.USER_CANCELED;
				handler.sendMessage(msg);
			}
		});
		ab.show();
	}
	
	public void close(){
		this.activity.getApplicationContext().unbindService(this.localDiscovery.serviceConnection);
		if (this.localDiscovery != null)
			this.localDiscovery.close();
		
		if (this.globalDiscovery != null)
			this.globalDiscovery.close();
	}
}
