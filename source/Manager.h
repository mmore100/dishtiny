#pragma once

#include <algorithm>

#include "tools/Random.h"

#include "Config.h"
#include "ManagerApoptosis.h"
#include "ManagerChannel.h"
#include "ManagerFamily.h"
#include "ManagerInbox.h"
#include "ManagerPriority.h"
#include "ManagerStockpile.h"
#include "ManagerWave.h"

class Manager {

private:
  emp::vector<emp::Ptr<ManagerApoptosis>> mas;
  emp::vector<emp::Ptr<ManagerChannel>> mcs;
  emp::vector<emp::Ptr<ManagerFamily>> mfs;
  emp::vector<emp::Ptr<ManagerInbox>> mis;
  emp::vector<emp::Ptr<ManagerPriority>> mps;
  emp::vector<emp::Ptr<ManagerStockpile>> mss;
  emp::vector<emp::vector<emp::Ptr<ManagerWave>>> mws;


public:
  Manager(
    emp::vector<emp::Ptr<emp::Random>> &local_rngs,
    emp::vector<emp::Ptr<emp::Random>> &global_rngs,
    Config &cfg
  ) : mws(GeometryHelper(cfg).GetLocalSize()) {

    const size_t size = GeometryHelper(cfg).GetLocalSize();

    for(size_t i = 0; i < size; ++i) {
      mas.push_back(
        emp::NewPtr<ManagerApoptosis>()
      );
      mcs.push_back(
        emp::NewPtr<ManagerChannel>(cfg, *local_rngs[i])
      );
      mfs.push_back(
        emp::NewPtr<ManagerFamily>()
      );
      mis.push_back(
        emp::NewPtr<ManagerInbox>(cfg, *local_rngs[i])
      );
      mps.push_back(
        emp::NewPtr<ManagerPriority>(*local_rngs[i])
      );
      mss.push_back(
        emp::NewPtr<ManagerStockpile>(cfg)
      );
    }

    /* ManagerWaves part one */
    for(size_t i = 0; i < size; ++i) {
      for(size_t l = 0; l < cfg.NLEV(); ++l) {
        mws[i].push_back(
          emp::NewPtr<ManagerWave>(
            *mcs[i],
            *mss[i],
            l,
            i,
            *global_rngs[i],
            cfg
        ));
      }
    }

    /* ManagerWaves part two */
    for(size_t i = 0; i < size; ++i) {
      auto neigh_poses = GeometryHelper(cfg).CalcLocalNeighs(i);
      for(size_t l = 0; l < cfg.NLEV(); ++l) {
        emp::vector<emp::Ptr<ManagerWave>> res(neigh_poses.size());
        std::transform(
          neigh_poses.begin(),
          neigh_poses.end(),
          res.begin(),
          [this, l](size_t pos) { return mws[pos][l]; }
        );
        mws[i][l]->SetNeighs(res);
      }
    }

  }

  ~Manager() {
    for (auto &ptr : mas) ptr.Delete();
    for (auto &ptr : mcs) ptr.Delete();
    for (auto &ptr : mfs) ptr.Delete();
    for (auto &ptr : mis) ptr.Delete();
    for (auto &ptr : mps) ptr.Delete();
    for (auto &ptr : mss) ptr.Delete();
    for (auto &vec : mws) for (auto &ptr : vec) ptr.Delete();
  }

  ManagerApoptosis& Apoptosis(size_t pos) {
    return *mas[pos];
  }

  ManagerChannel& Channel(size_t pos) {
    return *mcs[pos];
  }

  ManagerFamily& Family(size_t pos) {
    return *mfs[pos];
  }

  ManagerInbox& Inbox(size_t pos) {
    return *mis[pos];
  }

  ManagerPriority& Priority(size_t pos) {
    return *mps[pos];
  }

  ManagerStockpile& Stockpile(size_t pos) {
    return *mss[pos];
  }

  ManagerWave& Wave(size_t pos, size_t lev) {
    return *mws[pos][lev];
  }

};
