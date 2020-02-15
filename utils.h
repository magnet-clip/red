#pragma once
#include <string>
#include <unordered_map>
#include <vector>

// #define PERF

template <class K, class V>
using Map = std::unordered_map<K, V>;

std::vector<std::string> SplitIntoWords(std::string_view str, std::vector<std::string>& words);