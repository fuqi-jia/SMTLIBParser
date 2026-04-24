#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"

namespace fs = std::filesystem;

static void collect_smt2_flat(const fs::path& dir, std::vector<std::string>& out) {
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".smt2") {
            out.push_back(entry.path().string());
        }
    }
}

static void collect_smt2_recursive(const fs::path& dir, std::vector<std::string>& out) {
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".smt2") {
            out.push_back(entry.path().string());
        }
    }
}

/** First path component relative to root (e.g. QF_DT); single-level files -> "(root)". */
static std::string logic_bucket_for_file(const std::string& root_dir, const std::string& file_path) {
    std::string prefix = root_dir;
    if (!prefix.empty() && prefix.back() != '/' && prefix.back() != '\\') {
        prefix += '/';
    }
    if (file_path.compare(0, prefix.size(), prefix) != 0) {
        return "?";
    }
    std::string rel = file_path.substr(prefix.size());
    while (!rel.empty() && (rel[0] == '/' || rel[0] == '\\')) {
        rel.erase(0, 1);
    }
    if (rel.empty()) {
        return "(root)";
    }
    auto pos = rel.find_first_of("/\\");
    if (pos == std::string::npos) {
        return "(root)";
    }
    return rel.substr(0, pos);
}

static void print_usage(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " [-r|--recursive] [path]\n";
    std::cerr << "  path            Directory (non-recursive by default) or a single .smt2 file.\n";
    std::cerr << "  -r, --recursive Recursively find .smt2 under a directory.\n";
    std::cerr << "With no arguments, searches common locations for test/instances.\n";
}

int main(int argc, char* argv[]) {
    bool recursive = false;
    std::string instances_dir;
    int argi = 1;
    while (argi < argc) {
        std::string a = argv[argi];
        if (a == "-r" || a == "--recursive") {
            recursive = true;
            ++argi;
        } else if (a == "-h" || a == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            break;
        }
    }
    if (argi < argc - 1) {
        print_usage(argv[0]);
        return 1;
    }
    if (argi == argc - 1) {
        instances_dir = argv[argi];
    } else if (argc == 1) {
        if (fs::exists("test/instances")) {
            instances_dir = "test/instances";
        } else if (fs::exists("../test/instances")) {
            instances_dir = "../test/instances";
        } else if (fs::exists("../../test/instances")) {
            instances_dir = "../../test/instances";
        } else if (fs::exists("instances")) {
            instances_dir = "instances";
        } else if (fs::exists("./instances")) {
            instances_dir = "./instances";
        } else {
            std::cerr << "Error: Cannot find instances directory. Try running from project root or test directory.\n";
            std::cerr << "Or specify: " << argv[0] << " [-r] <instances_directory_or_file>\n";
            return 1;
        }
    } else {
        print_usage(argv[0]);
        return 1;
    }

    if (!fs::exists(instances_dir)) {
        std::cerr << "Error: " << instances_dir << " is not a valid path\n";
        return 1;
    }

    const bool is_directory = fs::is_directory(instances_dir);
    std::vector<std::string> smt2_files;
    if (is_directory) {
        if (recursive) {
            collect_smt2_recursive(instances_dir, smt2_files);
        } else {
            collect_smt2_flat(instances_dir, smt2_files);
        }
    } else {
        smt2_files.push_back(instances_dir);
    }

    std::sort(smt2_files.begin(), smt2_files.end());

    if (smt2_files.empty()) {
        std::cout << "No .smt2 files found in " << instances_dir << std::endl;
        return 0;
    }

    std::cout << "Found " << smt2_files.size() << " .smt2 files to test"
              << (recursive ? " (recursive)\n" : "\n");

    int successful_parses = 0;
    int failed_parses = 0;
    std::vector<std::string> failed_paths;

    for (const auto& input_file : smt2_files) {
        auto parser = std::make_shared<SOMTParser::Parser>();
        parser->setOption("keep_let", false);

        std::cout << "\n=== Testing file: " << input_file << " ===" << std::endl;

        auto start_time = std::chrono::high_resolution_clock::now();
        bool parse_success = parser->parse(input_file);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        if (parse_success) {
            std::cout << "PARSE_SUCCESS" << std::endl;
            successful_parses++;

            auto assertions = parser->getAssertions();
            auto variables = parser->getVariables();
            auto functions = parser->getFunctions();
            auto nodes = parser->getNodeCount();

            std::cout << "ASSERTIONS:" << assertions.size() << std::endl;
            std::cout << "VARIABLES:" << variables.size() << std::endl;
            std::cout << "FUNCTIONS:" << functions.size() << std::endl;
            std::cout << "NODES:" << nodes << std::endl;
            std::cout << "TIME:" << duration.count() << "ms" << std::endl;

            if (!is_directory) {
                for (auto a : assertions) {
                    std::cout << parser->toString(a) << std::endl;
                }
                std::cout << "=== SMT2 ===" << std::endl;
                std::cout << parser->dumpSMT2() << std::endl;
            }
        } else {
            std::cout << "PARSE_FAILURE" << std::endl;
            std::cout << "TIME:" << duration.count() << "ms" << std::endl;
            failed_parses++;
            failed_paths.push_back(input_file);
        }
    }

    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total files tested: " << smt2_files.size() << std::endl;
    std::cout << "Successful parses: " << successful_parses << std::endl;
    std::cout << "Failed parses: " << failed_parses << std::endl;

    if (!failed_paths.empty() && is_directory) {
        std::map<std::string, int> by_logic;
        for (const auto& fp : failed_paths) {
            by_logic[logic_bucket_for_file(instances_dir, fp)]++;
        }
        std::cout << "\n=== FAILURES BY TOP-LEVEL SUBDIR (under root) ===" << std::endl;
        for (const auto& kv : by_logic) {
            std::cout << "  " << kv.first << ": " << kv.second << std::endl;
        }
    }

    return (failed_parses == 0) ? 0 : 1;
}
