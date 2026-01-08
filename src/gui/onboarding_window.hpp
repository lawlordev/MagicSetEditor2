//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/about_window.hpp> // for HoverButtonBase

// ----------------------------------------------------------------------------- : Simple text button for onboarding

/// A simple text button with hover effect for the onboarding screen
class OnboardingButton : public HoverButtonBase {
public:
  OnboardingButton(Window* parent, int id, const String& label);

  void SetLabel(const String& label) override;

private:
  String label;
  wxFont font;

  wxSize DoGetBestSize() const override;
  void draw(DC& dc) override;
};

// ----------------------------------------------------------------------------- : OnboardingWindow

/// Onboarding window shown at startup before the Welcome window
/** Guides the user through initial setup steps:
 *    - Update check (GitHub data pack updates)
 *    - Font installation check
 */
class OnboardingWindow : public wxFrame {
public:
  OnboardingWindow();

private:
  DECLARE_EVENT_TABLE();

  enum Step {
    STEP_FONTS,
    STEP_UPDATES
  };

  /// State for the update checking step
  enum UpdateState {
    UPDATE_IDLE,
    UPDATE_CHECKING,
    UPDATE_AVAILABLE,
    UPDATE_DOWNLOADING,
    UPDATE_COMPLETE,
    UPDATE_ERROR,
    UPDATE_UP_TO_DATE,
    UPDATE_POSTPONED
  };

  Step currentStep;
  UpdateState updateState;

  // Header elements (painted)
  Bitmap logo;

  // Content area elements
  wxStaticText* titleLabel;
  wxStaticText* statusLabel;
  OnboardingButton* actionButton;
  OnboardingButton* continueButton;

  // Font step state
  int missingFontCount;

  void showStep(Step step);
  void updateFontsStep();
  void updateUpdatesStep();

  void onPaint(wxPaintEvent&);
  void draw(DC& dc);

  void onActionButton(wxCommandEvent&);
  void onContinue(wxCommandEvent&);
  void onComplete();

  // Idle handler for polling update checker status
  void onIdle(wxIdleEvent& ev);
};

// ----------------------------------------------------------------------------- : Event IDs

enum {
  ID_ONBOARDING_ACTION = wxID_HIGHEST + 100,
  ID_ONBOARDING_CONTINUE
};
