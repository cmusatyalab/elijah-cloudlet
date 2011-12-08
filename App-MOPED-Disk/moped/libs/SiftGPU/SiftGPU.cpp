////////////////////////////////////////////////////////////////////////////
//	File:		SiftGPU.cpp
//	Author:		Changchang Wu
//	Description :	Implementation of the SIFTGPU classes.
//					SiftGPU:	The SiftGPU Tool.  
//					SiftGPUEX:	SiftGPU + viewer
//					SiftParam:	Sift Parameters
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


#include "GL/glew.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <math.h>
#include <stdlib.h>
#include <time.h>
using namespace std;


#include "GlobalUtil.h"
#include "SiftGPU.h"
#include "IL/il.h"
#include "GLTexImage.h"
#include "ShaderMan.h"
#include "FrameBufferObject.h"
#include "SiftPyramid.h"
#include "PyramidGL.h"

//CUDA works only with vc8 or higher
#if (!defined(_MSC_VER) ||_MSC_VER >= 1400) && defined(CUDA_SIFTGPU_ENABLED)
#include "PyramidCU.h"
#endif


////
#if  defined(_WIN32) 
	#include "direct.h"
	#pragma comment(lib, "../lib/DevIL.lib")
	#pragma warning (disable : 4786) 
	#pragma warning (disable : 4996) 
#else
	//compatible with linux
	#define _stricmp strcasecmp
#endif

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//
//just want to make this class invisible
class ImageList:public std::vector<std::string> {};

SiftGPU::SiftGPU(int np)
{ 
	_texImage = new GLTexInput;
	_imgpath[0] = 0;
	_outpath[0] = 0;
	_initialized = 0;
	_image_loaded = 0;
	 GlobalUtil::_UseSiftGPUEX = 0;
	_current = 0;
	_list = new ImageList();
	
	_nPyramid = np < 1? 1 : np;
	_pyramids = NULL;
	_pyramid = NULL;
}



SiftGPUEX::SiftGPUEX() 
{
	_view = _sub_view = 0;
	_view_debug = 0;
	GlobalUtil::_UseSiftGPUEX = 1;
	srand((unsigned int)time(NULL));
	RandomizeColor();
}

void* SiftGPU::operator new (size_t  size){
  void * p = malloc(size);
  if (p == 0)  
  {
	  const std::bad_alloc ba;
	  throw ba; 
  }
  return p; 
}

void SiftGPU::SetActivePyramid(int index)
{
	if(index >=0 && index < _nPyramid)	
	{
		_pyramid = _pyramids[index];
	}
}

void SiftGPUEX::RandomizeColor()
{
	//
	float hsv[3] = {0, 0.8f, 1.0f};
	for(int i = 0; i < COLOR_NUM*3; i+=3)
	{
		hsv[0] = (rand()%100)*0.01f; //i/float(COLOR_NUM);
		HSVtoRGB(hsv, _colors+i);		
	}
}

SiftGPU::~SiftGPU()
{
	if(_pyramids)
	{
		for(int i = 0; i < _nPyramid; i++)
		{
			delete _pyramids[i];
		}
		delete _pyramids;
	}
	delete _texImage;
	delete _list;

	if(_initialized)
	{
		//destroy all the shaders?
		ShaderMan::DestroyShaders(_sigma_num);
		//shutdown iamge loader
		ilShutDown();
	} 

	//Calling glutDestroyWindow function will somehow give a heap corruption 
	//if(_glut_id >0) glutDestroyWindow(_glut_id);
}


inline void SiftGPU::InitSiftGPU()
{
	if(_initialized || GlobalUtil::_GoodOpenGL ==0) return;

#if (defined(_MSC_VER) && _MSC_VER < 1400) || !defined(CUDA_SIFTGPU_ENABLED)
	if(GlobalUtil::_UseCUDA)
	{
		GlobalUtil::_UseCUDA = 0;
		std::cerr	<< "---------------------------------------------------------------------------\n"
					<< "CUDA is not supported in this binary! To enable CUDA implementation, please\n" 
					<< "use SiftGPU_CUDA_Enable Project for VS2005 or define CUDA_SIFTGPU_ENABLED\n"
					<< "----------------------------------------------------------------------------\n";
		GlobalUtil::_usePackedTex = 1;

	}
#endif


	_pyramids = new SiftPyramid*[_nPyramid];
	for(int i = 0; i < _nPyramid; i++)
	{
#if (!defined(_MSC_VER) || _MSC_VER >= 1400) && defined(CUDA_SIFTGPU_ENABLED)
		if(GlobalUtil::_UseCUDA)
			_pyramids[i] = new PyramidCU(*this);
		else 
#endif
		if(GlobalUtil::_usePackedTex) 
			_pyramids[i] = new PyramidPacked(*this);
		else
			_pyramids[i] = new PyramidNaive(*this);
	}
	_pyramid =  _pyramids[0];

	ilInit();
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);
	///

	//init opengl parameters
	GlobalUtil::InitGLParam();

	//sift parameters
	ParseSiftParam();

	if(GlobalUtil::_GoodOpenGL)
	{
		if(GlobalUtil::_verbose)	std::cout<<"\n[GPU Language]:\t"<<
		(GlobalUtil::_UseCUDA? "CUDA" : (GlobalUtil::_UseGLSL?"GLSL" : "CG")) <<"\n\n";

		//load shaders..
		if(!GlobalUtil::_UseCUDA)
		{
			GlobalUtil::StartTimer("Load OpenGL Shaders");
			ShaderMan::InitShaderMan();
			ShaderMan::LoadDogShaders(_dog_threshold, _edge_threshold);
			ShaderMan::LoadGenListShader(_dog_level_num, 0);
			ShaderMan::CreateGaussianFilters(*this);
			GlobalUtil::StopTimer();
		}

		if(GlobalUtil::_InitPyramidWidth >0 && GlobalUtil::_InitPyramidHeight >0)
		{
			GlobalUtil::StartTimer("Initialize Pyramids");
			for(int i = 0; i < _nPyramid; i++)
			{
				_pyramid[i].InitPyramid(GlobalUtil::_InitPyramidWidth,
										GlobalUtil::_InitPyramidHeight, 0);
			}
			GlobalUtil::StopTimer();
		}
	}


	ClockTimer::InitHighResolution();

	_initialized = 1;

}

