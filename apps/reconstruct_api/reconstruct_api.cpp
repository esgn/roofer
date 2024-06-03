#include <CGAL/IO/OBJ.h>
#include <roofer/roofer.h>

#include <roofer/io/PointCloudReader.hpp>
#include <roofer/io/VectorReader.hpp>
#include <roofer/misc/cgal_utils.hpp>

#include "argh.h"
#include "fmt/format.h"
#include "git.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Point_3<K> Point_3;

void print_help(std::string program_name) {
  // see http://docopt.org/
  fmt::print("Usage:\n");
  fmt::print("   {}", program_name);
  fmt::print("Options:\n");
  // std::cout << "   -v, --version                Print version information\n";
  fmt::print("   -h, --help                   Show this help message\n");
  fmt::print("   -V, --version                Show version\n");
  fmt::print("   -v, --verbose                Be more verbose\n");
}

void print_version() {
  fmt::print("roofer {} ({}{}{})\n", git_Describe(),
             std::strcmp(git_Branch(), "main")
                 ? ""
                 : fmt::format("{}, ", git_Branch()),
             git_AnyUncommittedChanges() ? "dirty, " : "", git_CommitDate());
}

int main(int argc, const char* argv[]) {
  auto cmdl = argh::parser({"-c", "--config"});

  cmdl.parse(argc, argv);
  std::string program_name = cmdl[0];

  std::string path_pointcloud = "./input.laz";
  std::string path_footprint = "./input.gpkg";
  float floor_elevation = -0.16899998486042023;

  bool verbose = cmdl[{"-v", "--verbose"}];
  bool version = cmdl[{"-V", "--version"}];

  if (cmdl[{"-h", "--help"}]) {
    print_help(program_name);
    return EXIT_SUCCESS;
  }
  if (version) {
    print_version();
    return EXIT_SUCCESS;
  }

  // if (verbose) {
  //   spdlog::set_level(spdlog::level::debug);
  // } else {
  //   spdlog::set_level(spdlog::level::warn);
  // }

  // read inputs
  auto pj = roofer::misc::createProjHelper();
  auto PointReader = roofer::io::createPointCloudReaderLASlib(*pj);
  auto VectorReader = roofer::io::createVectorReaderOGR(*pj);

  VectorReader->open(path_footprint);
  std::vector<roofer::LinearRing> footprints;
  VectorReader->readPolygons(footprints);

  PointReader->open(path_pointcloud);
  // spdlog::info("Reading pointcloud from {}", path_pointcloud);
  roofer::vec1i classification;
  roofer::PointCollection points, points_ground, points_roof;
  roofer::AttributeVecMap attributes;
  PointReader->readPointCloud(points, &classification);
  // spdlog::info("Read {} points", points.size());

  // split into ground and roof points
  for (size_t i = 0; i < points.size(); ++i) {
    if (2 == classification[i]) {
      points_ground.push_back(points[i]);
    } else if (6 == classification[i]) {
      points_roof.push_back(points[i]);
    }
  }
  // spdlog::info("{} ground points and {} roof points", points_ground.size(),
  // points_roof.size());

  // todo temp test get the footprint heights
  roofer::proj_tri_util::DT cdt_test;
  // get the DT of the terrain
  //  spdlog::info("Constructing DT from terrain and interpolating footprint
  //  elevations");
  int k = 0;
  for (auto& p : points_ground) {
    if (k % 200 == 0) {  // randomly thin
      roofer::proj_tri_util::DT::Point pt(p[0], p[1], p[2]);
      cdt_test.insert(pt);
    }
    ++k;
  }
  // write that terrain to obj
  roofer::proj_tri_util::write_cdt_to_obj(cdt_test, "terrain.obj");
  // interpolate from terrain pts to get footprint elevations
  for (auto& p : footprints.front()) {
    roofer::Point_2 pt(p[0], p[1]);
    auto elevation = roofer::proj_tri_util::interpolate_from_cdt(pt, cdt_test);
    p[2] = elevation;
  }

  // reconstruct
  // spdlog::info("Reconstructing LoD2.2");
  //  auto mesh_lod22 = roofer::reconstruct_single_instance(points_roof,
  //  points_ground, footprints.front(),
  //                                                        {.floor_elevation =
  //                                                        floor_elevation,
  //                                                         .override_with_floor_elevation
  //                                                         = true});
  auto mesh_lod22 = roofer::reconstruct_single_instance(
      points_roof, points_ground, footprints.front());

  /*
  spdlog::info("Reconstructing LoD1.3");
  auto mesh_lod13 = roofer::reconstruct_single_instance(points_roof,
  points_ground, footprints, floor_elevation, { .lod = 13, .lod13_step_height
  = 2. }); spdlog::info("Reconstructing LoD1.2"); auto mesh_lod12 =
  roofer::reconstruct_single_instance(points_roof, points_ground, footprints,
  floor_elevation,
                                                        {.lod = 12 });
                                                        */

  //  std::vector<roofer::Mesh> meshes = {mesh_lod22, mesh_lod13, mesh_lod12};
  std::vector<roofer::Mesh> meshes = {mesh_lod22};
  std::vector<std::string> names = {"lod22"};

  // spdlog::info("Outputting to OBJ files");
  // spdlog::info("Converting to CGAL mesh");
  int i = 0;
  for (auto& mesh : meshes) {
    // spdlog::info(names[i] + ": converting to CGAL mesh and outputting");
    CGAL::Surface_mesh<Point_3> cgalmesh =
        roofer::misc::Mesh2CGALSurfaceMesh<Point_3>(mesh);

    // spdlog::info("Writing to obj file");
    std::string filename("output_" + names[i++] + ".obj");
    CGAL::IO::write_OBJ(filename, cgalmesh);
  }
}
