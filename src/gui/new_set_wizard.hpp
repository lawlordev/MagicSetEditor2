//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/control/gallery_list.hpp>
#include <functional>

class PackageList;
class StyledPackageGrid;
class StyledSearchBox;
class NeomorphicButton;
class Game;
DECLARE_POINTER_TYPE(Set);
DECLARE_POINTER_TYPE(StyleSheet);
DECLARE_POINTER_TYPE(Game);

// ----------------------------------------------------------------------------- : SearchablePackageList

/// A package list with search/filter functionality and styled grid
class SearchablePackageList : public wxPanel {
public:
  SearchablePackageList(Window* parent, int id, int direction = wxVERTICAL, bool showSearch = true);

  /// Shows packages that match a specific pattern, and that are of the given type
  template <typename T>
  void showData(const String& pattern = _("*")) {
    showDataInternal(pattern + _(".mse-") + T::typeNameStatic());
  }

  /// Filter the list by search text
  void filterBySearch(const String& searchText);

  /// Clear the list
  void clear();

  /// Select package by name
  void select(const String& name, bool send_event = true);

  /// Is there a selection?
  bool hasSelection() const;

  /// Set number of columns in grid
  void setColumnCount(size_t cols);

  /// Get the underlying grid (for getting selection)
  StyledPackageGrid* getGrid() { return styledGrid; }

  /// Scroll to top of the list
  void scrollToTop();

private:
  PackageList* list;  // Not used anymore, kept for compatibility
  StyledPackageGrid* styledGrid;
  StyledSearchBox* styledSearchBox;
  wxTextCtrl* searchBox;
  String currentPattern;
  String searchText;

  void showDataInternal(const String& pattern);
  void onSearchText(wxCommandEvent& ev);

  DECLARE_EVENT_TABLE();
};

// ----------------------------------------------------------------------------- : NewSetWizard

/// Multi-step wizard for creating a new set
class NewSetWizard : public wxDialog {
public:
  /// The newly created set, if any
  SetP set;

  NewSetWizard(Window* parent);

private:
  DECLARE_EVENT_TABLE();

  enum WizardStep {
    STEP_SELECT_GAME,
    STEP_SELECT_STYLE
  };

  WizardStep currentStep;

  // Fonts
  wxFont titleFont;
  wxFont subtitleFont;
  wxFont labelFont;

  // Step 1: Game selection
  wxPanel* gamePanel;
  wxStaticText* gameTitleLabel;
  wxStaticText* gameSubtitleLabel;
  SearchablePackageList* gameList;
  NeomorphicButton* gameNextButton;

  // Step 2: Style selection
  wxPanel* stylePanel;
  wxStaticText* styleTitleLabel;
  wxStaticText* styleSubtitleLabel;
  wxStaticText* styleDescLabel;
  SearchablePackageList* styleList;
  NeomorphicButton* styleBackButton;
  NeomorphicButton* styleCreateButton;

  // Selected game (for step 2)
  GameP selectedGame;

  // --------------------------------------------------- : Methods

  void createGameStep();
  void createStyleStep();

  void showStep(WizardStep step);
  void updateButtonStates();

  // --------------------------------------------------- : Events

  void onGameSelect(wxCommandEvent&);
  void onGameActivate(wxCommandEvent&);
  void onGameNext(wxCommandEvent&);

  void onStyleSelect(wxCommandEvent&);
  void onStyleActivate(wxCommandEvent&);
  void onStyleBack(wxCommandEvent&);
  void onStyleCreate(wxCommandEvent&);

  void onUpdateUI(wxUpdateUIEvent&);
  void onIdle(wxIdleEvent&);

  void done();
};

// ----------------------------------------------------------------------------- : Entry point

/// Show the new set wizard, return the new set, if any
SetP new_set_wizard(Window* parent);

// ----------------------------------------------------------------------------- : Event IDs

enum {
  ID_WIZARD_GAME_LIST = wxID_HIGHEST + 200,
  ID_WIZARD_STYLE_LIST,
  ID_WIZARD_GAME_SEARCH,
  ID_WIZARD_STYLE_SEARCH,
  ID_WIZARD_GAME_NEXT,
  ID_WIZARD_STYLE_BACK,
  ID_WIZARD_STYLE_CREATE
};