int	 SiftGPU::RunSIFT(int index)
{
	if(_list->size()>0 )
	{
		index = index % _list->size();
		if(strcmp(_imgpath, _list->at(index).data()))
		{
			strcpy(_imgpath, _list->at(index).data());
			_image_loaded = 0;
			_current = index;
		}
		return RunSIFT();
	}else
	{
		return 0;
	}

}

int  SiftGPU::RunSIFT( int width,  int height, const void * data, unsigned int gl_format, unsigned int gl_type)
{

	if(GlobalUtil::_GoodOpenGL ==0 ) return 0;
	if(!_initialized) InitSiftGPU();
	if(GlobalUtil::_GoodOpenGL ==0 ) return 0;

	if(width > 0 && height >0 && data != NULL)
	{
		_imgpath[0] = 0;
		//try downsample on CPU
		GlobalUtil::StartTimer("Upload Image data");
		if(_texImage->SetImageData(width, height, data, gl_format, gl_type))
		{
			_image_loaded = 2; //gldata;
			GlobalUtil::StopTimer();
			_timing[0] = GlobalUtil::GetElapsedTime();
			
			//if the size of image is different
			//pyramid need to be reallocated.
			GlobalUtil::StartTimer("Initialize Pyramid");
			_pyramid->InitPyramid(width, height, _texImage->_down_sampled);
			GlobalUtil::StopTimer();
			_timing[1] = GlobalUtil::GetElapsedTime();

			return RunSIFT();
		}else
		{
			return 0;
		}
	}else
	{
		return 0;
	}

}

int  SiftGPU::RunSIFT(char * imgpath)
{
	if(imgpath && imgpath[0])
	{
		if(strcmp(_imgpath, imgpath))
		{
			//set the new image
			strcpy(_imgpath, imgpath);
			_image_loaded = 0;
		}
		return RunSIFT();
	}else
	{
		return 0;
	}


}

int SiftGPU::RunSIFT(int num, const SiftKeypoint * keys, int keys_have_orientation)
{
	if(num <=0) return 0;
	_pyramid->SetKeypointList(num, (const float*) keys, 1, keys_have_orientation);
	return RunSIFT();
}

int SiftGPU::RunSIFT()
{
	//check image data
	if(_imgpath[0]==0 && _image_loaded == 0) return 0;

	//check OpenGL support
	if(GlobalUtil::_GoodOpenGL ==0 ) return 0;

	ClockTimer timer;

	//initialize SIFT GPU for once
	if(!_initialized)
	{
		InitSiftGPU();
		if(GlobalUtil::_GoodOpenGL ==0 ) return 0;
	}

	timer.StartTimer("RUN SIFT");
	//process input image file
	if( _image_loaded ==0)
	{
		//load image
		GlobalUtil::StartTimer("Load Input Image");

		//try to down-sample on cpu
		int width, height; 

		if(_texImage->LoadImageFile(_imgpath, width, height)==0)
		{
			return 0;
		}

		_image_loaded = 1;

		GlobalUtil::StopTimer();
		_timing[0] = GlobalUtil::GetElapsedTime();

		//make sure the pyrmid can hold the new image.
		GlobalUtil::StartTimer("Initialize Pyramid");
		_pyramid->InitPyramid(width, height, _texImage->_down_sampled);
		GlobalUtil::StopTimer();
		_timing[1] = GlobalUtil::GetElapsedTime();

	}else
	{
		//change some global states
		if(!GlobalUtil::_UseCUDA)
		{
			GlobalUtil::FitViewPort(1,1);
			_texImage->FitTexViewPort();
		}
		if(_image_loaded == 1)
		{
			_timing[0] = _timing[1] = 0;
		}else
		{//2
			_image_loaded = 1; 
		}
	}

	if(_pyramid->_allocated ==0 ) return 0;

	//process the image
#ifdef DEBUG_SIFTGPU
	_pyramid->BeginDEBUG(_imgpath);
#endif
	_pyramid->RunSIFT(_texImage);
	_pyramid->GetPyramidTiming(_timing + 2); //

	//write output once if there is only one input
	if(_outpath[0] )
	{
		SaveSIFT(_outpath);
		_outpath[0] = 0;
	}
	
	//if you just want to call TestWin(Glut) as a sift processor
	//now we end the process
	if(GlobalUtil::_ExitAfterSIFT && GlobalUtil::_UseSiftGPUEX) exit(0); 

	if(GlobalUtil::_UseCUDA == 0)
	{
		//clean up OpenGL stuff
		GLTexImage::UnbindMultiTex(3);
		ShaderMan::UnloadProgram();
		FrameBufferObject::DeleteGlobalFBO();
		GlobalUtil::CleanupOpenGL();
	}
	timer.StopTimer();
	if(GlobalUtil::_verbose)std::cout<<endl;

	return 1;
}


