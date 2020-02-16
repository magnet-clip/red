#include "parse.h"
#include "profile.h"
#include "search_server.h"
#include "test_runner.h"
#include "utils.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
using namespace std;

void TestFunctionality(const vector<string> &docs,
                       const vector<string> &queries,
                       const vector<string> &expected) {
  istringstream docs_input(Join('\n', docs));
  istringstream queries_input(Join('\n', queries));

  SearchServer srv;
  srv.UpdateDocumentBase(docs_input);
  ostringstream queries_output;
  srv.AddQueriesStream(queries_input, queries_output);

  const string result = queries_output.str();
  const auto lines = SplitBy(Strip(result), '\n');
  ASSERT_EQUAL(lines.size(), expected.size());
  for (size_t i = 0; i < lines.size(); ++i) {
    ASSERT_EQUAL(lines[i], expected[i]);
  }
}

void TestSerpFormat() {
  const vector<string> docs = {"london is the capital of great britain",
                               "i am travelling down the river"};
  const vector<string> queries = {"london", "the"};
  const vector<string> expected = {
      "london: {docid: 0, hitcount: 1}",
      Join(' ', vector{"the:", "{docid: 0, hitcount: 1}",
                       "{docid: 1, hitcount: 1}"})};

  TestFunctionality(docs, queries, expected);
}

void TestTop5() {
  const vector<string> docs = {"milk a",  "milk b",        "milk c", "milk d",
                               "milk e",  "milk f",        "milk g", "water a",
                               "water b", "fire and earth"};

  const vector<string> queries = {"milk", "water", "rock"};
  const vector<string> expected = {
      Join(' ', vector{"milk:", "{docid: 0, hitcount: 1}",
                       "{docid: 1, hitcount: 1}", "{docid: 2, hitcount: 1}",
                       "{docid: 3, hitcount: 1}", "{docid: 4, hitcount: 1}"}),
      Join(' ',
           vector{
               "water:",
               "{docid: 7, hitcount: 1}",
               "{docid: 8, hitcount: 1}",
           }),
      "rock:",
  };
  TestFunctionality(docs, queries, expected);
}

void TestHitcount() {
  const vector<string> docs = {
      "the river goes through the entire city there is a house near it",
      "the wall",
      "walle",
      "is is is is",
  };
  const vector<string> queries = {"the", "wall", "all", "is", "the is"};
  const vector<string> expected = {
      Join(' ',
           vector{
               "the:",
               "{docid: 0, hitcount: 2}",
               "{docid: 1, hitcount: 1}",
           }),
      "wall: {docid: 1, hitcount: 1}",
      "all:",
      Join(' ',
           vector{
               "is:",
               "{docid: 3, hitcount: 4}",
               "{docid: 0, hitcount: 1}",
           }),
      Join(' ',
           vector{
               "the is:",
               "{docid: 3, hitcount: 4}",
               "{docid: 0, hitcount: 3}",
               "{docid: 1, hitcount: 1}",
           }),
  };
  TestFunctionality(docs, queries, expected);
}

void TestRanking() {
  const vector<string> docs = {
      "london is the capital of great britain",
      "paris is the capital of france",
      "berlin is the capital of germany",
      "rome is the capital of italy",
      "madrid is the capital of spain",
      "lisboa is the capital of portugal",
      "bern is the capital of switzerland",
      "moscow is the capital of russia",
      "kiev is the capital of ukraine",
      "minsk is the capital of belarus",
      "astana is the capital of kazakhstan",
      "beijing is the capital of china",
      "tokyo is the capital of japan",
      "bangkok is the capital of thailand",
      "welcome to moscow the capital of russia the third rome",
      "amsterdam is the capital of netherlands",
      "helsinki is the capital of finland",
      "oslo is the capital of norway",
      "stockgolm is the capital of sweden",
      "riga is the capital of latvia",
      "tallin is the capital of estonia",
      "warsaw is the capital of poland",
  };

  const vector<string> queries = {"moscow is the capital of russia"};
  const vector<string> expected = {
      Join(' ', vector{
                    "moscow is the capital of russia:",
                    "{docid: 7, hitcount: 6}",
                    "{docid: 14, hitcount: 6}",
                    "{docid: 0, hitcount: 4}",
                    "{docid: 1, hitcount: 4}",
                    "{docid: 2, hitcount: 4}",
                })};
  TestFunctionality(docs, queries, expected);
}

void TestBasicSearch() {
  const vector<string> docs = {
      "we are ready to go",
      "come on everybody shake you hands",
      "i love this game",
      "just like exception safety is not about writing try catch everywhere in "
      "your code move semantics are not about typing double ampersand "
      "everywhere in your code",
      "daddy daddy daddy dad dad dad",
      "tell me the meaning of being lonely",
      "just keep track of it",
      "how hard could it be",
      "it is going to be legen wait for it dary legendary",
      "we dont need no education"};

  const vector<string> queries = {"we need some help", "it",
                                  "i love this game",  "tell me why",
                                  "dislike",           "about"};

  const vector<string> expected = {
      Join(' ', vector{"we need some help:", "{docid: 9, hitcount: 2}",
                       "{docid: 0, hitcount: 1}"}),
      Join(' ',
           vector{
               "it:",
               "{docid: 8, hitcount: 2}",
               "{docid: 6, hitcount: 1}",
               "{docid: 7, hitcount: 1}",
           }),
      "i love this game: {docid: 2, hitcount: 4}",
      "tell me why: {docid: 5, hitcount: 2}",
      "dislike:",
      "about: {docid: 3, hitcount: 2}",
  };
  TestFunctionality(docs, queries, expected);
}

