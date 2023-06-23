#include "ball.hpp"

float force_relation_tab[FLAVOUR_COUNT][FLAVOUR_COUNT] = {
    {1., -0.4, 0.1, 0.6},
    {-0.8, 1.1, -0.5, 0.9},
    {1., -0.5, -0.3, -0.2},
    {0.1, -0.4, 1., .91}};

template <typename T>
T Vector2length(const sf::Vector2<T> &v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

sf::Color HUEtoRGB(float H)
{
    float s = 1;
    float v = 1;
    float C = s * v;
    float X = C * (1 - fabs(fmod(H / 60.0, 2) - 1));
    float m = v - C;
    float r, g, b;

    if (H >= 0 && H < 60)
    {
        r = C, g = X, b = 0;
    }
    else if (H >= 60 && H < 120)
    {
        r = X, g = C, b = 0;
    }
    else if (H >= 120 && H < 180)
    {
        r = 0, g = C, b = X;
    }
    else if (H >= 180 && H < 240)
    {
        r = 0, g = X, b = C;
    }
    else if (H >= 240 && H < 300)
    {
        r = X, g = 0, b = C;
    }
    else
    {
        r = C, g = 0, b = X;
    }

    return sf::Color((r + m) * 255, (g + m) * 255, (b + m) * 255);
}

Ball::Ball() : mass{(float)rand() / RAND_MAX + 2.f}, velocity_3D{0, 0, 0}
{
    this->flavour = (flavour_t)((float)FLAVOUR_COUNT * rand() / ((float)RAND_MAX + 1));
    this->force_relations = force_relation_tab[this->flavour];
    this->velocity = {velocity_3D[0], velocity_3D[1]};
    this->total_velocity = Vector2length(velocity);
    this->position_3D = {100.f * rand() / RAND_MAX, 100.f * rand() / RAND_MAX, 100.f * rand() / RAND_MAX};
    shape.setPosition(position_3D[0], position_3D[1]);
    shape.setFillColor(HUEtoRGB(60 * this->flavour));
    shape.setRadius(this->radius);
    shape.setOrigin(this->radius, this->radius);
}

void Ball::draw(RenderTarget &target, RenderStates state) const
{
    target.draw(this->shape, state);
}

void Ball::update(float dt)
{
    this->shape.move(this->velocity * dt);
    this->total_velocity = Vector2length(this->velocity);
    this->force = {0, 0, 0};
    // shape.setFillColor(HUEtoRGB(this->total_velocity * 60 * (this->force_coef > 0 ? 1 : -1)));
}

float Ball::distance_2D(Ball &sec_ball) const
{
    return Vector2length(this->shape.getPosition() - sec_ball.shape.getPosition());
}

float Ball::distance_3D(Ball &sec_ball) const
{
    float dx = sec_ball.position_3D[0] - this->position_3D[0];
    float dy = sec_ball.position_3D[1] - this->position_3D[1];
    float dz = sec_ball.position_3D[2] - this->position_3D[2];

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}