# Troubleshooting #

When encounter a problem, please follow the steps below.

1. Reproduce the problem in a clean environment (e.g. fresh copy of your project, removed intermediate test data)
2. Execute f1x with `--verbose` option to obtain more detailed information about execution
3. Save the intermediate data directory of the failing execution (the directory can be found in log messages)

## Common problems ##

### f1x produces different results in multiple runs with the same input  ###

f1x implementation is deternimistic. The sources of non-determinism can be

- Non-determinism of program tests
- Timeouts (check if the total number of timeouts is the same for different runs)
