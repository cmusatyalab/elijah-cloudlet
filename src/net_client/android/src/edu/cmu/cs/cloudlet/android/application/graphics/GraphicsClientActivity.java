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

import javax.microedition.khronos.opengles.GL10;

import edu.cmu.cs.cloudlet.android.CloudletActivity;
import edu.cmu.cs.cloudlet.android.R;
import edu.cmu.cs.cloudlet.android.application.Preview;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Point;
import android.hardware.SensorListener;
import android.hardware.SensorManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Environment;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
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
	private PointRenderer pointRenderer;
	
	
	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
	    super.onCreate(savedInstanceState);

		setContentView(R.layout.graphics);
		this.GLView = (GLSurfaceView) findViewById(R.id.fluidgl_view);
		this.textView = (TextView) findViewById(R.id.fluid_textView);
		
		/** INIT GRAPHICS **/
		//Graphics(true) == 3D, Graphics(false) == 2D
		this.graphics = new Graphics();	
		pointRenderer = new PointRenderer(this.graphics);
		GLView.setRenderer(pointRenderer);
		//RENDER ONLY WHEN THE SCENE HAS CHANGED
		GLView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
		/***************ALLOW TOUCH COMMANDS **************/
		GLView.requestFocus();
		GLView.setFocusableInTouchMode(true);
		/*******************/
		
		textView.setText("Initialization");
		
		sensor = (SensorManager) getSystemService(SENSOR_SERVICE);
		this.connector = new GNetworkClient(this, GraphicsClientActivity.this);

		Bundle extras = getIntent().getExtras();
		this.SERVER_ADDRESS = extras.getString("address");
		this.SERVER_PORT = extras.getInt("port");
		
		// Screen Size for visualization
		Display display = getWindowManager().getDefaultDisplay();
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
	
	private int counter = 0;
	
	public void updateData(Object obj) {
		ByteBuffer buffer = (ByteBuffer)obj;
		
		graphics.updatePosition(buffer);
		
		//TELL THE RENDERER TO REDRAW THE SCENE
		GLView.requestRender();
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
        	if (connector != null)
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
		if(this.connector != null){
			this.connector.close();
			this.connector = null;
		}
		super.onStop();
	}

	@Override
    protected void onPause() {
        super.onPause();
		GLView.onPause();
        this.sensor.unregisterListener(this);
    }
	
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
            Intent caller = getIntent(); 
            caller.putExtra("message", "finish"); 
            setResult(RESULT_OK, caller); 
			this.connector.close();
			this.connector = null;
			finish();
		}
		return super.onKeyDown(keyCode, event);
	}
	
	
	/**********************************************************************
	 * 							SCREEN ROTATION 						  *
	 * ****************************************************************** */
	
    /* Rotation values */
	private float xrot = 19.0f;					//X Rotation
	private float yrot = -64.0f;					//Y Rotation

	private float z = -5.0f;	
    /*
	 * These variables store the previous X and Y
	 * values as well as a fix touch scale factor.
	 * These are necessary for the rotation transformation
	 * added to this lesson, based on the screen touches. ( NEW )
	 */
	private float oldX;
    private float oldY;
	private final float TOUCH_SCALE = 0.2f;		//Proved to be good for normal rotation ( NEW )
	
    /**
	 * Override the touch screen listener.
	 * 
	 * React to moves and presses on the touchscreen.
	 */
	public boolean onTouchEvent(MotionEvent event) {
		//
		float x = event.getX();
        float y = event.getY();
        
        //If a touch is moved on the screen
        if(event.getAction() == MotionEvent.ACTION_MOVE) {
        	//Calculate the change
        	float dx = x - oldX;
	        float dy = y - oldY;
        	//Define an upper area of 10% on the screen
        	int upperArea = VisualizationStaticInfo.screenHeight / 10;
        	
        	//Zoom in/out if the touch move has been made in the upper
        	if(y < upperArea) {
        		z -= dx * TOUCH_SCALE / 4;
        	
        	//Rotate around the axis otherwise
        	} else {        		
    	        xrot += dy * TOUCH_SCALE;
    	        yrot += dx * TOUCH_SCALE;
        	}        
        	
        	//send changed data
//        	pointRenderer.xrot = xrot;
//        	pointRenderer.yrot = yrot;
//        	pointRenderer.z = z;
        	
        	GLView.requestRender();
        }
        
        //Remember the values
        oldX = x;
        oldY = y;
        
        //We handled the event
		return true;
	}
	
	
	
	
}