void SiftGPU::SetKeypointList(int num, const SiftKeypoint * keys, int keys_have_orientation)
{
	_pyramid->SetKeypointList(num, (const float*)keys, 0, keys_have_orientation);
}

void SiftGPUEX::DisplayInput() 
{
	if(_texImage==NULL) return;
	_texImage->BindTex();
	_texImage->DrawImage();
	_texImage->UnbindTex();

}

void SiftGPU::GetImageDimension( int &w,  int &h)
{
	w = _texImage->GetImgWidth();
	h = _texImage->GetImgHeight();

}

void SiftGPU::SetVerbose(int verbose)
{
	GlobalUtil::_timingO = verbose>2;
	GlobalUtil::_timingL = verbose>3;
	if(verbose == -1)
	{
		//Loop between verbose level 0, 1, 2
		if(GlobalUtil::_verbose)
		{
			GlobalUtil::_verbose  = GlobalUtil::_timingS;
			GlobalUtil::_timingS = 0;
			if(GlobalUtil::_verbose ==0 && GlobalUtil::_UseSiftGPUEX)
				std::cout << "Console ouput disabled, press Q/V to enable\n\n";
		}else
		{
			GlobalUtil::_verbose = 1;
			GlobalUtil::_timingS = 1;
		}
	}else if(verbose == -2)
	{
		//trick for disabling all output (still keeps the timing level)
		GlobalUtil::_verbose = 0;
		GlobalUtil::_timingS = 1;
	}else 
	{
		GlobalUtil::_verbose = verbose>0;
		GlobalUtil::_timingS = verbose>1;
	}

}


SiftParam::SiftParam()
{

	_level_min = -1;
	_dog_level_num  = 3;
	_level_max = 0;
	_sigma0 = 0;
	_sigman = 0;
	_edge_threshold = 0;
	_dog_threshold =  0;


}

float SiftParam::GetInitialSmoothSigma(int octave_min)
{
	float	sa = _sigma0 * powf(2.0f, float(_level_min)/float(_dog_level_num)) ; 
	float   sb = _sigman / powf(2.0f,  float(octave_min)) ;//
	float sigma_skip0 = sa>sb+ 0.001?sqrt(sa*sa - sb*sb): 0.0f;
	return sigma_skip0; 
}

void SiftParam::ParseSiftParam()
{ 

	if(_dog_level_num ==0) _dog_level_num = 3;
	if(_level_max ==0) _level_max = _dog_level_num + 1;
	if(_sigma0 ==0.0f) _sigma0 = 1.6f * powf(2.0f, 1.0f / _dog_level_num) ;
	if(_sigman == 0.0f) _sigman = 0.5f;


	_level_num = _level_max -_level_min + 1;

	_level_ds  = _level_min + _dog_level_num;
	if(_level_ds > _level_max ) _level_ds = _level_max ;


	///
	float _sigmak = powf(2.0f, 1.0f / _dog_level_num) ;
	float dsigma0 = _sigma0 * sqrt (1.0f - 1.0f / (_sigmak*_sigmak) ) ;
	float sa, sb;

 
	sa = _sigma0 * powf(_sigmak, (float)_level_min) ; 
	sb = _sigman / powf(2.0f,   (float)GlobalUtil::_octave_min_default) ;//

	_sigma_skip0 = sa>sb+ 0.001?sqrt(sa*sa - sb*sb): 0.0f;

    sa = _sigma0 * powf(_sigmak, float(_level_min )) ;
    sb = _sigma0 * powf(_sigmak, float(_level_ds - _dog_level_num)) ;

	_sigma_skip1 = sa>sb + 0.001? sqrt(sa*sa - sb*sb): 0.0f;

	_sigma_num = _level_max - _level_min;
	_sigma = new float[_sigma_num];

	for(int i = _level_min + 1; i <= _level_max; i++)
	{
		_sigma[i-_level_min -1] =  dsigma0 * powf(_sigmak, float(i)) ;
	}

	if(_dog_threshold ==0)	_dog_threshold      = 0.02f / _dog_level_num ;
	if(_edge_threshold==0) _edge_threshold		= 10.0f;
}


