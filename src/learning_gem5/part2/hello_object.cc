#include <iostream>

#include "base/trace.hh"
#include "debug/HelloExample.hh"
#include "learning_gem5/part2/hello_object.hh"

namespace gem5
{

HelloObject::HelloObject(const HelloObjectParams& params) : SimObject(params)
{
    // std::cout << "Hello World! From a SimObject!" << std::endl;
    DPRINTF(HelloExample, "Created the hello object\n");
}

} // namespace gem5
