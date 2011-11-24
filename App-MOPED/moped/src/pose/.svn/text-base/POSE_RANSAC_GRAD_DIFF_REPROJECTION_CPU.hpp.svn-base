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

	class POSE_RANSAC_GRAD_DIFF_REPROJECTION_CPU :public MopedAlg {
	
		int MaxRANSACTests;
		int MaxLMTests;
		int NPtsAlign;
		int MinNPtsObject;
		float ErrorThreshold;
		
		struct LmData {

			int imageIdx;
			Image *image;
			
			Pt<2> coord2D;
			Pt<3> coord3D;
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
		
		float testProjectPose( Pose pose, const vector<LmData *> &samples, int nDiscard ) {

			float errors[ samples.size() ];
			
			TransformMatrix PoseTM;
			pose.rotation.norm();
			PoseTM.init( pose );
			
			for( int i=0; i<(int)samples.size(); i++ ) {
				
				LmData *sample = samples[i];
				
				Pt<3> p3D;
				PoseTM.transform( p3D, sample->coord3D );
				sample->image->TM.inverseTransform( p3D, p3D );

				Pt<2> p;
				p[0] = p3D[0]/p3D[2] * sample->image->intrinsicLinearCalibration[0] + sample->image->intrinsicLinearCalibration[2];
				p[1] = p3D[1]/p3D[2] * sample->image->intrinsicLinearCalibration[1] + sample->image->intrinsicLinearCalibration[3];
			
				if( p3D[2] < 0 ) {
					 
					errors[i]  = 2*(-p3D[2] + 10)*(-p3D[2] + 10);
			
				} else {
					
					errors[i]  = (p[0] - sample->coord2D[0]) * (p[0] - sample->coord2D[0]);
					errors[i] += (p[1] - sample->coord2D[1]) * (p[1] - sample->coord2D[1]);
				}
			}
			
			sort( &errors[0], &errors[samples.size() ] );
			
			float error=0;
			for(int i=nDiscard; i<(int)samples.size()-nDiscard; i++ )
					error+=errors[i];
			
			return error;
		}
				
		float optimizeCamera( Pose &pose, const vector<LmData *> &samples, const int samplesSize, const int maxLMTests ) {

			float globalErr;
			
			for(int i=0; i<maxLMTests; i++ ) {
			
				globalErr = testProjectPose( pose, samples, 1 );

				Pt<7> grad;
				for(int i=0; i<7; i++ ) {
					
					pose[i] += 0.0001;
					grad[i] = (testProjectPose( pose, samples, 1 ) - globalErr);
					pose[i] -= 0.0001;
				}
				grad.norm();
				
				cout << grad << endl;
				
				//line search;
				float mn=0, fmn=globalErr;
				for(int x=0; x<20; x++ ) {
					float step = 0.00001;
					for(int z=0; z<20; z++) {
						float e = testProjectPose( pose - grad*(mn+step), samples, 1 );
						if( e >= fmn ) break;
						step*=2;
						fmn=e;
					}
					mn += step;
				}
				
				pose = pose-grad*mn;
				pose.rotation.norm();
			}
			return globalErr;
		}
				
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
		
		bool RANSAC( Pose &pose, const vector<LmData *> &cluster ) {
		
			vector<LmData *> samples;
			for ( int nIters = 0; nIters<MaxRANSACTests; nIters++) {

				samples.clear();
				if( !randSample( samples, cluster, NPtsAlign ) ) return false;
				
				initPose( pose, samples );
					
				int LMIterations = optimizeCamera( pose, samples, NPtsAlign, MaxLMTests );
				if( LMIterations == -1 ) continue;
				
				int nConsistentCorresp = testAllPoints( pose, cluster, ErrorThreshold );
				
				if ( nConsistentCorresp > MinNPtsObject ) {
					
					//optimizeCamera( pose, cluster, nConsistentCorresp, MaxLMTests );
					return true;
				}		
			}
			return false;
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
				
				lmData[model][match].image = images[ matches[model][match].imageIdx ].get();
				lmData[model][match].imageIdx = matches[model][match].imageIdx;
				lmData[model][match].coord2D = matches[model][match].coord2D;
				lmData[model][match].coord3D = matches[model][match].coord3D;
			}
		}

	public:

		POSE_RANSAC_GRAD_DIFF_REPROJECTION_CPU( int MaxRANSACTests, int MaxLMTests, int NPtsAlign, int MinNPtsObject, float ErrorThreshold )
		: MaxRANSACTests(MaxRANSACTests), MaxLMTests(MaxLMTests), NPtsAlign(NPtsAlign), MinNPtsObject(MinNPtsObject), ErrorThreshold(ErrorThreshold) {
		}

		void getConfig( map<string,string> &config ) const {
			
			GET_CONFIG( MaxRANSACTests );
			GET_CONFIG( MaxLMTests );
			GET_CONFIG( NPtsAlign );
			GET_CONFIG( MinNPtsObject );
			GET_CONFIG( ErrorThreshold );
		}
		
		void setConfig( map<string,string> &config ) {

			SET_CONFIG( MaxRANSACTests );
			SET_CONFIG( MaxLMTests );
			SET_CONFIG( NPtsAlign );
			SET_CONFIG( MinNPtsObject );
			SET_CONFIG( ErrorThreshold );
		}
		
		void process( FrameData &frameData ) {

			
			vector< SP_Image > &images = frameData.images;
			vector< vector< FrameData::Match > > &matches = frameData.matches;
			vector< vector< FrameData::Cluster > >&clusters = frameData.clusters;

			vector< vector< LmData > > lmData;
			preprocessAllMatches( lmData, matches, images );
			
			
			vector< pair<int,int> > tasks; tasks.reserve(1000);
			for(int model=0; model<(int)clusters.size(); model++ )
				for(int cluster=0; cluster<(int)clusters[model].size(); cluster++ )
					tasks.push_back( make_pair(model, cluster) );
			
				
			//#pragma omp parallel for
			for(int task=0; task<(int)tasks.size(); task++) {
				
				int model=tasks[task].first;
				int cluster=tasks[task].second;
	
				vector<LmData *> cl;
				foreach( point, clusters[model][cluster] )
					cl.push_back( & lmData[model][ point ] );

				Pose pose;
				bool found = RANSAC( pose, cl );
				
				if( found ) 
				#pragma omp critical(POSE)
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
