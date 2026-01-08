//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/new_set_wizard.hpp>
#include <gui/control/package_list.hpp>
#include <gui/util.hpp>
#include <util/io/package_manager.hpp>
#include <data/game.hpp>
#include <data/stylesheet.hpp>
#include <data/set.hpp>
#include <data/card.hpp>
#include <data/settings.hpp>
#include <util/window_id.hpp>
#include <util/alignment.hpp>
#include <wx/dcbuffer.h>

#ifdef __WXOSX__
#include <objc/objc.h>
#include <objc/message.h>
#endif

// ----------------------------------------------------------------------------- : NeomorphicButton

/// A custom styled button with neomorphic design and hover effects
class NeomorphicButton : public wxWindow {
public:
  NeomorphicButton(wxWindow* parent, wxWindowID id, const String& label,
                   bool isPrimary = true, const wxSize& size = wxSize(140, 40));

  void SetLabel(const String& label) { this->label = label; Refresh(); }
  bool Enable(bool enable = true) override { enabled = enable; Refresh(); return true; }
  bool IsEnabled() const { return enabled; }

private:
  String label;
  bool isPrimary;
  bool enabled = true;
  bool hover = false;
  bool pressed = false;
  wxFont font;

  void onPaint(wxPaintEvent& ev);
  void onMouseEnter(wxMouseEvent& ev);
  void onMouseLeave(wxMouseEvent& ev);
  void onLeftDown(wxMouseEvent& ev);
  void onLeftUp(wxMouseEvent& ev);

  DECLARE_EVENT_TABLE();
};

NeomorphicButton::NeomorphicButton(wxWindow* parent, wxWindowID id, const String& label,
                                   bool isPrimary, const wxSize& size)
  : wxWindow(parent, id, wxDefaultPosition, size)
  , label(label)
  , isPrimary(isPrimary)
  , font(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, _("Beleren"))
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetMinSize(size);
}

void NeomorphicButton::onPaint(wxPaintEvent&) {
  wxBufferedPaintDC dc(this);
  wxSize size = GetClientSize();

  // Clear background (parent's white)
  dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.DrawRectangle(0, 0, size.x, size.y);

  // Button colors
  wxColour bgColor, borderColor, textColor;

  if (!enabled) {
    bgColor = wxColour(240, 240, 242);
    borderColor = wxColour(210, 210, 215);
    textColor = wxColour(160, 160, 165);
  } else if (pressed) {
    if (isPrimary) {
      bgColor = wxColour(50, 110, 180);
      borderColor = wxColour(40, 90, 150);
      textColor = wxColour(255, 255, 255);
    } else {
      bgColor = wxColour(235, 235, 238);
      borderColor = wxColour(180, 180, 185);
      textColor = wxColour(50, 55, 65);
    }
  } else if (hover) {
    if (isPrimary) {
      bgColor = wxColour(80, 145, 220);
      borderColor = wxColour(60, 120, 190);
      textColor = wxColour(255, 255, 255);
    } else {
      bgColor = wxColour(245, 245, 248);
      borderColor = wxColour(190, 190, 195);
      textColor = wxColour(40, 45, 55);
    }
  } else {
    if (isPrimary) {
      bgColor = wxColour(70, 130, 200);
      borderColor = wxColour(55, 110, 175);
      textColor = wxColour(255, 255, 255);
    } else {
      bgColor = wxColour(255, 255, 255);
      borderColor = wxColour(200, 200, 205);
      textColor = wxColour(50, 55, 65);
    }
  }

  // Draw button background with rounded corners
  dc.SetBrush(wxBrush(bgColor));
  dc.SetPen(wxPen(borderColor, 1));
  dc.DrawRoundedRectangle(0, 0, size.x, size.y, 8);

  // Draw label centered
  dc.SetFont(font);
  dc.SetTextForeground(textColor);
  wxSize textSize = dc.GetTextExtent(label);
  int textX = (size.x - textSize.x) / 2;
  int textY = (size.y - textSize.y) / 2;
  dc.DrawText(label, textX, textY);
}

void NeomorphicButton::onMouseEnter(wxMouseEvent&) {
  hover = true;
  Refresh();
}

void NeomorphicButton::onMouseLeave(wxMouseEvent&) {
  hover = false;
  pressed = false;
  Refresh();
}

