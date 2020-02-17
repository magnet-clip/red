#pragma once

#include <mutex>

template <typename T>
class Synchronized {
 public:
  explicit Synchronized(T initial = T()) : value(std::move(initial)) {}

  struct Access {
    T &ref_to_value;
    std::lock_guard<std::mutex> guard;
  };

  Access GetAccess() { return {value, lock_guard(m)}; }

 private:
  std::mutex m;
  T value;
};