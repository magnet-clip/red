#pragma once
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define DEFAULT_PAGES (8)

template <class K, class V> using Map = std::unordered_map<K, V>;

std::vector<std::string> SplitIntoWords(std::string_view line,
                                        std::vector<std::string> &words);

std::vector<std::string> GetLines(std::istream &stream);

bool ComparePairs(std::pair<size_t, size_t> lhs, std::pair<size_t, size_t> rhs);

size_t GetThreadsCount();

template <typename Container>
std::pair<typename Container::const_iterator,
          typename Container::const_iterator>
GetChunkStartStop(const Container &items, size_t chunk_num,
                  size_t chunk_count) {
  const size_t chunk_size = items.size() / chunk_count;
  auto start = items.begin() + chunk_num * chunk_size;
  typename Container::const_iterator end;
  if (chunk_num == chunk_count - 1) {
    end = items.end();
  } else {
    end = start + chunk_size;
  }

  return std::make_pair(start, end);
}