void NeomorphicButton::onLeftDown(wxMouseEvent&) {
  if (enabled) {
    pressed = true;
    Refresh();
    CaptureMouse();
  }
}

void NeomorphicButton::onLeftUp(wxMouseEvent& ev) {
  if (HasCapture()) {
    ReleaseMouse();
  }
  if (enabled && pressed) {
    pressed = false;
    Refresh();

    // Check if still inside button
    wxSize size = GetClientSize();
    if (ev.GetX() >= 0 && ev.GetX() < size.x && ev.GetY() >= 0 && ev.GetY() < size.y) {
      // Send button click event
      wxCommandEvent event(wxEVT_BUTTON, GetId());
      event.SetEventObject(this);
      ProcessWindowEvent(event);
    }
  }
}

BEGIN_EVENT_TABLE(NeomorphicButton, wxWindow)
  EVT_PAINT(NeomorphicButton::onPaint)
  EVT_ENTER_WINDOW(NeomorphicButton::onMouseEnter)
  EVT_LEAVE_WINDOW(NeomorphicButton::onMouseLeave)
  EVT_LEFT_DOWN(NeomorphicButton::onLeftDown)
  EVT_LEFT_UP(NeomorphicButton::onLeftUp)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------- : StyledSearchBox

/// A custom styled search box with rounded corners and subtle focus
class StyledSearchBox : public wxPanel {
public:
  StyledSearchBox(wxWindow* parent, wxWindowID id);

  String GetValue() const { return textCtrl->GetValue(); }
  void Clear() { textCtrl->Clear(); }
  void SetChangeCallback(std::function<void(const String&)> callback) { onChange = callback; }

private:
  wxTextCtrl* textCtrl;
  bool focused = false;
  std::function<void(const String&)> onChange;

  void onPaint(wxPaintEvent& ev);
  void onText(wxCommandEvent& ev);
  void onFocus(wxFocusEvent& ev);
  void onKillFocus(wxFocusEvent& ev);
  void onSize(wxSizeEvent& ev);

  DECLARE_EVENT_TABLE();
};

StyledSearchBox::StyledSearchBox(wxWindow* parent, wxWindowID id)
  : wxPanel(parent, id, wxDefaultPosition, wxSize(280, 44), wxCLIP_CHILDREN)
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetMinSize(wxSize(280, 44));

  // Create the actual text control
  // Position: 16px from left edge, centered vertically
  // For 44px height with 4px padding (inner 36px), center ~22px text = y ~11
  textCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString,
                            wxPoint(16, 11), wxSize(248, 22),
                            wxTE_PROCESS_ENTER | wxBORDER_NONE | wxTE_NO_VSCROLL);
  textCtrl->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _("Beleren")));
  textCtrl->SetBackgroundColour(wxColour(248, 248, 250));
  textCtrl->SetForegroundColour(wxColour(35, 40, 50));

  // Disable macOS focus ring
#ifdef __WXOSX__
  void* handle = textCtrl->GetHandle();
  if (handle) {
    // NSFocusRingTypeNone = 1
    void* sel = sel_registerName("setFocusRingType:");
    ((void (*)(void*, void*, long))objc_msgSend)(handle, sel, 1);
  }
#endif

  // Bind events
  textCtrl->Bind(wxEVT_TEXT, &StyledSearchBox::onText, this);
  textCtrl->Bind(wxEVT_SET_FOCUS, &StyledSearchBox::onFocus, this);
  textCtrl->Bind(wxEVT_KILL_FOCUS, &StyledSearchBox::onKillFocus, this);
}

void StyledSearchBox::onPaint(wxPaintEvent&) {
  wxBufferedPaintDC dc(this);
  wxSize size = GetClientSize();

  // Clear with parent background
  dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.DrawRectangle(0, 0, size.x, size.y);

  // Draw rounded rectangle background with padding from edges
  // Leave 4px padding on all sides so the focus ring fits inside
  wxColour bgColor(248, 248, 250);
  wxColour borderColor = focused ? wxColour(100, 140, 190) : wxColour(210, 210, 215);

  dc.SetBrush(wxBrush(bgColor));
  dc.SetPen(wxPen(borderColor, focused ? 2 : 1));
  dc.DrawRoundedRectangle(4, 4, size.x - 8, size.y - 8, 8);
}

