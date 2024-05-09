from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject

class SimpleCache(ClockedObject):
    type = 'SimpleCache'
    cxx_header = 'learning_gem5/part2/simple_cache.hh'
    cxx_class = 'gem5::SimpleCache'

    cpu_side = VectorResponsePort('CPU side port, receives requests')   # To receive requests from the CPU
    mem_side = RequestPort('Memory side port, sends requests')   # To send requests to the memory

    latency = Param.Cycles(1, 'Cycles taken on a hit or to resolve a miss')
    
    size = Param.MemorySize('16kB', 'The size of the cache')

    system = Param.System(Parent.any, 'The system that the cache is part of')
