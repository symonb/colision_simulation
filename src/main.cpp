#include "benchmark/benchmark.h"
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include <time.h>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <opencv2/core/core.hpp>       // Basic OpenCV structures (cv::Mat)
#include <opencv2/highgui/highgui.hpp> // Video write
#include <opencv2/opencv.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <tbb/parallel_for.h>
#include <tbb/global_control.h>
#include <tbb/tbb.h>
#include <algorithm>
#include "ball.hpp"

class SFML_window
{
public:
    const int wall_thickness = 30;
    const int window_x = 1476;
    const int window_y = 804;
    const int window_z = 1000;
    const int frames = 10000;
    const float drag_coef = .8f;
    const float force_zero_coef = 5.f;
    const float dt = 0.1;
    const double accuracy = 10E6;
    sf::RenderWindow window;
    sf::RectangleShape box;
    sf::Texture texture;
    sf::RectangleShape loading_bar_out;
    sf::RectangleShape loading_bar_in;

    SFML_window() : window{sf::VideoMode(window_x + 2 * wall_thickness, window_y + 2 * wall_thickness), "COLISION SIMULATOR", sf::Style::Close | sf::Style::Titlebar}
    {
        srand(NULL);
        // texture object for saving rendered images:
        texture.create(window.getSize().x, window.getSize().y);

        // bounding box
        box.setSize(sf::Vector2f{(float)window_x, (float)window_y});
        box.setOrigin(window_x / 2, window_y / 2);
        box.setPosition(window_x / 2 + wall_thickness, window_y / 2 + wall_thickness);
        box.setFillColor(sf::Color::Transparent);
        box.setOutlineColor(sf::Color::White);
        box.setOutlineThickness(wall_thickness);

        // loading bar:
        loading_bar_out.setSize(sf::Vector2f{(float)window_x, (float)wall_thickness / 2});
        loading_bar_out.setOrigin(window_x / 2, wall_thickness / 4);
        loading_bar_out.setPosition(window_x / 2 + wall_thickness, window_y + wall_thickness * 1.5);
        loading_bar_out.setFillColor(sf::Color::Transparent);
        loading_bar_out.setOutlineColor(sf::Color::Cyan);
        loading_bar_out.setOutlineThickness(3);

        loading_bar_in.setSize(sf::Vector2f{0, (float)wall_thickness / 2});
        loading_bar_in.setOrigin(0, wall_thickness / 4);
        loading_bar_in.setPosition(wall_thickness, window_y + wall_thickness * 1.5);
        loading_bar_in.setFillColor(sf::Color::Cyan);
    };

    void draw_background()
    {
        window.clear();
        window.draw(box);
        window.draw(loading_bar_out);
        window.draw(loading_bar_in);
    }

private:
};

void balls_initialization(std::vector<Ball> &all_balls, SFML_window &window)
{
    for (auto ball = all_balls.begin(); ball != all_balls.end(); ball++)
    {
        // initiate position:
        ball->position_3D[0] = (float)rand() / RAND_MAX * (window.window_x - 2 * ball->radius) + window.wall_thickness + ball->radius;
        ball->position_3D[1] = (float)rand() / RAND_MAX * (window.window_y - 2 * ball->radius) + window.wall_thickness + ball->radius;
        ball->position_3D[2] = (float)rand() / RAND_MAX * (window.window_z - 2 * ball->radius) + ball->radius;
        // check collisions:
        if (ball != all_balls.begin())
        {
            for (auto ball_comp = all_balls.begin(); ball_comp != ball; ball_comp++)
            {
                if (ball->distance_3D(*ball_comp) <= (ball->radius + ball_comp->radius))
                {
                    // choose new position:
                    ball->position_3D[0] = (float)rand() / RAND_MAX * (window.window_x - 2 * ball->radius) + window.wall_thickness + ball->radius;
                    ball->position_3D[1] = (float)rand() / RAND_MAX * (window.window_y - 2 * ball->radius) + window.wall_thickness + ball->radius;
                    ball->position_3D[2] = (float)rand() / RAND_MAX * (window.window_z - 2 * ball->radius) + ball->radius;

                    ball_comp = all_balls.begin();
                }
            }
        }
        ball->shape.setPosition(ball->position_3D[0], ball->position_3D[1]);
    }
};

