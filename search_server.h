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

using DocIds = vector<pair<size_t, size_t>>;
class InvertedIndex1 {
 public:
  InvertedIndex1() { docs.reserve(50'000); }
  void Add(const string &document, vector<string>& buffer);
  const DocIds &Lookup(const string &word) const;
  const string &GetDocument(size_t id) const { return docs[id]; }
  size_t DocumentCount() const { return docs.size(); }

  void FinishUpdate();

 private:
  Map<string, DocIds> index;  // word -> [doc_id]
  vector<string> docs;        // doc_id -> document

  // word -> [doc_id, how many times this word shows up in this document]
  Map<string, Map<size_t, size_t>> temp_index;

  DocIds none;
};

class SearchServer {
 public:
  SearchServer() = default;
  explicit SearchServer(istream &document_input);
  void UpdateDocumentBase(istream &document_input);
  void AddQueriesStream(istream &query_input, ostream &search_results_output);

 private:
  InvertedIndex1 index;
};
