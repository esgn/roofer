
#include <memory>
#include <roofer/common/datastructures.hpp>
#include <roofer/misc/projHelper.hpp>

namespace roofer {
  struct PointCloudReaderInterface {
    projHelperInterface& pjHelper;

    PointCloudReaderInterface(projHelperInterface& pjh) : pjHelper(pjh){};
    virtual ~PointCloudReaderInterface() = default;

    virtual void open(const std::string& source) = 0;

    virtual void readPointCloud(PointCollection& points,
                                vec1i* classification = nullptr,
                                vec1i* order = nullptr,
                                vec1f* intensities = nullptr,
                                vec3f* colors = nullptr) = 0;
  };

  std::unique_ptr<PointCloudReaderInterface> createPointCloudReaderLASlib(
      projHelperInterface& pjh);
}  // namespace roofer
