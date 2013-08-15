package edu.cmu.cs.cloudlet.application.esvmtrainer;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.List;

import edu.cmu.cs.cloudlet.application.esvmtrainer.util.FileDialog;
import edu.cmu.cs.cloudlet.application.esvmtrainer.util.FileDialog.FileSelectedListener;

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
	public static final int ACTION_ANNOTATION = 110;
	public static final String LOG_TAG = "krha";

	private static final String VIDEO_BASE_DIR = Environment
			.getExternalStorageDirectory() + File.separator + "ESVM";
	public static final String VIDEO_TEST_DIR = "/sdcard/ESVM/20130328_154219/";
	public static final String VIDEO_TEST_VIDEO_FILE = "/sdcard/ESVM/20130328_154219/20130328_154219.mp4";
	protected ProgressDialog progDialog = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_esvmtrain);
		this.progDialog = new ProgressDialog(this);
		this.progDialog.setMessage("Extracting frames..");

		// Buttons
		findViewById(R.id.videoRecording).setOnClickListener(clickListener);
		findViewById(R.id.videoSelecting).setOnClickListener(clickListener);
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
			} else if (v.getId() == R.id.videoSelecting) {
				File mPath = new File(ESVMTrainActivity.VIDEO_BASE_DIR);
				FileDialog fileDialog = new FileDialog(ESVMTrainActivity.this,
						mPath);
				fileDialog.setFileEndsWith(".mp4");
				fileDialog.addFileListener(dirSelctionListenser);
				fileDialog.showDialog();
			}
		}
	};

	FileSelectedListener dirSelctionListenser = new FileSelectedListener() {
		public void fileSelected(File file) {
			Log.d(LOG_TAG, "selected file " + file.toString());
			String destDir = null;
			if (file.isDirectory())
				destDir = file.getAbsolutePath();
			else
				destDir = file.getParentFile().getAbsolutePath();
			Intent cropIntent = new Intent(ESVMTrainActivity.this,
					AnnotationActivity.class);
			cropIntent.putExtra(AnnotationActivity.INTENT_ARGS_IMAGE_DIR,
					destDir);
			cropIntent.putExtra(AnnotationActivity.INTENT_ARGS_VIDEO_FILE, file);
			startActivityForResult(cropIntent, ACTION_ANNOTATION);
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
				String originalFilePath = getRealPathFromURI(videoUri);
				startFrameExtractor(originalFilePath);
			}
		} else if (requestCode == ACTION_ANNOTATION) {
			if (resultCode == RESULT_OK) {

			}
		}
	}

	private void startFrameExtractor(String originalFilePath) {
		String[] fileNameSplit = originalFilePath.split(File.separator);
		String fileName = fileNameSplit[fileNameSplit.length - 1];
		String videoSavingDir = ESVMTrainActivity.VIDEO_BASE_DIR
				+ File.separator
				+ new File(originalFilePath).getName().replaceFirst(
						"[.][^.]+$", "");
		File frameDirFile = new File(videoSavingDir);
		frameDirFile.mkdirs();

		// copy video file
		String newFilePath = videoSavingDir + File.separator + fileName;
		try {
			copyFile(originalFilePath, newFilePath);
		} catch (IOException e) {
			e.printStackTrace();

			String message = "Cannot copy video file from " + originalFilePath
					+ " to " + newFilePath;
			showAlertMessage(message);
			return;
		}

		FrameExtractor frameExtractor = new FrameExtractor(this, new File(
				newFilePath), frameDirFile, frameExtractHandler);
		frameExtractor.start();
		this.progDialog.show();

	}

	private void showAlertMessage(String message) {
		AlertDialog.Builder ab = new AlertDialog.Builder(ESVMTrainActivity.this);
		ab.setTitle("Failed");
		ab.setMessage(message);
		ab.setIcon(R.drawable.ic_launcher);
		ab.setPositiveButton("Confirm", null);
		ab.show();
	}

	private void copyFile(String originalFilePath, String newFilePath)
			throws IOException {
		File newFile = new File(newFilePath);
		newFile.createNewFile();
		InputStream input = new FileInputStream(originalFilePath);
		FileOutputStream fo = new FileOutputStream(newFile);
		byte[] buffer = new byte[1024 * 1024];
		int bytesRead = 0;
		while ((bytesRead = input.read(buffer)) > 0) {
			fo.write(buffer, 0, bytesRead);
		}
		input.close();
		fo.close();
	}

	public String getRealPathFromURI(Uri contentUri) {
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
				Bundle data = msg.getData();
				String destDir = data
						.getString(AnnotationActivity.INTENT_ARGS_IMAGE_DIR);
				String videoFile = data
						.getString(AnnotationActivity.INTENT_ARGS_VIDEO_FILE);
				Intent cropIntent = new Intent(ESVMTrainActivity.this,
						AnnotationActivity.class);
				cropIntent.putExtra(AnnotationActivity.INTENT_ARGS_IMAGE_DIR,
						destDir);
				cropIntent.putExtra(AnnotationActivity.INTENT_ARGS_VIDEO_FILE,
						videoFile);
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
