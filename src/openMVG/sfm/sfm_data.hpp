#pragma once

#include <openMVG/types.hpp>
#include <openMVG/sfm/sfm_view.hpp>
#include <openMVG/sfm/Rig.hpp>
#include <openMVG/sfm/sfm_landmark.hpp>
#include <openMVG/geometry/pose3.hpp>
#include <openMVG/cameras/cameras.hpp>
#include <fstream>

#include <stdexcept>
#include <cassert>

namespace openMVG {
namespace sfm {

/// Define a collection of View
using Views = Hash_Map<IndexT, std::shared_ptr<View> >;

/// Define a collection of Pose (indexed by View::id_pose)
using Poses = Hash_Map<IndexT, geometry::Pose3>;

/// Define a collection of IntrinsicParameter (indexed by View::id_intrinsic)
using Intrinsics = Hash_Map<IndexT, std::shared_ptr<cameras::IntrinsicBase> >;

/// Define a collection of landmarks are indexed by their TrackId
using Landmarks = Hash_Map<IndexT, Landmark>;

/// Define a collection of Rig
using Rigs = std::map<IndexT, Rig>;

/// Contain all the information about a Bundle Adjustment loop
struct LocalBA_statistics
{
  LocalBA_statistics(const std::set<IndexT>& newlyResectedViewsId = std::set<IndexT>()) {newViewsId = newlyResectedViewsId;}
  
  // Parameters returned by Ceres:
  double time = 0.0;                          // spent time to solve the BA (s)
  std::size_t numSuccessfullIterations = 0;   // number of successfull iterations
  std::size_t numUnsuccessfullIterations = 0; // number of unsuccessfull iterations
  
  std::size_t numResidualBlocks = 0;          // num. of resiudal block in the Ceres problem
  
  double RMSEinitial = 0.0; // sqrt(initial_cost / num_residuals)
  double RMSEfinal = 0.0;   // sqrt(final_cost / num_residuals)
  
  // Parameters specifically used by Local BA:
  std::size_t numRefinedPoses = 0;           // number of refined poses among all the estimated views          
  std::size_t numConstantPoses = 0;          // number of poses set constant in the BA solver
  std::size_t numIgnoredPoses = 0;           // number of not added poses to the BA solver
  std::size_t numRefinedIntrinsics = 0;      // num. of refined intrinsics
  std::size_t numConstantIntrinsics = 0;     // num. of intrinsics set constant in the BA solver
  std::size_t numIgnoredIntrinsics = 0;      // num. of not added intrinsicsto the BA solver
  std::size_t numRefinedLandmarks = 0;       // num. of refined landmarks
  std::size_t numConstantLandmarks = 0;      // num. of landmarks set constant in the BA solver
  std::size_t numIgnoredLandmarks = 0;       // num. of not added landmarks to the BA solver
  
  std::map<int, std::size_t> map_distance_numCameras; // distribution of the cameras for each graph distance
  
  std::set<IndexT> newViewsId;  // index of the new views added (newly resected)
};

struct LocalBA_timeProfiler
{
  double graphUpdating = 0.0;
  double distMapsComputing = 0.0;
  double statesMapsComputing = 0.0;
  double adjusting = 0.0;
  double allLocalBA = 0.0;
  
  bool exportTimes(const std::string& filename)
  {
    std::ofstream os;
    os.open(filename, std::ios::app);
    os.seekp(0, std::ios::end); //put the cursor at the end
    if (!os.is_open())
    {
      OPENMVG_LOG_DEBUG("Unable to open the Time profiling file '" << filename << "'.");
      return false;
    }
    
    if (os.tellp() == 0) // 'tellp' return the cursor's position
    {
      // If the file does't exist: add a header.
      std::vector<std::string> header;
      header.push_back("graphUpdating (s)");
      header.push_back("distMapsComputing (s)"); 
      header.push_back("statesMapsComputing (s)"); 
      header.push_back("adjusting (s)"); 
      header.push_back("allLocalBA (s)"); 
      
      for (std::string & head : header)
        os << head << "\t";
      os << "\n"; 
    }
    
    os << graphUpdating << "\t"
       << distMapsComputing << "\t"
       << statesMapsComputing << "\t"
       << adjusting << "\t"
       << allLocalBA << "\t";
    
    os << "\n";
    os.close();
    return true;
  }
  
