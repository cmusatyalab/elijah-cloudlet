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

	class POSE_GRAD_CPU :public MopedAlg {
	
		struct PreData {
			
			Image *image;
			Pt<2> coord2D;
			Pt<3> coord3D;
		};
				
		static double getScore( Pose pose, vector< PreData > &preData, float scale, FrameData::Cluster &cluster = *(FrameData::Cluster *)NULL ) {
			
			double score = 0;
			pose.rotation.norm();
			TransformMatrix PoseTM;
			PoseTM.init( pose);
		
			typeof(cluster.begin()) it;
			if( &cluster != NULL ) it=cluster.begin();
			for( int i=0; i<(int)preData.size(); i++ ) {
				
				int n = i;
				if( &cluster != NULL ) {
					if( it == cluster.end() ) break;
					n = *it++;
				}
				
				Pt<3> p3D;
				PoseTM.transform( p3D, preData[n].coord3D );
				preData[n].image->TM.inverseTransform( p3D, p3D );
				
				if( p3D[2] < 0 ) {
					
					score += p3D[2];
				} else {
					
					Pt<2> p2D;
					p2D[0] = p3D[0]/p3D[2] * preData[n].image->intrinsicLinearCalibration[0] + preData[n].image->intrinsicLinearCalibration[2];
					p2D[1] = p3D[1]/p3D[2] * preData[n].image->intrinsicLinearCalibration[1] + preData[n].image->intrinsicLinearCalibration[3];
					
					p2D -= preData[n].coord2D;

					score += 1. / ((p2D[0]*p2D[0]+p2D[1]*p2D[1])/(scale*scale)+1.);
				}
			}
			return score;
		}
	
		float optimizePose( Pose &pose, vector< PreData > &preData, float _scale, FrameData::Cluster &cluster = *(FrameData::Cluster *)NULL   ) {
			
			double score;
			
			double oldGrad[7];
			for( int n=0; n<7; n++ ) oldGrad[n]=1.;

			double conj[7];
			for( int n=0; n<7; n++ ) conj[n]=0.;

			for( double scale = _scale; scale>4.; scale*=0.999 ) {

				double grad[7];
				score = getScore( pose, preData, scale, cluster );
				for( int n=0; n<7; n++ ) {
					
					double fdif = max( 0.01, abs((double)pose[n]*0.01) );

					float f = pose[n];
					pose[n] += fdif;
					grad[n] = ( getScore( pose, preData, scale, cluster ) - score ) / fdif;
					pose[n] = f;
				}

				double d=0;
				for( int n=0; n<7; n++ ) d += grad[n]*grad[n];
				d = 1./sqrt(d);
				for( int n=0; n<7; n++ ) grad[n]*=d;
				
				double Bpr = 0, BprD = 0;
				for( int n=0; n<7; n++ ) {
					Bpr  += grad[n]*(grad[n]-oldGrad[n]);
					BprD += oldGrad[n]*oldGrad[n];
					oldGrad[n]=grad[n];
				}
				
				double b = max(0., Bpr/BprD);
				for( int n=0; n<7; n++ ) 
					grad[n]=conj[n]=grad[n]+b*conj[n];
				
				d=0;
				for( int n=0; n<7; n++ ) d += grad[n]*grad[n];
				d = 1./sqrt(d);
				for( int n=0; n<7; n++ ) grad[n]*=d;
				
				Pose newPose, bestPose;
				bestPose = pose;
				
				for(double step=0.0001; step<2.; step*=2) {
					newPose = pose;
					for(int n=0; n<7; n++) newPose[n]+=grad[n]*step;
					newPose.rotation.norm();
					double newScore = getScore( newPose, preData, scale, cluster );
					if( newScore > score ) {
						bestPose = newPose;
						score = newScore;
					} else break;
				}
				
				pose = bestPose;
			}

			pose.rotation.norm();
			return (float)score;
		}
		
		Pose distortPose( Pose pose, vector< PreData > &preData, FrameData::Cluster &cluster ) {
			
			pose.rotation.init( (rand()&1023)-512., (rand()&1023)-512., (rand()&1023)-512., (rand()&1023)-512. );
			pose.translation += 0.00001 * ( (rand()&1023)-512. );
			return pose;
		}
									
		Pose initPose( vector< PreData > &preData, FrameData::Cluster &cluster ) {
			
			Pose pose;
			pose.rotation.init( (rand()&1023)-512., (rand()&1023)-512., (rand()&1023)-512., (rand()&1023)-512. );
			
			Pt<3> centroid3D;
			centroid3D.init(0.,0.);
			foreach( i, cluster )
				centroid3D += preData[i].coord3D;
			centroid3D /= (float)cluster.size();
			
			
			pose.translation.init( 0.,0.,1. );
			return pose;
		}

		void preprocessAllMatches( vector< vector< PreData > > &preData, const vector< vector< FrameData::Match > > &matches, const vector< SP_Image > &images ) {
			
			
			TransformMatrix TM[images.size()];
			
			
			#pragma omp parallel for
			for(int i=0; i<(int)images.size(); i++ )
				TM[i].init( images[i]->cameraPose );
			
			preData.resize( matches.size() );
			for(int model=0; model<(int)matches.size(); model++ )
				preData[model].resize( matches[model].size() );	
			
			vector< pair<int,int> > tasks; tasks.reserve(1000);
			for(int model=0; model<(int)matches.size(); model++ )
				for(int match=0; match<(int)matches[model].size(); match++ )
					tasks.push_back( make_pair(model, match) );
				
			#pragma omp parallel for
			for(int task=0; task<(int)tasks.size(); task++) {
				
				int model=tasks[task].first;
				int match=tasks[task].second;
				
				const SP_Image &image = images[ matches[model][match].imageIdx ];


				preData[model][match].image = image.get();
				preData[model][match].coord2D = matches[model][match].coord2D;
				preData[model][match].coord3D = matches[model][match].coord3D;
			}
		}

	public:
		
		void process( FrameData &frameData ) {

			int NDistorts = 4;
			float MinScore = 3;
			
			vector< SP_Image > &images = frameData.images;
			vector< vector< FrameData::Match > > &matches = frameData.matches;
			vector< vector< FrameData::Cluster > >&clusters = frameData.clusters;


			vector< vector< PreData > > preData;
			preprocessAllMatches( preData, matches, images );

			
			vector< pair<int,int> > tasks; tasks.reserve(1000);
			for(int model=0; model<(int)clusters.size(); model++ )
				for(int cluster=0; cluster<(int)clusters[model].size(); cluster++ )
					tasks.push_back( make_pair(model, cluster) );
			

			#pragma omp parallel for
			for(int task=0; task<(int)tasks.size(); task++) {
				
				int model=tasks[task].first;
				int cluster=tasks[task].second;
				
				SP_Object object(new Object);
				object->model = (*models)[model];
				
				
				// INIT THE POSE
				object->pose = initPose( preData[model], clusters[model][cluster] );
				
				// NDistorts TIMES: DISTORT THE POSE, OPTIMIZE, AND SUBSTITUTE IF BETTER
				/*object->score = 0;
				for(int i = 0; i < NDistorts; i++ ) {
					
					Pose newPose = distortPose( object->pose, preData[model], clusters[model][cluster] );
					float newScore = optimizePose( newPose, preData[model], 1000, clusters[model][cluster] );
					
					if( newScore > object->score ) {
						
						object->pose = newPose;
						object->score = newScore;
					}
				}*/
				object->score = optimizePose( object->pose, preData[model], 1000, clusters[model][cluster] );
				
				// OPTIMIZES POSE WITH ALL MERGED POINTS
				//object->score = optimizePose( object->pose, preData[model], 1000 ); //(allow for 1 cm catches)
				
				// STORE OBJECT IF HAS SCORE > MinScore
				//if( object->score > MinScore ) 
					#pragma omp critical(STOREPOSE)
					frameData.objects->push_back(object);
			}
		}
	};
};
