#include <Logger.hpp>
#include <csignal>

#include <Heli/Top/HeliTopologyAc.hpp>

static volatile bool terminate = false;
static Heli::TopologyState state;

static void sighandler(int signum)
{
    (void) signum;
    Heli::teardown(state);
    terminate = true;
}

void run_cycle()
{
    // call interrupt to emulate a clock
    Heli::blockDrv.callIsr();
    Os::Task::delay(200); // 5Hz
}

I32 main()
{
    // register signal handlers to exit program
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    Fw::Logger::logMsg("Booting up\n");
    state = Heli::TopologyState();
    Heli::setup(state);

    while (!terminate)
    {
        run_cycle();
    }

    Fw::Logger::logMsg("Shutting down...\n");
    Os::Task::delay(1000);

    return 0;
}
