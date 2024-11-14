#include <cstdio>
#include <cstdint>
#include <string_view>

#include "Handle.hpp"
#include "HandleIO.hpp"
#include "Process.hpp"
#include "Thread.hpp"
#include "Pipe.hpp"
#include "ArgParse.hpp"

int main(int argc, const char **argv) {
    using namespace abel;

    ArgParser parser{};

    parser.add_arg(
        "help",
        ArgParser::handler_help(
            "Usage: RemoteCMD.exe [-h] (-c|-s) [--host <host>] [--port <port>]\n"
            "  -c, --client: Run as a client\n"
            "  -s, --server: Run as a server\n"
        ),
        'h'
    );

    bool client = false;
    parser.add_arg("client", ArgParser::handler_store_flag(client), 'c');
    bool server = false;
    parser.add_arg("server", ArgParser::handler_store_flag(server), 's');
    std::string_view host = "127.0.0.1";
    parser.add_arg("host", ArgParser::handler_store_str(host));
    uint16_t port = 12345;
    parser.add_arg("port", ArgParser::handler_store_int(port));

    parser.parse(argc, argv);

    Thread::create([]() -> DWORD {
        printf("Thread test\n");
        return 0;
    });

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

    auto result = pipe.read.io().read(1024, false);

    printf(
        "<data>\n%s\n</data>\n",
        std::string(result.begin(), result.end()).c_str()
    );

    printf("Done\n");

    return 0;
}
