#pragma once
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

template <class K, class V> using Map = std::unordered_map<K, V>;

std::vector<std::string> SplitIntoWords(std::string_view line,
                                        std::vector<std::string> &words);

std::vector<std::string> GetLines(std::istream &stream);