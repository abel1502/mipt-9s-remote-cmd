#include <cstdio>

#include "Handle.hpp"
#include "Process.hpp"
#include "Pipe.hpp"

int main(int argc, const char **argv) {
    using namespace abel;

    printf("Starting\n");

    auto pipe = Pipe::create(true);
    auto process = Process::create(
        L"C:\\Windows\\System32\\cmd.exe",
        L"/c dir",
        L"",
        true,
        0,
        0,
        NULL,
        pipe.write.raw(),
        pipe.write.raw()
    );

    WaitForSingleObject(process.process.raw(), INFINITE);

    auto result = pipe.read.read(1024, false);

    printf("<data>\n%s\n</data>\n", std::string(result.begin(), result.end()).c_str());

    printf("Done\n");



    return 0;
}
