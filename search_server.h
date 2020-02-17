#pragma once

#include "inverted_index.h"
#include "synchronized.h"
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

class SearchServer {
 public:
  SearchServer() = default;
  explicit SearchServer(istream &document_input);
  void UpdateDocumentBase(istream &document_input);
  void AddQueriesStream(istream &query_input, ostream &search_results_output);

 private:
  vector<future<void>> update_futures;
  vector<future<string>> query_futures;
  Synchronized<InvertedIndex> index;
};
