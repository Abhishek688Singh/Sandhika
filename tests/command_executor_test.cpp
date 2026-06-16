#include "sandhika/commands/command_executor.h"
#include <cassert>
#include <iostream>

using namespace sandhika::commands;

int main() {
    CommandExecutor executor;
    
    // Test basic command
    auto res = executor.execute("echo hello");
    assert(res.exit_code == 0);
    assert(res.stdout_text == "hello\n");
    assert(!res.timed_out);

    // Test failing command
    auto res2 = executor.execute("false");
    assert(res2.exit_code == 1);

    // Test timeout
    auto res3 = executor.execute("sleep 2", std::chrono::milliseconds(100));
    assert(res3.timed_out);

    std::cout << "CommandExecutor tests passed\n";
    return 0;
}
