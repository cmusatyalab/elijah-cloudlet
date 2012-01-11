package edu.cmu.cs.cloudlet.android.network;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.mime.HttpMultipartMode;
import org.apache.http.entity.mime.MultipartEntity;
import org.apache.http.entity.mime.content.FileBody;
import org.apache.http.entity.mime.content.InputStreamBody;
import org.apache.http.entity.mime.content.StringBody;
import org.apache.http.impl.client.BasicResponseHandler;
import org.apache.http.impl.client.DefaultHttpClient;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.application.CloudletCameraActivity;
import edu.cmu.cs.cloudlet.android.data.VMInfo;
import edu.cmu.cs.cloudlet.android.upnp.DeviceDisplay;
import edu.cmu.cs.cloudlet.android.upnp.UPnPDiscovery;
import edu.cmu.cs.cloudlet.android.util.KLog;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.widget.Toast;

public class HTTPCommandSender extends Thread {
	protected static final int SUCCESS = 0;
	protected static final int FAIL = 1;
	
	protected ProgressDialog mDialog;
	protected String httpURL = "";
	
	private CloudletActivity activity = null;
	protected Context context;
	private String command = null;
	private VMInfo overlayVM = null;

	public HTTPCommandSender(CloudletActivity activity, Context context, String command, VMInfo overlayVM) {
		this.activity = activity;
		this.context = context;
		this.command = command;
		this.overlayVM = overlayVM;
	}

	public void initSetup(String url) {
		httpURL = "http://" + CloudletActivity.TEST_CLOUDLET_SERVER_IP + ":" + CloudletActivity.SYNTHESIS_HTTP_PORT + "/" + url;
		mDialog = ProgressDialog.show(context, "Info", "Connecting to " + httpURL + "\nwaiting for " + overlayVM.getInfo(VMInfo.JSON_KEY_NAME) + " to run at " + command, true);		
		mDialog.show();
	}
	
	Handler networkHandler = new Handler() {
		public void handleMessage(Message msg) {			
			if (msg.what == HTTPCommandSender.SUCCESS) {
				String applicationName = ((VMInfo)msg.obj).getInfo(VMInfo.JSON_KEY_NAME);
				activity.runStandAlone(applicationName);
			}else if(msg.what == HTTPCommandSender.FAIL){
				String ret = (String)msg.obj;
				activity.showAlert("Error", "Failed to connect to " + httpURL + "\n" + ret);
			}
		}
	};

	public void run() {
		String ret = httpPostCommand(httpURL, this.command, this.overlayVM);
		if(ret != null && ret.equalsIgnoreCase("SUCCESS")){
			KLog.println("HTTP Return : " + ret);
			Message message = Message.obtain();
			message.what = HTTPCommandSender.SUCCESS;
			message.obj = this.overlayVM;
			networkHandler.sendMessage(message);
		}else{
			Message message = Message.obtain();
			message.what = HTTPCommandSender.FAIL;
			message.obj = ret;
			networkHandler.sendMessage(message);
		}
		
		
		if(mDialog != null){
			mDialog.dismiss();	
		}
	}

	private String httpPostCommand(String httpURL, String runType, VMInfo overlayInfo) {
		// Create a new HttpClient and Post Header		
		HttpClient httpclient = new DefaultHttpClient();
		HttpPost httppost = new HttpPost(httpURL);

		try {
			// Set JSON Command
			JSONObject json = getJSONCommand(overlayInfo);

			File overlayDisk = new File(overlayInfo.getInfo(VMInfo.JSON_KEY_DISKIMAGE_PATH));
			File overlayMem = new File(overlayInfo.getInfo(VMInfo.JSON_KEY_MEMORYSNAPSHOT_PATH));
			KLog.println("connecting to " + httpURL);
//			MultipartEntity entity = new MultipartEntity();
			MultipartEntity entity = new MultipartEntity(HttpMultipartMode.BROWSER_COMPATIBLE);
			
			
			File tempFile = new File(CloudletCameraActivity.TEST_IMAGE_PATH);
			byte[] data = new byte[(int) tempFile.length()];
			FileInputStream is;
			try {
				is = new FileInputStream(tempFile);
				is.read(data);
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}	
			entity.addPart("disk_file", new InputStreamBody(new FileInputStream(overlayDisk), overlayDisk.getName()));

//			entity.addPart("info", new StringBody(json.toString()));
//			entity.addPart("disk_file", new FileBody(overlayDisk));
//			entity.addPart("mem_file", new FileBody(overlayMem));			
//			entity.addPart("disk_file", new InputStreamBody(new FileInputStream(overlayDisk), overlayDisk.getName()));
//			entity.addPart("mem_file", new InputStreamBody(new FileInputStream(overlayMem), overlayMem.getName()));
			httppost.setEntity(entity);

			// Execute HTTP Post Request
			KLog.println("Execute Request " + httpURL);
			HttpResponse response = httpclient.execute(httppost);
			BasicResponseHandler myHandler = new BasicResponseHandler();
			String endResult = myHandler.handleResponse(response);
			
			return endResult;			
		} catch (ClientProtocolException e) {
			KLog.printErr(e.toString());
			return e.toString();
		} catch (IOException e) {
			KLog.printErr(e.toString());
			return e.toString();
		} catch (IllegalStateException e){
			KLog.printErr(e.toString());
			return e.toString();			
		}
	}

	private JSONObject getJSONCommand(VMInfo overlayInfo) {
		JSONObject jsonRoot = new JSONObject();
		JSONObject jsonVM = new JSONObject();
		JSONArray jsonVMArray = new JSONArray();
		
		try {
			jsonVM.put("type", "baseVM");
			jsonVM.put("name", overlayInfo.getInfo(VMInfo.JSON_KEY_BASE_NAME));
			jsonVMArray.put(jsonVM);
			
			jsonRoot.put("CPU-core", "2");
			jsonRoot.put("Memory-Size", "4GB");
			jsonRoot.put("VM", jsonVMArray);			
		} catch (JSONException e) {
			KLog.printErr(e.toString());
		}
		
		return jsonRoot;
	}
}

class InputStreamKnownSizeBody extends InputStreamBody {
	private int lenght;

	public InputStreamKnownSizeBody(
			final InputStream in, final int lenght,
			final String mimeType, final String filename) {
		super(in, mimeType, filename);
		this.lenght = lenght;
	}

	@Override
	public long getContentLength() {
		return this.lenght;
	}
}