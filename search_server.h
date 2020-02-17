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

class InvertedIndex {
public:
  InvertedIndex() = default;

  explicit InvertedIndex(const vector<string> &documents) : InvertedIndex() {
    Map<string, Map<size_t, size_t>> temp_index;
    vector<string> buffer(1000);

    size_t docid = 0;
    for (string document : documents) {
      for (const auto &word : SplitIntoWords(document, buffer)) {
        temp_index[word][docid]++;
      }
      docid++;
    }
    for (const auto &[word, id_count_pair] : temp_index) {
      this->index[word] = {id_count_pair.begin(), id_count_pair.end()};
    }
    docs = move(documents);
  }

  optional<DocIds> Lookup(const string &word) const {
    if (auto it = this->index.find(word); it != this->index.end()) {
      return it->second;
    } else {
      return {};
    }
  }
  size_t DocumentCount() const { return this->docs.size(); }
  const string &GetDocument(size_t id) const { return this->docs[id]; }

private:
  Map<string, DocIds> index;
  vector<string> docs;
};

class SearchServer {
public:
  SearchServer() = default;
  explicit SearchServer(istream &document_input);
  void UpdateDocumentBase(istream &document_input);
  void AddQueriesStream(istream &query_input, ostream &search_results_output);

private:
  string AddQueriesStreamSync(const vector<string> &queries);
  vector<future<void>> update_futures;
  vector<future<string>> query_futures;
  InvertedIndex index;
};