void generate_video(benchmark::State &state)
{
    const int frames = state.range(0);
    const float fps = state.range(1);

    std::vector<cv::Mat> images(frames, cv::Mat{});

    std::string path = "./images/img_";
    std::cout << "debug1 " << std::endl;

    for (int counter = 0; counter < frames; ++counter)
    {
        images[counter] = (cv::imread(path + std::to_string(counter) + ".png", cv::IMREAD_COLOR));
        std::cout << "image " << counter << " loaded" << std::endl;
    }
    std::cout << "debug2 " << std::endl;

    cv::VideoWriter outputVideo("./video_3.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(images[0].size()), true);

    if (!outputVideo.isOpened())
    {
        std::cout << "Could not open the output video for write: " << std::endl;
    }
    std::cout << "debug3 " << std::endl;

    for (auto img = images.begin(); img != images.end(); img++)
    {
        outputVideo.write(*img);
    }
};

void sim(benchmark::State &state)
{
    const int param = state.range(0);
    const bool video_generate = state.range(1) != 0 ? true : false;

    SFML_window window;

    // create particles:
    std::vector<Ball> all_balls(param);
    balls_initialization(all_balls, window);

    for (auto _ : state)
    {

        // render a defined number of frames and close the simulation:
        for (int i = 0; i < window.frames; i++)
        {
            sf::Event event;
            while (window.window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.window.close();
            }

            for (auto ball = all_balls.begin(); ball != all_balls.end(); ball++)
            {
                // check collision with walls:
                if (ball->shape.getPosition().x + ball->radius >= window.window_x + window.wall_thickness)
                {
                    ball->velocity.x = -1 * fabs(ball->velocity.x);
                }
                else if (ball->shape.getPosition().x - ball->radius <= window.wall_thickness)
                {
                    ball->velocity.x = fabs(ball->velocity.x);
                }
                if (ball->shape.getPosition().y + ball->radius >= window.window_y + window.wall_thickness)
                {
                    ball->velocity.y = -1 * fabs(ball->velocity.y);
                }
                else if (ball->shape.getPosition().y - ball->radius <= window.wall_thickness)
                {
                    ball->velocity.y = fabs(ball->velocity.y);
                }

                // update interactions with balls:
                for (auto ball_comp = all_balls.begin(); ball_comp != all_balls.end(); ball_comp++)
                {
                    if (ball_comp != ball)
                    {
                        // distanse (multiplication of sum of radiuses):
                        float dist = ball->distance_2D(*ball_comp);
                        float dist_rel = dist / (ball->radius + ball_comp->radius) / window.force_zero_coef;
                        if (dist_rel > 0.1)
                        {
                            sf::Vector2f force_change = (ball_comp->shape.getPosition() - ball->shape.getPosition()) / dist * (float)(pow(dist_rel, 2) - 1.0) * (float)exp(-0.25 * pow(dist_rel, 2));

                            // before:

                            // float f_value = ball->force_relations[ball_comp->flavour] * ball_comp->mass * (pow(dist, 2) - 1.0) * exp(-0.25 * pow(dist, 2));
                            // ball->force += (ball_comp->shape.getPosition() - ball->shape.getPosition()) / dist * f_value;

                            // after:
                            ball->force[0] += (int64_t)(window.accuracy * force_change.x * ball_comp->mass * ball->force_relations[ball_comp->flavour]);
                            ball->force[1] += (int64_t)(window.accuracy * force_change.y * ball_comp->mass * ball->force_relations[ball_comp->flavour]);
                        }
                    }
                }
                // update velocity of the ball delta V = F/M (F with drag force proportional to velocity):
                ball->velocity += (sf::Vector2f((float)ball->force[0] / window.accuracy, (float)ball->force[1] / window.accuracy) - (ball->velocity / ball->mass * window.drag_coef)) * window.dt;
            }

            window.loading_bar_in.setSize(sf::Vector2f{(float)window.window_x * i / window.frames, (float)window.wall_thickness / 2});

            // draw frame:
            window.draw_background();

            for (auto ball = all_balls.begin(); ball != all_balls.end(); ball++)
            {
                ball->update(window.dt);
                window.window.draw(*ball);
            }
            if (video_generate)
            {
                window.texture.update(window.window);
                window.texture.copyToImage().saveToFile("./images/img_" + std::to_string(i) + ".png");
            }

            window.window.display();
        }
    }
    window.window.close();
    std::cout.precision(16);
    float sum = 0;
    for (auto it = all_balls.begin(); it != all_balls.end(); ++it)
    {
        sum += it->total_velocity;
    }
    // std::cout << "\t" << sum << std::endl;
}

void sim_faster(benchmark::State &state)
{
    const int param = state.range(0);
    const bool video_generate = state.range(1) != 0 ? true : false;

    SFML_window window;

    // create particles:
    std::vector<Ball> all_balls(param);
    balls_initialization(all_balls, window);

    for (auto _ : state)
    {
        // render a defined number of frames and close the simulation:
        for (int frame_nr = 0; frame_nr < window.frames; ++frame_nr)
        {
            sf::Event event;
            while (window.window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.window.close();
            }

            for (auto ball = all_balls.begin(); ball != all_balls.end(); ++ball)
            {
                // check collision with walls:
                if (ball->shape.getPosition().x + ball->radius >= window.window_x + window.wall_thickness)
                {
                    ball->velocity.x = -1 * fabs(ball->velocity.x);
                }
                else if (ball->shape.getPosition().x - ball->radius <= window.wall_thickness)
                {
                    ball->velocity.x = fabs(ball->velocity.x);
                }
                if (ball->shape.getPosition().y + ball->radius >= window.window_y + window.wall_thickness)
                {
                    ball->velocity.y = -1 * fabs(ball->velocity.y);
                }
                else if (ball->shape.getPosition().y - ball->radius <= window.wall_thickness)
                {
                    ball->velocity.y = fabs(ball->velocity.y);
                }
                // update interactions with balls:
                for (auto ball_comp = std::next(ball); ball_comp != all_balls.end(); ++ball_comp)
                {
                    // distanse (multiplication of sum of radiuses):
                    float dist = ball->distance_2D(*ball_comp);
                    float dist_rel = dist / (ball->radius + ball_comp->radius) / window.force_zero_coef;
                    if (dist_rel > 0.1)
                    {
                        sf::Vector2f force_change = (ball_comp->shape.getPosition() - ball->shape.getPosition()) / dist * (float)(pow(dist_rel, 2) - 1.0) * (float)exp(-0.25 * pow(dist_rel, 2));

                        ball->force[0] += (int64_t)(window.accuracy * force_change.x * ball_comp->mass * ball->force_relations[ball_comp->flavour]);
                        ball->force[1] += (int64_t)(window.accuracy * force_change.y * ball_comp->mass * ball->force_relations[ball_comp->flavour]);

                        ball_comp->force[0] -= (int64_t)(window.accuracy * force_change.x * ball->mass * ball_comp->force_relations[ball->flavour]);
                        ball_comp->force[1] -= (int64_t)(window.accuracy * force_change.y * ball->mass * ball_comp->force_relations[ball->flavour]);
                    }
                }

                // update velocity of the ball delta V = F/M (F with drag force proportional to velocity):https://www.geeksforgeeks.org/random-access-iterators-in-cpp/?ref=ml_lbp
                ball->velocity += (sf::Vector2f((float)ball->force[0] / window.accuracy, (float)ball->force[1] / window.accuracy) - (ball->velocity / ball->mass * window.drag_coef)) * window.dt;
            }

            window.loading_bar_in.setSize(sf::Vector2f{(float)window.window_x * frame_nr / window.frames, (float)window.wall_thickness / 2});

            // draw frame:
            window.draw_background();

            for (auto ball = all_balls.begin(); ball != all_balls.end(); ball++)
            {
                ball->update(window.dt);
                window.window.draw(*ball);
            }
            window.window.display();

            if (video_generate)
            {
                window.texture.update(window.window);
                window.texture.copyToImage().saveToFile("./images/img_" + std::to_string(frame_nr) + ".png");
            }
        }
    }

    window.window.close();
    std::cout.precision(16);
    float sum = 0;
    for (auto it = all_balls.begin(); it != all_balls.end(); ++it)
    {
        sum += it->total_velocity;
    }
    // std::cout << "\t" << sum << std::endl;
}

void sim_TBB(benchmark::State &state)
{
    const int param = state.range(0);
    const bool video_generate = state.range(1) != 0 ? true : false;
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, state.range(2));

    SFML_window window;

    // create particles:
    std::vector<Ball> all_balls(param);
    balls_initialization(all_balls, window);

    for (auto _ : state)
    {
        tbb::spin_mutex m;

        // render a defined number of frames and close the simulation:
        for (int frame_nr = 0; frame_nr < window.frames; frame_nr++)
        {
            sf::Event event;
            while (window.window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.window.close();
            }
            tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r)
                              {
                for (int i = r.begin(); i < r.end(); ++i)
                {

                    // check collision with walls:
                    if (all_balls[i].shape.getPosition().x + all_balls[i].radius >= window.window_x + window.wall_thickness)
                    {
                        all_balls[i].velocity.x = -1 * fabs(all_balls[i].velocity.x);
                    }
                    else if (all_balls[i].shape.getPosition().x - all_balls[i].radius <= window.wall_thickness)
                    {
                        all_balls[i].velocity.x = fabs(all_balls[i].velocity.x);
                    }
                    if (all_balls[i].shape.getPosition().y + all_balls[i].radius >= window.window_y + window.wall_thickness)
                    {
                        all_balls[i].velocity.y = -1 * fabs(all_balls[i].velocity.y);
                    }
                    else if (all_balls[i].shape.getPosition().y - all_balls[i].radius <= window.wall_thickness)
                    {
                        all_balls[i].velocity.y = fabs(all_balls[i].velocity.y);
                    }

                    // update interactions with balls:
                    for(int j =0; j<all_balls.size();++j)
                    {                    
                        // distanse and relative disanse (multiplication of sum of radiuses):
                        float dist = all_balls[i].distance_2D(all_balls[j]);
                        float dist_rel = dist/(all_balls[i].radius + all_balls[j].radius)/window.force_zero_coef;
                        if (dist_rel > 0.1)
                        {
                            sf::Vector2f force_change = (all_balls[j].shape.getPosition() - all_balls[i].shape.getPosition()) / dist  * (float)(pow(dist_rel,2) - 1.0) * (float)exp(-0.25 * pow(dist_rel,2));
        
                            all_balls[i].force[0] += (int64_t)(window.accuracy*force_change.x * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                            all_balls[i].force[1] += (int64_t)(window.accuracy*force_change.y * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                    
                        }     
                    }

                    // update velocity of the ball delta V = F/M (F with drag force proportional to velocity):
                    all_balls[i].velocity += (sf::Vector2f((float)all_balls[i].force[0]/window.accuracy,(float)all_balls[i].force[1]/window.accuracy) - (all_balls[i].velocity/all_balls[i].mass * window.drag_coef))* window.dt ;              
                } });

            window.loading_bar_in.setSize(sf::Vector2f{(float)window.window_x * frame_nr / window.frames, (float)window.wall_thickness / 2});

            // draw frame:
            window.draw_background();

            tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r1)
                              {
                                            for (int i = r1.begin();i < r1.end(); ++i)
                                            {
            all_balls[i].update(window.dt);
       
                                            } });

            for (int i = 0; i < all_balls.size(); ++i)
            {
                window.window.draw(all_balls[i]);
            }

            window.window.display();

            if (video_generate)
            {
                window.texture.update(window.window);
                window.texture.copyToImage().saveToFile("./images/img_" + std::to_string(frame_nr) + ".png");
            }
        }
    }
    window.window.close();

    std::cout.precision(16);
    float sum = 0;
    for (auto it = all_balls.begin(); it != all_balls.end(); ++it)
    {
        sum += it->total_velocity;
    }
    // std::cout << "\t" << sum << std::endl;
}

