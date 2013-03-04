package edu.cmu.cs.cloudlet.application.esvmtrainer.util;

import java.io.File;

import android.graphics.Rect;

public class CropImageDS {
	protected File originalFile = null;
	protected File cropFile = null;
	protected boolean isCropped = false;
	protected Rect cropRect = null;
	
	public CropImageDS(File originalFile){
		this.originalFile = originalFile;
		this.cropFile = new File(originalFile.getAbsoluteFile() + ".crop");
		this.isCropped = false;
	}

	public File getCropFile() {
		return this.cropFile;
	}

	public boolean isCrop() {
		return this.isCropped;
	}
	public void setCrop(boolean isCrop) {
		this.isCropped = isCrop;
	}

	public File getOriginalFile() {
		return this.originalFile;
	}

	public void setCropRect(Rect cropRect) {
		this.cropRect = cropRect;
	}
	
	public Rect getCropRect(){
		return this.cropRect;
	}
}
