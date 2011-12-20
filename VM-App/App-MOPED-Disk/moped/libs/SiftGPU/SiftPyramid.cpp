////////////////////////////////////////////////////////////////////////////
//	File:		SiftPyramid.cpp
//	Author:		Changchang Wu
//	Description :	Implementation of the SiftPyramid class.
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


#include "GL/glew.h"
#include <iostream>
#include <cstring>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <fstream>
#include <math.h>
using namespace std;

#include "GlobalUtil.h"
#include "SiftPyramid.h"


#ifdef DEBUG_SIFTGPU
#include "IL/il.h"
#include "direct.h"
#include "io.h"
#include <sys/stat.h> 
#endif



void SiftPyramid::RunSIFT(GLTexInput*input)
{
	//
	if(_existing_keypoints & SIFT_SKIP_FILTERING)
	{
		_timing[0] = 0;
	}else
	{
		GlobalUtil::StartTimer("Build    Pyramid");
		BuildPyramid(input);
		GlobalUtil::StopTimer();
		_timing[0] = GetElapsedTime();
	}


	if(_existing_keypoints)
	{
		//existing keypoint list should at least have the locations and scale
		GlobalUtil::StartTimer("Upload Feature List");
		ComputeGradient();
		GenerateFeatureListTex();
		GlobalUtil::StopTimer();
		_timing[2] = GetElapsedTime();
		_timing[1] = 0;
	}else
	{

		GlobalUtil::StartTimer("Detect Keypoints");
		DetectKeypointsEX();
		GlobalUtil::StopTimer();
		_timing[1] = GetElapsedTime();


		if(GlobalUtil::_ListGenGPU ==1)
		{
			GlobalUtil::StartTimer("Get Feature List");
			GenerateFeatureList();
			GlobalUtil::StopTimer();

		}else
		{
			GlobalUtil::StartTimer("Transfer Feature List");
			GenerateFeatureListCPU();
			GlobalUtil::StopTimer();
		}

		_timing[2] = GetElapsedTime();
	}

	
	if(_existing_keypoints& SIFT_SKIP_ORIENTATION)
	{
		//use exisitng feature orientation or
		
		_timing[3] = _timing[4] =0;
	}else 	if(GlobalUtil::_MaxOrientation>0)
	{
		//some extra tricks are done to handle existing keypoint list
		GlobalUtil::StartTimer("Feature Orientations");
		GetFeatureOrientations();
		GlobalUtil::StopTimer();
		_timing[3] = GetElapsedTime();

		//for existing keypoint list, only the best orientation is kept.
		if(GlobalUtil::_MaxOrientation >1 && !_existing_keypoints && !GlobalUtil::_FixedOrientation)
		{
			GlobalUtil::StartTimer("MultiO Feature List");
			ReshapeFeatureListCPU();
			GlobalUtil::StopTimer();	
			_timing[4] = GetElapsedTime();
		}else
		{
			_timing[4] = 0;
		}
	}else
	{
		GlobalUtil::StartTimer("Feature Orientations");
		GetSimplifiedOrientation();
		GlobalUtil::StopTimer();
		_timing[3] = GetElapsedTime();
		_timing[4] = 0; 
	}

	PrepareBuffer();

	if(_existing_keypoints & SIFT_SKIP_ORIENTATION)
	{
		//no need to read back feature if all fields of keypoints are already specified
		_timing[5] = 0;
	}else
	{
		GlobalUtil::StartTimer("Download Keypoints");
#ifdef NO_DUPLICATE_DOWNLOAD
		if(GlobalUtil::_MaxOrientation < 2 || GlobalUtil::_FixedOrientation)
#endif
		DownloadKeypoints();
		GlobalUtil::StopTimer();
		_timing[5] =  GetElapsedTime(); 
	}



	if(GlobalUtil::_DescriptorPPT)
	{
		//desciprotrs are downloaded in descriptor computation of each level
		GlobalUtil::StartTimer("Get Descriptor");
		GetFeatureDescriptors();
		GlobalUtil::StopTimer();
		_timing[6] =  GetElapsedTime(); 
	}else
	{
		_timing[6] = 0;
	}

	//reset the existing keypoints
	_existing_keypoints = 0;
	_keypoint_index.resize(0);

	if(GlobalUtil::_UseSiftGPUEX)
	{
		GlobalUtil::StartTimer("Gen. Display VBO");
		GenerateFeatureDisplayVBO();
		GlobalUtil::StopTimer();
		_timing[7] = GlobalUtil::GetElapsedTime();
	}else
	{
		_timing[7] = 0;
	} 
}




void SiftPyramid::PrepareBuffer()
{
	//when there is no existing keypoint list, the feature list need to be downloaded
	//when an existing keypoint list does not have orientaiton, we need to download them again.
	if(!(_existing_keypoints & SIFT_SKIP_ORIENTATION)) 
	{
		//_keypoint_buffer.resize(4 * (_featureNum +align));
		_keypoint_buffer.resize(4 * (_featureNum + GlobalUtil::_texMaxDim)); //11/19/2008
	}
	if(GlobalUtil::_DescriptorPPT)
	{
		//_descriptor_buffer.resize(128*(_featureNum + align)); 
		_descriptor_buffer.resize(128 * _featureNum + 16 * GlobalUtil::_texMaxDim);//11/19/2008
	}

}



void SiftPyramid::CopyFeatureVector(float*keys, float *descriptors)
{
	if(keys)		memcpy(keys, &_keypoint_buffer[0], 4*_featureNum*sizeof(float));
	if(descriptors)	memcpy(descriptors, &_descriptor_buffer[0], 128*_featureNum*sizeof(float));
}

