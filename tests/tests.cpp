#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <iostream>
#include <memory>
#include <array>
#include <vector>

std::string exec(std::string cmd) {
    std::cout << "Executing command: " << cmd << std::endl;

    std::array<char, 128> buffer;
    std::string result;
    int exit_status;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    exit_status = pclose(pipe.release());
    if (exit_status != 0) {
        std::cerr << "Command failed with exit code " << WEXITSTATUS(exit_status) << std::endl;
        std::cerr << "Command: " << cmd << std::endl;
        std::cerr << "Output: " << result << std::endl;
        throw std::runtime_error("Command execution failed");
    }

    return result;
}

std::string run_passes(const std::vector<std::string>& pass_names) {
    std::string input_ll = "test.ll";
    std::string output;

    // Generate initial .ll file
    output = exec("clang -c -S -emit-llvm -O1 ../tests/test.c -o " + input_ll);

    std::string passes_names = "";
    for (size_t i = 0; i < pass_names.size(); ++i) {
        const std::string& pass_name = pass_names[i];
        if (passes_names != "") {
            passes_names += ",";
        }
        passes_names += pass_name;
        std::string output_ll = passes_names + ".ll";

        std::string command = "opt -load-pass-plugin=./libpasses.so -passes \"";
        command += pass_name;
        command += "\" " + input_ll + " -S -o " + output_ll + " -debug-pass-manager";
        output = exec(command.c_str());

        // Set the output as input for the next pass
        input_ll = output_ll;
    }

    // Compile the final obfuscated .ll to binary
    std::string binary_name = passes_names.size() ? passes_names : "test_obfuscated";
    output = exec("clang " + input_ll + " -o " + binary_name);

    // Run the obfuscated binary
    return exec("./" + binary_name);
}

std::string original_output;

TEST(ExamplePassTest, OutputMatches) {
    std::string obfuscated_output = run_passes({"example-pass"});
    EXPECT_EQ(original_output, obfuscated_output);
}

TEST(BogusControlFlowTest, OutputMatches) {
    std::string obfuscated_output = run_passes({"pluto-bogus-control-flow"});
    EXPECT_EQ(original_output, obfuscated_output);
}

// TODO: Standalone test failed after noinline introduced for indirect call
TEST(FlatteningTest, DISABLED_OutputMatches) {
    std::string obfuscated_output = run_passes({"pluto-flattening"});
    EXPECT_EQ(original_output, obfuscated_output);
}

TEST(GlobalEncryptionTest, OutputMatches) {
    std::string obfuscated_output = run_passes({"pluto-global-encryption"});
    EXPECT_EQ(original_output, obfuscated_output);
}

TEST(IndirectCallTest, OutputMatches) {
    std::string obfuscated_output = run_passes({"pluto-indirect-call"});
    EXPECT_EQ(original_output, obfuscated_output);
}

TEST(MBAObfuscationTest, OutputMatches) {
    std::string obfuscated_output = run_passes({"pluto-mba-obfuscation"});
    EXPECT_EQ(original_output, obfuscated_output);
}

// Add a new test case for combined passes
TEST(CombinedTest, OutputMatches) {
    std::string obfuscated_output = run_passes({
        "example-pass",
        "pluto-bogus-control-flow",
        "pluto-flattening",
        "pluto-global-encryption",
        "pluto-indirect-call",
        "pluto-mba-obfuscation" 
    });
    EXPECT_EQ(original_output, obfuscated_output);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    original_output = run_passes({});
    
    // print current working directory
    {
        std::cout << "Current working directory: " << getcwd(nullptr, 0) << std::endl;
        const auto result = exec("file ./libpasses.so");
        std::cout << "File result: " << result << std::endl;
    }

    std::cout << "Current working directory: " << getcwd(nullptr, 0) << std::endl;
    return RUN_ALL_TESTS();
}