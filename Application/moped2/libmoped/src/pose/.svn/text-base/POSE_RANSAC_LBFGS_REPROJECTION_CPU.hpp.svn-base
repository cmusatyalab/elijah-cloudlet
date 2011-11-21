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

#include <lbfgs.h>

namespace MopedNS {

	class POSE_RANSAC_LBFGS_REPROJECTION_CPU :public MopedAlg {
	
		int MaxRANSACTests;
		int MaxOptTests;
		int NObjectsCluster;
		int NPtsAlign;
		int MinNPtsObject;
		float ErrorThreshold;
		
		struct OptData {
			
			Image *image;
			Pt<2> coord2D;
			Pt<3> coord3D;
		};
		 
		// This function populates "samples" with nSamples references to object correspondences
		// The samples are randomly choosen, and aren't repeated
		bool randSample( vector<OptData *> &samples, const vector<OptData *> &cluster, unsigned int nSamples) {
		
			// Do not add a correspondence of the same image at the same coordinate
			map< pair<Image *, Pt<2> >, int > used;
		
			// Create a vector of samples prefixed with a random int. The int has preference over the pointer when sorting the vector.
			deque< pair< float, OptData * > > randomSamples;
			foreach( match, cluster )
				randomSamples.push_back( make_pair( (float)rand(), match ) );
			sort( randomSamples.begin(), randomSamples.end() );
			
			while( used.size() < nSamples && !randomSamples.empty() ) {

				pair<Image *, Pt<2> > imageAndPoint( randomSamples.front().second->image, randomSamples.front().second->coord2D );
				
				if( !used[ imageAndPoint ]++ )
					samples.push_back( randomSamples.front().second );
				
				randomSamples.pop_front();
			}
			
			return used.size() == nSamples;
		}
		
		static lbfgsfloatval_t lbfgsFuncQuat( void *data, const lbfgsfloatval_t *ps, lbfgsfloatval_t *grad, const int N, const lbfgsfloatval_t step ) {
		
			vector<OptData *> &optData = *(vector<OptData *> *)data;
			
			Pose pose;
			pose.rotation.init( ps[0], ps[1], ps[2], ps[3] );
			pose.rotation.norm();
			pose.translation.init( ps[4], ps[5], ps[6] );
			
			TransformMatrix PoseTM;
			PoseTM.init( pose );			
			
			lbfgsfloatval_t r=0;
			for( int i=0; i<(int)optData.size(); i++ ) {
				
				Pt<3> p3D;
				PoseTM.transform( p3D, optData[i]->coord3D );
				optData[i]->image->TM.inverseTransform( p3D, p3D );

				if( p3D[2] > 0 ) {

					Pt<2> p;
					p[0] = p3D[0]/p3D[2] * optData[i]->image->intrinsicLinearCalibration[0] + optData[i]->image->intrinsicLinearCalibration[2];
					p[1] = p3D[1]/p3D[2] * optData[i]->image->intrinsicLinearCalibration[1] + optData[i]->image->intrinsicLinearCalibration[3];
					
					p -= optData[i]->coord2D;
					
					r += p[0]*p[0]+p[1]*p[1];
					
				} else {
					
					r += -p3D[2] + 10;
				}
			}


			// calc grad
			for( int n=0; n<7; n++ ) {
				
				Pose poseG = pose;
				poseG[n] += max( 0.000001, abs(pose[n]*0.000001) );
				poseG.rotation.norm();
				
				TransformMatrix PoseTMG;
				PoseTMG.init( poseG );			

				grad[n]=0;
				for( int i=0; i<(int)optData.size(); i++ ) {

					Pt<3> p3D;
					PoseTMG.transform( p3D, optData[i]->coord3D );
					optData[i]->image->TM.inverseTransform( p3D, p3D );

					if( p3D[2] > 0 ) {

						Pt<2> p;
						p[0] = p3D[0]/p3D[2] * optData[i]->image->intrinsicLinearCalibration[0] + optData[i]->image->intrinsicLinearCalibration[2];
						p[1] = p3D[1]/p3D[2] * optData[i]->image->intrinsicLinearCalibration[1] + optData[i]->image->intrinsicLinearCalibration[3];
						
						p -= optData[i]->coord2D;
						
						grad[n] += p[0]*p[0]+p[1]*p[1];
						
					} else {
						
						grad[n] += -p3D[2] + 10;
					}
				}
				grad[n] = ( grad[n] - r ) / max( 0.000001, abs(pose[n]*0.000001) );
			}
			
			return r;
		}
		
		int optimizeCamera( Pose &pose, const vector<OptData *> &samples, const int maxOptTests ) {

			lbfgs_parameter_t param;
			lbfgsfloatval_t fx;
			lbfgsfloatval_t camPoseLM[7] = {
				pose.rotation[0], pose.rotation[1], pose.rotation[2], pose.rotation[3],
				pose.translation[0], pose.translation[1], pose.translation[2] };

			lbfgs_parameter_init(&param);
   
			// call lbgfs
			int retValue = lbfgs(7, camPoseLM, &fx, lbfgsFuncQuat, NULL, (void *)&samples, &param);
			
			pose.rotation.init( camPoseLM[0], camPoseLM[1], camPoseLM[2], camPoseLM[3] );
			pose.translation.init( camPoseLM[4], camPoseLM[5], camPoseLM[6] );
			
			pose.rotation.norm();
			// output is in camPoseLM
			return (retValue<=0)?1:-1;
		}
		
