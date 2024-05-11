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

pthread_t scoreThread;
bool shouldExit = false;
string action = "";
char prevDirection = ' ', directon[] = {'U', 'D', 'R', 'L'};
int uSleepTime = 150000, score = 0;
bool pauseGameBool = false, needsKey[] = {1, 1, 1, 1}, needsPermit[] = {1, 1, 1, 1}, keyAvailable = true, permitAvailable = true;

struct pacmanStruct
{
    int x;
    int y;
    int pacmanGridX;
    int pacmanGridY;
    char direction;
    CircleShape pacmanSprite;
} pacman;

struct ghostStruct
{
    int x;
    int y;
    int ghostGridX;
    int ghostGridY;
    RectangleShape ghostSprite;
    bool insideHome = true;
} ghost1;

void resetMaze()
{
    for (int a = 0; a < rows; ++a)
    {
        for (int b = 0; b < cols; ++b)
        {
            if (grid[a][b] == 2)
            {
                grid[a][b] = 1;
            }
        }
    }
}

void displaycoins(RenderWindow &window, CircleShape coins)
{
    RectangleShape wall(Vector2f(20, 20));
    wall.setFillColor(Color::Blue); // Set fill color

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            if (grid[i][j] == 1)
            {
                coins.setPosition(j * 25 + 15, i * 25 + 15);
                window.draw(coins);
            }
            else if (grid[i][j] == 0)
            {
                wall.setPosition(j * 25 + 15, i * 25 + 15);
                window.draw(wall);
            }
        }
    }
}

void *incScore(void *arg)
{
    while (!shouldExit)
    {
        // READING the grid to see if coin is there
        if (grid[pacman.pacmanGridY][pacman.pacmanGridX] == 1)
        {
            grid[pacman.pacmanGridY][pacman.pacmanGridX] = 2;
            ++score;
        }
    }

    pthread_exit(0);
}

void *sfmlWindow(void *p)
{
    RenderWindow window(VideoMode(800, 800), "PAC-MAN");
    RenderWindow window2(VideoMode(800, 800), "Input Handling");

    window.setPosition(Vector2i(100, 100));
    window2.setPosition(Vector2i(1000, 100));

    pacman.pacmanGridX = 29;
    pacman.pacmanGridY = 27;
    pacman.x = 25 * pacman.pacmanGridX + 15;
    pacman.y = 25 * pacman.pacmanGridY + 15;
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

    Text scoreText("Score: ", font, 20), scoreIntText(to_string(score), font, 20);
    scoreText.setFillColor(Color::White);
    scoreText.setPosition(10, 10);
    scoreIntText.setFillColor(Color::White);
    scoreIntText.setPosition(scoreText.getPosition().x + scoreText.getLocalBounds().width, 10);

    pthread_create(&scoreThread, nullptr, &incScore, nullptr);

    while (!shouldExit)
    {
        Event event;
        prevDirection = pacman.direction;

        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                shouldExit = true;
            }
        }

        // Check events for window 2
        while (window2.pollEvent(event))
        {
            if (event.type == Event::Closed)
            {
                shouldExit = true;
            }
            // Handle keypress events for window 2
            if (event.type == Event::KeyPressed)
            {
                // Example: Check if the key 'B' is pressed
                if (event.key.code == Keyboard::Left)
                {
                    // Do something for window 2 when left key is pressed
                    pacman.direction = 'L';
                    action = "Left";
                    text.setString("Left");
                }
                else if (event.key.code == Keyboard::Right)
                {
                    pacman.direction = 'R';
                    action = "Right";
                    text.setString("Right");
                }
                else if (event.key.code == Keyboard::Up)
                {
                    pacman.direction = 'U';
                    action = "Up";
                    text.setString("Up");
                }
                else if (event.key.code == Keyboard::Down)
                {
                    pacman.direction = 'D';
                    action = "Down";
                    text.setString("Down");
                }
            }
        }

        window.clear();
        window2.clear();

        pacman.pacmanSprite.setPosition(pacman.x, pacman.y);
        ghost1.ghostSprite.setPosition(ghost1.x, ghost1.y);
        displaycoins(window, coins);

        window.draw(text);
        window.draw(ghost1.ghostSprite);
        window.draw(pacman.pacmanSprite);

        window2.draw(scoreText);
        scoreIntText.setString(to_string(score));
        window2.draw(scoreIntText);

        window.display();
        window2.display();

        usleep(uSleepTime);
    }

    window.close();
    window2.close();

    resetMaze();

    pthread_exit(0);
}

