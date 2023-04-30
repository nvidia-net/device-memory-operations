# Device Memory Operations

A library that provides few functions for device memory. Its main purpose is to allocate Device Memory (DM) buffer and increment its value using Device Memory Operations API.

As a sanity check, the repo provides a test that calls the library and prints on the CLI the DM values (before and after the increment operation).

## Known issues

### Issue #1

Since we are going to increment the buffer using MEMIC Atomic operations, and since the PXTH, while doing the read operation (during the read-modify-write to implement the atomicity) assumes that the data it reads is in Big Endian we **set the initial value also in Big Endian**. This assumption of the PXTH (that it reads MEMIC in Big Endian in the MEMIC atomic interface) is an HW Bug this BUG will be fixed in CX-8. 

### Issue #2

In CX-7 PXTH assumes the atomic operand it gets from the host is in Big Endian. This is an HW BUG and will be fixed in CX-8.


### What scenario reveals the bugs?

SW that increments some MEMIC buffer with values larger than what a single byte can hold (i.e., `0xff` or `255` in decimal) will see that the value that is written in MEMIC is incorrect. For example the following psuedo code:

```c
uint32_t* addr = memic_alloc();
memic_init(addr, 0); // Write 0 to MEMIC using "regular" PCIe Write request
memic_add(addr, 254); // Increments the MEMIC using MEMIC Atomic operations
memic_read(addr, &result); // result = 254
memic_add(addr, 2);
memic_read(addr, &result); // result = 1 instead of 256! <--- BUG
```

### SW workarounds

```c
uint32_t* addr = memic_alloc();
memic_init(addr, htobe64(0)); // 0 Does not really require any conversion...
memic_add(addr, htobe64(254)); // Increments the MEMIC using MEMIC Atomic operations
memic_read(addr, &result); // betoh64(result) = 254
memic_add(addr, htobe64(2));
memic_read(addr, &result); // betoh64(result) = 256
```

### PXTH registers

The HW has 3 registers that control the endianness of the MEMIC operations for a "Write" operation.

```sh
pxth.network_memory.atomic_memic_operation.wr_data_after_add_big_endian
pxth.network_memory.atomic_memic_operation.icmc_resp_for_wr_big_endian
pxth.network_memory.atomic_memic_operation.wr_operand_big_endian
```

Resetting the registers (i.e., setting their value to zero) fixes the bugs and the HCA behaves as initially intended in the architecture for the MEMIC Atomic operations feature for CX-7. Configuring the registers can be done using the simple script (`configure-endianness-in-pxth-to-fix-cx-7-bug.sh`):

```sh
#!/bin/bash

# Should be something like mlx5_4 (see `ibstat` command output)
#dev=mlx5_4 # Example
dev=$1

# Setting the register at address 0x5830b4, with offset 2 bits and width 1 bit to 0x0
mcra $dev 0x5830b4.2:1 0x0

# Setting the register at address 0x5830b4, with offset 3 bits and width 1 bit to 0x0
mcra $dev 0x5830b4.3:1 0x0

# Setting the register at address 0x5830b4, with offset 4 bits and width 1 bit to 0x0
mcra $dev 0x5830b4.4:1 0x0
```

## Build with test

```sh
cd <root-dir>
mkdir build
cd build
cmake -DBUILD_TEST=ON ..
make
```

## Build without test

```sh
cd <repo-root-dir>
mkdir build
cd build
cmake -DBUILD_TEST=OFF ..
make
```

## Run (you need to build with unit-tests)

The following command is running the unit test built [here](#build-with-test):

```sh
./device_memory_functions_test -i 100 -r 5 -s 50 -d mlx5_3
```