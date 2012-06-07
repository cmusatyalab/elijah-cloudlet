package edu.cmu.cs.cloudlet.android.application.graphics;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.opengl.GLU;
import android.util.Log;

public class PointRenderer implements GLSurfaceView.Renderer {
	
	private Cube cube;
	private Graphics graphics;
	private boolean is3D;
	
	/* Rotation values */
	protected float xrot = 19.0f;					//X Rotation
	protected float yrot = -64.0f;					//Y Rotation
	
	/* Rotation speed values */
	private float xspeed;				//X Rotation Speed ( NEW )
	private float yspeed;				//Y Rotation Speed ( NEW )
	
	protected float z = -5.0f;			//Depth Into The Screen ( NEW )
	
	/*
	 * These variables store the previous X and Y
	 * values as well as a fix touch scale factor.
	 * These are necessary for the rotation transformation
	 * added to this lesson, based on the screen touches. ( NEW )
	 */
	protected float oldX;
    protected float oldY;
	private final float TOUCH_SCALE = 0.2f;		//Proved to be good for normal rotation ( NEW )
	
	/** The Activity Context */
	private Context context;
	
	public PointRenderer(Graphics graphics){
		this.graphics = graphics;
		this.is3D = graphics.is3D();
		if(this.is3D)
			cube = new Cube();
	}
	
	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		if(is3D){
			gl.glDisable(GL10.GL_DITHER);				//Disable dithering ( NEW )
		}
		gl.glShadeModel(GL10.GL_SMOOTH); 			//Enable Smooth Shading
		gl.glClearColor(0.0f, 0.0f, 0.0f, 0.5f); 	//Black Background
		gl.glClearDepthf(1.0f); 					//Depth Buffer Setup
		gl.glEnable(GL10.GL_DEPTH_TEST); 			//Enables Depth Testing
		gl.glDepthFunc(GL10.GL_LEQUAL); 			//The Type Of Depth Testing To Do

		//Really Nice Perspective Calculations
//		gl.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT, GL10.GL_FASTEST);
		gl.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT, GL10.GL_NICEST);
	}
	
	@Override
	public void onDrawFrame(GL10 gl) {
		gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);	
		gl.glLoadIdentity();
		
		if(is3D){
			/************ DRAW THE CUBE **************/
			gl.glTranslatef(0.0f, 0.0f, z);			//Move z units into the screen
			gl.glScalef(0.8f, 0.8f, 0.8f); 			//Scale the Cube to 80 percent, otherwise it would be too large for the screen
			
			//Rotate around the axis based on the rotation matrix (rotation, x, y, z)
			gl.glRotatef(xrot, 1.0f, 0.0f, 0.0f);	//X
			gl.glRotatef(yrot, 0.0f, 1.0f, 0.0f);	//Y
			
			//Draw the cube
			cube.draw(gl);					
			
			/************ DRAW THE POINTS ************/
			gl.glLoadIdentity();					//Reset The Current Modelview Matrix
			gl.glTranslatef(0.0f, 0.0f, z);			//Move z units into the screen
			gl.glScalef(0.8f, 0.8f, 0.8f); 			//Scale the Cube to 80 percent, otherwise it would be too large for the screen
			
			//Rotate around the axis based on the rotation matrix (rotation, x, y, z)
			gl.glRotatef(xrot, 1.0f, 0.0f, 0.0f);	//X
			gl.glRotatef(yrot, 0.0f, 1.0f, 0.0f);	//Y
		
			graphics.draw(gl);					//Draw the graphic	
			
			//Change rotation factors
			xrot += xspeed;
			yrot += yspeed;
		}
		//The graphic is 2D
		else{ 
			graphics.draw(gl);
		}
	}

	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		if(is3D){
			if(height == 0) { 						//Prevent A Divide By Zero By
				height = 1; 						//Making Height Equal One
			}
	
			gl.glViewport(0, 0, width, height); 	//Reset The Current Viewport
			gl.glMatrixMode(GL10.GL_PROJECTION); 	//Select The Projection Matrix
			gl.glLoadIdentity(); 					//Reset The Projection Matrix
	
			//Calculate The Aspect Ratio Of The Window
			GLU.gluPerspective(gl, 45.0f, (float)width / (float)height, 0.1f, 100.0f);
	
			gl.glMatrixMode(GL10.GL_MODELVIEW); 	//Select The Modelview Matrix
			gl.glLoadIdentity(); 					//Reset The Modelview Matrix
		}
		//Graphic is 2D
		else{
			gl.glViewport(0, 0, width, height);
		}
	}
	
}
