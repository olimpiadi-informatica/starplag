#include "file.hpp"
#include "root_subs.hpp"
#include "subs_dist.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using info_t = std::tuple<float, std::string, std::string>;
using queue_t =
    std::priority_queue<info_t, std::vector<info_t>, std::greater<info_t>>;

const size_t MAX_RESULTS = 500;
const float SPACE_WEIGHT = 0.3;

float smart_dist(const file_t &file1, const file_t &file2) {
  auto [subs, add_del_dist, space_dist, _1, _2, _3, _4] =
      root_subs(file1, file2);
  int token_dist = add_del_dist + subs_dist(subs);
  float token_perc =
      100 - 100.0 * token_dist / (file1.content.size() + file2.content.size());
  float space_perc =
      100 - 100.0 * space_dist / (file1.spaces.size() + file2.spaces.size());
  return token_perc * (1 - SPACE_WEIGHT) + space_perc * SPACE_WEIGHT;
}

void worker(std::vector<std::vector<file_t>> *files_ptr, size_t cutoff,
            int nthreads, int wid, std::pair<queue_t, queue_t> *result,
            std::atomic<size_t> *progress) {
  queue_t &hi = result->first;
  queue_t &lo = result->second;
  const std::vector<std::vector<file_t>> &files = *files_ptr;

  for (size_t index = wid; index < files.size(); index += nthreads) {
    for (size_t j = index + 1; j < files.size(); j++) {
      info_t best = {-1, "", ""};
      for (const auto &f1 : files[index]) {
        for (const auto &f2 : files[j]) {
          float perc = smart_dist(f1, f2);
          if (perc > std::get<0>(best)) {
            best = {perc, f1.path, f2.path};
          }
        }
      }
      *progress += files[index].size() * files[j].size();
      if (std::get<0>(best) >= 0) {
        auto &pq = index < cutoff ? hi : lo;
        pq.push(best);
        if (pq.size() > MAX_RESULTS)
          pq.pop();
      }
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0] << " soldir ranking.txt cutoff target"
              << std::endl;
    return 1;
  }

  std::string soldir = argv[1];
  std::string ranking_path = argv[2];
  int cutoff = std::atoi(argv[3]);
  std::string target_path = argv[4];

  std::vector<std::string> ranking;
  std::ifstream ranking_file(ranking_path);
  std::string user;
  while (ranking_file >> user) {
    ranking.push_back(user);
  }

  std::vector<std::vector<file_t>> files(ranking.size());
  int tot = 0;

  for (size_t u = 0; u < ranking.size(); u++) {
    for (const auto &path :
         std::filesystem::directory_iterator(soldir + "/" + ranking[u])) {
      const size_t THRESHOLD = 32 * 1024;
      if (std::filesystem::file_size(path.path()) < THRESHOLD) {
        files[u].emplace_back(path.path());
        tot++;
      }
    }
  }
  mapping.clear();

  std::cerr << "Total files: " << tot << std::endl;

  int nthreads = std::thread::hardware_concurrency();
  std::vector<std::pair<queue_t, queue_t>> results(nthreads);
  std::vector<std::thread> threads;
  std::atomic<size_t> progress;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < nthreads; i++) {
    threads.emplace_back(worker, &files, cutoff, nthreads, i, &results[i],
                         &progress);
  }

  size_t num_pairs = 0;
  for (const auto &u : files) {
    num_pairs += u.size() * (tot - u.size());
  }
  num_pairs /= 2;

  while (progress < num_pairs) {
    if (progress) {
      int delta = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::high_resolution_clock::now() - start)
                      .count() /
                  1.0e9 * (num_pairs - progress) / progress;
      int h = delta / 3600;
      int m = (delta % 3600) / 60;
      int s = delta % 60;
      printf("\r%8ld / %ld (%6.2f%%) ETA %4d:%02d:%02d\r", progress.load(),
             num_pairs, progress * 100.0 / num_pairs, h, m, s);
      fflush(stdout);
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
  }
  printf("\n");

  for (int i = 0; i < nthreads; i++) {
    threads[i].join();
  }
}