void StyledSearchBox::onText(wxCommandEvent&) {
  if (onChange) {
    onChange(textCtrl->GetValue());
  }
}

void StyledSearchBox::onFocus(wxFocusEvent& ev) {
  focused = true;
  Refresh();
  ev.Skip();
}

void StyledSearchBox::onKillFocus(wxFocusEvent& ev) {
  focused = false;
  Refresh();
  ev.Skip();
}

void StyledSearchBox::onSize(wxSizeEvent& ev) {
  wxSize size = GetClientSize();
  if (textCtrl) {
    // Position text control with margins and centered vertically
    // Left margin: 16px, right margin: 16px
    // Vertical: center a 22px control in the box
    int textHeight = 22;
    int y = (size.y - textHeight) / 2;
    textCtrl->SetSize(16, y, size.x - 32, textHeight);
  }
  ev.Skip();
}

BEGIN_EVENT_TABLE(StyledSearchBox, wxPanel)
  EVT_PAINT(StyledSearchBox::onPaint)
  EVT_SIZE(StyledSearchBox::onSize)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------- : StyledPackageGrid

/// A custom styled grid for displaying packages with neomorphic design
class StyledPackageGrid : public wxScrolledWindow {
public:
  StyledPackageGrid(Window* parent, int id);

  void showData(const String& pattern);
  void filter(const String& searchText);
  void clear();

  bool hasSelection() const { return selection < filteredIndices.size(); }
  size_t getSelectionId() const { return selection; }

  template <typename T>
  intrusive_ptr<T> getSelection(bool load_fully = true) const {
    if (!hasSelection()) return nullptr;
    intrusive_ptr<T> ret = dynamic_pointer_cast<T>(allPackages[filteredIndices[selection]].package);
    if (ret && load_fully) ret->loadFully();
    return ret;
  }

  void select(const String& name, bool send_event = true);
  void setColumnCount(size_t cols) { columnCount = cols; updateLayout(); }
  void scrollToTop() { Scroll(0, 0); }

private:
  struct PackageData {
    PackagedP package;
    Bitmap image;
    bool imageLoaded = false;  // For lazy loading
  };

  vector<PackageData> allPackages;
  vector<size_t> filteredIndices;  // Indices into allPackages for filtered view
  String currentSearchText;
  size_t selection = (size_t)-1;
  size_t columnCount = 4;

  // Item dimensions
  static const int ITEM_WIDTH = 140;
  static const int ITEM_HEIGHT = 170;
  static const int ITEM_SPACING = 16;
  static const int CARD_PADDING = 8;
  static const int CARD_RADIUS = 8;

  wxFont nameFont;
  wxFont descFont;

  void updateLayout();
  void onPaint(wxPaintEvent& ev);
  void onLeftDown(wxMouseEvent& ev);
  void onLeftDClick(wxMouseEvent& ev);
  void onSize(wxSizeEvent& ev);

  size_t findItemAt(int x, int y) const;
  wxRect getItemRect(size_t index) const;
  void loadImageForItem(PackageData& item);  // Lazy image loading

  DECLARE_EVENT_TABLE();
};

StyledPackageGrid::StyledPackageGrid(Window* parent, int id)
  : wxScrolledWindow(parent, id, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxFULL_REPAINT_ON_RESIZE)
  , nameFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, _("Beleren"))
  , descFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _("Beleren"))
{
  SetBackgroundStyle(wxBG_STYLE_PAINT);
  SetBackgroundColour(wxColour(250, 250, 252));
  SetScrollRate(0, 20);
}

void StyledPackageGrid::showData(const String& pattern) {
  allPackages.clear();
  filteredIndices.clear();
  selection = (size_t)-1;
  currentSearchText.clear();

  vector<PackagedP> matching;
  package_manager.findMatching(pattern, matching);

  // Just populate the list - images will be loaded lazily on paint
  for (const auto& p : matching) {
    PackageData data;
    data.package = p;
    data.imageLoaded = false;
    allPackages.push_back(data);
  }

  // Sort by position hint
  std::sort(allPackages.begin(), allPackages.end(), [](const PackageData& a, const PackageData& b) {
    if (a.package->position_hint != b.package->position_hint)
      return a.package->position_hint < b.package->position_hint;
    return a.package->name() < b.package->name();
  });

  // Initialize filtered indices to show all items
  for (size_t i = 0; i < allPackages.size(); ++i) {
    filteredIndices.push_back(i);
  }
  updateLayout();
  Refresh();
}

