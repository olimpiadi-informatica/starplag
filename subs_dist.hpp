#pragma once

#include "root_subs.hpp"

static std::map<std::tuple<size_t, size_t, size_t>, int> dp;

int local_dist(const subs_t::value_type &v, size_t l, size_t r,
               const std::vector<int> &pointers, size_t base) {
  if (l == r)
    return 0;

  if (dp.count({l, r, base}))
    return dp[{l, r, base}];

  auto &ref = dp[{l, r, base}];
  if (v[l] == v[base])
    return ref = local_dist(v, l + 1, r, pointers, l);
  if (v[r - 1] == v[base])
    return ref = local_dist(v, l, r - 1, pointers, base);

  // overwrite base
  ref = 1 + local_dist(v, l + 1, r, pointers, l);

  // use base with split
  for (size_t split = pointers[base]; split < r; split = pointers[split]) {
    ref = std::min(ref, local_dist(v, l, split, pointers, base) +
                            local_dist(v, split + 1, r, pointers, split));
  }

  return ref;
}

int subs_dist(const subs_t &subs) {
  int dist = 0;
  for (key_t i = 0; i < (key_t)subs.size(); i++) {
    std::vector<int> pointers(subs[i].size());
    std::vector<int> last(mapping.size(), subs[i].size());
    for (int j = subs[i].size() - 1; j >= 0; --j) {
      pointers[j] = last[subs[i][j]];
      last[subs[i][j]] = j;
    }

    dp.clear();
    // note that subs[i][0] = i
    dist += local_dist(subs[i], 1, subs[i].size(), pointers, 0);
  }
  return dist;
}
