#include <Logger.hpp>
#include <Log.hpp>
#include <csignal>

#include <Heli/Top/HeliTopologyAc.hpp>
#include <getopt.h>

static volatile bool terminate = false;
static Heli::TopologyState state;

static void sighandler(int signum)
{
    (void) signum;
    Heli::teardown(state);
    terminate = true;
}

static void print_usage(const char* app)
{
    printf("Usage: ./%s [options]\n"
           "  -p\tport_number\n"
           "  -a\thostname/IP address\n", app);
}

static void run_cycle()
{
    // call interrupt to emulate a clock
    Heli::blockDrv.callIsr();
    Os::Task::delay(200); // 5Hz
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

    while (!terminate)
    {
        run_cycle();
    }

    Fw::Logger::logMsg("Shutting down...\n");
    Os::Task::delay(1000);

    return 0;
}
