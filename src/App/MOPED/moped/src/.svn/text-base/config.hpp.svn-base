/*
 * LICENCE: BSD
 */

// Copyright: Carnegie Mellon University & Intel Corporation
// 
// Authors:
//  Alvaro Collet (alvaro.collet@gmail.com)
//  Manuel Martinez (salutte@gmail.com)
//  Siddhartha Srinivasa (siddhartha.srinivasa@intel.com)

#pragma once

#define MAX_THREADS 4

#include <util/UTIL_UNDISTORT.hpp>

#include <feat/FEAT_SIFT_GPU.hpp>
#include <feat/FEAT_SIFT_CPU.hpp>
#include <feat/FEAT_SURF_CPU.hpp>
#include <feat/FEAT_DISPLAY.hpp>

#include <match/MATCH_HASH_CPU.hpp>
//#include <match/MATCH_BRUTE_CPU.hpp>
#include <match/MATCH_ANN_CPU.hpp>
#include <match/MATCH_FLANN_CPU.hpp>
#include <match/MATCH_DISPLAY.hpp>

#include <cluster/CLUSTER_MEAN_SHIFT_CPU.hpp>
#include <cluster/CLUSTER_CUSTOM_CPU.hpp>
#include <cluster/CLUSTER_DISPLAY.hpp>

//#include <pose/POSE_RANSAC_LM_DIFF_CPU.hpp>
#include <pose/POSE_RANSAC_LM_DIFF_REPROJECTION_CPU.hpp>
//#include <pose/POSE_RANSAC_GRAD_DIFF_REPROJECTION_CPU.hpp>
#include <pose/POSE_RANSAC_LBFGS_REPROJECTION_CPU.hpp>

#include <pose/POSE_LBFGS_CPU.hpp>
//#include <pose/POSE_GRAD_CPU.hpp>

#include <pose/POSE_DISPLAY.hpp>

#include <filter/FILTER_PROJECTION_CPU.hpp>
//#include <filter/FILTER_DISPLAY.hpp>

#include <STATUS_DISPLAY.hpp>
#include <GLOBAL_DISPLAY.hpp>

#define DEFAULT_DISPLAY_LEVEL 1

//#define PERFORMANCE_TESTING_GPU 1
//#define PERFORMANCE_TESTING_CPU 1

