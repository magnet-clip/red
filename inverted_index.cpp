#include "inverted_index.h"
#include "utils.h"

InvertedIndex::InvertedIndex(const vector<string> &documents) {
  for (auto &document : documents) {
    docs.push_back(move(document));
    auto docid = docs.size() - 1;
    for (const auto &word : SplitIntoWords(docs.back())) {
      auto &records = index[word];
      if (!records.empty() && records.back().docid == docid) {
        records.back().count++;
      } else {
        records.push_back({docid, 1});
      }
    }
  }
}

optional<DocIds> InvertedIndex::Lookup(const string_view &word) const {
  if (auto it = this->index.find(word); it != this->index.end()) {
    return it->second;
  } else {
    return {};
  }
}
