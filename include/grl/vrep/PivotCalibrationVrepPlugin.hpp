#ifndef _PIVOT_CALIBRATION_VREP_PLUGIN_HPP_
#define _PIVOT_CALIBRATION_VREP_PLUGIN_HPP_

#include <iostream>
#include <memory>

#include <boost/exception/all.hpp>
#include <boost/algorithm/string.hpp>
#include <TRTK/PivotCalibration.hpp>

#include <spdlog/spdlog.h>

#include "grl/vrep/Eigen.hpp"
#include "grl/vrep/Vrep.hpp"

#include "v_repLib.h"

#include "grl/vector_ostream.hpp"

namespace grl {



/// Creates a complete vrep plugin object
/// usage:
/// @code
///    auto pivotCalib = std::make_shared<grl::PivotCalibrationVrepPlugin>();
///    pivotCalib->construct();
///    // move objects to new position
/// @endcode
///
/// @todo the rotation component may be different between the
class PivotCalibrationVrepPlugin : public std::enable_shared_from_this<PivotCalibrationVrepPlugin> {
public:

  static const bool debug = false;

    enum ParamIndex {
        ToolTipToModifyName,       ///<  V-REP handle string for the tip of the tool that will be *modified*
        ToolTipToMeasureName,      ///<  V-REP handle string for the tip of the tool that will be *measured*
        ToolBaseToModifyName,      ///<  V-REP handle string for the base of the tool that will be *modified*
        ToolBaseToMeasureName,     ///<  V-REP handle string for the base of the tool that will be *measured*
    };
    
