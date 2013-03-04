package edu.cmu.cs.cloudlet.application.esvmtrainer;

import java.io.File;
import java.io.FileFilter;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.List;

import edu.cmu.cs.cloudlet.application.esvmtrainer.util.AnnotationActivity;

import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.CursorLoader;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.database.Cursor;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.widget.Toast;

public class ESVMTrainActivity extends Activity {
	public static final int ACTION_TAKE_VIDEO = 100;
	public static final int ACTION_ANNOTATION = 0;
	public static final String LOG_TAG = "krha";

	// private static final String VIDEO_BASE_DIR = Environment
	// .getExternalStorageDirectory() + File.separator + "ESVM";
	public static final String VIDEO_BASE_DIR = "/sdcard/ESVM";
	public static final String VIDEO_TEST_DIR = "/sdcard/ESVM/VID_20130225_215420/";
	protected ProgressDialog progDialog = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_esvmtrain);
		this.progDialog = new ProgressDialog(this);
		this.progDialog.setMessage("Processing..");

		// Buttons
		findViewById(R.id.videoRecording).setOnClickListener(clickListener);
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.esvmtrain, menu);
		return true;
	}

	View.OnClickListener clickListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {

			if (v.getId() == R.id.videoRecording) {
				startTakingVideo();
			}
		}
	};

	/*
	 * Taking Video
	 */
	public void startTakingVideo() {
		// Launch an intent to capture video from MediaStore
		Intent takeVideoIntent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
		startActivityForResult(takeVideoIntent, ACTION_TAKE_VIDEO);
	}

	public static boolean isIntentAvailable(Context context, String action) {
		final PackageManager packageManager = context.getPackageManager();
		final Intent intent = new Intent(action);
		List<ResolveInfo> list = packageManager.queryIntentActivities(intent,
				PackageManager.MATCH_DEFAULT_ONLY);
		return list.size() > 0;
	}

	public void onActivityResult(int requestCode, int resultCode, Intent intent) {
		if (requestCode == ACTION_TAKE_VIDEO) {
			if (resultCode == RESULT_OK) {
				Uri videoUri = intent.getData();
				String filePath = getRealPathFromURI(videoUri);
				String frameDir = ESVMTrainActivity.VIDEO_BASE_DIR
						+ File.separator
						+ new File(filePath).getName().replaceFirst(
								"[.][^.]+$", "");
				File frameDirFile = new File(frameDir);

				this.progDialog.show();
				FrameExtractor frameExtractor = new FrameExtractor(this,
						videoUri, frameDirFile, frameExtractHandler);
				frameExtractor.start();
			}
		} else if (requestCode == ACTION_ANNOTATION) {
			if (resultCode == RESULT_OK) {

			}
		}
	}

	private String getRealPathFromURI(Uri contentUri) {
		String[] proj = { MediaStore.Images.Media.DATA };
		CursorLoader loader = new CursorLoader(this, contentUri, proj, null,
				null, null);
		Cursor cursor = loader.loadInBackground();
		int column_index = cursor
				.getColumnIndexOrThrow(MediaStore.Images.Media.DATA);
		cursor.moveToFirst();
		return cursor.getString(column_index);
	}

	Handler frameExtractHandler = new Handler() {
		public void handleMessage(Message msg) {
			if (progDialog != null && progDialog.isShowing()) {
				progDialog.dismiss();
			}
			
			if (msg.what == FrameExtractor.EXTRACT_SUCCESS) {
				// Start user annotation
				File destDir = (File) msg.obj;
				Intent cropIntent = new Intent(ESVMTrainActivity.this,
						AnnotationActivity.class);
				cropIntent.putExtra(AnnotationActivity.INTENT_IMAGE_DIR,
						destDir.getAbsolutePath());
				startActivityForResult(cropIntent, ACTION_ANNOTATION);
			} else if (msg.what == FrameExtractor.EXTRACT_FAIL) {
				// String msg = "Cannot extract frames from video :"
				// + e.getMessage();
				Toast.makeText(getApplicationContext(), "FAILED",
						Toast.LENGTH_LONG).show();
			}
		}
	};
}
