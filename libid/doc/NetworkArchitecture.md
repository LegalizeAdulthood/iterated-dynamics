# Network Architecture and Distributed Rendering

## Table of Contents
1. [Overview](#overview)
2. [Current Architecture](#current-architecture)
3. [Parameter Communication Patterns](#parameter-communication-patterns)
4. [Network Rendering with ZeroMQ](#network-rendering-with-zeromq)
5. [Multithreading with ZeroMQ](#multithreading-with-zeromq)
6. [Batch and Console Rendering](#batch-and-console-rendering)
7. [Implementation Roadmap](#implementation-roadmap)
8. [Code Examples](#code-examples)

---

## Overview

This document describes the architectural evolution from the current global variable-based parameter system to a network-capable, distributed rendering architecture using ZeroMQ message passing.

### Key Requirements
- Support local multithreaded rendering
- Enable distributed network rendering across multiple machines
- Support batch/console rendering without GUI
- Maintain backward compatibility during migration
- Minimize performance impact on calculation hot paths

### Key Insight
Using ZeroMQ's `inproc://` transport enables **identical code** for both local multithreading and network rendering. The same worker implementation works whether connected via in-process message passing or TCP sockets.

---

## Current Architecture

### How Variables Are Changed in UI Code

The current implementation uses **direct global variable mutation**:

#### 1. Dialog-Based Input (ChoiceBuilder)

From `libid/ui/get_corners.cpp`:

    ChoiceBuilder<11> builder;
    builder.double_number("Center X", x_ctr)
           .double_number("Center Y", y_ctr)
           .double_number("Magnification", magnification);
    
    int result = builder.prompt("Image Coordinates");
    
    // Direct global updates after dialog closes
    g_image_region.m_min.x = builder.read_double_number();
    g_image_region.m_max.x = builder.read_double_number();
    g_use_center_mag = true;

#### 2. Keyboard/Mouse Events

From `libid/ui/zoom.cpp`:

    void resize_box(int steps) {
        double delta_x = steps * 0.036;
        double delta_y = g_zoom_box_height * delta_x / g_zoom_box_width;
        
        // Direct global mutation
        g_zoom_box_width += delta_x;
        g_zoom_box_height += delta_y;
        move_box(delta_width / -2.0, delta_height / -2.0);
    }

#### 3. Global Variable Groups

Based on `libid/GlobalsIndex.md` documentation, there are **~100-300 KB of global state** organized into groups:

**Transform3D Group** (`include/geometry/3d.h`):
- `g_x_rot`, `g_y_rot`, `g_z_rot` - Rotation angles (degrees)
- `g_x_scale`, `g_y_scale` - Scale factors (percent)
- `g_sphere` - Sphere mode flag

**Image Region Group** (`include/engine/ImageRegion.h`):
- `g_image_region.m_min`, `g_image_region.m_max` - Corner coordinates
- `g_image_region.m_3rd` - Third corner (for rotation)

**Calculation Settings** (`include/engine/calcfrac.h`):
- `g_max_iterations` - Maximum iteration count
- `g_magnitude_limit` - Bailout value
- `g_periodicity_check` - Periodicity checking flag

**Render Settings** (`include/geometry/line3d.h`):
- `g_fill_type` - Rendering mode (wireframe, solid, etc.)
- `g_preview` - Preview mode flag
- `g_glasses_type` - Stereoscopic 3D mode

### How Image Computation Uses Variables

#### 1. Direct Global Access

From fractal calculation engines:

    int calc_fract() {
        // Direct reads from globals
        for (int row = 0; row < g_logical_screen.y_dots; row++) {
            for (int col = 0; col < g_logical_screen.x_dots; col++) {
                // Uses g_max_iterations, g_magnitude_limit, etc.
                int color = mandel_per_pixel();
                g_put_color(col, row, color);
            }
        }
    }

#### 2. Hot Paths

Performance-critical per-pixel calculations access globals **millions of times**:
- `g_delta_x`, `g_delta_y` - Pixel increments
- `g_magnitude_limit` - Bailout test
- `g_max_iterations` - Loop termination
- `g_periodicity_check` - Optimization flag

#### 3. Cross-Cutting Dependencies

- `g_fractal_type` - Determines algorithm selection
- `g_fill_type` - Affects rendering strategy
- `g_display_3d` - Switches between 2D/3D modes

### Problems with Current Architecture

- **Tight Coupling**: UI directly modifies 100+ global variables
- **No Validation Layer**: Invalid combinations possible
- **No Change Tracking**: Can't detect what changed or optimize re-computation
- **Testing Nightmare**: Unit tests interfere with each other via global state
- **Thread Safety**: Impossible to parallelize with shared mutable globals
- **Hidden Dependencies**: Hard to track which computation code uses which UI parameters

---

## Parameter Communication Patterns

### Recommended Clean Architecture

#### Pattern 1: Parameter Objects (Lightweight Refactor)

Group related globals into immutable parameter structures:

    // Before: Scattered globals
    extern double g_x_rot;
    extern double g_y_rot;
    extern double g_z_rot;
    extern double g_x_scale;
    extern double g_y_scale;
    
    // After: Cohesive parameter object
    struct Transform3DParams {
        double x_rot{0};
        double y_rot{0};
        double z_rot{0};
        double x_scale{100};
        double y_scale{100};
        
        // Validation
        bool is_valid() const {
            return std::isfinite(x_rot) && std::isfinite(y_rot) &&
                   x_scale > 0 && y_scale > 0;
        }
    };
    
    // Singleton/global instance (transitional)
    extern Transform3DParams g_transform_params;

**Benefits:**
- Groups related parameters
- Enables validation
- Easy to pass as single argument
- Minimal performance impact

#### Pattern 2: Command Pattern (Better Separation)

Decouple UI events from parameter updates:

    // Abstract command
    class ParameterCommand {
    public:
        virtual ~ParameterCommand() = default;
        virtual void execute(FractalEngine& engine) = 0;
        virtual bool invalidates_calculation() const = 0;
    };
    
    // Concrete command
    class Set3DRotationCommand : public ParameterCommand {
        double m_x_rot, m_y_rot, m_z_rot;
    public:
        Set3DRotationCommand(double x, double y, double z)
            : m_x_rot(x), m_y_rot(y), m_z_rot(z) {}
        
        void execute(FractalEngine& engine) override {
            engine.set_3d_rotation(m_x_rot, m_y_rot, m_z_rot);
        }
        
        bool invalidates_calculation() const override {
            return true; // Rotation change requires recalc
        }
    };
    
    // UI layer
    void on_rotation_dialog_ok(double x, double y, double z) {
        auto cmd = std::make_unique<Set3DRotationCommand>(x, y, z);
        g_command_queue.enqueue(std::move(cmd));
    }
    
    // Computation layer processes commands
    void FractalEngine::process_commands() {
        while (auto cmd = m_command_queue.dequeue()) {
            cmd->execute(*this);
            if (cmd->invalidates_calculation()) {
                restart_calculation();
            }
        }
    }

**Benefits:**
- Clear UI to Computation boundary
- Enables undo/redo
- Validation in command constructors
- Can log/debug parameter changes

#### Pattern 3: Observer Pattern (Reactive Updates)

Notify computation code when parameters change:

    // Observable parameter container
    class FractalParameters {
        Transform3DParams m_transform;
        ImageRegion m_region;
        std::vector<IParameterObserver*> m_observers;
        
    public:
        void set_3d_rotation(double x, double y, double z) {
            if (m_transform.x_rot != x || 
                m_transform.y_rot != y ||
                m_transform.z_rot != z) {
                m_transform.x_rot = x;
                m_transform.y_rot = y;
                m_transform.z_rot = z;
                notify_observers(ParameterChange::TRANSFORM_3D);
            }
        }
        
        void add_observer(IParameterObserver* obs) {
            m_observers.push_back(obs);
        }
        
    private:
        void notify_observers(ParameterChange change) {
            for (auto* obs : m_observers) {
                obs->on_parameter_changed(change);
            }
        }
    };
    
    // Computation layer reacts to changes
    class FractalEngine : public IParameterObserver {
        void on_parameter_changed(ParameterChange change) override {
            switch (change) {
                case ParameterChange::TRANSFORM_3D:
                    rebuild_transformation_matrix();
                    invalidate_cache();
                    break;
                case ParameterChange::IMAGE_REGION:
                    restart_calculation();
                    break;
            }
        }
    };

**Benefits:**
- Decoupled notification
- Multiple observers (UI refresh, computation, logging)
- Fine-grained change detection
- Can batch updates

#### Pattern 4: Separation of Concerns (Best Long-Term)

Layer diagram:

    +------------------+
    |   UI Layer       |  - Dialogs (ChoiceBuilder)
    |  (win32/libid)   |  - Mouse/keyboard events
    +--------+---------+  - Validation
             |
             | ParameterCommands / Events
             v
    +------------------+
    | Parameter Store  |  - Immutable parameter objects
    |   (libid/ui)     |  - Change detection
    +--------+---------+  - Persistence (save/load)
             |
             | Read-only access
             v
    +------------------+
    | Computation      |  - Fractal engines
    |   (libid/        |  - 3D rendering
    |    engine/       |  - Color calculation
    |    fractals/)    |
    +------------------+

Implementation:

    // 1. Parameter Store (single source of truth)
    class ParameterStore {
        Transform3DParams m_transform;
        ImageRegionParams m_region;
        CalculationParams m_calculation;
        
    public:
        // Immutable access
        const Transform3DParams& transform() const { return m_transform; }
        
        // Mutation with validation
        Result<void> set_transform(const Transform3DParams& params) {
            if (!params.is_valid()) {
                return Error("Invalid transform parameters");
            }
            m_transform = params;
            return Ok();
        }
        
        // Transactional updates
        Transaction begin_transaction() {
            return Transaction(*this);
        }
    };
    
    // 2. UI Layer uses transactions
    void on_3d_dialog_ok(const DialogValues& values) {
        auto txn = g_params.begin_transaction();
        txn.set_x_rotation(values.x_rot);
        txn.set_y_rotation(values.y_rot);
        txn.set_z_rotation(values.z_rot);
        
        if (auto result = txn.commit(); !result) {
            show_error(result.error());
        } else {
            request_recalculation();
        }
    }
    
    // 3. Computation reads immutable snapshots
    void FractalEngine::render_frame() {
        const auto& transform = m_params.transform();
        const auto& region = m_params.region();
        
        // No risk of parameters changing mid-calculation
        build_transformation_matrix(transform);
        calculate_fractal(region);
    }

---

## Network Rendering with ZeroMQ

### Why Network Communication Changes Everything

| Concern | Local (Current) | Network (ZeroMQ Required) |
|---------|----------------|---------------------------|
| Serialization | Not needed (shared memory) | MANDATORY - must serialize 100+ parameters |
| Validation | Implicit (same process) | EXPLICIT - prevent corrupt/malicious data |
| Versioning | Not needed | REQUIRED - client/server version mismatch |
| Immutability | Nice-to-have | ESSENTIAL - concurrent access across network |
| Performance | Nanoseconds (memory access) | Milliseconds (network latency) |
| Parameter Size | 0 bytes | 5-20 KB |
| Atomicity | Can change parameters mid-calc | MUST snapshot entire state per render job |

### Serializable Parameter Objects (MANDATORY for Network)

    // Serializable parameter struct
    struct Transform3DParams {
        double x_rot{0};
        double y_rot{0};
        double z_rot{0};
        double x_scale{100};
        double y_scale{100};
        
        // Validation for network safety
        bool validate() const {
            return std::isfinite(x_rot) && std::isfinite(y_rot) &&
                   x_scale > 0 && y_scale > 0;
        }
        
        // Serialization (choose ONE format)
        std::vector<uint8_t> serialize() const;
        static Transform3DParams deserialize(const std::vector<uint8_t>& data);
    };

### Serialization Format Choices

#### Option A: Binary Serialization (Fastest)

    // Custom binary protocol (least overhead, hardest to maintain)
    std::vector<uint8_t> Transform3DParams::serialize() const {
        std::vector<uint8_t> buffer(sizeof(Transform3DParams));
        std::memcpy(buffer.data(), this, sizeof(*this));
        return buffer;
    }

**Pros**: Fastest, smallest payload
**Cons**: No version tolerance, endianness issues, not human-readable

#### Option B: Protocol Buffers (Recommended)

    // transform3d.proto
    syntax = "proto3";
    
    message Transform3DParams {
        double x_rot = 1;
        double y_rot = 2;
        double z_rot = 3;
        double x_scale = 4;
        double y_scale = 5;
    }
    
    message RenderJobRequest {
        Transform3DParams transform = 1;
        ImageRegionParams region = 2;
        CalculationParams calc_params = 3;
        ColorParams color_params = 4;
        // ... all parameter groups
    }

**Pros**: Version-tolerant, efficient, cross-language
**Pros**: Built-in validation
**Cons**: Requires protobuf library

#### Option C: JSON (Easiest Debugging)

    #include <nlohmann/json.hpp>
    
    void to_json(json& j, const Transform3DParams& p) {
        j = json{
            {"x_rot", p.x_rot},
            {"y_rot", p.y_rot},
            {"z_rot", p.z_rot},
            {"x_scale", p.x_scale},
            {"y_scale", p.y_scale}
        };
    }
    
    void from_json(const json& j, Transform3DParams& p) {
        p.x_rot = j.at("x_rot").get<double>();
        p.y_rot = j.at("y_rot").get<double>();
        // ... with validation
        if (!p.validate()) throw std::runtime_error("Invalid params");
    }

**Pros**: Human-readable, easy debugging, schema evolution
**Cons**: Larger payloads (2-5x size), slower parsing

**Recommendation**: **Protocol Buffers** for production, **JSON** for development/debugging.

### ZeroMQ Communication Patterns

#### Pattern 1: Request-Reply (Simplest)

CLIENT (UI) - Sends job, waits for result:

    zmq::context_t ctx{1};
    zmq::socket_t client{ctx, zmq::socket_type::req};
    client.connect("tcp://render-server:5555");
    
    RenderJobRequest request = build_request_from_ui();
    auto serialized = request.serialize(); // protobuf or JSON
    
    client.send(zmq::buffer(serialized), zmq::send_flags::none);
    
    zmq::message_t reply;
    client.recv(reply, zmq::recv_flags::none);
    RenderJobResult result = RenderJobResult::deserialize(reply.data());

SERVER (Render Engine) - Receives job, computes, replies:

    zmq::context_t ctx{1};
    zmq::socket_t server{ctx, zmq::socket_type::rep};
    server.bind("tcp://*:5555");
    
    while (true) {
        zmq::message_t request;
        server.recv(request, zmq::recv_flags::none);
        
        auto job = RenderJobRequest::deserialize(request.data());
        if (!job.validate()) {
            server.send(zmq::buffer("ERROR: Invalid parameters"));
            continue;
        }
        
        auto result = compute_fractal(job);
        auto serialized = result.serialize();
        server.send(zmq::buffer(serialized));
    }

#### Pattern 2: Push-Pull (Better for Multiple Workers)

VENTILATOR (UI) - Distributes tiles to workers:

    zmq::socket_t ventilator{ctx, zmq::socket_type::push};
    ventilator.bind("tcp://*:5555");
    
    // Split image into tiles
    for (const auto& tile : split_into_tiles(image_region)) {
        RenderTileJob job = create_tile_job(tile, params);
        ventilator.send(zmq::buffer(job.serialize()));
    }

WORKERS (Render Nodes) - Process tiles:

    zmq::socket_t receiver{ctx, zmq::socket_type::pull};
    receiver.connect("tcp://ventilator:5555");
    zmq::socket_t sender{ctx, zmq::socket_type::push};
    sender.connect("tcp://sink:5556");
    
    while (true) {
        zmq::message_t msg;
        receiver.recv(msg);
        auto job = RenderTileJob::deserialize(msg.data());
        auto result = compute_tile(job);
        sender.send(zmq::buffer(result.serialize()));
    }

SINK (UI) - Collects results:

    zmq::socket_t sink{ctx, zmq::socket_type::pull};
    sink.bind("tcp://*:5556");
    
    while (tiles_remaining > 0) {
        zmq::message_t result;
        sink.recv(result);
        auto tile = RenderTileResult::deserialize(result.data());
        display_tile(tile);
        tiles_remaining--;
    }

### Unified Parameter Package

All parameters must be bundled into a single atomic request:

    struct RenderJobRequest {
        // Fractal Definition
        FractalType fractal_type;
        ImageRegionParams region;         // x_min, x_max, y_min, y_max, etc.
        CalculationParams calc;           // max_iter, bailout, periodicity
        
        // 3D Parameters (if applicable)
        Transform3DParams transform;
        LightingParams lighting;
        StereoParams stereo;
        
        // Color/Output
        ColorParams colors;
        PaletteData palette;              // Full palette data (256*3 bytes)
        
        // Optimization Hints
        TileRegion tile;                  // Which portion of image to render
        uint32_t priority;
        
        // Metadata
        uint64_t job_id;
        uint32_t client_version;
        std::string client_id;
        
        // Methods
        bool validate() const;
        std::vector<uint8_t> serialize_protobuf() const;
        std::string serialize_json() const;
        static RenderJobRequest deserialize(const std::vector<uint8_t>& data);
    };

### Stateless Render Server (CRITICAL)

The render server must be **completely stateless** - every job contains ALL parameters:

    class FractalRenderServer {
        zmq::context_t m_ctx{1};
        zmq::socket_t m_socket;
        
    public:
        FractalRenderServer(const std::string& endpoint) 
            : m_socket(m_ctx, zmq::socket_type::rep) {
            m_socket.bind(endpoint);
        }
        
        void serve() {
            while (true) {
                // Receive job
                zmq::message_t request;
                m_socket.recv(request);
                
                try {
                    // Deserialize
                    auto job = RenderJobRequest::deserialize(
                        {static_cast<uint8_t*>(request.data()), 
                         request.size()});
                    
                    // Validate (CRITICAL for network safety)
                    if (!job.validate()) {
                        send_error("Invalid parameters");
                        continue;
                    }
                    
                    // Create ISOLATED render context (no globals!)
                    FractalEngine engine(job);
                    auto result = engine.render();
                    
                    // Serialize response
                    auto serialized = result.serialize();
                    m_socket.send(zmq::buffer(serialized));
                    
                } catch (const std::exception& e) {
                    send_error(std::string("Render failed: ") + e.what());
                }
            }
        }
    };

**Key Requirements:**
- No global variables accessed
- Each job creates isolated `FractalEngine` instance
- Engine constructed entirely from `RenderJobRequest`
- Thread-safe (can run multiple workers)

### UI Layer Changes

    class NetworkRenderClient {
        zmq::context_t m_ctx{1};
        zmq::socket_t m_socket;
        ParameterStore m_params;          // Local parameter state
        
    public:
        void on_parameter_changed() {
            // Parameters changed in UI, but don't send yet
            m_params.mark_dirty();
        }
        
        void start_render() {
            // Build atomic snapshot of ALL parameters
            RenderJobRequest job = m_params.to_render_job();
            
            // Validate BEFORE sending
            if (!job.validate()) {
                show_error("Invalid parameter combination");
                return;
            }
            
            // Send to server
            auto serialized = job.serialize_protobuf();
            m_socket.send(zmq::buffer(serialized));
            
            // Wait for result (async in production)
            zmq::message_t reply;
            m_socket.recv(reply);
            
            auto result = RenderJobResult::deserialize(reply.data());
            display_image(result.pixels);
        }
    };

### Data Flow Comparison

#### Current (In-Process)

    UI Dialog --> Global Variables --> Fractal Engine (reads globals)
                      |
                      v
                Live updates possible

#### Network (ZeroMQ)

    UI Dialog --> ParameterStore --> RenderJobRequest (snapshot)
                                            |
                                            v
                                    [Serialize to bytes]
                                            |
                                            v
                                    ZeroMQ Socket (network)
                                            |
                                            v
                                    [Deserialize from bytes]
                                            |
                                            v
                               FractalEngine (isolated instance)
                                            |
                                            v
                               RenderJobResult (image pixels)
                                            |
                                            v
                                    [Serialize to bytes]
                                            |
                                            v
                                    ZeroMQ Socket (network)
                                            |
                                            v
                                    [Deserialize from bytes]
                                            |
                                            v
                               UI Display (show image)

---

## Multithreading with ZeroMQ

### Key Insight: ZeroMQ Dramatically Simplifies Multithreaded Rendering

Using ZeroMQ's **`inproc://`** transport enables **identical code** for both network and local multithreading:

    // SAME worker code for network AND local threads!
    void worker_thread(zmq::context_t& ctx, const std::string& endpoint) {
        zmq::socket_t socket(ctx, zmq::socket_type::rep);
        socket.connect(endpoint);  // Works with tcp:// OR inproc://
        
        while (true) {
            zmq::message_t request;
            socket.recv(request);
            
            auto job = RenderJobRequest::deserialize(request.data());
            auto result = compute_fractal(job);  // Thread-safe, no globals!
            
            socket.send(zmq::buffer(result.serialize()));
        }
    }
    
    // CLIENT: Render locally OR over network by changing ONE LINE
    void start_rendering(RenderMode mode) {
        zmq::context_t ctx(1);
        zmq::socket_t client(ctx, zmq::socket_type::req);
        
        if (mode == RenderMode::LOCAL_MULTITHREAD) {
            client.connect("inproc://workers");  // In-process threads
        } else {
            client.connect("tcp://render-server:5555");  // Network
        }
        
        // REST OF CODE IDENTICAL
        auto job = build_render_job();
        client.send(zmq::buffer(job.serialize()));
        // ...
    }

### Advantages Over Traditional Threading

| Concern | Traditional Threads | ZeroMQ inproc:// |
|---------|---------------------|------------------|
| Synchronization | Mutexes, condition variables, locks | None needed - message passing |
| Data Sharing | Shared memory + guards | Serialized messages - no sharing |
| Thread Safety | Must carefully protect globals | Enforced by design |
| Scalability | Hard limit (local cores) | Seamless network scaling |
| Code Reuse | Separate threading code | 100% shared with network code |
| Debugging | Race conditions, deadlocks | Message logs, deterministic |
| Testing | Hard to mock/stub | Easy - swap endpoints |

### Unified Worker Pool (Local + Network)

    class FractalRenderDispatcher {
        zmq::context_t m_ctx;
        std::vector<std::thread> m_local_workers;
        
    public:
        FractalRenderDispatcher(int num_local_threads) : m_ctx(1) {
            // Create ventilator (distributes work)
            zmq::socket_t ventilator(m_ctx, zmq::socket_type::push);
            ventilator.bind("inproc://jobs");
            
            // Create sink (collects results)
            zmq::socket_t sink(m_ctx, zmq::socket_type::pull);
            sink.bind("inproc://results");
            
            // Spawn local worker threads
            for (int i = 0; i < num_local_threads; ++i) {
                m_local_workers.emplace_back([this]() {
                    worker_thread(m_ctx, "inproc://jobs", "inproc://results");
                });
            }
        }
        
        void render_image_tiles(const RenderJobRequest& base_job) {
            zmq::socket_t ventilator(m_ctx, zmq::socket_type::push);
            ventilator.connect("inproc://jobs");
            
            zmq::socket_t sink(m_ctx, zmq::socket_type::pull);
            sink.connect("inproc://results");
            
            // Split image into tiles
            auto tiles = split_into_tiles(base_job);
            
            // Send tiles to workers (local threads OR network nodes)
            for (const auto& tile : tiles) {
                ventilator.send(zmq::buffer(tile.serialize()));
            }
            
            // Collect results
            for (size_t i = 0; i < tiles.size(); ++i) {
                zmq::message_t result;
                sink.recv(result);
                auto tile_result = RenderTileResult::deserialize(result.data());
                display_tile(tile_result);
            }
        }
    };
    
    // SAME worker code for local threads and network nodes
    void worker_thread(zmq::context_t& ctx, 
                       const std::string& job_endpoint,
                       const std::string& result_endpoint) {
        zmq::socket_t job_socket(ctx, zmq::socket_type::pull);
        job_socket.connect(job_endpoint);
        
        zmq::socket_t result_socket(ctx, zmq::socket_type::push);
        result_socket.connect(result_endpoint);
        
        while (true) {
            zmq::message_t job_msg;
            job_socket.recv(job_msg);
            
            auto tile_job = RenderTileJob::deserialize(job_msg.data());
            
            // ISOLATED computation - no globals!
            FractalEngine engine(tile_job.params);
            auto pixels = engine.render_tile(tile_job.tile_region);
            
            RenderTileResult result{tile_job.tile_id, pixels};
            result_socket.send(zmq::buffer(result.serialize()));
        }
    }

### Hybrid Architecture: Local + Network Workers

    +--------------+
    |  UI Thread   |
    |  (Client)    |
    +------+-------+
           | Send jobs
           v
    +-------------------------------+
    |  Job Broker (ROUTER socket)   | <-- Distributes work
    +------+------------------------+
           |
           +---> inproc://worker1  (Local Thread 1)
           +---> inproc://worker2  (Local Thread 2)
           +---> inproc://worker3  (Local Thread 3)
           +---> tcp://192.168.1.10:5555  (Remote Machine 1)
           +---> tcp://192.168.1.11:5555  (Remote Machine 2)
           
    All send results back to:
           
    +-------------------------------+
    | Result Collector (PULL)       |
    +------+------------------------+
           v
      Display Thread

**Benefits:**
- Automatic load balancing - ZeroMQ round-robins to fastest available worker
- Fault tolerance - Workers can crash/restart without affecting others
- Dynamic scaling - Add network nodes at runtime
- No code changes - Local and network workers identical

### Performance Comparison

#### Traditional Threading (Current State)

    // PROBLEM: Shared global state
    std::mutex g_param_mutex;
    extern double g_x_rot, g_y_rot, g_z_rot; // 100+ globals
    
    void render_thread() {
        while (true) {
            std::lock_guard lock(g_param_mutex);
            // Read globals... but they might change!
            double x_rot = g_x_rot;
            // ...
            lock.unlock();
            
            // Compute... but parameters might be stale
            compute_fractal();
        }
    }

**Problems:**
- Lock contention on every parameter read
- Risk of partial updates (e.g., rotation changed mid-calculation)
- Can't distribute to network without rewrite

#### ZeroMQ inproc:// (Proposed)

    void render_thread(zmq::context_t& ctx) {
        zmq::socket_t socket(ctx, zmq::socket_type::pull);
        socket.connect("inproc://jobs");
        
        while (true) {
            zmq::message_t msg;
            socket.recv(msg);  // ZERO locks - ZeroMQ handles synchronization
            
            auto job = RenderTileJob::deserialize(msg.data());
            // ALL parameters in job - atomic snapshot, no globals!
            
            compute_fractal(job.params);
        }
    }

**Advantages:**
- Zero lock contention - each thread gets atomic job
- No race conditions - parameters immutable per job
- Network-ready - change `inproc://` to `tcp://`

### Migration Strategy with ZeroMQ

#### Minimal Changes to Existing Code

    // BEFORE: Current single-threaded code
    void render_fractal() {
        // Reads g_x_rot, g_y_rot, etc. (100+ globals)
        FractalEngine engine;
        engine.calculate();  
    }
    
    // AFTER: Wrap existing code in worker
    void worker_thread(zmq::context_t& ctx) {
        zmq::socket_t socket(ctx, zmq::socket_type::pull);
        socket.connect("inproc://jobs");
        
        while (true) {
            zmq::message_t msg;
            socket.recv(msg);
            
            auto job = RenderJobRequest::deserialize(msg.data());
            
            // TEMPORARILY set globals from job (during migration)
            g_x_rot = job.transform.x_rot;
            g_y_rot = job.transform.y_rot;
            // ... set all other globals
            
            // Call existing code!
            FractalEngine engine;
            engine.calculate();
            
            // Send result back
            // ...
        }
    }

**This allows incremental refactoring:**
1. Get ZeroMQ working with globals (2 weeks)
2. Gradually eliminate globals from `FractalEngine` (ongoing)
3. Eventually pass `job.params` directly to engine

---

## Batch and Console Rendering

### Existing Batch Mode Infrastructure

The codebase **already has comprehensive batch rendering** built-in.

#### Batch Mode Enum (`libid/include/engine/cmdfiles.h`)

    enum class BatchMode {
        FINISH_CALC_BEFORE_SAVE = -1,
        NONE,                              // Interactive mode
        NORMAL,                            // Batch: render & save
        SAVE,                              // Mid-batch: saving now
        BAILOUT_ERROR_NO_SAVE,            // Exit code 2: error, no save
        BAILOUT_INTERRUPTED_TRY_SAVE,     // Exit code 1: interrupted, try save
        BAILOUT_INTERRUPTED_SAVE          // Saving after interrupt
    };

#### Batch Execution Flow (`libid/ui/big_while_loop.cpp`)

    else if (g_init_batch == BatchMode::NONE)  // NOT batch mode
    {
        // Wait for user input
        driver_wait_key_pressed(true);
    }
    else  // BATCH MODE - no user interaction
    {
        if (g_init_batch == BatchMode::NORMAL) {
            context.key = 's';  // Auto-save
            g_init_batch = BatchMode::SAVE;
        }
        else {
            if (g_calc_status != CalcStatus::COMPLETED) {
                g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE;
            }
            goodbye();  // Exit with error code
        }
    }

#### Exit Codes (`libid/ui/goodbye.cpp`)

    int ret = 0;
    if (g_init_batch == BatchMode::BAILOUT_ERROR_NO_SAVE) {
        ret = 2;  // Error occurred
    }
    else if (g_init_batch == BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE) {
        ret = 1;  // Interrupted but saved
    }
    std::exit(ret);  // Shell scripts can check exit code

### Batch Rendering Workflow

    1. Parse command line (cmd_files)
       |
       v
    2. Set g_init_batch = BatchMode::NORMAL
       |
       v
    3. Skip intro screen (if batch mode)
       |
       v
    4. Load parameters from command line
       |
       v
    5. Start fractal calculation (calc_fract)
       |
       v
    6. No user input required - auto-continues
       |
       v
    7. Auto-save when complete (key = 's')
       |
       v
    8. Exit with code 0 (success) or 1/2 (error)

### ZeroMQ + Batch Mode = Perfect Console Renderer

#### Architecture: Console Batch Renderer

    // Console-only batch renderer (no GUI)
    class ConsoleBatchRenderer {
        zmq::context_t m_ctx{1};
        std::vector<std::thread> m_workers;
        
    public:
        int render_from_params(const RenderJobRequest& job) {
            // Set batch mode - NO user interaction
            g_init_batch = BatchMode::NORMAL;
            
            // Set driver to "disk" mode (no display)
            driver_select_disk();
            
            // Spawn local worker threads
            for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
                m_workers.emplace_back([this, &job]() {
                    worker_thread(m_ctx, "inproc://jobs", "inproc://results");
                });
            }
            
            // Send job to workers
            zmq::socket_t sender(m_ctx, zmq::socket_type::push);
            sender.bind("inproc://jobs");
            sender.send(zmq::buffer(job.serialize()));
            
            // Wait for result
            zmq::socket_t receiver(m_ctx, zmq::socket_type::pull);
            receiver.bind("inproc://results");
            zmq::message_t result;
            receiver.recv(result);
            
            // Save to disk automatically
            auto tile_result = RenderTileResult::deserialize(result.data());
            save_image(tile_result.pixels, job.output_filename);
            
            // Return exit code
            return g_init_batch == BatchMode::BAILOUT_ERROR_NO_SAVE ? 2 : 0;
        }
    };

#### Example: Headless Console App

    // New console-only executable
    int main(int argc, char* argv[]) {
        // NO window creation - pure console
        if (argc < 2) {
            std::cerr << "Usage: id-console @params.par\n";
            return 1;
        }
        
        // Parse command-line parameters
        RenderJobRequest job = parse_command_line(argc, argv);
        
        // Validate
        if (!job.validate()) {
            std::cerr << "Invalid parameters\n";
            return 2;
        }
        
        // Render using batch mode
        ConsoleBatchRenderer renderer;
        return renderer.render_from_params(job);
    }

#### Usage Examples

Command-line examples:

    # Single image render (console, no GUI)
    id-console.exe type=mandel maxiter=1000 savename=mandel.gif
    
    # Network render server (console, no GUI)
    id-render-server.exe tcp://*:5555
    
    # Batch process multiple images
    for /L %i in (1,1,100) do (
        id-console.exe @params%i.txt
    )

### Project Structure Updates

#### New Project: `win32\id-console.vcxproj`
- Console-only executable (no WinMain, just main)
- Links against `libid.lib` and `os.lib`
- No GUI dependencies
- Batch mode only

#### New Project: `win32\id-render-server.vcxproj`
- ZeroMQ network render server
- Console-only (runs as service/daemon)
- Accepts jobs via TCP sockets
- Returns rendered images

#### Existing Projects Enhanced
- **`win32\id.vcxproj`** - GUI application (interactive + batch)
- **`libid\libid.vcxproj`** - Core library (already supports batch)

### Key Advantages for Network/Batch Rendering

| Feature | Current GUI App | Console Batch | ZeroMQ Network |
|---------|----------------|---------------|----------------|
| User Interaction | Required | None | None |
| Window | GUI window | Console only | Headless |
| Multithreading | Complex | Simple (inproc://) | Same code |
| Remote Rendering | No | No | Yes (tcp://) |
| Scripting | Limited | Full support | Full support |
| Exit Codes | Yes | Yes (0/1/2) | Yes |
| Automation | Hard | Easy | Easy |

---

## Implementation Roadmap

### Phase 1: Encapsulate Parameters (6-8 weeks)
- Group globals into serializable structs
- Add validation methods
- Implement local serialization (for testing)
- **No ZeroMQ yet** - use in-process message passing

Example parameter groups to create:
- `Transform3DParams` - 3D rotation/scale
- `ImageRegionParams` - Corner coordinates
- `CalculationParams` - Max iterations, bailout
- `ColorParams` - Palette, cycling
- `LightingParams` - Light vector, ambient

### Phase 2: Stateless Engine (4-6 weeks)
- Refactor `FractalEngine` to accept parameter structs
- Eliminate all global variable reads from engine
- Test with parameter snapshots
- Ensure hot paths remain efficient

### Phase 3: Serialization Layer (3-4 weeks)
- Choose format (recommend Protocol Buffers)
- Implement serialize/deserialize for all param structs
- Add version detection
- Write serialization unit tests

### Phase 4: ZeroMQ Integration (2-3 weeks)
- Add ZeroMQ dependency (vcpkg or submodule)
- Implement request-reply pattern
- Test with localhost server
- Create `inproc://` worker pool

### Phase 5: Production Hardening (4-6 weeks)
- Add error handling, retries, timeouts
- Implement tile-based parallel rendering
- Add progress reporting
- Security hardening (authentication, encryption)
- Performance profiling and optimization

**Total Estimate**: 19-27 weeks for production-ready network rendering

### Incremental Migration Path

#### Week 1-2: Single Worker (Prove Architecture)

    // Prove serialization works locally
    zmq::context_t ctx(1);
    
    // Create local worker in SAME process
    std::thread worker([&ctx]() {
        worker_thread(ctx, "inproc://jobs", "inproc://results");
    });
    
    // UI sends jobs
    zmq::socket_t sender(ctx, zmq::socket_type::push);
    sender.bind("inproc://jobs");
    
    RenderJobRequest job = build_job_from_ui();
    sender.send(zmq::buffer(job.serialize()));

#### Week 3: Multi-Threaded (Scale Up)

    // Spawn thread pool - SAME worker code!
    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        std::thread([&ctx]() {
            worker_thread(ctx, "inproc://jobs", "inproc://results");
        }).detach();
    }

#### Week 4: Network Rendering (Distributed)

    // Change ONE line to enable network
    // workers.connect("inproc://jobs");  // Local
    workers.connect("tcp://render-farm:5555");  // Network

**Total**: ~4 weeks for basic distributed rendering vs. 19-27 weeks for traditional approach

---

## Code Examples

### Complete Worker Implementation

    void worker_thread(zmq::context_t& ctx, 
                       const std::string& job_endpoint,
                       const std::string& result_endpoint) {
        zmq::socket_t job_socket(ctx, zmq::socket_type::pull);
        job_socket.connect(job_endpoint);
        
        zmq::socket_t result_socket(ctx, zmq::socket_type::push);
        result_socket.connect(result_endpoint);
        
        while (true) {
            zmq::message_t job_msg;
            auto recv_result = job_socket.recv(job_msg, zmq::recv_flags::none);
            if (!recv_result) {
                break; // Context terminated
            }
            
            try {
                auto tile_job = RenderTileJob::deserialize(
                    static_cast<const uint8_t*>(job_msg.data()), 
                    job_msg.size());
                
                if (!tile_job.params.validate()) {
                    // Send error response
                    continue;
                }
                
                // ISOLATED computation - no globals!
                FractalEngine engine(tile_job.params);
                auto pixels = engine.render_tile(tile_job.tile_region);
                
                RenderTileResult result{
                    tile_job.tile_id, 
                    tile_job.tile_region,
                    std::move(pixels)
                };
                
                auto serialized = result.serialize();
                result_socket.send(zmq::buffer(serialized));
                
            } catch (const std::exception& e) {
                // Log error, send error response
                std::cerr << "Worker error: " << e.what() << "\n";
            }
        }
    }

### Complete Parameter Object

    struct RenderJobRequest {
        // Identification
        uint64_t job_id{0};
        uint32_t client_version{1};
        std::string client_id;
        
        // Fractal Definition
        FractalType fractal_type{FractalType::MANDEL};
        ImageRegionParams region;
        CalculationParams calculation;
        
        // 3D Parameters
        std::optional<Transform3DParams> transform;
        std::optional<LightingParams> lighting;
        std::optional<StereoParams> stereo;
        
        // Color
        ColorParams colors;
        std::array<RGB, 256> palette;
        
        // Optimization
        TileRegion tile;
        uint32_t priority{0};
        
        // Output
        std::filesystem::path output_filename;
        
        // Validation
        bool validate() const {
            if (!region.validate()) return false;
            if (!calculation.validate()) return false;
            if (!colors.validate()) return false;
            if (transform && !transform->validate()) return false;
            if (lighting && !lighting->validate()) return false;
            if (stereo && !stereo->validate()) return false;
            return true;
        }
        
        // Serialization (Protocol Buffers or JSON)
        std::vector<uint8_t> serialize() const;
        static RenderJobRequest deserialize(const uint8_t* data, size_t size);
    };

### Complete Render Dispatcher

    class FractalRenderDispatcher {
        zmq::context_t m_ctx;
        std::vector<std::thread> m_local_workers;
        std::atomic<bool> m_running{true};
        
    public:
        FractalRenderDispatcher(int num_threads) : m_ctx(1) {
            for (int i = 0; i < num_threads; ++i) {
                m_local_workers.emplace_back([this]() {
                    worker_thread(m_ctx, "inproc://jobs", "inproc://results");
                });
            }
        }
        
        ~FractalRenderDispatcher() {
            m_running = false;
            m_ctx.shutdown();
            for (auto& worker : m_local_workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }
        
        std::vector<TileResult> render_image(const RenderJobRequest& job) {
            zmq::socket_t ventilator(m_ctx, zmq::socket_type::push);
            ventilator.bind("inproc://jobs");
            
            zmq::socket_t sink(m_ctx, zmq::socket_type::pull);
            sink.bind("inproc://results");
            
            auto tiles = split_into_tiles(job);
            
            // Send all tiles
            for (const auto& tile : tiles) {
                auto serialized = tile.serialize();
                ventilator.send(zmq::buffer(serialized));
            }
            
            // Collect results
            std::vector<TileResult> results;
            results.reserve(tiles.size());
            
            for (size_t i = 0; i < tiles.size(); ++i) {
                zmq::message_t msg;
                sink.recv(msg);
                
                auto result = TileResult::deserialize(
                    static_cast<const uint8_t*>(msg.data()),
                    msg.size());
                results.push_back(std::move(result));
            }
            
            return results;
        }
    };

---

## Summary

### Current State
- **100+ global variables** control all fractal parameters
- UI directly mutates globals
- Computation reads globals millions of times per frame
- No thread safety, no network capability

### Target State with ZeroMQ
- **Serialized parameter objects** contain all settings
- **Stateless workers** receive jobs, compute, return results
- **Same worker code** for local threads (`inproc://`) and network (`tcp://`)
- **Batch mode support** for headless console rendering
- **Thread-safe by design** - no shared mutable state

### Key Benefits
1. Unified codebase - local and network rendering use identical code
2. Simpler than traditional threading - no mutexes, no condition variables
3. Forces good architecture - serializable parameters, stateless workers
4. Incremental migration - can wrap existing globals initially
5. Production-proven - ZeroMQ powers high-performance financial trading systems
6. Faster time-to-market - 4 weeks for basic implementation vs. 27 weeks traditional

### Recommended Architecture

    UI Thread --> ZeroMQ inproc:// --> Local Worker Threads --> ZeroMQ inproc:// --> UI Thread
                          |
                          v
                (Future: Change to tcp:// for network rendering)

The beauty is that **you get multithreading as a side effect** of building the proper network rendering architecture. The serialization, validation, and stateless design required for network rendering automatically solve the threading problems.

---

## References

- **ZeroMQ Guide**: https://zguide.zeromq.org/
- **Protocol Buffers**: https://protobuf.dev/
- **nlohmann/json**: https://github.com/nlohmann/json
- **Global Variables Documentation**: `libid/GlobalsIndex.md`, `libid/EngineGlobals.md`, `libid/GeometryGlobals.md`
- **Batch Mode Implementation**: `libid/ui/big_while_loop.cpp`, `libid/ui/goodbye.cpp`

---

*Document created: December 12, 2025*
*Last updated: December 12, 2025*
*Status: Design Proposal*
