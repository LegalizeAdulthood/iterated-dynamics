// SPDX-License-Identifier: GPL-3.0-only
//
// ZeroMQ Distributed Rendering Example - Worker
//
// This example demonstrates how ZeroMQ can be used to create a distributed
// fractal rendering system. This is the worker component that receives
// rendering tasks and returns results.

#include <cmath>
#include <iostream>
#include <string>
#include <zmq.hpp>

// Simple structure to represent a rendering task
struct RenderTask
{
    int tile_x;
    int tile_y;
    int tile_width;
    int tile_height;
    double min_real;
    double max_real;
    double min_imag;
    double max_imag;
    int max_iterations;
};

// Simple Mandelbrot calculation for demonstration
int mandelbrot(double cr, double ci, int max_iter)
{
    double zr = 0.0;
    double zi = 0.0;
    int iter = 0;

    while (iter < max_iter && (zr * zr + zi * zi) < 4.0)
    {
        double temp = zr * zr - zi * zi + cr;
        zi = 2.0 * zr * zi + ci;
        zr = temp;
        ++iter;
    }

    return iter;
}

// Render a tile of the fractal
std::string render_tile(const RenderTask &task)
{
    std::string result;
    result.reserve(task.tile_width * task.tile_height);

    double real_step = (task.max_real - task.min_real) / task.tile_width;
    double imag_step = (task.max_imag - task.min_imag) / task.tile_height;

    for (int y = 0; y < task.tile_height; ++y)
    {
        for (int x = 0; x < task.tile_width; ++x)
        {
            double cr = task.min_real + x * real_step;
            double ci = task.min_imag + y * imag_step;
            int iter = mandelbrot(cr, ci, task.max_iterations);

            // Store iteration count as a byte (simplified)
            char pixel = static_cast<char>(iter % 256);
            result.push_back(pixel);
        }
    }

    return result;
}

int main()
{
    try
    {
        // Initialize ZeroMQ context and socket
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::rep);

        // Connect to the master node
        socket.connect("tcp://localhost:5555");

        std::cout << "Worker started and connected to master at tcp://localhost:5555\n";
        std::cout << "Waiting for rendering tasks...\n";

        while (true)
        {
            // Receive task from master
            zmq::message_t request;
            auto result = socket.recv(request, zmq::recv_flags::none);

            if (!result)
            {
                continue;
            }

            // Parse task (in a real implementation, you'd use proper serialization)
            // For this example, we'll use a simple string protocol
            std::string task_str(static_cast<char *>(request.data()), request.size());

            if (task_str == "STOP")
            {
                std::cout << "Received stop signal. Shutting down...\n";
                zmq::message_t reply(2);
                memcpy(reply.data(), "OK", 2);
                socket.send(reply, zmq::send_flags::none);
                break;
            }

            std::cout << "Received task: " << task_str << "\n";

            // For demonstration, create a simple task
            RenderTask task;
            task.tile_x = 0;
            task.tile_y = 0;
            task.tile_width = 100;
            task.tile_height = 100;
            task.min_real = -2.0;
            task.max_real = 1.0;
            task.min_imag = -1.5;
            task.max_imag = 1.5;
            task.max_iterations = 256;

            // Render the tile
            std::string rendered_data = render_tile(task);

            std::cout << "Rendered tile of " << rendered_data.size() << " pixels\n";

            // Send result back to master
            zmq::message_t reply(rendered_data.size());
            memcpy(reply.data(), rendered_data.data(), rendered_data.size());
            socket.send(reply, zmq::send_flags::none);
        }
    }
    catch (const zmq::error_t &e)
    {
        std::cerr << "ZeroMQ error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
