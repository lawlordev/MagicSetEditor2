//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <util/github_update_checker.hpp>
#include <util/json_parser.hpp>
#include <data/settings.hpp>
#include <wx/wfstream.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/zipstrm.h>
#include <wx/wfstream.h>

// ----------------------------------------------------------------------------- : Constants

// GitHub repository info
const char* GITHUB_REPO_INFO_URL = "https://api.github.com/repos/MagicSetEditorPacks/Full-Magic-Pack/commits/main";
const char* GITHUB_ZIP_URL = "https://github.com/MagicSetEditorPacks/Full-Magic-Pack/archive/refs/heads/main.zip";

// Local marker file name
const char* VERSION_MARKER_FILE = "pack_version.txt";

// ----------------------------------------------------------------------------- : Platform-specific paths

String GitHubUpdateChecker::getAppSupportDirectory() {
#if defined(__APPLE__)
  return wxGetHomeDir() + wxS("/Library/Application Support/MSE3");
#elif defined(__WXMSW__)
  return wxStandardPaths::Get().GetUserDataDir();
#else
  return wxGetHomeDir() + wxS("/.local/share/MSE3");
#endif
}

String GitHubUpdateChecker::getLocalDataDirectory() {
  String dir = getAppSupportDirectory();
  if (!wxDirExists(dir)) {
    // Create directory with full path (including parents)
    if (!wxFileName::Mkdir(dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
      // If mkdir fails, try creating parent first
      wxFileName fn(dir + wxS("/"));
      fn.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
  }
  return dir;
}

// ----------------------------------------------------------------------------- : Helper functions

/// Read the stored version SHA from local marker file
static String readLocalVersion() {
  try {
    String baseDir = GitHubUpdateChecker::getLocalDataDirectory();
    String markerPath = baseDir + wxS("/") + VERSION_MARKER_FILE;
    if (!wxFileExists(markerPath)) {
      return wxEmptyString;
    }

    wxFile file(markerPath, wxFile::read);
    if (!file.IsOpened()) {
      return wxEmptyString;
    }

    wxString content;
    file.ReadAll(&content);
    return content.Trim();
  } catch (...) {
    return wxEmptyString;
  }
}

/// Write the version SHA to local marker file
static void writeLocalVersion(const String& sha) {
  try {
    String baseDir = GitHubUpdateChecker::getLocalDataDirectory();
    String markerPath = baseDir + wxS("/") + VERSION_MARKER_FILE;
    wxFile file(markerPath, wxFile::write);
    if (file.IsOpened()) {
      file.Write(sha);
    }
  } catch (...) {
    // Ignore write errors
  }
}

/// Check if local data folders exist
static bool localDataExists() {
  try {
    String baseDir = GitHubUpdateChecker::getLocalDataDirectory();
    return wxDirExists(baseDir + wxS("/data")) || wxDirExists(baseDir + wxS("/Magic - Fonts"));
  } catch (...) {
    return false;
  }
}

// ----------------------------------------------------------------------------- : GitHubUpdateChecker Implementation

GitHubUpdateChecker& GitHubUpdateChecker::getInstance() {
  static GitHubUpdateChecker instance;
  return instance;
}

void GitHubUpdateChecker::startCheck() {
  // Run check synchronously but quickly (just one small API call)
  {
    wxMutexLocker lock(mutex);
    if (status == CHECKING || status == DOWNLOADING) return;
    status = CHECKING;
    result = UpdateCheckResult();
  }

  try {
    // Ensure app support directory exists
    getLocalDataDirectory();
  } catch (...) {
    // Directory creation failed, but continue anyway
  }

  // Get local version
  String localVersion = readLocalVersion();
  bool hasLocalData = localDataExists();

  // Fetch remote version using curl (synchronous, but fast - small response)
  wxArrayString output;
  wxArrayString errors;
  wxString cmd = wxString::Format(wxS("curl -s -L \"%s\""), GITHUB_REPO_INFO_URL);

  int exitCode = wxExecute(cmd, output, errors, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);

  if (exitCode != 0 || output.empty()) {
    wxMutexLocker lock(mutex);
    result.errorMessage = _("Failed to connect to GitHub");
    status = ERROR;
    return;
  }

  // Combine output and parse JSON
  std::string jsonStr;
  for (const auto& line : output) {
    jsonStr += line.ToStdString();
  }

  try {
    json commitData = json::parse(jsonStr);

    if (!commitData.contains("sha")) {
      wxMutexLocker lock(mutex);
      result.errorMessage = _("Invalid response from GitHub");
      status = ERROR;
      return;
    }

    String remoteSha = wxString::FromUTF8(commitData["sha"].get<std::string>());

    // Compare versions
    bool needsUpdate = false;
    if (!hasLocalData) {
      // No local data - need initial download
      needsUpdate = true;
      result.errorMessage = _("Initial setup required");
    } else if (localVersion.empty()) {
      // Have data but no version marker - assume needs update
      needsUpdate = true;
    } else if (localVersion != remoteSha) {
      // Version mismatch - updates available
      needsUpdate = true;
    }

    {
      wxMutexLocker lock(mutex);
      result.success = true;

      if (needsUpdate) {
        // Store the remote SHA for download
        FileUpdateInfo info;
        info.sha = remoteSha;
        info.path = wxS("Full-Magic-Pack");
        info.size = 500 * 1024 * 1024;  // Estimate ~500MB for the full pack
        info.isNew = !hasLocalData;
        result.filesToUpdate.push_back(info);
        result.fileCount = 1;
        result.totalDownloadSize = info.size;
      } else {
        result.fileCount = 0;
        result.totalDownloadSize = 0;
      }

      status = CHECK_COMPLETE;
    }

  } catch (const std::exception& e) {
    wxMutexLocker lock(mutex);
    result.errorMessage = wxString::FromUTF8(e.what());
    status = ERROR;
  }
}

void GitHubUpdateChecker::startDownload() {
  {
    wxMutexLocker lock(mutex);
    if (status != CHECK_COMPLETE || result.filesToUpdate.empty()) return;
    status = DOWNLOADING;
    progress = DownloadProgress();
    progress.totalFiles = 1;
    progress.currentFile = 1;
    progress.currentFileName = wxS("Full-Magic-Pack.zip");
  }

  String remoteSha = result.filesToUpdate[0].sha;
  String baseDir = getLocalDataDirectory();
  String zipPath = baseDir + wxS("/Full-Magic-Pack.zip");

  // Download the zip file
  {
    wxMutexLocker lock(mutex);
    progress.currentFileName = wxS("Downloading Full-Magic-Pack.zip...");
  }

  wxString cmd = wxString::Format(wxS("curl -s -L -o \"%s\" \"%s\""), zipPath, GITHUB_ZIP_URL);
  int exitCode = wxExecute(cmd, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);

  if (exitCode != 0 || !wxFileExists(zipPath)) {
    wxMutexLocker lock(mutex);
    result.errorMessage = _("Failed to download update");
    status = ERROR;
    return;
  }

  // Extract the zip file
  {
    wxMutexLocker lock(mutex);
    progress.currentFileName = wxS("Extracting files...");
  }

  // Use unzip command (available on macOS/Linux, and usually Windows)
  wxString extractCmd = wxString::Format(wxS("unzip -o -q \"%s\" -d \"%s\""), zipPath, baseDir);
  exitCode = wxExecute(extractCmd, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);

  if (exitCode != 0) {
    wxMutexLocker lock(mutex);
    result.errorMessage = _("Failed to extract update");
    status = ERROR;
    wxRemoveFile(zipPath);
    return;
  }

  // The zip extracts to Full-Magic-Pack-main/ folder
  // Move contents to the right places
  String extractedDir = baseDir + wxS("/Full-Magic-Pack-main");

  // Remove old folders and move new ones
  String oldDataDir = baseDir + wxS("/data");
  String oldFontsDir = baseDir + wxS("/Magic - Fonts");
  String newDataDir = extractedDir + wxS("/data");
  String newFontsDir = extractedDir + wxS("/Magic - Fonts");

  // Remove old directories if they exist
  if (wxDirExists(oldDataDir)) {
    wxString rmCmd = wxString::Format(wxS("rm -rf \"%s\""), oldDataDir);
    wxExecute(rmCmd, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
  }
  if (wxDirExists(oldFontsDir)) {
    wxString rmCmd = wxString::Format(wxS("rm -rf \"%s\""), oldFontsDir);
    wxExecute(rmCmd, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
  }

  // Move new directories
  if (wxDirExists(newDataDir)) {
    wxString mvCmd = wxString::Format(wxS("mv \"%s\" \"%s\""), newDataDir, oldDataDir);
    wxExecute(mvCmd, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
  }
  if (wxDirExists(newFontsDir)) {
    wxString mvCmd = wxString::Format(wxS("mv \"%s\" \"%s\""), newFontsDir, oldFontsDir);
    wxExecute(mvCmd, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
  }

  // Clean up
  wxRemoveFile(zipPath);
  if (wxDirExists(extractedDir)) {
    wxString rmCmd = wxString::Format(wxS("rm -rf \"%s\""), extractedDir);
    wxExecute(rmCmd, wxEXEC_SYNC | wxEXEC_HIDE_CONSOLE);
  }

  // Save the version marker
  writeLocalVersion(remoteSha);

  {
    wxMutexLocker lock(mutex);
    progress.bytesDownloaded = progress.totalBytes;
    status = DOWNLOAD_COMPLETE;
  }
}

GitHubUpdateChecker::Status GitHubUpdateChecker::getStatus() {
  wxMutexLocker lock(mutex);
  return status;
}

const UpdateCheckResult& GitHubUpdateChecker::getResult() {
  wxMutexLocker lock(mutex);
  return result;
}

DownloadProgress GitHubUpdateChecker::getProgress() {
  wxMutexLocker lock(mutex);
  return progress;
}

void GitHubUpdateChecker::postponeUpdate() {
  // No longer using timed postpone - just skip for this session
  // The check will run again next time the app opens
}

bool GitHubUpdateChecker::isPostponed() {
  // Always return false - we want to check every time
  return false;
}
