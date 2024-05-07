#include "learning_gem5/part2/hello_object.hh"

#include <iostream>

#include "base/trace.hh"
#include "debug/HelloExample.hh"

namespace gem5
{

HelloObject::HelloObject(const HelloObjectParams& params) : SimObject(params), event([this]{processEvent();}, name()), goodbye(params.goodbye_object), myName(params.name), latency(params.time_to_wait), timesLeft(params.number_of_fires)
{
    // std::cout << "Hello World! From a SimObject!" << std::endl;
    DPRINTF(HelloExample, "Created the hello object\n");
    panic_if(!goodbye, "HelloObject must have a non-null GoodbyeObject!\n");
}

void HelloObject::processEvent()
{
    // DPRINTF(HelloExample, "Hello world! Processing the event\n");

    --timesLeft;
    DPRINTF(HelloExample, "Hello world! Processing the event. %d times left\n", timesLeft);

    if (timesLeft <= 0) {
        DPRINTF(HelloExample, "Done firing!\n");
        goodbye->sayGoodbye();
    } else {
        schedule(event, curTick() + latency);
    }
}

void HelloObject::startup()
{
    schedule(event, latency);
}

} // namespace gem5