  void showTimes()
  {
    std::cout << "\n----- Local BA durations ------" << std::endl;
    std::cout << "graph updating : " << graphUpdating << " ms" << std::endl;
    std::cout << "dist. Maps Computing : " << distMapsComputing << " ms" << std::endl;
    std::cout << "states Maps Computing : " << statesMapsComputing << " ms" << std::endl;
    std::cout << "adjusting : " << adjusting << " ms" << std::endl;
    std::cout << "** all Local BA: " << allLocalBA << " ms" << std::endl;
    std::cout << "-------------------------------\n" << std::endl;
  }
};


/// Generic SfM data container
/// Store structure and camera properties:
class SfM_Data
{
public:
  /// Considered views
  Views views;
  /// Considered camera intrinsics (indexed by view.getIntrinsicId())
  Intrinsics intrinsics;
  /// Structure (3D points with their 2D observations)
  Landmarks structure;
  /// Controls points (stored as Landmarks (id_feat has no meaning here))
  Landmarks control_points;
  /// Root Views path
  std::string s_root_path;
  /// Feature folder path
  std::string _featureFolder;
  /// Matching folder path
  std::string _matchingFolder;

  // Operators

  bool operator==(const SfM_Data& other) const;

  // Accessors

  const Views& GetViews() const {return views;}
  const Poses& GetPoses() const {return _poses;}
  Poses& GetPoses() {return _poses;}
  const Rigs& getRigs() const {return _rigs;}
  Rigs& getRigs() {return _rigs;}
  const Intrinsics& GetIntrinsics() const {return intrinsics;}
  const Landmarks& GetLandmarks() const {return structure;}
  Landmarks& GetLandmarks() {return structure;}
  const Landmarks& GetControl_Points() const {return control_points;}
  const std::string& getFeatureFolder() const {return _featureFolder;}
  const std::string& getMatchingFolder() const {return _matchingFolder;}

  /**
   * @brief List the view indexes that have valid camera intrinsic and pose.
   * @return view indexes list
   */
  std::set<IndexT> getValidViews() const;

  /**
   * @brief List the intrinsic indexes that have valid camera intrinsic and pose.
   * @return intrinsic indexes list
   */
  std::set<IndexT> getReconstructedIntrinsics() const;
  
  /**
   * @brief Return a pointer to an intrinsic if available or nullptr otherwise.
   * @param[in] intrinsicId
   */
  const cameras::IntrinsicBase * GetIntrinsicPtr(IndexT intrinsicId) const
  {
    if(intrinsics.count(intrinsicId))
      return intrinsics.at(intrinsicId).get();
    return nullptr;
  }
  
  /**
   * @brief Return a pointer to an intrinsic if available or nullptr otherwise.
   * @param[in] intrinsicId
   */
  cameras::IntrinsicBase * GetIntrinsicPtr(IndexT intrinsicId)
  {
    if(intrinsics.count(intrinsicId))
      return intrinsics.at(intrinsicId).get();
    return nullptr;
  }

  /**
   * @brief Return a shared pointer to an intrinsic if available or nullptr otherwise.
   * @param[in] intrinsicId
   */
  std::shared_ptr<cameras::IntrinsicBase> GetIntrinsicSharedPtr(IndexT intrinsicId)
  {
    if(intrinsics.count(intrinsicId))
      return intrinsics.at(intrinsicId);
    return nullptr;
  }

  /**
   * @brief Get a set of views keys
   * @return set of views keys
   */
   std::set<IndexT> GetViewsKeys() const
  {
    std::set<IndexT> viewKeys;
    for(auto v: views)
      viewKeys.insert(v.first);
    return viewKeys;
  }
  
  /**
  * @brief GetIntrinsicsUsage
  * @return 
  */
  std::map<IndexT, std::size_t> GetIntrinsicsUsage() const
  {
    std::map<IndexT, std::size_t> map_intrinsicId_usageNum;
    
    for (const auto& itView : views)
    {
      const View * view = itView.second.get();
      
      if (IsPoseAndIntrinsicDefined(view))
      {
        auto itIntr = map_intrinsicId_usageNum.find(view->getIntrinsicId());
        if (itIntr == map_intrinsicId_usageNum.end())
          map_intrinsicId_usageNum[view->getIntrinsicId()] = 1;
        else
          map_intrinsicId_usageNum[view->getIntrinsicId()]++;
      }
    }
    return map_intrinsicId_usageNum;
  }
  
  //  const Landmarks & GetLandmarks() const {return structure;}
  //  const Landmarks & GetControl_Points() const {return control_points;}
  
  /**
   * @brief Check if the given view have defined intrinsic and pose
   * @param[in] view The given view
   * @return true if intrinsic and pose defined
   */
  bool IsPoseAndIntrinsicDefined(const View* view) const
  {
    if (view == nullptr)
      return false;
    return (
      view->getIntrinsicId() != UndefinedIndexT &&
      view->getPoseId() != UndefinedIndexT &&
      (!view->isPartOfRig() || getRigSubPose(*view).status != ERigSubPoseStatus::UNINITIALIZED) &&
      intrinsics.find(view->getIntrinsicId()) != intrinsics.end() &&
      _poses.find(view->getPoseId()) != _poses.end());
  }
  
