#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "somtparser/frontend/parser.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

int main(int argc, char* argv[]){
    std::string instances_dir;
    
    if (argc == 2) {
        instances_dir = argv[1];
    } else if (argc == 1) {
        if (std::filesystem::exists("test/instances")) {
            instances_dir = "test/instances";
        } else if (std::filesystem::exists("../test/instances")) {
            instances_dir = "../test/instances";
        } else if (std::filesystem::exists("../../test/instances")) {
            instances_dir = "../../test/instances";
        } else if (std::filesystem::exists("instances")) {
            instances_dir = "instances";
        } else if (std::filesystem::exists("./instances")) {
            instances_dir = "./instances";
        } else {
            std::cerr << "Error: Cannot find instances directory. Try running from project root or test directory." << std::endl;
            std::cerr << "Or specify the path: " << argv[0] << " <instances_directory>" << std::endl;
            return 1;
        }
    } else {
        std::cerr << "Usage: " << argv[0] << " [instances_directory]" << std::endl;
        std::cerr << "If no directory is specified, will look for instances in common locations." << std::endl;
        return 1;
    }
    
    if (!std::filesystem::exists(instances_dir)) {
        std::cerr << "Error: " << instances_dir << " is not a valid directory" << std::endl;
        return 1;
    }

    bool is_directory = std::filesystem::is_directory(instances_dir);
    std::vector<std::string> smt2_files;
    if(is_directory){
        for (const auto& entry : std::filesystem::directory_iterator(instances_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".smt2") {
                smt2_files.push_back(entry.path().string());
            }
        }
    }
    else{
        smt2_files.push_back(instances_dir); // if it is a file, just add it to the list
    }

    std::sort(smt2_files.begin(), smt2_files.end());

    if (smt2_files.empty()) {
        std::cout << "No .smt2 files found in " << instances_dir << std::endl;
        return 0;
    }

    std::cout << "Found " << smt2_files.size() << " .smt2 files to test" << std::endl;


    int successful_parses = 0;
    int failed_parses = 0;
    
    for (const auto& input_file : smt2_files) {
        std::shared_ptr<SOMTParser::Parser> parser = 
            std::make_shared<SOMTParser::Parser>();
        parser->setOption("keep_let", false);
        // parser->setOption("expand_functions", false);

        std::cout << "\n=== Testing file: " << std::filesystem::path(input_file).filename() << " ===" << std::endl;

        // Record start time
        auto start_time = std::chrono::high_resolution_clock::now();

        // parse the input file
        bool parse_success = parser->parse(input_file);

        // Record end time
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        if (parse_success) {
            std::cout << "PARSE_SUCCESS" << std::endl;
            successful_parses++;
            
            // Print statistics
            auto assertions = parser->getAssertions();
            auto variables = parser->getVariables();
            auto functions = parser->getFunctions();
            auto nodes = parser->getNodeCount();    
            
            std::cout << "ASSERTIONS:" << assertions.size() << std::endl;
            std::cout << "VARIABLES:" << variables.size() << std::endl;
            std::cout << "FUNCTIONS:" << functions.size() << std::endl;
            std::cout << "NODES:" << nodes << std::endl;
            std::cout << "TIME:" << duration.count() << "ms" << std::endl;
            
            if(!is_directory){
                for(auto a:assertions){
                    std::cout<<parser->toString(a)<<std::endl;
                }
                std::cout<<"=== SMT2 ==="<<std::endl;
                std::cout<<parser->dumpSMT2()<<std::endl;
            }
            

        } else {
            std::cout << "PARSE_FAILURE" << std::endl;
            std::cout << "TIME:" << duration.count() << "ms" << std::endl;
            failed_parses++;
        }
    }

    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Total files tested: " << smt2_files.size() << std::endl;
    std::cout << "Successful parses: " << successful_parses << std::endl;
    std::cout << "Failed parses: " << failed_parses << std::endl;
    
    return (failed_parses == 0) ? 0 : 1;
}