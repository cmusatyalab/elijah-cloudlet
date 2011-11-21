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
#include <moped.hpp>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <dirent.h>
#include <omp.h>

#define foreach( i, c ) for( typeof((c).begin()) i##_hid=(c).begin(), *i##_hid2=((typeof((c).begin())*)1); i##_hid2 && i##_hid!=(c).end(); ++i##_hid) for( typeof( *(c).begin() ) &i=*i##_hid, *i##_hid3=(typeof( *(c).begin() )*)(i##_hid2=NULL); !i##_hid3 ; ++i##_hid3, ++i##_hid2) 

using namespace std;

using namespace MopedNS;

int main( int argc, char **argv ) {

	omp_set_num_threads(4);

	Moped moped;


	DIR *dp;
	struct dirent *dirp;

	if((dp  = opendir(argv[1])) ==  NULL) 
		throw string("Error opening \"") + argv[1] + "\"";

	list<string> fileNames;
	while( (dirp = readdir(dp)) != NULL ) {
		string fileName =  string(argv[1]) + "/" + string(dirp->d_name);
		reverse(fileName.begin(), fileName.end() );
		if( fileName.find("gpj.") != 0 ) continue;
		fileName=string(fileName, 4);
		reverse(fileName.begin(), fileName.end() );
		fileNames.push_back( fileName );
	}
	fileNames.sort();
		
		
	if( fileNames.size() == 0 )
		throw string("No Image files");


	vector<SP_Image> images;

	foreach( fn, fileNames) {

		IplImage *image = cvLoadImage( (fn+".jpg").c_str() ) ;

		SP_Image mopedImage( new Image );

		mopedImage->intrinsicLinearCalibration.init( 472., 470., 312., 240.); 
		mopedImage->intrinsicNonlinearCalibration.init(-2e-6, 2e-6, -2e-12, -2e-12);
		
		mopedImage->cameraPose.translation.init(0.,0.,0.);
		mopedImage->cameraPose.rotation.init(0.,0.,0.,1.);
		
		IplImage* gs = cvCreateImage(cvSize(image->width,image->height), IPL_DEPTH_8U, 1);
		cvCvtColor(image, gs, CV_BGR2GRAY);

		mopedImage->data.resize( image->width * image->height );
		for (int y = 0; y < image->height; y++) 
			memcpy( &mopedImage->data[y*image->width], &gs->imageData[y*gs->widthStep], image->width );

		mopedImage->width = image->width;
		mopedImage->height = image->height;
			
		mopedImage->name = fn;
	
		images.push_back( mopedImage );

		cvReleaseImage(&gs);
		cvReleaseImage( &image );
	}
	
	vector<shared_ptr<sXML> > models = moped.createPlanarModelsFromImages( images, atof( argv[2] ) );
	foreach( model, models ) {
		
		ofstream out(string((*model)["name"]+".moped.xml").c_str(),  ofstream::out);
		out << *model;
	}
	return 1;
}
