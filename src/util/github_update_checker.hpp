//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <wx/thread.h>

// ----------------------------------------------------------------------------- : Data Structures

/// Information about a file that needs updating
struct FileUpdateInfo {
  String path;           ///< Relative path in repo
  String sha;            ///< GitHub SHA hash
  size_t size;           ///< File size in bytes
  bool isNew;            ///< True if file doesn't exist locally
};

/// Result of update check
struct UpdateCheckResult {
  bool success = false;
  String errorMessage;
  vector<FileUpdateInfo> filesToUpdate;
  size_t totalDownloadSize = 0;
  int fileCount = 0;
};

/// Download progress data
struct DownloadProgress {
  int currentFile = 0;
  int totalFiles = 0;
  size_t bytesDownloaded = 0;
  size_t totalBytes = 0;
  String currentFileName;
};

// ----------------------------------------------------------------------------- : GitHubUpdateChecker

/// Manages GitHub-based update checking and downloading for data packs
class GitHubUpdateChecker {
public:
  /// Status enum for the update checker
  enum Status {
    IDLE,
    CHECKING,
    CHECK_COMPLETE,
    DOWNLOADING,
    DOWNLOAD_COMPLETE,
    ERROR
  };

  /// Get the singleton instance
  static GitHubUpdateChecker& getInstance();

  /// Start async update check
  void startCheck();

  /// Start async download of updates
  void startDownload();

  /// Get current status (thread-safe)
  Status getStatus();

  /// Get check result (only valid after CHECK_COMPLETE)
  const UpdateCheckResult& getResult();

  /// Get download progress (only valid during DOWNLOADING)
  DownloadProgress getProgress();

  /// Get the app support directory for the current platform
  static String getAppSupportDirectory();

  /// Get local data directory (where we sync to)
  static String getLocalDataDirectory();

  /// Mark update as postponed (for 24 hours)
  void postponeUpdate();

  /// Check if user has postponed recently
  static bool isPostponed();

private:
  GitHubUpdateChecker() = default;
  ~GitHubUpdateChecker() = default;

  // Prevent copying
  GitHubUpdateChecker(const GitHubUpdateChecker&) = delete;
  GitHubUpdateChecker& operator=(const GitHubUpdateChecker&) = delete;

  wxMutex mutex;
  Status status = IDLE;
  UpdateCheckResult result;
  DownloadProgress progress;

  // Allow thread classes to access private members
  friend class GitHubUpdateCheckerThread;
  friend class GitHubDownloadThread;
};

/// Convenience accessor for the singleton
inline GitHubUpdateChecker& githubUpdateChecker() {
  return GitHubUpdateChecker::getInstance();
}
