#include <assert.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>

#define SEED 34

using namespace std;
class CaseGenerator {
 public:
  struct GenParams {
    // Максимальное число слов в документе
    size_t max_words_in_document;

    // Количество документов в базе. Документ = строка.
    size_t documents_count;

    // Число различных слов в документе
    size_t distinct_words_in_documents;

    // Число запросов
    size_t number_of_queries;

    // Максимальное число слов в запросе
    size_t max_words_in_query;

    // Максимальная длина слова - как в запросе, так и в документе
    size_t max_word_length;
  };

  CaseGenerator() = delete;
  explicit CaseGenerator(GenParams params);
  void Generate();

 private:
  void GenerateInfo();

  string GetRandomWord();
  vector<string> GetDistinctWords();
  string GetStringOfWords(const vector<string> &words, size_t count);
  string GetDocument(vector<string> &words);
  string GetQuery(vector<string> &words);

  GenParams _params;
  default_random_engine _rd;
  uniform_int_distribution<char> _char;
  uniform_int_distribution<int> _word_length;
  uniform_int_distribution<int> _words_in_document;
  uniform_int_distribution<int> _words_in_query;
};

CaseGenerator::CaseGenerator(GenParams params)
    : _params(params),
      _rd(SEED),
      _char('a', 'z'),
      _word_length(1, params.max_word_length),
      _words_in_document(1, params.max_words_in_document),
      _words_in_query(1, params.max_words_in_query) {
  assert(_params.max_word_length > 0);
  assert(_params.documents_count > 0);
  assert(_params.max_words_in_document > 0);
  assert(_params.distinct_words_in_documents > 0);
  assert(_params.number_of_queries > 0);
  assert(_params.max_words_in_query > 0);
  assert(_params.max_word_length <= 100);
  assert(_params.documents_count <= 50'000);
  assert(_params.max_words_in_document <= 1'000);
  assert(_params.distinct_words_in_documents <= 10'000);
  assert(_params.number_of_queries <= 500'000);
  assert(_params.max_words_in_query <= 10);
}

void CaseGenerator::GenerateInfo() {
  ofstream out("info.txt");
  out << "Максимальная длина слова: " << _params.max_word_length << endl;
  out << endl;
  out << "Количество документов: " << _params.documents_count << endl;
  out << "Максимальное число слов в документе: "
      << _params.max_words_in_document << endl;
  out << "Число различных слов в документе: "
      << _params.distinct_words_in_documents << endl;
  out << endl;
  out << "Число запросов: " << _params.number_of_queries << endl;
  out << "Максимальное число слов в запросе: " << _params.max_words_in_query
      << endl;
}

string CaseGenerator::GetRandomWord() {
  string result(_word_length(_rd), ' ');
  generate(begin(result), end(result), [&] { return _char(_rd); });
  return result;
}

vector<string> CaseGenerator::GetDistinctWords() {
  unordered_set<string> words;
  string word;

  while (words.size() < _params.distinct_words_in_documents) {
    do {
      word = GetRandomWord();
    } while (words.count(word) > 0);
    words.emplace(move(word));
  }

  return {make_move_iterator(words.begin()), make_move_iterator(words.end())};
}

string CaseGenerator::GetStringOfWords(const vector<string> &words,
                                       size_t count) {
  vector<int> indices(words.size());
  iota(indices.begin(), indices.end(), 0);
  shuffle(indices.begin(), indices.end(), _rd);

  ostringstream res;
  for (size_t i = 0; i < count; i++) {
    res << words[indices[i]] << " ";
  }
  return res.str();
}

string CaseGenerator::GetDocument(vector<string> &words) {
  int words_count = _words_in_document(_rd);
  return GetStringOfWords(words, words_count);
}

string CaseGenerator::GetQuery(vector<string> &words) {
  int words_count = _words_in_query(_rd);
  return GetStringOfWords(words, words_count);
}

// Вывод:
// * info.txt - файл с параметрами (documents_count,
// max_words_in_document, distinct_words_in_documents, number_of_queries,
// max_words_in_query, max_word_length)
//
// * document_base.txt - файл базой документов.
// В нем documents_count строк.
// В каждой строке не более max_words_in_document слов.
// Во всем документе не более distinct_words_in_documents различных слов
// Наибольшая длина слова max_word_length
//
// * queries.txt - файл с запросами
// В нем number_of_queries строк
// В каждой строке не более max_words_in_query слов
// Наибольшая длина слова max_word_length
// !! В query могут быть слова, которых нет в базе документов
// !! Поэтому можно завести параметр - доля незнакомых слов в запросах
void CaseGenerator::Generate() {
  cout << "Generating report file..." << endl;
  GenerateInfo();

  //------------------- Пул слов -----------------------
  cout << "Generating word pool..." << endl;
  vector<string> words = GetDistinctWords();

  //---------------- documents.txt ---------------------
  {
    cout << "Generating documents..." << endl;
    ofstream out("documents.txt");
    for (size_t i = 0; i < _params.documents_count; i++) {
      out << GetDocument(words) << endl;
    }
  }

  //----------------- queries.txt ----------------------
  {
    cout << "Generating queries..." << endl;
    ofstream out("queries.txt");
    for (size_t i = 0; i < _params.number_of_queries; i++) {
      out << GetQuery(words) << endl;
    }
  }
  cout << "Done." << endl;
}

int main() {
  // Максимальное число слов в документе
  size_t max_words_in_document = 50;  // <= 1'000

  // Количество документов в базе. Документ = строка.
  size_t documents_count = 50'000;  // <= 50'000

  // Число различных слов в документах
  size_t distinct_words_in_documents = 1000;  // <= 10'000

  // Число запросов
  size_t number_of_queries = 50'000;  // <= 500'000

  // Максимальное число слов в запросе
  size_t max_words_in_query = 10;  // <= 10

  // Максимальная длина слова - как в запросе, так и в документе
  size_t max_word_length = 50;  // <= 100
  CaseGenerator::GenParams params{
      max_words_in_document, documents_count,    distinct_words_in_documents,
      number_of_queries,     max_words_in_query, max_word_length};
  CaseGenerator gen = CaseGenerator(params);
  gen.Generate();
}