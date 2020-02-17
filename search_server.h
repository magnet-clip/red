#pragma once

#include "utils.h"
#include <future>
#include <istream>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

using DocIds = vector<pair<size_t, size_t>>;

class InvertedIndex {
public:
  InvertedIndex() { this->docs.reserve(50'000); }

  explicit InvertedIndex(istream &documents) : InvertedIndex() {
    vector<string> buffer(1000);
    Map<string, Map<size_t, size_t>> temp_index;
    for (string document; getline(documents, document);) {
      this->docs.push_back(document);
      const size_t docid = this->docs.size() - 1;
      for (const auto &word : SplitIntoWords(this->docs.back(), buffer)) {
        temp_index[word][docid]++;
      }
    }
    for (const auto &[word, id_count_pair] : temp_index) {
      this->index[word] = {id_count_pair.begin(), id_count_pair.end()};
    }
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
  explicit SearchServer(istream &document_input) : index(document_input) {}
  void UpdateDocumentBase(istream &document_input);
  void AddQueriesStream(istream &query_input, ostream &search_results_output);

private:
  vector<future<void>> futures;
  InvertedIndex index;
};
