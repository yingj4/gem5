from m5.params import *
from m5.proxy import *
from m5.SimObject import SimObject

class SimpleMemobj(SimObject):
    type = 'SimpleMemobj'
    cxx_header = 'learning_gem5/part2/simple_memobj.hh'
    cxx_class = 'gem5::SimpleMemobj'

    inst_port = ResponsePort('CPU side port, receives requests')
    data_port = ResponsePort('CPU side port, receives requests')
    mem_port = RequestPort('Memory side port, sends requests')
