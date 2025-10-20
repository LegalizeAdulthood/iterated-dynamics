<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# ZeroMQ Integration

## Overview

Iterated Dynamics now includes support for ZeroMQ, a high-performance asynchronous messaging library. This integration enables distributed and parallel fractal rendering capabilities.

## What is ZeroMQ?

ZeroMQ (also spelled ØMQ or 0MQ) is a high-performance asynchronous messaging library aimed at use in distributed or concurrent applications. It provides a message queue, but unlike message-oriented middleware, a ZeroMQ system can run without a dedicated message broker.

## Why ZeroMQ for Fractal Rendering?

Fractal rendering is computationally intensive and can benefit significantly from distributed computing. ZeroMQ provides:

1. **Distributed Rendering**: Split fractal rendering across multiple machines or processes
2. **Load Balancing**: Automatically distribute work units to available workers
3. **Scalability**: Easily scale from single-machine to cluster computing
4. **Low Latency**: Minimal overhead for inter-process communication
5. **Flexibility**: Support for various messaging patterns (request-reply, publish-subscribe, push-pull)

## Use Cases

### 1. Distributed Rendering
Split a large fractal image into tiles and distribute the rendering across multiple worker nodes:
- Master node divides the image into tiles
- Worker nodes request tiles and render them
- Master node assembles the final image

### 2. Real-time Parameter Updates
Use publish-subscribe pattern to broadcast parameter changes to multiple rendering instances in real-time.

### 3. Render Farm
Create a render farm where multiple machines collaborate to render high-resolution fractals or animation sequences.

## Implementation

ZeroMQ has been added as a dependency in `vcpkg.json` and is available for integration with the Iterated Dynamics fractal renderer.

### Example Architecture

```
                    ┌─────────────┐
                    │   Master    │
                    │  (Divider)  │
                    └──────┬──────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
         ┌────▼───┐   ┌────▼───┐   ┌────▼───┐
         │Worker 1│   │Worker 2│   │Worker 3│
         │(Render)│   │(Render)│   │(Render)│
         └────┬───┘   └────┬───┘   └────┬───┘
              │            │            │
              └────────────┼────────────┘
                           │
                    ┌──────▼──────┐
                    │   Master    │
                    │ (Collector) │
                    └─────────────┘
```

## Future Enhancements

Potential future enhancements could include:

1. **Command-line tools**: Utilities for distributed rendering
2. **Render server**: A dedicated rendering server accepting jobs via ZeroMQ
3. **Client library**: API for submitting rendering jobs programmatically
4. **Job queue**: Persistent job queue for batch processing
5. **Progress monitoring**: Real-time progress updates via pub-sub

## References

- [ZeroMQ Official Documentation](https://zeromq.org/)
- [ZeroMQ Guide](http://zguide.zeromq.org/)
- [Distributed Computing Patterns with ZeroMQ](https://zeromq.org/socket-api/)

## Related Projects

Other fractal renderers and distributed rendering systems:
- **Flam3**: Uses distributed rendering for fractal flames
- **Electric Sheep**: Distributed rendering of animated fractal flames
- **Mandelbulb 3D**: Supports network rendering

This integration makes Iterated Dynamics one of the fractal renderers that uses ZeroMQ for distributed computing capabilities.
