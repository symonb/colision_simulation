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
#include "ball.hpp"

void sim(benchmark::State &state)
{
    const int param = state.range(0);
    const bool video_generate = state.range(1) != 0 ? true : false;
    const int wall_thickness = 30;
    const int window_x = 800;
    const int window_y = 800;
    const int frames = 5000;
    float dt = 0.1;
    const float fps = 300;

    // create particles:
    std::vector<Ball> all_balls(param);
    for (auto ball = all_balls.begin(); ball != all_balls.end(); ball++)
    {
        // initiate position:
        ball->shape.setPosition((float)rand() / RAND_MAX * (window_x - 2 * ball->radious) + wall_thickness + ball->radious, (float)rand() / RAND_MAX * (window_y - 2 * ball->radious) + wall_thickness + ball->radious);
        // check collisions:
        if (ball != all_balls.begin())
        {
            for (auto ball_comp = all_balls.begin(); ball_comp != ball; ball_comp++)
            {
                if (ball->distance(*ball_comp) <= (ball->radious + ball_comp->radious))
                {
                    ball->shape.setPosition((float)rand() / RAND_MAX * (window_x - 2 * ball->radious) + wall_thickness + ball->radious, (float)rand() / RAND_MAX * (window_y - 2 * ball->radious) + wall_thickness + ball->radious);
                    ball_comp = all_balls.begin();
                }
            }
        }
    }
    printf("initialization done\n\n");

    // window for a live view:
    sf::RenderWindow window(sf::VideoMode(window_x + 2 * wall_thickness, window_y + 2 * wall_thickness), "COLISION SIMULATOR");

    // texture object for saving rendered images:
    sf::Texture texture;
    texture.create(window.getSize().x, window.getSize().y);

    // bounding box
    sf::RectangleShape box;
    box.setSize(sf::Vector2f{window_x, window_y});
    box.setOrigin(window_x / 2, window_y / 2);
    box.setPosition(window_x / 2 + wall_thickness, window_y / 2 + wall_thickness);
    box.setFillColor(sf::Color::Transparent);
    box.setOutlineColor(sf::Color::White);
    box.setOutlineThickness(wall_thickness);
    // loading bar:
    sf::RectangleShape loading_bar_out;
    sf::RectangleShape loading_bar_in;
    loading_bar_out.setSize(sf::Vector2f{window_x, wall_thickness / 2});
    loading_bar_out.setOrigin(window_x / 2, wall_thickness / 4);
    loading_bar_out.setPosition(window_x / 2 + wall_thickness, window_y + wall_thickness * 1.5);
    loading_bar_out.setFillColor(sf::Color::Transparent);
    loading_bar_out.setOutlineColor(sf::Color::Cyan);
    loading_bar_out.setOutlineThickness(3);

    loading_bar_in.setSize(sf::Vector2f{0, wall_thickness / 2});
    loading_bar_in.setOrigin(0, wall_thickness / 4);
    loading_bar_in.setPosition(wall_thickness, window_y + wall_thickness * 1.5);
    loading_bar_in.setFillColor(sf::Color::Cyan);

    // vector for images:
    std::vector<cv::Mat> images(frames);

    {
        for (auto _ : state)
        {

            // render a defined number of frames and close the simulation:
            for (int i = 0; i < frames; i++)
            {
                sf::Event event;
                while (window.pollEvent(event))
                {
                    if (event.type == sf::Event::Closed)
                        window.close();
                }

                for (auto ball = all_balls.begin(); ball != all_balls.end(); ball++)
                {
                    // check collision with walls:
                    if (ball->shape.getPosition().x + ball->radious >= window_x + wall_thickness)
                    {
                        ball->velocity.x = -1 * fabs(ball->velocity.x);
                    }
                    else if (ball->shape.getPosition().x - ball->radious <= wall_thickness)
                    {
                        ball->velocity.x = fabs(ball->velocity.x);
                    }
                    if (ball->shape.getPosition().y + ball->radious >= window_y + wall_thickness)
                    {
                        ball->velocity.y = -1 * fabs(ball->velocity.y);
                    }
                    else if (ball->shape.getPosition().y - ball->radious <= wall_thickness)
                    {
                        ball->velocity.y = fabs(ball->velocity.y);
                    }

                    Vector2f force = {0, 0};
                    // update interactions with balls:
                    for (auto ball_comp = all_balls.begin(); ball_comp != all_balls.end(); ball_comp++)
                    {
                        if (ball_comp != ball)
                        {
                            // distanse (multiplication of sum of radiouses):
                            float dist = ball->distance(*ball_comp) / (ball->radious + ball_comp->radious);
                            if (dist > 0.1)
                            {
                                float f_value = ball->force_relations[ball_comp->flavour] * ball->mass * ball_comp->mass * (pow(dist, 2) - 1.0) * exp(-0.25 * pow(dist, 2));
                                force += (ball_comp->shape.getPosition() - ball->shape.getPosition()) / dist * f_value;
                            }
                        }
                    }
                    // update velocity of the ball delta V = F/M (F with drag force proportional to velocity):
                    ball->velocity += force * dt - (ball->velocity * 0.001f) / ball->mass;
                }

                loading_bar_in.setSize(sf::Vector2f{(float)window_x * i / frames, (float)wall_thickness / 2});

                // draw frame:
                window.clear();
                window.draw(box);
                window.draw(loading_bar_out);
                window.draw(loading_bar_in);

                for (auto ball = all_balls.begin(); ball != all_balls.end(); ball++)
                {
                    ball->update(dt);
                    window.draw(*ball);
                }
                if (video_generate)
                {
                    texture.update(window);
                    texture.copyToImage().saveToFile("./images/img_" + std::to_string(i) + ".png");
                }
                window.display();

                // save frames:
                // images[i] = cv::Mat(cv::Size(window.getSize().x, window.getSize().y), CV_8UC4, (void *)texture.copyToImage().getPixelsPtr()).clone();
                // cv::cvtColor(images[i], images[i], cv::COLOR_BGR2RGB);
            }
        }

        window.close();
    }
    // save video from images:
    if (video_generate)
    {
        std::string path = "./images/img_";

        for (int counter = 0; counter < frames; ++counter)
        {
            images[counter] = (cv::imread(path + std::to_string(counter) + ".png", cv::IMREAD_COLOR));
        }

        cv::VideoWriter outputVideo("./video_1.avi", cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(images[0].size()), true);

        if (!outputVideo.isOpened())
        {
            std::cout << "Could not open the output video for write: " << std::endl;
        }

        for (auto img = images.begin(); img != images.end(); img++)
        {
            outputVideo.write(*img);
        }

        std::cout << "Finished writing" << std::endl;
    }
}

BENCHMARK(sim)->Name("simulation")->Args(std::vector<int64_t>{1000, 0});
BENCHMARK_MAIN();