#pragma once

#include <istream>
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "utils.h"
using namespace std;

using DocIds = vector<size_t>;
class InvertedIndex {
 public:
  void Add(const string &document);
  const DocIds &Lookup(const string &word) const;
  size_t DocumentCount() const { return _last_doc_id; }
  size_t NextDocumentId() { return _last_doc_id++; }

 private:
  size_t _last_doc_id = 0;
  Map<string, DocIds> index;  // word -> [doc_id]
  DocIds none;
};

class SearchServer {
 public:
  SearchServer() = default;
  explicit SearchServer(istream &document_input);
  void UpdateDocumentBase(istream &document_input);
  void AddQueriesStream(istream &query_input, ostream &search_results_output);

 private:
  InvertedIndex index;
};