  /**
   * @brief Check if the given view have defined intrinsic and pose
   * @param[in] viewID The given viewID
   * @return true if intrinsic and pose defined
   */
  bool IsPoseAndIntrinsicDefined(IndexT viewId) const
  { 
    return IsPoseAndIntrinsicDefined(views.at(viewId).get());
  }

  /**
   * @brief Check if the given view has an existing pose
   * @param[in] view The given view
   * @return true if the pose exists
   */
  bool existsPose(const View& view) const
  {
     return (_poses.find(view.getPoseId()) != _poses.end());
  }

  /**
   * @brief Gives the pose of the input view. If this view is part of a RIG, it returns rigPose + rigSubPose.
   * @param[in] view The given view
   */
  const geometry::Pose3 getPose(const View& view) const
  {
    // check the view has valid pose / rig etc
    if(!view.isPartOfRig())
    {
      return _poses.at(view.getPoseId());
    }

    // get the pose of the rig and the subpose of the camera
    const geometry::Pose3& rigPose = getRigPose(view);
    const geometry::Pose3& subPose = getRigSubPose(view).pose;

    // multiply rig pose by camera subpose
    return  subPose * rigPose;
  }

  /**
   * @brief Get the rig of the given view
   * @param[in] view The given view
   * @return rig of the given view
   */
  const Rig& getRig(const View& view) const
  {
    assert(view.isPartOfRig());
    return _rigs.at(view.getRigId());
  }
  
  void setFeatureFolder(const std::string& featureFolder)
  {
    _featureFolder = featureFolder;
  }

  void setMatchingFolder(const std::string& matchingFolder)
  {
    _matchingFolder = matchingFolder;
  }

  /**
   * @brief Set the given pose for the given view
   * if the view is part of a rig, this method update rig pose/sub-pose
   * @param[in] view The given view
   * @param[in] pose The given pose
   */
  void setPose(const View& view, const geometry::Pose3& pose);


  /**
   * @brief Set the given pose for the given poseId
   * @param[in] poseId The given poseId
   * @param[in] pose The given pose
   */
  void setAbsolutePose(IndexT poseId, const geometry::Pose3& pose)
  {
    _poses[poseId] = pose;
  }

  /**
   * @brief Erase yhe pose for the given poseId
   * @param[in] poseId The given poseId
   */
  void erasePose(IndexT poseId)
  {
    auto it =_poses.find(poseId);
    if(it != _poses.end())
      _poses.erase(it);
    else
      throw std::out_of_range(std::string("Can't erase unfind pose ") + std::to_string(poseId));
  }

  /**
   * @brief Reset rigs sub-poses parameters
   */
  void resetRigs()
  {
    for(auto rigIt : _rigs)
      rigIt.second.reset();
  }

private:

  /// Considered poses (indexed by view.getPoseId())
  Poses _poses;
  /// Considered rigs
  Rigs _rigs;

  /**
   * @brief Get Rig pose of a given camera view
   * @param[in] view The given view
   * @return Rig pose of the given camera view
   */
  const geometry::Pose3& getRigPose(const View& view) const
  {
    return _poses.at(view.getPoseId());
  }

  /**
   * @brief Get Rig subPose of a given camera view
   * @param[in] view The given view
   * @return Rig subPose of the given camera view
   */
  const RigSubPose& getRigSubPose(const View& view) const
  {
    assert(view.isPartOfRig());
    const Rig& rig = _rigs.at(view.getRigId());
    return rig.getSubPose(view.getSubPoseId());
  }

  /**
   * @brief Get Rig pose of a given camera view
   * @param[in] view The given view
   * @return Rig pose of the given camera view
   */
  geometry::Pose3& getRigPose(const View& view)
  {
    return _poses.at(view.getPoseId());
  }

  /**
   * @brief Get Rig subPose of a given camera view
   * @param[in] view The given view
   * @return Rig subPose of the given camera view
   */
  RigSubPose& getRigSubPose(const View& view)
  {
    assert(view.isPartOfRig());
    Rig& rig = _rigs.at(view.getRigId());
    return rig.getSubPose(view.getSubPoseId());
  }
};

/**
 * @brief ColorizeTracks Add the associated color to each 3D point of
 * the sfm_data, using the track to determine the best view from which
 * to get the color.
 * @param sfm_data The container of the data
 * @return true if everything went well
 */
bool ColorizeTracks( SfM_Data & sfm_data );

} // namespace sfm
} // namespace openMVG
