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
#include <lm.h>

namespace MopedNS {

	class POSE_RANSAC_LM_DIFF_CPU :public MopedAlg {
	
		int MaxRANSACTests;
		int MaxObjectsXCluster;
		int MaxLMTests;
		int NPtsAlign;
		int MinNPtsObject;
		float ErrorThreshold;
		
		struct LmData {
			
			int imageIdx;
			Image *image;
			Pt<2> coord2D;
			
			Pt<3> coord3D;
			Pt<3> line;
			Pt<3> cameraTranslation;
		};
		 
		// This function populates "samples" with nSamples references to object correspondences
		// The samples are randomly choosen, and aren't repeated
		bool randSample( vector<LmData *> &samples, const vector<LmData *> &cluster, unsigned int nSamples) {
		
			// Do not add a correspondence of the same image at the same coordinate
			map< pair<int, Pt<2> >, int > used;
		
			// Create a vector of samples prefixed with a random int. The int has preference over the pointer when sorting the vector.
			deque< pair< float, LmData * > > randomSamples;
			foreach( match, cluster )
				randomSamples.push_back( make_pair( (float)rand(), match ) );
			sort( randomSamples.begin(), randomSamples.end() );
			
			while( used.size() < nSamples && !randomSamples.empty() ) {

				pair<int, Pt<2> > imageAndPoint( randomSamples.front().second->imageIdx, randomSamples.front().second->coord2D );
				
				if( !used[ imageAndPoint ]++ )
					samples.push_back( randomSamples.front().second );
				
				randomSamples.pop_front();
			}
			
			return used.size() == nSamples;
		}
		
		static void lmFuncQuat(float *pose, float *pts2D, int nPose, int nPts2D, void *data) {
		
			vector<LmData *> &lmData = *(vector<LmData *> *)data;
			
			float d=0; for(int x=0; x<4; x++) d+=pose[x]*pose[x]; d=1./sqrtf(d); for(int x=0; x<4; x++) pose[x]*=d;
			
			TransformMatrix TM;
			TM.init( pose, pose + 4 );
			
			for( int i=0; i<nPts2D; ) {

				Pt<3> p;  
				TM.transform( p, lmData[i/3]->coord3D );
				
				Pt<3> &c = lmData[i/3]->cameraTranslation;
				Pt<3> &v = lmData[i/3]->line;
				
				p[0]=c[0]-p[0]; 
				p[1]=c[1]-p[1]; 
				p[2]=c[2]-p[2];
				
				float d=p[0]*v[0] + p[1]*v[1] + p[2]*v[2];
				if( d < 0 ) {
					 
					pts2D[i++] = -d + 10;
					pts2D[i++] = -d + 10;
					pts2D[i++] = -d + 10;

				} else {
					pts2D[i++] = v[1]*p[2]-v[2]*p[1];
					pts2D[i++] = v[2]*p[0]-v[0]*p[2];
					pts2D[i++] = v[0]*p[1]-v[1]*p[0];
				}
			}
		}
		
		float optimizeCamera( Pose &pose, const vector<LmData *> &samples, const int samplesSize, const int maxLMTests ) {

			// set up vector for LM
			float camPoseLM[7] = {
				pose.rotation[0], pose.rotation[1], pose.rotation[2], pose.rotation[3],
				pose.translation[0], pose.translation[1], pose.translation[2] };
			
			// LM expects pts2D as a single vector
			vector<float> pts2D( samplesSize*3, 0 );
		
			float info[LM_INFO_SZ];
			
			// call levmar
			int retValue = slevmar_dif(lmFuncQuat, camPoseLM, &pts2D[0], 7, samplesSize*3, maxLMTests, 
							NULL, info, NULL, NULL, (void *)&samples);

			if( retValue < 0 ) return retValue;

			pose.rotation.init( camPoseLM );
			pose.translation.init( camPoseLM + 4 );
			
			pose.rotation.norm();
			// output is in camPoseLM
			return info[1];
		}
				
		/*int testAllPoints( const Pose &pose, const vector<LmData *> &consistentCorresp, const float ErrorThreshold ) {
			
			int nConsistentCorresp = 0;
			
			TransformMatrix TM;
			TM.init( pose.rotation, pose.translation );
			
			foreach( corresp, consistentCorresp ) {

				Pt<3> p;  
				TM.transform( p, corresp->coord3D );
				
				Pt<3> &c = corresp->cameraTranslation;
				Pt<3> &v = corresp->line;
				
				p[0]-=c[0]; p[1]-=c[1]; p[2]-=c[2];
				
				float backProjectionError = powf(p[1]*v[2]-p[2]*v[1],2)+powf(p[2]*v[0]-p[0]*v[2],2)+powf(p[0]*v[1]-p[1]*v[0],2);

				if( backProjectionError < ErrorThreshold ) 
					nConsistentCorresp++;
			}
			return nConsistentCorresp;
		}*/
		
		int testAllPoints( const Pose &pose, const vector<LmData *> &consistentCorresp, const float ErrorThreshold ) {
			
			int nConsistentCorresp = 0;
						
			foreach( corresp, consistentCorresp ) {

				Pt<2> p = project( pose, corresp->coord3D, *corresp->image );
				
				float projectionError = powf( p[0] - corresp->coord2D[0], 2) + powf( p[1] - corresp->coord2D[1], 2 );
				
				if( projectionError < ErrorThreshold ) 
					nConsistentCorresp++;
			}
			return nConsistentCorresp;
		}
		
		void initPose( Pose &pose, const vector<LmData *> &samples ) {
			
			pose.rotation.init( (rand()&255)/256., (rand()&255)/256., (rand()&255)/256., (rand()&255)/256. );
			pose.translation.init( 0.,0.,0.5 );
		}
		