void StyledPackageGrid::loadImageForItem(PackageData& item) {
  if (item.imageLoaded) return;
  item.imageLoaded = true;

  auto stream = item.package->openIconFile();
  Image img;
  if (stream && image_load_file(img, *stream)) {
    item.image = Bitmap(img);
  }
}

void StyledPackageGrid::filter(const String& searchText) {
  currentSearchText = searchText.Lower();
  filteredIndices.clear();

  if (currentSearchText.empty()) {
    // Show all items
    for (size_t i = 0; i < allPackages.size(); ++i) {
      filteredIndices.push_back(i);
    }
  } else {
    for (size_t i = 0; i < allPackages.size(); ++i) {
      const auto& pkg = allPackages[i];
      String name = pkg.package->short_name.Lower();
      String fullName = pkg.package->full_name.Lower();
      if (name.find(currentSearchText) != String::npos ||
          fullName.find(currentSearchText) != String::npos) {
        filteredIndices.push_back(i);
      }
    }
  }

  // Reset selection if it's no longer valid
  if (selection >= filteredIndices.size()) {
    selection = filteredIndices.empty() ? (size_t)-1 : 0;
  }

  updateLayout();
  Refresh();

  // Send selection changed event
  wxCommandEvent evt(EVENT_GALLERY_SELECT, GetId());
  ProcessEvent(evt);
}

void StyledPackageGrid::clear() {
  allPackages.clear();
  filteredIndices.clear();
  selection = (size_t)-1;
  updateLayout();
  Refresh();
}

void StyledPackageGrid::select(const String& name, bool send_event) {
  for (size_t i = 0; i < filteredIndices.size(); ++i) {
    if (allPackages[filteredIndices[i]].package->name() == name) {
      selection = i;
      updateLayout();
      Refresh();

      // Scroll to selection
      wxRect rect = getItemRect(selection);
      int scrollY;
      GetViewStart(nullptr, &scrollY);
      scrollY *= 20; // scroll rate

      if (rect.y < scrollY) {
        Scroll(0, rect.y / 20);
      } else if (rect.y + rect.height > scrollY + GetClientSize().y) {
        Scroll(0, (rect.y + rect.height - GetClientSize().y) / 20 + 1);
      }

      if (send_event) {
        wxCommandEvent evt(EVENT_GALLERY_SELECT, GetId());
        ProcessEvent(evt);
      }
      return;
    }
  }
  selection = (size_t)-1;
}

void StyledPackageGrid::updateLayout() {
  if (filteredIndices.empty()) {
    SetVirtualSize(GetClientSize().x, 100);
    return;
  }

  int cols = columnCount;
  int rows = (filteredIndices.size() + cols - 1) / cols;
  int totalHeight = rows * (ITEM_HEIGHT + ITEM_SPACING) + ITEM_SPACING;

  SetVirtualSize(GetClientSize().x, totalHeight);
}

wxRect StyledPackageGrid::getItemRect(size_t index) const {
  if (index >= filteredIndices.size()) return wxRect();

  int cols = columnCount;
  int row = index / cols;
  int col = index % cols;

  int totalWidth = GetClientSize().x;
  int gridWidth = cols * ITEM_WIDTH + (cols - 1) * ITEM_SPACING;
  int startX = (totalWidth - gridWidth) / 2;

  int x = startX + col * (ITEM_WIDTH + ITEM_SPACING);
  int y = ITEM_SPACING + row * (ITEM_HEIGHT + ITEM_SPACING);

  return wxRect(x, y, ITEM_WIDTH, ITEM_HEIGHT);
}

size_t StyledPackageGrid::findItemAt(int x, int y) const {
  // Account for scroll position
  int scrollY;
  GetViewStart(nullptr, &scrollY);

  int cols = columnCount;
  int totalWidth = GetClientSize().x;
  int gridWidth = cols * ITEM_WIDTH + (cols - 1) * ITEM_SPACING;
  int startX = (totalWidth - gridWidth) / 2;

  int scrolledY = y + scrollY * 20;

  for (size_t i = 0; i < filteredIndices.size(); ++i) {
    int row = i / cols;
    int col = i % cols;

    int itemX = startX + col * (ITEM_WIDTH + ITEM_SPACING);
    int itemY = ITEM_SPACING + row * (ITEM_HEIGHT + ITEM_SPACING);

    if (x >= itemX && x < itemX + ITEM_WIDTH &&
        scrolledY >= itemY && scrolledY < itemY + ITEM_HEIGHT) {
      return i;
    }
  }

  return (size_t)-1;
}

