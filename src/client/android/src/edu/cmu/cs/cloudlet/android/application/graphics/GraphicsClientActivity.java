package edu.cmu.cs.cloudlet.android.application.graphics;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.application.Preview;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.graphics.Point;
import android.hardware.SensorListener;
import android.hardware.SensorManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Display;
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

	protected ArrayList<String> testAccList = new ArrayList<String>();
	
	// Visualization
	private Graphics graphics;
	private GLSurfaceView GLView;
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
	    super.onCreate(savedInstanceState);

		setContentView(R.layout.graphics);
		this.GLView = (GLSurfaceView) findViewById(R.id.fluidgl_view);
		this.textView = (TextView) findViewById(R.id.fluid_textView);
		this.graphics = new Graphics();
		GLView.setRenderer(new PointRenderer(this.graphics));
		textView.setText("Initialization");
		
		sensor = (SensorManager) getSystemService(SENSOR_SERVICE);
		this.connector = new GNetworkClient(this, GraphicsClientActivity.this);

		Bundle extras = getIntent().getExtras();
		this.SERVER_ADDRESS = extras.getString("address");
		this.SERVER_PORT = extras.getInt("port");
		
		// Screen Size for visualization
		Display display = getWindowManager().getDefaultDisplay();
		Point size = new Point();
		VisualizationStaticInfo.screenWidth = display.getWidth();
		VisualizationStaticInfo.screenHeight = display.getHeight();
		
		TimerTask autoStart = new TimerTask(){
			@Override
			public void run() {
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
	
	public void updateData(Object obj) {
		ByteBuffer buffer = (ByteBuffer)obj;
		graphics.updatePosition(buffer);
	}
	
	
	public void updateLog(String msg){
		textView.setText(msg);
		/*
		this.textView.append(msg);
		this.scrollView.post(new Runnable()
	    {
	        public void run()
	        {
	            scrollView.fullScroll(View.FOCUS_DOWN);
	        }
	    });
	    */
	}

	
	@Override
	public void onAccuracyChanged(int arg0, int arg1) {

	}

	@Override
	public void onSensorChanged(int sensor, float[] values) {
        if (sensor == SensorManager.SENSOR_ACCELEROMETER) {
        	connector.updateAccValue(values);
        }
	}

	@Override
	protected void onResume() {
		super.onResume();
		GLView.onResume();
		this.sensor.registerListener(this, SensorManager.SENSOR_ACCELEROMETER,
				SensorManager.SENSOR_DELAY_GAME);
	}

	@Override
	protected void onStop() {
		this.sensor.unregisterListener(this);
		if(this.connector != null)
			this.connector.close();
		super.onStop();
	}

	@Override
    protected void onPause() {
        super.onPause();
		GLView.onPause();
        this.sensor.unregisterListener(this);
    }


	
}