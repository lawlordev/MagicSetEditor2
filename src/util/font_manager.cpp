//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/font_manager.hpp>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/fontenum.h>

// ----------------------------------------------------------------------------- : FontManager singleton

FontManager& FontManager::getInstance() {
  static FontManager instance;
  return instance;
}

// ----------------------------------------------------------------------------- : Directory paths

String FontManager::getUserFontDirectory() {
#if defined(__APPLE__)
  // macOS: ~/Library/Fonts/
  return wxGetHomeDir() + _("/Library/Fonts");
#elif defined(__WXMSW__)
  // Windows: %LOCALAPPDATA%\Microsoft\Windows\Fonts
  return wxStandardPaths::Get().GetUserLocalDataDir() + _("\\Microsoft\\Windows\\Fonts");
#else
  // Linux: ~/.local/share/fonts/
  return wxGetHomeDir() + _("/.local/share/fonts");
#endif
}

String FontManager::getBundledFontsDirectory() {
  // Look for "Magic - Fonts" directory relative to the executable
  String dataDir = wxStandardPaths::Get().GetDataDir();
  String fontsDir = dataDir + _("/Magic - Fonts");

  if (wxDirExists(fontsDir)) {
    return fontsDir;
  }

  // Try parent directory (for development builds)
  wxFileName fn(dataDir);
  fn.RemoveLastDir();
  fontsDir = fn.GetPath() + _("/Magic - Fonts");

  if (wxDirExists(fontsDir)) {
    return fontsDir;
  }

  // Try alongside executable
  fontsDir = wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + _("/Magic - Fonts");

  return fontsDir;
}

// ----------------------------------------------------------------------------- : Font discovery

class FontFileTraverser : public wxDirTraverser {
public:
  std::vector<String>& fonts;

  FontFileTraverser(std::vector<String>& f) : fonts(f) {}

  wxDirTraverseResult OnFile(const wxString& filename) override {
    wxString ext = wxFileName(filename).GetExt().Lower();
    if (ext == _("ttf") || ext == _("otf")) {
      fonts.push_back(filename);
    }
    return wxDIR_CONTINUE;
  }

  wxDirTraverseResult OnDir(const wxString&) override {
    return wxDIR_CONTINUE;
  }
};

std::vector<String> FontManager::findAllFonts(const String& directory) {
  std::vector<String> fonts;

  if (!wxDirExists(directory)) {
    return fonts;
  }

  wxDir dir(directory);
  if (!dir.IsOpened()) {
    return fonts;
  }

  FontFileTraverser traverser(fonts);
  dir.Traverse(traverser);

  return fonts;
}

// ----------------------------------------------------------------------------- : Font installation check

bool FontManager::isFontFileInstalled(const String& fontFileName) {
  String userFontDir = getUserFontDirectory();
  wxFileName fn(fontFileName);
  String destPath = userFontDir + _("/") + fn.GetFullName();
  return wxFileExists(destPath);
}

int FontManager::countMissingFonts() {
  String fontsDir = getBundledFontsDirectory();
  std::vector<String> allFonts = findAllFonts(fontsDir);

  int missing = 0;
  for (const String& fontPath : allFonts) {
    if (!isFontFileInstalled(fontPath)) {
      missing++;
    }
  }
  return missing;
}

// ----------------------------------------------------------------------------- : Font installation

bool FontManager::installFontFile(const String& sourcePath) {
  String userFontDir = getUserFontDirectory();

  // Create font directory if it doesn't exist
  if (!wxDirExists(userFontDir)) {
    wxFileName::Mkdir(userFontDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
  }

  wxFileName fn(sourcePath);
  String destPath = userFontDir + _("/") + fn.GetFullName();

  // Skip if already installed
  if (wxFileExists(destPath)) {
    return true;
  }

  // Copy the font file
  return wxCopyFile(sourcePath, destPath);
}

int FontManager::installAllFonts() {
  String fontsDir = getBundledFontsDirectory();
  std::vector<String> allFonts = findAllFonts(fontsDir);

  int installed = 0;
  for (const String& fontPath : allFonts) {
    if (installFontFile(fontPath)) {
      installed++;
    }
  }

  // Refresh font cache after installing
  if (installed > 0) {
    refreshFontCache();
  }

  return installed;
}

void FontManager::refreshFontCache() {
#if defined(__APPLE__)
  // macOS: Use atsutil to reset font cache
  wxExecute(_("atsutil databases -remove"), wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
#elif defined(__WXMSW__)
  // Windows: Broadcast font change message
  // Note: This requires Windows API calls, fonts should be available after restart
#else
  // Linux: Run fc-cache
  wxExecute(_("fc-cache -f"), wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
#endif
}

// ----------------------------------------------------------------------------- : Beleren check

bool FontManager::isBelerenAvailable() {
  // Check if Beleren Bold is in the system font list
  wxArrayString facenames = wxFontEnumerator::GetFacenames();
  for (const wxString& face : facenames) {
    if (face.Contains(_("Beleren")) || face.Contains(_("beleren"))) {
      return true;
    }
  }

  // Also check if the file is installed
  return isFontFileInstalled(_("beleren-bold_P1.01.ttf"));
}
