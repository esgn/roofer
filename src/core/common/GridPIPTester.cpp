// This file is part of gfp-building-reconstruction
// Copyright (C) 2018-2022 Ravi Peters

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <roofer/common/GridPIPTester.hpp>

namespace roofer {

  pGridSet build_grid(const vec3f& ring, int Grid_Resolution) {
    int size = ring.size();
    std::vector<pPipoint> pgon;
    for (auto pi : ring) {
      pgon.emplace_back(new Pipoint{pi[0], pi[1]});
    }
    pGridSet grid_set = new GridSet();
    // skip last point in the ring, ie the repetition of the first vertex
    GridSetup(&pgon[0], pgon.size(), Grid_Resolution, grid_set);
    for (int i = 0; i < size; i++) {
      // delete pgon[i];
    }
    return grid_set;
  }

  GridPIPTester::GridPIPTester(const LinearRing& polygon) {
    ext_gridset = build_grid(polygon, Grid_Resolution);
    for (auto& hole : polygon.interior_rings()) {
      hole_gridsets.push_back(build_grid(hole, Grid_Resolution));
    }
  }
  GridPIPTester::~GridPIPTester() {
    GridCleanup(ext_gridset);
    ext_gridset = nullptr;
    for (auto& h : hole_gridsets) {
      GridCleanup(h);
      h = nullptr;
    }
  }

  bool GridPIPTester::test(const arr3f& p) {
    pPipoint pipoint = new Pipoint{p[0], p[1]};
    bool inside = GridTest(ext_gridset, pipoint);
    if (inside) {
      for (auto& hole_gridset : hole_gridsets) {
        inside = inside && !GridTest(hole_gridset, pipoint);
        if (!inside) break;
      }
    }
    delete pipoint;
    return inside;
  }

}  // namespace roofer