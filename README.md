# liteseq

liteseq is a lightweight C library for parsing GFA files, using a bidirected
model of the graph.
In this model, each vertex has two sides (or ends): a left side and a right side.



## Features and Limitations

 - **Configurable Parsing**: Choose whether to include vertex labels, references, etc.
 - **Minimal Graph**: Improve performance by fetching only what's needed
 - **GFA Version Support**:
    - [ ] GFA v1.0 (VN:Z:1.0)
 - **Limitations**
  - Sequence IDs in the GFA should be numerical

## Configuration

Often, we only need to read a subset of the GFA data. Therefore, liteseq
depends on a config to determine what graph properties to read from the GFA.
This can be seen in `examples/main.c`.

When both `inc_vtx_labels` and `inc_refs` are false it only reads the most minimal graph possible,
that is, vertex IDs and the edges.

## Building liteseq

Prerequisites:

  - CMake (3.0+ recommended)
  - C compiler (e.g., GCC or Clang)


### Fetch the source code
```
git clone https://github.com/urbanslug/liteseq.git

cd liteseq
```


### Release mode
```
cmake -DCMAKE_BUILD_TYPE=Release  -H. -Bbuild && cmake --build build -- -j 3
```

### Debug mode
```
cmake -DCMAKE_BUILD_TYPE=Debug  -H. -Bbuild && cmake --build build -- -j 3
```


### Building a Specific Target

If you want to compile a specific target (such as examples/main.c), run:

```
cmake -H. -DCMAKE_BUILD_TYPE=Debug -Bbuild && cmake --build build --target liteseq -- -j 8
```

## Usage and Examples

1. Include Headers:

```
#include <gfa.h>
```

2. Set Up Configuration:

```
gfa_config config = {
    .inc_vtx_labels = false,
    .inc_refs = false,
    // ...
};
```
3. Initialize, Parse, and Use:

```
gfa_props *gfa = gfa_new(config);
// ...
// parse, process, etc.
```

4. Cleanup:

```
gfa_free(gfa);
```

See `examples/main.c` for more detailed usage.

## Contributing
Contributions, bug reports, and feature requests are welcome. Please open an issue or a pull request on GitHub.

## License
MIT

Happy parsing!
