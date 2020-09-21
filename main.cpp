#include "file.hpp"
#include "root_subs.hpp"
#include "smart_dist.hpp"
#include "snapshot.hpp"
#include "subs_dist.hpp"
#include "worker.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

std::vector<std::string> read_ranking(std::string ranking_path) {
  std::vector<std::string> ranking;
  std::ifstream ranking_file(ranking_path);
  std::string user;
  while (ranking_file >> user) {
    ranking.push_back(user);
  }
  return ranking;
}

bool ensure_snap_same_task(const partial_t &partial, std::string soldir) {
  if (!partial.empty()) {
    auto &[perc, f1, f2] = *partial.begin();
    if (f1.rfind(soldir, 0) != 0) {
      std::cerr << "\033[41;37;1mWARNING!\033[0m The snapshot is using a "
                   "different directory with the solutions!"
                << std::endl;
      std::cerr
          << "The snapshot is using: "
          << std::filesystem::path(f1).parent_path().parent_path().string()
          << std::endl;
      std::cerr << "You are using: " << soldir << std::endl;
      std::cerr << "Are you sure to continue? [yn] " << std::flush;
      std::string prompt;
      std::cin >> prompt;
      if (prompt != "y") {
        std::cerr << "quitting..." << std::endl;
        return false;
      }
    }
  }
  return true;
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

  std::filesystem::create_directories(target_path);

  std::cerr << "Reading the ranking file..." << std::endl;
  std::vector<std::string> ranking = read_ranking(ranking_path);

  std::cerr << "Checking local snapshots..." << std::endl;
  partial_t partial_hi, partial_lo;
  size_t resume_index = std::numeric_limits<size_t>::max();

  for (auto &f : std::filesystem::directory_iterator(target_path)) {
    auto path = f.path();
    auto name = path.filename().string();
    if (name.find("_temp", name.size() - 5) != std::string::npos) {
      std::cerr << "Partial snapshot found... ignoring " << path << std::endl;
    } else if (name.rfind("snap", 0) == 0) {
      std::cerr << "Loading snapshot from " << path << std::endl;
      read_snap(path.string(), false, resume_index, partial_hi, partial_lo);
    }
  }
  if (std::filesystem::exists(target_path + "/partial")) {
    std::cerr << "Loading partial results from " << target_path + "/partial"
              << std::endl;
    read_snap(target_path + "/partial", true, resume_index, partial_hi,
              partial_lo);
  }
  if (resume_index == std::numeric_limits<size_t>::max())
    resume_index = 0;

  // prune partial results
  prune_extra_results(partial_hi);
  prune_extra_results(partial_lo);
  // make sure the snapshot used the same solution directory
  if (!ensure_snap_same_task(partial_hi, soldir)) {
    return 1;
  }

  // save partial results
  save_snap(resume_index, partial_hi, partial_lo, target_path + "/partial");

  std::cerr << "Starting from " << resume_index << std::endl;

  std::vector<std::vector<file_t>> files(ranking.size());
  int tot = 0;

  for (size_t u = 0; u < ranking.size(); u++) {
    for (const auto &path :
         std::filesystem::directory_iterator(soldir + "/" + ranking[u])) {
      const size_t THRESHOLD = 32 * 1024;
      if (std::filesystem::file_size(path.path()) < THRESHOLD) {
        files[u].emplace_back(path.path());
        if (u > resume_index)
          tot++;
      }
    }
  }
  // files are read, since we don't want to print them this mapping is useless
  mapping.clear();

  std::cerr << "Total files left: " << tot << std::endl;

  int nthreads = std::thread::hardware_concurrency();
  std::cerr << "Using " << nthreads << " threads" << std::endl;

  std::vector<std::pair<queue_t, queue_t>> results(nthreads);
  std::vector<std::thread> threads;
  std::vector<size_t> current_index(nthreads);
  std::atomic<size_t> progress(0), global_pos(resume_index);
  auto start = std::chrono::high_resolution_clock::now();

  // spawn all the workers
  for (int i = 0; i < nthreads; i++) {
    threads.emplace_back(worker, &files, cutoff, &global_pos, i, &results[i],
                         &progress, &current_index[i], target_path);
  }

  // meanwhile compute the total number of pairs to process
  size_t num_pairs = 0;
  for (size_t i = resume_index; i < files.size(); i++) {
    num_pairs += files[i].size() * (tot - files[i].size());
  }
  num_pairs /= 2;

  printf(" pairs %8ld / %ld (%6.2f%%) | user %4ld / %4ld\r", progress.load(),
         num_pairs, 0.0, global_pos.load() - nthreads, ranking.size());

  // UI loop, print the progress in one line using `\r` for going back to the
  // start of the line
  while (progress < num_pairs) {
    if (progress) {
      int delta = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::high_resolution_clock::now() - start)
                      .count() /
                  1.0e9 * (num_pairs - progress) / progress;
      int h = delta / 3600;
      int m = (delta % 3600) / 60;
      int s = delta % 60;
      printf("\033[J pairs %8ld / %ld (%6.2f%%) | user %4ld / %4ld | ETA "
             "%4d:%02d:%02d | cur",
             progress.load(), num_pairs, progress * 100.0 / num_pairs,
             global_pos.load() - nthreads, ranking.size(), h, m, s);
      for (auto index : current_index)
        printf(" %ld", index);
      printf("\r");
      fflush(stdout);
    }
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
  }
  printf(" pairs %8ld / %ld (%6.2f%%) | user %4ld / %4ld\033[J\n",
         progress.load(), num_pairs, 0.0, global_pos.load() - nthreads,
         ranking.size());

  // since progress >= num_pairs we know that all threads have finished
  for (auto &thread : threads) {
    thread.join();
  }

  // merge the partial results of each thread with the previous partial results
  for (auto &[hi, lo] : results) {
    partial_hi.insert(hi.begin(), hi.end());
    partial_lo.insert(lo.begin(), lo.end());
  }
  prune_extra_results(partial_hi);
  prune_extra_results(partial_lo);
  save_snap(global_pos.load(), partial_hi, partial_lo,
            target_path + "/partial");
  save_snap(global_pos.load(), partial_hi, partial_lo, target_path + "/total");
}
