# Concurrent Red-Black Tree

## Requirements

- CMake 4.0 or higher
- C++20 compatible compiler (GCC 10+, Clang 10+, MSVC 2019+)
- pthread library (usually included with your compiler)

## Building the Project

### Linux/macOS
```bash
mkdir build
cd build
cmake ..
make
```

### Windows (Visual Studio)
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Running the Application

After building, run the executable:
```bash
./YTDB
```

On Windows:
```bash
.\Debug\YTDB.exe
```

## Tests

The application includes three concurrent tests:

1. **Concurrent Writes**: 8 threads performing 1,000 writes each
2. **Concurrent Reads**: 16 threads performing 10,000 reads each on a pre-populated tree
3. **Mixed Read/Write**: 4 writer threads and 12 reader threads performing 5,000 operations each

Expected output shows timing and verification results for each test.

## Implementation Details

The implementation uses a Red-Black Tree for balanced performance and a `std::shared_mutex` for thread synchronization, allowing multiple concurrent readers while ensuring exclusive access for writers.