void StyledPackageGrid::onPaint(wxPaintEvent&) {
  wxAutoBufferedPaintDC dc(this);
  DoPrepareDC(dc);

  // Clear background
  wxSize size = GetVirtualSize();
  dc.SetBrush(wxBrush(wxColour(250, 250, 252)));
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.DrawRectangle(0, 0, size.x, size.y);

  // Get visible area
  int scrollY;
  GetViewStart(nullptr, &scrollY);
  scrollY *= 20;
  int clientHeight = GetClientSize().y;

  // Draw items
  for (size_t i = 0; i < filteredIndices.size(); ++i) {
    wxRect rect = getItemRect(i);

    // Skip if not visible
    if (rect.y + rect.height < scrollY || rect.y > scrollY + clientHeight) {
      continue;
    }

    // Lazy load the image for this visible item
    PackageData& pkg = allPackages[filteredIndices[i]];
    if (!pkg.imageLoaded) {
      loadImageForItem(pkg);
    }

    bool isSelected = (i == selection);

    // Draw card background (neomorphic style)
    if (isSelected) {
      // Selected: subtle blue tint with stronger border
      dc.SetBrush(wxBrush(wxColour(240, 245, 255)));
      dc.SetPen(wxPen(wxColour(100, 140, 200), 2));
    } else {
      // Normal: white with light border
      dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
      dc.SetPen(wxPen(wxColour(220, 220, 225), 1));
    }
    dc.DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, CARD_RADIUS);

    // Draw image centered
    if (pkg.image.Ok()) {
      int imgX = rect.x + (rect.width - pkg.image.GetWidth()) / 2;
      int imgY = rect.y + CARD_PADDING;
      dc.DrawBitmap(pkg.image, imgX, imgY, true);
    }

    // Draw short name
    dc.SetFont(nameFont);
    dc.SetTextForeground(wxColour(35, 40, 50));
    String shortName = capitalize(pkg.package->short_name);
    wxSize textSize = dc.GetTextExtent(shortName);
    int textX = rect.x + (rect.width - textSize.x) / 2;
    int textY = rect.y + rect.height - 45;
    dc.DrawText(shortName, textX, textY);

    // Draw full name (smaller)
    dc.SetFont(descFont);
    dc.SetTextForeground(wxColour(100, 105, 115));
    String fullName = pkg.package->full_name;

    // Truncate if too long
    textSize = dc.GetTextExtent(fullName);
    if (textSize.x > rect.width - 2 * CARD_PADDING) {
      while (textSize.x > rect.width - 2 * CARD_PADDING - 10 && fullName.length() > 3) {
        fullName = fullName.substr(0, fullName.length() - 1);
        textSize = dc.GetTextExtent(fullName + _("..."));
      }
      fullName += _("...");
      textSize = dc.GetTextExtent(fullName);
    }

    textX = rect.x + (rect.width - textSize.x) / 2;
    textY = rect.y + rect.height - 25;
    dc.DrawText(fullName, textX, textY);
  }
}

void StyledPackageGrid::onLeftDown(wxMouseEvent& ev) {
  size_t item = findItemAt(ev.GetX(), ev.GetY());
  if (item < filteredIndices.size()) {
    selection = item;
    Refresh();

    wxCommandEvent evt(EVENT_GALLERY_SELECT, GetId());
    ProcessEvent(evt);
  }
}

void StyledPackageGrid::onLeftDClick(wxMouseEvent& ev) {
  size_t item = findItemAt(ev.GetX(), ev.GetY());
  if (item < filteredIndices.size()) {
    selection = item;
    Refresh();

    wxCommandEvent evt(EVENT_GALLERY_ACTIVATE, GetId());
    ProcessEvent(evt);
  }
}

void StyledPackageGrid::onSize(wxSizeEvent& ev) {
  updateLayout();
  Refresh();
  ev.Skip();
}