void sim_2_TBB(benchmark::State &state)
{
    const int param = state.range(0);
    const bool video_generate = state.range(1) != 0 ? true : false;
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, state.range(2));

    SFML_window window;

    // create particles:
    std::vector<Ball> all_balls(param);
    balls_initialization(all_balls, window);
    std::vector<int32_t> all_balls_forces(2 * param * param, 0);

    for (auto _ : state)
    {
        std::mutex m;

        // render a defined number of frames and close the simulation:
        for (int frame_nr = 0; frame_nr < window.frames; ++frame_nr)
        {
            sf::Event event;
            while (window.window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.window.close();
            }

            tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r)
                              {
                for (int i = r.begin(); i < r.end(); ++i)
            {
          
                // check collision with walls:
                if (all_balls[i].shape.getPosition().x + all_balls[i].radius >= window.window_x + window.wall_thickness)
                {
                    all_balls[i].velocity.x = -abs(all_balls[i].velocity.x);
                }
                else if (all_balls[i].shape.getPosition().x - all_balls[i].radius <= window.wall_thickness)
                {
                    all_balls[i].velocity.x = abs(all_balls[i].velocity.x);
                }
                if (all_balls[i].shape.getPosition().y + all_balls[i].radius >= window.window_y + window.wall_thickness)
                {
                    all_balls[i].velocity.y = -abs(all_balls[i].velocity.y);
                }
                else if (all_balls[i].shape.getPosition().y - all_balls[i].radius <= window.wall_thickness)
                {
                    all_balls[i].velocity.y = abs(all_balls[i].velocity.y);
                }

                // update interactions with balls:
                tbb::parallel_for(tbb::blocked_range<int>(i + 1, all_balls.size()), [&](tbb::blocked_range<int> r1)
                                  {
                                      for (int j = r1.begin(); j < r1.end(); ++j)
                                      {

                    // distanse (multiplication of sum of radiuses):
                    float dist = all_balls[i].distance_2D(all_balls[j]);
                    float dist_rel = dist/(all_balls[i].radius + all_balls[j].radius)/window.force_zero_coef;
                    if (dist_rel > 0.1)
                    {
                        sf::Vector2f force_change = (all_balls[j].shape.getPosition() - all_balls[i].shape.getPosition()) / dist  * (float)(pow(dist_rel,2) - 1.0) * (float)exp(-0.25 * pow(dist_rel,2));
                        // old code:
                        // m.lock();
                        // all_balls[i].force[0] += static_cast<int64>(window.accuracy * force_change.x * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        // all_balls[i].force[1] += static_cast<int64>(window.accuracy * force_change.y * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        // all_balls[j].force[0] -= static_cast<int64>(window.accuracy * force_change.x * all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        // all_balls[j].force[1] -= static_cast<int64>(window.accuracy * force_change.y * all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        // m.unlock();

                        // new code:
                        all_balls_forces[(2 * i) * all_balls.size() + j] = static_cast<int64>(window.accuracy * force_change.x * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        all_balls_forces[(2 * i + 1) * all_balls.size() + j] = static_cast<int64>(window.accuracy * force_change.y * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        all_balls_forces[(2 * j) * all_balls.size() + i] = -static_cast<int64>(window.accuracy * force_change.x * all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        all_balls_forces[(2 * j + 1) * all_balls.size() + i] = -static_cast<int64>(window.accuracy * force_change.y * all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        // sum for each ball is calculated after all forces are updated
                    }
                    else
                    {
                        all_balls_forces[(2 * i) * all_balls.size() + j] = 0;
                        all_balls_forces[(2 * i + 1) * all_balls.size() + j] = 0;
                        all_balls_forces[(2 * j) * all_balls.size() + i] = 0;
                        all_balls_forces[(2 * j + 1) * all_balls.size() + i] = 0;
                    }
                }
        
              } );
             } });

            tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r1)
                              {
                                for (int i = r1.begin();i < r1.end(); ++i)
                                {
         
                all_balls[i].force[0] = std::reduce((all_balls_forces.cbegin() + (2 * i) * all_balls.size()), (all_balls_forces.cbegin() + (2 * i + 1) * all_balls.size()), (int64_t)0);
                all_balls[i].force[1] = std::reduce((all_balls_forces.cbegin() + (2 * i + 1) * all_balls.size()), (all_balls_forces.cbegin() + (2 * i + 2) * all_balls.size()), (int64_t)0);

                // update velocity of the ball delta V = F/M (F with drag force proportional to velocity):
                all_balls[i].velocity += (sf::Vector2f((float)all_balls[i].force[0] / window.accuracy, (float)all_balls[i].force[1] / window.accuracy) - (all_balls[i].velocity /all_balls[i].mass* window.drag_coef))* window.dt ;
                // update ball position:
                all_balls[i].update(window.dt);
                } });

            window.loading_bar_in.setSize(sf::Vector2f{(float)window.window_x * frame_nr / window.frames, (float)window.wall_thickness / 2});
            // draw frame:
            window.draw_background();

            for (int i = 0; i < all_balls.size(); ++i)
            {
                window.window.draw(all_balls[i]);
            }

            window.window.display();

            if (video_generate)
            {
                window.texture.update(window.window);
                window.texture.copyToImage().saveToFile("./images/img_" + std::to_string(frame_nr) + ".png");
            }
        }
    }
    window.window.close();
    std::cout.precision(16);
    float sum = 0;
    for (auto it = all_balls.begin(); it != all_balls.end(); ++it)
    {
        sum += it->total_velocity;
    }
    // std::cout << "\t" << sum << std::endl;
}

