# device-memory-functions
A library that provides few functions for device memory.
It's main purpose is to allocate DM buffer and increment it's value using memic atomic feature.

As a sanity check, the repo provides a test that calls the library and prints on the CLI the DM values (before and after the increment operation).

# Known issues
> Issue #1:
> Since we are going to icrement the buffer using MEMIC atomics, and since the PXTH,
> while doing the read operation (during the read-modify-write to implement the atomicity)
> assumes that the data it reads is in Big Endian we set the inital value also in Big Endian.
> This assumption of the PXTH (that it reads MEMIC in Big Endian in the MEMIC atomic interface) is a HW Bug
> this BUG will be fixed in CX-8. 

> Issue #2:
> In CX-7 PXTH assumes the atomic operand it gets from the host is in Big Endian
> This is a HW BUG and will be fixed in CX-8

# Build with unit-test
```
cd <root-dir>
mkdir build
cd build
cmake -DBUILD_TEST=ON ..
make
```

# Build without unit-test
```
cd <root-dir>
mkdir build
cd build
cmake -DBUILD_TEST=OFF ..
make
```

# Run (you need to build with unit-tests)
```
./device_memory_functions_test -i 10 -r 5 -s 50 -d mlx5_3
```
