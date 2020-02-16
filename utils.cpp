#include "utils.h"

using namespace std;

vector<string> SplitIntoWords(string_view line, vector<string> &words) {
  // remove leading spaces
  line.remove_prefix(min(line.find_first_not_of(' '), line.size()));
  // peek separate words
  size_t count = 0;
  while (!line.empty()) {
    auto not_space_pos = min(line.find_first_of(' '), line.size());
    auto word = string(line.substr(0, not_space_pos));
    line.remove_prefix(not_space_pos);

    words[count++] = move(word);
    auto space_pos = min(line.find_first_not_of(' '), line.size());
    line.remove_prefix(space_pos);
  }
  return {make_move_iterator(words.begin()), make_move_iterator(next(words.begin(), count))};
}