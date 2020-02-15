#include "search_server.h"
#include "iterator_range.h"
#include "profile.h"
#include "utils.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <unordered_set>

#ifndef PERF
#define ADD_DURATION(dummy) \
  {}
#endif
#define FIVE ((size_t)5)

SearchServer::SearchServer(istream &document_input) {
  UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream &document_input) {
  InvertedIndex1 new_index;
  vector<string> buffer(1000);

  for (string current_document; getline(document_input, current_document);) {
    new_index.Add(move(current_document), buffer);
  }

  new_index.FinishUpdate();

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

  size_t total_count = index.DocumentCount();
  vector<size_t> docid_count(total_count);  // document_id -> hitcount
  vector<pair<size_t, size_t>> search_results(total_count);

  for (string current_query;
       getline(query_input, current_query);) {  // # queries <= 500k

    vector<string> words;
    {
      ADD_DURATION(split);
      vector<string> buffer(10);
      words = SplitIntoWords(current_query, buffer);
    }

    fill(docid_count.begin(), docid_count.end(), 0);
    for (const auto &word : words) {  // # of words <= 10 in query
      DocIds doc_ids;
      {
        ADD_DURATION(lookup);
        doc_ids = index.Lookup(word);
      }
      {
        ADD_DURATION(docid_fill);
        for (const auto &[doc_id, doc_count] : doc_ids) {  // # documents <= 50k
          docid_count[doc_id] += doc_count;
        }
      }
    }

    // # of search_results <= 50k (btw my case as all the query are found in
    // documents)
    size_t actual_count = 0;
    {
      ADD_DURATION(sorting_1);
      for (size_t i = 0; i < total_count; i++) {
        if (docid_count[i] > 0) {
          search_results[actual_count++] = {i, docid_count[i]};
        }
      }
    }
    {
      ADD_DURATION(sorting_2);
      const auto actual_end = next(begin(search_results), actual_count);
      if (actual_count < FIVE) {
        sort(begin(search_results), actual_end, comparer);
      } else {
        partial_sort(begin(search_results), next(begin(search_results), FIVE),
                     actual_end, comparer);
      }
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

void InvertedIndex1::Add(const string &document, vector<string> &buffer) {
  docs.push_back(document);
  const size_t docid = docs.size() - 1;
  for (const auto &word : SplitIntoWords(docs.back(), buffer)) {
    temp_index[word][docid]++;
  }
}

void InvertedIndex1::FinishUpdate() {
  for (const auto &[word, id_count_pair] : temp_index) {
    index[word] = {make_move_iterator(id_count_pair.begin()),
                   make_move_iterator(id_count_pair.end())};
  }
}

const DocIds &InvertedIndex1::Lookup(const string &word) const {
  if (auto it = index.find(word); it != index.end()) {
    return it->second;
  } else {
    return none;
  }
}
