#pragma once

#include <unordered_set>

class ManagerFamily {

private:
  size_t cell_gen;
  size_t parent_pos;
  size_t birth_update;
  std::unordered_set<size_t> child_pos;
  Config::chanid_t prev_chan;

public:
  ManagerFamily()
  : cell_gen(0)
  , parent_pos(0)
  , birth_update(0)
  , prev_chan(0)
  { ; }

  void Reset(const size_t cur_update) {
    birth_update = cur_update;
    child_pos.clear();
  }

  size_t GetBirthUpdate() const { return birth_update; }

  size_t GetCellAge(const size_t cur_update) {
    return cur_update - birth_update;
  };

  bool IsParentPos(const size_t pos) const { return pos == parent_pos; }

  size_t GetParentPos() const { return parent_pos; }

  bool HasChildPos(const size_t pos) const {
    return child_pos.find(pos) != child_pos.end();
  }
  Config::chanid_t GetPrevChan() const { return prev_chan; }

  void SetPrevChan(const Config::chanid_t chan) { prev_chan = chan; }

  void SetParentPos(const size_t pos) { parent_pos = pos; }

  void AddChildPos(const size_t pos) { child_pos.insert(pos); }

  void SetCellGen(const size_t gen) { cell_gen = gen; }

  size_t GetCellGen() const { return cell_gen; }
};
