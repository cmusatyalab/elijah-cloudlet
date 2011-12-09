package edu.cmu.cs.cloudlet.android.application.speech;

import java.io.BufferedReader;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.Socket;
import java.net.UnknownHostException;

import edu.cmu.cs.cloudlet.android.R;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public class ClientActivity extends Activity implements OnClickListener
{
	public static final int MENU_ID_SETTINGS = 76;
	public static final int MENU_ID_CLEAR_LOG = 86;

	public static final String PRESS_TO_SPEAK = "Press To Speak";
	public static final String PRESS_TO_STOP = "Press To Stop";

	public static final String AUDIO_FILE_PATH = Environment.getExternalStorageDirectory().toString()+
	"/Cloudlet/SPEECH/myrecordings/audio.wav";
	
	public static final String LOG_FILE_DIRECTORY = Environment.getExternalStorageDirectory().toString()+
	"/Cloudlet/SPEECH/log";

	private LinearLayout container;
	private Button pressToSpeakButton;

	private String serverAddress;
	private int portNumber;
	private Socket connection;
	private DataOutputStream outToServer;
	private FileInputStream fileInputStream;

	private ExtAudioRecorder recorder;
	private boolean recording = false;

	private ScrollView scrollview;

	private FileWriter logWriter;
	private File logFile;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.speech);
		
		File directory = new File( LOG_FILE_DIRECTORY );
		directory.mkdirs();
		
		logFile = new File( LOG_FILE_DIRECTORY+"/log_"+System.currentTimeMillis()+".txt" );
		try 
		{
			logWriter = new FileWriter( logFile );
		} 
		catch (IOException e) 
		{
			e.printStackTrace();
		}

		scrollview = (ScrollView)findViewById(R.id.scrollView);
		container = (LinearLayout)findViewById(R.id.container);
		pressToSpeakButton = (Button)findViewById(R.id.presstospeak_button);

		pressToSpeakButton.setOnClickListener( this );

		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
		serverAddress = prefs.getString("serveraddresspreference", "127.0.0.1");
		portNumber = Integer.parseInt(prefs.getString("serverportpreference", "6789"));

	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) 
	{
		menu.add(0, MENU_ID_SETTINGS, 0, "Settings");
		menu.add(0, MENU_ID_CLEAR_LOG, 0, "Clear Log");
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onMenuItemSelected(int featureId, MenuItem item)
	{
		switch( item.getItemId() )
		{
		case MENU_ID_SETTINGS:
			startActivity( new Intent( this, MyPreferenceActivity.class));
			break;
		case MENU_ID_CLEAR_LOG:
			container.removeAllViews();
			break;
		}
		return super.onMenuItemSelected(featureId, item);
	}

	public void print( String text )
	{
		TextView tv = new TextView( this );
		tv.setText(timeStamp() + text );
		container.addView( tv );
		try 
		{
			logWriter.write(text+"\n");
			logWriter.flush();
		} catch (IOException e) 
		{
			e.printStackTrace();
		}
	}

	public void onPressToSpeakButtonClick()
	{

		//new ConnectToServer( this ).execute( null );

		if( recording == false )
		{
			recorder = ExtAudioRecorder.getInstanse( false );
			recorder.setOutputFile( AUDIO_FILE_PATH );
			try 
			{
				recorder.prepare();
				recorder.start();
				recording = true;
				print("Recording...");
				pressToSpeakButton.setText( PRESS_TO_STOP );
			} 
			catch ( IllegalStateException e ) 
			{
				print("Error recording sound file...");
				pressToSpeakButton.setText( PRESS_TO_SPEAK );
				e.printStackTrace();
			} 
		}
		else
		{
			recorder.stop();
			recorder.reset();
			recorder.release();
			recording = false;
			print("Stopped recording...");
			pressToSpeakButton.setText( PRESS_TO_SPEAK );

			print("Connecting to server " + serverAddress+":"+portNumber);
			new ConnectToServer( this ).execute( null );

		}
	}



	@Override
	public void onClick(View v) 
	{
		if( v.getId() == pressToSpeakButton.getId() )
		{
			onPressToSpeakButtonClick();
		}
	}

	/*	public AlertDialog createRecordingAlert()
	{
		AlertDialog.Builder builder = new AlertDialog.Builder( this );
		builder.setMessage("Recording...");
		builder.setNegativeButton("Done", new DialogInterface.OnClickListener() {
			@Override
			public void onClick(DialogInterface dialog, int which)
			{
				recordingAlert.dismiss();
				recorder.stop();
				recorder.reset();
				recording = false;
				print("Stopped recording...");
				pressToSpeakButton.setText( PRESS_TO_SPEAK );
				print("Connecting to server " + serverAddress+":"+portNumber);
				new ConnectToServer( getBaseContext() ).execute( null );	

			}
		});
		AlertDialog alert = builder.create();
		alert.setCancelable( false );
		return alert;
	}*/


	/**
	 * Creates a ProgressDialog that tries to connect to the server via the information stored in preferences
	 */
	public class ConnectToServer extends AsyncTask<String, Integer, Integer>
	{

		//ProgressDialog progressDialog;

		public ConnectToServer( Context context )
		{
			//progressDialog = new ProgressDialog( context );
			//progressDialog.setMessage("Connecting to server...");
			//progressDialog.setCancelable( false );
		}

		@Override
		protected void onPreExecute() {
			super.onPreExecute();
			//progressDialog.show();
		}

		@Override
		protected void onPostExecute(Integer result) {
			super.onPostExecute(result);
			//progressDialog.dismiss();
		}
		@Override
		protected Integer doInBackground(String... params) 
		{
			try 
			{
				connection = new Socket( serverAddress, portNumber );
				runOnUiThread( new Runnable() {
					@Override
					public void run() {print("Connected to server...");
					scrollview.fullScroll(ScrollView.FOCUS_DOWN);
					}});

				outToServer = new DataOutputStream( connection.getOutputStream() );
				File file = new File ( AUDIO_FILE_PATH );
				outToServer.writeLong( file.length() );
				fileInputStream = new FileInputStream( file );

				//progressDialog.setMessage("Sending audio file to server...");

				//send audio file
				int readData;
				byte[] buffer = new byte[1024];

				runOnUiThread( new Runnable() {
					@Override
					public void run() {print("Sending file...");
					scrollview.fullScroll(ScrollView.FOCUS_DOWN);
					}});

				while( (readData=fileInputStream.read(buffer))!= -1 )
				{
					outToServer.write(buffer, 0, readData);
				}

				runOnUiThread( new Runnable() {
					@Override
					public void run() {print("File sent successfully...");
					scrollview.fullScroll(ScrollView.FOCUS_DOWN);
					}});

				BufferedReader inFromServer = new BufferedReader( new InputStreamReader( connection.getInputStream() ) );

				boolean done = false;

				runOnUiThread( new Runnable() {
					@Override
					public void run() {
						print("About to read from server...");
						scrollview.fullScroll(ScrollView.FOCUS_DOWN);
						print("-------------------------------------");
						scrollview.fullScroll(ScrollView.FOCUS_DOWN);
					}});

				while( !done )
				{
					final String line = inFromServer.readLine();
					if( line.equals("kill"))
					{
						done = true;
					}
					else
					{
						runOnUiThread( new Runnable() {
							@Override
							public void run() {print(line);
							scrollview.fullScroll(ScrollView.FOCUS_DOWN);}});
					}
				}

				runOnUiThread( new Runnable() {
					@Override
					public void run() {
						print("-------------------------------------");
						scrollview.fullScroll(ScrollView.FOCUS_DOWN);
						print("Finished...");
						scrollview.fullScroll(ScrollView.FOCUS_DOWN);
					}});

				fileInputStream.close();
				outToServer.flush(); 
				outToServer.close();
				inFromServer.close();
				connection.close();
				

			}
			catch (UnknownHostException e) 
			{
				runOnUiThread( new Runnable() {
					@Override
					public void run() {print("Error trying to connect to server...");}});
				e.printStackTrace();
			} 
			catch (IOException e) 
			{
				runOnUiThread( new Runnable() {
					@Override
					public void run() {print("Error trying to connect to server...");}});
				e.printStackTrace();
			}
			return null;
		}
		
	}
	public String timeStamp()
	{
		return System.currentTimeMillis()+": ";
	}
	
	@Override
	protected void onDestroy() {
		try {
			logWriter.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		super.onDestroy();
	}
}