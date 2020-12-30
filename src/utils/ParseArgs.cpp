// Copyright (c) 2016-2020 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com)

#include <utils/ParseArgs.hpp>

namespace utils {

// RegEx pattern for use when parsing IP/port strings, defined in misc.cpp.
const std::regex kIpPortPattern{R"((\d{1,3}(\.\d{1,3}){3}):(\d+))"};
const std::regex kHostPortPattern{R"((\S+(\.\S+)*):(\d+))"};
const std::regex kIpPattern{R"(\d{1,3}(\.\d{1,3}){3})"};

std::tuple<std::string, unsigned int> ParseIpPort(const std::string &ipPort) {

  std::smatch matches;
  if (std::regex_match(ipPort, matches, kIpPortPattern)) {
    return std::make_tuple(matches[1], atoi(matches[3].str().c_str()));
  }

  throw parse_error("Not a valid IP:port string: " + ipPort);
}

std::tuple<std::string, unsigned int> ParseHostPort(const std::string &hostPort) {

  std::smatch matches;
  if (std::regex_match(hostPort, matches, kHostPortPattern)) {
    return std::make_tuple(matches[1], atoi(matches[3].str().c_str()));
  }
  throw parse_error("Not a valid host:port string: " + hostPort);
}

std::string trim(const std::string &str) {

  const auto strBegin = str.find_first_not_of(' ');

  if (strBegin == std::string::npos)
    return ""; // no content

  const auto strEnd = str.find_last_not_of(' ');
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}

std::vector<std::string> split(
    const std::string &values,
    const std::string &sep,
    bool trim_spaces,
    bool preserve_empty
) {
  auto remains = values;
  std::vector<std::string> results;
  std::string::size_type pos;

  while ((pos = remains.find(sep)) != std::string::npos) {
    std::string value = remains.substr(0, pos);
    if (trim_spaces) {
      value = trim(value);
    }
    if (!value.empty() || preserve_empty) {
      results.push_back(value);
    }
    remains = remains.substr(pos + 1);
  }

  if (!remains.empty()) {
    results.push_back(remains);
  }
  return results;
}

void ParseArgs::parse() {
  if (parsed_)
    return;

  std::for_each(args_.begin(), args_.end(), [this](const std::string &s) {

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
  parsed_ = true;
};

ParseArgs::ParseArgs(const char *args[], size_t len) : parsed_(false) {
  std::regex progname{R"(/?(\w+)$)"};
  std::smatch matches;
  string prog{args[0]};

  if (regex_search(prog, matches, progname)) {
    progname_ = matches[1];
  }

  for (int i = 1; i < len; ++i) {
    args_.emplace_back(args[i]);
  }
  parse();
}

bool ParseArgs::Enabled(const std::string &name, bool ifAbsentValue) const {
  std::string value = Get(name);

  if (value.empty()) return ifAbsentValue;

  if (value == "on") {
    return true;
  } else if (value == "off") {
    return false;
  }
  throw parse_error("Option '" + name + "' does not appear to be a flag (on/off): '" + value + "'");
}
} // namespace utils
