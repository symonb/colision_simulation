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
    const int window_x = 800;
    const int window_y = 800;
    const int frames = 1000;
    const float drag_coef = 0.04;
    const float dt = 0.4;
    const float fps = 300;
    const double accuracy = 10E6;
    sf::RenderWindow window;
    sf::RectangleShape box;
    sf::Texture texture;
    sf::RectangleShape loading_bar_out;
    sf::RectangleShape loading_bar_in;
    std::vector<cv::Mat> images;

    SFML_window() : images{std::vector<cv::Mat>(frames)}, window{sf::RenderWindow(sf::VideoMode(window_x + 2 * wall_thickness, window_y + 2 * wall_thickness), "COLISION SIMULATOR")}
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
        ball->shape.setPosition((float)rand() / RAND_MAX * (window.window_x - 2 * ball->radious) + window.wall_thickness + ball->radious, (float)rand() / RAND_MAX * (window.window_y - 2 * ball->radious) + window.wall_thickness + ball->radious);
        // check collisions:
        if (ball != all_balls.begin())
        {
            for (auto ball_comp = all_balls.begin(); ball_comp != ball; ball_comp++)
            {
                if (ball->distance(*ball_comp) <= (ball->radious + ball_comp->radious))
                {
                    ball->shape.setPosition((float)rand() / RAND_MAX * (window.window_x - 2 * ball->radious) + window.wall_thickness + ball->radious, (float)rand() / RAND_MAX * (window.window_y - 2 * ball->radious) + window.wall_thickness + ball->radious);
                    ball_comp = all_balls.begin();
                }
            }
        }
    }
};

