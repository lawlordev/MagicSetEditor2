//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/onboarding_window.hpp>
#include <gui/welcome_window.hpp>
#include <gui/util.hpp>
#include <util/font_manager.hpp>
#include <util/github_update_checker.hpp>
#include <util/io/package_manager.hpp>
#include <data/locale.hpp>
#include <data/settings.hpp>
#include <wx/dcbuffer.h>

// ----------------------------------------------------------------------------- : OnboardingButton

OnboardingButton::OnboardingButton(Window* parent, int id, const String& label)
  : HoverButtonBase(parent, id, true)
  , label(label)
  , font(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, _("Beleren"))
{
  SetMinSize(wxSize(180, 40));
}

void OnboardingButton::SetLabel(const String& newLabel) {
  label = newLabel;
  Refresh();
}

wxSize OnboardingButton::DoGetBestSize() const {
  return wxSize(180, 40);
}

void OnboardingButton::draw(DC& dc) {
  wxSize ws = GetClientSize();

  // Clear background
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.SetBrush(wxColour(255, 255, 255));
  dc.DrawRectangle(0, 0, ws.GetWidth(), ws.GetHeight());

  // Button colors based on state
  Color bgColor, borderColor, textColor;
  bool isPressed = (mouse_down && hover) || key_down;

  if (isPressed) {
    bgColor = Color(245, 245, 245);
    borderColor = Color(180, 60, 60);
    textColor = Color(35, 40, 50);
  } else if (hover || focus) {
    bgColor = Color(255, 255, 255);
    borderColor = Color(200, 80, 80);
    textColor = Color(35, 40, 50);
  } else {
    bgColor = Color(255, 255, 255);
    borderColor = Color(200, 200, 200);
    textColor = Color(60, 65, 75);
  }

  // Draw button background with rounded corners
  int d = isPressed ? 1 : 0;
  dc.SetPen(wxPen(borderColor, 1));
  dc.SetBrush(wxBrush(bgColor));
  dc.DrawRoundedRectangle(1 + d, 1 + d, ws.GetWidth() - 2, ws.GetHeight() - 2, 6);

  // Draw label centered
  dc.SetFont(font);
  dc.SetTextForeground(textColor);
  int tw, th;
  dc.GetTextExtent(label, &tw, &th);
  dc.DrawText(label, (ws.GetWidth() - tw) / 2 + d, (ws.GetHeight() - th) / 2 + d);
}

// ----------------------------------------------------------------------------- : OnboardingWindow

OnboardingWindow::OnboardingWindow()
  : wxFrame(nullptr, wxID_ANY, _TITLE_("magic set editor"), wxDefaultPosition, wxSize(540, 400), wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN)
  , logo(load_resource_image(_("about")))
  , currentStep(STEP_FONTS)
  , updateState(UPDATE_IDLE)
  , missingFontCount(0)
{
  SetIcon(load_resource_icon(_("app")));
  SetBackgroundStyle(wxBG_STYLE_PAINT);

  // Scale the logo to fit nicely (target ~380px wide for compact look)
  if (logo.Ok() && logo.GetWidth() > 380) {
    double scale = 380.0 / logo.GetWidth();
    logo = wxBitmap(logo.ConvertToImage().Scale(
      static_cast<int>(logo.GetWidth() * scale),
      static_cast<int>(logo.GetHeight() * scale),
      wxIMAGE_QUALITY_HIGH
    ));
  }

  // Calculate header height
  int logoHeight = logo.Ok() ? logo.GetHeight() : 80;
  int headerHeight = logoHeight + 75;

  // Create content area
  wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
  mainSizer->AddSpacer(headerHeight);

  // Content panel for dynamic elements
  wxPanel* contentPanel = new wxPanel(this, wxID_ANY);
  contentPanel->SetBackgroundStyle(wxBG_STYLE_SYSTEM);

  wxSizer* contentSizer = new wxBoxSizer(wxVERTICAL);

  // Title label (step name)
  titleLabel = new wxStaticText(contentPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
  titleLabel->SetFont(wxFont(16, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, _("Beleren")));
  titleLabel->SetForegroundColour(wxColour(35, 40, 50));

  // Status label (description/status)
  statusLabel = new wxStaticText(contentPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
  statusLabel->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _("Beleren")));
  statusLabel->SetForegroundColour(wxColour(100, 105, 115));

  // Action button (e.g., "Install Fonts")
  actionButton = new OnboardingButton(contentPanel, ID_ONBOARDING_ACTION, wxEmptyString);

  // Continue button
  continueButton = new OnboardingButton(contentPanel, ID_ONBOARDING_CONTINUE, _("Continue"));

  contentSizer->Add(titleLabel, 0, wxALIGN_CENTER | wxTOP, 20);
  contentSizer->Add(statusLabel, 0, wxALIGN_CENTER | wxTOP, 12);
  contentSizer->Add(actionButton, 0, wxALIGN_CENTER | wxTOP, 24);
  contentSizer->Add(continueButton, 0, wxALIGN_CENTER | wxTOP, 16);

  contentPanel->SetSizer(contentSizer);

  mainSizer->Add(contentPanel, 1, wxEXPAND);
  SetSizer(mainSizer);

  // Initialize first step
  showStep(STEP_UPDATES);

  CentreOnScreen();
}

