////////////////////////////////////////////////////////////////////////////
//	File:		GlobalUtil.cpp
//	Author:		Changchang Wu
//	Description : Global Utility class for SiftGPU
//
//
//
//	Copyright (c) 2007 University of North Carolina at Chapel Hill
//	All Rights Reserved
//
//	Permission to use, copy, modify and distribute this software and its
//	documentation for educational, research and non-profit purposes, without
//	fee, and without a written agreement is hereby granted, provided that the
//	above copyright notice and the following paragraph appear in all copies.
//	
//	The University of North Carolina at Chapel Hill make no representations
//	about the suitability of this software for any purpose. It is provided
//	'as is' without express or implied warranty. 
//
//	Please send BUG REPORTS to ccwu@cs.unc.edu
//
////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
using std::cout;

#include "GL/glew.h"
#ifdef __APPLE__
	#include "GLUT/glut.h"
#else
	#include "GL/glut.h"
#endif

//#define HIGH_RESOLUTION_TIME

#include "GlobalUtil.h"

#if defined(_WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <mmsystem.h>
#else
	#include <sys/time.h>
#endif



//
int GlobalParam::		_verbose =  1;   
int	GlobalParam::       _timingS = 1;  //pint out information of each step
int	GlobalParam::       _timingO = 0;  //print out information of each octave
int	GlobalParam::       _timingL = 0;	//print out information of each level
GLuint GlobalParam::	_texTarget = GL_TEXTURE_RECTANGLE_ARB; //only this one is supported
GLuint GlobalParam::	_iTexFormat =GL_RGBA32F_ARB;	//or GL_RGBA16F_ARB
int	GlobalParam::		_debug = 0;		//enable debug code?
int	GlobalParam::		_usePackedTex = 1;//packed implementation
int GlobalParam::		_BetaFilter = 0;
int	GlobalParam::		_UseGLSL = 1;		//use GLSL or CG
int	GlobalParam::		_UseCUDA = 0;
int GlobalParam::		_MaxFilterWidth = -1;	//maximum filter width, use when GPU is not good enough
float GlobalParam::     _FilterWidthFactor	= 4.0f;	//the filter size will be _FilterWidthFactor*sigma*2+1
float GlobalParam::     _DescriptorWindowFactor = 3.0f; //descriptor sampling window factor
int GlobalParam::		_SubpixelLocalization = 1; //sub-pixel and sub-scale localization 	
int	GlobalParam::       _MaxOrientation = 2;	//whether we find multiple orientations for each feature 
int	GlobalParam::       _OrientationPack2 = 0;  //use one float to store two orientations
float GlobalParam::		_MaxFeaturePercent = 0.005f;//at most 0.005 of all pixels
int	GlobalParam::		_MaxLevelFeatureNum = 4096; //maximum number of features of a level
int GlobalParam::		_FeatureTexBlock = 4; //feature texture storagte alignment
int	GlobalParam::		_NarrowFeatureTex = 0; 
//if _ForceTightPyramid is not 0, pyramid will be reallocated to fit the size of input images.
//otherwise, pyramid can be reused for smaller input images. 
int GlobalParam::		_ForceTightPyramid = 0;
//use gpu or cpu to generate feature list ...gpu is a little bit faster
int GlobalParam::		_ListGenGPU =	1;	
int	GlobalParam::       _ListGenSkipGPU = 6;  //how many levels are skipped on gpu
int GlobalParam::		_PreProcessOnCPU = 1; //convert rgb 2 intensity on gpu, down sample on GPU
//hardware parameter, automatically retrieved
int GlobalParam::		_texMaxDim = 2560;	//maximum working size, 3200 for packed
int	GlobalParam::		_texMaxDimGL = 0; 
int	GlobalParam::		_IsNvidia = 0;				//GPU vendor
int	GlobalParam::		_MaxDrawBuffers = 0;		//max draw buffer

//you can't change the following 2 values
//all other versions of code are now dropped
int GlobalParam::       _DescriptorPPR = 8;
int	GlobalParam::		_DescriptorPPT = 16;

//whether orientation/descriptor is supported by hardware
int	GlobalParam::		_SupportFP40 = 0;
int GlobalParam::		_SupportNVFloat = 0;
int GlobalParam::       _SupportTextureRG = 0;
int	GlobalParam::		_UseDynamicIndexing = 0; 
int GlobalParam::		_FullSupported = 1;

