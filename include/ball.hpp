#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>

using namespace sf;

typedef enum
{
    FLAVOUR_RED,
    FLAVOUR_YELLOW,
    FLAVOUR_GREEN,
    FLAVOUR_BLUE,
    FLAVOUR_COUNT
} flavour_t;

extern float force_relation_tab[FLAVOUR_COUNT][FLAVOUR_COUNT];

class Ball : public sf::Drawable
{
public:
    Ball();
    ~Ball() = default;
    CircleShape shape;
    flavour_t flavour;
    float *force_relations;
    float mass{1.f};
    const float radious{3};
    float total_velocity;
    Vector2f velocity;
    std::vector<int64_t> force = {0, 0};

    void update(float dt);
    float distance(Ball &sec_ball) const;

private:
    void draw(RenderTarget &target, RenderStates state) const override;
};