BEGIN_EVENT_TABLE(StyledPackageGrid, wxScrolledWindow)
  EVT_PAINT(StyledPackageGrid::onPaint)
  EVT_LEFT_DOWN(StyledPackageGrid::onLeftDown)
  EVT_LEFT_DCLICK(StyledPackageGrid::onLeftDClick)
  EVT_SIZE(StyledPackageGrid::onSize)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------- : SearchablePackageList

SearchablePackageList::SearchablePackageList(Window* parent, int id, int direction, bool showSearch)
  : wxPanel(parent, wxID_ANY)
  , list(nullptr)
  , searchBox(nullptr)
{
  SetBackgroundColour(wxColour(255, 255, 255));

  wxSizer* sizer = new wxBoxSizer(wxVERTICAL);

  // Search box with proper styling
  if (showSearch) {
    wxPanel* searchPanel = new wxPanel(this, wxID_ANY);
    searchPanel->SetBackgroundColour(wxColour(255, 255, 255));
    wxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);

    wxStaticText* searchLabel = new wxStaticText(searchPanel, wxID_ANY, _("Search:"));
    searchLabel->SetFont(wxFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _("Beleren")));
    searchLabel->SetForegroundColour(wxColour(60, 65, 75));

    styledSearchBox = new StyledSearchBox(searchPanel, id + 1000);
    styledSearchBox->SetChangeCallback([this](const String& text) {
      filterBySearch(text);
    });

    searchSizer->Add(searchLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    searchSizer->Add(styledSearchBox, 0, wxALIGN_CENTER_VERTICAL);
    searchPanel->SetSizer(searchSizer);

    sizer->Add(searchPanel, 0, wxEXPAND | wxBOTTOM, 16);
  } else {
    styledSearchBox = nullptr;
  }

  // Custom styled grid
  styledGrid = new StyledPackageGrid(this, id);
  sizer->Add(styledGrid, 1, wxEXPAND);

  SetSizer(sizer);
}

void SearchablePackageList::showDataInternal(const String& pattern) {
  currentPattern = pattern;
  searchText.clear();
  if (styledSearchBox) styledSearchBox->Clear();
  styledGrid->showData(pattern);
}

void SearchablePackageList::filterBySearch(const String& text) {
  searchText = text;
  styledGrid->filter(text);
}

void SearchablePackageList::onSearchText(wxCommandEvent& ev) {
  // Not used anymore - using callback instead
}

void SearchablePackageList::clear() {
  styledGrid->clear();
}

void SearchablePackageList::select(const String& name, bool send_event) {
  styledGrid->select(name, send_event);
}

bool SearchablePackageList::hasSelection() const {
  return styledGrid->hasSelection();
}

void SearchablePackageList::setColumnCount(size_t cols) {
  styledGrid->setColumnCount(cols);
}

void SearchablePackageList::scrollToTop() {
  styledGrid->scrollToTop();
}

BEGIN_EVENT_TABLE(SearchablePackageList, wxPanel)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------- : NewSetWizard

SetP new_set_wizard(Window* parent) {
  NewSetWizard wnd(parent);
  wnd.ShowModal();
  return wnd.set;
}

NewSetWizard::NewSetWizard(Window* parent)
  : wxDialog(parent, wxID_ANY, _TITLE_("new set"), wxDefaultPosition, wxSize(700, 600),
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
  , currentStep(STEP_SELECT_GAME)
  , titleFont(22, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, _("Beleren"))
  , subtitleFont(13, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _("Beleren"))
  , labelFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _("Beleren"))
{
  wxBusyCursor wait;

  SetBackgroundColour(wxColour(255, 255, 255));

  // Main sizer
  wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

  // Create both step panels
  createGameStep();
  createStyleStep();

  mainSizer->Add(gamePanel, 1, wxEXPAND | wxALL, 24);
  mainSizer->Add(stylePanel, 1, wxEXPAND | wxALL, 24);

  SetSizer(mainSizer);

  // Show initial step
  showStep(STEP_SELECT_GAME);

  // Load game list
  gameList->showData<Game>();
  try {
    gameList->select(settings.default_game);
  } catch (FileNotFoundError const& e) {
    handle_error(e);
  }

  CentreOnScreen();
  UpdateWindowUI(wxUPDATE_UI_RECURSE);
}

