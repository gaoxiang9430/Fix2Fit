# f1x-bench #

f1x-bench is a tool and a format specification for executing program repair benchmarks. In f1x-bench, an experiment with a defect has the following lifecycle:

- `fetch` - obtain a fresh copy of the subject program
- `set-up` (optional) - configure the copy, etc.
- `run` - execute the repair tool with the subject program
- `tear-down` (optional) - check patch correctness, etc.
- `remove` - delete the copy of the subject program

## Tool ##

The tool must be executed from the benchmark root directory.

Usage:

- `f1x-bench DEFECT` - perform an experiment (fetch + set-up + run + tear-down + remove) for DEFECT
- `f1x-bench` - perform experiments for all defects
- `f1x-bench OPTIONS -- TOOL_OPTIONS` - you can pass options for the repair tool after `--`

Options:

- `--output DIR` - specify output directory (optional)
- `--timeout` - timeout for individual defect (optional)
- `--verbose` - produce extended output
- `--help` - print help message
- `--version` - print version

Options when used with a single defect:

- `--fetch` - only fetch (create directory DEFECT in current directory)
- `--set-up` - only set-up
- `--run` - only run
- `--tear-down` - only tear-down
- `--cmd` - only print f1x command for specified DEFECT

## Format specification ##

The benchmark directory should contain two files: `tests.json` and `benchmark.json`.

The content of `tests.json` should be as follows

    {
        "defect1": {
            "negative": [ "1", "2" ],
            "positive": [ "3", "4", "5"]
        },
        ...
    }



The content of `benchmark.json` should be as follows (`fetch`, `set-up`, `tear-down` are shell commands, `$F1X_BENCH_OUTPUT` is provided environment variable):

    {
        "defect1": {
            "fetch": "./fetch defect1",
            "set-up": "./configure defect1",
            "tear-down": "./check-patch $F1X_BENCH_OUTPUT defect1",
            "source": "defect1/src",
            "files": [ "lib/source.c" ],
            "build": "make -e && cd lib1 && make -e",
            "test-timeout": 1000,
            "driver": "driver"
        },
        ...
    }
    
All commands and paths (except for `files`) are relative to benchmark root directory. `set-up`, `tear-down` and `build` are optional.