#define STR_HELPER(N) #N
#define STR(N) STR_HELPER(N)

#define PERFORM(fun, N)                       \
  do {                                        \
    LOG_DURATION(#fun " x " STR(N) " times"); \
    for (size_t i = 0; i < N; i++) {          \
      fun();                                  \
    }                                         \
  } while (0)

void AllTests() {
  TestSerpFormat();
  TestTop5();
  TestHitcount();
  TestRanking();
  TestBasicSearch();
}

void PerformanceTest(string folder) {
  ifstream docs_input("../test/" + folder + "/documents.txt");
  if (!docs_input) {
    cerr << "Can't read docs file" << endl;
    return;
  };
  ifstream queries_input("../test/" + folder + "/queries.txt");
  if (!queries_input) {
    cerr << "Can't read queries file" << endl;
    return;
  };
  SearchServer srv;
  {
    LOG_DURATION("Update doc base");
    srv.UpdateDocumentBase(docs_input);
  }
  {
    LOG_DURATION("Add queries stream");
    ostringstream queries_output;
    srv.AddQueriesStream(queries_input, queries_output);
    const string result = queries_output.str();
  }
}

function<void()> PerformanceTester(string folder) {
  return [folder]() { PerformanceTest(folder); };
}

#define SEED 34
default_random_engine _rd(SEED);
uniform_int_distribution<int> value(10, 1000);
milliseconds random_time() { return milliseconds(value(_rd)); }

void TestSearchServer(vector<pair<ifstream, ostream *>> &streams) {
  // IteratorRange — шаблон из задачи Paginator
  // random_time() — функция, которая возвращает случайный
  // промежуток времени

  LOG_DURATION("Total");
  SearchServer srv(streams.front().first);
  int num = 0;
  int total = streams.size();
  for (auto &[input, output] :
       IteratorRange(begin(streams) + 1, end(streams))) {
    auto time = random_time();
    cout << "Iteration " << (num++) << "/" << total << endl;
    cout << "Sleeping for " << time.count() << "ms" << endl;
    this_thread::sleep_for(time);

    if (!output) {
      cout << "Updating..." << endl;
      srv.UpdateDocumentBase(input);
    } else {
      cout << "Searching..." << endl;
      srv.AddQueriesStream(input, *output);
    }
  }
}

class NullBuffer : public streambuf {
 public:
  int overflow(int c) { return c; }
};

// template <typename _Container>
// class flattening_iterator
//     : public iterator<output_iterator_tag, void, void, void, void> {
//  protected:
//   _Container *container;

//  public:
//   typedef _Container container_type;

//   explicit flattening_iterator(_Container &__x)
//       : container(std::__addressof(__x)) {}

//   flattening_iterator &operator=(
//       const typename _Container::value_type &__value) {
//     container->push_back(__value);
//     return *this;
//   }

//   flattening_iterator &operator=(typename _Container::value_type &&__value) {
//     container->push_back(std::move(__value));
//     return *this;
//   }

//   flattening_iterator &operator*() { return *this; }
//   flattening_iterator &operator++() { return *this; }
//   flattening_iterator operator++(int) { return *this; }
// };

// template <typename _Container>
// inline flattening_iterator<_Container> flattening_inserter(_Container &__x) {
//   return flattening_iterator<_Container>(__x);
// }

void DoTestSearchServer() {
  LOG_DURATION("Grand Total");
  // 1) Create an array of input and output streams
  vector<string> folders = {"1-test",     "2-test",   "3-test",
                            "4-test",     "coursera", "coursera-2",
                            "coursera-3", "mid",      "small"};
  NullBuffer null_buffer;
  ostream null_stream(&null_buffer);
  null_stream << "hello";
  vector<pair<ifstream, ostream *>> streams;

  for (const auto &folder : folders) {
    streams.push_back(make_pair<ifstream, ostream *>(
        ifstream("../test/" + folder + "/documents.txt"), nullptr));
    streams.push_back(make_pair<ifstream, ostream *>(
        ifstream("../test/" + folder + "/queries.txt"), &null_stream));
  }

  // 2) Feed those to server
  TestSearchServer(streams);
  // 3) ???

  // 4) PROFIT!!1
}

int main() {
  TestRunner tr;
  RUN_TEST(tr, TestSerpFormat);
  RUN_TEST(tr, TestTop5);
  RUN_TEST(tr, TestHitcount);
  RUN_TEST(tr, TestRanking);
  RUN_TEST(tr, TestBasicSearch);

  DoTestSearchServer();

  // PERFORM(PerformanceTester("coursera"), 1);
  // PERFORM(PerformanceTester("coursera-2"), 1);
  // PERFORM(PerformanceTester("coursera-2-1"), 1);
  // PERFORM(PerformanceTester("coursera-3"), 1);
}
