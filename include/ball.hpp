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
    float mass;
    const float radius{1 * mass};
    float total_velocity;
    Vector2f velocity;
    std::vector<float> velocity_3D = {0, 0, 0};
    std::vector<float> position_3D{3};
    std::vector<int64_t> force = {0, 0, 0};

    void update(float dt);
    float distance_2D(Ball &sec_ball) const;
    float distance_3D(Ball &sec_ball) const;

private:
    void draw(RenderTarget &target, RenderStates state) const override;
};
