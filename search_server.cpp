#include "search_server.h"
#include "inverted_index.h"
#include "iterator_range.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <shared_mutex>
#include <sstream>
#include <unordered_set>

#define FIVE ((size_t)5)

shared_mutex index_mutex;

SearchServer::SearchServer(istream &documents_input) {
  index = InvertedIndex(GetLines(documents_input));
}

void SearchServer::UpdateDocumentBase(istream &documents_input) {
  // update_futures.push_back(async([&documents_input, this]() {
  auto new_index = InvertedIndex(GetLines(documents_input));
  unique_lock lock(index_mutex);
  // swap(index, new_index);
  index = move(new_index);
  // }));
}

string SearchServer::AddQueriesStreamSync(const vector<string> &queries) {
  size_t total_count;
  {
    shared_lock lock(index_mutex);
    total_count = index.DocumentCount();
  }

  ostringstream result;

  vector<string> buffer(10);
  vector<size_t> docid_count(total_count); // document_id -> hitcount
  vector<pair<size_t, size_t>> search_results(total_count);

  for (auto query : queries) {
    fill(docid_count.begin(), docid_count.end(), 0);

    shared_lock lock(index_mutex);
    // # of words <= 10 in query
    for (const auto &word : SplitIntoWords(query, buffer)) {
      // # documents <= 50k
      auto counts = index.Lookup(word);
      if (!counts.has_value())
        continue;
      for (const auto &[doc_id, doc_count] : counts.value()) {
        docid_count[doc_id] += doc_count;
      }
    }

    // # of search_results <= 50k
    size_t actual_count = 0;
    for (size_t i = 0; i < total_count; i++) {
      if (docid_count[i] > 0) {
        search_results[actual_count++] = {i, docid_count[i]};
      }
    }
    const auto actual_end = next(search_results.begin(), actual_count);
    if (actual_count < FIVE) {
      sort(search_results.begin(), actual_end, ComparePairs);
    } else {
      partial_sort(search_results.begin(), next(search_results.begin(), FIVE),
                   actual_end, ComparePairs);
    }

    result << query << ':';
    for (auto [docid, hitcount] :
         Head(search_results, min(actual_count, FIVE))) {
      result << " {"
             << "docid: " << docid << ", "
             << "hitcount: " << hitcount << '}';
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

  const size_t page_count = GetPagesCount();

  for (size_t i = 0; i < page_count; i++) {
    query_futures.push_back(
        async([this, queries, page_count, i, &search_results_output]() {
          const size_t page_step = queries.size() / page_count;
          auto start = queries.begin() + i * page_step;
          vector<string>::const_iterator end;
          if (i == page_count - 1) {
            end = queries.end();
          } else {
            end = start + page_step;
          }
          return SearchServer::AddQueriesStreamSync({start, end});
        }));
  }
  for (auto &query_thread : query_futures) {
    search_results_output << query_thread.get();
  }
  query_futures.clear();
}
