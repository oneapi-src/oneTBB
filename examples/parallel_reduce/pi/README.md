# Pi sample
Parallel version of calculating &pi; by numerical integration.

## Building the example
```
cmake <path_to_example>
cmake --build .
```

## Running the sample
### Predefined make targets
* `make run_pi` - executes the example with predefined parameters
* `make perf_run_pi` - executes the example with suggested parameters to measure the oneTBB performance

### Application parameters
Usage:
```
pi [n-of-threads=value] [n-of-intervals=value] [silent] [-h] [n-of-threads [n-of-intervals]]
```
* `-h` - prints the help for command line options.
* `n-of-threads` - the number of threads to use; a range of the form low\[:high\], where low and optional high are non-negative integers or `auto` for a platform-specific default number.
* `n-of-intervals` - the number of intervals to subdivide into, must be a positive integer.
* `silent` - no output except elapsed time.
