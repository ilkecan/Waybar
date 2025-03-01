#include "AButton.hpp"

#include <fmt/format.h>

#include <util/command.hpp>

namespace waybar {

AButton::AButton(const Json::Value& config, const std::string& name, const std::string& id,
                 const std::string& format, uint16_t interval, bool ellipsize, bool enable_click,
                 bool enable_scroll)
    : AModule(config, name, id, config["format-alt"].isString() || enable_click, enable_scroll),
      format_(config_["format"].isString() ? config_["format"].asString() : format),
      interval_(config_["interval"] == "once"
                    ? std::chrono::seconds(100000000)
                    : std::chrono::seconds(
                          config_["interval"].isUInt() ? config_["interval"].asUInt() : interval)),
      default_format_(format_) {
  button_.set_name(name);
  button_.set_relief(Gtk::RELIEF_NONE);

  /* https://github.com/Alexays/Waybar/issues/1731 */
  auto css = Gtk::CssProvider::create();
  css->load_from_data("button { min-width: 0; }");
  button_.get_style_context()->add_provider(css, GTK_STYLE_PROVIDER_PRIORITY_USER);

  if (!id.empty()) {
    button_.get_style_context()->add_class(id);
  }
  event_box_.add(button_);
  if (config_["max-length"].isUInt()) {
    label_->set_max_width_chars(config_["max-length"].asInt());
    label_->set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
    label_->set_single_line_mode(true);
  } else if (ellipsize && label_->get_max_width_chars() == -1) {
    label_->set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
    label_->set_single_line_mode(true);
  }

  if (config_["min-length"].isUInt()) {
    label_->set_width_chars(config_["min-length"].asUInt());
  }

  uint rotate = 0;

  if (config_["rotate"].isUInt()) {
    rotate = config["rotate"].asUInt();
    label_->set_angle(rotate);
  }

  if (config_["align"].isDouble()) {
    auto align = config_["align"].asFloat();
    if (rotate == 90 || rotate == 270) {
      label_->set_yalign(align);
    } else {
      label_->set_xalign(align);
    }
  }

  if (!(config_["on-click"].isString() || config_["on-click-middle"].isString() ||
        config_["on-click-backward"].isString() || config_["on-click-forward"].isString() ||
        config_["on-click-right"].isString() || config_["format-alt"].isString() || enable_click)) {
    button_.set_sensitive(false);
  } else {
    button_.signal_pressed().connect([this] {
      GdkEventButton* e = (GdkEventButton*)gdk_event_new(GDK_BUTTON_PRESS);
      e->button = 1;
      handleToggle(e);
    });
  }
}

auto AButton::update() -> void { AModule::update(); }

std::string AButton::getIcon(uint16_t percentage, const std::string& alt, uint16_t max) {
  auto format_icons = config_["format-icons"];
  if (format_icons.isObject()) {
    if (!alt.empty() && (format_icons[alt].isString() || format_icons[alt].isArray())) {
      format_icons = format_icons[alt];
    } else {
      format_icons = format_icons["default"];
    }
  }
  if (format_icons.isArray()) {
    auto size = format_icons.size();
    auto idx = std::clamp(percentage / ((max == 0 ? 100 : max) / size), 0U, size - 1);
    format_icons = format_icons[idx];
  }
  if (format_icons.isString()) {
    return format_icons.asString();
  }
  return "";
}

std::string AButton::getIcon(uint16_t percentage, const std::vector<std::string>& alts,
                             uint16_t max) {
  auto format_icons = config_["format-icons"];
  if (format_icons.isObject()) {
    std::string _alt = "default";
    for (const auto& alt : alts) {
      if (!alt.empty() && (format_icons[alt].isString() || format_icons[alt].isArray())) {
        _alt = alt;
        break;
      }
    }
    format_icons = format_icons[_alt];
  }
  if (format_icons.isArray()) {
    auto size = format_icons.size();
    auto idx = std::clamp(percentage / ((max == 0 ? 100 : max) / size), 0U, size - 1);
    format_icons = format_icons[idx];
  }
  if (format_icons.isString()) {
    return format_icons.asString();
  }
  return "";
}

bool waybar::AButton::handleToggle(GdkEventButton* const& e) {
  if (config_["format-alt-click"].isUInt() && e->button == config_["format-alt-click"].asUInt()) {
    alt_ = !alt_;
    if (alt_ && config_["format-alt"].isString()) {
      format_ = config_["format-alt"].asString();
    } else {
      format_ = default_format_;
    }
  }
  return AModule::handleToggle(e);
}

std::string AButton::getState(uint8_t value, bool lesser) {
  if (!config_["states"].isObject()) {
    return "";
  }
  // Get current state
  std::vector<std::pair<std::string, uint8_t>> states;
  if (config_["states"].isObject()) {
    for (auto it = config_["states"].begin(); it != config_["states"].end(); ++it) {
      if (it->isUInt() && it.key().isString()) {
        states.emplace_back(it.key().asString(), it->asUInt());
      }
    }
  }
  // Sort states
  std::sort(states.begin(), states.end(), [&lesser](auto& a, auto& b) {
    return lesser ? a.second < b.second : a.second > b.second;
  });
  std::string valid_state;
  for (auto const& state : states) {
    if ((lesser ? value <= state.second : value >= state.second) && valid_state.empty()) {
      button_.get_style_context()->add_class(state.first);
      valid_state = state.first;
    } else {
      button_.get_style_context()->remove_class(state.first);
    }
  }
  return valid_state;
}

}  // namespace waybar
