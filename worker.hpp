#pragma once

#include "file.hpp"
#include "smart_dist.hpp"
#include "snapshot.hpp"
#include <atomic>
#include <chrono>
#include <queue>
#include <vector>

using queue_t = std::vector<info_t>;
using file_list_t = std::vector<std::vector<std::pair<file_t, float>>>;

const double SNAP_INTERVAL = 0;

double get_time() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
             .count() /
         1.0e9;
}

void worker(file_list_t *files_ptr, size_t cutoff,
            std::atomic<size_t> *global_pos, int wid,
            std::pair<queue_t, queue_t> *result, std::atomic<size_t> *progress,
            size_t *current_index, std::string targetdir) {
  queue_t &hi = result->first;
  queue_t &lo = result->second;
  const file_list_t &files = *files_ptr;

  auto last_snap = get_time();
  size_t &index = *current_index;

  for (index = (*global_pos)++; index < files.size(); index = (*global_pos)++) {
    auto now = get_time();
    if (now - last_snap > SNAP_INTERVAL) {
      last_snap = now;
      save_snap(index, hi, lo, targetdir + "/snap" + std::to_string(wid));
    }
    for (size_t j = index + 1; j < files.size(); j++) {
      info_t best = {-1, "", ""};
      for (const auto &[f1, p1] : files[index]) {
        for (const auto &[f2, p2] : files[j]) {
          float perc = 0.0;
          if (f1.group == f2.group)
            perc = smart_dist(f1, f2);
          // the solutions are more similar to a template than they are between
          // each other
          if (perc < p1 || perc < p2) {
            continue;
          }
          if (perc > std::get<0>(best)) {
            best = {perc, f1.path, f2.path};
          }
        }
      }
      *progress += files[index].size() * files[j].size();
      if (std::get<0>(best) >= 0) {
        auto &pq = index < cutoff ? hi : lo;
        pq.push_back(best);
        std::push_heap(pq.begin(), pq.end(), std::greater<info_t>());
        if (pq.size() > MAX_RESULTS) {
          std::pop_heap(pq.begin(), pq.end(), std::greater<info_t>());
          pq.pop_back();
        }
      }
    }
  }
  save_snap(files.size() - 1, hi, lo,
            targetdir + "/snap" + std::to_string(wid));
}
