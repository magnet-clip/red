#include "inverted_index.h"
#include "main_tests.h"
#include "parse.h"
#include "profile.h"
#include "search_server.h"
#include "test_runner.h"

#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <thread>
#include <vector>
using namespace std;

#define SEED 34
default_random_engine _rd(SEED);
uniform_int_distribution<int> value(10, 1000);
milliseconds random_time() { return milliseconds(value(_rd)); }

void TestSearchServer(vector<pair<ifstream, ostream *>> &streams) {
  LOG_DURATION("Total");
  SearchServer srv(streams.front().first); // Does not seem to read
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
  int overflow(int c) override { return c; }
};

int main() {
  // TestRunner tr;
  // RUN_TEST(tr, TestBasicSearch);
  // RUN_TEST(tr, TestHitcount);
  // RUN_TEST(tr, TestRanking);
  // RUN_TEST(tr, TestSerpFormat);
  // RUN_TEST(tr, TestTop5);
  // return 0;

  // 1) Create an array of input and output streams
  vector<string> folders = {"2-test",     "3-test",     "4-test", "coursera",
                            "coursera-2", "coursera-3", "mid",    "small"};
  NullBuffer null_buffer;
  ostream null_stream(&null_buffer);
  vector<pair<ifstream, ostream *>> streams;
  for (int i = 0; i < 3; i++) {
    for (const auto &folder : folders) {
      streams.push_back(make_pair<ifstream, ostream *>(
          ifstream("../test/" + folder + "/documents.txt"), nullptr));
      streams.push_back(make_pair<ifstream, ostream *>(
          ifstream("../test/" + folder + "/queries.txt"), &null_stream));
    }
  }

  // 2) Feed those to server
  TestSearchServer(streams);
  return 0;
}
