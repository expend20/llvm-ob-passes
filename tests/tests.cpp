#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#include <stdexcept>
#include <cstdio>
#include <iostream>
#include <memory>
#include <array>

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    int exit_status;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
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

std::string run_test(const char* pass_name) {
    // get the input .ll first: clang -c -emit-llvm test.c -o test.ll
    std::string output;
    output = exec("clang -c -S -emit-llvm -O1 ../tests/test.c -o test.ll");
    //std::cout << "Output: " << output << std::endl;

    // run opt with BogusControlFlow pass: opt -load-pass-plugin=./libLLVMExamplePass.so -passes="bogus-control-flow" test.ll -o test_obfuscated.ll
    std::string command = "opt -load-pass-plugin \"./libLLVMExamplePass.so\" -passes \"";
    command += pass_name;
    command += "\" test.ll -S -o test_obfuscated.ll -debug-pass-manager";
    std::cout << "Executing command: " << command << std::endl;
    output = exec(command.c_str());
    //std::cout << "Command output: " << output << std::endl;

    // compile the obfuscated .ll to the binary
    output = exec("clang test_obfuscated.ll -o test_obfuscated");
    //std::cout << "Output: " << output << std::endl;

    // run the obfuscated binary
    output = exec("./test_obfuscated");
    //std::cout << "Output: " << output << std::endl;
    return output;
}

std::string original_output;

TEST(BogusControlFlowTest, OutputMatches) {

    std::string obfuscated_output = run_test("bogus-control-flow");
    EXPECT_EQ(original_output, obfuscated_output);

}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    original_output = run_test("");
    // print current working directory
    {
        std::cout << "Current working directory: " << getcwd(nullptr, 0) << std::endl;
        // exec "touch ./libLLVMExamplePass.so"
        const auto result = exec("file ./libLLVMExamplePass.so");
        std::cout << "File result: " << result << std::endl;
    }

    std::cout << "Current working directory: " << getcwd(nullptr, 0) << std::endl;
    return RUN_ALL_TESTS();
}