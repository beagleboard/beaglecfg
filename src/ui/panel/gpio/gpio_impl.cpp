#include <filesystem>
#include <fstream>
#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ui/panel/panel.hpp"

using namespace ftxui;

namespace ui {

const std::vector<std::wstring> iOentries = {
    L"IN",
    L"OUT",
};

const std::vector<std::wstring> valueEntries = {
    L"0",
    L"1",
};

const std::vector<std::wstring> activeEntries = {
    L"Low",
    L"High",
};

const std::vector<std::wstring> edgeEntries = {
    L"Pos (+ve)",
    L"Neg (-ve)",
    L"Any",
    L"None",
};

class Gpio : public ComponentBase {
 public:
  Gpio(std::string path, int* tab, int* next, int* limit) : path_(path) {
    limit_ = limit;
    tab_ = tab;
    next_ = next;
    Fetch();
    BuildUI();
  }

  std::string label() const { return label_; }
  std::string direction() const { return direction_; }
  std::string edge() const { return edge_; }
  std::string value() const { return value_; }
  std::string active_low() const { return active_low_; }

 private:
  void Fetch() {
    std::ifstream(path_ + "/label") >> label_;
    std::ifstream(path_ + "/edge") >> edge_;
    std::ifstream(path_ + "/direction") >> direction_;
    std::ifstream(path_ + "/value") >> value_;
    std::ifstream(path_ + "/active_low") >> active_low_;
  }

  void StoreDirection(std::string direction) {
    std::ofstream(path_ + "/direction") << direction;
    Fetch();
  };

  void StoreEdge(std::string edge) {
    std::ofstream(path_ + "/edge") << edge;
    Fetch();
  };

  void StoreValue(std::string value) {
    std::ofstream(path_ + "/value") << value;
    Fetch();
  };

  void StoreActiveLow(std::string active_low) {
    std::ofstream(path_ + "/active_low") << active_low;
    Fetch();
  };

  void toggleHandler(int i) {
    auto val = toggles[i];
    switch (i) {
      case 0:
        if (val == 0)
          StoreDirection("in");
        else
          StoreDirection("out");
        break;
      case 1:
        if (val == 0)
          StoreValue("0");
        else
          StoreValue("1");
        break;
      case 2:
        if (val == 0)
          StoreActiveLow("0");
        else
          StoreActiveLow("1");
        break;
      case 3:
        switch (val) {
          case 0:
            StoreEdge("rising");
            break;
          case 1:
            StoreEdge("falling");
            break;
          case 2:
            StoreEdge("both");
            break;
          case 3:
            StoreEdge("none");
            break;
        }
        break;
    }
  }

  void BuildUI() {
    ToggleOption iOtoggleOpt;
    ToggleOption valueToggleOpt;
    ToggleOption activeToggleOpt;
    ToggleOption edgeToggleOpt;
    iOtoggleOpt.on_enter = [&] { toggleHandler(0); };
    valueToggleOpt.on_change = [&] { toggleHandler(1); };
    activeToggleOpt.on_change = [&] { toggleHandler(2); };
    edgeToggleOpt.on_change = [&] { toggleHandler(3); };
    toggleComponents.push_back(Toggle(&iOentries, &toggles[0], iOtoggleOpt));
    toggleComponents.push_back(
        Toggle(&valueEntries, &toggles[1], valueToggleOpt));
    toggleComponents.push_back(
        Toggle(&activeEntries, &toggles[2], activeToggleOpt));
    toggleComponents.push_back(
        Toggle(&edgeEntries, &toggles[3], edgeToggleOpt));
    Component actions = Renderer(Container::Vertical(toggleComponents), [&] {
      return vbox({
          text(to_wstring(label()) + L" Status "),
          hbox(text(L" * Direction       : "), text(to_wstring(direction()))),
          hbox(text(L" * Value           : "), text(to_wstring(value()))),
          hbox(text(L" * Active Low      : "), text(to_wstring(active_low()))),
          hbox(text(L" * Edge            : "), text(to_wstring(edge()))),
          text(L" Actions "),
          hbox(text(L" * Direction       : "), toggleComponents[0]->Render()),
          hbox(text(L" * Value           : "), toggleComponents[1]->Render()),
          hbox(text(L" * Active Low      : "), toggleComponents[2]->Render()),
          hbox(text(L" * Edge            : "), toggleComponents[3]->Render()),
      });
    });
    Add(Container::Vertical(
        {actions, Container::Horizontal({
                      Button(L"Back to Menu", [&] { *tab_ = 0; }),
                      Button(L"Prev",
                             [&] {
                               (*next_)--;
                               if (*next_ < 0) {
                                 *next_ = 0;
                               }
                             }),
                      Button(L"Next",
                             [&] {
                               (*next_)++;
                               if (*limit_ < *next_) {
                                 *next_ = 0;
                               }
                             }),
                  })}));
  }

  std::string path_;
  std::string label_;
  std::string direction_;
  std::string edge_;
  std::string value_;
  std::string active_low_;
  int* tab_;
  int* next_;
  int* limit_;
  int toggles[4];
  Components toggleComponents;
};

class GPIOImpl : public PanelBase {
 public:
  GPIOImpl() { BuildUI(); }
  std::wstring Title() override { return L"GPIO"; }

 private:
  void BuildUI() {
    MenuOption menuOpt;
    menuOpt.on_enter = [&] { tab = 1; };
    gpio_menu = Menu(&gpio_names, &selected, menuOpt);
    gpio_individual = Container::Vertical({}, &selected);
    for (const auto& it :
         std::filesystem::directory_iterator("/sys/class/gpio/")) {
      std::string gpio_path = it.path();
      if (gpio_path.find("gpiochip") == std::string::npos &&
          gpio_path.find("gpio") != std::string::npos) {
        auto gpio = std::make_shared<Gpio>(gpio_path, &tab, &selected, &limit);
        if (gpio->label().find("P") != std::string::npos) {
          children_.push_back(gpio);
          gpio_individual->Add(gpio);
          limit++;
        }
      }
    }

    Add(Container::Tab(
        {
            gpio_menu,
            gpio_individual,
        },
        &tab));
  }

  Element Render() override {
    gpio_names.clear();
    for (const auto& child : children_) {
      gpio_names.push_back(to_wstring(child->label()));
    }

    int i = 0;
    if (tab == 1)
      for (const auto& child : children_) {
        if (i == selected) {
          return child->Render();
        }
        i++;
      }

    return window(text(L"GPIO Menu"), gpio_menu->Render() | frame | flex);
  }

  std::vector<std::shared_ptr<Gpio>> children_;
  std::vector<std::wstring> gpio_names;
  Component gpio_menu;
  Component gpio_individual;
  int selected = 0;
  int tab = 0;
  int limit = 0;
};

namespace panel {
Panel GPIO() {
  return Make<GPIOImpl>();
}
}  // namespace panel
}  // namespace ui
