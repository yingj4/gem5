# Copyright (c) 2023 The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from typing import (
    List,
    Optional,
)

from gem5.components.processors.abstract_generator import AbstractGenerator
from gem5.utils.override import overrides

from .strided_generator_core import StridedGeneratorCore


class StridedGenerator(AbstractGenerator):
    def __init__(
        self,
        num_cores: int = 1,
        duration: str = "1ms",
        rate: str = "100GiB/s",
        block_size: int = 64,
        superblock_size: int = 64,
        stride_size: Optional[int] = None,
        min_addr: int = 0,
        max_addr: int = 32768,
        rd_perc: int = 100,
        data_limit: int = 0,
    ) -> None:
        if stride_size is None:
            stride_size = num_cores * superblock_size
        super().__init__(
            cores=self._create_cores(
                num_cores=num_cores,
                duration=duration,
                rate=rate,
                block_size=block_size,
                superblock_size=superblock_size,
                stride_size=stride_size,
                min_addr=min_addr,
                max_addr=max_addr,
                rd_perc=rd_perc,
                data_limit=data_limit,
            )
        )
        """The strided generator

        This class defines an external interface to create a list of strided
        generator cores that could replace the processing cores in a board.

        :param duration: The duration of time for which the generator generates
                         traffic. Must be a string containing a positive number
                         and some unit. For example, "1ms".
        :param rate: The rate at which the data accesses are demanded.
        :param block_size: The number of bytes to be read/written with each
                           request.
        :param superblock_size: The number of bytes to read/write contiguously
                                per stride. Must be a multiple of block_size.
        :param stride_size: The number of bytes from the beginning of one superblock
                            to the beginning of the next superblock.
                            Must be a multiple of superblock_size.
        :param min_addr: The lower bound of the address range the generator
                         will read/write from/to.
        :param max_addr: The upper bound of the address range the generator
                         will read/write from/to.
        :param offset: The starting offset in bytes from min_addr at which
                       the generator reads/writes. Must be a multiple of superblock_size.
        :param rd_perc: The percentage of read requests among all the generated
                        requests. The write percentage would be equal to
                        ``100 - rd_perc``.
        :param data_limit: The amount of data in bytes to read/write by the
                           generator before stopping generation. If set to 0,
                           there will be no data limit.
        """

    def _create_cores(
        self,
        num_cores: int,
        duration: str,
        rate: str,
        block_size: int,
        superblock_size: int,
        stride_size: int,
        min_addr: int,
        max_addr: int,
        rd_perc: int,
        data_limit: int,
    ) -> List[StridedGeneratorCore]:
        return [
            StridedGeneratorCore(
                duration=duration,
                rate=rate,
                block_size=block_size,
                superblock_size=superblock_size,
                stride_size=stride_size,
                min_addr=min_addr,
                max_addr=max_addr,
                offset=i * superblock_size,
                rd_perc=rd_perc,
                data_limit=data_limit,
            )
            for i in range(num_cores)
        ]

    @overrides(AbstractGenerator)
    def start_traffic(self) -> None:
        for core in self.cores:
            core.start_traffic()