    typedef std::tuple<
        std::string,
        std::string,
        std::string,
        std::string
        > Params;
    
    
    static const Params defaultParams(){
        return std::make_tuple(
                    "RobotMillTip"              , // ToolTipToModifyHandle,
                    "Fiducial#55"               , // ToolTipToMeasureHandle,
                    "RobotFlangeTip"            , // ToolTipHandle,
                    "Fiducial#22"                 // ToolBaseHandle,
                );
    }

/// @todo allow KukaFRIThreadSeparator parameters to be updated
PivotCalibrationVrepPlugin (Params params = defaultParams())
      :
      isFirstFrame(true),
      frameCount(0),
      params_(params)
{
/// @todo figure out how to re-enable when .so isn't loaded
 // initHandles();
  
}

/// construct() function completes initialization of the plugin
/// @todo move this into the actual constructor, but need to correctly handle or attach vrep shared libraries for unit tests.
void construct(){
  logger_ = spdlog::get("console");
  initHandles();
  
  // set the current transformEstimate to the initial estimate already in vrep
  if(allHandlesSet) transformEstimate = getObjectTransform(toolTip,toolBase);
}

/// Possible algorithms are:
///        TWO_STEP_PROCEDURE,         //!< First, estimate the pivot point in global coordinates. Second, estimate the sought translation by computing the difference vector between the pivot point and the center of the sensor system; then take the mean.\n This algorithm is advantageous in the case of zero-mean noise.
///        COMBINATORICAL_APPROACH     //!< Setup n equations that yield the pivot point from a single measurement using the sought (but unknown) translation. Eliminate the pivot point in the system of equations and finally solve for the sought translation. \n This algorithm is advantageous in the case of non-zero-mean noise (e.g. a systematic error).
///
/// If the algorithm string doesn't match an existing algorithm, this API will assume TWO_STEP_PROCEDURE.
void setAlgorithm(std::string algorithm_){
  algorithm = algorithm_;
  // if (boost::iequals(algorithm, std::string("COMBINATORICAL_APPROACH"))) algorithm = TRTK::PivotCalibration<double>::Algorithm::COMBINATORICAL_APPROACH;
  // else algorithm = TRTK::PivotCalibration<double>::Algorithm::TWO_STEP_PROCEDURE;
}

void addFrame() {
   logger_->info("Adding pivot calibration frame #", ++frameCount );
    
    auto toolTipInToolBase    = getObjectTransform(toolTip,toolBase);
    
    if(isFirstFrame){
      firstToolTipInToolBaseInverse             = toolTipInToolBase.inverse();
      isFirstFrame = false;
    }
    
    auto toolTipInFirstTipBase      = firstToolTipInToolBaseInverse * toolTipInToolBase;        // A_0_Inv * A_i
    
    
    rvecsArm.push_back(     toolTipInFirstTipBase.rotation().matrix());
    tvecsArm.push_back(     toolTipInFirstTipBase.translation()     );
    
    
   if(debug){
   
     logger_->info("\ntoolTipInToolBase:\n", poseString(toolTipInToolBase));
     
     logger_->info("\ntoolTipInFirstTipBase:\n", poseString(toolTipInFirstTipBase));
   
     // print simulation transfrom from tip to fiducial
     Eigen::Affine3d ToolTipToFiducial = getObjectTransform(toolTip,toolBase);
     logger_->info( "\n", poseString(ToolTipToFiducial,"expected ToolBaseToToolTip (simulation only): "));
   }
}

/// @brief run solver to estimate the unknown transform and set the simulation to the estimated value
///
/// after calling addFrame a number of times in different positions and orientations
///
/// @todo evaluate if applyEstimate should not be called by this
void estimatePivotOffset(){
  
   logger_->info("Running Pivot Calibration Estimate with the following numbers of entries in each category:  rvecsFiducial", " rvecsArm: ", rvecsArm.size(), " tvecsArm: ", tvecsArm.size() );

   BOOST_VERIFY(allHandlesSet);
  
   /// TODO(ahundt) use Ransac pivot calibration wrapper because there will likely be noise
   TRTK::PivotCalibrationTwoStep<double> pivotCalib;
   pivotCalib.setLocations(TRTK::make_range(tvecsArm));
   pivotCalib.setRotations(TRTK::make_range(rvecsArm));
   
   /// TODO(ahundt) Allow selection of "TWO_STEP_PROECDURE" and "COMBINATORIAL_APPROACH" from V-REP LUA API
   //pivotCalib.setAlgorithm("TWO_STEP_PROCEDURE");
   
   pivotCalib.compute();
   
   transformEstimate.translation() = pivotCalib.getPivotPoint();
    
    
    // get fiducial in optical tracker base frame
    int ret = simGetObjectPosition(toolTip, toolBase, detectedObjectPosition.begin());
    if(ret==-1) BOOST_THROW_EXCEPTION(std::runtime_error("PivotCalibrationVrepPlugin: Could not get position"));
    ret = simGetObjectQuaternion(toolTip, toolBase, detectedObjectQuaternion.begin());
    if(ret==-1) BOOST_THROW_EXCEPTION(std::runtime_error("PivotCalibrationVrepPlugin: Could not get quaternion"));

   if(debug){
     // print simulation transfrom from tip to fiducial
     Eigen::Affine3d ToolTipToFiducial = getObjectTransform(toolTip,toolBase);
     logger_->info( "\n", poseString(ToolTipToFiducial,"expected ToolBaseToToolTip (simulation only): "));
   }

   logger_->info( "\n", poseString(transformEstimate,"estimated toolBaseMeasured to toolTipMeasured:"));
   
   // print results
   Eigen::Quaterniond eigenQuat(transformEstimate.rotation());
   logger_->info( "Pivot Calibration Estimate quat wxyz\n: ", eigenQuat.w(), " ", eigenQuat.x(), " ", eigenQuat.y(), " ", eigenQuat.z(), " ", " translation xyz: ", transformEstimate.translation().x(), " ", transformEstimate.translation().y(), " ", transformEstimate.translation().z(), " ");
    
    logger_->info( "Optical Tracker Base Measured quat wxyz\n: ", detectedObjectQuaternion[0], " ", detectedObjectQuaternion[1], " ", detectedObjectQuaternion[2], " ", detectedObjectQuaternion[3], " ", " translation xyz: ", detectedObjectPosition[0], " ", detectedObjectPosition[1], " ", detectedObjectPosition[2], " ");
  
}

/// @brief  Will apply the stored estimate to the v-rep simulation value
///
/// A default transform is saved when construct() was called
/// so if no estimate has been found that will be used.
void applyEstimate(){

   // set transform between end effector and fiducial
   setObjectTransform(toolTipToModify, toolBaseToModify, transformEstimate);
}

Eigen::Affine3d getEstimate(){
   return transformEstimate;
}


/// @brief Correct modified transform for measured optical tracker data
///
/// If real time transform measurements are happening applyEstimate will move the measured object
/// relative to the sensor and this puts the transform measurement back so the object
/// appears in its real position. However, if running in simulation only
/// the sensor will be in the correct position, so you don't want to call this.
void restoreSensorPosition(){
   // set real optical tracker position and orientation relative to the robot fiducial
   simSetObjectPosition  (toolTip, toolBase, detectedObjectPosition.begin());
   simSetObjectQuaternion(toolTip, toolBase, detectedObjectQuaternion.begin());
}

private:



/// @todo create a function that calls simGetObjectHandle and throws an exception when it fails
/// @todo throw an exception if any of the handles is -1
void initHandles() {

	toolTip                         = grl::vrep::getHandleFromParam<ToolTipToMeasureName>                   (params_);					//Obtain ToolTip handle
	toolBase                        = grl::vrep::getHandleFromParam<ToolBaseToMeasureName>                  (params_);
	toolTipToModify                 = grl::vrep::getHandleFromParam<ToolTipToModifyName>                    (params_);					//Obtain ToolTip handle
	toolBaseToModify                = grl::vrep::getHandleFromParam<ToolBaseToModifyName>                   (params_);
    
	allHandlesSet  = true;
}

std::shared_ptr<spdlog::logger> logger_;
// TRTK::PivotCalibration<double>::Algorithm algorithm = TRTK::PivotCalibration<double>::Algorithm::TWO_STEP_PROCEDURE;
std::string algorithm = "TWO_STEP_PROCEDURE";

std::vector<typename TRTK::PivotCalibrationTwoStep<double>::Matrix3T > rvecsArm;
std::vector<typename TRTK::PivotCalibrationTwoStep<double>::Vector3T > tvecsArm;

bool isFirstFrame;
std::size_t frameCount;
Eigen::Affine3d firstToolTipInToolBaseInverse;

///
std::array<float,3> detectedObjectPosition;
std::array<float,4> detectedObjectQuaternion;
/// estimate transform between end effector and fiducial
Eigen::Affine3d transformEstimate;

int toolTip = -1;					//Obtain ToolTip handle
int toolBase = -1;
int toolTipToModify = -1;
int toolBaseToModify = -1;
//int ImplantCutPath = -1;
//int BallJointPath = -1;
//int bone = -1;

bool allHandlesSet = false;


private:
Params params_;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};
  
}

#endif
