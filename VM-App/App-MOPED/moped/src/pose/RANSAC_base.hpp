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

#include <moped.hpp>
#include <lm.h>

using namespace std;
using namespace MopedNS;

class PoseRANSAC_LM {
	
	string id;

	int MaxRANSACTests;
	unsigned int MaxObjectsPerCluster;
	int MaxLMTests;
	int NPtsAlign;
	int MinNPtsObject;
	float ErrorThreshold;
	
	// This function populates "samples" with nSamples references to object correspondences
	// The samples are randomly choosen, and aren't repeated
	bool randSample( vector<Object::Corresp *> &samples, Object &object, unsigned int nSamples) {
		
		samples.clear();
		
		// Do not add a correspondence which the same model point
		map< Model::IP *, int > used;
	
		// Create a vector of samples prefixed with a random int. The int has preference over the pointer when sorting the vector.
		vector< pair< double, Object::Corresp *> > randomSamples; randomSamples.reserve( object.correspondences.size() );
		foreach( corresp, object.correspondences ) {
			
			double rnd = rand();
			rnd *= 0.1 + corresp->matchingDistance;
			randomSamples.push_back( pair< double,Object::Corresp *>( rnd, &*corresp ) );
		}
		sort( randomSamples.begin(), randomSamples.end() );
		
		while( samples.size()<nSamples && randomSamples.size() ) {

			if( !used[ &*(randomSamples.begin()->second->modelIP) ] ) {
				
				used[ &*(randomSamples.begin()->second->modelIP) ] = 1;
				samples.push_back( randomSamples.begin()->second );
			}
			randomSamples.erase( randomSamples.begin() );
		}
		
		return samples.size() == nSamples;
	}
	
	static void lmfunc(float *pose, float *pts2D, int nPose, int nPts2D, void *data) {
		
		Object::Corresp **samples = &(*((vector<Object::Corresp *> *)data))[0];

		RotMat R = RotMat( Quat(pose[0], pose[1], pose[2] ) );

		Pt<3> translation(pose+3);	
			
		for( int i=0; i < nPts2D; i+=2 ) {

			Pt<3> pt3Drt;
			pt3Drt.muladd( R[0], samples[i>>1]->modelIP->coord[0] );
			pt3Drt.muladd( R[1], samples[i>>1]->modelIP->coord[1] );
			pt3Drt.muladd( R[2], samples[i>>1]->modelIP->coord[2] );
			pt3Drt+=translation;			
		
			if( pt3Drt[2] < 0 ) {
				
				pts2D[i]=pts2D[i+1]=10E10;
			} else {
				
				pts2D[i  ] = pt3Drt[0] / pt3Drt[2];
				pts2D[i+1] = pt3Drt[1] / pt3Drt[2];
			}
		}
	}
	
	static void lmjac(float *pose, float *jac, int nPose, int nPts2D, void *data) {
		

		Object::Corresp **samples = &(*((vector<Object::Corresp *> *)data))[0];

		RotMat R = RotMat( Quat(pose[0], pose[1], pose[2] ) );

		Pt<3> translation(pose+3);	
			
		int j=0;
		for( int i=0; i < nPts2D; i+=2 ) {

			Pt<3> pt3Drt;
			pt3Drt.muladd( R[0], samples[i>>1]->modelIP->coord[0] );
			pt3Drt.muladd( R[1], samples[i>>1]->modelIP->coord[1] );
			pt3Drt.muladd( R[2], samples[i>>1]->modelIP->coord[2] );
			
			float a = pt3Drt[0]+translation[0];
			float b = pt3Drt[1]+translation[1];
			float c = 1/(pt3Drt[2]+translation[2]);
			
			jac[j++]=-a*c*c*pt3Drt[1];//PhiX
			jac[j++]=c*(pt3Drt[2]+a*c*pt3Drt[0]);//PhyY
			jac[j++]=-c*pt3Drt[1];//PhyZ
			
			jac[j++]=c;//Dx
			jac[j++]=0;//Dy
			jac[j++]=-a*c*c;//Dz

			jac[j++]=-c*(pt3Drt[2]+b*c*pt3Drt[1]);//PhyX
			jac[j++]=b*c*c*pt3Drt[0];//PhiY
			jac[j++]=c*pt3Drt[0];//PhyZ

			jac[j++]=0;//Dx
			jac[j++]=c;//Dy
			jac[j++]=-b*c*c;//Dz

		}
	}
	
	
	int optimizeCamera( Object &object, vector<Object::Corresp *> &samples, int maxLMTests ) {
		
		// set up vector for LM
		float camPoseLM[6] = { 0, 0, 0,
			object.pose.translation[0], object.pose.translation[1], object.pose.translation[2] };

		object.pose.rotation.to3( camPoseLM[0], camPoseLM[1], camPoseLM[2] );		
		
		// LM expects pts2D as a single vector
		vector<float> pts2D(samples.size()*2);
		for( unsigned int i=0; i<pts2D.size(); i++)
			pts2D[i]=samples[i>>1]->calibratedImageCoord2D[i&1];

		float backPose[6];
		memcpy( backPose, camPoseLM, sizeof(float)*6);


		float info[LM_INFO_SZ];
		
		
		int retValue;
		// call levmar
		retValue = slevmar_dif(lmfunc, camPoseLM, &pts2D[0], 6, samples.size()*2, maxLMTests, NULL, info,
								NULL, NULL, (void *)&samples);
	
		float errdif=info[1];
	
		memcpy( camPoseLM, backPose, sizeof(float)*6);
		retValue = slevmar_der(lmfunc, lmjac, camPoseLM, &pts2D[0], 6, samples.size()*2, maxLMTests, NULL, info,
								NULL, NULL, (void *)&samples);

		float errder=info[1];
		
		cout << ((errdif>errder)?"DER":"DIF") << "\t" << errdif<< "\t" << errder << endl;
								
		Quat q( camPoseLM[0], camPoseLM[1], camPoseLM[2] );
		object.pose.rotation = q;
		object.pose.translation = Pt<3>(camPoseLM+3);	
		// output is in camPoseLM
		return retValue;
	}	