void SiftGPUEX::DisplayOctave(void (*UseDisplayShader)(), int i)
{
	if(_pyramid == NULL)return;
	const int grid_sz = (int)ceil(_level_num/2.0);
	double scale = 1.0/grid_sz ;
	int gx=0, gy=0, dx, dy;

	if(_pyramid->_octave_min >0) scale *= (1<<_pyramid->_octave_min);
	else if(_pyramid->_octave_min < 0) scale /= (1<<(-_pyramid->_octave_min));


	i = i% _pyramid->_octave_num;  //
	if(i<0 ) i+= _pyramid->_octave_num;

	scale *= ( 1<<(i));




	UseDisplayShader();

	glPushMatrix();
	glScaled(scale, scale, scale);
	for(int level = _level_min; level<= _level_max; level++)
	{
		GLTexImage * tex = _pyramid->GetLevelTexture(i+_pyramid->_octave_min, level);

		dx = tex->GetImgWidth();
		dy = tex->GetImgHeight();

		glPushMatrix();

		glTranslated(dx*gx, dy*gy, 0);

		tex->BindTex();

		tex->DrawImage();
		tex->UnbindTex();

		glPopMatrix();

		gx++;
		if(gx>=grid_sz) 
		{
			gx =0;
			gy++;
		}

	}

	glPopMatrix();
	ShaderMan::UnloadProgram();
}

void SiftGPUEX::DisplayPyramid( void (*UseDisplayShader)(), int dataName, int nskip1, int nskip2)
{

	if(_pyramid == NULL)return;
	int grid_sz = (_level_num -nskip1 - nskip2);
	if(grid_sz > 4) grid_sz = (int)ceil(grid_sz*0.5);
	double scale = 1.0/grid_sz;
	int stepx = 0, stepy = 0, dx, dy=0, nstep;

	if(_pyramid->_octave_min >0) scale *= (1<<_pyramid->_octave_min);
	else if(_pyramid->_octave_min < 0) scale /= (1<<(-_pyramid->_octave_min));


	glPushMatrix();
	glScaled(scale, scale, scale);

	for(int i = _pyramid->_octave_min; i < _pyramid->_octave_min+_pyramid->_octave_num; i++)
	{
	
		nstep = i==_pyramid->_octave_min? grid_sz: _level_num;
		dx = 0;
		UseDisplayShader();
		for(int j = _level_min + nskip1; j <= _level_max-nskip2; j++)
		{
			GLTexImage * tex = _pyramid->GetLevelTexture(i, j, dataName);
			if(tex->GetImgWidth() == 0 || tex->GetImgHeight() == 0) continue;
			stepx = tex->GetImgWidth();
			stepy = tex->GetImgHeight();	
			////
			if(j == _level_min + nskip1 + nstep)
			{
				dy += stepy;
				dx = 0;
			}

			glPushMatrix();
			glTranslated(dx, dy, 0);
			tex->BindTex();
			tex->DrawImage();
			tex->UnbindTex();
			glPopMatrix();

			dx += stepx;

		}

		ShaderMan::UnloadProgram();

		dy+= stepy;
	}

	glPopMatrix();
}


void SiftGPUEX::DisplayLevel(void (*UseDisplayShader)(), int i)
{
	if(_pyramid == NULL)return;

	i = i%(_level_num * _pyramid->_octave_num);
	if (i<0 ) i+= (_level_num * _pyramid->_octave_num);
	int octave = _pyramid->_octave_min + i/_level_num;
	int level  = _level_min + i%_level_num;
	double scale = 1.0;

	if(octave >0) scale *= (1<<octave);
	else if(octave < 0) scale /= (1<<(-octave));

	GLTexImage * tex = _pyramid->GetLevelTexture(octave, level);

	UseDisplayShader();

	glPushMatrix();
	glScaled(scale, scale, scale);
	tex->BindTex();
	tex->DrawImage();
	tex->UnbindTex();
	glPopMatrix();
	ShaderMan::UnloadProgram();
}

void SiftGPUEX::DisplaySIFT()
{
	if(_pyramid == NULL) return;
	if(_view_debug)
	{
		DisplayDebug();
		return;
	}
	switch(_view)
	{
	case 0:
		DisplayInput();
		DisplayFeatureBox(_sub_view);
		break;
	case 1:
		DisplayPyramid(ShaderMan::UseShaderDisplayGaussian, SiftPyramid::DATA_GAUSSIAN);
		break;
	case 2:
		DisplayOctave(ShaderMan::UseShaderDisplayGaussian, _sub_view);	
		break;
	case 3:
		DisplayLevel(ShaderMan::UseShaderDisplayGaussian, _sub_view);
		break;
	case 4:
		DisplayPyramid(ShaderMan::UseShaderDisplayDOG, SiftPyramid::DATA_DOG, 1);
		break;
	case 5:
		DisplayPyramid(ShaderMan::UseShaderDisplayGrad, SiftPyramid::DATA_GRAD, 1);
		break;
	case 6:
		DisplayPyramid(ShaderMan::UseShaderDisplayDOG, SiftPyramid::DATA_DOG,2, 1);
		DisplayPyramid(ShaderMan::UseShaderDisplayKeypoints, SiftPyramid::DATA_KEYPOINT, 2,1);
	}
}


