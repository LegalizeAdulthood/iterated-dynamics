<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# ZeroMQ Examples for Distributed Fractal Rendering

This directory contains example programs demonstrating how ZeroMQ can be used with Iterated Dynamics for distributed fractal rendering.

## Overview

These examples show a simple master-worker architecture where:
- The **master** node divides rendering work into tasks and distributes them
- The **worker** nodes receive tasks, render fractal tiles, and return results
- The master collects results and assembles the final image

## Building the Examples

To build these examples, you'll need:
1. ZeroMQ library (automatically managed by vcpkg)
2. C++17 compatible compiler
3. CMake 3.23 or later

The examples will be built when you build the main Iterated Dynamics project with ZeroMQ support enabled.

## Running the Examples

### Basic Usage

1. Start the master node in one terminal:
   ```bash
   ./master
   ```

2. Start one or more worker nodes in other terminals:
   ```bash
   ./worker
   ```

The workers will connect to the master and begin processing rendering tasks.

### Architecture

```
┌──────────────────────────────────────────┐
│              Master Node                 │
│  - Divides image into tiles              │
│  - Distributes work to workers           │
│  - Collects and assembles results        │
└────────────┬─────────────────────────────┘
             │ tcp://localhost:5555
             │
    ┌────────┴────────┬────────────┐
    │                 │            │
┌───▼────┐       ┌───▼────┐   ┌───▼────┐
│Worker 1│       │Worker 2│   │Worker 3│
│        │       │        │   │        │
│ Render │       │ Render │   │ Render │
│  Tile  │       │  Tile  │   │  Tile  │
└────────┘       └────────┘   └────────┘
```

## Distributed Rendering Concepts

### Work Distribution

The master node divides the fractal image into rectangular tiles. Each tile represents a portion of the complex plane to be rendered. Tiles are distributed to available workers using a request-reply pattern.

### Load Balancing

ZeroMQ's REQ-REP socket pattern provides natural load balancing:
- Workers request tasks when they're idle
- Faster workers automatically get more tasks
- No central coordination overhead

### Scalability

You can easily scale the system by:
- Adding more worker processes on the same machine
- Starting workers on different machines (update connection string)
- Using ZeroMQ's proxy patterns for more complex topologies

## Real-World Applications

These examples demonstrate concepts that could be extended for:

1. **High-Resolution Rendering**: Split large images across multiple machines
2. **Animation Rendering**: Distribute frames across a render farm
3. **Parameter Exploration**: Parallel rendering of multiple parameter variations
4. **Interactive Exploration**: Real-time distributed rendering for zoom sequences

## Advanced Patterns

For production use, consider:
- **Heartbeat mechanism**: Detect and handle worker failures
- **Job persistence**: Store incomplete jobs for recovery
- **Result caching**: Avoid re-rendering identical tiles
- **Dynamic work sizing**: Adjust tile size based on complexity
- **Progress reporting**: Pub-sub pattern for real-time progress updates

## See Also

- [ZeroMQ Integration Documentation](../../home/doc/zeromq-integration.md)
- [ZeroMQ Official Guide](http://zguide.zeromq.org/)
- [Distributed Computing Patterns](https://zeromq.org/socket-api/)

## License

These examples are part of Iterated Dynamics and are licensed under GPL-3.0-only.