		void testAllPoints( vector<OptData *> &consistentCorresp, const Pose &pose, const vector<OptData *> &testPoints, const float ErrorThreshold ) {
			
			consistentCorresp.clear();
						
			foreach( corresp, testPoints ) {

				Pt<2> p = project( pose, corresp->coord3D, *corresp->image );
				p-=corresp->coord2D;
				
				float projectionError = sqrtf(p[0]*p[0]+p[1]*p[1]);
				
				if( projectionError < ErrorThreshold )
					consistentCorresp.push_back(corresp);
			}
		}
		
		void initPose( Pose &pose, const vector<OptData *> &samples ) {
			
			pose.rotation.init( (rand()&255)/256., (rand()&255)/256., (rand()&255)/256., (rand()&255)/256. );
			pose.translation.init( 0.,0.,0.5 );
		}
		
		bool RANSAC( Pose &pose, const vector<OptData *> &cluster ) {
		
			vector<OptData *> samples;
			for ( int nIters = 0; nIters<MaxRANSACTests; nIters++) {

				samples.clear();
				if( !randSample( samples, cluster, NPtsAlign ) ) return false;
				
				initPose( pose, samples );
					
				int OptIterations = optimizeCamera( pose, samples, MaxOptTests );
				if( OptIterations == -1 ) continue;
				
				vector<OptData *> consistent;
				testAllPoints( consistent, pose, cluster, ErrorThreshold );
				
				if ( (int)consistent.size() > MinNPtsObject ) {
					
					optimizeCamera( pose, consistent, MaxOptTests );
					return true;
				}		
			}
			return false;
		}

		void preprocessAllMatches( vector< vector< OptData > > &optData, const vector< vector< FrameData::Match > > &matches, const vector< SP_Image > &images ) {
			
			optData.resize( matches.size() );
			for(int model=0; model<(int)matches.size(); model++ )
				optData[model].resize( matches[model].size() );	
			
			
			vector< pair<int,int> > tasks; tasks.reserve(1000);
			for(int model=0; model<(int)matches.size(); model++ )
				for(int match=0; match<(int)matches[model].size(); match++ )
					tasks.push_back( make_pair(model, match) );
				
			#pragma omp parallel for
			for(int task=0; task<(int)tasks.size(); task++) {
				
				
				int model=tasks[task].first;
				int match=tasks[task].second;
				
				const SP_Image &image = images[ matches[model][match].imageIdx ];
				
				optData[model][match].image = image.get();
				optData[model][match].coord2D = matches[model][match].coord2D;
				optData[model][match].coord3D = matches[model][match].coord3D;
			}
		}

	public:

		POSE_RANSAC_LBFGS_REPROJECTION_CPU( int MaxRANSACTests, int MaxOptTests, int NObjectsCluster, int NPtsAlign, int MinNPtsObject, float ErrorThreshold )
		: MaxRANSACTests(MaxRANSACTests), MaxOptTests(MaxOptTests), NObjectsCluster(NObjectsCluster), NPtsAlign(NPtsAlign), MinNPtsObject(MinNPtsObject), ErrorThreshold(ErrorThreshold) {
		}

		void getConfig( map<string,string> &config ) const {
			
			GET_CONFIG( MaxRANSACTests );
			GET_CONFIG( MaxOptTests );
			GET_CONFIG( NObjectsCluster );
			GET_CONFIG( NPtsAlign );
			GET_CONFIG( MinNPtsObject );
			GET_CONFIG( ErrorThreshold );
		}
		
		void setConfig( map<string,string> &config ) {

			SET_CONFIG( MaxRANSACTests );
			SET_CONFIG( MaxOptTests );
			SET_CONFIG( NObjectsCluster );
			SET_CONFIG( NPtsAlign );
			SET_CONFIG( MinNPtsObject );
			SET_CONFIG( ErrorThreshold );
		}
		
		void process( FrameData &frameData ) {
			
			vector< SP_Image > &images = frameData.images;
			vector< vector< FrameData::Match > > &matches = frameData.matches;
			vector< vector< FrameData::Cluster > >&clusters = frameData.clusters;

			vector< vector< OptData > > optData;
			preprocessAllMatches( optData, matches, images );
			
			
			vector< pair<int,int> > tasks; tasks.reserve(1000);
			for(int model=0; model<(int)clusters.size(); model++ )
				for(int cluster=0; cluster<(int)clusters[model].size(); cluster++ )
					for(int obj=0; obj<NObjectsCluster; obj++)
						tasks.push_back( make_pair(model, cluster) );
			
				
			#pragma omp parallel for
			for(int task=0; task<(int)tasks.size(); task++) {
				
				int model=tasks[task].first;
				int cluster=tasks[task].second;
	
				vector<OptData *> clData;
				foreach( point, clusters[model][cluster] )
					clData.push_back( &optData[model][ point ] );

				Pose pose;
				bool found = RANSAC( pose, clData );
				
				if( found > 0 ) 
				#pragma omp critical(POSE)
				{
					SP_Object obj(new Object);
					frameData.objects->push_back(obj);
					
					obj->pose = pose;
					obj->model = (*models)[model];
				}
			}
			
			if( _stepName == "POSE" ) frameData.oldObjects = *frameData.objects;
		}
	};
};
