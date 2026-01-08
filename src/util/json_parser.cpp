//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/json_parser.hpp>

// ----------------------------------------------------------------------------- : JSON Parser Implementation

json parse_json_stream(wxInputStream& stream) {
  try {
    // Read all data from stream into a string
    std::string data;
    char buffer[4096];
    while (!stream.Eof()) {
      stream.Read(buffer, sizeof(buffer));
      size_t bytesRead = stream.LastRead();
      if (bytesRead > 0) {
        data.append(buffer, bytesRead);
      }
    }

    // Parse JSON
    return json::parse(data);
  } catch (const json::parse_error& e) {
    // Return empty object on parse failure
    return json::object();
  } catch (...) {
    return json::object();
  }
}

json parse_json_string(const String& str) {
  try {
    std::string utf8_str = str.ToStdString();
    return json::parse(utf8_str);
  } catch (const json::parse_error& e) {
    return json::object();
  } catch (...) {
    return json::object();
  }
}