void *pacmanMovement(void *arg)
{
    // 1. LOOP MAIN 5/
    while (!shouldExit)
    {
        if (pacman.direction == 'U' && grid[pacman.pacmanGridY - 1][pacman.pacmanGridX] != 0)
        {
            prevDirection = pacman.direction;
            --pacman.pacmanGridY;
        }
        else if (pacman.direction == 'D' && grid[pacman.pacmanGridY + 1][pacman.pacmanGridX] != 0)
        {
            prevDirection = pacman.direction;
            ++pacman.pacmanGridY;
        }
        else if (pacman.direction == 'L')
        {
            if (pacman.pacmanGridX == 0)
            {
                pacman.pacmanGridX = cols - 1;
                pacman.pacmanGridY += 2;
                prevDirection = pacman.direction;
            }
            else if (grid[pacman.pacmanGridY][pacman.pacmanGridX - 1] != 0)
            {
                --pacman.pacmanGridX;
                prevDirection = pacman.direction;
            }
        }
        else if (pacman.direction == 'R')
        {
            if (pacman.pacmanGridX == cols - 1)
            {
                pacman.pacmanGridX = 0;
                pacman.pacmanGridY -= 2;
                prevDirection = pacman.direction;
            }
            else if (grid[pacman.pacmanGridY][pacman.pacmanGridX + 1] != 0)
            {
                ++pacman.pacmanGridX;
                prevDirection = pacman.direction;
            }
        }
        pacman.x = 25 * pacman.pacmanGridX + 15;
        pacman.y = 25 * pacman.pacmanGridY + 15;
        pacman.direction = prevDirection;

        usleep(uSleepTime);
    }

    pthread_exit(0);
}

void leaveHouse(int ghostID)
{
    while (!keyAvailable)
    {
    };

    if (needsKey[ghostID - 1] == true)
    {
        keyAvailable = false;
        needsKey[ghostID - 1] = false;

        while (!permitAvailable)
        {
        };

        if (needsPermit[ghostID - 1] == true)
        {
            permitAvailable = false;
            needsPermit[ghostID - 1] = false;
        }

        // has key and permit
    }
}

