#include <SFML/Graphics.hpp>
#include <time.h>
#include <iostream>
#include <string>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <mutex>
#include <semaphore.h>
#include "maze.h"
using namespace std;
using namespace sf;

char signal_input = '\0';
mutex mtx;

void displaycoins(RenderWindow& window, CircleShape coins)
{
    RectangleShape wall(Vector2f(20, 20));
    wall.setFillColor(sf::Color::Blue); // Set fill color

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            if (grid[i][j] == 1)
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

void* main_thread_funct(void* arg) 
{
    RenderWindow* windowPtr = static_cast<RenderWindow*>(arg);
    RenderWindow& window = *windowPtr;

    CircleShape  coins;
    coins.setRadius(3);
    coins.setFillColor(Color::Cyan);
    //Texture gridimage;
    // gridimage.loadFromFile("background.png");
    // Sprite sprite(gridimage);

    // 1. LOOP MAIN 5/
    bool shouldExit = false;
    while (window.isOpen() && !shouldExit)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
                shouldExit = true;;
            }
        }
        // Clear the window
        window.clear();
        displaycoins(window, coins);
        window.display();
    }  
    
    pthread_exit(0);
}

void* UI_thread_funct(void* arg)
{
    RenderWindow* windowPtr = static_cast<RenderWindow*>(arg);
    RenderWindow& window = *windowPtr;

    // 1. LOOP MAIN 5/
    //while (1) 
    {
            if (Keyboard::isKeyPressed(Keyboard::Left)) //If left key is pressed
                {
        //      lock_guard<mutex> lock(mtx);
                signal_input = 'D'; 
                }
        if (Keyboard::isKeyPressed(Keyboard::Right)) // If right key is pressed
                {
        //     lock_guard<mutex> lock(mtx);
                signal_input = 'A'; 
                }   
        if (Keyboard::isKeyPressed(Keyboard::Up)) //If up key is pressed
            {
            ///    lock_guard<mutex> lock(mtx);
                signal_input = 'W'; 
                }
        if (Keyboard::isKeyPressed(Keyboard::Down)) // If down key is pressed
            {
        ///     lock_guard<mutex> lock(mtx);
                signal_input = 'S'; 
                }          
    }
    return nullptr;
}

int main() 
{
    //1. IMPLEMENT SFML 
    // Create a window
    sf::RenderWindow window(sf::VideoMode(800, 800), "PAC-MAN");

    //2. CREATE/CALL MAIN THREAD 
    pthread_t mainengine;
    // Create a thread and call main_thread_funct as the thread function
    pthread_create(&mainengine, nullptr, &main_thread_funct, (void*)&window);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }
    }
    
    // Join the thread
    int returnValue;
    pthread_join(mainengine, NULL);


    //2. CREATE/CALL UI THREAD 
    //   pthread_t uithread;
    // Create a thread and call main_thread_funct as the thread function
    //  pthread_create(&uithread, nullptr, &UI_thread_funct, &window);
    // Join the thread
    // pthread_join(uithread, nullptr);

    // Create a circle shape
    //   sf::CircleShape circle(100.f);
    //   circle.setFillColor(sf::Color::Red);
    //  circle.setPosition(350.f, 250.f); // Position the circle at the center of the window

    // Main loop

    // pthread_exit(0);
    return 0;
}