void SiftGPUEX::SetView(int view, int sub_view, char *title)
{
	const char* view_titles[] =
	{
		"Original Image",
		"Gaussian Pyramid",
		"Octave Images",
		"Level Image",
		"Difference of Gaussian",
		"Gradient",
		"Keypoints"
	};
	const int view_num = 7;
	_view = view % view_num;
	if(_view <0) _view +=view_num;
	_sub_view = sub_view;

	if(_view_debug)
		strcpy(title, "Debug...");
	else
		strcpy(title, view_titles[_view]);

}


void SiftGPU::PrintUsage()
{
	std::cout
	<<"SiftGPU Usage:\n"
	<<"-h -help       : Help\n"
	<<"-i <strings>   : Filename(s) of the input image(s)\n"
	<<"-il <string>   : Filename of an image list file\n"
	<<"-o <string>    : Where to save SIFT features\n"
	<<"-f <float>     : Filter width factor; Width will be 2*factor+1 (default : 4.0)\n"
	<<"-w  <float>    : Orientation sample window factor (default: 2.0)\n"
	<<"-dw <float>  * : Descriptor grid size factor (default : 3.0)\n"
	<<"-fo <int>    * : First octave to detect DOG keypoints(default : 0)\n"
	<<"-no <int>      : Maximum number of Octaves (default : no limit)\n"
	<<"-d <int>       : Number of DOG levels in an octave (default : 3)\n"
	<<"-t <float>     : DOG threshold (default : 0.02/3)\n"
	<<"-e <float>     : Edge Threshold (default : 10.0)\n"
	<<"-m  <int=2>    : Multi Feature Orientations (default : 1)\n"
	<<"-m2p           : 2 Orientations packed as one float\n"
	<<"-s  <int=1>    : Sub-Pixel, Sub-Scale Localization, Multi-Refinement(num)\n"
	<<"-lcpu -lc <int>: CPU/GPU mixed Feature List Generation (defaut : 6)\n"
	<<"                 Use GPU first, and use CPU when reduction size <= pow(2,num)\n"
	<<"                 When <num> is missing or equals -1, no GPU will be used\n"
	<<"-noprep        : Upload raw data to GPU (default: RGB->LUM and down-sample on CPU)\n"
	<<"-sd            : Skip descriptor computation if specified\n"
	<<"-unn    *      : Write unnormalized descriptor if specified\n"
	<<"-b      *      : Write binary sift file if specified\n"
	<<"-fs <int>      : Block Size for freature storage <default : 4>\n"
	<<"-glsl          : Use GLSL SIFTGPU instead of CG (default : CG)\n"
	<<"-tight         : Automatically resize pyramid to fit new images tightly\n"
	<<"-p  WxH        : Inititialize the pyramids to contain image of WxH (eg -p 1024x768)\n"
	<<"-lm  <int>     : Maximum feature count for a level (for pre-allocation)\n"
	<<"-lmp <float>   : Maximum percent of pixels as features (for pre-allocaton)\n"
	<<"-v <int>       : Level of timing details. Same as calling Setverbose() function\n"
	<<"-loweo         : (0,0) at center of top-left pixel (defaut: corner)\n"
	<<"-maxd <int> *  : Max working dimension (default : 2560 (unpacked) / 3200 (packed))\n"
	<<"-exit          : Exit program after processing the input image\n"
	<<"-unpack        : Use the old unpacked implementation\n"
	<<"-di            : Use dynamic array indexing if available (defualt : no)\n"
	<<"                 It could make computation faster on cards like GTX 280\n"
	<<"-fastmath      : specify -fastmath to cg compiler (Not much difference.)\n"
	<<"-ofix     *    : use 0 as feature orientations.\n"
	<<"-ofix-not *	  : disable -ofix.\n"
	<<"------parameters marked with * can be changed after initialization---------\n"
	<<"\n";
}
void SiftGPU::ParseParam(int argc, char **argv)
{
	char* arg, *param;
	char* opt;
	int  HelpPrinted = 0, setMaxD = 0;
	int  i = 0;
	for( i = 0; i< argc; i++)
	{
		arg = argv[i];
		if(arg[0]!='-')continue;
		opt = arg+1;
		param = argv[i+1];
		if(_stricmp(opt, "h")==0 || _stricmp(opt,"help")==0)
		{
			HelpPrinted = 1;
			PrintUsage();
		}else if(_stricmp(opt, "glsl")==0)
		{
			GlobalUtil::_UseGLSL = 1;
		}else if(_stricmp(opt, "cg")==0)
		{
#if !defined(SIFTGPU_NO_CG)
			GlobalUtil::_UseGLSL = 0;
#endif
		}else if(_stricmp(opt, "cuda")==0)
		{
			GlobalUtil::_UseCUDA = 1;
			GlobalUtil::_usePackedTex = 0;
		}else if(_stricmp(opt, "pack")==0)
		{
			GlobalUtil::_usePackedTex = 1;
		}else if(_stricmp(opt, "unpack")==0)
		{
			GlobalUtil::_usePackedTex = 0;
		}else if(_stricmp(opt, "lcpu")==0||_stricmp(opt, "lc")==0)
		{
			int gskip = -1;
			if(i+1 <argc)	sscanf(param, "%d", &gskip);
			if(gskip >= 0)
			{
				GlobalUtil::_ListGenSkipGPU = gskip;
			}else
			{
				GlobalUtil::_ListGenGPU = 0;
			}
		}else if(_stricmp(opt, "prep")==0)
		{
			GlobalUtil::_PreProcessOnCPU = 1;
		}else if(_stricmp(opt, "noprep")==0)
		{
			GlobalUtil::_PreProcessOnCPU = 0;
		}else  if(_stricmp(opt, "fbo1")==0)
		{
			FrameBufferObject::UseSingleFBO =1;

		}else  if(_stricmp(opt, "fbos")==0)
		{
			FrameBufferObject::UseSingleFBO = 0;
		}
		else if(_stricmp(opt, "sd")==0)
		{
			GlobalUtil::_DescriptorPPT =0;
		}else if(_stricmp(opt, "unn")==0)
		{
			GlobalUtil::_NormalizedSIFT =0;
		}else if(_stricmp(opt, "b")==0)
		{
			GlobalUtil::_BinarySIFT = 1;
		}else if(_stricmp(opt, "tight")==0)
		{
			GlobalUtil::_ForceTightPyramid = 1;
		}else if(_stricmp(opt, "exit")==0)
		{
			GlobalUtil::_ExitAfterSIFT = 1;
		}else if(_stricmp(opt, "di")==0)
		{
			GlobalUtil::_UseDynamicIndexing = 1;
		}else if(_stricmp(opt, "sign")==0)
		{
			GlobalUtil::_KeepExtremumSign = 1;
		}else if(_stricmp(opt, "ov292")==0)
		{
			//for compatibility with old version//
			GlobalUtil::_GradientLevelOffset = 2; 
			GlobalUtil::_OrientationGaussianFactor = 4.5;
			GlobalUtil::_OrientationWindowFactor = 1.0;

		}else if(_stricmp(opt, "m")==0 || _stricmp(opt, "mo")==0)
		{
			int mo = 2; //default multi-orientation
			if(i+1 <argc)	sscanf(param, "%d", &mo);
			//at least two orientation
			GlobalUtil::_MaxOrientation = min(max(1, mo), 4);
		}else if(_stricmp(opt, "m2p") == 0)
		{
			GlobalUtil::_MaxOrientation = 2;
			GlobalUtil::_OrientationPack2 = 1;
		}else if(_stricmp(opt, "s") ==0)
		{
			int sp = 1; //default refinement
			if(i+1 <argc)	sscanf(param, "%d", &sp);
			//at least two orientation
			GlobalUtil::_SubpixelLocalization = min(max(0, sp),5);
		}
		else if(_stricmp(opt, "ofix")==0)
		{
			GlobalUtil::_FixedOrientation = 1;
		}else if(_stricmp(opt, "ofix-not")==0)
		{
			GlobalUtil::_FixedOrientation = 0;
		}else if(_stricmp(opt, "loweo")==0)
		{
			GlobalUtil::_LoweOrigin = 1;
		}else if(_stricmp(opt, "fastmath")==0)
		{
			GlobalUtil::_UseFastMath = 1;
		}else if(_stricmp(opt, "narrow")==0)
		{
			GlobalUtil::_NarrowFeatureTex = 1;
		}else if(_stricmp(opt, "debug")==0)
		{
			GlobalUtil::_debug = 1;
		}else if(i+1>=argc)
		{
			//make sure there is the param			
		}else if(_stricmp(opt, "i")==0)
		{
			strcpy(_imgpath, param);
			i++;
			//get the file list..
			_list->push_back(param);
			while( i+1 < argc && argv[i+1][0] !='-')
			{
				_list->push_back(argv[++i]);
			}
		}else if(_stricmp(opt, "il")==0)
		{
			LoadImageList(param);
			i++;
		}else if( _stricmp(opt, "o")==0)
		{
			strcpy(_outpath, param);
			i++;
		}else if( _stricmp(opt, "f")==0 )
		{

			float factor;
			sscanf(param, "%f", &factor);
			if(factor>0 )
			{
				GlobalUtil::_FilterWidthFactor  = factor;
				i++;
			}
		}else if( _stricmp(opt, "ot")==0 )
		{

			float factor;
			sscanf(param, "%f", &factor);
			if(factor>0 )
			{
				GlobalUtil::_MulitiOrientationThreshold  = factor;
				i++;
			}
		}else if(_stricmp(opt, "w")==0 )
		{

			float factor;
			sscanf(param, "%f", &factor);
			if(factor>0 )
			{
				GlobalUtil::_OrientationWindowFactor  = factor;
				i++;
			}
		}else if(_stricmp(opt, "dw")==0 )
		{

			float factor;
			sscanf(param, "%f", &factor);
			if(factor>0 )
			{
				GlobalUtil::_DescriptorWindowFactor  = factor;
				i++;
			}
		}else if(_stricmp(opt, "fo")==0)
		{

			int first_octave;
			sscanf(param, "%d", &first_octave);
			if(first_octave >=-2 )
			{
				GlobalUtil::_octave_min_default = first_octave;
				i++;
			}

		}else if(_stricmp(opt, "no")==0)
		{

			int octave_num=-1;
			sscanf(param, "%d", &octave_num);
			if(octave_num<=0) octave_num = -1;

			if(octave_num ==-1 || octave_num >=1)
			{
				GlobalUtil::_octave_num_default = octave_num;
				i++;
			}

		}else if( _stricmp(opt, "t")==0)
		{

			float threshold;
			sscanf(param, "%f", &threshold);
			if(threshold >0 && threshold < 0.5)
			{
				SiftParam::_dog_threshold = threshold;
				i++;
			}
		}else if(_stricmp(opt, "e")==0 )
		{
			float threshold;
			sscanf(param, "%f", &threshold);
			if(threshold >0 )
			{
				SiftParam::_edge_threshold = threshold;
				i++;
			}
		}else if(_stricmp(opt, "d")==0)
		{
			int num;
			sscanf(param, "%d", &num);
			if(num >=1 && num <=10)
			{
				SiftParam::_dog_level_num = num;
				i++;
			}

		}else if(_stricmp(opt, "fs")==0)
		{
			int num;
			sscanf(param, "%d", &num);
			if(num >=1)
			{
				GlobalParam::_FeatureTexBlock = num;
				i++;
			}

		}else if(_stricmp(opt, "p")==0)
		{
			int w =0, h=0;
			sscanf(param, "%dx%d", &w, &h);
			if(w >0 &&  h>0)
			{
				GlobalParam::_InitPyramidWidth = w;
				GlobalParam::_InitPyramidHeight = h;
			}
		}else if(_stricmp(opt, "levelmax")==0 || _stricmp(opt, "lm")==0)
		{
			int num;
			sscanf(param, "%d", &num);
			if(num >=1000)
			{
				GlobalParam::_MaxLevelFeatureNum = num;
				i++;
			}
		}else if(_stricmp(opt, "levelmaxpercent")==0 || _stricmp(opt, "lmp")==0)
		{
			float num;
			sscanf(param, "%f", &num);
			if(num >=0.001)
			{
				GlobalParam::_MaxFeaturePercent = num;
				i++;
			}
		}else if(_stricmp(opt, "v")==0 )
		{
			int num;
			sscanf(param, "%d", &num);
			if(num >=0 && num <=5)
			{
				SetVerbose(num);
			}
		}else if(_stricmp(opt, "maxd")==0 )
		{
			int num;
			sscanf(param, "%d", &num);
			if(num > 0)
			{
				GlobalUtil::_texMaxDim = num; 
				setMaxD = 1;
			}
		}
	}

	if(setMaxD == 0) GlobalUtil::_texMaxDim = GlobalUtil::_usePackedTex ? 3200 : 2560;

	//do not write result if there are more than one input images
	if(_outpath[0] && _list->size()>1)		_outpath[0] = 0;
} 

