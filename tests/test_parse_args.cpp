// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

// Ignore CLion warning caused by GTest TEST() macro.
#pragma ide diagnostic ignored "cert-err58-cpp"

#include <gtest/gtest.h>

#include "../include/utils/ParseArgs.hpp"


TEST(ParseArgsTests, canParseSimple) {
  const char *mine[] = {"/usr/bin/send", "--port=1023"};
  utils::ParseArgs parser(mine, 2);

  ASSERT_STREQ("send", parser.progname().c_str());
  ASSERT_EQ("1023", parser.Get("port"));
}


TEST(ParseArgsTests, canParseMany) {
  const char *mine[] = {
      "/usr/bin/runthis",
      "--port=1023",
      "send",
      "--server=google.com",
      "--enable-no-value",
      "--no-amend",
      "--zk=tcp://localhost:2181,tcp://localhost:2182,tcp://host123.com:9909",
      "--bogus=",
      "--=invalid",
      "myfile.txt"
  };
  utils::ParseArgs parser(mine, sizeof(mine) / sizeof(const char *));

  EXPECT_EQ("google.com", parser.Get("server"));
  EXPECT_EQ("", parser.Get("bogus"));
  EXPECT_EQ("off", parser.Get("amend"));
  EXPECT_EQ("on", parser.Get("enable-no-value"));

  ASSERT_EQ(2, parser.size());
  EXPECT_STREQ("send", parser.at(0).c_str());
  EXPECT_STREQ("myfile.txt", parser.at(1).c_str());

  ASSERT_STREQ("runthis", parser.progname().c_str());
}

TEST(ParseArgsTests, has) {
  utils::ParseArgs parser(
      {
          "--port=1023",
          "--server=google.com",
          "--enable-no-value",
          "--no-amend",
          "--zk=localhost:2181",
          "send",
          "myfile.txt",
          "another.jpeg",
          "fillme.png"
      }
  );

  ASSERT_TRUE(parser.has("amend"));
  ASSERT_TRUE(parser.has("zk"));
  ASSERT_TRUE(parser.has("port"));
  ASSERT_TRUE(parser.has("server"));
  ASSERT_TRUE(parser.has("enable-no-value"));

  ASSERT_FALSE(parser.has("randomstuff"));
  ASSERT_FALSE(parser.has("no-amend"));
  ASSERT_FALSE(parser.has("send"));
}

TEST(ParseArgsTests, canParseFromVector) {
  std::vector<std::string> args = {
      "--port=1023",
      "--server=google.com",
      "--enable-no-value",
      "--no-amend",
      "--zk=tcp://localhost:2181,tcp://localhost:2182,tcp://host123.com:9909",
      "send",
      "myfile.txt"
  };

  utils::ParseArgs parser(args);

  EXPECT_EQ("google.com", parser.Get("server"));
  EXPECT_EQ("off", parser.Get("amend"));

  ASSERT_EQ(2, parser.size());
  EXPECT_STREQ("send", parser.at(0).c_str());
  EXPECT_STREQ("myfile.txt", parser.at(1).c_str());
}

TEST(ParseArgsTests, canUseHelperMethods) {
  utils::ParseArgs parser(
      {
          "--port=1023",
          "--server=google.com",
          "--enable-no-value",
          "--no-amend",
          "--zk=localhost:2181",
          "send",
          "myfile.txt",
          "another.jpeg",
          "fillme.png"
      }
  );

  ASSERT_EQ(4, parser.size());
  ASSERT_EQ("fillme.png", parser[3]);

  ASSERT_STREQ("localhost:2181", parser["zk"].c_str());
  ASSERT_EQ(5, parser.GetNames().size());
  ASSERT_STREQ("amend", parser.GetNames()[0].c_str());
  ASSERT_STREQ("zk", parser.GetNames()[4].c_str());
}

TEST(ParseArgsTests, getsDefaultValueForMissing) {
  utils::ParseArgs parser(
      {
          "--port=8088",
          "--zk=localhost:2181",
          "myfile.txt"
      }
  );

  ASSERT_EQ(8088, parser.GetInt("port", 9099));
  ASSERT_EQ(9099, parser.GetInt("port-no", 9099));
  ASSERT_EQ(0, parser.GetInt("foo"));

  ASSERT_EQ("defaultValue", parser.Get("none", "defaultValue"));
  ASSERT_TRUE(parser.Get("missing").empty());
}


TEST(ParseArgsTests, boolFlags) {
  utils::ParseArgs parser(
      {
          "--debug",
          "--no-edit",
          "--zk=localhost:2181",
          "myfile.txt"
      }
  );
  ASSERT_FALSE(parser.Enabled("edit"));
  ASSERT_TRUE(parser.Enabled("debug"));
}


TEST(ParseArgsTests, throwsIfUnexpected)
{
  utils::ParseArgs parser(
      {
          "--port=8088",
          "--fail",
          "--zk=localhost:2181",
          "myfile.txt"
      }
  );

  // First, the happy path.
  EXPECT_NO_THROW(parser.Enabled("fail"));

  // Unfortunately Google Test gets mightily confused about the type of exception thrown,
  // so we can't be more specific here (even though, debugging through the code, one can
  // verify the correct type of exception is thrown).
  EXPECT_ANY_THROW(parser.Enabled("port"));
  EXPECT_ANY_THROW(parser[2]);

}

TEST(ParseArgsTests, parseIp) {
  // The DIY approach.
  ASSERT_TRUE(std::regex_match("10.0.2.15", ::utils::kIpPattern));

  ASSERT_TRUE(::utils::ParseIp("192.168.1.51"));
  ASSERT_TRUE(::utils::ParseIp("17.0.0.1"));
  ASSERT_TRUE(::utils::ParseIp("255.255.255.254"));
  ASSERT_FALSE(::utils::ParseIp("255,255,255,254"));
  ASSERT_FALSE(::utils::ParseIp("17.0.0.1:8080"));
  ASSERT_FALSE(::utils::ParseIp("a tomato is not an IP"));
}