	void RANSAC(Object &object,	list<Object> &retObjects ) {

		vector<Object::Corresp *> samples;
		unsigned int maxIters = MaxRANSACTests;
		for ( unsigned int nIters = 0; retObjects.size() < MaxObjectsPerCluster && nIters < maxIters; nIters++) {

			if( !randSample( samples, object, NPtsAlign ) ) break;
			
			object.pose.rotation = Quat( (rand()&255)/256., (rand()&255)/256., (rand()&255)/256., (rand()&255)/256. );
			object.pose.translation = Pt<3>(0,0,0.5);
				
			int LMIterations = optimizeCamera( object, samples, MaxLMTests );
			
			if( LMIterations == -1 ) continue;
			
			int nConsistentPoints = object.project( ErrorThreshold );
			
			if ( nConsistentPoints > MinNPtsObject ) {
				
				retObjects.push_back( object );
				vector<Object::Corresp *> optimizer;
				foreach( corresp, retObjects.back().correspondences ) 
					if( corresp->projectedImageCoord2D.sqEuclDist(corresp->detectedImageCoord2D) > ErrorThreshold ) 
						corresp = retObjects.back().correspondences.erase( corresp );
					else 
						optimizer.push_back( &*corresp );
				
				optimizeCamera( retObjects.back(), optimizer, MaxLMTests );
				
				maxIters += (nIters+1)/retObjects.size();
			}		
		}
	} 

public:

	PoseRANSAC_LM( const string &nameId = "" ) : id(nameId) {
		
		if( id == "" )
			id = string(__FILE__).substr(4,strlen(__FILE__)-8);
		
		MaxRANSACTests = 200;
		MaxObjectsPerCluster = 12;
		MaxLMTests = 50;
		NPtsAlign = 5;
		MinNPtsObject = 7;
		ErrorThreshold = 5;
	}

	void getConfig( map<string,string> &config ) {
		
		GET_CONFIG( MaxRANSACTests );
		GET_CONFIG( MaxObjectsPerCluster );
		GET_CONFIG( MaxLMTests );
		GET_CONFIG( NPtsAlign );
		GET_CONFIG( MinNPtsObject );
		GET_CONFIG( ErrorThreshold );
	}
	
	void setConfig( map<string,string> &config ) {

		SET_CONFIG( MaxRANSACTests );
		SET_CONFIG( MaxObjectsPerCluster );
		SET_CONFIG( MaxLMTests );
		SET_CONFIG( NPtsAlign );
		SET_CONFIG( MinNPtsObject );
		SET_CONFIG( ErrorThreshold );
	}
	
	void process( list<Object> &objects ) {
	
		objects.sort();
		objects.reverse();
		list<Object> aggregateReturnObjects;
		
		vector<Object *> vObjects;
		foreach( obj, objects ) vObjects.push_back(&*obj);		
		#pragma omp parallel for
		for( int x=0; x<(int)vObjects.size(); x++) {
			list<Object> retObjects;	
			RANSAC(*vObjects[x], retObjects);
				
			#pragma omp critical(RANSAC_LM)
			aggregateReturnObjects.splice( aggregateReturnObjects.end(), retObjects );
		}
		objects.clear();
		objects.splice( objects.end(), aggregateReturnObjects );
	}
};
