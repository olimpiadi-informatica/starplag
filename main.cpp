#include <iostream>

#include "file.hpp"
#include "root_subs.hpp"
#include "subs_dist.hpp"

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " file1 file2" << std::endl;
    return 1;
  }

  file_t file1(argv[1]);
  file_t file2(argv[2]);

  auto perc_dist = [&](int dist) {
    return 100 - 100.0 * dist / (file1.content.size() + file2.content.size());
  };

  auto [subs, add_del_dist, diff1, diff2] = root_subs(file1, file2);

  file1.print(diff1);
  std::cerr << "-------------------------------------------------------------------\n";
  file2.print(diff2);

  int edit_distance = edit_dist(file1, file2);
  std::cerr << "Edit dist: " << edit_distance << " ("
            << perc_dist(edit_distance) << "%)" << std::endl;

  int dist = add_del_dist + subs_dist(subs);
  std::cerr << "Dist: " << dist << " (" << perc_dist(dist) << "%)" << std::endl;
  std::cout << perc_dist(dist) << std::endl;
}
