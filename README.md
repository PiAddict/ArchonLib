# Archon

Archon is a small (and relatively opinionated) ECS library for games (or whatever else you might want to do with it).

The idea (should I ever stick with it long enough to bring it anywhere) is to build a library that I would want to use for my own games projects, that is fast, modern, and most importantly easy to use.

This project is heavily WIP, so the API and internal implementation are expected to change.

The goals are:

- As much compile-time checking as possible (I hate it when the game crashes due to something that could never have worked)
- Fast, archetype backed entity storage with non-structural Tagging support.
- Built with games performance in mind (and multi-threading friendly)
- Avoid exception-based control flow in normal ECS usage.

## Building and usage

Archon requires C++20 and is currently built as a static library.

```cmake
add_subdirectory(path/to/Archon)
target_link_libraries(MyGame PRIVATE Archon::Archon)
```

For most use cases, you should be good enough just to include the main heaader and go from there. There are currently no other docs, so good luck haha.

```cpp
#include <Archon/Archon.h>
```

## License

Archon is licensed under the MIT License. See [LICENSE](LICENSE).
