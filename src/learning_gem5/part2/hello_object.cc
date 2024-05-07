#include <iostream>

#include "base/trace.hh"
#include "debug/HelloExample.hh"
#include "learning_gem5/part2/hello_object.hh"

namespace gem5
{

HelloObject::HelloObject(const HelloObjectParams& params) : SimObject(params), event([this]{processEvent();}, name())
{
    // std::cout << "Hello World! From a SimObject!" << std::endl;
    DPRINTF(HelloExample, "Created the hello object\n");
}

void HelloObject::processEvent()
{
    DPRINTF(HelloExample, "Hello world! Processing the event\n");
}

void HelloObject::startup()
{
    schedule(event, 100);
}

} // namespace gem5