//when SiftGPUEX is used, display VBO generation is skipped
int GlobalParam::		_UseSiftGPUEX = 0;
int GlobalParam::		_InitPyramidWidth=0;
int GlobalParam::		_InitPyramidHeight=0;
int	GlobalParam::		_octave_min_default=0;
int	GlobalParam::		_octave_num_default=-1;
int	GlobalParam::		_GoodOpenGL = -1;
int	GlobalParam::		_FixedOrientation = 0;
int	GlobalParam::		_LoweOrigin = 0;
int	GlobalParam::       _NormalizedSIFT = 1;
int GlobalParam::       _BinarySIFT = 0;
int	GlobalParam::		_ExitAfterSIFT = 0; //exif after saving result
int	GlobalParam::		_KeepExtremumSign = 0; // if 1, scales of dog-minimum will be multiplied by -1
///
int	GlobalParam::		_ProcessOBO = 0;
int	GlobalParam::		_PreciseBorder = 1;

// parameter changing for better matching with Lowe's SIFT
int	GlobalParam::		_GradientLevelOffset = 1;  //2 in v 289 or older

float GlobalParam::		_OrientationWindowFactor = 2.0f;	// 1.0(-v292), 2(v293-), 
float GlobalParam::		_OrientationGaussianFactor = 1.5f;	// 4.5(-v292), 1.5(v293-)
float GlobalParam::     _MulitiOrientationThreshold = 0.8f;
///
int GlobalParam::		_UseFastMath = 1;
//int GlobalParam::		_UseInternalGLUT = 0;
////
ClockTimer GlobalUtil::	_globalTimer;


#ifdef _DEBUG
void GlobalUtil::CheckErrorsGL(const char* location)
{
	GLuint errnum;
	const char *errstr;
	while (errnum = glGetError()) 
	{
		errstr = (const char *)(gluErrorString(errnum));
		if(errstr) {
			std::cerr << errstr; 
		}
		else {
			std::cerr  << "Error " << errnum;
		}
		
		if(location) std::cerr  << " at " << location;		
		std::cerr  << "\n";
	}
	
	return;
}

#endif

void GlobalUtil::CleanupOpenGL()
{

}

int GlobalUtil::CreateWindowGLUT()
{
	static int _glut_init_called = 0;
	int glut_id = 0;
	//see if there is an existing window
	if(_glut_init_called) glut_id = glutGetWindow();

	//create one if no glut window exists
	if(glut_id == 0)
	{
		int argc = 1;
		char * argv[] = { "-iconic", 0};
		if(_glut_init_called == 0)
		{
			//make sure we only call glut once.
			glutInit(&argc, argv);
			glutInitDisplayMode (GLUT_RGBA ); 
			glutInitWindowSize (600,450);
			_glut_init_called = 1; 
		}
		glut_id = glutCreateWindow ("SIFT_GPU");
		glutHideWindow();
	}

	return glut_id; 
}

