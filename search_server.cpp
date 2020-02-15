#include "search_server.h"
#include "iterator_range.h"
#include "profile.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <unordered_set>

// #define PERF
#ifndef PERF
#define ADD_DURATION(dummy) \
  {}
#endif

SearchServer::SearchServer(istream &document_input) {
  UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream &document_input) {
  InvertedIndex new_index;

  for (string current_document; getline(document_input, current_document);) {
    new_index.Add(move(current_document));
  }

  index = move(new_index);
}

void SearchServer::AddQueriesStream(istream &query_input,
                                    ostream &search_results_output) {
#ifdef PERF
  TotalDuration split("Splitting words to lines");
  TotalDuration docid_fill("Filling in doc_id structure");
  TotalDuration sorting_1("Sorting 1");
  TotalDuration sorting_2("Sorting 2");
  TotalDuration lookup("Looking up for a document");
#endif

  const auto comparer = [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
    int64_t lhs_docid = lhs.first;
    auto lhs_hit_count = lhs.second;
    int64_t rhs_docid = rhs.first;
    auto rhs_hit_count = rhs.second;
    return make_pair(lhs_hit_count, -lhs_docid) >
           make_pair(rhs_hit_count, -rhs_docid);
  };
  const auto total_doc_count = index.DocumentCount();
  vector<size_t> docid_count(total_doc_count);  // document_id -> hitcount
  vector<pair<size_t, size_t>> search_results(total_doc_count);

  for (string current_query;
       getline(query_input, current_query);) {  // # queries <= 500k

    vector<string> words;
    {
      ADD_DURATION(split);
      words = SplitIntoWords(current_query);
    }

    fill(docid_count.begin(), docid_count.end(), 0);
    for (const auto &word : words) {  // # of words <= 10 in query
      DocIds doc_ids;
      {
        ADD_DURATION(lookup);
        doc_ids = move(index.Lookup(word));
      }
      {
        ADD_DURATION(docid_fill);
        for (const size_t docid : doc_ids) {  // # documents <= 50k
          docid_count[docid]++;
        }
      }
    }

    // # of search_results <= 50k (btw my case as all the query are found in
    // documents)
    size_t actual_count = 0;
    {
      ADD_DURATION(sorting_1);
      fill(search_results.begin(), search_results.end(), make_pair(0, 0));
      for (size_t i = 0; i < total_doc_count; i++) {
        if (docid_count[i] > 0) {
          search_results[actual_count++] = {i, docid_count[i]};
        }
      }
    }
    {
      ADD_DURATION(sorting_2);
      if (actual_count < 5) {
        sort(begin(search_results), next(begin(search_results), actual_count),
             comparer);
      } else {
        partial_sort(begin(search_results), next(begin(search_results), 5),
                     next(begin(search_results), actual_count), comparer);
      }
    }

    search_results_output << current_query << ':';
    for (auto [docid, hitcount] :
         Head(search_results, min(actual_count, (size_t)5))) {
      search_results_output << " {"
                            << "docid: " << docid << ", "
                            << "hitcount: " << hitcount << '}';
    }
    search_results_output << endl;
  }
}

void InvertedIndex::Add(const string &document) {
  const size_t docid = NextDocumentId();
  for (const auto &word : SplitIntoWords(document)) {
    index[word].push_back(docid);
  }
}

const DocIds &InvertedIndex::Lookup(const string &word) const {
  if (auto it = index.find(word); it != index.end()) {
    return it->second;
  } else {
    return none;
  }
}
