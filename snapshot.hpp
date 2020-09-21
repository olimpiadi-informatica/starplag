#pragma once

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

const size_t MAX_RESULTS = 500;

using info_t = std::tuple<float, std::string, std::string>;
using partial_t = std::set<info_t, std::greater<info_t>>;

template <typename T>
void save_snap(size_t index, const T &hi, const T &lo, std::string path) {
  std::string temp_snap = path + "_temp";
  std::ofstream snap(temp_snap);
  snap << index << " " << hi.size() << " " << lo.size() << std::endl;
  for (const auto &[a, b, c] : hi) {
    snap << a << " " << b << " " << c << std::endl;
  }
  for (const auto &[a, b, c] : lo) {
    snap << a << " " << b << " " << c << std::endl;
  }
  std::filesystem::rename(temp_snap, path);
}

void read_snap(const std::string &path, bool partial, size_t &resume_index,
               partial_t &partial_hi, partial_t &partial_lo) {
  float perc;
  std::string a, b;
  std::ifstream in(path);
  size_t index;
  in >> index;
  if (!partial || resume_index == std::numeric_limits<size_t>::max())
    resume_index = std::min(resume_index, index);
  size_t num_hi, num_lo;
  in >> num_hi >> num_lo;
  for (size_t i = 0; i < num_hi; i++) {
    in >> perc >> a >> b;
    partial_hi.emplace(perc, a, b);
  }
  for (size_t i = 0; i < num_lo; i++) {
    in >> perc >> a >> b;
    partial_lo.emplace(perc, a, b);
  }
};

void prune_extra_results(partial_t &partial) {
  if (partial.size() > MAX_RESULTS)
    partial.erase(std::next(partial.begin(), MAX_RESULTS), partial.end());
}
