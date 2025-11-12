/**
 * @file TimingExample.cpp
 * @brief Example usage of the cross-platform timing utilities
 */

#include "Utils/Timing.h"
#include <iostream>

void example_basic_delay()
{
    std::cout << "Starting 1 second delay..." << std::endl;
    utils::delay_ms(1000);
    std::cout << "Done!" << std::endl;
    
    std::cout << "Starting 500 microsecond delay..." << std::endl;
    utils::delay_us(500);
    std::cout << "Done!" << std::endl;
}

void example_timeout()
{
    std::cout << "Waiting for 2 seconds with timeout..." << std::endl;
    
    uint32_t start = utils::get_tick_ms();
    uint32_t timeout = 2000; // 2 seconds
    
    while (!utils::has_timeout(start, timeout))
    {
        // Do some work here
        utils::delay_ms(100);
        std::cout << "Elapsed: " << utils::elapsed_ms(start) << " ms" << std::endl;
    }
    
    std::cout << "Timeout occurred!" << std::endl;
}

void example_measuring_time()
{
    std::cout << "Measuring execution time..." << std::endl;
    
    uint32_t start = utils::get_tick_ms();
    
    // Simulate some work
    for (int i = 0; i < 5; i++)
    {
        utils::delay_ms(100);
    }
    
    uint32_t elapsed = utils::elapsed_ms(start);
    std::cout << "Operation took: " << elapsed << " ms" << std::endl;
}

int main()
{
    std::cout << "=== Cross-Platform Timing Examples ===" << std::endl << std::endl;
    
    example_basic_delay();
    std::cout << std::endl;
    
    example_timeout();
    std::cout << std::endl;
    
    example_measuring_time();
    
    return 0;
}
