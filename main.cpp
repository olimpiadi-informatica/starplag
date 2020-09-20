#include "file.hpp"
#include "root_subs.hpp"
#include "subs_dist.hpp"
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

using info_t = std::tuple<float, std::string, std::string>;
using queue_t = std::vector<info_t>;

const size_t MAX_RESULTS = 500;
const double SNAP_INTERVAL = 0;
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

double get_time() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
             .count() /
         1.0e9;
}

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

void worker(std::vector<std::vector<file_t>> *files_ptr, size_t cutoff,
            std::atomic<size_t> *global_pos, int wid,
            std::pair<queue_t, queue_t> *result, std::atomic<size_t> *progress,
            size_t *current_index, std::string targetdir) {
  queue_t &hi = result->first;
  queue_t &lo = result->second;
  const std::vector<std::vector<file_t>> &files = *files_ptr;

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
        pq.push_back(best);
        std::push_heap(pq.begin(), pq.end(), std::greater<info_t>());
        if (pq.size() > MAX_RESULTS) {
          std::pop_heap(pq.begin(), pq.end());
          pq.pop_back();
        }
      }
    }
  }
  save_snap(files.size() - 1, hi, lo,
            targetdir + "/snap" + std::to_string(wid));
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

  std::vector<std::string> ranking;
  std::ifstream ranking_file(ranking_path);
  std::string user;
  while (ranking_file >> user) {
    ranking.push_back(user);
  }

  std::cerr << "Checking local snapshots..." << std::endl;
  std::set<info_t, std::greater<info_t>> partial_hi, partial_lo;
  size_t resume_index = std::numeric_limits<size_t>::max();
  auto read_snap = [&](const auto &path, bool partial) {
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

  for (auto &f : std::filesystem::directory_iterator(target_path)) {
    auto path = f.path();
    auto name = path.filename().string();
    if (name.find("_temp", name.size() - 5) != std::string::npos) {
      std::cerr << "Partial snapshot found... ignoring " << path << std::endl;
    } else if (name.rfind("snap", 0) == 0) {
      std::cerr << "Loading snapshot from " << path << std::endl;
      read_snap(path, false);
    }
  }
  if (std::filesystem::exists(target_path + "/partial")) {
    std::cerr << "Loading partial results from " << target_path + "/partial"
              << std::endl;
    read_snap(target_path + "/partial", true);
  }
  if (resume_index == std::numeric_limits<size_t>::max())
    resume_index = 0;
  // prune partial results
  if (partial_hi.size() > MAX_RESULTS)
    partial_hi.erase(std::next(partial_hi.begin(), MAX_RESULTS),
                     partial_hi.end());
  if (partial_lo.size() > MAX_RESULTS)
    partial_lo.erase(std::next(partial_lo.begin(), MAX_RESULTS),
                     partial_lo.end());
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
  mapping.clear();

  std::cerr << "Total files left: " << tot << std::endl;

  int nthreads = std::thread::hardware_concurrency();
  std::cerr << "Using " << nthreads << " threads" << std::endl;
  std::vector<std::pair<queue_t, queue_t>> results(nthreads);
  std::vector<std::thread> threads;
  std::vector<size_t> current_index(nthreads);
  std::atomic<size_t> progress(0), global_pos(resume_index);
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < nthreads; i++) {
    threads.emplace_back(worker, &files, cutoff, &global_pos, i, &results[i],
                         &progress, &current_index[i], target_path);
  }

  size_t num_pairs = 0;
  for (size_t i = resume_index; i < files.size(); i++) {
    num_pairs += files[i].size() * (tot - files[i].size());
  }
  num_pairs /= 2;

  printf(" pairs %8ld / %ld (%6.2f%%) | user %4ld / %4ld\r", progress.load(),
         num_pairs, 0.0, global_pos.load() - nthreads, ranking.size());

  while (progress < num_pairs) {
    if (progress) {
      int delta = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      std::chrono::high_resolution_clock::now() - start)
                      .count() /
                  1.0e9 * (num_pairs - progress) / progress;
      int h = delta / 3600;
      int m = (delta % 3600) / 60;
      int s = delta % 60;
      printf(" pairs %8ld / %ld (%6.2f%%) | user %4ld / %4ld | ETA "
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
  printf(
      " pairs %8ld / %ld (%6.2f%%) | user %4ld / %4ld\033[J\n",
      progress.load(), num_pairs, 0.0, global_pos.load() - nthreads,
      ranking.size());

  for (auto &thread : threads) {
    thread.join();
  }

  for (auto &[hi, lo] : results) {
    partial_hi.insert(hi.begin(), hi.end());
    partial_lo.insert(lo.begin(), lo.end());
  }
  if (partial_hi.size() > MAX_RESULTS)
    partial_hi.erase(std::next(partial_hi.begin(), MAX_RESULTS),
                     partial_hi.end());
  if (partial_lo.size() > MAX_RESULTS)
    partial_lo.erase(std::next(partial_lo.begin(), MAX_RESULTS),
                     partial_lo.end());
  save_snap(global_pos.load(), partial_hi, partial_lo,
            target_path + "/partial");
  save_snap(global_pos.load(), partial_hi, partial_lo, target_path + "/total");
}
