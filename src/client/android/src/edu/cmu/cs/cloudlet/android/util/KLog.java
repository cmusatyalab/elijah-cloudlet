package edu.cmu.cs.cloudlet.android.util;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Writer;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.util.Log;

public class KLog {
	public static final String TAG = "krha";
	private static final SimpleDateFormat TIMESTAMP_FMT = new SimpleDateFormat("[HH:mm:ss]");
	private static Writer mWriter;
	
	static{
		File basePath = CloudletEnv.instance().getFilePath(CloudletEnv.ROOT_DIR);
		SimpleDateFormat df = new SimpleDateFormat("yyyyMMdd-hhmmssSSS");
		File f = new File(basePath + df.format(new Date()) + ".log");
		String mPath = f.getAbsolutePath();
		try {
			mWriter = new BufferedWriter(new FileWriter(mPath), 2048);
			println("Start logging.");
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public static void println(String message) {		
		Log.d(TAG, message);
		if(mWriter != null){
			try {
				mWriter.write(TIMESTAMP_FMT.format(new Date()));
				mWriter.write(message);
				mWriter.write('\n');
				mWriter.flush();	
			} catch (IOException e) {
				Log.e(TAG, e.toString());
			}
		}
	}
	
	public static void printErr(String message) {
		Log.e(TAG, message);
		StackTraceElement[] stackTraces = Thread.currentThread().getStackTrace();
		for(int i = 0; i < stackTraces.length; i++){
		    StackTraceElement stackTraceElement = stackTraces[i];
			Log.e(TAG, stackTraceElement.toString());
		}
		if(mWriter != null){
			try {
				mWriter.write("[ERROR] " + TIMESTAMP_FMT.format(new Date()));
				mWriter.write(message);
				mWriter.write('\n');
				mWriter.flush();
			} catch (IOException e) {
				Log.e(TAG, e.toString());
			}
		}
	}

	public static void close() throws IOException {
		mWriter.close();
	}
}
