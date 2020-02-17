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

using DocIds = vector<pair<size_t, size_t>>;
using TempIndex = Map<string, Map<size_t, size_t>>;

class InvertedIndex {
public:
  InvertedIndex() = default;
  explicit InvertedIndex(const vector<string> &documents);

  optional<DocIds> Lookup(const string &word) const;

  size_t DocumentCount() const { return this->docs.size(); }
  const string &GetDocument(size_t id) const { return this->docs[id]; }

private:
  void JoinTempIndices(TempIndex &main, TempIndex &addition);
  TempIndex FillIndexAsync(const vector<string> &documents);
  TempIndex FillIndexSync(const vector<string> &documents, size_t start_doc_id);
  Map<string, DocIds> index;
  vector<string> docs;
};
