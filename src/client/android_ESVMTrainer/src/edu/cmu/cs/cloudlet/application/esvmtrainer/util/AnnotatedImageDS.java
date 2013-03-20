package edu.cmu.cs.cloudlet.application.esvmtrainer.util;

import java.io.File;

import edu.cmu.cs.cloudlet.application.esvmtrainer.AnnotationActivity;

import android.graphics.Rect;

public class AnnotatedImageDS {
	protected File originalFile = null;
	protected File croppedFile = null;
	
	protected boolean isAnnotated = false;
	protected Rect cropRect = null;
	
	public AnnotatedImageDS(File originalFile){
		this.originalFile = originalFile;
		this.croppedFile = new File(originalFile.getAbsoluteFile() + AnnotationActivity.CROP_EXT);
		this.isAnnotated = false;
	}

	public File getCropFile() {
		return this.croppedFile;
	}

	public boolean isCrop() {
		return this.isAnnotated;
	}

	public File getOriginalFile() {
		return this.originalFile;
	}

	public void setCropRect(Rect cropRect) {
		this.cropRect = cropRect;
		this.isAnnotated = true;
	}
	
	public Rect getCropRect(){
		return this.cropRect;
	}
}
