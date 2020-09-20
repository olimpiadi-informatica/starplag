#include <iostream>

#include "file.hpp"
#include "root_subs.hpp"
#include "subs_dist.hpp"

int main(int argc, char **argv) {
  if (argc != 3 && argc != 6) {
    std::cerr << "Usage: " << argv[0] << " file1 file2 [diff1 diff2 meta]"
              << std::endl;
    return 1;
  }

  file_t file1(argv[1]);
  file_t file2(argv[2]);

  std::ofstream fdiff1(argc < 4 ? "/dev/stdout" : argv[3]);
  std::ofstream fdiff2(argc < 4 ? "/dev/stdout" : argv[4]);
  std::ofstream fmeta(argc < 4 ? "/dev/stdout" : argv[5]);

  std::cerr << "File 1: " << file1.content.size() << std::endl;
  std::cerr << "File 2: " << file2.content.size() << std::endl;

  const size_t THRESHOLD = 30000;
  if (file1.content.size() > THRESHOLD || file2.content.size() > THRESHOLD) {
    std::cout << 0.0 << std::endl;
    return 0;
  }

  auto perc_dist = [&](auto dist) {
    return 100 - 100.0 * dist / (file1.content.size() + file2.content.size());
  };

  auto [subs, add_del_dist, space_dist, diff1, diff2, wdiff1, wdiff2] =
      root_subs(file1, file2);

  file1.print(fdiff1, diff1, wdiff1);
  std::cerr << "------------------------------------------------------------\n";
  file2.print(fdiff2, diff2, wdiff2);

  int edit_distance = edit_dist(file1, file2);
  fmeta << "Edit dist: " << edit_distance << " (" << perc_dist(edit_distance)
        << "%)\t\t";

  int token_dist = add_del_dist + subs_dist(subs);
  fmeta << "Token dist: " << token_dist << " (" << perc_dist(token_dist)
        << "%)\t\t";
  fmeta << "Space dist: " << space_dist << " (" << perc_dist(space_dist)
        << "%)\n";
  double dist = token_dist * 0.7 + space_dist * 0.3;
  fmeta << "Dist: " << dist << " (" << perc_dist(dist) << "%)" << std::endl;
  std::cout << perc_dist(dist) << std::endl;
}
