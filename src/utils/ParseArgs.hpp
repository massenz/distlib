// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

#pragma once

#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <glog/logging.h>


namespace utils {

class ParseArgs {

  std::vector<std::string> args_;

  std::map<std::string, std::string> parsed_options_;
  std::vector<std::string> positional_args_;

  std::string progname_;

protected:

  std::map<std::string, std::string> parsed_options() const {
    return parsed_options_;
  }

  std::vector<std::string> positional_args() const {
    return positional_args_;
  };

public:
  ParseArgs() = delete;
  ParseArgs(const ParseArgs& other) = delete;

  explicit ParseArgs(const std::vector<std::string>& args) : args_(args) {}

  ParseArgs(const char* args[], size_t len) : progname_(args[0]) {
    size_t pos = progname_.rfind("/");
    if (pos != std::string::npos) {
      progname_ = progname_.substr(pos + 1);
    }
    for (int i = 1; i < len; ++i) {
      args_.push_back(std::string(args[i]));
    }
  }

  void parse() {
    std::for_each(args_.begin(), args_.end(), [this] (const std::string& s) {

      // TODO: would this be simpler, faster using std::regex?
      VLOG(2) << "Parsing: " << s;
      if (std::strspn(s.c_str(), "-") == 2) {
        size_t pos = s.find('=', 2);

        std::string key, value;
        if (pos != std::string::npos) {
          key = s.substr(2, pos - 2);
          if (key.length() == 0) {
            LOG(ERROR) << "Illegal option value; no name for configuration: " << s;
            return;
          }
          value = s.substr(pos + 1);
        } else {
          // Empty values are allowed for "flag-type" options (such as `--enable-log`).
          // They can also "turn off" the flag, by pre-pending a `no-`: `--no-edit`)
          key = s.substr(2, s.length());
          if (key.find("no-") == 0) {
            key = key.substr(3);
            value = "off";
          } else {
            value = "on";
          }
        }
        parsed_options_[key] = value;
        VLOG(2) << key << " -> " << value;
      } else {
        VLOG(2) << "Positional(" << positional_args_.size() + 1 << "): " << s;
        positional_args_.push_back(s);
      }
    });
  };

  std::string progname() const {
    return progname_;
  }

  std::string get(const std::string& name) const {
    return parsed_options()[name];
  }

  const char* get(const char* name) const {
    return parsed_options()[name].c_str();
  }

  std::string at(int pos) const {
    if (pos < count()) {
      return positional_args().at(pos);
    }
    return "";
  }

  int count() const {
    return positional_args().size();
  }

  std::string operator[](int pos) const {
    return at(pos);
  }

  std::string operator[](const std::string& name) const {
    return get(name);
  }

  std::vector<std::string> getNames() {
    std::vector<std::string> names;
    for(auto elem : parsed_options_) {
      names.push_back(elem.first);
    }
    return names;
  }
};

} // namespace utils
