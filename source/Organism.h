#pragma once

#include <numeric>
#include <cmath>
#include <iostream>
#include <string>

#include "base/vector.h"
#include "base/assert.h"
#include "tools/Random.h"

#include "CustomConfig.h"
#include "DishtinyConfig.h"

class Organism {
private:

  // pointer for quick and efficient reproduction
  // (shared between all organisms)
  emp::Ptr<emp::Random> rand;

  // genetic information...
  // ... how much resource should I endow an offspring when I reproduce
  // at a particular hierarchical level
  emp::vector<double> endowments;
  // ... what proportions of resource should I place into channel pools of
  // different hierarchical levels and what proportion should I place into my
  // own stockpile? (values here are always normalized to sum to 1)
  emp::vector<double> res_pools;
  // ... with what probability should I avoid reproducing over cells
  // that share my channel on a particular level?
  // (1 = always avoid this , 0 = don't care )
  emp::vector<double> avoid_overs;
  // ... how big can the population of same-channel cells on a particular level
  // get before I decide to reproduce on a higher hierarchical level?
  emp::vector<double> off_ch_caps;
  // ... for a particular hierarchical level, should I prefer to place offspring close to the channel centroid?
  emp::vector<double> sort_offs;
  // ... should I try to kill myself if I was mutated during reproduction?
  emp::vector<double> damage_suicides;

  // pointer for quick and efficient reproduction
  // (shared between all organisms)
  emp::Ptr<CustomConfig> cconfig;

public:
  const static int ENDOWMENT_DEPTH_ADJ = 1;
  const static int RESPOOL_DEPTH_ADJ = 1;
  const static int AVOIDOVER_DEPTH_ADJ = 0;
  const static int OFFCHCAP_DEPTH_ADJ = 0;
  const static int SORTOFF_DEPTH_ADJ = 0;
  const static int DAMAGESUICIDE_DEPTH_ADJ = 1;

  Organism(
    emp::Ptr<emp::Random> _r,
    DishtinyConfig& dconfig,
    emp::Ptr<CustomConfig> _cconfig)
  : rand(_r)
  , endowments(dconfig.NLEV()+ENDOWMENT_DEPTH_ADJ)
  , res_pools(dconfig.NLEV()+RESPOOL_DEPTH_ADJ)
  , avoid_overs(dconfig.NLEV()+AVOIDOVER_DEPTH_ADJ)
  , off_ch_caps(dconfig.NLEV()+OFFCHCAP_DEPTH_ADJ)
  , sort_offs(dconfig.NLEV()+SORTOFF_DEPTH_ADJ)
  , damage_suicides(dconfig.NLEV()+DAMAGESUICIDE_DEPTH_ADJ)
  , cconfig(_cconfig)
  {

    // initialize genetic information
    for (size_t lev = 0; lev < GetEndowmentDepth(); ++lev) {
      endowments[lev] = init_endowment(lev);
    }
    for (size_t lev = 0; lev < GetResPoolDepth(); ++lev) {
      res_pools[lev] = init_res_pool(lev);
    }
    for (size_t lev = 0; lev < GetOffChCapDepth(); ++lev) {
      off_ch_caps[lev] = init_off_ch_cap(lev);
    }
    for (size_t lev = 0; lev < GetAvoidOverDepth(); ++lev) {
      avoid_overs[lev] = init_avoid_over(lev);
    }
    for (size_t lev = 0; lev < GetSortOffDepth(); ++lev) {
      sort_offs[lev] = init_sort_off(lev);
    }
    for (size_t lev = 0; lev < GetDamageSuicideDepth(); ++lev) {
      damage_suicides[lev] = init_damage_suicide(lev);
    }

    // balance res_pools so values sum to 1
    bal_res_pools();

  }

  /*
   * Read in an organism from a string.
   */

