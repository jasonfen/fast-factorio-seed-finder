#include <print>
#include <string>

#include <argparse/argparse.hpp>

#include "finder.hpp"
#include "presets.hpp"
#include "registry.hpp"

namespace {

const FinderEntry* find_finder(const std::string& name) {
    for (const FinderEntry& e : finder_registry()) {
        if (e.name == name) return &e;
    }
    return nullptr;
}

void print_catalog() {
    std::println("Available finders (--finder):");
    for (const FinderEntry& e : finder_registry()) {
        std::println("  {:<22}{}", e.name, e.description);
    }
    std::println("");
    std::println("Available modes (--mode):");
    for (const Preset& p : presets()) {
        std::println("  {:<22}{}", p.name, p.description);
    }
}

bool has_flag(int argc, char* argv[], const std::string& flag) {
    for (int i = 1; i < argc; i++) {
        if (flag == argv[i]) return true;
    }
    return false;
}

} // namespace

int main(int argc, char* argv[]) {
    // Handle --list before argparse so it works without the otherwise-required -o.
    if (has_flag(argc, argv, "--list")) {
        print_catalog();
        return 0;
    }

    argparse::ArgumentParser program("seed_finder");
    program.add_description("Unified Factorio seed finder. Pick a goal with --finder "
                            "and a map-settings preset with --mode.");

    program.add_argument("--finder")
        .help("Which finder/goal to run (see --list).")
        .metavar("NAME");
    program.add_argument("--mode")
        .help("Map-settings preset, e.g. normal or railworld (see --list).")
        .default_value(std::string("normal"))
        .metavar("NAME");
    program.add_argument("--list")
        .help("List available finders and modes, then exit.")
        .flag();

    add_standard_arguments(program);

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    auto finder_name = program.present<std::string>("--finder");
    if (!finder_name) {
        std::println("Error: --finder is required.\n");
        print_catalog();
        return 1;
    }

    const FinderEntry* entry = find_finder(*finder_name);
    if (!entry) {
        std::println("Error: unknown finder '{}'.\n", *finder_name);
        print_catalog();
        return 1;
    }

    auto mode = program.get<std::string>("--mode");
    auto settings = preset_settings(mode);
    if (!settings) {
        std::println("Error: unknown mode '{}'.\n", mode);
        print_catalog();
        return 1;
    }

    std::println("Finder: {}. Mode: {}.", *finder_name, mode);
    return entry->run(*settings, read_standard_arguments(program));
}