void SiftGPU::SetImageList(int nimage, const char** filelist)
{
	_list->resize(0);
	for(int i = 0; i < nimage; i++)
	{
		_list->push_back(filelist[i]);
	}
	_current = 0;

}
void SiftGPU:: LoadImageList(char *imlist)
{
	char filename[_MAX_PATH];
	ifstream in(imlist);
	while(in>>filename)
	{
		_list->push_back(filename);
	}
	in.close();


	if(_list->size()>0)
	{
		strcpy(_imgpath, _list->at(0).data());
		strcpy(filename, imlist);
		char * slash = strrchr(filename, '\\');
		if(slash == 0) slash = strrchr(filename, '/');
		if(slash )
		{
			slash[1] = 0;
			chdir(filename);
		}
	}
	_image_loaded = 0;


}
float SiftParam::GetLevelSigma( int lev)
{
	return _sigma0 * powf( 2.0f,  float(lev) / float(_dog_level_num )); //bug fix 9/12/2007
}

void SiftGPUEX::DisplayFeatureBox(int view )
{
	view = view%3;
	if(view<0)view+=3;
	if(view ==2) return;
	int idx = 0;
	const int *fnum = _pyramid->GetLevelFeatureNum();
	const GLuint *vbo = _pyramid->GetFeatureDipslayVBO();
	const GLuint *vbop = _pyramid->GetPointDisplayVBO();
	if(vbo == NULL || vbop == NULL) return;
	//int  nvbo = _dog_level_num * _pyramid->_octave_num;
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnableClientState(GL_VERTEX_ARRAY);
	glPushMatrix();
//	glTranslatef(0.0f, 0.0f, -1.0f);
	glPointSize(2.0f);

	float scale = 1.0f;
	if(_pyramid->_octave_min >0) scale *= (1<<_pyramid->_octave_min);
	else if(_pyramid->_octave_min < 0) scale /= (1<<(-_pyramid->_octave_min));
	glScalef(scale, scale, 1.0f);


	for(int i = 0; i < _pyramid->_octave_num; i++)
	{

		for(int j = 0; j < _dog_level_num; j++, idx++)
		{
			if(fnum[idx]>0)
			{
				if(view ==0)
				{
					glColor3f(0.2f, 1.0f, 0.2f);
					glBindBuffer(GL_ARRAY_BUFFER_ARB, vbop[idx]);
					glVertexPointer( 4, GL_FLOAT,4*sizeof(float), (char *) 0);
					glDrawArrays( GL_POINTS, 0, fnum[idx]);
					glFlush();
				}else
				{
						
					//glColor3f(1.0f, 0.0f, 0.0f);
					glColor3fv(_colors+ (idx%COLOR_NUM)*3);
					glBindBuffer(GL_ARRAY_BUFFER_ARB, vbo[idx]);
					glVertexPointer( 4, GL_FLOAT,4*sizeof(float), (char *) 0);
					glDrawArrays( GL_LINES, 0, fnum[idx]*10 );
					glFlush();
				}

			}
		
		}
		glTranslatef(-.5f, -.5f, 0.0f);
		glScalef(2.0f, 2.0f, 1.0f);

	}
	glPopMatrix();
	glDisableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPointSize(1.0f);
							
}

