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
import edu.cmu.cs.cloudlet.application.esvmtrainer.util.ImageAdapter;

import android.app.Activity;
import android.graphics.drawable.Drawable;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.provider.MediaStore;
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
	
	private static final String META_FILENAME = "meta.json";
	private static final String META_JSON_ANNOTATION_KEY = "annotation_info";
	private static final String META_JSON_ANNOTATION_FILENAME = "filename";
	private static final String META_JSON_ANNOTATION_INFO_LEFT = "left";
	private static final String META_JSON_ANNOTATION_INFO_RIGHT = "right";
	private static final String META_JSON_ANNOTATION_INFO_TOP = "top";
	private static final String META_JSON_ANNOTATION_INFO_BOTTOM = "bottom";

	protected File iamgeSourceDir = null;
	protected ImageAdapter imageAdapter = null;
	LinkedList<AnnotatedImageDS> allImageList = null;
	private File currentProcessingImage = null;
	private File metaFile = null;

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
		
		try {
			updateMetaFile(allImageList, META_FILENAME);
		} catch (IOException e) {
			e.printStackTrace();
		} catch (JSONException e) {
			e.printStackTrace();
		}

		imageAdapter = new ImageAdapter(this, allImageList);
		GridView gridView = (GridView) findViewById(R.id.grid_view);
		gridView.setAdapter(imageAdapter);
		gridView.setOnItemClickListener(clickListener);
		;

		// start training button
		findViewById(R.id.startTraining).setOnClickListener(startTrainingListener);
	}

	private void updateMetaFile(LinkedList<AnnotatedImageDS> imageList, String metaFilename) throws IOException, JSONException {
		this.metaFile = new File(metaFilename);
		if (metaFile.isFile() == true) {
			// read meta json file
			/*
			 * JSON meta file structure (example) 
			 * {
			 *   "annotation_info": [
			 *     {"filename": "1.jpg", "left":10, "right": 20, "top" : 40, "bottom": 50}
			 *     ...
			 *     {"filename": "9.jpg", "left":10, "right": 20, "top" : 40, "bottom": 50} 
			 *   ]
			 * }
			 */
			BufferedReader br = new BufferedReader(new FileReader(metaFile));
			StringBuilder sb = new StringBuilder();
			String line = "";
			while ((line = br.readLine()) != null) {
				sb.append(line + "\n");
			}
			br.close();
			String jsonString = sb.toString();
			if (jsonString.length() != 0) {
				JSONObject rootObj = new JSONObject(jsonString);
				JSONArray annotationArray = rootObj.getJSONArray(META_JSON_ANNOTATION_KEY);
				for(int i = 0; i < annotationArray.length(); i++){
					JSONObject eachObj = annotationArray.getJSONObject(i);
					String filename = eachObj.getString(META_JSON_ANNOTATION_FILENAME);
					int left = eachObj.getInt(META_JSON_ANNOTATION_INFO_LEFT);
					int right = eachObj.getInt(META_JSON_ANNOTATION_INFO_RIGHT);
					int top = eachObj.getInt(META_JSON_ANNOTATION_INFO_TOP);
					int bottom = eachObj.getInt(META_JSON_ANNOTATION_INFO_BOTTOM);
					Rect box = new Rect(left, top, right, bottom);
					AnnotatedImageDS image = findImageByName(imageList, filename);
					if (image != null){
						image.setCropRect(box);
					}
				}
			}
		} else {
			try {
				metaFile.createNewFile();
			} catch (IOException e) {
				e.printStackTrace();
			}
			return;
		}

	}

	private void updateMetaFile(AnnotatedImageDS processedImage) throws JSONException, IOException {		
	}

	private AnnotatedImageDS findImageByName(LinkedList<AnnotatedImageDS> imageList, String filename) {
		for(int i = 0; i < imageList.size(); i++){
			AnnotatedImageDS image = imageList.get(i);
			if (image.getOriginalFile().getName().equals(filename)){
				return image;
			}
		}
		return null;
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
					updateMetaFile(processedImage);
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
			currentProcessingImage = cropImageFile;
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
}
