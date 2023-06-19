#include "ball.hpp"

float force_relation_tab[FLAVOUR_COUNT][FLAVOUR_COUNT] = {
    {1., 0.1, 0.4, 0.6},
    {-1., 1.1, -0.5, 0.5},
    {1., -0.5, 1.8, 0.2},
    {1.4, 2., 2.5, -2.01}};

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

Ball::Ball() : mass{(float)0. * rand() / RAND_MAX + 1.f}, velocity{0, 0}
{
    this->flavour = (flavour_t)((float)FLAVOUR_COUNT * rand() / ((float)RAND_MAX + 1));
    this->force_relations = force_relation_tab[this->flavour];
    this->total_velocity = Vector2length(this->velocity);
    shape.setPosition(100 * rand() / RAND_MAX, 100 * rand() / RAND_MAX);
    shape.setFillColor(HUEtoRGB(60 * this->flavour));
    shape.setRadius(this->radious);
    shape.setOrigin(this->radious, this->radious);
}

void Ball::draw(RenderTarget &target, RenderStates state) const
{
    target.draw(this->shape, state);
}

void Ball::update(float dt)
{
    this->shape.move(this->velocity * dt);
    this->total_velocity = Vector2length(this->velocity);
    // shape.setFillColor(HUEtoRGB(this->total_velocity * 60 * (this->force_coef > 0 ? 1 : -1)));
}

float Ball::distance(Ball &sec_ball) const
{
    return Vector2length(this->shape.getPosition() - sec_ball.shape.getPosition());
}