		bool RANSAC( vector<SP_Object> &objects, const vector<LmData *> &cluster, int MaxObjectsXCluster ) {
		
			vector<LmData *> samples;
			for ( int nIters = 0; nIters<MaxRANSACTests && objects.size() < MaxObjectsXCluster; nIters++) {

				samples.clear();
				if( !randSample( samples, cluster, NPtsAlign ) ) return false;
				
				initPose( pose, samples );
					
				float LMRet = optimizeCamera( pose, samples, NPtsAlign, MaxLMTests );
				if( LMRet == -1 || LMRet > ErrorThreshold ) continue;
				
				if( LMRet < 0.0001 ) {
					SP_Object obj(new Object);
					obj->pose = pose;
					obj->score = LMRet;
					object.push_back( obj );
				}
			}
		}

		void preprocessAllMatches( vector< vector< LmData > > &lmData, const vector< vector< FrameData::Match > > &matches, const vector< SP_Image > &images ) {
			
			TransformMatrix TM[images.size()];
			
			for(int i=0; i<(int)images.size(); i++ )
				TM[i].init( images[i]->cameraPose );
			
			lmData.resize( matches.size() );
			for(int model=0; model<(int)matches.size(); model++ )
				lmData[model].resize( matches[model].size() );	
			
			
			vector< pair<int,int> > tasks; tasks.reserve(1000);
			for(int model=0; model<(int)matches.size(); model++ )
				for(int match=0; match<(int)matches[model].size(); match++ )
					tasks.push_back( make_pair(model, match) );
				
			#pragma omp parallel for
			for(int task=0; task<(int)tasks.size(); task++) {
				
				
				int model=tasks[task].first;
				int match=tasks[task].second;
				const SP_Image &image = images[ matches[model][match].imageIdx ];
				
				Pt<3> pt3D;
				pt3D[0] = (matches[model][match].coord2D[0] - image->intrinsicLinearCalibration[2]) / image->intrinsicLinearCalibration[0];
				pt3D[1] = (matches[model][match].coord2D[1] - image->intrinsicLinearCalibration[3]) / image->intrinsicLinearCalibration[1];
				pt3D[2] = 1.;
				
				TM[ matches[model][match].imageIdx ].transform( lmData[model][match].line, pt3D );
				
				lmData[model][match].image = image.get();
				
				lmData[model][match].line[0] = image->cameraPose.translation[0] - lmData[model][match].line[0];
				lmData[model][match].line[1] = image->cameraPose.translation[1] - lmData[model][match].line[1];
				lmData[model][match].line[2] = image->cameraPose.translation[2] - lmData[model][match].line[2];
				
				lmData[model][match].line.norm();

				lmData[model][match].imageIdx = matches[model][match].imageIdx;
				lmData[model][match].coord2D = matches[model][match].coord2D;
			
				lmData[model][match].coord3D = matches[model][match].coord3D;
				lmData[model][match].cameraTranslation = image->cameraPose.translation;
			}
		}

	public:

		POSE_RANSAC_LM_DIFF_CPU( int MaxRANSACTests, int MaxObjectsXCluster, int MaxLMTests, int NPtsAlign, int MinNPtsObject, float ErrorThreshold )
		: MaxRANSACTests(MaxRANSACTests), MaxObjectsXCluster(MaxObjectsXCluster), MaxLMTests(MaxLMTests), NPtsAlign(NPtsAlign), MinNPtsObject(MinNPtsObject), ErrorThreshold(ErrorThreshold) {
		}

		void getConfig( map<string,string> &config ) const {
			
			GET_CONFIG( MaxRANSACTests );
			GET_CONFIG( MaxObjectsXCluster );
			GET_CONFIG( MaxLMTests );
			GET_CONFIG( NPtsAlign );
			GET_CONFIG( MinNPtsObject );
			GET_CONFIG( ErrorThreshold );
		}
		
		void setConfig( map<string,string> &config ) {

			SET_CONFIG( MaxRANSACTests );
			GET_CONFIG( MaxObjectsXCluster );
			SET_CONFIG( MaxLMTests );
			SET_CONFIG( NPtsAlign );
			SET_CONFIG( MinNPtsObject );
			SET_CONFIG( ErrorThreshold );
		}
		
		void process( FrameData &frameData ) {

			
			vector< SP_Image > &images = frameData.images;
			vector< vector< FrameData::Match > > &matches = frameData.matches;
			vector< vector< FrameData::Cluster > >&clusters = frameData.clusters;

			
			vector< vector< LmData > > lmData; lmData.reserve(1000);
			preprocessAllMatches( lmData, matches, images );
			
			
			vector< pair<int,int> > tasks; tasks.reserve(1000);
			for(int model=0; model<(int)clusters.size(); model++ )
				for(int cluster=0; cluster<(int)clusters[model].size(); cluster++ )
					tasks.push_back( make_pair(model, cluster) );
			
			
				
			#pragma omp parallel for
			for(int task=0; task<(int)tasks.size(); task++) {
				
				int model=tasks[task].first;
				int cluster=tasks[task].second;
	
				vector<LmData *> cl;
				foreach( point, clusters[model][cluster] )
					cl.push_back( & lmData[model][ point ] );

				Pose pose;
				bool found = RANSAC( pose, cl, MaxObjectsXCluster );
				
				if( found ) 
				#pragma omp critical(POSE_PROCESS)
				{
					SP_Object obj(new Object);
					frameData.objects->push_back(obj);
					
					obj->pose = pose;
					obj->model = (*models)[model];
				}
			}
		}
	};
};
