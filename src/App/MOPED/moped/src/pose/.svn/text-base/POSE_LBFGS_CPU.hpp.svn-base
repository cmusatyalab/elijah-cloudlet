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

	class POSE_LBFGS_CPU :public MopedAlg {
	
		struct PreData {
			
			Image *image;
			Pt<2> coord2D;
			Pt<3> coord3D;
		};
		
		template<typename T>
		static void cartesian2sphere( T *c) {
/*			
			float r = sqrtf( c[0]*c[0] + c[1]*c[1] + c[2]*c[2] );
			float p = atan( c[1] / c[0] );
			float t = acos( c[2] / r);
			c[0]=r; c[1]=p; c[2]=t;			*/
		}
		
		template<typename T>
		static void sphere2cartesian( T *s) {
			
/*			float x = s[0]*cos(s[1])*sin(s[2]);
			float y = s[0]*sin(s[1])*sin(s[2]);
			float z = s[0]*cos(s[2]);
			s[0]=x; s[1]=y; s[2]=z;			*/
		};
		
		static double getScore2( Pose pose, vector< PreData > &preData, float scale, FrameData::Cluster &cluster = *(FrameData::Cluster *)NULL ) {
			
			double score = 0;
			pose.rotation.norm();
			TransformMatrix PoseTM;
			sphere2cartesian(&pose.translation[0]);
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
					
					score += 1;
				} else {
					
					Pt<2> p2D;
					p2D[0] = p3D[0]/p3D[2] * preData[n].image->intrinsicLinearCalibration[0] + preData[n].image->intrinsicLinearCalibration[2];
					p2D[1] = p3D[1]/p3D[2] * preData[n].image->intrinsicLinearCalibration[1] + preData[n].image->intrinsicLinearCalibration[3];
					
					p2D -= preData[n].coord2D;
					
					float boxX[6]={ 0., 0.866, 0.866, 0., -.866, -.866 };
					float boxY[6]={ 1., .5, -5, -1., -5, 5 };					

					double s = 0;
					for(int x=0; x<6; x++ ) {
						double px2 = p2D[0]+scale*boxX[x]; px2*=px2;
						double py2 = p2D[1]+scale*boxY[x]; py2*=py2;
						s=max(s, 1./((px2+py2)/(scale*scale)+1.));
					}
					score += s;
				}
			}
			return score;
		}


		static lbfgsfloatval_t lbfgsFuncQuat2( void *data, const lbfgsfloatval_t *lmPose, lbfgsfloatval_t *grad, const int N, const lbfgsfloatval_t step ) {
		
			vector<void *> &d = *(vector<void *> *)data;
			vector< PreData > &preData = *(vector< PreData > *)d[0];
			FrameData::Cluster &cluster = *(FrameData::Cluster *)d[1];
			float &scale = *(float *)d[2];
		
			Pose pose;
			pose.rotation.init( lmPose[0], lmPose[1], lmPose[2], lmPose[3] );
			pose.translation.init( lmPose[4], lmPose[5], lmPose[6] );
			
			double score = getScore2( pose, preData, scale, cluster );
			for( int n=0; n<7; n++ ) {
				
				double fdif = max( 0.001, abs((double)pose[n]*0.001) );

				double f = pose[n];
				pose[n] += fdif;
				grad[n] = ( getScore2( pose, preData, scale, cluster ) - score ) / fdif;
				pose[n] = f;
			}
			
			return score;
		}
	
		float optimizePose2( Pose &pose, vector< PreData > &preData, float scale, FrameData::Cluster &cluster = *(FrameData::Cluster *)NULL   ) {
			
			lbfgs_parameter_t param;
			lbfgs_parameter_init(&param);
			
			lbfgsfloatval_t fx;
			
			lbfgsfloatval_t camPoseLM[7] = {
				pose.rotation[0], pose.rotation[1], pose.rotation[2], pose.rotation[3],
				pose.translation[0], pose.translation[1], pose.translation[2] };

			cartesian2sphere(camPoseLM+4);

			lbfgs_parameter_init(&param);
   
			vector<void *> data(3);
			data[0] = (void *)&preData;
			data[1] = (void *)&cluster;
			data[2] = (void *)&scale;
   
			// call lbgfs
			int retValue = lbfgs(7, camPoseLM, &fx, lbfgsFuncQuat2, NULL, (void *)&data, &param);

			sphere2cartesian(camPoseLM+4);
			
			pose.rotation.init( camPoseLM[0], camPoseLM[1], camPoseLM[2], camPoseLM[3] );
			pose.translation.init( camPoseLM[4], camPoseLM[5], camPoseLM[6] );
			
			pose.rotation.norm();
			// output is in camPoseLM
			return (retValue<0 && retValue!=-1001)?retValue:-fx;
		}

		static Float getScore( Pose pose, vector< PreData > &preData, Float scale, FrameData::Cluster &cluster = *(FrameData::Cluster *)NULL ) {
			
			double score = 0;
			pose.rotation.norm();
			TransformMatrix PoseTM;
			sphere2cartesian(&pose.translation[0]);
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


		static lbfgsfloatval_t lbfgsFuncQuat( void *data, const lbfgsfloatval_t *lmPose, lbfgsfloatval_t *grad, const int N, const lbfgsfloatval_t step ) {
		
			vector<void *> &d = *(vector<void *> *)data;
			vector< PreData > &preData = *(vector< PreData > *)d[0];
			FrameData::Cluster &cluster = *(FrameData::Cluster *)d[1];
			Float &scale = *(Float *)d[2];
		
			Pose pose;
			pose.rotation.init( lmPose[0], lmPose[1], lmPose[2], lmPose[3] );
			pose.translation.init( lmPose[4], lmPose[5], lmPose[6] );
			
			Float score = -getScore( pose, preData, scale, cluster );
			for( int n=0; n<7; n++ ) {
				
				Float fdif = max( 0.0000001, abs(pose[n]*0.0000001) );

				Float f = pose[n];
				pose[n] += fdif;
				grad[n] = ( -getScore( pose, preData, scale, cluster ) - score ) / fdif;
				pose[n] = f;
			}
			
			return score;
		}
	
		float optimizePose( Pose &pose, vector< PreData > &preData, Float scale, FrameData::Cluster &cluster = *(FrameData::Cluster *)NULL   ) {
			
			lbfgs_parameter_t param;
			lbfgs_parameter_init(&param);
			
			lbfgsfloatval_t fx;
			
			lbfgsfloatval_t camPoseLM[7] = {
				pose.rotation[0], pose.rotation[1], pose.rotation[2], pose.rotation[3],
				pose.translation[0], pose.translation[1], pose.translation[2] };

			cartesian2sphere(camPoseLM+4);

			lbfgs_parameter_init(&param);
   
			vector<void *> data(3);
			data[0] = (void *)&preData;
			data[1] = (void *)&cluster;
			data[2] = (void *)&scale;
   
			// call lbgfs
			int retValue = lbfgs(7, camPoseLM, &fx, lbfgsFuncQuat, NULL, (void *)&data, &param);

			sphere2cartesian(camPoseLM+4);
			
			pose.rotation.init( camPoseLM[0], camPoseLM[1], camPoseLM[2], camPoseLM[3] );
			pose.translation.init( camPoseLM[4], camPoseLM[5], camPoseLM[6] );
			
			pose.rotation.norm();
			// output is in camPoseLM
			return (retValue<0 && retValue!=-1001)?retValue:-fx;
		}
		
		Pose distortPose( Pose pose, vector< PreData > &preData, FrameData::Cluster &cluster ) {
			
			pose.rotation.init( (rand()&1023)-512., (rand()&1023)-512., (rand()&1023)-512., (rand()&1023)-512. );
			pose.translation += 0.00001 * ( (rand()&1023)-512. );
			return pose;
		}
									
		Pose initPose( vector< PreData > &preData, FrameData::Cluster &cluster ) {
			
			Pose pose;
			pose.rotation.init( (rand()&1023)-512., (rand()&1023)-512., (rand()&1023)-512., (rand()&1023)-512. );
			pose.translation[0]+=(rand()&1023)/1024.-0.5;
			pose.translation[1]+=(rand()&1023)/1024.-0.5;
			pose.translation[2]+=(rand()&1023)/1024.-0.5;			
			
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

			int NDistorts = 1;
			float MinScore = 4.;
			
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
				object->score = 0;
				for(int i = 0; i < NDistorts; i++ ) {
					
					Pose newPose = distortPose( object->pose, preData[model], clusters[model][cluster] );
					
					Float newScore = 10E10;
					for( Float scale = 50.; scale>2. && newScore>MinScore; scale *=0.8 ) 
						newScore = optimizePose( newPose, preData[model], scale, clusters[model][cluster] );
				
					if( newScore > object->score ) {
						
						object->pose = newPose;
						object->score = newScore;
					}
				}
				
				//object->score = optimizePose2( object->pose, preData[model], 10000, clusters[model][cluster] );
				
				// OPTIMIZES POSE WITH ALL MERGED POINTS
				//object->score = optimizePose( object->pose, preData[model], 2 ); //(allow for 1 cm catches)
				
				// STORE OBJECT IF HAS SCORE > MinScore
				if( object->score > MinScore ) 
					#pragma omp critical(STOREPOSE)
					frameData.objects->push_back(object);
			}
		}
	};
};
