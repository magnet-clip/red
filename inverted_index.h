#pragma once

#include "utils.h"

#include <future>
#include <iostream>
#include <istream>
#include <list>
#include <map>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
struct Entry {
  size_t docid, count;
};
using DocIds = vector<Entry>;

class InvertedIndex {
 public:
  InvertedIndex() = default;
  explicit InvertedIndex(const vector<string> &documents);

  optional<DocIds> Lookup(const string_view &word) const;

  size_t DocumentCount() const { return this->docs.size(); }
  const string &GetDocument(size_t id) const { return this->docs[id]; }

 private:
  Map<string_view, DocIds> index;
  vector<string> docs;
};
