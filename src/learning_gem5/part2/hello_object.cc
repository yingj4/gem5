#include "learning_gem5/part2/hello_object.hh"

#include <iostream>

namespace gem5
{

HelloObject::HelloObject(const HelloObjectParams& params) : SimOjbect(params)
{
    std::cout << "Hello World! from a SimObject" << std::endl;
}

} // namespace gem5
