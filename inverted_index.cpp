#include "inverted_index.h"

InvertedIndex::InvertedIndex(const vector<string> &documents) {
  // auto temp_index = FillIndexAsync(documents);
  auto temp_index = FillIndexSync(documents, 0);
  for (const auto &[word, id_count_pair] : temp_index) {
    this->index[move(word)] = {id_count_pair.begin(), id_count_pair.end()};
  }
  docs = move(documents);
}

TempIndex InvertedIndex::FillIndexSync(const vector<string> &documents,
                                       size_t start_doc_id = 0) {
  TempIndex temp_index;
  vector<string> buffer(1000);

  for (auto document : documents) {
    for (const auto &word : SplitIntoWords(document, buffer)) {
      temp_index[word][start_doc_id]++;
    }
    start_doc_id++;
  }
  return temp_index;
}

TempIndex InvertedIndex::FillIndexAsync(const vector<string> &documents) {
  TempIndex temp_index;
  vector<future<TempIndex>> futures;
  const size_t page_count = GetPagesCount();

  for (size_t i = 0; i < page_count; i++) {
    futures.push_back(async([&documents, page_count, i, this]() {
      const size_t page_size = documents.size() / page_count;
      auto start = documents.begin() + i * page_size;
      vector<string>::const_iterator end;
      if (i == page_count - 1) {
        end = documents.end();
      } else {
        end = start + page_size;
      }

      return FillIndexSync({start, end}, i * page_size);
    }));
  }
  for (auto &index_thread : futures) {
    auto thread_temp_index = index_thread.get();
    JoinTempIndices(temp_index, thread_temp_index);
  }
  futures.clear();

  return temp_index;
}

optional<DocIds> InvertedIndex::Lookup(const string &word) const {
  if (auto it = this->index.find(word); it != this->index.end()) {
    return it->second;
  } else {
    return {};
  }
}

void InvertedIndex::JoinTempIndices(TempIndex &main, TempIndex &addition) {
  for (const auto &[added_word, added_counts] : addition) {
    if (auto it = main.find(added_word); it != main.end()) {
      for (const auto &[docid, count] : added_counts) {
        (it->second)[docid] += count;
      }
    } else {
      main.insert({move(added_word),
                   {make_move_iterator(added_counts.begin()),
                    make_move_iterator(added_counts.end())}});
    }
  }
}