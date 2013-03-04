package edu.cmu.cs.cloudlet.application.esvmtrainer;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Random;

import android.content.Context;
import android.graphics.Bitmap;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.Toast;

public class FrameExtractor extends Thread{
	protected static final int EXTRACT_SUCCESS = 1;
	protected static final int EXTRACT_FAIL = 0;

	private static final int FRAME_PERIOD = 1000; // every 100 ms
	protected Context context = null;
	protected Uri videoURI = null;
	protected File destDir = null;
	protected Handler handler = null;

	public FrameExtractor(Context context, Uri videoURI, File destDir, Handler handler) {
		this.context = context;
		this.videoURI = videoURI;
		this.destDir = destDir;
		this.handler = handler;
	}
	
	public void run(){
		Message msg = Message.obtain();
		try{
			this.saveFrames();
			msg.what = FrameExtractor.EXTRACT_SUCCESS;
			msg.obj = this.destDir;
			this.handler.sendMessage(msg);
		}catch(IOException e){
			String errMsg = "" + e;
			msg.what = FrameExtractor.EXTRACT_FAIL;
			msg.obj = errMsg;
			this.handler.sendMessage(msg);
		}finally{
			this.close();
		}
	}

	protected void saveFrames() throws IOException{
		// Check directory
		if (!this.destDir.exists()) {
			this.destDir.mkdirs();
		}else{
			if (!this.destDir.isDirectory()){
				throw new IOException(this.destDir + " is not a directory");
			}
		}
		
		MediaMetadataRetriever retriever = new MediaMetadataRetriever();
		retriever.setDataSource(this.context, this.videoURI);

		String milliString = retriever
				.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION);
		long millis = Long.parseLong(milliString);
		for (int i = 0; i < millis; i += FRAME_PERIOD) {
			Bitmap bitmap = retriever.getFrameAtTime(i * 1000,
					MediaMetadataRetriever.OPTION_CLOSEST_SYNC);
			File savePath = new File(this.destDir + File.separator + "frame_" + i + ".jpg");
			this.saveBitmap(savePath, bitmap);
			Log.d(ESVMTrainActivity.LOG_TAG, "Save frame at : (" + savePath + ")");
		}
	}

	protected void saveBitmap(File savePath, Bitmap b) throws IOException{
		ByteArrayOutputStream bytes = new ByteArrayOutputStream();
		b.compress(Bitmap.CompressFormat.JPEG, 80, bytes);
		savePath.createNewFile();
		FileOutputStream fo = new FileOutputStream(savePath);
		fo.write(bytes.toByteArray());
		fo.flush();
		fo.close();
	}

	public void close() {
	}

}
