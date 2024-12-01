#include "Handle.hpp"
#include "Process.hpp"
#include "Thread.hpp"
#include "Pipe.hpp"
#include "ArgParse.hpp"
#include "Socket.hpp"
#include "Concurrency.hpp"
#include "Service.hpp"

#include <cstdio>
#include <cstdint>
#include <string_view>
#include <string>
#include <span>
#include <vector>
#include <memory>

struct Args {
    bool svc = false;
    bool client = false;
    bool server = false;
    std::string_view host = "127.0.0.1";
    std::uint16_t port = 12345;

    void parse(int argc, const char **argv) {
        using namespace abel;
        ArgParser parser{};

        parser.add_arg(
            "help",
            ArgParser::handler_help(
                "Usage: RemoteCMD.exe [-h] [--svc] (-c|-s) [--host <host>] [--port <port>]\n"
                "  --svc: Run as a Windows service. Requires server mode\n"
                "  -c, --client: Run as a client\n"
                "  -s, --server: Run as a server\n"
                "  --host <host>: Host to connect to (default: 127.0.0.1). Ignored for servers\n"
                "  --port <port>: Port to connect to / listen at (default: 12345)"
            ),
            'h'
        );

        parser.add_arg("svc", ArgParser::handler_store_flag(svc));
        parser.add_arg("client", ArgParser::handler_store_flag(client), 'c');
        parser.add_arg("server", ArgParser::handler_store_flag(server), 's');
        parser.add_arg("host", ArgParser::handler_store_str(host));
        parser.add_arg("port", ArgParser::handler_store_int(port));

        parser.parse(argc, argv);
    }
};

class Client {
protected:
    abel::OwningSocket socket{};

public:
    Client() {
    }

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
        //auto input_thread = abel::Thread::create<Client, &Client::input_to_socket>(this, true, true).handle;

        //printf("Starting the socket -> output thread...\n");
        //auto output_thread = abel::Thread::create<Client, &Client::output_from_socket>(this, true, true).handle;

        //printf("Ready!\n");
        //input_thread.resume_thread();
        //output_thread.resume_thread();
        //abel::Handle::wait_multiple<true>(input_thread, output_thread);

        auto my_stdin = abel::Handle::get_stdin();
        auto my_stdout = abel::Handle::get_stdout();

        my_stdin.set_console_mode(my_stdin.get_console_mode() & ~ENABLE_ECHO_INPUT & ~ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
        //my_stdin.set_console_mode(my_stdin.get_console_mode() | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);

        printf("Ready!\n");
        abel::ParallelAIOs(
            abel::async_transfer(my_stdin.console_async_io(), socket.borrow()),
            abel::async_transfer(socket.borrow(), my_stdout.console_async_io())
        ).run();
    }
};

class ServerSvc;

class Server {
protected:
    struct ClientConn {
        abel::OwningSocket socket{};
        abel::OwningHandle thread{};
        abel::Pipe pipe_out{};
        abel::Pipe pipe_in{};
        std::atomic<bool> dead{false};

        void handle() {
            try {
                pipe_out = abel::Pipe::create_async(true);
                pipe_in = abel::Pipe::create_async(true);

                auto cmd = abel::Process::create(
                    "C:\\Windows\\System32\\cmd.exe",
                    // "/q",  // Because otherwise echo is only done a newline
                    "",
                    "",
                    true,
                    CREATE_NO_WINDOW /*CREATE_NEW_CONSOLE /*DETACHED_PROCESS*/,
                    STARTF_USESHOWWINDOW,
                    pipe_in.read,
                    pipe_out.write,
                    pipe_out.write,
                    [](STARTUPINFOA &info) {
                        info.wShowWindow = SW_HIDE;
                    }
                );

                //printf("Client ready!\n");
                //abel::ParallelAIOs(
                //    abel::async_transfer(socket.borrow(), socket.borrow())
                //).run();
                //abel::ParallelAIOs(
                //    abel::async_transfer(socket.borrow(), pipe_in.write.borrow()),
                //    abel::async_transfer(pipe_in.read.borrow(), socket.borrow())
                //).run();
                abel::ParallelAIOs(
                    abel::async_transfer(pipe_out.read.borrow(), socket.borrow()),
                    abel::async_transfer(socket.borrow(), pipe_in.write.borrow())
                ).run();

                if (cmd.process.get_exit_code_process() == STILL_ACTIVE) {
                    cmd.process.terminate_process();
                }
                cmd.process.wait();
            } catch (std::exception &e) {
                printf("Client error: %s\n", e.what());
            }

            dead.store(true);
        }
    };

    abel::OwningSocket listenSocket{};
    std::vector<std::unique_ptr<ClientConn>> clients{};
    bool service_mode = false;

public:
    Server(bool service_mode_) :
        service_mode(service_mode_) {
    }

    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;
    Server(Server &&) noexcept = default;
    Server &operator=(Server &&) noexcept = default;

    static Server setup(const char *host, uint16_t port, bool service_mode = false) {
        Server sv{service_mode};
        //printf("Setting up server...\n");
        (void)host;  // TODO: resolve host, bind to single address?
        sv.listenSocket = abel::Socket::listen(port);
        return sv;
    }

    void serve() {
        // TODO: Shutdown logic
        while (true) {
            abel::OwningSocket clientSocket = listenSocket.accept();

            // Note: if no new clients have connected in a while, old ones won't be
            // cleaned up, but that's not actually a problem, since the buffer wouldn't
            // have grown either in that time.
            std::erase_if(
                clients,
                [](const auto &client) {
                    return client->dead.load();
                }
            );

            log("Serving new client");
            auto client = std::make_unique<ClientConn>();
            client->socket = std::move(clientSocket);
            client->thread = abel::Thread::create<ClientConn, &ClientConn::handle>(client.get()).handle;
            clients.push_back(std::move(client));
        }
    }

    template <DWORD kind = EVENTLOG_INFORMATION_TYPE, typename... Args>
    void log(const char *message, Args... args) {
        if (service_mode) {
            ServerSvc::get().log<kind>(message, args...);
        } else {
            printf(message, args...);
        }
    }

};

class ServerSvc : public abel::Service<ServerSvc> {
protected:
    Server server{true};

public:
    static constexpr const char *name = "RemoteCMDServer";

    void work() {
        Args args{};
        args.parse(argc, argv);

        abel::SocketLibGuard socket_lib_guard{};

        log("Serving now");

        auto server = Server::setup(args.host.data(), args.port, true);
        server.serve();
    }

};


int main(int argc, const char **argv) {
    try {
        using namespace abel;

        Args args{};
        args.parse(argc, argv);

        if (args.server + args.client != 1) {
            fail("Specify exactly one of --client or --server");
        }

        if (args.svc) {
            ServerSvc::startup();
            return 0;
        }

        SocketLibGuard socket_lib_guard{};

        if (args.server) {
            printf("Running as server...\n");

            auto server = Server::setup(args.host.data(), args.port);
            server.serve();
        } else {
            printf("Running as client...\n");

            auto client = Client::connect(args.host.data(), args.port);
            client.run();
        }

        printf("Done\n");
    } catch (const std::exception &e) {
        printf("ERROR! %s\n", e.what());
        return -1;
    }

    return 0;
}
