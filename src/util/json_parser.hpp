//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <nlohmann/json.hpp>
#include <wx/stream.h>

// ----------------------------------------------------------------------------- : JSON Parser

using json = nlohmann::json;

/// Parse JSON from a wxInputStream
/// Returns an empty object on failure
json parse_json_stream(wxInputStream& stream);

/// Parse JSON from a string
/// Returns an empty object on failure
json parse_json_string(const String& str);
