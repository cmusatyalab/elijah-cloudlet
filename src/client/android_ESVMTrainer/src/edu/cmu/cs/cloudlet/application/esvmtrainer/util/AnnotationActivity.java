package edu.cmu.cs.cloudlet.application.esvmtrainer.util;

import java.io.File;
import java.io.FilenameFilter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;

import edu.cmu.cs.cloudlet.application.esvmtrainer.ESVMTrainActivity;
import edu.cmu.cs.cloudlet.application.esvmtrainer.R;
import edu.cmu.cs.cloudlet.application.esvmtrainer.cropimage.CropImage;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.GridView;
import android.widget.Toast;

public class AnnotationActivity extends Activity {
	public static final String INTENT_IMAGE_DIR = "imageDirectory";
	public static final int PIC_CROP = 1121312;
	private static final String TEMP_PHOTO_FILE = "crop_image.jpg";

	protected File sourceDir = null;
	protected ImageAdapter imageAdapter = null;
	LinkedList<CropImageDS> cropList = null;
	private File currentCropImage = null;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_annotation_main);

		Bundle extras = getIntent().getExtras();
		if (extras != null && extras.getString(INTENT_IMAGE_DIR) != null) {
			this.sourceDir = new File(extras.getString(INTENT_IMAGE_DIR));
		} else {
			this.sourceDir = new File(ESVMTrainActivity.VIDEO_TEST_DIR);
		}

		cropList = new LinkedList<CropImageDS>();
		File[] imageFiles = this.sourceDir.listFiles(new FilenameFilter() {
			@Override
			public boolean accept(File dir, String filename) {
				if (filename.endsWith(".jpg"))
					return true;
				return false;
			}
		});
		for (File originalFile : imageFiles) {
			cropList.add(new CropImageDS(originalFile));
		}

		imageAdapter = new ImageAdapter(this, cropList);
		GridView gridView = (GridView) findViewById(R.id.grid_view);
		gridView.setAdapter(imageAdapter);
		gridView.setOnItemClickListener(clickListener);
		
		// start training button
		findViewById(R.id.startTraining).setOnClickListener(startTrainingListener);
	}

	private void starTraining() {
		
	}
	
	protected void onActivityResult(int requestCode, int resultCode, Intent data) {
		if (resultCode == RESULT_OK) {
			if (requestCode == PIC_CROP) {
				Bundle extras = data.getExtras();
				CropImageDS retImage = null;
				for (CropImageDS cropimage : this.cropList) {
					if (cropimage.getCropFile() == this.currentCropImage){
						retImage = cropimage;
						break;
					}
				}
				if (retImage != null){
					retImage.setCrop(true);
					int cropLeft = extras.getInt("crop-left");
					int cropRight = extras.getInt("crop-right");
					int cropBottom = extras.getInt("crop-bottom");
					int cropTop = extras.getInt("crop-top");
					Rect cropRect = new Rect(cropLeft, cropTop, cropRight, cropBottom);
					retImage.setCropRect(cropRect);
					
					Toast.makeText(AnnotationActivity.this,
							"SUCCESS", Toast.LENGTH_SHORT).show();
					this.imageAdapter.notifyDataSetChanged();
				}else{
					Toast.makeText(AnnotationActivity.this,
							"FAILED", Toast.LENGTH_SHORT).show();
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
		public void onItemClick(AdapterView<?> parent, View v, int position,
				long id) {

			CropImageDS image = (CropImageDS) imageAdapter.getItem(position);
			File cropImageFile = image.getCropFile();
			currentCropImage = cropImageFile;
			Uri picUri = Uri.fromFile(image.getOriginalFile());
			try {


				Intent cropIntent = new Intent(AnnotationActivity.this,
						CropImage.class);
				cropIntent.putExtra("image-path", picUri.getPath());
				cropIntent.putExtra("scale", true);
				cropIntent.putExtra(MediaStore.EXTRA_OUTPUT,
						Uri.fromFile(cropImageFile));

				cropIntent.putExtra("outputFormat",
						Bitmap.CompressFormat.JPEG.toString());
				startActivityForResult(cropIntent, PIC_CROP);
				
			} catch (ActivityNotFoundException e) {
				// display an error message
				String errorMessage = "Whoops - your device doesn't support the crop action!";
				Toast toast = Toast.makeText(AnnotationActivity.this,
						errorMessage, Toast.LENGTH_SHORT);
				toast.show();
			}
		}
	};
}