  Organism(
    std::string& in,
    emp::Ptr<emp::Random> _r,
    DishtinyConfig& dconfig,
    emp::Ptr<CustomConfig> _cconfig)
  : rand(_r)
  , endowments(dconfig.NLEV()+ENDOWMENT_DEPTH_ADJ)
  , res_pools(dconfig.NLEV()+RESPOOL_DEPTH_ADJ)
  , avoid_overs(dconfig.NLEV()+AVOIDOVER_DEPTH_ADJ)
  , off_ch_caps(dconfig.NLEV()+OFFCHCAP_DEPTH_ADJ)
  , sort_offs(dconfig.NLEV()+SORTOFF_DEPTH_ADJ)
  , damage_suicides(dconfig.NLEV()+DAMAGESUICIDE_DEPTH_ADJ)
  , cconfig(_cconfig)
  {

    std::string str = in;

    //seed
    emp::string_pop(str, ",");

    //update
    emp::string_pop(str, ",");


    // read in genetic information
    for (size_t i = 0; i < endowments.size(); ++i) {
      endowments[i]=std::stod(emp::string_pop(str, ","));
    }

    for (size_t i = 0; i < res_pools.size(); ++i) {
      res_pools[i]=std::stod(emp::string_pop(str, ","));
    }

    for (size_t i = 0; i < avoid_overs.size(); ++i) {
      avoid_overs[i]=std::stod(emp::string_pop(str, ","));
    }

    for (size_t i = 0; i < off_ch_caps.size(); ++i) {
      off_ch_caps[i]=std::stod(emp::string_pop(str, ","));
    }

    for (size_t i = 0; i < sort_offs.size(); ++i) {
      sort_offs[i]=std::stod(emp::string_pop(str, ","));
    }

    for (size_t i = 0; i < damage_suicides.size(); ++i) {
      damage_suicides[i]=std::stod(emp::string_pop(str, ","));
    }

  }

  /*
   * Make an exact copy of a parent Organism.
   */
  Organism(const Organism& par)
  : rand(par.rand)
  , endowments(par.endowments.size())
  , res_pools(par.res_pools.size())
  , avoid_overs(par.avoid_overs.size())
  , off_ch_caps(par.off_ch_caps.size())
  , sort_offs(par.sort_offs.size())
  , damage_suicides(par.damage_suicides.size())
  , cconfig(par.cconfig)
  {
    // initialize genetic information from parent
    for (size_t lev = 0; lev < endowments.size(); ++lev) {
      endowments[lev] = par.endowments[lev];
    }

    for (size_t lev = 0; lev < res_pools.size(); ++lev) {
      res_pools[lev] = par.res_pools[lev];
    }

    for (size_t lev = 0; lev < avoid_overs.size(); ++lev) {
      avoid_overs[lev] = par.avoid_overs[lev];
    }

    for (size_t lev = 0; lev < off_ch_caps.size(); ++lev) {
      off_ch_caps[lev] = par.off_ch_caps[lev];
    }

    for (size_t lev = 0; lev < sort_offs.size(); ++lev) {
      sort_offs[lev] = par.sort_offs[lev];
    }

    for (size_t lev = 0; lev < damage_suicides.size(); ++lev) {
      damage_suicides[lev] = par.damage_suicides[lev];
    }

  }

  /*
   * Equality determined by content.
   */
  inline bool operator<(const Organism& other) const {


    for (size_t lev = 0; lev < endowments.size(); ++lev) {
      if (endowments[lev] != other.endowments[lev]) {
        return endowments[lev] < other.endowments[lev];
      }
    }

    for (size_t lev = 0; lev < res_pools.size(); ++lev) {
      if (res_pools[lev] != other.res_pools[lev]) {
        return res_pools[lev] < other.res_pools[lev];
      }
    }

    for (size_t lev = 0; lev < avoid_overs.size(); ++lev) {
      if (avoid_overs[lev] != other.avoid_overs[lev]) {
        return avoid_overs[lev] < other.avoid_overs[lev];
      }
    }

    for (size_t lev = 0; lev < off_ch_caps.size(); ++lev) {
      if (off_ch_caps[lev] != other.off_ch_caps[lev]) {
        return off_ch_caps[lev] < other.off_ch_caps[lev];
      }
    }

    for (size_t lev = 0; lev < sort_offs.size(); ++lev) {
      if (sort_offs[lev] != other.sort_offs[lev]) {
        return sort_offs[lev] < other.sort_offs[lev];
      }
    }

    for (size_t lev = 0; lev < damage_suicides.size(); ++lev) {
      if (damage_suicides[lev] != other.damage_suicides[lev]) {
        return damage_suicides[lev] < other.damage_suicides[lev];
      }
    }

    return false;
  }

