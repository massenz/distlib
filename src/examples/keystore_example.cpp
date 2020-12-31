// Copyright (c) 2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com)

#include <atomic>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#include "keystore/InMemoryKeyStore.hpp"
#include "utils/ParseArgs.hpp"

using namespace std;
using namespace keystore;

long InsertValues(InMemoryKeyStore<std::string, std::string> &store,
                  long num_threads,
                  long chunk_size) {
  std::vector<std::thread> threads;
  std::atomic_long inserted= 0;
  threads.reserve(num_threads);

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back([&](long from, long to) {
      for (long i = from; i < to; ++i) {
        auto key = to_string(i);
        auto value = "this is a random value for " + key;
        if (store.Put(key, value)) {
          inserted++;
        }
      }
    }, i * chunk_size, (i + 1) * chunk_size);
  }
  std::for_each(threads.begin(), threads.end(),
                [](auto &t) { t.join(); });

  return inserted;
}

long LookupValues(const InMemoryKeyStore<std::string, std::string> &store,
                  long num) {

  // Uniform random number generator, to get random keys.
  // Will be used to obtain a seed for the random number engine
  random_device rd;

  // Standard mersenne_twister_engine seeded with rd()
  mt19937 gen(rd());
  uniform_int_distribution<> distrib(1, 999999);

  long found = 0;
  for (int i = 0; i < num; ++i) {
    auto key = to_string(distrib(gen));
    if (store.Get(key)) {
      found++;
    } else {
      cout << "<<<<< " << key << " not found >>>>>" << endl;
    }
  }
  return found;
}


int main(int argc, const char **argv) {
  google::InitGoogleLogging(argv[0]);
  ::utils::ParseArgs parser(argv, argc);

  FLAGS_v = parser.Enabled("verbose") ? 2 : 0;
  FLAGS_logtostderr = parser.Enabled("verbose");

  int buckets = parser.GetInt("buckets", 5);
  int partitions = parser.GetInt("partitions", 10);
  long inserts = parser.GetInt("values", 1000000);
  long num_threads = parser.GetInt("threads", 5);

  utils::PrintVersion("KeyValue Store -- Performance Evaluation", RELEASE_STR);
  utils::PrintCurrentTime() << "  InMemoryKeyStore: using `optional`, with "
                            << num_threads << " threads" << endl;
  if (parser.Enabled("version")) {
    return EXIT_SUCCESS;
  }

  std::shared_ptr<View> pv = std::move(make_balanced_view(buckets, partitions));
  std::unordered_set<std::string> bucket_names;
  for (int i = 0; i < buckets; ++i) {
    bucket_names.insert("bucket-" + std::to_string(i));
  }
  InMemoryKeyStore<std::string, std::string> store {"KeyStore Demo "s + RELEASE_STR, pv,
                                                    bucket_names};

  long chunk_size = inserts / num_threads;

  auto insert_starts = std::chrono::system_clock::now();
  long inserted = InsertValues(store, num_threads, chunk_size);
  auto insert_ends = std::chrono::system_clock::now();

  PrintStats(store);

  auto lookup_starts = std::chrono::system_clock::now();
  long found = LookupValues(store, inserts / 1000);
  auto lookup_ends = std::chrono::system_clock::now();

  auto miss = inserts - inserted;
  auto ins = std::chrono::duration_cast<std::chrono::milliseconds>(
      insert_ends - insert_starts).count();
  auto gets = std::chrono::duration_cast<std::chrono::milliseconds>(
      lookup_ends - lookup_starts).count();

  cout << "It took " << ins << " msec to insert " << inserts << " values;\n"
       << "  of those " << miss << " were not in scope.\n\n"
       << "It took " << gets << " msec to lookup " << inserts / 1000 << " values;\n"
       << "  of those " << found << " were successfully found" << endl;

  return EXIT_SUCCESS;
}

