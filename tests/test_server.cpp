// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/24/16.


#include <memory>
#include <random>
#include <thread>

#include <gtest/gtest.h>

#include "swim/SwimServer.hpp"
#include "swim/SwimClient.hpp"


using namespace swim;


Server parse(void *data, size_t size)
{
  Server s2;
  s2.ParseFromArray(data, size);
  return s2;
}


TEST(SwimServerProto, allocations)
{
  Server server;
  server.set_hostname("fakehost");
  server.set_port(9999);
  size_t bufSize = server.ByteSize();

  char *data = new char[bufSize];
  ASSERT_NE(nullptr, data);
  memset(data, 0, bufSize);

  server.SerializeToArray(data, bufSize);
  Server svr2 = parse(data, bufSize);

  // This shouldn't matter, as the data has now been squirreled away in the PB.
  memset(data, 0, reinterpret_cast<size_t>(bufSize));
  delete (data);

  ASSERT_STREQ("fakehost", svr2.hostname().c_str());
  ASSERT_EQ(9999, svr2.port());
}


class TestServer : public SwimServer {

  bool wasUpdated_;

public:
  TestServer(unsigned int port) : SwimServer(port, 1),
      wasUpdated_(false) {}

  virtual ~TestServer() {
  }

  virtual void onUpdate(Server* client, long timestamp) {
    if (client) {
      VLOG(2) << "TestServer::onUpdate " << client->hostname() << ":" << client->port();
      wasUpdated_ = true;
      delete (client);
    }
  }

  bool wasUpdated() { return wasUpdated_; }
};

class SwimServerTest : public ::testing::Test {
protected:
  std::default_random_engine dre;

  std::shared_ptr<SwimServer> server_;
  std::unique_ptr<std::thread> thread_;

  unsigned short randomPort() {
    std::uniform_int_distribution<unsigned short> di(10000, 20000);
    return di(dre);
  }

  virtual void SetUp() {
    server_.reset(new SwimServer(randomPort()));
  }

  virtual void TearDown() {
    VLOG(2) << "Tearing down...";
    if (server_->isRunning()) {
      VLOG(2) << "TearDown: stopping server...";
      server_->stop();
    }
    if (thread_) {
      VLOG(2) << "TearDown: joining thread";
      if (thread_->joinable()) {
        thread_->join();
      }
    }
  }

  virtual void runServer() {
    if (server_) {
      thread_.reset(new std::thread([this] {
        ASSERT_FALSE(server_->isRunning());
        server_->start();
      }));
      // Wait a beat to allow the socket connection to be established.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
};


TEST_F(SwimServerTest, canCreate)
{
  ASSERT_NE(nullptr, server_);
  ASSERT_FALSE(server_->isRunning());
}


TEST_F(SwimServerTest, canStartAndConnect)
{
  unsigned short port = server_->port();
  SwimClient client("localhost", port);

  EXPECT_FALSE(server_->isRunning());
  runServer();

  ASSERT_TRUE(server_->isRunning());
  EXPECT_TRUE(client.Ping());

  server_->stop();
  EXPECT_FALSE(server_->isRunning());
}


TEST_F(SwimServerTest, canOverrideOnUpdate)
{
  unsigned short port = randomPort();
  std::unique_ptr<SwimClient> client(new SwimClient("localhost", port, 20));

  TestServer server(port);
  EXPECT_FALSE(server.isRunning());
  std::thread t([&] {
    VLOG(2) << "Test server thread started";
    server.start();
    VLOG(2) << "Test server thread exiting";
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  ASSERT_TRUE(server.isRunning());
  ASSERT_TRUE(client->Ping());
  ASSERT_TRUE(server.wasUpdated());

  server.stop();
  ASSERT_FALSE(server.isRunning());

  if (t.joinable()) {
    VLOG(2) << "Waiting for server to shutdown";
    t.join();
  }
}

TEST_F(SwimServerTest, destructorStopsServer)
{
  unsigned short port = randomPort();
  std::unique_ptr<SwimClient> client(new SwimClient("localhost", port));
  {
    TestServer server(port);
    std::thread t([&] { server.start(); });
    t.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_TRUE(server.isRunning());
    ASSERT_TRUE(client->Ping());
  }
    std::this_thread::sleep_for(std::chrono::milliseconds());

  ASSERT_FALSE(client->Ping());
}


TEST_F(SwimServerTest, canRestart)
{
  runServer();

  ASSERT_TRUE(server_->isRunning());
  SwimClient client("localhost", server_->port());
  ASSERT_TRUE(client.Ping());

  server_->stop();
  // Start a timer that will fail the test if the server hasn't stopped
  // within the timeout (200 msec).
  std::atomic<bool> joined(false);

  std::thread timeout([&joined] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ASSERT_TRUE(joined);
  });
  timeout.detach();

  if (thread_->joinable()) {
    thread_->join();
  }
  joined = true;

  ASSERT_FALSE(server_->isRunning());
  ASSERT_FALSE(client.Ping());

  runServer();
  ASSERT_TRUE(server_->isRunning());
  ASSERT_TRUE(client.Ping());
}
