from m5.params import *
from m5.SimObject import SimObject

class HelloObject(SimObject):
    type = 'HelloObject'
    cxx_header = 'learning_gem5/part2/hello_object.hh'
    cxx_class = 'gem5::HelloObject'

    time_to_wait = Param.Latency('Time before firing the event')
    number_of_fires = Param.Int(1, 'Number of times to fire the event before goodbye')


class GoodbyeObject(SimObject):
    type = 'GoodbyeObject'
    cxx_header = 'learning_gem5/part2/goodbye_object.hh'
    cxx_class = 'gem5::GoodbyeObject'

    buffer_size = Param.MemorySize('1kB', 'Size of buffer to fill with Goodbye')
    write_bandwidth = Param.MemoryBandwidth('100MB/s', 'Bandwidth to fill the buffer')
