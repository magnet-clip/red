#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>

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
  vector<size_t> docid_count(50'000);  // document_id -> hitcount

  for (string current_query;
       getline(query_input, current_query);) {  // # queries <= 500k

    vector<string> words = SplitIntoWords(current_query);

    fill(docid_count.begin(), docid_count.end(), 0);
    for (const auto &word : words) {  // # of words <= 10 in query
      DocIds doc_ids = move(index.Lookup(word));
      for (const size_t docid : doc_ids) {  // # documents <= 50k
        docid_count[docid]++;
      }
    }

    // # of search_results <= 50k (btw my case as all the query are found in
    // documents)
    vector<pair<size_t, size_t>> search_results;
    for (size_t i = 0; i < 50'000; i++) {
      if (docid_count[i] > 0) {
        search_results.push_back({i, docid_count[i]});
      }
    }
    partial_sort(
        begin(search_results),
        next(begin(search_results), min(search_results.size(), (size_t)5)),
        end(search_results),
        [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
          int64_t lhs_docid = lhs.first;
          auto lhs_hit_count = lhs.second;
          int64_t rhs_docid = rhs.first;
          auto rhs_hit_count = rhs.second;
          return make_pair(lhs_hit_count, -lhs_docid) >
                 make_pair(rhs_hit_count, -rhs_docid);
        });

    search_results_output << current_query << ':';
    for (auto [docid, hitcount] : Head(search_results, 5)) {
      search_results_output << " {"
                            << "docid: " << docid << ", "
                            << "hitcount: " << hitcount << '}';
    }
    search_results_output << endl;
  }
}

void InvertedIndex::Add(const string &document) {
  docs.push_back(document);

  const size_t docid = docs.size() - 1;
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
