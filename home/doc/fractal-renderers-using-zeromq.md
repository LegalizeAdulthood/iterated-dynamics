<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Fractal Renderers Using ZeroMQ

## Question: Are there any fractal renderers that use ZeroMQ?

**Yes!** As of this release, **Iterated Dynamics** now includes ZeroMQ support, making it a fractal renderer that uses ZeroMQ for distributed and parallel rendering capabilities.

## Iterated Dynamics + ZeroMQ

Iterated Dynamics has been enhanced with ZeroMQ integration to enable:

- **Distributed Rendering**: Split fractal rendering across multiple machines or processes
- **Load Balancing**: Automatically distribute work units to available workers
- **Scalability**: Scale from single-machine to cluster computing
- **Low Latency**: Minimal overhead for inter-process communication

## Why This Integration Matters

Fractal rendering is computationally intensive, especially for:
- High-resolution images (4K, 8K and beyond)
- Deep zoom levels requiring many iterations
- Animation sequences with hundreds or thousands of frames
- Parameter exploration across multiple variations

ZeroMQ enables Iterated Dynamics to distribute these computationally expensive tasks across multiple CPUs, machines, or even cloud resources, dramatically reducing rendering time.

## Example Use Cases

### 1. Multi-Machine Rendering
Distribute a single high-resolution fractal across multiple computers:
```
Computer A (Master): Coordinates rendering, divides work
Computer B (Worker): Renders tiles 1-10
Computer C (Worker): Renders tiles 11-20
Computer D (Worker): Renders tiles 21-30
```

### 2. Animation Rendering
Create fractal animations by distributing frames:
```
Master: Manages 1000-frame animation sequence
Worker 1: Renders frames 1-250
Worker 2: Renders frames 251-500
Worker 3: Renders frames 501-750
Worker 4: Renders frames 751-1000
```

### 3. Parameter Space Exploration
Render multiple parameter variations in parallel:
```
Master: Defines parameter range to explore
Workers: Each renders a different parameter combination
Result: Gallery of fractal variations
```

## Other Fractal Renderers with Distributed Computing

While ZeroMQ specifically is not commonly used in other fractal renderers, several projects use distributed computing:

- **Electric Sheep**: Distributed rendering of animated fractal flames using their own protocol
- **Flam3**: Support for distributed rendering of fractal flames
- **Mandelbulb 3D**: Network rendering capabilities for 3D fractals

Iterated Dynamics' use of ZeroMQ provides several advantages over custom protocols:
- Industry-standard messaging library
- Well-tested and maintained
- Excellent performance characteristics
- Support for multiple messaging patterns
- Cross-platform compatibility
- Language-agnostic (can integrate with Python, C++, Java, etc.)

## Getting Started

To use ZeroMQ with Iterated Dynamics:

1. **See the documentation**: [ZeroMQ Integration Guide](zeromq-integration.md)
2. **Try the examples**: [ZeroMQ Examples](../../examples/zeromq/README.md)
3. **Build with ZeroMQ**: The dependency is automatically included via vcpkg

## Technical Details

- **ZeroMQ Version**: 4.3.5 (via vcpkg)
- **C++ Bindings**: cppzmq 4.10.0 (header-only)
- **Messaging Patterns**: Request-Reply, Push-Pull, Publish-Subscribe
- **Transport**: TCP, IPC, Inproc
- **License**: MPL-2.0 (ZeroMQ), MIT (cppzmq)

## Future Directions

Potential enhancements for distributed rendering:

1. **Command-line tools**: Utilities for managing distributed rendering jobs
2. **Render server daemon**: Persistent rendering service
3. **Web interface**: Browser-based job submission and monitoring
4. **Cloud integration**: Deploy workers to cloud computing platforms
5. **Load balancing**: Advanced work distribution strategies
6. **Fault tolerance**: Automatic retry and recovery mechanisms
7. **Progress monitoring**: Real-time rendering progress visualization

## Conclusion

**Yes, there are fractal renderers that use ZeroMQ** - Iterated Dynamics is now one of them! This integration brings professional-grade distributed computing capabilities to open-source fractal rendering, enabling artists and researchers to create higher-resolution images and animations than ever before.

For more information:
- [ZeroMQ Official Site](https://zeromq.org/)
- [Iterated Dynamics Project](https://github.com/LegalizeAdulthood/iterated-dynamics)
- [Examples and Documentation](../../examples/zeromq/)
