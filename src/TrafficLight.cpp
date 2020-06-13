#include "TrafficLight.h"
#include <iostream>
#include <random>

template <typename T> T MessageQueue<T>::receive()
{
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_messages.empty(); });

    auto message = std::move(_messages.back());
    _messages.pop_back();

    return message;
}

template <typename T> void MessageQueue<T>::send(T&& message)
{
    std::lock_guard<std::mutex> uLock(_mutex);

    std::cout << "   Message #" << static_cast<int>(message)
              << " will be added to the queue" << std::endl;
    _messages.push_back(std::move(message));
    _cond.notify_one();
}

template class MessageQueue<TrafficLightPhase>;

TrafficLight::TrafficLight() : _currentPhase{TrafficLightPhase::red}
{
}

void TrafficLight::waitForGreen()
{
    while (true) {

        auto message = _queue.receive();

        if (message == TrafficLightPhase::green) {
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase() const
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

void TrafficLight::cycleThroughPhases()
{
    std::random_device random_device;
    std::mt19937 random_engine(random_device());
    std::uniform_int_distribution<int> distribution(4000, 6000);

    auto startTime = std::chrono::system_clock::now();
    std::chrono::milliseconds cycleDuration{distribution(random_engine)};

    while (true) {
        auto currentTime = std::chrono::system_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                currentTime - startTime) > cycleDuration) {

            _currentPhase == TrafficLightPhase::green
                ? _currentPhase = TrafficLightPhase::red
                : _currentPhase = TrafficLightPhase::green;

            startTime = std::chrono::system_clock::now();

            std::default_random_engine generator;
            cycleDuration =
                std::chrono::milliseconds{distribution(random_engine)};

            _queue.send(std::move(_currentPhase));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
}