void NewSetWizard::createGameStep() {
  gamePanel = new wxPanel(this, wxID_ANY);
  gamePanel->SetBackgroundColour(wxColour(255, 255, 255));

  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

  // Title
  gameTitleLabel = new wxStaticText(gamePanel, wxID_ANY, _("Select Game Type"));
  gameTitleLabel->SetFont(titleFont);
  gameTitleLabel->SetForegroundColour(wxColour(35, 40, 50));

  // Subtitle
  gameSubtitleLabel = new wxStaticText(gamePanel, wxID_ANY,
    _("Choose the game system for your new set"));
  gameSubtitleLabel->SetFont(subtitleFont);
  gameSubtitleLabel->SetForegroundColour(wxColour(100, 105, 115));

  // Package list with search
  gameList = new SearchablePackageList(gamePanel, ID_WIZARD_GAME_LIST, wxVERTICAL, true);
  gameList->setColumnCount(4);

  // Next button panel
  wxPanel* buttonPanel = new wxPanel(gamePanel, wxID_ANY);
  buttonPanel->SetBackgroundColour(wxColour(255, 255, 255));
  wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

  gameNextButton = new NeomorphicButton(buttonPanel, ID_WIZARD_GAME_NEXT, _("Next"), true, wxSize(140, 40));

  buttonSizer->AddStretchSpacer();
  buttonSizer->Add(gameNextButton, 0, wxALIGN_CENTER_VERTICAL);
  buttonPanel->SetSizer(buttonSizer);

  // Layout
  sizer->Add(gameTitleLabel, 0, wxBOTTOM, 8);
  sizer->Add(gameSubtitleLabel, 0, wxBOTTOM, 20);
  sizer->Add(gameList, 1, wxEXPAND | wxBOTTOM, 20);
  sizer->Add(buttonPanel, 0, wxEXPAND);

  gamePanel->SetSizer(sizer);
}

void NewSetWizard::createStyleStep() {
  stylePanel = new wxPanel(this, wxID_ANY);
  stylePanel->SetBackgroundColour(wxColour(255, 255, 255));

  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

  // Title
  styleTitleLabel = new wxStaticText(stylePanel, wxID_ANY, _("Select Default Card Style"));
  styleTitleLabel->SetFont(titleFont);
  styleTitleLabel->SetForegroundColour(wxColour(35, 40, 50));

  // Subtitle
  styleSubtitleLabel = new wxStaticText(stylePanel, wxID_ANY,
    _("Choose the default appearance for new cards"));
  styleSubtitleLabel->SetFont(subtitleFont);
  styleSubtitleLabel->SetForegroundColour(wxColour(100, 105, 115));

  // Description
  styleDescLabel = new wxStaticText(stylePanel, wxID_ANY,
    _("This will be the default style when you create new cards in your set. You can change individual card styles later."));
  styleDescLabel->SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_NORMAL, false, _("Beleren")));
  styleDescLabel->SetForegroundColour(wxColour(120, 125, 135));
  styleDescLabel->Wrap(620);

  // Package list with search
  styleList = new SearchablePackageList(stylePanel, ID_WIZARD_STYLE_LIST, wxVERTICAL, true);
  styleList->setColumnCount(4);

  // Buttons panel
  wxPanel* buttonPanel = new wxPanel(stylePanel, wxID_ANY);
  buttonPanel->SetBackgroundColour(wxColour(255, 255, 255));
  wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

  styleBackButton = new NeomorphicButton(buttonPanel, ID_WIZARD_STYLE_BACK, _("Back"), false, wxSize(100, 40));
  styleCreateButton = new NeomorphicButton(buttonPanel, ID_WIZARD_STYLE_CREATE, _("Create Set"), true, wxSize(140, 40));

  buttonSizer->Add(styleBackButton, 0, wxALIGN_CENTER_VERTICAL);
  buttonSizer->AddStretchSpacer();
  buttonSizer->Add(styleCreateButton, 0, wxALIGN_CENTER_VERTICAL);
  buttonPanel->SetSizer(buttonSizer);

  // Layout
  sizer->Add(styleTitleLabel, 0, wxBOTTOM, 8);
  sizer->Add(styleSubtitleLabel, 0, wxBOTTOM, 8);
  sizer->Add(styleDescLabel, 0, wxBOTTOM, 20);
  sizer->Add(styleList, 1, wxEXPAND | wxBOTTOM, 20);
  sizer->Add(buttonPanel, 0, wxEXPAND);

  stylePanel->SetSizer(sizer);
}

