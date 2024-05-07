#include "learning_gem5/part2/goodbye_object.hh"

#include "base/trace.hh"
#include "debug/Hello.hh"
#include "sim/sim_exit.hh"

GoodbyeObject::GoodbyeObject(const GoodbyeObjectParams& param) : SimObject(param), event([this]{processEvent()}, name()), bandwidth(params.write_bandwidth), bufferSize(params.buffer_size), buffer(nullptr), bufferUsed(0)
{
    buffer = new char[bufferSize];
    DPRINTF(Hello, "Created the goodbye object\n");
}

GoodbyeObject::~GoodbyeObject()
{
    delete buffer;
}

void GoodbyeObject::processEvent()
{
    DPRINTF(Hello, "Processing the goodbye event\n");
    fillBuffer();
}

void GoodbyeObject::sayGoodbye(std::string name)
{
    DPRINTF(Hello, "Say goodbye to %s\n", name);
    message = "Goodbye" + name + "!\n";
    fillBuffer();
}


