#include "search_server.h"
#include "iterator_range.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

#define FIVE ((size_t)5)

SearchServer::SearchServer(istream &document_input) : index(document_input) {
}

void SearchServer::UpdateDocumentBase(istream &document_input) {
  futures.push_back(async(
      [&document_input, this]() {
        auto new_index = InvertedIndex(document_input);
        unique_lock lock(index_mutex);
        index = move(new_index);
      }));
}

void SearchServer::AddQueriesStream(istream &query_input,
                                    ostream &search_results_output) {
  size_t total_count;
  {
    shared_lock lock(index_mutex);
    total_count = index.DocumentCount();
  }

  const auto comparer = [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
    int64_t lhs_docid = lhs.first;
    auto lhs_hit_count = lhs.second;
    int64_t rhs_docid = rhs.first;
    auto rhs_hit_count = rhs.second;
    return make_pair(lhs_hit_count, -lhs_docid) >
        make_pair(rhs_hit_count, -rhs_docid);
  };

  vector<size_t> docid_count(total_count);  // document_id -> hitcount
  vector<pair<size_t, size_t>> search_results(total_count);
  vector<string> buffer(10);

  // # queries <= 500k
  for (string current_query; getline(query_input, current_query);) {
    fill(docid_count.begin(), docid_count.end(), 0);
    {
      shared_lock lock(index_mutex);
      // # of words <= 10 in query
      for (const auto &word : SplitIntoWords(current_query, buffer)) {
        // # documents <= 50k
        for (const auto &[doc_id, doc_count] : index.Lookup(word)) {
          docid_count[doc_id] += doc_count;
        }
      }
    }

    // # of search_results <= 50k
    size_t actual_count = 0;
    for (size_t i = 0; i < total_count; i++) {
      if (docid_count[i] > 0) {
        search_results[actual_count++] = {i, docid_count[i]};
      }
    }
    const auto actual_end = next(begin(search_results), actual_count);
    if (actual_count < FIVE) {
      sort(begin(search_results), actual_end, comparer);
    } else {
      partial_sort(begin(search_results), next(begin(search_results), FIVE),
                   actual_end, comparer);
    }

    search_results_output << current_query << ':';
    for (auto[docid, hitcount] :
        Head(search_results, min(actual_count, FIVE))) {
      search_results_output << " {"
                            << "docid: " << docid << ", "
                            << "hitcount: " << hitcount << '}';
    }
    search_results_output << endl;
  }
}
