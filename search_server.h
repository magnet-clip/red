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
using DocIds = list<size_t>;
class InvertedIndex {
 public:
  void Add(const string &document);
  const DocIds &Lookup(const string &word) const;

  const string &GetDocument(size_t id) const { return docs[id]; }

 private:
  Map<string, list<size_t>> index;  // word -> [doc_id]
  vector<string> docs;              // doc_id -> document

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
