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

#include <SiftGPU.h>
#include <GL/glew.h>
#include <GL/glx.h>

namespace MopedNS {

	class GLOBAL_DISPLAY:public MopedAlg {

		int display;
	public:

		GLOBAL_DISPLAY( int display ) 
		: display(display) { }

		void getConfig( map<string,string> &config ) const {

			GET_CONFIG( display );
		}
			
		void setConfig( map<string,string> &config ) {
			
			SET_CONFIG( display );
		}
		
		void process(FrameData &frameData) {
			
			vector<CvScalar> objectColors(256);
			for( int i=0; i<256; i++ )  {
				
				int r = i;
				
				r = (((r * 214013L + 2531011L) >> 16) & 32767);
				int a = 192 + r%64;
				r = (((r * 214013L + 2531011L) >> 16) & 32767);
				int b = 64 + r%128;
				r = (((r * 214013L + 2531011L) >> 16) & 32767);
				int c = r%64;
				for(int x=0; x<100; x++) if( (r = (((r * 214013L + 2531011L) >> 16) & 32767)) % 2 ) swap(a,b); else swap(a,c);
				
				objectColors[i] = cvScalar( a,b,c );
			}

			
			for( int i=0; i<(int)frameData.images.size(); i++) {
				
				IplImage *img;
				IplImage *small;
				
				IplImage* big = cvCreateImage(cvSize(1200,1800), IPL_DEPTH_8U, 3);
				
				
				// DRAW MATCH
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);


				//clog << frameData.images[i]->width << " " << frameData.images[i]->height << endl;

				for (int y = 0; y < frameData.images[i]->height; y++) {
					for (int x = 0; x < frameData.images[i]->width; x++) { 
						img->imageData[y*img->widthStep+3*x + 0] = frameData.images[i]->data[y*frameData.images[i]->width + x];
						img->imageData[y*img->widthStep+3*x + 1] = frameData.images[i]->data[y*frameData.images[i]->width + x];
						img->imageData[y*img->widthStep+3*x + 2] = frameData.images[i]->data[y*frameData.images[i]->width + x];
					}
				}
				
				vector< vector< FrameData::Match > >   &matches = frameData.matches;
				vector< vector< FrameData::Cluster > >&clusters = frameData.oldClusters;
			
				
				
				
				for(int model=0; model<(int)matches.size(); model++ ) {
					
					int objectHash = 0; 
					for(unsigned int x=0; x<(*models)[model]->name.size(); x++) objectHash = objectHash ^ (*models)[model]->name[x];
					CvScalar color = objectColors[objectHash % 256];
					
					foreach( match, matches[model] )
						if( match.imageIdx == i )
							cvCircle(img, cvPoint( match.coord2D[0], match.coord2D[1]), 3, color , CV_FILLED, CV_AA );
				}
				
				for(int model=0; model<(int)clusters.size(); model++ ) {
					
					int objectHash = 0; 
					for(unsigned int x=0; x<(*models)[model]->name.size(); x++) objectHash = objectHash ^ (*models)[model]->name[x];
					CvScalar color = objectColors[objectHash % 256];
					

					foreach( cluster, clusters[model] ) {
						
						if( cluster.size() &&  matches[model][ cluster.front() ].imageIdx == i ) {

							vector<Pt<2> > points; points.reserve( cluster.size() );
							foreach( match, cluster ) 
								points.push_back( matches[model][match].coord2D ); 
							
							list<Pt<2> > hull=getConvexHull(points); 
							eforeach( p, pt, hull ) {
								if( pt == hull.begin() ) continue;
								list<Pt<2> >::iterator ptm = pt; ptm--;
								cvLine(img, cvPoint( (*ptm)[0], (*ptm)[1]), cvPoint( p[0], p[1] ), color, 2, CV_AA );
							}
							cvLine(img, cvPoint( (*hull.begin())[0], (*hull.begin())[1]), cvPoint( (*hull.rbegin())[0], (*hull.rbegin())[1] ), color, 2, CV_AA );
						}
					}
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(400,300), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 300; y++) {
					for (int x = 0; x < 400; x++) { 
						big->imageData[(y+0)*big->widthStep + 3*(x+0) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+0)*big->widthStep + 3*(x+0) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+0)*big->widthStep + 3*(x+0) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				
				
				
				// DRAW MULTIPOSE
				
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);

				for (int y = 0; y < frameData.images[i]->height; y++) {
					for (int x = 0; x < frameData.images[i]->width; x++) { 
						img->imageData[y*img->widthStep+3*x + 0] = frameData.images[i]->data[y*frameData.images[i]->width + x];
						img->imageData[y*img->widthStep+3*x + 1] = frameData.images[i]->data[y*frameData.images[i]->width + x];
						img->imageData[y*img->widthStep+3*x + 2] = frameData.images[i]->data[y*frameData.images[i]->width + x];
					}
				}
				
				foreach( object, frameData.oldObjects ) {
					
					int objectHash = 0;
					for(unsigned int x=0; x<object->model->name.size(); x++) objectHash = objectHash ^ object->model->name[x];
					CvScalar color = objectColors[objectHash % 256];

				
					bool skip = false;
					Pt<2> bBox2D[2][2][2];
					for(int x=0; x<2; x++) {
						for(int y=0; y<2; y++) {
							for(int z=0; z<2; z++) {
								Pt<3> bBox3D;
								bBox3D.init( object->model->boundingBox[x][0], object->model->boundingBox[y][1], object->model->boundingBox[z][2] );
								bBox2D[x][y][z] = project( object->pose, bBox3D, *frameData.images[i].get() );
								skip = skip || bBox2D[x][y][z][0]==0 || bBox2D[x][y][z][1]==0;
							}
						}
					}
					
					if( skip ) continue;
					
					for(int x1=0; x1<2; x1++) 
						for(int y1=0; y1<2; y1++) 
							for(int z1=0; z1<2; z1++) 
								for(int x2=0; x2<2; x2++) 
									for(int y2=0; y2<2; y2++) 
										for(int z2=0; z2<2; z2++) 
											if( ((x1==x2)?1:0) + ((y1==y2)?1:0) + ((z1==z2)?1:0) == 2 )
												cvLine(img, cvPoint( bBox2D[x1][y1][z1][0], bBox2D[x1][y1][z1][1]), cvPoint( bBox2D[x2][y2][z2][0], bBox2D[x2][y2][z2][1] ), color, 2, CV_AA);
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(400,300), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 300; y++) {
					for (int x = 0; x < 400; x++) { 
						big->imageData[(y+300)*big->widthStep + 3*(x+0) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+300)*big->widthStep + 3*(x+0) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+300)*big->widthStep + 3*(x+0) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				// DRAW FILTER
				
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);

				for (int y = 0; y < frameData.images[i]->height; y++) {
					for (int x = 0; x < frameData.images[i]->width; x++) { 
						img->imageData[y*img->widthStep+3*x + 0] = frameData.images[i]->data[y*frameData.images[i]->width + x];
						img->imageData[y*img->widthStep+3*x + 1] = frameData.images[i]->data[y*frameData.images[i]->width + x];
						img->imageData[y*img->widthStep+3*x + 2] = frameData.images[i]->data[y*frameData.images[i]->width + x];
					}
				}
				
				foreach( object, *frameData.objects ) {
					
					int objectHash = 0;
					for(unsigned int x=0; x<object->model->name.size(); x++) objectHash = objectHash ^ object->model->name[x];
					CvScalar color = objectColors[objectHash % 256];

				
					Pt<2> hull_center; hull_center.init(0.,0.);
				
					bool skip = false;
					Pt<2> bBox2D[2][2][2];
					for(int x=0; x<2; x++) {
						for(int y=0; y<2; y++) {
							for(int z=0; z<2; z++) {
								Pt<3> bBox3D;
								bBox3D.init( object->model->boundingBox[x][0], object->model->boundingBox[y][1], object->model->boundingBox[z][2] );
								bBox2D[x][y][z] = project( object->pose, bBox3D, *frameData.images[i].get() );
								hull_center += bBox2D[x][y][z];
								skip = skip || bBox2D[x][y][z][0]==0 || bBox2D[x][y][z][1]==0;
							}
						}
					}
					hull_center /= 8.;
					
					if( skip ) continue;
					
					for(int x1=0; x1<2; x1++) 
						for(int y1=0; y1<2; y1++) 
							for(int z1=0; z1<2; z1++) 
								for(int x2=0; x2<2; x2++) 
									for(int y2=0; y2<2; y2++) 
										for(int z2=0; z2<2; z2++) 
											if( ((x1==x2)?1:0) + ((y1==y2)?1:0) + ((z1==z2)?1:0) == 2 )
												cvLine(img, cvPoint( bBox2D[x1][y1][z1][0], bBox2D[x1][y1][z1][1]), cvPoint( bBox2D[x2][y2][z2][0], bBox2D[x2][y2][z2][1] ), color, 2, CV_AA);

					{
				
						float dist = object->pose[4]*object->pose[4]+object->pose[5]*object->pose[5]+object->pose[6]*object->pose[6];
						dist = sqrt(dist);
						dist = float(int(1000*dist)/10.);
						
			
						CvFont font;
						double hScale=.8;
						double vScale=.8;
						int    lineWidth=1;
						cvInitFont(&font,CV_FONT_HERSHEY_DUPLEX, hScale,vScale,0,lineWidth, CV_AA);
						
						CvSize size_dist, size_txt;
						int baseline;
						cvGetTextSize( (toString(dist)+"cm").c_str(),  &font, &size_dist, &baseline);
						cvGetTextSize( (object->model->name).c_str(),  &font, &size_txt, &baseline);
						
						for (int y = max(hull_center[1]-size_dist.height - 4, (Float)0.); y < min(hull_center[1], (Float)img->height); y++) 
							for (int x = 3*max(hull_center[0]-size_dist.width/2 - 12, (Float)0.); x < 3*min(hull_center[0]+size_dist.width/2 + 12, (Float)img->width); x++) 
								img->imageData[y*img->widthStep+x] = ((unsigned char)(img->imageData[y*img->widthStep+x]))/3;

						for (int y = max(hull_center[1] + 4, (Float)0.); y < min(hull_center[1] + size_txt.height + 8, (Float)img->height); y++) 
							for (int x = 3*max(hull_center[0]-size_txt.width/2 - 12, (Float)0.); x < 3*min(hull_center[0]+size_txt.width/2 + 12, (Float)img->width); x++) 
								img->imageData[y*img->widthStep+x] = ((unsigned char)(img->imageData[y*img->widthStep+x]))/3;
						
						cvPutText(img, (toString(dist)+"cm").c_str(),cvPoint(hull_center[0]-size_dist.width/2,hull_center[1]), &font, color);
						cvPutText(img, (object->model->name).c_str(),cvPoint(hull_center[0]-size_txt.width/2,hull_center[1]+size_txt.height + 4), &font, color);

					}
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(800,600), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 600; y++) {
					for (int x = 0; x < 800; x++) { 
						big->imageData[(y+0)*big->widthStep + 3*(x+400) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+0)*big->widthStep + 3*(x+400) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+0)*big->widthStep + 3*(x+400) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				
				// DRAW MULTIPLE POSES
				
				
				Pose alternate;
				
				
				// DRAW POSE LEFT
				
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);
				
				for (int y = 0; y < img->height; y++)
					for (int x = 0; x <  img->width; x++) 
						for (int z = 0; z <  3; z++) 
							img->imageData[(y+0)*img->widthStep + 3*(x+0) + z] = 0;

				alternate = frameData.images[i].get()->cameraPose;
				
				alternate.translation[0]+=0.2;
				alternate.translation[1]-=0.5;				
				alternate.rotation.init( -sin(0.3), 0., 0., cos(0.3) );
				alternate.translation[2]-=0.5;
				
				{ 
					CvScalar grid = cvScalar( 64., 64., 64. );
				
					for(double x=-1; x<1; x+=0.1) {
						
						Pose p;
						p.translation.init(0.,0.4,1.2);
						p.rotation.init(0.,0.,0., 1.);
						
						Pt<3> a, b, c, d;
						a.init( x, 0., -1. );
						b.init( x, 0.,  1. );
						c.init(-1., 0.,  x  );
						d.init( 1., 0.,  x  );
						
						Pt<2> aa, bb, cc, dd;
						
						aa = project( p, a, *frameData.images[i].get(), alternate );
						bb = project( p, b, *frameData.images[i].get(), alternate );
						cc = project( p, c, *frameData.images[i].get() , alternate);
						dd = project( p, d, *frameData.images[i].get(), alternate );

						cvLine(img, cvPoint( aa[0],aa[1] ) , cvPoint( bb[0], bb[1] ), grid, 2, CV_AA);
						cvLine(img, cvPoint( cc[0],cc[1] ) , cvPoint( dd[0], dd[1] ), grid, 2, CV_AA);
					}
				}	
					
				
				foreach( object, *frameData.objects ) {
					
					int objectHash = 0;
					for(unsigned int x=0; x<object->model->name.size(); x++) objectHash = objectHash ^ object->model->name[x];
					CvScalar color = objectColors[objectHash % 256];

				
					bool skip = false;
					Pt<2> bBox2D[2][2][2];
					for(int x=0; x<2; x++) {
						for(int y=0; y<2; y++) {
							for(int z=0; z<2; z++) {
								Pt<3> bBox3D;
								bBox3D.init( object->model->boundingBox[x][0], object->model->boundingBox[y][1], object->model->boundingBox[z][2] );
								bBox2D[x][y][z] = project( object->pose, bBox3D, *frameData.images[i].get(), alternate );
								skip = skip || bBox2D[x][y][z][0]==0 || bBox2D[x][y][z][1]==0;
							}
						}
					}
					
					if( skip ) continue;
					
					for(int x1=0; x1<2; x1++) 
						for(int y1=0; y1<2; y1++) 
							for(int z1=0; z1<2; z1++) 
								for(int x2=0; x2<2; x2++) 
									for(int y2=0; y2<2; y2++) 
										for(int z2=0; z2<2; z2++) 
											if( ((x1==x2)?1:0) + ((y1==y2)?1:0) + ((z1==z2)?1:0) == 2 )
												cvLine(img, cvPoint( bBox2D[x1][y1][z1][0], bBox2D[x1][y1][z1][1]), cvPoint( bBox2D[x2][y2][z2][0], bBox2D[x2][y2][z2][1] ), color, 2, CV_AA);
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(400,300), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 300; y++) {
					for (int x = 0; x < 400; x++) { 
						big->imageData[(y+600)*big->widthStep + 3*(x+0) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+600)*big->widthStep + 3*(x+0) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+600)*big->widthStep + 3*(x+0) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				
				// DRAW CAMERA CENTER
				
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);
				
				for (int y = 0; y < img->height; y++)
					for (int x = 0; x <  img->width; x++) 
						for (int z = 0; z <  3; z++) 
							img->imageData[(y+0)*img->widthStep + 3*(x+0) + z] = 0;
				
				
				alternate = frameData.images[i].get()->cameraPose;
				
				alternate.translation[0]-=0.0;

				alternate.translation[1]-=2.;				
				alternate.rotation.init( -sin(0.5), 0., 0., cos(0.5) );
				alternate.translation[2]-=.71;
				
								{ 
					CvScalar grid = cvScalar( 64., 64., 64. );
				
					for(double x=-1.2; x<1.2; x+=0.1) {
						
						Pose p;
						p.translation.init(0.,0.4,1.2);
						p.rotation.init(0.,0.,0., 1.);
						
						Pt<3> a, b, c, d;
						a.init( x, 0., -1.2 );
						b.init( x, 0.,  1.2 );
						c.init(-1.2, 0.,  x  );
						d.init( 1.2, 0.,  x  );
						
						Pt<2> aa, bb, cc, dd;
						
						aa = project( p, a, *frameData.images[i].get(), alternate );
						bb = project( p, b, *frameData.images[i].get(), alternate );
						cc = project( p, c, *frameData.images[i].get(), alternate );
						dd = project( p, d, *frameData.images[i].get(), alternate );

						cvLine(img, cvPoint( aa[0],aa[1] ) , cvPoint( bb[0], bb[1] ), grid, 2, CV_AA);
						cvLine(img, cvPoint( cc[0],cc[1] ) , cvPoint( dd[0], dd[1] ), grid, 2, CV_AA);
					}
				}
				foreach( object, *frameData.objects ) {
					
					int objectHash = 0;
					for(unsigned int x=0; x<object->model->name.size(); x++) objectHash = objectHash ^ object->model->name[x];
					CvScalar color = objectColors[objectHash % 256];

					Pt<2> hull_center; hull_center.init(0.,0.);
				
					bool skip = false;
					Pt<2> bBox2D[2][2][2];
					for(int x=0; x<2; x++) {
						for(int y=0; y<2; y++) {
							for(int z=0; z<2; z++) {
								Pt<3> bBox3D;
								bBox3D.init( object->model->boundingBox[x][0], object->model->boundingBox[y][1], object->model->boundingBox[z][2] );
								bBox2D[x][y][z] = project( object->pose, bBox3D, *frameData.images[i].get(), alternate );
								hull_center += bBox2D[x][y][z];
								skip = skip || bBox2D[x][y][z][0]==0 || bBox2D[x][y][z][1]==0;
							}
						}
					}
					hull_center /= 8;
					
					if( skip ) continue;
					
					for(int x1=0; x1<2; x1++) 
						for(int y1=0; y1<2; y1++) 
							for(int z1=0; z1<2; z1++) 
								for(int x2=0; x2<2; x2++) 
									for(int y2=0; y2<2; y2++) 
										for(int z2=0; z2<2; z2++) 
											if( ((x1==x2)?1:0) + ((y1==y2)?1:0) + ((z1==z2)?1:0) == 2 )
												cvLine(img, cvPoint( bBox2D[x1][y1][z1][0], bBox2D[x1][y1][z1][1]), cvPoint( bBox2D[x2][y2][z2][0], bBox2D[x2][y2][z2][1] ), color, 2, CV_AA);
				

				
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(400,300), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 300; y++) {
					for (int x = 0; x < 400; x++) { 
						big->imageData[(y+600)*big->widthStep + 3*(x+400) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+600)*big->widthStep + 3*(x+400) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+600)*big->widthStep + 3*(x+400) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				// DRAW CAMERA RIGHT
				
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);
				for (int y = 0; y < img->height; y++)
					for (int x = 0; x <  img->width; x++) 
						for (int z = 0; z <  3; z++) 
							img->imageData[(y+0)*img->widthStep + 3*(x+0) + z] = 0;
				
				alternate = frameData.images[i].get()->cameraPose;
				
				alternate.translation[0]-=0.2;
				
				alternate.translation[1]-=0.5;				
				alternate.rotation.init( -sin(0.3), 0., 0., cos(0.3) );
				alternate.translation[2]-=0.5;

								{ 
					CvScalar grid = cvScalar( 64., 64., 64. );
				
					for(double x=-1; x<1; x+=0.1) {
						
						Pose p;
						p.translation.init(0.,0.4,1.2);
						p.rotation.init(0.,0.,0., 1.);
						
						Pt<3> a, b, c, d;
						a.init( x, 0., -1. );
						b.init( x, 0.,  1. );
						c.init(-1., 0.,  x  );
						d.init( 1., 0.,  x  );
						
						Pt<2> aa, bb, cc, dd;
						
						aa = project( p, a, *frameData.images[i].get(), alternate );
						bb = project( p, b, *frameData.images[i].get(), alternate );
						cc = project( p, c, *frameData.images[i].get(), alternate );
						dd = project( p, d, *frameData.images[i].get(), alternate );

						cvLine(img, cvPoint( aa[0],aa[1] ) , cvPoint( bb[0], bb[1] ), grid, 2, CV_AA);
						cvLine(img, cvPoint( cc[0],cc[1] ) , cvPoint( dd[0], dd[1] ), grid, 2, CV_AA);
					}
				}foreach( object, *frameData.objects ) {
					
					int objectHash = 0;
					for(unsigned int x=0; x<object->model->name.size(); x++) objectHash = objectHash ^ object->model->name[x];
					CvScalar color = objectColors[objectHash % 256];

				
					bool skip = false;
					Pt<2> bBox2D[2][2][2];
					for(int x=0; x<2; x++) {
						for(int y=0; y<2; y++) {
							for(int z=0; z<2; z++) {
								Pt<3> bBox3D;
								bBox3D.init( object->model->boundingBox[x][0], object->model->boundingBox[y][1], object->model->boundingBox[z][2] );
								bBox2D[x][y][z] = project( object->pose, bBox3D, *frameData.images[i].get(), alternate );
								skip = skip || bBox2D[x][y][z][0]==0 || bBox2D[x][y][z][1]==0;
							}
						}
					}
					
					if( skip ) continue;
					
					for(int x1=0; x1<2; x1++) 
						for(int y1=0; y1<2; y1++) 
							for(int z1=0; z1<2; z1++) 
								for(int x2=0; x2<2; x2++) 
									for(int y2=0; y2<2; y2++) 
										for(int z2=0; z2<2; z2++) 
											if( ((x1==x2)?1:0) + ((y1==y2)?1:0) + ((z1==z2)?1:0) == 2 )
												cvLine(img, cvPoint( bBox2D[x1][y1][z1][0], bBox2D[x1][y1][z1][1]), cvPoint( bBox2D[x2][y2][z2][0], bBox2D[x2][y2][z2][1] ), color, 2, CV_AA);
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(400,300), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 300; y++) {
					for (int x = 0; x < 400; x++) { 
						big->imageData[(y+600)*big->widthStep + 3*(x+800) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+600)*big->widthStep + 3*(x+800) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+600)*big->widthStep + 3*(x+800) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				
				// DRAW POSE TOP LEFT
				
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);
				
				for (int y = 0; y < img->height; y++)
					for (int x = 0; x <  img->width; x++) 
						for (int z = 0; z <  3; z++) 
							img->imageData[(y+0)*img->widthStep + 3*(x+0) + z] = 0;

				alternate = frameData.images[i].get()->cameraPose;
				
				alternate.translation[0]+=0.1;
				
								{ 
					CvScalar grid = cvScalar( 64., 64., 64. );
				
					for(double x=-1; x<1; x+=0.1) {
						
						Pose p;
						p.translation.init(0.,0.4,1.2);
						p.rotation.init(0.,0.,0., 1.);
						
						Pt<3> a, b, c, d;
						a.init( x, 0., -1. );
						b.init( x, 0.,  1. );
						c.init(-1., 0.,  x  );
						d.init( 1., 0.,  x  );
						
						Pt<2> aa, bb, cc, dd;
						
						aa = project( p, a, *frameData.images[i].get(), alternate );
						bb = project( p, b, *frameData.images[i].get(), alternate );
						cc = project( p, c, *frameData.images[i].get(), alternate );
						dd = project( p, d, *frameData.images[i].get(), alternate );

						cvLine(img, cvPoint( aa[0],aa[1] ) , cvPoint( bb[0], bb[1] ), grid, 2, CV_AA);
						cvLine(img, cvPoint( cc[0],cc[1] ) , cvPoint( dd[0], dd[1] ), grid, 2, CV_AA);
					}
				}foreach( object, *frameData.objects ) {
					
					int objectHash = 0;
					for(unsigned int x=0; x<object->model->name.size(); x++) objectHash = objectHash ^ object->model->name[x];
					CvScalar color = objectColors[objectHash % 256];

				
					bool skip = false;
					Pt<2> bBox2D[2][2][2];
					for(int x=0; x<2; x++) {
						for(int y=0; y<2; y++) {
							for(int z=0; z<2; z++) {
								Pt<3> bBox3D;
								bBox3D.init( object->model->boundingBox[x][0], object->model->boundingBox[y][1], object->model->boundingBox[z][2] );
								bBox2D[x][y][z] = project( object->pose, bBox3D, *frameData.images[i].get(), alternate );
								skip = skip || bBox2D[x][y][z][0]==0 || bBox2D[x][y][z][1]==0;
							}
						}
					}
					
					if( skip ) continue;
					
					for(int x1=0; x1<2; x1++) 
						for(int y1=0; y1<2; y1++) 
							for(int z1=0; z1<2; z1++) 
								for(int x2=0; x2<2; x2++) 
									for(int y2=0; y2<2; y2++) 
										for(int z2=0; z2<2; z2++) 
											if( ((x1==x2)?1:0) + ((y1==y2)?1:0) + ((z1==z2)?1:0) == 2 )
												cvLine(img, cvPoint( bBox2D[x1][y1][z1][0], bBox2D[x1][y1][z1][1]), cvPoint( bBox2D[x2][y2][z2][0], bBox2D[x2][y2][z2][1] ), color, 2, CV_AA);
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(400,300), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 300; y++) {
					for (int x = 0; x < 400; x++) { 
						big->imageData[(y+900)*big->widthStep + 3*(x+0) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+900)*big->widthStep + 3*(x+0) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+900)*big->widthStep + 3*(x+0) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				
				// DRAW CAMERA TOP CENTER
				
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);
				
				for (int y = 0; y < img->height; y++)
					for (int x = 0; x <  img->width; x++) 
						for (int z = 0; z <  3; z++) 
							img->imageData[(y+0)*img->widthStep + 3*(x+0) + z] = 0;
				
				
				alternate = frameData.images[i].get()->cameraPose;
				
				alternate.translation[0]-=0.0;
				
								{ 
					CvScalar grid = cvScalar( 64., 64., 64. );
				
					for(double x=-1; x<1; x+=0.1) {
						
						Pose p;
						p.translation.init(0.,0.4,1.2);
						p.rotation.init(0.,0.,0., 1.);
						
						Pt<3> a, b, c, d;
						a.init( x, 0., -1. );
						b.init( x, 0.,  1. );
						c.init(-1., 0.,  x  );
						d.init( 1., 0.,  x  );
						
						Pt<2> aa, bb, cc, dd;
						
						aa = project( p, a, *frameData.images[i].get(), alternate );
						bb = project( p, b, *frameData.images[i].get(), alternate );
						cc = project( p, c, *frameData.images[i].get(), alternate );
						dd = project( p, d, *frameData.images[i].get(), alternate );

						cvLine(img, cvPoint( aa[0],aa[1] ) , cvPoint( bb[0], bb[1] ), grid, 2, CV_AA);
						cvLine(img, cvPoint( cc[0],cc[1] ) , cvPoint( dd[0], dd[1] ), grid, 2, CV_AA);
					}
				}foreach( object, *frameData.objects ) {
					
					int objectHash = 0;
					for(unsigned int x=0; x<object->model->name.size(); x++) objectHash = objectHash ^ object->model->name[x];
					CvScalar color = objectColors[objectHash % 256];

				
					bool skip = false;
					Pt<2> bBox2D[2][2][2];
					for(int x=0; x<2; x++) {
						for(int y=0; y<2; y++) {
							for(int z=0; z<2; z++) {
								Pt<3> bBox3D;
								bBox3D.init( object->model->boundingBox[x][0], object->model->boundingBox[y][1], object->model->boundingBox[z][2] );
								bBox2D[x][y][z] = project( object->pose, bBox3D, *frameData.images[i].get(), alternate );
								skip = skip || bBox2D[x][y][z][0]==0 || bBox2D[x][y][z][1]==0;
							}
						}
					}
					
					if( skip ) continue;
					
					for(int x1=0; x1<2; x1++) 
						for(int y1=0; y1<2; y1++) 
							for(int z1=0; z1<2; z1++) 
								for(int x2=0; x2<2; x2++) 
									for(int y2=0; y2<2; y2++) 
										for(int z2=0; z2<2; z2++) 
											if( ((x1==x2)?1:0) + ((y1==y2)?1:0) + ((z1==z2)?1:0) == 2 )
												cvLine(img, cvPoint( bBox2D[x1][y1][z1][0], bBox2D[x1][y1][z1][1]), cvPoint( bBox2D[x2][y2][z2][0], bBox2D[x2][y2][z2][1] ), color, 2, CV_AA);
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(400,300), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 300; y++) {
					for (int x = 0; x < 400; x++) { 
						big->imageData[(y+900)*big->widthStep + 3*(x+400) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+900)*big->widthStep + 3*(x+400) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+900)*big->widthStep + 3*(x+400) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				// DRAW CAMERA TOP RIGHT
				
				img = cvCreateImage(cvSize(frameData.images[i]->width,frameData.images[i]->height), IPL_DEPTH_8U, 3);
				for (int y = 0; y < img->height; y++)
					for (int x = 0; x <  img->width; x++) 
						for (int z = 0; z <  3; z++) 
							img->imageData[(y+0)*img->widthStep + 3*(x+0) + z] = 0;
				
				alternate = frameData.images[i].get()->cameraPose;
				
				alternate.translation[0]-=0.1;
				
								{ 
					CvScalar grid = cvScalar( 64., 64., 64. );
				
					for(double x=-1; x<1; x+=0.1) {
						
						Pose p;
						p.translation.init(0.,0.4,1.2);
						p.rotation.init(0.,0.,0., 1.);
						
						Pt<3> a, b, c, d;
						a.init( x, 0., -1. );
						b.init( x, 0.,  1. );
						c.init(-1., 0.,  x  );
						d.init( 1., 0.,  x  );
						
						Pt<2> aa, bb, cc, dd;
						
						aa = project( p, a, *frameData.images[i].get(), alternate );
						bb = project( p, b, *frameData.images[i].get(), alternate );
						cc = project( p, c, *frameData.images[i].get(), alternate );
						dd = project( p, d, *frameData.images[i].get(), alternate );

						cvLine(img, cvPoint( aa[0],aa[1] ) , cvPoint( bb[0], bb[1] ), grid, 2, CV_AA);
						cvLine(img, cvPoint( cc[0],cc[1] ) , cvPoint( dd[0], dd[1] ), grid, 2, CV_AA);
					}
				}foreach( object, *frameData.objects ) {
					
					int objectHash = 0;
					for(unsigned int x=0; x<object->model->name.size(); x++) objectHash = objectHash ^ object->model->name[x];
					CvScalar color = objectColors[objectHash % 256];

				
					bool skip = false;
					Pt<2> bBox2D[2][2][2];
					for(int x=0; x<2; x++) {
						for(int y=0; y<2; y++) {
							for(int z=0; z<2; z++) {
								Pt<3> bBox3D;
								bBox3D.init( object->model->boundingBox[x][0], object->model->boundingBox[y][1], object->model->boundingBox[z][2] );
								bBox2D[x][y][z] = project( object->pose, bBox3D, *frameData.images[i].get(), alternate );
								skip = skip || bBox2D[x][y][z][0]==0 || bBox2D[x][y][z][1]==0;
							}
						}
					}
					
					if( skip ) continue;
					
					for(int x1=0; x1<2; x1++) 
						for(int y1=0; y1<2; y1++) 
							for(int z1=0; z1<2; z1++) 
								for(int x2=0; x2<2; x2++) 
									for(int y2=0; y2<2; y2++) 
										for(int z2=0; z2<2; z2++) 
											if( ((x1==x2)?1:0) + ((y1==y2)?1:0) + ((z1==z2)?1:0) == 2 )
												cvLine(img, cvPoint( bBox2D[x1][y1][z1][0], bBox2D[x1][y1][z1][1]), cvPoint( bBox2D[x2][y2][z2][0], bBox2D[x2][y2][z2][1] ), color, 2, CV_AA);
				}
				
				
				
				
				
				small = cvCreateImage(cvSize(400,300), IPL_DEPTH_8U, 3);
				cvResize( img, small, CV_INTER_CUBIC);
				
				for (int y = 0; y < 300; y++) {
					for (int x = 0; x < 400; x++) { 
						big->imageData[(y+900)*big->widthStep + 3*(x+800) + 0] = small->imageData[ y*small->widthStep + 3*x + 0];
						big->imageData[(y+900)*big->widthStep + 3*(x+800) + 1] = small->imageData[ y*small->widthStep + 3*x + 1];
						big->imageData[(y+900)*big->widthStep + 3*(x+800) + 2] = small->imageData[ y*small->widthStep + 3*x + 2];
					}
				}
				
				cvReleaseImage(&small);
				cvReleaseImage(&img);
				
				// FILL DATA INFO:
				
				for (int y = 0; y < 600; y++)
					for (int x = 0; x < 1200; x++) 
						for (int z = 0; z <  3; z++) 
							big->imageData[(y+1200)*big->widthStep + 3*x + z] = 0;
				
				// SEPARATE ZONES
				CvScalar intelColor = cvScalar( 64,32,32 );
				
				cvLine(big, cvPoint( 0, 0 ), cvPoint( 0, 1799 ),intelColor, 2, CV_AA);
				cvLine(big, cvPoint( 1199, 0 ), cvPoint( 1199, 1799 ), intelColor, 2, CV_AA);
				cvLine(big, cvPoint( 0, 0 ), cvPoint( 1199, 0 ), intelColor, 2, CV_AA);
				cvLine(big, cvPoint( 0, 1799 ), cvPoint( 1199, 1799 ), intelColor, 2, CV_AA);

				cvLine(big, cvPoint( 400, 0 ), cvPoint( 400, 600 ), intelColor, 2, CV_AA);
				cvLine(big, cvPoint( 0, 300 ), cvPoint( 400, 300 ), intelColor, 2, CV_AA);
				cvLine(big, cvPoint( 0, 600 ), cvPoint( 1199, 600 ), intelColor, 2, CV_AA);
				
				cvLine(big, cvPoint( 0, 900 ), cvPoint( 1199, 900 ), intelColor, 2, CV_AA);
				cvLine(big, cvPoint( 0, 1200 ), cvPoint( 1199, 1200 ), intelColor, 2, CV_AA);

				cvLine(big, cvPoint( 400, 600 ), cvPoint( 400, 1200 ), intelColor, 2, CV_AA);
				cvLine(big, cvPoint( 800, 600 ), cvPoint( 800, 1200 ), intelColor, 2, CV_AA);
				
				
				// FRAME RATE INFO

				/*{
				
					float dist = object->pose[4]*object->pose[4]+object->pose[5]*object->pose[5]+object->pose[6]*object->pose[6];
					dist = sqrt(dist);
					dist *= 63./41.;
					dist = float(int(1000*dist)/10.);
					
		
					CvFont font;
					double hScale=.8;
					double vScale=.8;
					int    lineWidth=1;
					cvInitFont(&font,CV_FONT_HERSHEY_DUPLEX, hScale,vScale,0,lineWidth, CV_AA);
					
					CvSize size;
					int baseline;
					cvGetTextSize( (toString(dist)+"cm").c_str(),  &font, &size, &baseline);

					for (int y = max(hull_center[1]-size.height/2 - 4, (float)0.); y < min(hull_center[1]+size.height/2 + 4, (float)img->height); y++) 
						for (int x = 3*max(hull_center[0]-size.width/2 - 12, (float)0.); x < 3*min(hull_center[0]+size.width/2 + 12, (float)img->width); x++) 
							img->imageData[y*img->widthStep+x] = ((unsigned char)(img->imageData[y*img->widthStep+x]))/3;

					cvPutText(img, (toString(dist)+"cm").c_str(),cvPoint(hull_center[0]-size.width/2,hull_center[1]+size.height/2), &font, color);

				}*/

				
			/*	"UNDISTORTORTED_IMAGE"
				"SIFT128"
				"MATCH_SIFT128"
				"CLUSTER"
				"POSE"
				"FILTER"
				"POSE2"
				"FILTER2" */
				
				Pt<8> times;
				times[0] = frameData.times["UNDISTORTORTED_IMAGE"];
				times[1] = frameData.times["SIFT128"];
				times[2] = frameData.times["MATCH_SIFT128"];
				times[3] = frameData.times["CLUSTER"];
				times[4] = frameData.times["POSE"];
				times[5] = frameData.times["FILTER"];
				times[6] = frameData.times["POSE2"];
				times[7] = frameData.times["FILTER2"];
				
				float totalTime = 0;
				for( int t=0; t<8; t++)
					totalTime += times[t]*1000.;
				
				
				
				
				//times.norm();
				times *= 2;
				float maxTime = 0;
				for( int t=0; t<8; t++)
					maxTime = max( maxTime, (float)times[t] );
	
				if( maxTime>1. ) times /= maxTime;
				
				

				for( int t=0; t<8; t++)
					for (int y = 0; y < 45; y++) 
						for (int x = 0 ; x < times[t]*200; x++)  {
							big->imageData[(y+1270+50*t)*big->widthStep - 3*x +750+ 0] = (t%2?192:128);
							big->imageData[(y+1270+50*t)*big->widthStep - 3*x +750+ 1] = (t%2?128:96);
						}
	
				{
				
					CvFont font;
					double hScale=.8;
					double vScale=.8;
					int    lineWidth=1.;
					cvInitFont(&font,CV_FONT_HERSHEY_DUPLEX, hScale,vScale,0,lineWidth, CV_AA);
					
					cvPutText(big, "UNDISTORT",cvPoint(270, 1300), &font, cvScalar( 192., 192., 192.) );
					cvPutText(big, "FEATURE EXTRACTION",cvPoint(270, 1350), &font, cvScalar( 192., 192., 192.) );
					cvPutText(big, "MATCHING",cvPoint(270, 1400), &font, cvScalar( 192., 192., 192.) );
					cvPutText(big, "CLUSTERING",cvPoint(270, 1450), &font, cvScalar( 192., 192., 192.) );
					cvPutText(big, "OBJECT DETECTION",cvPoint(270, 1500), &font, cvScalar( 192., 192., 192.) );
					cvPutText(big, "CLUSTER MERGING",cvPoint(270, 1550), &font, cvScalar( 192., 192., 192.) );
					cvPutText(big, "POSE ESTIMATION",cvPoint(270, 1600), &font, cvScalar( 192., 192., 192.) );
					cvPutText(big, "FILTERING",cvPoint(270, 1650), &font, cvScalar( 192., 192., 192.) );
				}
				
				
				
				{
				
					CvFont font;
					double hScale=.8;
					double vScale=.8;
					int    lineWidth=1.;
					cvInitFont(&font,CV_FONT_HERSHEY_DUPLEX, hScale,vScale,0,lineWidth, CV_AA);
					
					cvPutText(big, "RESOLUTION: 1024x768",cvPoint(700, 1300), &font, cvScalar( 192., 192., 192.) );
					cvPutText(big, (string("PROCESSING TIME: ")+toString(totalTime) +"ms").c_str(),cvPoint(700, 1350), &font, cvScalar( 192., 192., 192.) );
					
				}
				
				
				
				
				
				// CREATE FINAL MERGE
				
				string windowName = "MOPED v0.2 Experimental";
				
				cvNamedWindow( windowName.c_str(), !CV_WINDOW_AUTOSIZE);
				
				cvShowImage( windowName.c_str(), big );
				
				cvReleaseImage(&big);
			}
			
			cvWaitKey( 100 );			
		}
	};
};