void NewSetWizard::showStep(WizardStep step) {
  currentStep = step;

  gamePanel->Show(step == STEP_SELECT_GAME);
  stylePanel->Show(step == STEP_SELECT_STYLE);

  Layout();
  updateButtonStates();
}

void NewSetWizard::updateButtonStates() {
  if (currentStep == STEP_SELECT_GAME) {
    gameNextButton->Enable(gameList->hasSelection());
  } else {
    styleCreateButton->Enable(styleList->hasSelection());
  }
}

void NewSetWizard::onGameSelect(wxCommandEvent&) {
  updateButtonStates();
}

void NewSetWizard::onGameActivate(wxCommandEvent& ev) {
  if (gameList->hasSelection()) {
    onGameNext(ev);
  }
}

void NewSetWizard::onGameNext(wxCommandEvent&) {
  if (!gameList->hasSelection()) return;

  wxBusyCursor wait;

  // Get selected game
  selectedGame = gameList->getGrid()->getSelection<Game>(false);
  settings.default_game = selectedGame->name();

  // Load stylesheets for this game
  styleList->showData<StyleSheet>(selectedGame->name() + _("-*"));
  styleList->select(settings.gameSettingsFor(*selectedGame).default_stylesheet);
  styleList->scrollToTop();

  // Switch to style step
  showStep(STEP_SELECT_STYLE);
}

void NewSetWizard::onStyleSelect(wxCommandEvent&) {
  if (styleList->hasSelection() && selectedGame) {
    StyleSheetP stylesheet = styleList->getGrid()->getSelection<StyleSheet>(false);
    if (stylesheet) {
      settings.gameSettingsFor(*selectedGame).default_stylesheet = stylesheet->name();
    }
  }
  updateButtonStates();
}

void NewSetWizard::onStyleActivate(wxCommandEvent&) {
  if (styleList->hasSelection()) {
    done();
  }
}

void NewSetWizard::onStyleBack(wxCommandEvent&) {
  showStep(STEP_SELECT_GAME);
}

void NewSetWizard::onStyleCreate(wxCommandEvent&) {
  done();
}

void NewSetWizard::done() {
  try {
    if (!styleList->hasSelection()) return;
    StyleSheetP stylesheet = styleList->getGrid()->getSelection<StyleSheet>();
    set = make_intrusive<Set>(stylesheet);
    set->validate();
    EndModal(wxID_OK);
  } catch (const Error& e) {
    handle_error_now(e);
  }
}

void NewSetWizard::onUpdateUI(wxUpdateUIEvent& ev) {
  switch (ev.GetId()) {
    case ID_WIZARD_GAME_NEXT:
      ev.Enable(gameList->hasSelection());
      break;
    case ID_WIZARD_STYLE_CREATE:
      ev.Enable(styleList->hasSelection());
      break;
  }
}

void NewSetWizard::onIdle(wxIdleEvent& ev) {
  // Handle any pending operations
}

BEGIN_EVENT_TABLE(NewSetWizard, wxDialog)
  EVT_GALLERY_SELECT  (ID_WIZARD_GAME_LIST,   NewSetWizard::onGameSelect)
  EVT_GALLERY_ACTIVATE(ID_WIZARD_GAME_LIST,   NewSetWizard::onGameActivate)
  EVT_BUTTON          (ID_WIZARD_GAME_NEXT,   NewSetWizard::onGameNext)
  EVT_GALLERY_SELECT  (ID_WIZARD_STYLE_LIST,  NewSetWizard::onStyleSelect)
  EVT_GALLERY_ACTIVATE(ID_WIZARD_STYLE_LIST,  NewSetWizard::onStyleActivate)
  EVT_BUTTON          (ID_WIZARD_STYLE_BACK,  NewSetWizard::onStyleBack)
  EVT_BUTTON          (ID_WIZARD_STYLE_CREATE, NewSetWizard::onStyleCreate)
  EVT_UPDATE_UI       (wxID_ANY,              NewSetWizard::onUpdateUI)
  EVT_IDLE            (                       NewSetWizard::onIdle)
END_EVENT_TABLE()

