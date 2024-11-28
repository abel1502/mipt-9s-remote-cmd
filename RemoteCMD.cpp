#include "Handle.hpp"
#include "Process.hpp"
#include "Thread.hpp"
#include "Pipe.hpp"
#include "ArgParse.hpp"
#include "Socket.hpp"
#include "Concurrency.hpp"

#include <cstdio>
#include <cstdint>
#include <string_view>

template <abel::async_io S, abel::async_io D>
abel::AIO<void> async_connect(S src, D dst) {
    constexpr size_t buf_size = 4096;
    std::unique_ptr<unsigned char[buf_size]> buf = std::make_unique<unsigned char[buf_size]>();
    while (true) {
        auto read_result = co_await src.read_async_into(buf.get());
        if (read_result.eof) {
            break;
        }
        auto write_result = co_await dst.write_async_full_from(std::span(buf.get(), read_result.value));
        if (write_result.eof) {
            break;
        }
    }
}

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

    SocketLibGuard socket_lib_guard{};

    /*Thread::create([]() -> DWORD {
        printf("Thread test\n");
        return 0;
    });*/

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

    process.process.wait();

    auto result = pipe.read.read((size_t)1024, false).value;

    printf(
        "<data>\n%s\n</data>\n",
        std::string(result.begin(), result.end()).c_str()
    );

    printf("Done\n");

    return 0;
}
