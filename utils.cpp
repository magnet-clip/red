#include "utils.h"
#include <iterator>
#include <sstream>
#include <thread>
using namespace std;

void LeftStrip(string_view& sv) {
  while (!sv.empty() && isspace(sv[0])) {
    sv.remove_prefix(1);
  }
}

string_view ReadToken(string_view& sv) {
  LeftStrip(sv);

  auto pos = sv.find(' ');
  auto result = sv.substr(0, pos);
  sv.remove_prefix(pos != sv.npos ? pos : sv.size());
  return result;
}

vector<string_view> SplitIntoWords(string_view str) {
  vector<string_view> result;

  for (string_view word = ReadToken(str); !word.empty();
       word = ReadToken(str)) {
    result.push_back(word);
  }

  return result;
}

vector<string> GetLines(istream& stream) {
  vector<string> lines(50'000);
  size_t count = 0;
  for (string line; getline(stream, line);) {
    lines[count++] = line;
  }
  lines.resize(count);
  return lines;
}

bool ComparePairs(pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
  int64_t lhs_docid = lhs.first;
  auto lhs_hit_count = lhs.second;
  int64_t rhs_docid = rhs.first;
  auto rhs_hit_count = rhs.second;
  return make_pair(lhs_hit_count, -lhs_docid) >
         make_pair(rhs_hit_count, -rhs_docid);
};

size_t GetThreadsCount() {
  return thread::hardware_concurrency() != 0 ? thread::hardware_concurrency()
                                             : DEFAULT_PAGES;
}
