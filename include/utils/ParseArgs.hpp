// Copyright (c) 2016 AlertAvert.com. All rights reserved.
// Created by M. Massenzio (marco@alertavert.com) on 11/22/16.

#pragma once

#include <algorithm>
#include <cstring>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include <glog/logging.h>


namespace utils {

// RegEx pattern for use when parsing IP/port strings, defined in misc.cpp.
const std::regex kIpPortPattern{R"((\d{1,3}(\.\d{1,3}){3}):(\d+))"};
const std::regex kHostPortPattern{R"((\S+(\.\S+)*):(\d+))"};
const std::regex kIpPattern{R"(\d{1,3}(\.\d{1,3}){3})"};


class parse_error : public std::exception {
  std::string what_;

public:
  explicit parse_error(const std::string& error) : what_(error) {}

#if defined( __linux__ )
  virtual const char* what() const _GLIBCXX_NOTHROW {
#elif defined( __APPLE__ )
  virtual const char* what() const _NOEXCEPT {
#endif
    return what_.c_str();
  }
};


/**
 * Splits an ip:port string into its component parts, also ensuring that they are correctly formed.
 *
 * @param ipPort a string of the form `IP:port`, as in " "192.168.51.123:8084".
 * @return the parsed `{ip, port}` tuple.
 * @throws parse_error if the string is not correctly formatted.
 */
std::tuple<std::string, unsigned int> ParseIpPort(const std::string& ipPort) {

  std::smatch matches;

  if (std::regex_match(ipPort, matches, kIpPortPattern)) {
    return std::make_tuple(matches[1], atoi(matches[3].str().c_str()));
  }

  throw new parse_error("Not a valid IP:port string: " + ipPort);
}

/**
 * Splits a `host:port` string into its component parts.
 *
 * @param hostPort a string of the form `host:port`, as in " "host.example.com:8084".
 * @return the parsed `{host, port}` tuple.
 * @throws parse_error if the string is not correctly formatted.
 */
std::tuple<std::string, unsigned int> ParseHostPort(const std::string& hostPort) {

  std::smatch matches;

  if (std::regex_match(hostPort, matches, kHostPortPattern)) {
    return std::make_tuple(matches[1], atoi(matches[3].str().c_str()));
  }

  throw new parse_error("Not a valid host:port string: " + hostPort);
}


/**
 * Parses a string and checks whether it is a valid IP address.
 *
 * param ip
 * @return `true` if `ip` is an IP address.
 */
inline bool ParseIp(const std::string &ip) {
  return std::regex_match(ip, kIpPattern);
}

/**
 * Removes leading and trailing spaces from the string.
 *
 * @param str
 * @return the `str` without leading and trailing spaces, if any
 */
std::string trim(const std::string &str) {
  const auto strBegin = str.find_first_not_of(' ');

  if (strBegin == std::string::npos)
    return ""; // no content

  const auto strEnd = str.find_last_not_of(' ');
  const auto strRange = strEnd - strBegin + 1;

  return str.substr(strBegin, strRange);
}


/**
 * Splits a `sep`-separated string into its components part.
 *
 * <p>For example using the default ',' comma separator, it would return `"one,two,three"` as a
 * three-element vector `{"one", "two", "three"}`.
 *
 * It can optionaly skip empty values (by default, they are returned as empty strings) and remove
 * leading and trailing spaces around the values (the default behavior).
 *
 * Spaces inside the values (unless space is used as `sep`) are preserved.
 *
 * @param values the string containing 0 or more string separated by `sep`
 * @param sep the separator; by defaul the comma (`,`) but can be any string
 * @param trim_spaces if `true` (the default) spaces around the values are trimmed
 * @param preserve_empty if `true` (the default) consecutive `sep` strings will be considered as
 *      a single separator (in other words, `"one,two,,,,three"` will return a three-elements vector
 *
 * @return a vector of string containing the values, possibly empty.
 */
std::vector<std::string> split(const std::string &values, const std::string &sep = ",",
                          bool trim_spaces = true, bool preserve_empty = true) {
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


/**
 * Command-line options parser.
 *
 * @author M. Massenzio (marco@alertavert.com) 2016-11-24
 */
class ParseArgs {

  std::vector<std::string> args_;

  std::map<std::string, std::string> parsed_options_;
  std::vector<std::string> positional_args_;

  std::string progname_;

  bool parsed_;

protected:

  std::map<std::string, std::string> parsed_options() const {
    return parsed_options_;
  }

  std::vector<std::string> positional_args() const {
    return positional_args_;
  };


  void parse() {
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

public:
  ParseArgs() = delete;

  ParseArgs(const ParseArgs &other) = delete;

  /**
   * Used to build a parser from a list of strings.
   *
   * <p>Identical in behavior to the other C-style constructor, but will <strong>NOT</strong> assume
   * that the first element of the vector is the program name.
   *
   * @param args a list of both positional and named arguments.
   */
  explicit ParseArgs(const std::vector<std::string> &args) : args_(args), parsed_(false) {
    parse();
  }

  /**
   * Builds a command-line arguments parser.
   *
   * Use directly with your `main()` arguments, to parse the `--option=value` named arguments and
   * all other "positional" arguments.
   *
   * <p>Example:
   * <pre>
   *   int main(int argc, const char* argv[]) {
   *     utils::ParseArgs parser(argv, argc);
   *     parser.parse();
   *
   *     if (parser.has("help")) {
   *       usage();
   *       return EXIT_SUCCESS;
   *     }
   *     // ...
   *   }
   * </pre>
   *
   * <p>`args[0]` is assumed to be the name of the program (see `progname()`).
   *
   * @param args an array of C-string values.
   * @param len the number of elements in `args`.
   */
  ParseArgs(const char *args[], size_t len) : progname_(args[0]), parsed_(false) {
    size_t pos = progname_.rfind("/");
    if (pos != std::string::npos) {
      progname_ = progname_.substr(pos + 1);
    }
    for (int i = 1; i < len; ++i) {
      args_.push_back(std::string(args[i]));
    }
    parse();
  }


  /**
   * This is the value of `args[0]` when the parser is constructed with a list of C-strings.
   * Otherwise it will be the emtpy string.
   *
   * @return `args[0]` if the parser is constructed via `ParseArgs::ParseArgs(const char* , size_t len)`.
   */
  std::string progname() const {
    return progname_;
  }

  /**
   * @param name the name of the option to look for
   * @return `true` if the option was passed on the command line; `false` otherwise
   */
  bool has(const std::string& name) {
    return parsed_options_.find(name) != parsed_options_.end();
  }

  /**
   * Gets the value of the `--name=value` option.
   *
   * @param name the name of the option to lookup
   * @param defaultValue the default value to return if no flag was passed in
   * @return the value of the named option if specified, or the empty string instead.
   */
  std::string get(const std::string &name, std::string defaultValue = "") const {
    if (parsed_options_.find(name) == parsed_options_.end()) {
      return defaultValue;
    }
    return parsed_options()[name];
  }


  /**
   * If present, converts the value of the option into an integer value.
   *
   * <p>Optionally, a default value can be provided, which will be returned if the option is
   * missing: <strong>be careful with this API</strong> because it is impossible to distinguish
   * the case of when the option is missing from when the default value was passed in.
   *
   * <p>If it is necessary to take a different action if the flag is missing, then use the `has()`
   * method first.
   *
   * @param name the name of the option, whose value MUST be convertible into a valid integer
   * @param defaultInt an optional value to be returned if the flag is missing (0 will be
   *        returned if this is not provided)
   * @return the value of the option, if present, the `defaultInt` if not.
   */
  int getInt(const std::string &name, int defaultInt = 0) {
    std::string value = get(name);
    if (value.empty()) {
      return defaultInt;
    }
    return atoi(value.c_str());
  }

  inline unsigned int getUInt(const std::string& name, int defaultInt = 0) {
    return static_cast<unsigned int>(getInt(name, defaultInt));
  }

  /**
   * @param pos the index of the positional argument
   * @return the value of the positional argument at position `pos`, if valid.
   * @throws std::out_of_range if `pos` > `size()`
   */
  std::string at(int pos) const {
    if (pos < size()) {
      return positional_args().at(pos);
    }
    throw new std::out_of_range("Not enough positional arguments");
  }

  /**
   * @return the number of positional arguments found.
   */
  int size() const {
    return positional_args().size();
  }

  /**
   * Alias for `get(pos)`.
   *
   * @param pos
   * @return the value of the positional argument at position `pos`.
   */
  std::string operator[](int pos) const {
    return at(pos);
  }

  /**
   * Alias for `get(name)`.
   *
   * @param name
   * @return the value of the named option
   */
  std::string operator[](const std::string &name) const {
    return get(name);
  }

  /**
   * @return all the names of the options being parsed (those of the `--name=value` form).
   */
  std::vector<std::string> getNames() {
    std::vector<std::string> names;
    for (auto elem : parsed_options_) {
      names.push_back(elem.first);
    }
    return names;
  }

  /**
   * Converts an on/off flag into its boolean equivalent.
   *
   * @param name the name of the option being passed on the command line.
   * @param ifAbsentValue if the flag has not been specified, the default value returned.
   * @return `false` if the option is "off"; `true` otherwise.
   */
  bool enabled(const std::string &name, bool ifAbsentValue = false) {
    std::string value = get(name);

    if (value.empty()) return ifAbsentValue;

    if (value == "on") {
      return true;
    } else if (value == "off") {
      return false;
    }
    throw new parse_error("Option '" + name + "' does not appear to be a flag (on/off): "
        "'" + value + "'");
  }
};

} // namespace utils
