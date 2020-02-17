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

size_t GetPagesCount();