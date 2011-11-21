/* 
  MOPED (Multiple Object Pose Estimation and Detection) is a fast and
  scalable object recognition and pose estimation system. If you use this
  code, please reference our work in the following publications:
  
  [1] Collet, A., Berenson, D., Srinivasa, S. S., & Ferguson, D. "Object
  recognition and full pose registration from a single image for robotic
  manipulation." In ICRA 2009.  
  [2] Martinez, M., Collet, A., & Srinivasa, S. S. "MOPED: A Scalable and low
  Latency Object Recognition and Pose Estimation System." In ICRA 2010.
  
  Copyright: Carnegie Mellon University & Intel Corporation
  
  Authors:
   Alvaro Collet (alvaro.collet@gmail.com)
   Manuel Martinez (salutte@gmail.com)
   Siddhartha Srinivasa (siddhartha.srinivasa@intel.com)
  
  The MOPED software is developed at Intel Labs Pittsburgh. For more info,
  visit http://personalrobotics.intel-research.net/pittsburgh
  
  All rights reserved under the BSD license.
  
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products
     derived from this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

namespace MopedNS {

	class UTIL_UNDISTORT :public MopedAlg {

		struct CompareCameraParameters {
			bool operator() (const SP_Image& i1, const SP_Image& i2) const {
				
				if( i1->width != i2->width ) 
					return i1->width<i2->width;
				else if( i1->height != i2->height )
					return i1->height<i2->height;
				else if( i1->intrinsicLinearCalibration != i2->intrinsicLinearCalibration )
					return i1->intrinsicLinearCalibration < i2->intrinsicLinearCalibration;
				else 
					return i1->intrinsicNonlinearCalibration < i2->intrinsicNonlinearCalibration;
			}	
		};
		
		map< SP_Image, pair<IplImage*,IplImage*>, CompareCameraParameters > distortionMaps;

		void init( const SP_Image& i ) {

			CvSize imageSize = cvSize( i->width, i->height );
			IplImage *MapX = cvCreateImage( imageSize, IPL_DEPTH_32F, 1);
			IplImage *MapY = cvCreateImage( imageSize, IPL_DEPTH_32F, 1);

			float kk[9]={0};
			for(int x=0; x<9; x++) kk[x]=0;
			
			kk[0] = i->intrinsicLinearCalibration[0];
			kk[2] =	i->intrinsicLinearCalibration[2];
			kk[4] =	i->intrinsicLinearCalibration[1];
			kk[5] =	i->intrinsicLinearCalibration[3];
			kk[8] = 1.;
			CvMat cvK = cvMat( 3, 3, CV_32FC1, kk);  

			float kk_c[5];
			for(int x=0; x<4; x++) kk_c[x]= i->intrinsicNonlinearCalibration[x];
      kk_c[4] = 0;
			CvMat dist = cvMat( 5, 1, CV_32FC1, kk_c );

		#ifdef HAVE_CV_UNDISTORT_RECTIFY_MAP
				float feye[9] = {1,0,0,0,1,0,0,0,1};
				CvMat eye = cvMat(3,3,CV_32F, feye);
				cvInitUndistortRectifyMap(&cvK, &dist, NULL, &cvK, MapX, MapY);
		#else
				cvInitUndistortMap(&cvK, &dist, MapX, MapY);
		#endif

			distortionMaps[ i ] = make_pair( MapX, MapY );
		}

	public:
		
		
		void process( FrameData &frameData ) {
			
			#pragma omp parallel for
			for( int i=0; i<(int)frameData.images.size(); i++) {
				
				Image *image = frameData.images[i].get();
				pair<IplImage*,IplImage*> *maps;

				#pragma omp critical(UNDISTORT)
				{
					maps = &distortionMaps[ frameData.images[i] ];
				
					if( !maps->first || !maps->second ) {
						init( frameData.images[i] );
						maps = &distortionMaps[ frameData.images[i] ];
					}			
				}
				
				IplImage* gs = cvCreateImage(cvSize(image->width,image->height), IPL_DEPTH_8U, 1);

				for (int y = 0; y < image->height; y++) 
					memcpy( &gs->imageData[y*gs->widthStep], &image->data[y*image->width], image->width );

				IplImage *img = cvCloneImage(gs);
				cvRemap( img, gs, maps->first, maps->second, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS);
				cvReleaseImage(&img);

				for (int y = 0; y < image->height; y++) 
					memcpy( &image->data[y*image->width], &gs->imageData[y*gs->widthStep], image->width );
				
				cvReleaseImage(&gs);
			}
		}
	};
};
