package edu.cmu.cs.cloudlet.android.application.esvm;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.PrintWriter;
import java.util.HashMap;
import java.util.Locale;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Intent;
import android.hardware.Camera;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.speech.tts.TextToSpeech;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.widget.Button;
import android.widget.ScrollView;
import android.widget.TextView;

public class ESVMRecognActivity extends Activity implements TextToSpeech.OnInitListener {

	public static final String SERVER_IP = "hail.elijah.cs.cmu.edu";
	public static final int SERVER_PORT = 9095;
	public static final String TAG = "krha_app";
	
	protected static String server_ipaddress = null;
	protected static int server_port = -1;
	protected NetworkClient client;
	
	protected ProgressDialog mDialog;
	protected boolean gFocussed = false;
	protected boolean gCameraPressed = false;
	protected Uri mImageCaptureUri;
	protected Button mSendButton;
	protected Button mTestButton;

	protected byte[] mImageData;
	protected Preview mPreview;
	protected TextToSpeech mTTS;
	
	// time stamp for test
	protected long startApp;
	protected long endApp;
	private TextView textView;
	private ScrollView scrollView;

	static protected final String OUTLOG_FILENAME = "/mnt/sdcard/cloulet_exp_result.txt";
	static protected PrintWriter outlogWriter;		
	static{
		try {
			outlogWriter = new PrintWriter(new File(OUTLOG_FILENAME));
		} catch (FileNotFoundException e) {
			Log.e("krha_app", "Cannot Create Log File");
		}
	}

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		setContentView(R.layout.activity_esvmrecogn);		
		mPreview = (Preview) findViewById(R.id.camera_preview);
		
		// assign server connection information
		Bundle extras = getIntent().getExtras();
		if (extras != null){
			server_ipaddress = extras.getString("address");
			server_port = extras.getInt("port", -1);
		}		
		if (server_ipaddress == null || server_port == -1){
			server_ipaddress = ESVMRecognActivity.SERVER_IP;
			server_port = ESVMRecognActivity.SERVER_PORT;
		}

		// buttons
		mSendButton = (Button) findViewById(R.id.sendButton);
		mSendButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {				
				// capture image
				if (mPreview.mCamera != null) {
					mPreview.mCamera.takePicture(null, null, mPictureCallbackJpeg);
				}
			}
		});		

		// TextToSpeech.OnInitListener
		mTTS = new TextToSpeech(this, this);				
