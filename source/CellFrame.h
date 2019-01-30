#pragma once

#include <algorithm>

#include "tools/Random.h"

#include "Cardi.h"
#include "Config.h"
#include "FacingSet.h"
#include "GeometryHelper.h"

//forward declaration
class Manager;

class CellFrame {

private:

  emp::vector<double> endowments;
  FacingSet facing_set;
  Manager &man;
  emp::Random &local_rng;

  emp::vector<size_t> neighs;
  const size_t pos;

public:

  CellFrame(
    emp::Ptr<emp::Random> local_rng_,
    Config &cfg,
    Manager &man_,
    size_t pos_
  ) : endowments(cfg.NLEV()+1)
    , facing_set(*local_rng_, cfg)
    , man(man_)
    , local_rng(*local_rng_)
    , neighs(GeometryHelper(cfg).CalcLocalNeighs(pos))
    , pos(pos_)
  {
    Reset();
  }

  void Reset() {
    /* TODO why does this make everything run so fast */
    std::fill(endowments.begin(), endowments.end(), 0);
    facing_set.Spin(facing_set.GetAllFacings());
  }

  const size_t GetPos() {
    return pos;
  }

  FacingSet& GetFacingSet() {
    return facing_set;
  }

  Manager& GetManager() {
    return man;
  }

  const size_t GetNeigh(size_t dir) {
    return neighs[dir];
  }

  const double GetEndowment(size_t which) {
    return endowments[which];
  }

  void AdjEndowment(double amt, size_t which) {
    endowments[which] += amt;
    endowments[which] = std::max(endowments[which],0.0);
    emp_assert(endowments[which] >= 0);
  }

};