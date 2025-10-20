# Fractal Renderers Using ZeroMQ

This document provides information about fractal rendering software that uses ZeroMQ for distributed computing and network communication.

## What is ZeroMQ?

ZeroMQ (also known as ØMQ, 0MQ, or zmq) is a high-performance asynchronous messaging library aimed at use in distributed or concurrent applications. It provides a message queue, but unlike message-oriented middleware, a ZeroMQ system can run without a dedicated message broker.

## ZeroMQ in Fractal Rendering

Fractal rendering is computationally intensive and can benefit significantly from distributed computing. ZeroMQ's lightweight, high-performance messaging makes it suitable for:

- **Distributed Rendering**: Splitting fractal computation across multiple machines
- **Real-time Collaboration**: Multiple users exploring the same fractal space
- **Work Distribution**: Load balancing fractal tile rendering across worker nodes
- **Result Aggregation**: Collecting computed fractal tiles from workers

## Known Fractal Renderers Using ZeroMQ

### Mandelbulber

[Mandelbulber](http://www.mandelbulber.com/) is a 3D fractal rendering application that supports distributed computing through its NetRender system. While the exact implementation details vary by version, Mandelbulber has capabilities for network-based distributed rendering that can utilize ZeroMQ or similar messaging systems for coordinating render jobs across multiple computers.

### Custom/Research Projects

Many researchers and hobbyists have created custom fractal renderers using ZeroMQ for distributed computing. These are typically:

- Research projects exploring parallel fractal generation algorithms
- Educational implementations demonstrating distributed computing concepts
- Performance benchmarking tools for testing ZeroMQ in compute-intensive tasks

## Potential Use in Iterated Dynamics

Iterated Dynamics could potentially leverage ZeroMQ for distributed rendering features, particularly for:

1. **Network Rendering**: Distribute fractal computation across multiple machines on a local network
2. **Cloud Computing Integration**: Coordinate rendering with cloud-based compute resources
3. **Interactive Collaboration**: Allow multiple users to explore fractal space together
4. **Render Farm Support**: Professional-grade distributed rendering for high-resolution outputs

See the [ToDo.md](ToDo.md) file for more information about potential distributed computing features.

## Technical Considerations

When implementing ZeroMQ in a fractal renderer:

- **Work Distribution Strategy**: Decide how to split the fractal space (tiles, scanlines, adaptive regions)
- **Result Collection**: Efficiently gather and composite results from workers
- **Fault Tolerance**: Handle worker failures and network interruptions
- **Progress Reporting**: Aggregate progress information from distributed workers
- **Parameter Synchronization**: Ensure all workers use consistent fractal parameters

## References

- [ZeroMQ Official Website](https://zeromq.org/)
- [ZeroMQ Guide](https://zguide.zeromq.org/)
- [Mandelbulber Documentation](http://www.mandelbulber.com/manual/)

## Related Documentation

- [ToDo.md](ToDo.md) - See "Distributed/collaborative computing of fractals over the network"

---

Last updated: October 2025
