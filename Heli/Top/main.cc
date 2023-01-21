#include <Logger.hpp>
#include <Log.hpp>
#include <csignal>

#include <Heli/Top/HeliTopologyAc.hpp>
#include <getopt.h>

static std::atomic<bool> is_alive = true;
static Heli::TopologyState state;

static void sighandler(int signum)
{
    (void) signum;
    Heli::linuxTimer.quit();
}

static void print_usage(const char* app)
{
    printf("Usage: ./%s [options]\n"
           "  -p\tport_number\n"
           "  -a\thostname/IP address\n", app);
}

I32 main(int argc, char* argv[])
{
    state = Heli::TopologyState();

    I32 option;
    while ((option = getopt(argc, argv, "hp:a:")) != -1)
    {
        switch (option)
        {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'p':
                state.portNumber = std::stoi(optarg);
                break;
            case 'a':
                state.hostName = optarg;
                break;
            case '?':
                return 1;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // register signal handlers to exit program
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    Os::Log consoleLogger;
    Fw::Logger::registerLogger(&consoleLogger);

    Fw::Logger::logMsg("Booting up\n");
    Fw::Logger::logMsg("Connecting to GDS service @ %s:%d", (POINTER_CAST)state.hostName, state.portNumber);

    Heli::setup(state);

    // Only run the FSW if initialization was successful
    // Otherwise shutdown immediately
    if (Heli::Init::status)
    {
        Heli::linuxTimer.startTimer(100);
    }

    Fw::Logger::logMsg("Shutting down...\n");
    Heli::teardown(state);

    Os::Task::delay(1000);
    return 0;
}