void SiftGPUEX::ToggleDisplayDebug()
{
	_view_debug = !_view_debug;
}

void SiftGPUEX::DisplayDebug()
{
	glPointSize(1.0f);
	glColor3f(1.0f, 0.0f, 0.0f);
	ShaderMan::UseShaderDebug();
	glBegin(GL_POINTS);
	for(int i = 0; i < 100; i++)
	{
		glVertex2f(i*4.0f+0.5f, i*4.0f+0.5f);
	}
	glEnd();
	ShaderMan::UnloadProgram();
}

int SiftGPU::CreateContextGL()
{
	//use GLUT to create an OpenGL Context
	if(!GlobalUtil::CreateWindowGLUT()) return 0;
	return VerifyContextGL();
}

int SiftGPU::VerifyContextGL()
{
	//GlobalUtil::_GoodOpenGL = -1;  //unknown
	InitSiftGPU();
	return GlobalUtil::_GoodOpenGL + GlobalUtil::_FullSupported;
}

int SiftGPU::IsFullSupported()
{
	return GlobalUtil::_GoodOpenGL==1 &&  GlobalUtil::_FullSupported;
}

void SiftGPU::SaveSIFT(const char * szFileName)
{
	_pyramid->SaveSIFT(szFileName);
}

int SiftGPU::GetFeatureNum()
{
	return _pyramid->GetFeatureNum();
}

