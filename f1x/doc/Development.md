# Developer documentation #

The system consists of three main parts:

1. Repair module (f1x) 
2. Code transformation module (f1x-transform)
3. Analysis runtime (libf1xrt.so)

The repair module is responsible for running tests, maintaining search space and partitioning, and synthesizing analysis runtime. f1x-transform is responsible for instrumenting buggy code, applying transformation schemas to suspicious locations and applying generated patches. The analysis runtime library is responsible for computing test-equivalence partitions.

## Repair workflow ##

![Workflow](./workflow.svg)

## Search space representation ##

The entire search space is explicitly represented as a C++ vector `searchSpace` in SearchEngine.cpp. The search space can be efficiently traversed in an arbitrary order.

The following data structures are used for search space representation (in `repair/Core.h`):

- `TransformationSchema` is a high-level transformation rule (e.g. adding guard)
- `SynthesisRule` is a kind of expression modification (e.g. operator replacement)
- `SchemaApplication` is an application of a schema to a program location
- `Patch` is a concrete patch

Search space elements are assigned unique identifiers. Each application of a transformation schema is identified using a positive integer `f1xapp`. Each candidate patch is transparently identified using five positive integers: `f1xid_base`, `f1xid_int2`, `f1xid_bool2`, `f1xid_comp3`, `f1xid_param` (`PatchId` data structure in `repair/Core.h`) .

## Runtime ##

f1x analysis runtime is generated automatically and dynamically linked to the buggy program. The runtime is responsible for computing test-equivalence partitions. It takes a candidate and a search space to partition as the arguments and outputs a subset of the given search space that have the same semantic impact as the given candidate.

The repair process and the analysis runtime interact through shared memory (POSIX Shared Memory).

## Transformation ##

f1x relies on Clang to perform source code transformation.

f1x-transform represents applications of transformation schemas to program locations in the following way:

    [
        {
            "appId": 1,
            "schema": "if_guard",
            "context": "condition",
            "location" : {...},
            "expression": {...},
            "components": [...]
        },
        ...
    ]
    
Where each location is represented as follows:

    {
        "fileId": 0,
        "beginLine": 1,
        "beginColumn": 2,
        "endLine": 1,
        "endColumn": 10
    }

Each expression is represented as follows:

    {
        "kind": "operator",
        "type": "integer",
        "rawType": "int",
        "repr": ">",
        "args": [
            {
                "kind": "constant",
                "type": "integer",
                "rawType": "int",
                "repr": "1"
            },
            {
                "kind": "variable",
                "type": "integer",
                "rawType": "unsigned int",
                "repr": "x"
            }
        ]
    }
    
Kinds: `operator`, `constant`, `variable`. Types: `integer`, `pointer`. For pointers, `rawType` is the pointee type.
