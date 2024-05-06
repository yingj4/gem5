#include "learning_gem5/part2/hello_object.hh"

#include <iostream>

namespace gem5
{

HelloObject::HelloObject(const HelloObjectParams& params) : SimObject(params)
{
    std::cout << "Hello World! From a SimObject!" << std::endl;
}

} // namespace gem5
