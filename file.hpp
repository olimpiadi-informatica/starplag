#pragma once

#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

enum class diff_t { SAME, ADDED, CHANGED };
using diffs_t = std::vector<diff_t>;
using key_t = int;

std::map<std::string, key_t> mapping = {{"", 0}};
std::map<key_t, std::string> rev_mapping = {{0, ""}};

static const std::string specials = "!\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";

constexpr const char *diff_color(diff_t d) {
  switch (d) {
  case diff_t::SAME:
    return "\033[0m";
  case diff_t::ADDED:
    return "\033[41;37;1m";
  case diff_t::CHANGED:
    return "\033[1;30;43m";
  }
  return "";
}

std::string replace_all(std::string s, const std::string &c,
                        const std::string &repl) {
  for (size_t i = s.find(c); i != std::string::npos;
       i = s.find(c, i + repl.size())) {
    s.replace(i, c.size(), repl);
  }
  return s;
}

struct file_t {
  using content_t = std::vector<key_t>;

  std::string group;
  std::string path;
  content_t content;
  std::vector<std::string> spaces;

  file_t(std::string file) {
    path = file;
    std::ifstream in(file);
    std::string str((std::istreambuf_iterator<char>(in)),
                    std::istreambuf_iterator<char>());
    std::string current;
    bool is_spaces = true;

    auto take = [&]() {
      if (mapping.count(current) == 0) {
        rev_mapping[mapping.size()] = current;
        mapping[current] = mapping.size();
      }
      content.push_back(mapping[current]);
      current = "";
      is_spaces = true;
    };
    for (size_t i = 0; i < str.size();) {
      char c = str[i];
      if (is_spaces) {
        if (isspace(c)) {
          current += c;
          i++;
        } else {
          spaces.push_back(current);
          current = "";
          is_spaces = false;
        }
      } else {
        auto spec_pos = specials.find(c);
        if (isspace(c)) {
          take();
        } else if (spec_pos != std::string::npos) {
          if (!current.empty()) {
            take();
          } else {
            content.push_back(-1 - spec_pos);
            i++;
            is_spaces = true;
          }
        } else {
          current += c;
          i++;
        }
      }
    }
    if (is_spaces) {
      spaces.push_back(current);
    } else {
      take();
      spaces.push_back("");
    }

    assert(spaces.size() == content.size() + 1);
  }

  const key_t &operator[](size_t i) const { return content[i]; }

  void print(std::ofstream &out, const diffs_t &d, const diffs_t &wd) {
    assert(d.size() == content.size());
    out << "\033[1;4m==> " << path << " <==\033[0m" << std::endl << std::endl;

    for (size_t i = 0; i < content.size(); i++) {
      out << diff_color(wd[i]);
      out << ((wd[i] == diff_t::SAME)
                  ? spaces[i]
                  : replace_all(replace_all(spaces[i], "\r\n", "\n"), "\n",
                                std::string("\\n") + diff_color(diff_t::SAME) +
                                    "\n" + diff_color(wd[i])));
      out << diff_color(diff_t::SAME);

      out << diff_color(d[i])
          << (content[i] >= 0 ? rev_mapping[content[i]]
                              : specials.substr(-1 - content[i], 1))
          << diff_color(diff_t::SAME);
    }
    if (!spaces.empty())
      out << spaces.back() << std::endl;
  }
};
