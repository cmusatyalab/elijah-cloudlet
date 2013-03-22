package edu.cmu.cs.cloudlet.application.esvmtrainer;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.LinkedList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import edu.cmu.cs.cloudlet.application.esvmtrainer.R;
import edu.cmu.cs.cloudlet.application.esvmtrainer.cropimage.CropImage;
import edu.cmu.cs.cloudlet.application.esvmtrainer.util.AnnotatedImageDS;
import edu.cmu.cs.cloudlet.application.esvmtrainer.util.AnnotationDBHelper;
import edu.cmu.cs.cloudlet.application.esvmtrainer.util.ImageAdapter;

import android.app.Activity;
import android.app.AlertDialog;
import android.graphics.drawable.Drawable;
import android.content.ActivityNotFoundException;
import android.content.ContentValues;
import android.content.DialogInterface;
import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.view.KeyEvent;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.Toast;

public class AnnotationActivity extends Activity {
	public static final int PIC_CROP = 1121312;
	public static Drawable CHECKMARK_DONE;

	public static final String INTENT_ARGS_IMAGE_DIR = "imageDirectory";
	public static final String CROP_EXT = ".crop";

	protected File iamgeSourceDir = null;
	protected ImageAdapter imageAdapter = null;
	LinkedList<AnnotatedImageDS> allImageList = null;
	private File currentProcessingImage = null;
	private AnnotationDBHelper dbHelper = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_annotation_main);

		this.CHECKMARK_DONE = getResources().getDrawable(R.drawable.checkmark_done);

		Bundle extras = getIntent().getExtras();
		if (extras != null && extras.getString(INTENT_ARGS_IMAGE_DIR) != null) {
			this.iamgeSourceDir = new File(extras.getString(INTENT_ARGS_IMAGE_DIR));
		} else {
			this.iamgeSourceDir = new File(ESVMTrainActivity.VIDEO_TEST_DIR);
		}

		allImageList = new LinkedList<AnnotatedImageDS>();
		File[] imageFiles = this.iamgeSourceDir.listFiles(new FilenameFilter() {
			@Override
			public boolean accept(File dir, String filename) {
				if (filename.endsWith(".jpg"))
					return true;
				return false;
			}
		});
		for (File originalFile : imageFiles) {
			allImageList.add(new AnnotatedImageDS(originalFile));
		}
		this.dbHelper = new AnnotationDBHelper(this);
		this.dbHelper.updateImageFromDB(allImageList);

		imageAdapter = new ImageAdapter(this, allImageList);
		GridView gridView = (GridView) findViewById(R.id.grid_view);
		gridView.setAdapter(imageAdapter);
		gridView.setOnItemClickListener(clickListener);
		;

		// start training button
		findViewById(R.id.startTraining).setOnClickListener(startTrainingListener);
	}


	private void starTraining() {

	}

	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (resultCode == RESULT_OK) {
			if (requestCode == PIC_CROP) {
				Bundle extras = data.getExtras();
				AnnotatedImageDS processedImage = null;
				for (AnnotatedImageDS image : this.allImageList) {
					if (image.getOriginalFile() == this.currentProcessingImage) {
						processedImage = image;
						break;
					}
				}

				if (processedImage != null) {
					// save annotation information to DS
					int cropLeft = extras.getInt("crop-left");
					int cropRight = extras.getInt("crop-right");
					int cropBottom = extras.getInt("crop-bottom");
					int cropTop = extras.getInt("crop-top");
					Rect cropRect = new Rect(cropLeft, cropTop, cropRight, cropBottom);
					processedImage.setCropRect(cropRect);
					this.dbHelper.updateDB(processedImage);
					Toast.makeText(AnnotationActivity.this, "SUCCESS", Toast.LENGTH_SHORT).show();

					// update image to cropped region
					this.imageAdapter.notifyDataSetChanged();
				} else {
					Toast.makeText(AnnotationActivity.this, "FAILED", Toast.LENGTH_SHORT).show();
				}
			}
		}
	}

	View.OnClickListener startTrainingListener = new View.OnClickListener() {
		@Override
		public void onClick(View v) {
			if (v.getId() == R.id.startTraining) {
				starTraining();
			}
		}
	};

	OnItemClickListener clickListener = new OnItemClickListener() {
		@Override
		public void onItemClick(AdapterView<?> parent, View v, int position, long id) {

			AnnotatedImageDS image = (AnnotatedImageDS) imageAdapter.getItem(position);
			File cropImageFile = image.getCropFile();
			currentProcessingImage = image.getOriginalFile();
			Uri picUri = Uri.fromFile(image.getOriginalFile());
			try {

				Intent cropIntent = new Intent(AnnotationActivity.this, CropImage.class);
				cropIntent.putExtra("image-path", picUri.getPath());
				cropIntent.putExtra("scale", true);
				cropIntent.putExtra(MediaStore.EXTRA_OUTPUT, Uri.fromFile(cropImageFile));

				cropIntent.putExtra("outputFormat", Bitmap.CompressFormat.JPEG.toString());
				startActivityForResult(cropIntent, PIC_CROP);

			} catch (ActivityNotFoundException e) {
				// display an error message
				String errorMessage = "Whoops - your device doesn't support the crop action!";
				Toast toast = Toast.makeText(AnnotationActivity.this, errorMessage, Toast.LENGTH_SHORT);
				toast.show();
			}
		}
	};
	
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			this.close();			
			/*
			new AlertDialog.Builder(AnnotationActivity.this).setTitle("Exit").setMessage("Finish Application")
					.setPositiveButton("Confirm", new DialogInterface.OnClickListener() {
						public void onClick(DialogInterface dialog, int which) {
							moveTaskToBack(true);
							finish();
						}
					}).setNegativeButton("Cancel", null).show();
			*/
		}
		return super.onKeyDown(keyCode, event);
	}


	private void close() {
		if (this.dbHelper != null){
			this.dbHelper.close();
			this.dbHelper = null;
		}
	}

}