  /*
   * Equality determined by content.
   */
  inline bool operator==(const Organism& other) const {

    bool res = true;

    for (size_t lev = 0; lev < endowments.size(); ++lev) {
      res &= endowments[lev] == other.endowments[lev];
    }

    for (size_t lev = 0; lev < res_pools.size(); ++lev) {
      res &= res_pools[lev] == other.res_pools[lev];
    }

    for (size_t lev = 0; lev < avoid_overs.size(); ++lev) {
      res &= avoid_overs[lev] == other.avoid_overs[lev];
    }

    for (size_t lev = 0; lev < off_ch_caps.size(); ++lev) {
      res &= off_ch_caps[lev] == other.off_ch_caps[lev];
    }

    for (size_t lev = 0; lev < sort_offs.size(); ++lev) {
      res &= sort_offs[lev] == other.sort_offs[lev];
    }

    for (size_t lev = 0; lev < damage_suicides.size(); ++lev) {
      res &= damage_suicides[lev] == other.damage_suicides[lev];
    }

    return res;
  }

  /*
   * Equality determined by content.
   */
  bool operator!=(const Organism& other) const {
    return !(operator==(other));
  }

  /*
   * Potentially mutate genetic information of this Organism. (Whether mutation
   * is actually performed is determined by RNG results.) Return true if
   * muation occured else return false.
   */
  inline bool DoMutations(emp::Random& x) {

    bool mutation_occured = false;

    // mutate genetic information
    for (size_t lev = 0; lev < endowments.size(); ++lev) {
      mutation_occured |= mut_endowment( lev);
    }

    for (size_t lev = 0; lev < res_pools.size(); ++lev) {
      mutation_occured |= mut_res_pool(lev);
    }

    for (size_t lev = 0; lev < avoid_overs.size(); ++lev) {
      mutation_occured |= mut_avoid_over(lev);
    }

    for (size_t lev = 0; lev < off_ch_caps.size(); ++lev) {
      mutation_occured |= mut_off_ch_cap(lev);
    }

    for (size_t lev = 0; lev < sort_offs.size(); ++lev) {
      mutation_occured |= mut_sort_off(lev);
    }

    for (size_t lev = 0; lev < damage_suicides.size(); ++lev) {
      mutation_occured |= mut_damage_suicide(lev);
    }

    bal_res_pools();

    return mutation_occured;

  }

  /*
   * No cleanup required --- random generator, config pointers are shared
   * between all organisms.
   */
  ~Organism() { ; }

  /*
   * Accessor function.
   */
  inline double GetEndowment(size_t lev) const {
    return endowments[lev];
  }

  /*
   * Accessor function.
   */
  inline size_t GetEndowmentDepth() const {
    return endowments.size();
  }

  /*
   * Accessor function.
   */
  inline double GetResPool(size_t lev) const {
    return res_pools[lev];
  }

  /*
   * Accessor function.
   */
  inline size_t GetResPoolDepth() const {
    return res_pools.size();
  }

  /*
   * Accessor function.
   */
  inline double GetAvoidOver(size_t lev) const {
    return avoid_overs[lev];
  }

  /*
   * Accessor function.
   */
  inline size_t GetAvoidOverDepth() const {
    return avoid_overs.size();
  }

  /*
   * Accessor function.
   */
  inline double GetSortOff(size_t lev) const {
    return sort_offs[lev];
  }

  /*
   * Accessor function.
   */
  inline size_t GetSortOffDepth() const {
    return sort_offs.size();
  }

  /*
   * Accessor function.
   */
  inline double GetOffChCap(size_t lev) const {
    return off_ch_caps[lev];
  }

  /*
   * Accessor function.
   */
  inline size_t GetOffChCapDepth() const {
    return off_ch_caps.size();
  }

  /*
   * Accessor function.
   */
  inline double GetDamageSuicide(size_t lev) const {
    return damage_suicides[lev];
  }