void OnboardingWindow::onPaint(wxPaintEvent&) {
  wxBufferedPaintDC dc(this);
  draw(dc);
}

void OnboardingWindow::draw(DC& dc) {
  wxSize ws = GetClientSize();

  // Pure white background
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.SetBrush(wxColour(255, 255, 255));
  dc.DrawRectangle(0, 0, ws.GetWidth(), ws.GetHeight());

  // Red accent line under logo area
  int lineY = logo.Ok() ? logo.GetHeight() + 50 : 120;
  dc.SetPen(wxPen(wxColour(200, 60, 60), 2));
  dc.DrawLine(0, lineY, ws.GetWidth(), lineY);

  // Draw logo centered in header
  if (logo.Ok()) {
    int logoX = (ws.GetWidth() - logo.GetWidth()) / 2;
    int logoY = 25;
    dc.DrawBitmap(logo, logoX, logoY);
  }
}

void OnboardingWindow::showStep(Step step) {
  currentStep = step;

  switch (step) {
    case STEP_FONTS:
      updateFontsStep();
      break;
    case STEP_UPDATES:
      updateUpdatesStep();
      break;
  }

  Layout();
  Refresh();
}

void OnboardingWindow::updateFontsStep() {
  titleLabel->SetLabel(_("Font Installation"));

  // Check for missing fonts
  missingFontCount = fontManager().countMissingFonts();

  if (missingFontCount > 0) {
    statusLabel->SetLabel(wxString::Format(_("%d fonts are available for installation"), missingFontCount));
    actionButton->SetLabel(wxString::Format(_("Install %d Fonts"), missingFontCount));
    actionButton->Show(true);
  } else {
    statusLabel->SetLabel(_("All fonts are installed"));
    actionButton->Show(false);
  }

  continueButton->SetLabel(_("Get Started"));
}

void OnboardingWindow::updateUpdatesStep() {
  titleLabel->SetLabel(_("Data Pack"));

  updateState = UPDATE_CHECKING;
  statusLabel->SetLabel(_("Checking for updates..."));
  actionButton->Show(false);
  continueButton->Show(false);
  Layout();
  Refresh();
  Update();  // Force immediate repaint

  // Run the check (it's fast now - just one small API call)
  githubUpdateChecker().startCheck();

  // Check result immediately since it runs synchronously
  auto status = githubUpdateChecker().getStatus();
  if (status == GitHubUpdateChecker::CHECK_COMPLETE) {
    const auto& result = githubUpdateChecker().getResult();
    if (result.fileCount > 0) {
      updateState = UPDATE_AVAILABLE;

      // Check if this is initial setup or an update
      bool isInitial = result.filesToUpdate[0].isNew;

      if (isInitial) {
        statusLabel->SetLabel(_("Data pack not installed. Download required (~500 MB)"));
        actionButton->SetLabel(_("Download Now"));
      } else {
        statusLabel->SetLabel(_("Updates available (~500 MB)"));
        actionButton->SetLabel(_("Download Updates"));
      }
      actionButton->Show(true);
      continueButton->SetLabel(_("Skip"));
      continueButton->Show(true);
    } else {
      updateState = UPDATE_UP_TO_DATE;
      statusLabel->SetLabel(_("Data pack is up to date"));
      actionButton->Show(false);
      continueButton->SetLabel(_("Continue"));
      continueButton->Show(true);
    }
  } else if (status == GitHubUpdateChecker::ERROR) {
    updateState = UPDATE_ERROR;
    const auto& result = githubUpdateChecker().getResult();
    statusLabel->SetLabel(wxString::Format(_("Error: %s"), result.errorMessage));
    actionButton->Show(false);
    continueButton->SetLabel(_("Continue"));
    continueButton->Show(true);
  }

  Layout();
  Refresh();
}