namespace MopedNS {
	
	
	void createPipeline( MopedPipeline &pipeline ) {
		
		pipeline.addAlg( "UNDISTORTED_IMAGE", new UTIL_UNDISTORT );
		
		//FEAT_SURF_GPU( Float Threshold ) 
		//pipeline.addAlg( "SURF", new FEAT_SURF_GPU(64) );
		
		// FEAT_SIFT_GPU( string ScaleOrigin, string Verbosity, string GPUDisplay ) 
		//pipeline.addAlg( "SIFT", new FEAT_SIFT_GPU("-1", "0", ":0.0") );

#ifdef PERFORMANCE_TESTING_GPU 
		pipeline.addAlg( "SIFT", new FEAT_SIFT_GPU("-1", "0", ":0.0") );

#else
#ifdef PERFORMANCE_TESTING_CPU
		pipeline.addAlg( "SIFT", new FEAT_SIFT_CPU("-1") );
#else
		pipeline.addAlg( "SIFT", new FEAT_SIFT_GPU("-1", "0", ":0.0") );
#endif
#endif
		// FEAT_SIFT_CPU( string ScaleOrigin ) 

		
		//pipeline.addAlg( "FEAT_DISPLAY", new FEAT_DISPLAY( 2 ) );

		//MATCH_ANN_CPU( int DescriptorSize, string DescriptorType, float Quality, float Ratio )
		pipeline.addAlg( "MATCH_SIFT", new MATCH_ANN_CPU( 128, "SIFT", 5., 0.8) );
		//pipeline.addAlg( "MATCH_SURF", new MATCH_ANN_CPU(  64, "SURF", 5., 0.85) );
		//MATCH_FLANN_CPU( int DescriptorSize, string DescriptorType, float Precision, float Ratio )
		//pipeline.addAlg( "MATCH_SIFT", new MATCH_FLANN_CPU( 128, "SIFT", 0.01, 0.85) );
		//pipeline.addAlg( "MATCH_SURF", new MATCH_FLANN_CPU(  64, "SURF", 0.01, 0.85) );
		//pipeline.addAlg( "MATCH_SURF", new MATCH_FLANN_CPU(  64, "SURF", 0.8, 0.85) );
		//MATCH_HASH_CPU( int DescriptorSize, string DescriptorType, int NHashTables, int NDims, float Ratio, char Threshold, float SizeLimit )
		//pipeline.addAlg( "MATCH_SIFT", new MATCH_HASH_CPU( 128, "SIFT128", 64, 22, 0.8, 3, 16) );
		//MATCH_HASH_CPU( int DescriptorSize, string DescriptorType, int NHashTables, int NDims, float Ratio, char Threshold, float SizeLimit )
		//pipeline.addAlg( "MATCH_SURF", new MATCH_HASH_CPU( 64, "SURF", 64, 22, 0.8, 3, 16) );
		
		
		//MATCH_BRUTE_CPU( int DescriptorSize, string DescriptorType, float Ratio )
		//pipeline.addAlg( "CHECK_MATCH_SIFT128", new MATCH_BRUTE_CPU( 128, "SIFT128", 0.9) );
		
		//NO-DISP		pipeline.addAlg( "MATCH_DISPLAY", new MATCH_DISPLAY( DEFAULT_DISPLAY_LEVEL ) );

		//CLUSTER_MEAN_SHIFT_CPU( float Radius, float Merge, unsigned int MinPts, unsigned int MaxIterations ) 
		pipeline.addAlg( "CLUSTER", new CLUSTER_MEAN_SHIFT_CPU( 200, 20, 7, 100) );
		//NO-DISP		pipeline.addAlg( "CLUSTER_DISPLAY", new CLUSTER_DISPLAY( DEFAULT_DISPLAY_LEVEL ) );
		
		//POSE_RANSAC_LM_DIFF_CPU( int MaxRANSACTests, int MaxLMTests, int NPtsAlign, int MinNPtsObject, float ErrorThreshold )
		//pipeline.addAlg( "POSE", new POSE_RANSAC_LM_DIFF_REPROJECTION_CPU( 24, 200, 5, 6, 100) );
		
		
		//POSE_RANSAC_LBFGS_REPROJECTION_CPU( int MaxRANSACTests, int MaxOptTests, int NObjectsCluster, int NPtsAlign, int MinNPtsObject, float ErrorThreshold )
		//pipeline.addAlg( "POSE", new POSE_RANSAC_LBFGS_REPROJECTION_CPU( 250, 200, 4, 5, 6, 10) );
		pipeline.addAlg( "POSE", new POSE_RANSAC_LM_DIFF_REPROJECTION_CPU( 600, 200, 4, 5, 6, 10) );
		
		//NO-DISP		pipeline.addAlg( "POSE_DISPLAY", new POSE_DISPLAY( DEFAULT_DISPLAY_LEVEL ) );
		
		//FILTER_PROJECTION_CPU( int MinPoints, float FeatureDistance )
		pipeline.addAlg( "FILTER", new FILTER_PROJECTION_CPU( 5, 4096., 2) );
		
		//pipeline.addAlg( "POSE2", new POSE_RANSAC_LBFGS_REPROJECTION_CPU( 24, 500, 4, 6, 8, 2) );
		pipeline.addAlg( "POSE2", new POSE_RANSAC_LM_DIFF_REPROJECTION_CPU( 100, 500, 4, 6, 8, 5) );
		
		pipeline.addAlg( "FILTER2", new FILTER_PROJECTION_CPU( 7, 4096., 3) );

#ifndef PERFORMANCE_TESTING		
		//
		//pipeline.addAlg( "POSE2_DISPLAY", new POSE_DISPLAY( DEFAULT_DISPLAY_LEVEL ) );
#endif
		
		//		
		pipeline.addAlg( "STATUS_DISPLAY", new STATUS_DISPLAY( DEFAULT_DISPLAY_LEVEL ) );
		
#ifndef PERFORMANCE_TESTING
		//		
		//pipeline.addAlg( "GLOBAL_DISPLAY", new GLOBAL_DISPLAY( 2 ) );
#endif
	}
};
