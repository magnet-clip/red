#include "search_server.h"
#include "inverted_index.h"
#include "iterator_range.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <numeric>
#include <shared_mutex>
#include <sstream>
#include <unordered_set>

#define FIVE ((size_t)5)

shared_mutex index_mutex;

SearchServer::SearchServer(istream &documents_input) {
  index = InvertedIndex(GetLines(documents_input));
}

void SearchServer::UpdateDocumentBase(istream &documents_input) {
  update_futures.push_back(async([&documents_input, this]() {
    auto new_index = InvertedIndex(GetLines(documents_input));
    unique_lock lock(index_mutex);
    swap(index, new_index);
  }));
}

string SearchServer::AddQueriesStreamSync(const vector<string> &queries) {
  ostringstream result;

  vector<string> buffer(10);
  vector<size_t> docid_count; // document_id -> hitcount
  vector<size_t> ids;

  for (auto query : queries) {
    fill(docid_count.begin(), docid_count.end(), 0);

    unique_lock lock(index_mutex);
    auto total_count = index.DocumentCount();
    docid_count.assign(total_count, 0);
    ids.resize(total_count);
    // # of words <= 10 in query
    for (const auto &word : SplitIntoWords(query, buffer)) {
      // # documents <= 50k
      auto counts = index.Lookup(word);
      if (!counts.has_value()) {
        continue;
      }
      for (const auto &[doc_id, doc_count] : counts.value()) {
        docid_count[doc_id] += doc_count;
      }
    }

    iota(ids.begin(), ids.end(), 0);
    partial_sort(ids.begin(), next(ids.begin(), min(FIVE, ids.size())),
                 ids.end(), [&docid_count](int64_t lhs, int64_t rhs) {
                   return pair(docid_count[lhs], -lhs) >
                          pair(docid_count[rhs], -rhs);
                 });

    result << query << ':';
    for (auto docid : Head(ids, min(ids.size(), FIVE))) {
      auto count = docid_count[docid];
      if (count == 0)
        break;
      result << " {"
             << "docid: " << docid << ", "
             << "hitcount: " << count << '}';
    }
    result << endl;
  }

  return result.str();
}

void SearchServer::AddQueriesStream(istream &query_input,
                                    ostream &search_results_output) {

  // # queries <= 500k
  vector<string> queries(500'000);
  size_t count = 0;
  for (string current_query; getline(query_input, current_query);) {
    queries[count++] = current_query;
  }
  queries.resize(count);

  const size_t page_count = GetThreadsCount();

  for (size_t i = 0; i < page_count; i++) {
    const auto &[start, end] = GetChunkStartStop(queries, i, page_count);
    vector<string> sub_queries = {make_move_iterator(start),
                                  make_move_iterator(end)};
    query_futures.push_back(async([this, sub_queries]() {
      return SearchServer::AddQueriesStreamSync(sub_queries);
    }));
  }
  for (auto &query_thread : query_futures) {
    search_results_output << query_thread.get();
  }
  query_futures.clear();
}
