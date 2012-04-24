package edu.cmu.cs.cloudlet.android.application.graphics;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import edu.cmu.cs.cloudlet.android.R;

import android.app.Activity;
import android.app.AlertDialog;
import android.hardware.SensorListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.ScrollView;
import android.widget.TextView;

public class GraphicsClientActivity extends Activity implements SensorListener {
	public static final String TEST_ACC_FILE = "/mnt/sdcard/Cloudlet/Graphics/acc_input_50sec";
	
	public static String SERVER_ADDRESS = "server.krha.kr";
	public static int SERVER_PORT = 9093;

	private SensorManager sensor;
	private GNetworkClient connector;
	private TextView textView;
	private ScrollView scrollView;

	protected ArrayList<String> testAccList = new ArrayList<String>();
	private float[] latest_acc = new float[2];
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.graphics);
		sensor = (SensorManager) getSystemService(SENSOR_SERVICE);
		this.textView = (TextView)findViewById(R.id.logView);
		this.connector = new GNetworkClient(this, GraphicsClientActivity.this);
	    this.scrollView = (ScrollView) this.findViewById(R.id.logScroll);

		Bundle extras = getIntent().getExtras();
		this.SERVER_ADDRESS = extras.getString("address");
		this.SERVER_PORT = extras.getInt("port");
		
		// For OSDI Test, just start sending data
		File testFile = new File(TEST_ACC_FILE);
		if(testFile.exists() == false){
			new AlertDialog.Builder(GraphicsClientActivity.this).setTitle("Error")
			.setMessage("No test acc data at " + TEST_ACC_FILE)
			.setIcon(R.drawable.ic_launcher)
			.setNegativeButton("Confirm", null)
			.show();
		}
		try{
			BufferedReader reader = new BufferedReader(new FileReader(testFile));
			String oneLine = "";
			while((oneLine = reader.readLine()) != null){
				String[] tokens = oneLine.split("  ");
				if(tokens.length != 3)
					continue;
				testAccList.add(tokens[1] + " " + tokens[2]);
			}
			reader.close();	
		}catch(IOException e){
			Log.e("krha", e.toString());
		}
		
		TimerTask autoStart = new TimerTask(){
			@Override
			public void run() {
				connector.updateAccList(testAccList);
				connector.startConnection(SERVER_ADDRESS, SERVER_PORT);
			} 
		};
	    
		Timer startTiemr = new Timer();
		startTiemr.schedule(autoStart, 1000);

	}


	public void showAlert(String type, String message) {
		new AlertDialog.Builder(GraphicsClientActivity.this).setTitle(type)
				.setMessage(message).setIcon(R.drawable.ic_launcher)
				.setNegativeButton("Confirm", null).show();
	}
	
	public float[] getLatestAcc(){
		return this.latest_acc;
	}
	
	public void updateLog(String msg){
		this.textView.append(msg);
		this.scrollView.post(new Runnable()
	    {
	        public void run()
	        {
	            scrollView.fullScroll(View.FOCUS_DOWN);
	        }
	    });
	}

	
	@Override
	public void onAccuracyChanged(int arg0, int arg1) {

	}

	@Override
	public void onSensorChanged(int sensor, float[] values) {
        if (sensor == SensorManager.SENSOR_ACCELEROMETER) {
//            Log.d("krha", "X: " + values[0] + "\tY: " + values[1] + "\tZ: " + values[2]);
        	this.latest_acc[0] = values[0];
        	this.latest_acc[1] = values[1];
        }
	}

	@Override
	protected void onResume() {
		super.onResume();
		this.sensor.registerListener(this, SensorManager.SENSOR_ACCELEROMETER,
				SensorManager.SENSOR_DELAY_GAME);
	}

	@Override
	protected void onStop() {
		this.sensor.unregisterListener(this);
		this.connector.close();
		super.onStop();
	}

	@Override
    protected void onPause() {
        super.onPause();
        this.sensor.unregisterListener(this);
    }
}