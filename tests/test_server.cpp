// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/24/16.


#include <memory>
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


TEST(SwimServerFail, allocations)
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
  const unsigned int PORT = 3000;

  std::shared_ptr<SwimServer> server_;
  std::unique_ptr<std::thread> thread_;

  virtual void SetUp() {
    server_.reset(new SwimServer(PORT));
  }

  virtual void TearDown() {
    LOG(INFO) << "tearing down...";
    if (server_->isRunning()) {
      LOG(INFO) << "stopping server...";
      server_->stop();
    }
    if (thread_) {
      LOG(INFO) << "joining thread";
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
  SwimClient client("localhost", PORT);

  EXPECT_FALSE(server_->isRunning());
  runServer();

  // Just make it so the other thread has a chance to run.
  int retries = 10;
  while (--retries > 0 && !server_->isRunning()) {
    // We really don't want to slow down the tests unnecessarily, keep it really tight.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  ASSERT_TRUE(server_->isRunning());

  EXPECT_TRUE(client.ping());

  server_->stop();

  // TODO: for now we need to get the server unstuck...
  client.ping();
  EXPECT_FALSE(server_->isRunning());
}


TEST_F(SwimServerTest, canOverrideOnUpdate)
{
  std::unique_ptr<SwimClient> client(new SwimClient("localhost", 8888));

  TestServer server(8888);
  EXPECT_FALSE(server.isRunning());
  std::thread t([&] {
    VLOG(2) << "Test server thread started";
    server.start();
    VLOG(2) << "Test server thread exiting";
  });
  t.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  ASSERT_TRUE(server.isRunning());
  LOG(INFO) << "Server is running";

  ASSERT_TRUE(client->ping());
  ASSERT_TRUE(server.wasUpdated());
  server.stop();

  // Unblock the server, or it would wait forever for an incoming connection.
  client->ping();
  ASSERT_FALSE(server.isRunning());
}
