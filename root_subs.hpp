#pragma once

#include "file.hpp"
#include <algorithm>
#include <cstring>
#include <tuple>
#include <vector>

using subs_t = std::vector<std::vector<key_t>>;

struct root_subs_t {
  subs_t subs;
  size_t add_del_dist;
  size_t space_dist;
  diffs_t diff1;
  diffs_t diff2;
  diffs_t wdiff1;
  diffs_t wdiff2;
};

// Try to transform file1 into file2 by removing tokens or by substituing
// tokens. It returns the substitutions of each token.
root_subs_t root_subs(const file_t &file1, const file_t &file2) {
  size_t len1 = file1.content.size();
  size_t len2 = file2.content.size();
  std::vector<std::vector<size_t>> DP(len2 + 1, std::vector<size_t>(len1 + 1));
  for (size_t i = 0; i <= len1; i++)
    DP[0][i] = i;

  for (size_t i = 1; i <= len2; i++) {
    for (size_t j = 0; j <= len1; j++) {
      if (j == 0)
        DP[i][j] = i;
      else if (file1[j - 1] == file2[i - 1]) {
        DP[i][j] = DP[i - 1][j - 1];
      } else {
        DP[i][j] = 1 + std::min(DP[i - 1][j], DP[i][j - 1]);
        if (file1[j - 1] >= 0 && file2[i - 1] >= 0)
          DP[i][j] = std::min(DP[i][j], 1 + DP[i - 1][j - 1]);
      }
    }
  }

  size_t i1 = len1;
  size_t i2 = len2;
  size_t add_del_dist = 0;
  std::vector<key_t> s1, s2;
  std::vector<int> si1, si2;
  diffs_t diff1, diff2;
  while (i1 > 0 && i2 > 0) {
    if (file1[i1 - 1] == file2[i2 - 1]) {
      // same char
      if (file1[i1 - 1] >= 0) {
        s1.push_back(file1[--i1]);
        s2.push_back(file2[--i2]);
      } else {
        --i1;
        --i2;
      }
      si1.push_back(i1);
      si2.push_back(i2);
      diff1.push_back(diff_t::SAME);
      diff2.push_back(diff_t::SAME);
    } else if (DP[i2][i1] == DP[i2 - 1][i1 - 1] + 1 && file1[i1 - 1] >= 0 &&
               file2[i2 - 1] >= 0) {
      // subst token (not symbol)
      s1.push_back(file1[--i1]);
      s2.push_back(file2[--i2]);
      si1.push_back(i1);
      si2.push_back(i2);
      diff1.push_back(diff_t::CHANGED);
      diff2.push_back(diff_t::CHANGED);
    } else {
      // add/delete
      if (DP[i2][i1] == DP[i2 - 1][i1] + 1) {
        --i2;
        diff2.push_back(diff_t::ADDED);
      } else {
        --i1;
        diff1.push_back(diff_t::ADDED);
      }
      ++add_del_dist;
    }
  }
  add_del_dist += i1 + i2;
  while (i1--)
    diff1.push_back(diff_t::ADDED);
  while (i2--)
    diff2.push_back(diff_t::ADDED);

  std::reverse(s1.begin(), s1.end());
  std::reverse(s2.begin(), s2.end());
  std::reverse(diff1.begin(), diff1.end());
  std::reverse(diff2.begin(), diff2.end());

  si1.push_back(-1);
  si2.push_back(-1);
  std::reverse(si1.begin(), si1.end());
  std::reverse(si2.begin(), si2.end());
  si1.push_back(len1);
  si2.push_back(len2);

  subs_t subs(rev_mapping.size());

  for (size_t i = 0; i < rev_mapping.size(); i++) {
    subs[i].push_back(i);
  }
  for (size_t i = 0; i < s1.size(); i++) {
    subs[s1[i]].push_back(s2[i]);
  }

  size_t space_dist = len1 + len2 + 2;
  diffs_t wdiff1(file1.spaces.size(), diff_t::ADDED);
  diffs_t wdiff2(file2.spaces.size(), diff_t::ADDED);
  for (size_t i = 0; i < si1.size() - 1; i++) {
    if (si1[i] + 1 == si1[i + 1] && si2[i] + 1 == si2[i + 1]) {
      if (file1.spaces[si1[i + 1]] == file2.spaces[si2[i + 1]]) {
        space_dist -= 2;
        wdiff1[si1[i + 1]] = diff_t::SAME;
        wdiff2[si2[i + 1]] = diff_t::SAME;
      } else {
        wdiff1[si1[i + 1]] = diff_t::CHANGED;
        wdiff2[si2[i + 1]] = diff_t::CHANGED;
      }
    }
  }

  return {subs, add_del_dist, space_dist, diff1, diff2, wdiff1, wdiff2};
}

int edit_dist(const file_t &file1, const file_t &file2) {
  auto [subs, dist, _1, _2, _3, _4, _5] = root_subs(file1, file2);
  for (key_t i = 0; i < (key_t)subs.size(); i++) {
    for (key_t &k : subs[i]) {
      dist += (k != i);
    }
  }
  return dist;
}