void SiftGPU::GetFeatureVector(SiftKeypoint * keys, float * descriptors)
{
//	keys.resize(_pyramid->GetFeatureNum());
	if(GlobalUtil::_DescriptorPPT)
	{
	//	descriptors.resize(128*_pyramid->GetFeatureNum());
		_pyramid->CopyFeatureVector((float*) (&keys[0]), &descriptors[0]);
	}else
	{
		//descriptors.resize(0);
		_pyramid->CopyFeatureVector((float*) (&keys[0]), NULL);
	}
}

void SiftGPU::SetTightPyramid(int tight)
{
	GlobalUtil::_ForceTightPyramid = tight;
}

int SiftGPU::AllocatePyramid(int width, int height)
{
	_pyramid->_down_sample_factor = 0;
	_pyramid->_octave_min = GlobalUtil::_octave_min_default;
	if(GlobalUtil::_octave_min_default>=0)
	{
		width >>= GlobalUtil::_octave_min_default;
		height >>= GlobalUtil::_octave_min_default;
	}else
	{
		width <<= (-GlobalUtil::_octave_min_default);
		height <<= (-GlobalUtil::_octave_min_default);
	}
	_pyramid->ResizePyramid(width, height);
	return _pyramid->_pyramid_height == height && width == _pyramid->_pyramid_width ;
}
void SiftGPU::SetMaxDimension(int sz)
{
	if(sz < GlobalUtil::_texMaxDimGL)
	{
		GlobalUtil::_texMaxDim = sz;
	}
}
int SiftGPU::GetImageCount()
{
	return _list->size();
}

void SiftGPUEX::HSVtoRGB(float hsv[3],float rgb[3] )
{

	int i;
	float q, t, p;
	float hh,f, v = hsv[2];
	if(hsv[1]==0.0f)
	{
		rgb[0]=rgb[1]=rgb[2]=v;
	}
	else
	{
		//////////////
		hh =hsv[0]*6.0f ;   // sector 0 to 5
		i =(int)hh ;
		f = hh- i;   // factorial part of h
		//////////
		p=  v * ( 1 - hsv[1] );
		q = v * ( 1 - hsv[1] * f );
		t = v * ( 1 - hsv[1] * ( 1 - f ) );
		switch( i ) {
			case 0:rgb[0] = v;rgb[1] = t;rgb[2] = p;break;
			case 1:rgb[0] = q;rgb[1] = v;rgb[2] = p;break;
			case 2:rgb[0] = p;rgb[1] = v;rgb[2] = t;break;
			case 3:rgb[0] = p;rgb[1] = q;rgb[2] = v;break;
			case 4:rgb[0] = t;rgb[1] = p;rgb[2] = v;break;
			case 5:rgb[0] = v;rgb[1] = p;rgb[2] = q;break;
			default:rgb[0]= 0;rgb[1] = 0;rgb[2] = 0;
		}
	}
}

SiftGPU* CreateNewSiftGPU(int np)
{
	return new SiftGPU(np);
}
