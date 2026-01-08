//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/welcome_window.hpp>
#include <gui/util.hpp>
#include <gui/new_window.hpp>
#include <gui/set/window.hpp>
#include <gui/update_checker.hpp>
#include <gui/packages_window.hpp>
#include <util/window_id.hpp>
#include <data/settings.hpp>
#include <data/format/formats.hpp>
#include <wx/dcbuffer.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>

// 2007-02-06: New HoverButton, hopefully this on works on GTK
#define USE_HOVERBUTTON

// ----------------------------------------------------------------------------- : WelcomeWindow

WelcomeWindow::WelcomeWindow()
  : wxFrame(nullptr, wxID_ANY, _TITLE_("magic set editor"), wxDefaultPosition, wxSize(540, 460), wxDEFAULT_DIALOG_STYLE | wxTAB_TRAVERSAL | wxCLIP_CHILDREN)
  , logo(load_resource_image(_("about")))
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

  // Scale icons to 48x48 for balanced look
  auto scaleIcon = [](const wxImage& img) -> wxImage {
    if (img.Ok() && (img.GetWidth() > 48 || img.GetHeight() > 48)) {
      return img.Scale(48, 48, wxIMAGE_QUALITY_HIGH);
    }
    return img;
  };

  // init controls with scaled icons
  wxControl* new_set   = new HoverButtonExt(this, ID_FILE_NEW,           scaleIcon(load_resource_image(_("welcome_new"))),     _BUTTON_("new set"),       _HELP_("new set"));
  wxControl* open_set  = new HoverButtonExt(this, ID_FILE_OPEN,          scaleIcon(load_resource_image(_("welcome_open"))),    _BUTTON_("open set"),      _HELP_("open set"));
  #if !USE_OLD_STYLE_UPDATE_CHECKER
  wxControl* updates   = new HoverButtonExt(this, ID_FILE_CHECK_UPDATES, scaleIcon(load_resource_image(_("welcome_updates"))), _BUTTON_("check updates"), _HELP_("check updates"));
  #endif
  wxControl* open_last = nullptr;
  if (!settings.recent_sets.empty()) {
    const String& filename = settings.recent_sets.front();
    if (wxFileName::FileExists(filename) || wxFileName::DirExists(filename + _("/"))) {
      wxFileName n(filename);
      open_last = new HoverButtonExt(this, ID_FILE_RECENT, scaleIcon(load_resource_image(_("welcome_last"))), _BUTTON_("last opened set"), _HELP_1_("last opened set", n.GetName()));
    }
  }

  // Modern centered layout
  wxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

  // Space for logo area + red line + padding (drawn in onPaint)
  int logoHeight = logo.Ok() ? logo.GetHeight() : 80;
  mainSizer->AddSpacer(logoHeight + 75); // extra space after red line

  // Centered button container
  wxSizer* buttonSizer = new wxBoxSizer(wxVERTICAL);
  buttonSizer->Add(new_set,  0, wxALIGN_CENTER | wxBOTTOM, 10);
  buttonSizer->Add(open_set, 0, wxALIGN_CENTER | wxBOTTOM, 10);
  #if !USE_OLD_STYLE_UPDATE_CHECKER
  buttonSizer->Add(updates,  0, wxALIGN_CENTER | wxBOTTOM, 10);
  #endif
  if (open_last) buttonSizer->Add(open_last, 0, wxALIGN_CENTER | wxBOTTOM, 10);

  mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER);
  mainSizer->AddStretchSpacer();

  SetSizer(mainSizer);
  CentreOnScreen();
}

void WelcomeWindow::onPaint(wxPaintEvent&) {
  wxBufferedPaintDC dc(this);
  draw(dc);
}

void WelcomeWindow::draw(DC& dc) {
  wxSize ws = GetClientSize();

  // Pure white background
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.SetBrush(Color(255, 255, 255));
  dc.DrawRectangle(0, 0, ws.GetWidth(), ws.GetHeight());

  // Red accent line under logo area
  int lineY = logo.Ok() ? logo.GetHeight() + 50 : 120;
  dc.SetPen(wxPen(Color(200, 60, 60), 2));
  dc.DrawLine(0, lineY, ws.GetWidth(), lineY);

  // Draw logo centered in header
  if (logo.Ok()) {
    int logoX = (ws.GetWidth() - logo.GetWidth()) / 2;
    int logoY = 25;
    dc.DrawBitmap(logo, logoX, logoY);
  }

  #if USE_BETA_LOGO
    dc.DrawBitmap(logo2, ws.GetWidth() - logo2.GetWidth(), ws.GetHeight() - logo2.GetHeight());
  #endif

  // Version number at bottom center - subtle
  dc.SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _("Beleren")));
  dc.SetTextForeground(Color(160, 165, 175));
  String version_string = _("Version ") + app_version.toString() + version_suffix;
  int tw, th;
  dc.GetTextExtent(version_string, &tw, &th);
  dc.DrawText(version_string, (ws.GetWidth() - tw) / 2, ws.GetHeight() - th - 14);
}