void sim_3(benchmark::State &state)
{
    const int balls_number = state.range(0);
    const bool video_generate = state.range(1) != 0 ? true : false;
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, state.range(2));

    SFML_window window;

    // create particles:
    std::vector<Ball> all_balls(balls_number);
    balls_initialization(all_balls, window);
    std::vector<int32_t> all_balls_forces(2 * balls_number * balls_number, 0);

    // don't use tab of struct but pure arrays:
    std::vector<float> balls_pos_x(balls_number);
    std::vector<float> balls_pos_y(balls_number);
    std::vector<float> balls_vel_x(balls_number);
    std::vector<float> balls_vel_y(balls_number);
    std::vector<float> balls_mass(balls_number);
    std::vector<float> balls_radius(balls_number);
    std::vector<int64_t> balls_force_x(balls_number);
    std::vector<int64_t> balls_force_y(balls_number);
    // copy data from initialized structeres:
    for (int i = 0; i < all_balls.size(); ++i)
    {
        balls_pos_x[i] = all_balls[i].shape.getPosition().x;
        balls_pos_y[i] = all_balls[i].shape.getPosition().y;
        balls_vel_x[i] = all_balls[i].velocity.x;
        balls_vel_y[i] = all_balls[i].velocity.y;
        balls_mass[i] = all_balls[i].mass;
        balls_radius[i] = all_balls[i].radius;
    }
    for (auto _ : state)
    {

        std::mutex m;

        // render a defined number of frames and close the simulation:
        for (int frame_nr = 0; frame_nr < window.frames; ++frame_nr)
        {
            sf::Event event;
            while (window.window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.window.close();
            }

            tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r)
                              {
                for (int i = r.begin(); i < r.end(); ++i)
            {
                // check collision with walls:
                if (balls_pos_x[i] + balls_radius[i] >= window.window_x + window.wall_thickness)
                {
                    balls_vel_x[i] = -abs(balls_vel_x[i]);
                }
                else if (balls_pos_x[i] - balls_radius[i] <= window.wall_thickness)
                {
                    balls_vel_x[i] = abs(balls_vel_x[i]);
                }
                if (balls_pos_y[i] + balls_radius[i] >= window.window_y + window.wall_thickness)
                {
                    balls_vel_y[i] = -abs(balls_vel_y[i]);
                }
                else if (balls_pos_y[i] - balls_radius[i] <= window.wall_thickness)
                {
                    balls_vel_y[i] = abs(balls_vel_y[i]);
                }
                // update interactions with balls:
                tbb::parallel_for(tbb::blocked_range<int>(i + 1, all_balls.size()), [&](tbb::blocked_range<int> r1)
                                  {
                for (int j = r1.begin(); j < r1.end(); ++j)
                {
                    // distanse (multiplication of sum of radiuses):
                    float dist = all_balls[i].distance_2D(all_balls[j]);
                    float dist_rel = dist/(all_balls[i].radius + all_balls[j].radius)/window.force_zero_coef;
                    if (dist_rel > 0.1)
                    {
                        sf::Vector2f force_change = (all_balls[j].shape.getPosition() - all_balls[i].shape.getPosition()) / dist  * (float)(pow(dist_rel,2) - 1.0) * (float)exp(-0.25 * pow(dist_rel,2));
                        // old code:
                        // m.lock();
                        // balls_force_x[i] += static_cast<int64>(window.accuracy * force_change.x * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        // balls_force_y[i] += static_cast<int64>(window.accuracy * force_change.y * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);

                        // balls_force_x[j] -= static_cast<int64>(window.accuracy * force_change.x * all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        // balls_force_y[j] -= static_cast<int64>(window.accuracy * force_change.y * all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        // m.unlock();

                        all_balls_forces[(2 * i) * all_balls.size() + j] = static_cast<int64>(window.accuracy * force_change.x * balls_mass[j] * all_balls[i].force_relations[all_balls[j].flavour]);
                        all_balls_forces[(2 * i + 1) * all_balls.size() + j] = static_cast<int64>(window.accuracy * force_change.y * balls_mass[j] * all_balls[i].force_relations[all_balls[j].flavour]);
                        all_balls_forces[(2 * j) * all_balls.size() + i] = -static_cast<int64>(window.accuracy * force_change.x * balls_mass[i] * all_balls[j].force_relations[all_balls[i].flavour]);
                        all_balls_forces[(2 * j + 1) * all_balls.size() + i] = -static_cast<int64>(window.accuracy * force_change.y * balls_mass[i] * all_balls[j].force_relations[all_balls[i].flavour]);
                        // sum for each ball is calculated after all forces are updated
                    }
                    else
                    {
                        all_balls_forces[(2 * i) * all_balls.size() + j] = 0;
                        all_balls_forces[(2 * i + 1) * all_balls.size() + j] = 0;
                        all_balls_forces[(2 * j) * all_balls.size() + i] = 0;
                        all_balls_forces[(2 * j + 1) * all_balls.size() + i] = 0;
                    }
                } });
            } });

            window.loading_bar_in.setSize(sf::Vector2f{(float)window.window_x * frame_nr / window.frames, (float)window.wall_thickness / 2});

            // draw frame:
            window.draw_background();

            tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r)
                              {
                for (int i = r.begin(); i < r.end(); ++i)
            {
                balls_force_x[i] = std::reduce((all_balls_forces.cbegin() + (2 * i) * all_balls.size()), (all_balls_forces.cbegin() + (2 * i + 1) * all_balls.size()), (int64_t)0);
                balls_force_y[i] = std::reduce((all_balls_forces.cbegin() + (2 * i + 1) * all_balls.size()), (all_balls_forces.cbegin() + (2 * i + 2) * all_balls.size()), (int64_t)0);

                // compute vel and add drag effect:
                balls_vel_x[i] += ((float)((float)balls_force_x[i] / window.accuracy) - balls_vel_x[i] / balls_mass[i] * window.drag_coef) * window.dt;
                balls_vel_y[i] += ((float)((float)balls_force_y[i] / window.accuracy) - balls_vel_y[i] / balls_mass[i] * window.drag_coef) * window.dt;

                // balls_force_x[i] = 0;
                // balls_force_y[i] = 0;

                balls_pos_x[i] += balls_vel_x[i] * window.dt;
                balls_pos_y[i] += balls_vel_y[i] * window.dt;

                all_balls[i].shape.setPosition(balls_pos_x[i], balls_pos_y[i]);
              
            } });

            for (int i = 0; i < all_balls.size(); ++i)
            {
                window.window.draw(all_balls[i]);
            }

            window.window.display();

            if (video_generate)
            {
                window.texture.update(window.window);
                window.texture.copyToImage().saveToFile("./images/img_" + std::to_string(frame_nr) + ".png");
            }
        }
    }
    window.window.close();
    std::cout.precision(16);
    float sum = 0;
    for (auto i = 0; i < all_balls.size(); ++i)
    {
        sum += std::sqrt(balls_vel_x[i] * balls_vel_x[i] + balls_vel_y[i] * balls_vel_y[i]);
    }

    // std::cout << "\t" << sum << std::endl;
}

