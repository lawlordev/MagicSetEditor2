//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <vector>

// ----------------------------------------------------------------------------- : Font Manager

/// Manages custom font installation and loading
class FontManager {
public:
  /// Get the singleton instance
  static FontManager& getInstance();

  /// Get the user's font directory based on OS
  String getUserFontDirectory();

  /// Get the path to the bundled fonts directory (Magic - Fonts)
  String getBundledFontsDirectory();

  /// Find all font files (.ttf, .otf) in a directory recursively
  std::vector<String> findAllFonts(const String& directory);

  /// Check how many bundled fonts are not yet installed
  int countMissingFonts();

  /// Install all bundled fonts to user's font directory
  /// Returns number of fonts successfully installed
  int installAllFonts();

  /// Check if a specific font file is already installed
  bool isFontFileInstalled(const String& fontFileName);

  /// Copy a single font file to user's font directory
  bool installFontFile(const String& sourcePath);

  /// Refresh system font cache (platform-specific)
  void refreshFontCache();

  /// Check if Beleren Bold font is available
  bool isBelerenAvailable();

private:
  FontManager() = default;
  FontManager(const FontManager&) = delete;
  FontManager& operator=(const FontManager&) = delete;
};

/// Global font manager instance
inline FontManager& fontManager() { return FontManager::getInstance(); }

// ----------------------------------------------------------------------------- : Constants

#define FONT_BELEREN_BOLD _("Beleren")
