// SPDX-License-Identifier: GPL-3.0-only
//
// ZeroMQ Distributed Rendering Example - Master
//
// This example demonstrates how ZeroMQ can be used to create a distributed
// fractal rendering system. This is the master component that distributes
// rendering tasks to workers and collects results.

#include <zmq.hpp>
#include <string>
#include <iostream>
#include <vector>

int main()
{
    try
    {
        // Initialize ZeroMQ context and socket
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::rep);
        
        // Bind to a port for workers to connect
        socket.bind("tcp://*:5555");
        
        std::cout << "Master node started on tcp://*:5555\n";
        std::cout << "Waiting for workers to connect...\n";
        std::cout << "Press Ctrl+C to stop\n\n";
        
        // In a real implementation, you would:
        // 1. Divide the fractal image into tiles
        // 2. Create a work queue
        // 3. Distribute tiles to workers as they become available
        // 4. Collect rendered tiles
        // 5. Assemble the final image
        
        int tasks_sent = 0;
        int tasks_completed = 0;
        const int total_tasks = 10;
        
        while (tasks_completed < total_tasks)
        {
            // Wait for a worker to request work
            zmq::message_t request;
            auto result = socket.recv(request, zmq::recv_flags::none);
            
            if (!result)
            {
                continue;
            }
            
            // In this simple example, any message is treated as a work request
            if (tasks_sent < total_tasks)
            {
                // Send a task to the worker
                std::string task = "TASK_" + std::to_string(tasks_sent);
                zmq::message_t reply(task.size());
                memcpy(reply.data(), task.data(), task.size());
                socket.send(reply, zmq::send_flags::none);
                
                std::cout << "Sent " << task << " to worker\n";
                tasks_sent++;
            }
            else
            {
                // No more tasks, send stop signal
                zmq::message_t reply(4);
                memcpy(reply.data(), "STOP", 4);
                socket.send(reply, zmq::send_flags::none);
                
                std::cout << "Sent STOP signal to worker\n";
            }
            
            // Collect the result
            zmq::message_t result_msg;
            auto recv_result = socket.recv(result_msg, zmq::recv_flags::none);
            
            if (recv_result)
            {
                std::cout << "Received result of " << result_msg.size() << " bytes\n";
                tasks_completed++;
            }
        }
        
        std::cout << "\nAll tasks completed!\n";
        std::cout << "Tasks sent: " << tasks_sent << "\n";
        std::cout << "Tasks completed: " << tasks_completed << "\n";
    }
    catch (const zmq::error_t& e)
    {
        std::cerr << "ZeroMQ error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
