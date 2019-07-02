
#include <csignal>
#include <cmath>

#include <iostream>
#include <random>
#include <utility>
#include <chrono>
#include <algorithm>
#include <thread>


namespace
{


volatile auto sensorEmitting = true;


void sigint(int signo)
{
    if (signo != SIGINT)
    {
        return;
    }

    sensorEmitting = false;
}


template <typename iterator, typename callback>
void pairwise_for_each(iterator b1, iterator e1, iterator b2, iterator e2, callback c)
{
    for (auto it1 = b1, it2 = b2; it1 != e1 && it2 != e2; ++it1, ++it2)
    {
        c(*it1, *it2);
    }
}

} // namespace


int main(int argc, char * argv[])
{
    using namespace std::chrono_literals;

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    const auto numSensors = static_cast<std::size_t>(argc > 1 ? std::atoi(argv[1]) : 1);
    const auto sleepTime = argc > 2 ? std::chrono::microseconds(std::atoi(argv[2])) : 10000us;

    if (signal(SIGINT, sigint) == SIG_ERR)
    {
        std::cerr << "Couldn't register SIGINT handler" << std::endl;
    }

    std::mt19937_64 generator;
    std::normal_distribution<double> distribution(100.0,5.0);

    const auto startTimestamp = std::chrono::high_resolution_clock::now();

    auto currentValues = std::vector<double>(numSensors);
    auto nextValues = std::vector<double>(numSensors);

    pairwise_for_each(currentValues.begin(), currentValues.end(), nextValues.begin(), nextValues.end(), [&](double & currentValue, double & nextValue) {
        currentValue = distribution(generator);
        nextValue = distribution(generator);
    });

    auto t = 0.0;
    while (sensorEmitting)
    {
        const auto currentTimestamp = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTimestamp - startTimestamp).count();

        std::cout << duration;

        pairwise_for_each(currentValues.begin(), currentValues.end(), nextValues.begin(), nextValues.end(), [&](double & currentValue, double & nextValue) {
            const auto value = t * nextValue + (1.0 - t) * currentValue;

            std::cout << ';' << value;
        });

        std::cout << '\n';

        t += 0.1;

        if (t > 1.0)
        {
            t -= 1.0;

            pairwise_for_each(currentValues.begin(), currentValues.end(), nextValues.begin(), nextValues.end(), [&](double & currentValue, double & nextValue) {
                currentValue = std::exchange(nextValue, distribution(generator));
            });
        }

        std::cout.flush();

        std::this_thread::sleep_for(sleepTime);
    }

    return 0;
}