void *ghost1Movement(void *arg)
{
    // int* ghost = (int*)arg;
    // int& ghostID = *ghost;
    // leaveHouse(ghostID);

    ghost1.ghostGridX = 13;
    ghost1.ghostGridY = 15;
    ghost1.x = 25 * ghost1.ghostGridX + 15;
    ghost1.y = 25 * ghost1.ghostGridY + 15;
    ghost1.ghostSprite.setSize(Vector2f(20, 20));
    ghost1.ghostSprite.setFillColor(Color::Magenta); // Set color to pink
    ghost1.ghostSprite.setPosition(ghost1.x, ghost1.y);

    int prevDirec, directionInt = 0, actualMovement = 0;

    while (!shouldExit)
    {
        if (ghost1.insideHome && ghost1.ghostGridX == 15 && ghost1.ghostGridY == 14)
        {
            // take that path and LEAVE BITCH
            ghost1.insideHome = false;
            // NEVER enter again. or else...
        }

        directionInt = rand() % 4;

        if (directionInt == 0 && prevDirec != 1) // up
        {
            while (grid[ghost1.ghostGridY - 1][ghost1.ghostGridX] != 0)
            {
                --ghost1.ghostGridY;
                ghost1.x = 25 * ghost1.ghostGridX + 15;
                ghost1.y = 25 * ghost1.ghostGridY + 15;
                prevDirec = 0;
                usleep(uSleepTime);

                if (grid[ghost1.ghostGridY][ghost1.ghostGridX + 1] != 1 || grid[ghost1.ghostGridY][ghost1.ghostGridX - 1] != 1) 
                {
                    break;
                }
            }
        }
        else if (directionInt == 1 && prevDirec != 0) // down
        {
            while (grid[ghost1.ghostGridY + 1][ghost1.ghostGridX] != 0)
            {
                if (ghost1.ghostGridY + 1 == 14 && ghost1.ghostGridX == 15) 
                {
                    break;
                }

                ++ghost1.ghostGridY;
                ghost1.x = 25 * ghost1.ghostGridX + 15;
                ghost1.y = 25 * ghost1.ghostGridY + 15;
                prevDirec = 1;
                usleep(uSleepTime);

                if (grid[ghost1.ghostGridY][ghost1.ghostGridX + 1] != 1 || grid[ghost1.ghostGridY][ghost1.ghostGridX - 1] != 1) 
                {
                    break;
                }
            }
        }
        else if (directionInt == 2 && prevDirec != 3) // left
        {
            if (ghost1.ghostGridX == 0) 
            {
                ghost1.ghostGridX = cols - 1;
                ghost1.ghostGridY += 2;
                ghost1.x = 25 * ghost1.ghostGridX + 15;
                ghost1.y = 25 * ghost1.ghostGridY + 15;
                prevDirec = 2;
                usleep(uSleepTime);
            }

            while (ghost1.ghostGridX != 0 && grid[ghost1.ghostGridY][ghost1.ghostGridX - 1] != 0)
            {
                --ghost1.ghostGridX;
                ghost1.x = 25 * ghost1.ghostGridX + 15;
                ghost1.y = 25 * ghost1.ghostGridY + 15;
                prevDirec = 2;
                usleep(uSleepTime);

                if (grid[ghost1.ghostGridY + 1][ghost1.ghostGridX] != 1 || grid[ghost1.ghostGridY - 1][ghost1.ghostGridX] != 1) 
                {
                    break;
                }
            }
        }
        else if (directionInt == 3 && prevDirec != 2) // right
        {
            if (ghost1.ghostGridX == cols - 1) 
            {
                cout << "TELE2" << endl;
                ghost1.ghostGridX = 0;
                ghost1.ghostGridY -= 2;
                ghost1.x = 25 * ghost1.ghostGridX + 15;
                ghost1.y = 25 * ghost1.ghostGridY + 15;
                prevDirec = 3;
                usleep(uSleepTime);
            }

            while (ghost1.ghostGridX != cols - 1 && grid[ghost1.ghostGridY][ghost1.ghostGridX + 1] != 0)
            {
                ++ghost1.ghostGridX;
                ghost1.x = 25 * ghost1.ghostGridX + 15;
                ghost1.y = 25 * ghost1.ghostGridY + 15;
                prevDirec = 3;
                usleep(uSleepTime);

                if (grid[ghost1.ghostGridY + 1][ghost1.ghostGridX] != 1 || grid[ghost1.ghostGridY - 1][ghost1.ghostGridX] != 1) 
                {
                    break;
                }
            }
        }
    }

    pthread_exit(0);
}

void *ghost2Movement(void *arg)
{

    pthread_exit(0);
}

void *ghost3Movement(void *arg)
{

    pthread_exit(0);
}

void *ghost4Movement(void *arg)
{

    pthread_exit(0);
}

int main()
{
    srand(time(0));

    mutex only1GhostCanLeave;

    int gID1 = 1, gID2 = 2, gID3 = 3, gID4 = 4;

    pthread_t pacmanThread, ghost1Thread, ghost2Thread, ghost3Thread, ghost4Thread, sfmlThread;
    pthread_create(&sfmlThread, nullptr, &sfmlWindow, nullptr);
    pthread_create(&pacmanThread, nullptr, &pacmanMovement, nullptr);

    // ghost with random movemnt
    pthread_create(&ghost1Thread, nullptr, &ghost1Movement, nullptr);

    // pthread_create(&ghost2Thread, nullptr, &ghost2Movement, (void *)&gID2);
    // pthread_create(&ghost3Thread, nullptr, &ghost3Movement, (void *)&gID3);
    // pthread_create(&ghost4Thread, nullptr, &ghost4Movement, (void *)&gID4);

    pthread_join(sfmlThread, nullptr);
    pthread_join(pacmanThread, NULL);
    pthread_join(ghost1Thread, nullptr);
    pthread_join(scoreThread, nullptr);

    return 0;
}
