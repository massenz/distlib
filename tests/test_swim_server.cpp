// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/24/16.


#include <memory>
#include <thread>

#include <gtest/gtest.h>

#include "swim/SwimServer.hpp"
#include "swim/SwimClient.hpp"

#include "tests.h"

using namespace swim;


Server parse(void *data, size_t size) {
  Server s2;
  s2.ParseFromArray(data, size);
  return s2;
}


TEST(SwimServerProtoTests, allocations) {
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
  TestServer(unsigned short port) : SwimServer(port, 1),
                                  wasUpdated_(false) {}

  virtual ~TestServer() {}

  virtual void onUpdate(Server *client) {
    if (client) {
      VLOG(2) << "TestServer::onUpdate " << client->hostname() << ":" << client->port();
      wasUpdated_ = true;
      delete (client);
    }
  }

  bool wasUpdated() { return wasUpdated_; }
};


class SwimServerTests : public ::testing::Test {

protected:

  std::shared_ptr<SwimServer> server_;
  std::unique_ptr<std::thread> thread_;

  SwimServerTests() {
    unsigned short port = tests::randomPort();
    VLOG(2) << "TestFixture: creating server on port " << port;
    server_.reset(new SwimServer(port));
  }


  virtual void TearDown() {
    VLOG(2) << "Tearing down...";
    if (server_) {
      VLOG(2) << "TearDown: stopping server...";
      server_->stop();
    }
    if (thread_) {
      VLOG(2) << "TearDown: joining thread";
      if (thread_->joinable()) {
        thread_->join();
        VLOG(2) << "TearDown: server thread terminated";
      }
    }
  }

  virtual void runServer() {
    if (server_) {
      ASSERT_FALSE(server_->isRunning());
      thread_.reset(new std::thread([this] {
        server_->start();
      }));
      // Wait a beat to allow the socket connection to be established.
      int retries = 10;
      while (retries-- > 0 && !server_->isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      ASSERT_TRUE(server_->isRunning());
    } else {
      FAIL() << "server_ has not been allocated";
    }
  }
};


TEST_F(SwimServerTests, canCreate) {
  ASSERT_NE(nullptr, server_);
  ASSERT_FALSE(server_->isRunning());
}


TEST_F(SwimServerTests, canStartAndConnect) {
  unsigned short port = server_->port();
  ASSERT_TRUE(port >= tests::kMinPort && port < tests::kMaxPort);

  std::unique_ptr<Server> localhost = MakeServer("localhost", port);
  SwimClient client(*localhost);

  EXPECT_FALSE(server_->isRunning());
  ASSERT_NO_FATAL_FAILURE(runServer()) << "Could not get the server started";

  ASSERT_TRUE(server_->isRunning());
  EXPECT_TRUE(client.Ping());

  server_->stop();
  EXPECT_FALSE(server_->isRunning());
}


TEST_F(SwimServerTests, canOverrideOnUpdate) {
  unsigned short port = tests::randomPort();
  ASSERT_TRUE(port >= tests::kMinPort && port < tests::kMaxPort);


  TestServer server(port);
  EXPECT_FALSE(server.isRunning());
  std::thread t([&] {
    VLOG(2) << "Test server thread started";
    server.start();
    VLOG(2) << "Test server thread exiting";
  });
  unsigned short count = 50;
  while (count-- > 0 && !server.isRunning()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  ASSERT_TRUE(server.isRunning());

  auto dest = MakeServer("localhost", port);
  SwimClient client(*dest);
  ASSERT_TRUE(client.Ping());
  ASSERT_TRUE(server.wasUpdated());

  server.stop();
  ASSERT_FALSE(server.isRunning());

  if (t.joinable()) {
    VLOG(2) << "Waiting for server to shutdown";
    t.join();
  }
}

TEST_F(SwimServerTests, destructorStopsServer) {
  unsigned short port = 55234;

  auto server = MakeServer("localhost", port);
  std::unique_ptr<SwimClient> client(new SwimClient(*server));
  {
    TestServer server(port);
    std::thread t([&] { server.start(); });
    t.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ASSERT_TRUE(server.isRunning());
    ASSERT_TRUE(client->Ping());
  }
  // It may take a bit for the server to stop, but not that long.
  // NOTE: we must pass the std::unique_ptr by ref or the compiler gets upset.
  ASSERT_TRUE(tests::WaitAtMostFor([&]() -> bool { return !client->Ping(); },
      std::chrono::milliseconds(500)));
}


TEST_F(SwimServerTests, canRestart) {
  ASSERT_NO_FATAL_FAILURE(runServer()) << "Could not get the server started";

  ASSERT_TRUE(server_->isRunning());
  auto svr = MakeServer("localhost", server_->port());
  SwimClient client(*svr);
  ASSERT_TRUE(client.Ping());

  server_->stop();
  // Start a timer that will fail the test if the server hasn't stopped
  // within the timeout (200 msec).
  std::atomic<bool> joined(false);

  std::thread timeout([&joined] {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    ASSERT_TRUE(joined);
  });
  timeout.detach();

  if (thread_->joinable()) {
    thread_->join();
  }
  joined = true;

  ASSERT_FALSE(server_->isRunning());
  ASSERT_FALSE(client.Ping());

  ASSERT_NO_FATAL_FAILURE(runServer()) << "Could not get the server started";
  ASSERT_TRUE(server_->isRunning());
  ASSERT_TRUE(client.Ping());
}
