#pragma once

#include <SFML/Graphics.hpp>

struct Ball {
    sf::Vector2f position{};
    sf::Vector2f velocity{};
    float radius = 10.0f;
    bool stuckToPaddle = false;
    float stuckOffsetX = 0.0f;
};

struct Paddle {
    sf::FloatRect rect{};
    float speed = 0.0f;
    float targetWidth = 0.0f;
};