void generate_video(SFML_window &window)
{

    std::string path = "./images/img_";

    for (int counter = 0; counter < window.frames; ++counter)
    {
        window.images[counter] = (cv::imread(path + std::to_string(counter) + ".png", cv::IMREAD_COLOR));
    }

    cv::VideoWriter outputVideo("./video_1.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), window.fps, cv::Size(window.images[0].size()), true);

    if (!outputVideo.isOpened())
    {
        std::cout << "Could not open the output video for write: " << std::endl;
    }

    for (auto img = window.images.begin(); img != window.images.end(); img++)
    {
        outputVideo.write(*img);
    }

    std::cout << "Finished writing" << std::endl;
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
                if (ball->shape.getPosition().x + ball->radious >= window.window_x + window.wall_thickness)
                {
                    ball->velocity.x = -1 * fabs(ball->velocity.x);
                }
                else if (ball->shape.getPosition().x - ball->radious <= window.wall_thickness)
                {
                    ball->velocity.x = fabs(ball->velocity.x);
                }
                if (ball->shape.getPosition().y + ball->radious >= window.window_y + window.wall_thickness)
                {
                    ball->velocity.y = -1 * fabs(ball->velocity.y);
                }
                else if (ball->shape.getPosition().y - ball->radious <= window.wall_thickness)
                {
                    ball->velocity.y = fabs(ball->velocity.y);
                }

                // update interactions with balls:
                for (auto ball_comp = all_balls.begin(); ball_comp != all_balls.end(); ball_comp++)
                {
                    if (ball_comp != ball)
                    {
                        // distanse (multiplication of sum of radiouses):
                        float dist = ball->distance(*ball_comp) / (ball->radious + ball_comp->radious);
                        if (dist > 0.1)
                        {
                            // before:

                            // float f_value = ball->force_relations[ball_comp->flavour] * ball_comp->mass * (pow(dist, 2) - 1.0) * exp(-0.25 * pow(dist, 2));
                            // ball->force += (ball_comp->shape.getPosition() - ball->shape.getPosition()) / dist * f_value;

                            // after:

                            sf::Vector2f force_change = (ball_comp->shape.getPosition() - ball->shape.getPosition()) / dist / (ball->radious + ball_comp->radious) * (float)(pow(dist, 2) - 1.0) * (float)exp(-0.25 * pow(dist, 2));
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
    std::cout << "\t" << sum << std::endl;

    // save video from images:
    if (video_generate)
    {
        generate_video(window);
    }
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
                if (ball->shape.getPosition().x + ball->radious >= window.window_x + window.wall_thickness)
                {
                    ball->velocity.x = -1 * fabs(ball->velocity.x);
                }
                else if (ball->shape.getPosition().x - ball->radious <= window.wall_thickness)
                {
                    ball->velocity.x = fabs(ball->velocity.x);
                }
                if (ball->shape.getPosition().y + ball->radious >= window.window_y + window.wall_thickness)
                {
                    ball->velocity.y = -1 * fabs(ball->velocity.y);
                }
                else if (ball->shape.getPosition().y - ball->radious <= window.wall_thickness)
                {
                    ball->velocity.y = fabs(ball->velocity.y);
                }
            }
            for (auto ball = all_balls.begin(); ball != all_balls.end(); ++ball)
            {
                // update interactions with balls:
                for (auto ball_comp = std::next(ball); ball_comp != all_balls.end(); ++ball_comp)
                {
                    // distanse (multiplication of sum of radiouses):

                    float dist = ball->distance(*ball_comp) / (ball->radious + ball_comp->radious);
                    if (dist > 0.1)
                    {
                        sf::Vector2f force_change = (ball_comp->shape.getPosition() - ball->shape.getPosition()) / dist / (ball->radious + ball_comp->radious) * (float)(pow(dist, 2) - 1.0) * (float)exp(-0.25 * pow(dist, 2));
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
    std::cout << "\t" << sum << std::endl;

    // save video from images:
    if (video_generate)
    {
        generate_video(window);
    }
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
                    if (all_balls[i].shape.getPosition().x + all_balls[i].radious >= window.window_x + window.wall_thickness)
                    {
                        all_balls[i].velocity.x = -1 * fabs(all_balls[i].velocity.x);
                    }
                    else if (all_balls[i].shape.getPosition().x - all_balls[i].radious <= window.wall_thickness)
                    {
                        all_balls[i].velocity.x = fabs(all_balls[i].velocity.x);
                    }
                    if (all_balls[i].shape.getPosition().y + all_balls[i].radious >= window.window_y + window.wall_thickness)
                    {
                        all_balls[i].velocity.y = -1 * fabs(all_balls[i].velocity.y);
                    }
                    else if (all_balls[i].shape.getPosition().y - all_balls[i].radious <= window.wall_thickness)
                    {
                        all_balls[i].velocity.y = fabs(all_balls[i].velocity.y);
                    }

                    // update interactions with balls:
                    tbb::parallel_for(tbb::blocked_range<int>(0, all_balls.size()), [&](tbb::blocked_range<int> r1)
                                      {
                                            for (int j = r1.begin();j < r1.end(); ++j)
                                      {
                                        
                    // distanse (multiplication of sum of radiouses):
                    float dist = all_balls[i].distance(all_balls[j]) / (all_balls[i].radious + all_balls[j].radious);
                    if (dist > 0.1)
                    {
                        sf::Vector2f force_change = (all_balls[j].shape.getPosition() - all_balls[i].shape.getPosition()) / dist/(all_balls[i].radious + all_balls[j].radious) * (float)(pow(dist, 2) - 1.0) * (float)exp(-0.25 * pow(dist, 2));
                        m.lock() ;
                        all_balls[i].force[0] += (int64_t)(window.accuracy*force_change.x * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        all_balls[i].force[1] += (int64_t)(window.accuracy*force_change.y * all_balls[j].mass * all_balls[i].force_relations[all_balls[j].flavour]);
                        m.unlock();
                    }     
                                      } });

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
    std::cout << "\t" << sum << std::endl;

    // save video from images:
    if (video_generate)
    {
        generate_video(window);
    }
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
                if (all_balls[i].shape.getPosition().x + all_balls[i].radious >= window.window_x + window.wall_thickness)
                {
                    all_balls[i].velocity.x = -1 * fabs(all_balls[i].velocity.x);
                }
                else if (all_balls[i].shape.getPosition().x - all_balls[i].radious <= window.wall_thickness)
                {
                    all_balls[i].velocity.x = fabs(all_balls[i].velocity.x);
                }
                if (all_balls[i].shape.getPosition().y + all_balls[i].radious >= window.window_y + window.wall_thickness)
                {
                    all_balls[i].velocity.y = -1 * fabs(all_balls[i].velocity.y);
                }
                else if (all_balls[i].shape.getPosition().y - all_balls[i].radious <= window.wall_thickness)
                {
                    all_balls[i].velocity.y = fabs(all_balls[i].velocity.y);
                }

                // update interactions with balls:
                tbb::parallel_for(tbb::blocked_range<int>(i + 1, all_balls.size()), [&](tbb::blocked_range<int> r1)
                                  {
                                      for (int j = r1.begin(); j < r1.end(); ++j)
                                      {

                    // distanse (multiplication of sum of radiouses):
                    float dist = all_balls[i].distance(all_balls[j]) / (all_balls[i].radious + all_balls[j].radious);
                    if (dist > 0.1)
                    {
                        sf::Vector2f force_change = (all_balls[j].shape.getPosition() - all_balls[i].shape.getPosition()) / dist / (all_balls[i].radious + all_balls[j].radious) * (float)(pow(dist, 2) - 1.0) * (float)exp(-0.25 * pow(dist, 2));
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

            window.loading_bar_in.setSize(sf::Vector2f{(float)window.window_x * frame_nr / window.frames, (float)window.wall_thickness / 2});

            // draw frame:
            window.draw_background();

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
    std::cout << "\t" << sum << std::endl;

    // save video from images:
    if (video_generate)
    {
        generate_video(window);
    }
}

const int balls = 500;
// BENCHMARK(sim)->Name("simulation")->Args(std::vector<int64_t>{balls, 0})->Unit(benchmark::kSecond);
BENCHMARK(sim_faster)->Name("faster code")->Args(std::vector<int64_t>{balls, 0})->Unit(benchmark::kSecond);
// BENCHMARK(sim_TBB)->Name("simulation with TBB")->Args({balls, 0, 1})->Args({balls, 0, 4})->Args({balls, 0, 8})->Unit(benchmark::kSecond);
BENCHMARK(sim_2_TBB)->Name("faster code with TBB")->Args({balls, 0, 8})->Args({balls, 0, 4})->Args({balls, 0, 8})->Unit(benchmark::kSecond);
BENCHMARK_MAIN();