void WelcomeWindow::onOpenSet(wxCommandEvent&) {
  wxFileDialog* dlg = new wxFileDialog(this, _TITLE_("open set"), settings.default_set_dir, wxEmptyString, import_formats(), wxFD_OPEN);
  if (dlg->ShowModal() == wxID_OK) {
    settings.default_set_dir = dlg->GetDirectory();
    wxBusyCursor wait;
    try {
      close(import_set(dlg->GetPath()));
    } catch (Error& e) {
      handle_error(_("Error loading set: ") + e.what());
    }
  }
}

void WelcomeWindow::onNewSet(wxCommandEvent&) {
  close(new_set_window(this));
}

void WelcomeWindow::onOpenLast(wxCommandEvent&) {
  wxBusyCursor wait;
  assert(!settings.recent_sets.empty());
  try {
    close( open_package<Set>(settings.recent_sets.front()) );
  } catch (PackageNotFoundError& e) {
    handle_error(_("Cannot find set ") + e.what() + _(" to open."));
    // remove this package from the recent sets, so we don't get this error again
    settings.recent_sets.erase(settings.recent_sets.begin());
  }
}

void WelcomeWindow::onCheckUpdates(wxCommandEvent&) {
  Show(false); // hide, so the PackagesWindow will not use this window as its parent
  (new PackagesWindow(nullptr))->Show();
  Close();
}

void WelcomeWindow::close(const SetP& set) {
  if (!set) return;
  (new SetWindow(nullptr, set))->Show();
  Close();
}


BEGIN_EVENT_TABLE(WelcomeWindow, wxFrame)
  EVT_BUTTON         (ID_FILE_NEW,           WelcomeWindow::onNewSet)
  EVT_BUTTON         (ID_FILE_OPEN,          WelcomeWindow::onOpenSet)
  EVT_BUTTON         (ID_FILE_RECENT,        WelcomeWindow::onOpenLast)
  EVT_BUTTON         (ID_FILE_CHECK_UPDATES, WelcomeWindow::onCheckUpdates)
  EVT_PAINT          (                       WelcomeWindow::onPaint)
//  EVT_IDLE           (                       WelcomeWindow::onIdle)
END_EVENT_TABLE  ()


// ----------------------------------------------------------------------------- : Hover button with label

HoverButtonExt::HoverButtonExt(Window* parent, int id, const wxImage& icon, const String& label, const String& sub_label)
  : HoverButton(parent, id, _("btn"), Color(255, 255, 255))
  , icon(icon)
  , label(label), sub_label(sub_label)
  // Use Beleren for both labels
  , font_large(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, _("Beleren"))
  , font_small(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, _("Beleren"))
{
  // Wider buttons for better clickability
  SetMinSize(wxSize(380, 72));
}

void HoverButtonExt::draw(DC& dc) {
  wxSize ws = GetClientSize();
  int d = drawDelta();

  // Clear background
  dc.SetPen(*wxTRANSPARENT_PEN);
  dc.SetBrush(Color(255, 255, 255));
  dc.DrawRectangle(0, 0, ws.GetWidth(), ws.GetHeight());

  // Button colors - pure white with red border on hover
  Color bgColor, borderColor;
  bool isPressed = (mouse_down && hover) || key_down;

  if (isPressed) {
    bgColor = Color(255, 255, 255);
    borderColor = Color(180, 60, 60);
  } else if (hover) {
    bgColor = Color(255, 255, 255);
    borderColor = Color(200, 80, 80);
  } else {
    bgColor = Color(255, 255, 255);
    borderColor = Color(220, 222, 228);
  }

  // Subtle shadow when not pressed
  if (!isPressed) {
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(Color(0, 0, 0, 12));
    dc.DrawRoundedRectangle(2, 3, ws.GetWidth() - 4, ws.GetHeight() - 4, 8);
  }

  // Main button background
  dc.SetPen(wxPen(borderColor, 1));
  dc.SetBrush(wxBrush(bgColor));
  dc.DrawRoundedRectangle(1 + d, 1 + d, ws.GetWidth() - 2, ws.GetHeight() - 2, 8);


  // Draw icon (48x48, vertically centered)
  int iconSize = icon.Ok() ? icon.GetWidth() : 48;
  int iconX = 20 + d;
  int iconY = (ws.GetHeight() - iconSize) / 2 + d;
  if (icon.Ok()) dc.DrawBitmap(icon, iconX, iconY);

  // Text positioning
  int textX = iconX + iconSize + 16;
  int centerY = ws.GetHeight() / 2;

  // Main label
  dc.SetFont(font_large);
  dc.SetTextForeground(Color(35, 40, 50));
  int labelH = dc.GetTextExtent(label).GetHeight();
  dc.DrawText(label, textX, centerY - labelH - 1 + d);

  // Sub label
  dc.SetFont(font_small);
  dc.SetTextForeground(Color(120, 125, 140));
  dc.DrawText(sub_label, textX, centerY + 3 + d);
}
