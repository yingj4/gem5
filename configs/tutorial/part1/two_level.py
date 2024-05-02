import m5
from m5.objects import *
from caches import *

s = System()

s.clk_domain = SrcClockDomain()
s.clk_domain.clock = '1GHz'
s.clk_domain.voltage_domain = VoltageDomain()

s.mem_mode = 'timing'
s.mem_ranges = [AddrRange('512MB')]

s.cpu = X86TimingSimpleCPU()


s.membus = SystemXBar()

s.cpu.icache = L1ICache()
s.cpu.dcache = L1DCache()
s.cpu.icache.connectCPU(s.cpu)
s.cpu.dcache.connectCPU(s.cpu)


s.l2bus = L2XBar()
s.cpu.icache.connectBus(s.l2bus)
s.cpu.dcache.connectBus(s.l2bus)

s.cpu.createInterruptController()
s.cpu.interrupts[0].pio = s.membus.mem_side_ports
s.cpu.interrupts[0].int_requestor = s.membus.cpu_side_ports
s.cpu.interrupts[0].int_responder = s.membus.mem_side_ports

s.system_port = s.membus.cpu_side_ports

s.mem_ctrl = MemCtrl()
s.mem_ctrl.dram = DDR3_1600_8x8()
s.mem_ctrl.dram.range = s.mem_ranges[0]
s.mem_ctrl.port = s.membus.mem_side_ports

thispath = os.path.dirname(os.path.realpath(__file__))
binary = os.path.join(thispath, "../../../", "tests/test-progs/hello/bin/x86/linux/hello")
# binary = "/home/gem5/tests/test-progs/hello/bin/x86/linux/hello"

s.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = [binary]
s.cpu.workload = process
s.cpu.createThreads()


root = Root(full_system = False, system = s)
m5.instantiate()


print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'
      .format(m5.curTick(), exit_event.getCause()))
