#include <SFML/Graphics.hpp>
#include <time.h>
#include <iostream>
#include <string>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <mutex>
#include <semaphore.h>
#include <chrono>
#include <thread>
#include "maze.h"
using namespace std;
using namespace sf;

char signal_input = '\0';
bool shouldExit = false;
string action = "";
char prevDirection = ' ';
int uSleepTime = 150000;

struct pacmanStruct {
    int x;
    int y;
    int pacmanGridX;
    int pacmanGridY;
    char direction;
    CircleShape pacmanSprite;
} pacman;

struct ghostStruct {
    int x;
    int y;
    RectangleShape ghostSprite;
} ghost1;

void displaycoins(RenderWindow& window, CircleShape coins)
{
    RectangleShape wall(Vector2f(20, 20));
    wall.setFillColor(sf::Color::Blue); // Set fill color

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            if (grid[i][j] == 1 && (i != 27 || j != 29))
            {
                coins.setPosition(j*25 + 15, i*25 + 15);
                window.draw(coins);
            }
            else if (grid[i][j] == 0)
            {
                wall.setPosition(j*25 + 15, i*25 + 15);
                window.draw(wall);
            }
        }
    }
}

void* sfmlWindow(void *p) {
    sf::RenderWindow window(sf::VideoMode(800, 800), "PAC-MAN");
    sf::RenderWindow window2(sf::VideoMode(800, 800), "Input Handling");

    window.setPosition(Vector2i(100, 100));
    window2.setPosition(Vector2i(1000, 100));

    ghost1.x = 25*13 + 15;
    ghost1.y = 25*15 + 15;
    ghost1.ghostSprite.setSize(Vector2f(20, 20));
    ghost1.ghostSprite.setFillColor(sf::Color::Magenta); // Set color to pink
    ghost1.ghostSprite.setPosition(ghost1.x, ghost1.y);

    pacman.pacmanGridX = 29;
    pacman.pacmanGridY = 27;
    pacman.x = 25*pacman.pacmanGridX + 15;
    pacman.y = 25*pacman.pacmanGridY + 15;
    pacman.pacmanSprite.setRadius(13);
    pacman.pacmanSprite.setFillColor(Color::Yellow);
    pacman.pacmanSprite.setPosition(pacman.x, pacman.y);

    CircleShape coins;
    coins.setRadius(3);
    coins.setFillColor(Color::Cyan);

    Font font;
    font.loadFromFile("Alegreya-SemiBold.ttf");

    Text text;
    text.setFont(font);
    text.setCharacterSize(24);
    text.setFillColor(Color::White);

    while (!shouldExit) {
        Event event;
        prevDirection = pacman.direction;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                shouldExit = true;
            }
        }

        // Check events for window 2
        while (window2.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                shouldExit = true;
            }
            // Handle keypress events for window 2
            if (event.type == sf::Event::KeyPressed) {
                // Example: Check if the key 'B' is pressed
                if (event.key.code == sf::Keyboard::Left) {
                    // Do something for window 2 when left key is pressed
                    pacman.direction = 'L';
                    action = "Left";
                    text.setString("Left");
                }
                else if(event.key.code == sf::Keyboard::Right) {
                    pacman.direction = 'R';
                    action = "Right";
                    text.setString("Right");

                } 
                else if(event.key.code == sf::Keyboard::Up) {
                    pacman.direction = 'U';
                    action = "Up";
                    text.setString("Up");
                }
                else if(event.key.code == sf::Keyboard::Down) {
                    pacman.direction = 'D';
                    action = "Down";
                    text.setString("Down");
                }
            }
        }

        window.clear();
        window2.clear();

        displaycoins(window, coins);
        window.draw(text);
        window.draw(ghost1.ghostSprite);

        pacman.pacmanSprite.setPosition(pacman.x, pacman.y);
        window.draw(pacman.pacmanSprite);

        window.display();
        window2.display();

        usleep(uSleepTime);
    }

    window.close();
    window2.close();

    pthread_exit(0);
}

void* main_thread_funct(void* arg) 
{
    // 1. LOOP MAIN 5/
    while (!shouldExit)
    {
        if (pacman.direction == 'U' && grid[pacman.pacmanGridY - 1][pacman.pacmanGridX] != 0) {
            prevDirection = pacman.direction;
            --pacman.pacmanGridY;
        }
        else if (pacman.direction == 'D' && grid[pacman.pacmanGridY + 1][pacman.pacmanGridX] != 0) {
            prevDirection = pacman.direction;
            ++pacman.pacmanGridY;
        }
        else if (pacman.direction == 'L') {
            if (pacman.pacmanGridX == 0) {
                pacman.pacmanGridX = cols - 1;
                pacman.pacmanGridY += 2;
                prevDirection = pacman.direction;
            } else if (grid[pacman.pacmanGridY][pacman.pacmanGridX - 1] != 0) {
                --pacman.pacmanGridX;
                prevDirection = pacman.direction;
            }
        }
        else if (pacman.direction == 'R') {
            if (pacman.pacmanGridX == cols - 1) {
                pacman.pacmanGridX = 0;
                pacman.pacmanGridY -= 2;
                prevDirection = pacman.direction;
            } else if (grid[pacman.pacmanGridY][pacman.pacmanGridX + 1] != 0) {
                ++pacman.pacmanGridX;
                prevDirection = pacman.direction;
            }
        }
        pacman.x = 25*pacman.pacmanGridX + 15;
        pacman.y = 25*pacman.pacmanGridY + 15;
        pacman.direction = prevDirection;

        usleep(uSleepTime);
    }  
    
    pthread_exit(0);
}

void* UI_thread_funct(void* arg)
{
    while (!shouldExit) 
    {
    }

    pthread_exit(0);
}

int main() 
{
    //1. IMPLEMENT SFML 
    // Create a window
    pthread_t mainengine, uithread, sfmlThread;
    pthread_create(&uithread, nullptr, &sfmlWindow, nullptr);
    pthread_create(&mainengine, nullptr, &main_thread_funct, nullptr);
    pthread_create(&sfmlThread, nullptr, &UI_thread_funct, nullptr);

    pthread_join(mainengine, NULL);
    pthread_join(uithread, nullptr);
    pthread_join(sfmlThread, nullptr);

    return 0;
}
