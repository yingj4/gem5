import m5
from m5.objects import *

# Needed for running C++ threads
m5.util.addToPath("../../")
from common.FileSystemConfig import config_filesystem

from msi_caches import MyCacheSystem

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'
system.mem_ranges = [AddrRange('512MB')]

system.cpu = [X86TimingSimpleCPU() for _ in range(2)]

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]

for cpu in system.cpu:
    cpu.createInterruptController()

system.caches = MyCacheSystem()
system.caches.setup(system, system.cpu, [system.mem_ctrl])


# system.membus = SystemXBar()

# system.cpu.icache_port = system.membus.cpu_side_ports
# system.cpu.dcache_port = system.membus.cpu_side_ports

# system.cpu.interrupts[0].pio = system.membus.mem_side_ports
# system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
# system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

# system.system_port = system.membus.cpu_side_ports

# system.mem_ctrl.port = system.membus.mem_side_ports

thispath = os.path.dirname(os.path.realpath(__file__))
binary = os.path.join(thispath, "../../../", "tests/test-progs/hello/bin/x86/linux/threads")
# binary = "/home/gem5/tests/test-progs/hello/bin/x86/linux/threads"

system.workload = SEWorkload.init_compatible(binary)

process = Process()
process.cmd = [binary]

for cpu in system.cpu:
    cpu.workload = process
    cpu.createThreads()

config_filesystem(system)

root = Root(full_system = False, system = system)
m5.instantiate()


print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'.format(m5.curTick(), exit_event.getCause()))
