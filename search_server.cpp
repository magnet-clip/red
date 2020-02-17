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

SearchServer::SearchServer(istream &documents_input)
    : index({InvertedIndex(GetLines(documents_input))}) {}

void UpdateIndex(istream &document_input, Synchronized<InvertedIndex> &index) {
  InvertedIndex new_index(GetLines(document_input));
  swap(index.GetAccess().ref_to_value, new_index);
}

void SearchServer::UpdateDocumentBase(istream &documents_input) {
  update_futures.push_back(
      async(UpdateIndex, ref(documents_input), ref(index)));
}

string AddQueriesStreamSync(const vector<string> &queries,
                            Synchronized<InvertedIndex> &index) {
  ostringstream result;

  vector<size_t> docid_count;  // document_id -> hitcount
  vector<size_t> ids;

  for (auto query : queries) {
    const auto words = SplitIntoWords(query);
    {
      auto index_access = index.GetAccess();
      auto &index = index_access.ref_to_value;
      auto total_count = index.DocumentCount();
      docid_count.assign(total_count, 0);
      ids.resize(total_count);
      // # of words <= 10 in query
      for (const auto &word : words) {
        // # documents <= 50k
        auto counts = index.Lookup(word);
        if (!counts.has_value()) {
          continue;
        }
        for (const auto &[doc_id, doc_count] : counts.value()) {
          docid_count[doc_id] += doc_count;
        }
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
      if (count == 0) break;
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
    query_futures.push_back(
        async(AddQueriesStreamSync, sub_queries, ref(index)));
  }
  for (auto &query_thread : query_futures) {
    search_results_output << query_thread.get();
  }
  query_futures.clear();
}
