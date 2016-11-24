// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.



#include <gtest/gtest.h>

#include "utils/ParseArgs.hpp"

class ParseArgsTest : public ::testing::Test {
protected:

  virtual void SetUp() {

  }
};

TEST_F(ParseArgsTest, canParseSimple) {
  const char* mine[] = {"/usr/bin/send", "--port=1023"};
  utils::ParseArgs parser(mine, 2);

  ASSERT_STREQ("send", parser.progname().c_str());
  ASSERT_STREQ("1023", parser.get("port"));
}


TEST_F(ParseArgsTest, canParseMany) {
  const char* mine[] = {
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
  utils::ParseArgs parser(mine, sizeof(mine) / sizeof(const char*));

  EXPECT_STREQ("google.com", parser.get("server"));
  EXPECT_STREQ("", parser.get("bogus"));
  EXPECT_STREQ("off", parser.get("amend"));
  EXPECT_STREQ("on", parser.get("enable-no-value"));

  ASSERT_EQ(2, parser.count());
  EXPECT_STREQ("send", parser.at(0).c_str());
  EXPECT_STREQ("myfile.txt", parser.at(1).c_str());

  ASSERT_STREQ("runthis", parser.progname().c_str());
}

TEST_F(ParseArgsTest, canParseFromVector) {
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

  EXPECT_STREQ("google.com", parser.get("server"));
  EXPECT_STREQ("off", parser.get("amend"));

  ASSERT_EQ(2, parser.count());
  EXPECT_STREQ("send", parser.at(0).c_str());
  EXPECT_STREQ("myfile.txt", parser.at(1).c_str());
}

TEST_F(ParseArgsTest, canUseHelperMethods) {
  std::vector<std::string> args = {
      "--port=1023",
      "--server=google.com",
      "--enable-no-value",
      "--no-amend",
      "--zk=localhost:2181",
      "send",
      "myfile.txt",
      "another.jpeg",
      "fillme.png"
  };

  utils::ParseArgs parser(args);

  ASSERT_EQ(4, parser.count());
  ASSERT_EQ("fillme.png", parser[3]);

  ASSERT_STREQ("localhost:2181", parser["zk"].c_str());
  ASSERT_EQ(5, parser.getNames().size());
  ASSERT_STREQ("amend", parser.getNames()[0].c_str());
  ASSERT_STREQ("zk", parser.getNames()[4].c_str());
}