void sim_3D_TBB(benchmark::State &state)
{
    const int param = state.range(0);
    const bool video_generate = state.range(1) != 0 ? true : false;
    tbb::global_control c(tbb::global_control::max_allowed_parallelism, state.range(2));

    SFML_window window;

    // create particles:
    std::vector<Ball> all_balls(param);
    balls_initialization(all_balls, window);
    std::vector<int32_t> all_balls_forces(3 * param * param, 0);

    for (auto _ : state)
    {
        // render a defined number of frames and close the simulation:
        for (int frame_nr = 0; frame_nr < window.frames; ++frame_nr)
        {
            sf::Event event;
            while (window.window.pollEvent(event))
            {
                if (event.type == sf::Event::Closed)
                    window.window.close();
            }

            tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r)
                              {
                for (int i = r.begin(); i < r.end(); ++i)
            {
          
                // check collision with walls:
                // X:
                if (all_balls[i].position_3D[0] + all_balls[i].radius >= window.window_x + window.wall_thickness)
                {
                    all_balls[i].velocity_3D[0]= -abs(all_balls[i].velocity_3D[0]);
                }
                else if (all_balls[i].position_3D[0] - all_balls[i].radius <= window.wall_thickness)
                {
                    all_balls[i].velocity_3D[0] = abs(all_balls[i].velocity_3D[0]);
                }
                // Y:
                if (all_balls[i].position_3D[1]+ all_balls[i].radius >= window.window_y + window.wall_thickness)
                {
                    all_balls[i].velocity_3D[1] = -abs(all_balls[i].velocity_3D[1]);
                }
                else if (all_balls[i].position_3D[1] - all_balls[i].radius <= window.wall_thickness)
                {
                    all_balls[i].velocity_3D[1] = abs(all_balls[i].velocity_3D[1]);
                }
                // Z:
                if (all_balls[i].position_3D[2]+ all_balls[i].radius >= window.window_z )
                {
                    all_balls[i].velocity_3D[2] = -abs(all_balls[i].velocity_3D[2]);
                }
                else if (all_balls[i].position_3D[2] - all_balls[i].radius <= 0)
                {
                    all_balls[i].velocity_3D[2] = abs(all_balls[i].velocity_3D[2]);
                }

                // update interactions with balls:
                tbb::parallel_for(tbb::blocked_range<int>(i + 1, all_balls.size()), [&](tbb::blocked_range<int> r1)
                                  {
                                      for (int j = r1.begin(); j < r1.end(); ++j)
                                      {

                    // distanse (multiplication of sum of radiuses):
                    float dist = all_balls[i].distance_3D(all_balls[j]);
                    float dist_rel = dist/(all_balls[i].radius + all_balls[j].radius)/window.force_zero_coef;
                    if (dist_rel > 0.1)
                    {
                        const float temp = 1.f/dist*(pow(dist_rel,2) - 1.0) * (float)exp(-0.25 * pow(dist_rel,2));
                        const float force_change[3] =  {(all_balls[j].position_3D[0]-all_balls[i].position_3D[0])*temp,
                                                (all_balls[j].position_3D[1]-all_balls[i].position_3D[1])*temp,
                                                (all_balls[j].position_3D[2]-all_balls[i].position_3D[2])*temp}; 
                                            
                        // forces XYZ:
                        all_balls_forces[(3 * i) * all_balls.size() + j] = static_cast<int64>(window.accuracy * force_change[0] * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        all_balls_forces[(3 * i + 1) * all_balls.size() + j] = static_cast<int64>(window.accuracy * force_change[1] * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        all_balls_forces[(3 * i + 2) * all_balls.size() + j] = static_cast<int64>(window.accuracy * force_change[2] * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        all_balls_forces[(3 * j) * all_balls.size() + i] = -static_cast<int64>(window.accuracy * force_change[0]* all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        all_balls_forces[(3 * j + 1) * all_balls.size() + i] = -static_cast<int64>(window.accuracy * force_change[1] * all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        all_balls_forces[(3 * j + 2) * all_balls.size() + i] = -static_cast<int64>(window.accuracy * force_change[2] * all_balls[i].mass * all_balls[j].force_relations[all_balls[i].flavour]);
                        
                        // sum for each ball is calculated after all forces are updated
                    }
                    else
                    {
                        all_balls_forces[(3 * i) * all_balls.size() + j] = 0;
                        all_balls_forces[(3 * i + 1) * all_balls.size() + j] = 0;
                        all_balls_forces[(3 * i + 2) * all_balls.size() + j] = 0;
                        all_balls_forces[(3 * j) * all_balls.size() + i] = 0;
                        all_balls_forces[(3 * j + 1) * all_balls.size() + i] = 0;
                        all_balls_forces[(3 * j + 2) * all_balls.size() + i] = 0;
                    }
                }
        
              } );
             } });

            window.loading_bar_in.setSize(sf::Vector2f{(float)window.window_x * frame_nr / window.frames, (float)window.wall_thickness / 2});
            // draw frame:
            window.draw_background();

            tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r1)
                              {
                                for (int i = r1.begin();i < r1.end(); ++i)
                                {
                // XYZ:
                all_balls[i].force[0] = std::reduce((all_balls_forces.cbegin() + (3 * i) * all_balls.size()), (all_balls_forces.cbegin() + (3 * i + 1) * all_balls.size()), (int64_t)0);
                all_balls[i].force[1] = std::reduce((all_balls_forces.cbegin() + (3 * i + 1) * all_balls.size()), (all_balls_forces.cbegin() + (3 * i + 2) * all_balls.size()), (int64_t)0);
                all_balls[i].force[2] = std::reduce((all_balls_forces.cbegin() + (3 * i + 2) * all_balls.size()), (all_balls_forces.cbegin() + (3 * i + 3) * all_balls.size()), (int64_t)0);
               
                // update velocity of the ball delta V = F/M (F with drag force proportional to velocity):
                
                all_balls[i].velocity_3D[0] += ((float)((float)all_balls[i].force[0]/ window.accuracy) - all_balls[i].velocity_3D[0]/all_balls[i].mass* window.drag_coef)*window.dt;
                all_balls[i].velocity_3D[1] += ((float)((float)all_balls[i].force[1]/ window.accuracy) - all_balls[i].velocity_3D[1]/all_balls[i].mass* window.drag_coef)*window.dt;
                all_balls[i].velocity_3D[2] += ((float)((float)all_balls[i].force[2]/ window.accuracy) - all_balls[i].velocity_3D[2]/all_balls[i].mass* window.drag_coef)*window.dt;

                all_balls[i].velocity += sf::Vector2f(all_balls[i].velocity_3D[0] ,all_balls[i].velocity_3D[1]);
                
                // update position:
                all_balls[i].position_3D[0] += all_balls[i].velocity_3D[0]*window.dt;
                all_balls[i].position_3D[1] += all_balls[i].velocity_3D[1]*window.dt;
                all_balls[i].position_3D[2] += all_balls[i].velocity_3D[2]*window.dt;

                // update ball position (for drawing):
                all_balls[i].shape.setPosition(all_balls[i].position_3D[0],all_balls[i].position_3D[1]);
                } });

            for (int i = 0; i < all_balls.size(); ++i)
            {
                window.window.draw(all_balls[i]);
            }

            window.window.display();

            if (video_generate)
            {
                window.texture.update(window.window);
                window.texture.copyToImage().saveToFile("./images/img_" + std::to_string(frame_nr) + ".png");
            }
        }
    }
    window.window.close();
    std::cout.precision(16);
    float sum = 0;
    for (auto it = all_balls.begin(); it != all_balls.end(); ++it)
    {
        sum += std::sqrt(it->position_3D[0] * it->position_3D[0] + it->position_3D[1] * it->position_3D[1] + it->position_3D[2] * it->position_3D[2]);
        ;
    }
    // std::cout << "\t" << sum << std::endl;
}

const int balls = 2000;
// BENCHMARK(sim_2_TBB)->Name("faster code + save images")->Args({balls, 1, 8})->Unit(benchmark::kSecond);
BENCHMARK(sim_3D_TBB)->Name("3D sim")->Args({balls, 1, 8})->Unit(benchmark::kSecond);
// BENCHMARK(sim)->Name("simulation ")->Args({balls, 0})->Unit(benchmark::kSecond);
// BENCHMARK(sim_faster)->Name("faster code ")->Args({balls, 0})->Unit(benchmark::kSecond);
// BENCHMARK(sim_TBB)->Name("simulation with TBB ")->Args({balls, 0, 1})->Args({balls, 0, 4})->Args({balls, 0, 8})->Unit(benchmark::kSecond);
// BENCHMARK(sim_2_TBB)->Name("faster code with TBB ")->Args({balls, 0, 1})->Args({balls, 0, 4})->Args({balls, 0, 8})->Unit(benchmark::kSecond);
// BENCHMARK(sim_3)->Name("SoA test ")->Args({balls, 0, 1})->Args({balls, 0, 4})->Args({balls, 0, 8})->Unit(benchmark::kSecond);
// BENCHMARK(sim_TBB)->Name("save images ")->Args({balls, 1, 8})->Unit(benchmark::kSecond);
// BENCHMARK(generate_video)->Name("generate video")->Args({balls, 30})->Unit(benchmark::kSecond);

BENCHMARK_MAIN();