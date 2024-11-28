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
#include <string>
#include <span>

class Client {
protected:
    abel::Socket socket{};

    Client() {}

public:
    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;
    Client(Client &&) noexcept = default;
    Client &operator=(Client &&) noexcept = default;

    static Client connect(const char *host, uint16_t port) {
        Client cl{};
        printf("Connecting to server...\n");
        cl.socket = abel::Socket::connect(host, port);
        return cl;
    }

    void run() {
        //printf("Starting the input -> socket thread...\n");
        //auto input_thread = abel::Thread::create(&Client::input_to_socket, this, true, true).handle;

        //printf("Starting the socket -> output thread...\n");
        //auto output_thread = abel::Thread::create(&Client::output_from_socket, this, true, true).handle;

        //printf("Ready!\n");
        //input_thread.resume_thread();
        //output_thread.resume_thread();
        //abel::Handle::wait_multiple<true>(input_thread, output_thread);

        printf("Ready!\n");
        abel::ParallelAIOs(input_to_socket(), output_from_socket()).run();
    }

    abel::AIO<void> input_to_socket() {
        auto console_in = abel::Handle::get_stdin();

        // TODO: Disable echoing?

        std::vector<unsigned char> buffer{};

        while (true) {
            co_await abel::event_signaled{console_in};

            buffer.clear();

            size_t queue_size = console_in.console_input_queue_size();

            for (size_t i = 0; i < queue_size; ++i) {
                auto input = console_in.read_console_input();
                if (input.EventType != KEY_EVENT) {
                    continue;
                }

                buffer.push_back(input.Event.KeyEvent.uChar.AsciiChar);
            }

            if (buffer.empty()) {
                continue;
            }

            auto result = co_await socket.write_async_full_from(buffer);
            if (result.is_eof) {
                break;
            }
        }
    }

    abel::AIO<void> output_from_socket() {
        auto console_out = abel::Handle::get_stdout();

        std::vector<unsigned char> buffer(4096);

        while (true) {
            auto result = co_await socket.read_async_into(buffer);
            if (result.is_eof) {
                break;
            }
            console_out.write_full_from(std::span(buffer).subspan(0, result.value));
        }
    }
};

class Server {
protected:

public:
};

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
        "C:\\Windows\\System32\\cmd.exe",
        "/c dir",
        "",
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
