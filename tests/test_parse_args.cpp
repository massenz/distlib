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
  parser.parse();

  ASSERT_STREQ("send", parser.progname().c_str());
  ASSERT_EQ(parser.parsed_options().size(), 1);
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
  parser.parse();

  EXPECT_EQ(6, parser.parsed_options().size());
  EXPECT_STREQ("google.com", parser.parsed_options()["server"].c_str());
  EXPECT_NE(parser.parsed_options().end(), parser.parsed_options().find("bogus"));
  EXPECT_STREQ("off", parser.parsed_options()["amend"].c_str());

  ASSERT_EQ(2, parser.positional_args().size());
  EXPECT_STREQ("send", parser.positional_args()[0].c_str());
  EXPECT_STREQ("myfile.txt", parser.positional_args()[1].c_str());

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
  parser.parse();

  EXPECT_EQ(5, parser.parsed_options().size());
  EXPECT_STREQ("google.com", parser.parsed_options()["server"].c_str());
  EXPECT_STREQ("off", parser.parsed_options()["amend"].c_str());

  ASSERT_EQ(2, parser.positional_args().size());
  EXPECT_STREQ("send", parser.positional_args()[0].c_str());
  EXPECT_STREQ("myfile.txt", parser.positional_args()[1].c_str());
}
