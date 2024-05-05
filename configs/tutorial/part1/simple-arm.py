import m5
from m5.objects import *

s = System()

s.clk_domain = SrcClockDomain()
s.clk_domain.clock = '1GHz'
s.clk_domain.voltage_domain = VoltageDomain()

s.mem_mode = 'timing'
s.mem_ranges = [AddrRange('512MB')]

s.cpu = X86TimingSimpleCPU()


s.membus = SystemXBar()

s.cpu.icache_port = s.membus.cpu_side_ports
s.cpu.dcache_port = s.membus.cpu_side_ports

s.cpu.createInterruptController()
s.system_port = s.membus.cpu_side_ports

s.mem_ctrl = MemCtrl()
s.mem_ctrl.dram = DDR3_1600_8x8()
s.mem_ctrl.dram.range = s.mem_ranges[0]
s.mem_ctrl.port = s.membus.mem_side_ports

thispath = os.path.dirname(os.path.realpath(__file__))
# binary = os.path.join(thispath, "../../../", "tests/test-progs/hello/bin/x86/linux/hello")
binary = os.path.join(thispath, "../../../", "tests/test-progs/hello/bin/arm/linux/hello")
# binary = os.path.join(thispath, "../../../", "cpu_tests/benchmarks/bin/arm/Bubblesort")
# binary = os.path.join(thispath, "../../../", "cpu_tests/benchmarks/bin/arm/FloatMM")

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
