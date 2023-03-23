# device-memory-functions
A library that provides few functions for device memory.
It's main purpose is to allocate DM buffer and increment it's value using memic atomic feature.

As a sanity check, the repo provides a test that calls the library and prints on the CLI the DM values (before and after the increment operation).

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