//		this.textView = (TextView) this.findViewById(R.id.cameraLogView);
//	    this.scrollView = (ScrollView) this.findViewById(R.id.cameraLogScroll);
	}
	

	/*
	 * Network Event Handler
	 */
	// network progress dialog
	private ProgressDialog progDialog;
	private Handler networkHandler = new Handler() {
		protected void updateMessage(String msg) {
			if ((progDialog != null) && (progDialog.isShowing())) {
				progDialog.setMessage(msg);
			}
		}

		public void handleMessage(Message msg) {
			if (msg.what == NetworkClient.CALLBACK_SUCCESS) {
				String retMsg = (String) msg.obj;
				HashMap<String, Float> retObjects = this.parseReturn(retMsg);
				StringBuffer sb = new StringBuffer();
				 
				AlertDialog.Builder ab = new AlertDialog.Builder(
						ESVMRecognActivity.this);
				ab.setTitle("Success");
				ab.setMessage(retObjects.toString());
				ab.setIcon(R.drawable.ic_launcher);
				ab.setPositiveButton("Confirm", null);
				ab.show();
				if (progDialog.isShowing()){
					progDialog.dismiss();
				}
			} else if (msg.what == NetworkClient.CALLBACK_UPDATE) {
				String retMsg = (String) msg.obj;
				this.updateMessage(retMsg);
			} else if (msg.what == NetworkClient.CALLBACK_FAILED) {
				String retMsg = (String) msg.obj;
				AlertDialog.Builder ab = new AlertDialog.Builder(
						ESVMRecognActivity.this);
				ab.setTitle("Failed");
				ab.setMessage(retMsg);
				ab.setIcon(R.drawable.ic_launcher);
				ab.setPositiveButton("Confirm", null);
				ab.show();
				
				if (progDialog.isShowing()){
					progDialog.dismiss();
				}
			}
		}

		private HashMap parseReturn(String retMsg) {
			String[] retList = retMsg.split(",");
			HashMap<String, Float> retMap = new HashMap<String, Float>();
			for (int i = 0; i < retList.length; i++){
				String objName = retList[i].split(":", 2)[0];
				Float score = Math.abs(Float.parseFloat(retList[i].split(":", 2)[1]));
				Float originalScore = retMap.get(objName);
				if (originalScore == null){
					retMap.put(objName, score);
				} else if(originalScore > score) {
					retMap.put(objName, originalScore);				
				}
			}
			return retMap;
		}
	};
	
	
	/*
	 * Upload Log 
	 */
	public void updateLog(String msg){
		if (this.textView != null){
			this.textView.append(msg);
			this.scrollView.post(new Runnable()
		    {
		        public void run()
		        {
		            scrollView.fullScroll(View.FOCUS_DOWN);
		        }
		    });
		}
	}

	/*
	 * TTS
	 */
	private static final String FEEDBACK_PREFIX = "Found items are ";
	private void TTSFeedback(String ttsString) {
		// Show Application Runtime
		String message = "Time for app run\n start: " + startApp + "\nend: " + endApp + "\ndiff: " + (endApp-startApp);		
		new AlertDialog.Builder(ESVMRecognActivity.this).setTitle("Info")
		.setMessage(message)
		.setIcon(R.drawable.ic_launcher)
		.setNegativeButton("Confirm", null)
		.show();
		
		// Select a random hello.
		Log.d("krha", "tts string origin: " + ttsString);
		String[] objects = ttsString.split(" ");
		if(ttsString == null || objects == null || objects.length == 0 || ttsString.trim().length() == 0){
			mTTS.speak("We found nothing", TextToSpeech.QUEUE_FLUSH, null);			
		}else if(objects.length == 1){
			objects[0].replace("_", " ");
			mTTS.speak("We found a " + objects[0], TextToSpeech.QUEUE_FLUSH, null);			
		}else{
			StringBuffer sb = new StringBuffer();
			for(int i = 0; i < objects.length; i++){
				sb.append(objects[i].replaceAll("_", " "));
				if(i != objects.length-1){
					sb.append(" and ");					
				}
			}
			Log.d("krha", "tts string : " + sb.toString());
			mTTS.setSpeechRate(1f);
			mTTS.speak("We found " + sb.toString(), TextToSpeech.QUEUE_FLUSH, null);
		}
		
	}

	// Implements TextToSpeech.OnInitListener
	public void onInit(int status) {
		if (status == TextToSpeech.SUCCESS) {
			int result = mTTS.setLanguage(Locale.US);
			if (result == TextToSpeech.LANG_MISSING_DATA || result == TextToSpeech.LANG_NOT_SUPPORTED) {
				Log.e("krha_app", "Language is not available.");
			}
		} else {
			// Initialization failed.
			Log.e("krha_app", "Could not initialize TextToSpeech.");
		}
	}
	
	/*
	 * Camera Capture
	 */
	Camera.PictureCallback mPictureCallbackJpeg = new Camera.PictureCallback() {
		public void onPictureTaken(byte[] data, Camera c) {
			//time stamp			
			mImageData = data;
			saveImage(mImageData);
			mImageData = null;

			if (mPreview.mCamera != null) {
				try {
					mPreview.mCamera.startPreview();
				} catch (Exception e) {

				}
			}
		}
	};

	public void saveImage(byte[] data) {
		// start progress bar
		if (this.progDialog == null) {
			this.progDialog = ProgressDialog.show(this, "Info",
					"Offloading to " + this.server_ipaddress, true);
			this.progDialog.setIcon(R.drawable.ic_launcher);
		} else {
			this.progDialog.setMessage("Offloading to " + this.server_ipaddress);
		}
		this.progDialog.show();
		
		// upload image
		if(client == null){
			client = new NetworkClient(ESVMRecognActivity.this, networkHandler, this.server_ipaddress, this.server_port);
			client.start();
		}
		client.uploadImage(data);
	}
	
	Camera.AutoFocusCallback cb = new Camera.AutoFocusCallback() {
		public void onAutoFocus(boolean success, Camera c) {
			if (success) {
				ToneGenerator tg = new ToneGenerator(AudioManager.STREAM_SYSTEM, 100);
				if (tg != null)
					tg.startTone(ToneGenerator.TONE_PROP_BEEP2);
				gFocussed = true;
				try {
					if (gCameraPressed) {
						if (mPreview.mCamera != null) {
							mPreview.mCamera.takePicture(null, null, mPictureCallbackJpeg);
						}
					}
				} catch (Exception e) {
					Log.i("Exc", e.toString());
				}
			} else {
				ToneGenerator tg = new ToneGenerator(AudioManager.STREAM_SYSTEM, 100);
				if (tg != null)
					tg.startTone(ToneGenerator.TONE_PROP_BEEP2);

				try {
					if (gCameraPressed) {
						if (mPreview.mCamera != null) {
							mPreview.mCamera.takePicture(null, null, mPictureCallbackJpeg);
						}
					}
				} catch (Exception e) {
					Log.i("Exc", e.toString());
				}
			}
		}
	};

	/*
	 * Destroy
	 * @see android.app.Activity#onKeyDown(int, android.view.KeyEvent)
	 */
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {

            Intent caller = getIntent(); 
            caller.putExtra("message", "finish"); 
            setResult(RESULT_OK, caller); 
            finish();
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public void onDestroy() {
		if(client != null)
			client.close();		
		if(mPreview != null)
			mPreview.close();
		if (mTTS != null) {
			mTTS.stop();
			mTTS.shutdown();
		}
		super.onDestroy();
	}

}