void GlobalUtil::SetTextureParameter()
{

	glTexParameteri (_texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri (_texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glTexParameteri(_texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(_texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

//if image need to be up sampled ..use this one

void GlobalUtil::SetTextureParameterUS()
{

	glTexParameteri (_texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
	glTexParameteri (_texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
	glTexParameteri(_texTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
	glTexParameteri(_texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}


void GlobalUtil::FitViewPort(int width, int height)
{
	GLint port[4];
	glGetIntegerv(GL_VIEWPORT, port);
	if(port[2] !=width || port[3] !=height)
	{
		glViewport(0, 0, width, height);      
		glMatrixMode(GL_PROJECTION);    
		glLoadIdentity();               
		glOrtho(0, width, 0, height,  0, 1);		
		glMatrixMode(GL_MODELVIEW);     
		glLoadIdentity();  
	}
}


bool GlobalUtil::CheckFramebufferStatus() {
    GLenum status;
    status=(GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            std::cerr<<("Framebuffer incomplete,incomplete attachment\n");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            std::cerr<<("Unsupported framebuffer format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            std::cerr<<("Framebuffer incomplete,missing attachment\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            std::cerr<<("Framebuffer incomplete,attached images must have same dimensions\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
             std::cerr<<("Framebuffer incomplete,attached images must have same format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            std::cerr<<("Framebuffer incomplete,missing draw buffer\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            std::cerr<<("Framebuffer incomplete,missing read buffer\n");
            return false;
    }
	return false;
}


int ClockTimer::ClockMS()
{
	static int    started = 0;
#ifdef _WIN32
	static int	tstart;
	if(started == 0)
	{
		tstart = timeGetTime();
		started = 1;
		return 0;
	}else
	{
		return timeGetTime() - tstart;
	}
#else
	static struct timeval tstart;
	if(started == 0) 
	{
		gettimeofday(&tstart, NULL);
		started = 1;
		return 0;
	}else
	{	
		struct timeval now;
		gettimeofday(&now, NULL) ;
		return (now.tv_usec - tstart.tv_usec)/1000 + (now.tv_sec - tstart.tv_sec) * 1000;
	}
#endif
}

double ClockTimer::CLOCK()
{
	return ClockMS() * 0.001;
}

void ClockTimer::InitHighResolution()
{
#if defined(_WIN32) 
	timeBeginPeriod(1);
#endif
}

void ClockTimer::StartTimer(const char* event, int verb)
{	
	strcpy(_current_event, event);
	_time_start = ClockMS();
	if(verb && GlobalUtil::_verbose)
	{
		std::cout<<"\n["<<_current_event<<"]:\tbegin ...\n";
	}
} 

void ClockTimer::StopTimer(int verb)
{
	_time_stop = ClockMS();
	if(verb && GlobalUtil::_verbose)
	{
		std::cout<<"["<<_current_event<<"]:\t"<<GetElapsedTime()<<"\n";
	}
}

float ClockTimer::GetElapsedTime()
{
	return (_time_stop - _time_start)  * 0.001f;
}


void GlobalUtil::InitGLParam()
{
	if(GlobalUtil::_GoodOpenGL != -1) return;

	glewInit();
	GLint value;
	const char * vendor = (const char * )glGetString(GL_VENDOR);
	if(vendor)
	{
		GlobalUtil::_IsNvidia  = strstr(vendor, "NVIDIA") !=NULL? 1:0;
		if(GlobalUtil::_verbose)std::cout<<"[GPU VENDOR]:\t"<< vendor <<"\n";
	}

	if (glewGetExtension("GL_ARB_fragment_shader")    != GL_TRUE ||
		glewGetExtension("GL_ARB_shader_objects")       != GL_TRUE ||
		glewGetExtension("GL_ARB_shading_language_100") != GL_TRUE)
	{
		std::cerr<<"Shader not supported by your hardware!\n";
		GlobalUtil::_GoodOpenGL = 0;
		return;
	}

	if (glewGetExtension("GL_EXT_framebuffer_object") != GL_TRUE) 
	{
		std::cerr<< "Framebuffer object not supported!\n";
		GlobalUtil::_GoodOpenGL = 0;
		return;
	}

	if(glewGetExtension("GL_ARB_texture_rectangle")==GL_TRUE)
	{
		GlobalUtil::_texTarget =  GL_TEXTURE_RECTANGLE_ARB;
		glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE_EXT, &value);
		GlobalUtil::_texMaxDimGL = value; 
		if(GlobalUtil::_verbose) std::cout<<"GL_TEXTURE_RECTANGLE_ARB:\t"<<GlobalUtil::_texMaxDimGL<<"\n";

		if(GlobalUtil::_texMaxDim == 0 || GlobalUtil::_texMaxDim > GlobalUtil::_texMaxDimGL)
		{
			GlobalUtil::_texMaxDim = GlobalUtil::_texMaxDimGL; 
		}
		glEnable(GlobalUtil::_texTarget);
	}else
	{
		std::cerr<<"GL_ARB_texture_rectangle not supported!\n";
		GlobalUtil::_GoodOpenGL = 0;
		return;
	}

	GlobalUtil::_SupportNVFloat = glewGetExtension("GL_NV_float_buffer");
	GlobalUtil::_SupportFP40 = glewGetExtension("GL_NV_fragment_program2");//fp40 at least for cg
	GlobalUtil::_SupportTextureRG = glewGetExtension("GL_ARB_texture_rg");

	glGetIntegerv(GL_MAX_DRAW_BUFFERS_ARB, &value);
	GlobalUtil::_MaxDrawBuffers = value;


	glShadeModel(GL_FLAT);
	glPolygonMode(GL_FRONT, GL_FILL);

	GlobalUtil::SetTextureParameter();


	if(GlobalUtil::_GoodOpenGL && GlobalUtil::_IsNvidia == 0 && 
		(GlobalUtil::_UseGLSL == 0 || GlobalUtil::_UseCUDA))
	{
		std::cerr << "#####----Switch to GLSL for non-nVidia graphic card----#####\n";
		GlobalUtil::_UseGLSL = 1;
		GlobalUtil::_UseCUDA = 0;
	}

	GlobalUtil::_GoodOpenGL = 1;
}