void OnboardingWindow::onActionButton(wxCommandEvent&) {
  if (currentStep == STEP_UPDATES && updateState == UPDATE_AVAILABLE) {
    // Start downloading updates
    updateState = UPDATE_DOWNLOADING;
    actionButton->Show(false);
    continueButton->Show(false);
    statusLabel->SetLabel(_("Starting download..."));
    Layout();
    Refresh();
    githubUpdateChecker().startDownload();
  } else if (currentStep == STEP_FONTS) {
    // Install fonts
    wxBusyCursor wait;
    int installed = fontManager().installAllFonts();

    if (installed > 0) {
      missingFontCount = fontManager().countMissingFonts();

      if (missingFontCount == 0) {
        statusLabel->SetLabel(_("All fonts installed successfully.\nPlease restart your computer and relaunch the app to see the new fonts."));
        actionButton->Show(false);
      } else {
        statusLabel->SetLabel(wxString::Format(_("%d fonts remaining"), missingFontCount));
        actionButton->SetLabel(wxString::Format(_("Install %d Fonts"), missingFontCount));
      }

      Layout();
      Refresh();
    }
  }
}

void OnboardingWindow::onContinue(wxCommandEvent&) {
  switch (currentStep) {
    case STEP_UPDATES:
      // User clicked "Skip" - just continue to next step
      // Next app launch will check again
      showStep(STEP_FONTS);
      break;
    case STEP_FONTS:
      onComplete();
      break;
  }
}

void OnboardingWindow::onComplete() {
  // Load locale now that data is available (was skipped during startup)
  if (package_manager.hasData()) {
    the_locale = Locale::byName(settings.locale);
  }

  // Show welcome window and close this one
  (new WelcomeWindow())->Show();
  Close();
}

void OnboardingWindow::onIdle(wxIdleEvent& ev) {
  // Only handle idle events during download
  if (currentStep != STEP_UPDATES || updateState != UPDATE_DOWNLOADING) return;

  auto status = githubUpdateChecker().getStatus();

  switch (status) {
    case GitHubUpdateChecker::DOWNLOADING: {
      auto progress = githubUpdateChecker().getProgress();
      statusLabel->SetLabel(progress.currentFileName);
      Layout();
      Refresh();
      ev.RequestMore();
      break;
    }

    case GitHubUpdateChecker::DOWNLOAD_COMPLETE:
      updateState = UPDATE_COMPLETE;
      // Reinitialize package manager now that data is available
      package_manager.reinit();
      statusLabel->SetLabel(_("Data pack installed successfully!"));
      actionButton->Show(false);
      continueButton->SetLabel(_("Continue"));
      continueButton->Show(true);
      Layout();
      Refresh();
      break;

    case GitHubUpdateChecker::ERROR: {
      updateState = UPDATE_ERROR;
      const auto& result = githubUpdateChecker().getResult();
      statusLabel->SetLabel(wxString::Format(_("Error: %s"), result.errorMessage));
      actionButton->Show(false);
      continueButton->SetLabel(_("Continue"));
      continueButton->Show(true);
      Layout();
      Refresh();
      break;
    }

    default:
      break;
  }
}

BEGIN_EVENT_TABLE(OnboardingWindow, wxFrame)
  EVT_PAINT(OnboardingWindow::onPaint)
  EVT_BUTTON(ID_ONBOARDING_ACTION, OnboardingWindow::onActionButton)
  EVT_BUTTON(ID_ONBOARDING_CONTINUE, OnboardingWindow::onContinue)
  EVT_IDLE(OnboardingWindow::onIdle)
END_EVENT_TABLE()