void SiftPyramid:: SetKeypointList(int num, const float * keys, int run_on_current, int skip_orientation)
{
	//for each input keypoint
	//sort the key point list by size, and assign them to corresponding levels
	if(num <=0) return;
	_featureNum = num;
	///copy the keypoints
	_keypoint_buffer.resize(num * 4);
	memcpy(&_keypoint_buffer[0], keys, 4 * num * sizeof(float));
	//location and scale can be skipped
	_existing_keypoints = SIFT_SKIP_DETECTION;
	//filtering is skipped if it is running on the same image
	if(run_on_current) _existing_keypoints |= SIFT_SKIP_FILTERING;
	//orientation can be skipped if specified
	if(skip_orientation) _existing_keypoints |= SIFT_SKIP_ORIENTATION;
}


void SiftPyramid::SaveSIFT(const char * szFileName)
{
	if (_featureNum <=0) return;
	float * pk = &_keypoint_buffer[0];

	if(GlobalUtil::_BinarySIFT)
	{
		std::ofstream out(szFileName, ios::binary);
		out.write((char* )(&_featureNum), sizeof(int));

		if(GlobalUtil::_DescriptorPPT)
		{
			int dim = 128;
			out.write((char* )(&dim), sizeof(int));
			float * pd = &_descriptor_buffer[0] ;
			for(int i = 0; i < _featureNum; i++, pk+=4, pd +=128)
			{
				out.write((char* )(pk +1), sizeof(float));
				out.write((char* )(pk), sizeof(float));
				out.write((char* )(pk+2), 2 * sizeof(float));
				out.write((char* )(pd), 128 * sizeof(float));
			}
		}else
		{
			int dim = 0;
			out.write((char* )(&dim), sizeof(int));
			for(int i = 0; i < _featureNum; i++, pk+=4)
			{
				out.write((char* )(pk +1), sizeof(float));
				out.write((char* )(pk), sizeof(float));
				out.write((char* )(pk+2), 2 * sizeof(float));
			}
		}
	}else
	{
		std::ofstream out(szFileName);
		out.flags(ios::fixed);

		if(GlobalUtil::_DescriptorPPT)
		{
			float * pd = &_descriptor_buffer[0] ;
			out<<_featureNum<<" 128"<<endl;

			for(int i = 0; i < _featureNum; i++)
			{
				//in y, x, scale, orientation order
				out<<setprecision(2) << pk[1]<<" "<<setprecision(2) << pk[0]<<" "
					<<setprecision(3) << pk[2]<<" " <<setprecision(3) <<  pk[3]<< endl; 

				////out << setprecision(12) << pk[1] <<  " " << pk[0] << " " << pk[2] << " " << pk[3] << endl;
				pk+=4;
				for(int k = 0; k < 128; k ++, pd++) 
				{
					if(GlobalUtil::_NormalizedSIFT)
						out<< ((unsigned int)floor(0.5+512.0f*(*pd)))<<" ";
					else
						out << setprecision(8) << pd[0] << " ";

					if ( (k+1)%20 == 0 ) out<<endl; //suggested by Martin Schneider

				}
				out<<endl;

			}
		
		}else
		{
			out<<_featureNum<<" 0"<<endl;
			for(int i = 0; i < _featureNum; i++, pk+=4)
			{
				out<<pk[1]<<" "<<pk[0]<<" "<<pk[2]<<" " << pk[3]<<endl;
			}
		}
	}
}

#ifdef DEBUG_SIFTGPU
void SiftPyramid::BeginDEBUG(const char *imagepath)
{
	if(imagepath && imagepath[0])
	{
		strcpy(_debug_path, imagepath);
		strcat(_debug_path, ".debug");
	}else
	{
		strcpy(_debug_path, ".debug");
	}

	mkdir(_debug_path);
	chmod(_debug_path, _S_IREAD | _S_IWRITE);
}

void SiftPyramid::StopDEBUG()
{
	_debug_path[0] = 0;
}


void SiftPyramid::WriteTextureForDEBUG(GLTexImage * tex, const char *namet, ...)
{
	char name[_MAX_PATH];
	char * p = name, * ps = _debug_path;
	while(*ps) *p++ = *ps ++;
	*p++ = '/';
	va_list marker;
	va_start(marker, namet);
	vsprintf(p, namet, marker);
	va_end(marker);
	unsigned int imID;
	int width = tex->GetImgWidth();
	int height = tex->GetImgHeight();
	float* buffer1 = new float[ width * height  * 4];
	float* buffer2 = new float[ width * height  * 4];

	//read data back
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	tex->AttachToFBO(0);
	tex->FitTexViewPort();
	glReadPixels(0, 0, width, height, GL_RGBA , GL_FLOAT, buffer1);

	//Tiffs saved with IL are flipped
	for(int i = 0; i < height; i++)
	{
		memcpy(buffer2 + i * width * 4, 
			buffer1 + (height - i - 1) * width * 4,  
			width * 4 * sizeof(float));
	}

	//save data as floating point tiff file
	ilGenImages(1, &imID);
	ilBindImage(imID); 
	ilEnable(IL_FILE_OVERWRITE);
	ilTexImage(width, height, 1, 4, IL_RGBA, IL_FLOAT, buffer2);
	ilSave(IL_TIF, name); 
	ilDeleteImages(1, &imID); 

	delete buffer1;
	delete buffer2;
	glReadBuffer(GL_NONE);
}


#endif
