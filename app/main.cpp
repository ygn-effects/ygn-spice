#include "config.hpp"

#include "core/include/module.hpp"
#include "library/include/module.hpp"
#include <ygn_spice/netlist/module.hpp>
#include <ygn_spice/simulation/module.hpp>
#include <ygn_spice/viewport/module.hpp>

#include <gtkmm.h>

#include <array>
#include <string>
#include <string_view>

namespace {

class MainWindow final : public Gtk::Window {
public:
  MainWindow() {
    set_title("YGN Spice");
    set_default_size(960, 640);

    const std::array modules{
      ygn::spice::core::module_name(),
      ygn::spice::library::module_name(),
      ygn::spice::netlist::module_name(),
      ygn::spice::simulation::module_name(),
      ygn::spice::viewport::module_name(),
    };

    std::string message = "YGN Spice foundation ready\n\nModules: ";
    for (std::size_t index = 0; index < modules.size(); ++index) {
      if (index != 0) {
        message += ", ";
      }
      message += modules[index];
    }

#if YGN_SPICE_HAVE_NGSPICE
    message += "\nngspice backend dependency: available";
#else
    message += "\nngspice backend dependency: not installed";
#endif

    label_.set_text(message);
    label_.set_justify(Gtk::Justification::CENTER);
    label_.set_margin(24);
    set_child(label_);
  }

private:
  Gtk::Label label_;
};

} // namespace

int main(int argc, char *argv[]) {
  const auto application = Gtk::Application::create();
  return application->make_window_and_run<MainWindow>(argc, argv);
}