  /*
   * Accessor function.
   */
  inline size_t GetDamageSuicideDepth() const {
    return damage_suicides.size();
  }

private:
  /*
   * Make sure res_pools elements sum to 1.
   */
  inline void bal_res_pools() {
    // we can assume res_pools values are non-negative
    double s = 0.0;
    for (size_t i = 0; i < res_pools.size(); ++i) {
      s += res_pools[i];
    }
    if (s > 0) {
      for (size_t i = 0; i < res_pools.size(); ++i) {
        res_pools[i] /= s;
      }
    } else {
      for (size_t i = 0; i < res_pools.size(); ++i) {
        res_pools[i] = 1.0 / res_pools.size();
      }
    }

    emp_assert(
      std::abs(
        std::accumulate(res_pools.begin(), res_pools.end(), 0.0) - 1
      ) < 0.0001
    );
  }

  /*
   * Initialization function.
   */
  inline double init_off_ch_cap(size_t lev) {
    return rand->GetDouble(48.0);
  }

  /*
   * Initialization function.
   */
  inline double init_endowment(size_t lev) {
    return rand->GetDouble(3.0) * (lev+1) * (lev+1) * (lev+1) * (lev+1);
  }

  /*
   * Initialization function.
   */
  inline double init_res_pool(size_t lev) {
    return rand->GetDouble(1.0);
  }

  /*
   * Initialization function.
   */
  inline double init_avoid_over(size_t lev) {
    return std::max(std::min(rand->GetDouble(-0.5, 1.5), 1.0), 0.0);
  }

  /*
   * Initialization function.
   */
  inline double init_sort_off(size_t lev) {
    return std::max(std::min(rand->GetDouble(-0.5, 1.5), 1.0), 0.0);
  }

  /*
   * Initialization function.
   */
  inline double init_damage_suicide(size_t lev) {
    return std::max(std::min(rand->GetDouble(-0.5, 1.5), 1.0), 0.0);
  }

  /*
   * Draw double from random generator. If it says mutate value, return mutated
   * value. Else, return original value.
   */
  inline bool mut_off_ch_cap(size_t lev) {
    double cur = off_ch_caps[lev];
    if (rand->GetDouble() < cconfig->PM_OFF_CH_CAP[lev]) {
      off_ch_caps[lev] = std::max(cur + rand->GetRandNormal(0.0, 24.0), 0.0);
      return true;
    } else {
      return false;
    }
  }

  /*
   * Draw double from random generator. If it says mutate value, return mutated
   * value. Else, return original value.
   */
  inline bool mut_endowment(size_t lev) {
    double cur = endowments[lev];
    if (rand->GetDouble() < cconfig->PM_ENDOWMENT[lev]) {
      endowments[lev] = std::max(cur + rand->GetRandNormal(0.0, 10.0), 0.0);
      return true;
    } else {
      return false;
    }
  }

  /*
   * Draw double from random generator. If it says mutate value, return mutated
   * value. Else, return original value.
   */
  inline bool mut_res_pool(size_t lev) {
    double cur = res_pools[lev];
    if (rand->GetDouble() < cconfig->PM_RES_POOL[lev]) {
      res_pools[lev] = std::max(cur + rand->GetRandNormal(0.0, 0.2), 0.0);
      return true;
    } else {
      return false;
    }
  }

  /*
   * Draw double from random generator. If it says mutate value, return mutated
   * value. Else, return original value.
   */
  inline bool mut_avoid_over(size_t lev) {

    if (rand->GetDouble() < cconfig->PM_AVOID_OVER[lev]) {
      avoid_overs[lev] = init_avoid_over(lev);
      return true;
    } else {
      return false;
    }
  }

  /*
   * Draw double from random generator. If it says mutate value, return mutated
   * value. Else, return original value.
   */
  inline bool mut_sort_off(size_t lev) {

    if (rand->GetDouble() < cconfig->PM_SORT_OFF[lev]) {
      sort_offs[lev] = init_sort_off(lev);
      return true;
    } else {
      return false;
    }
  }

  /*
   * Draw double from random generator. If it says mutate value, return mutated
   * value. Else, return original value.
   */
  inline bool mut_damage_suicide(size_t lev) {

    if (rand->GetDouble() < cconfig->PM_DAMAGE_SUICIDE[lev]) {
      damage_suicides[lev] = init_damage_suicide(lev);
      return true;
    } else {
      return false;
    }
  }

};
