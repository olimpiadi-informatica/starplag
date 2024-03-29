#pragma once

#include "root_subs.hpp"
#include "subs_dist.hpp"

bool is_template(const file_t &file) {
  return file.path.find("template") != std::string::npos;
}

float smart_dist(const file_t &file1, const file_t &file2, float space_weight = 0.3) {
  if (file1.group != file2.group && !is_template(file1) && !is_template(file2)) {
    return 0;
  }
  auto [subs, add_del_dist, space_dist, _1, _2, _3, _4] =
      root_subs(file1, file2);
  int token_dist = add_del_dist + subs_dist(subs);
  float token_perc =
      100 - 100.0 * token_dist / (file1.content.size() + file2.content.size());
  float space_perc =
      100 - 100.0 * space_dist / (file1.spaces.size() + file2.spaces.size());
  return token_perc * (1 - space_weight) + space_perc * space_weight;
}
