#ifndef __TUTORIAL_HELLO_OBJECT_HH__
#define __TUTORIAL_HELLO_OBJECT_HH__

#include "params/HelloObject.hh"
#include "sim/sim_objects.hh"

namespace gem5
{

class HelloObject : public SimOjbect
{
  public:
    HelloObject(const HelloObjectParams& p);
};

} // namespace gem5

#endif // __TUTORIAL_HELLO_OBJECT_HH__
