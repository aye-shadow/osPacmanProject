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

pthread_t scoreThread, ProducePallets;
bool shouldExit = false;
string action = "";
char prevDirection = ' ', directon[] = {'U', 'D', 'R', 'L'};
int uSleepTime = 100000, score = 0, lives =3;
float respawntime = 3.0f;
float shield = 0.0f;
bool respawned = false; 
bool pauseGameBool = false, needsKey[] = {1, 1, 1, 1}, needsPermit[] = {1, 1, 1, 1}, keyAvailable = true, permitAvailable = true;

//PRODUCER (another thread) - CONSUMER (ScoreThread)
// power pallet number = 4 in grid 
mutex consumed;
int pallet_size = 4, pallet_count = 4;
sf::Clock clock1;
float PowerPalletTime = 10.0f; // Loop duration in seconds
float ConsumptionTime = 0.0f; 
bool allow = true;


struct pacmanStruct
{
    int x;
    int y;
    int pacmanGridX;
    int pacmanGridY;
    char direction;
    Sprite pacmanSprite;
    Texture t;
    pacmanStruct()
    {
        t.loadFromFile("G.png");
        sf::Vector2u ts = t.getSize();       
        pacmanSprite.setTexture(t); 
        float scaleX = 30.f / ts.x;
        float scaleY = 30.f / ts.y;
        pacmanSprite.setScale(scaleX, scaleY);
    }
} pacman;

struct ghostStruct
{
    int x;
    int y;
    int ghostGridX;
    int ghostGridY;
    //RectangleShape ghostSprite;
    Texture t;
    Sprite  ghostSprite;
    bool insideHome = true;
    bool eaten = false;
    ghostStruct()
    {
        t.loadFromFile("hi.png");
        sf::Vector2u ts = t.getSize();       
        ghostSprite.setTexture(t); 
        float scaleX = 40.f / ts.x;
        float scaleY = 40.f / ts.y;
        ghostSprite.setScale(scaleX, scaleY);
    }

    bool colide()  // pacman will eat ghost
    {
       if(ghostSprite.getGlobalBounds().intersects(pacman.pacmanSprite.getGlobalBounds()) && !this->eaten)
       {
        ghostSprite.setColor(Color::White);
        (this->eaten = true); 
        return true;           
       }
       return false;
    }
    bool collide() // this will eat pacman 
    {
        if(ghostSprite.getGlobalBounds().intersects(pacman.pacmanSprite.getGlobalBounds()) && allow && !respawned)
       {
        lives--; 
        pacman.pacmanGridX = 29;
        pacman.pacmanGridY = 27;
        pacman.pacmanSprite.setPosition(pacman.x, pacman.y);
        respawned = true; 
        return true;           
       }
       return false;
    }
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
    coins.setFillColor(Color::Yellow);
    CircleShape pallet;
    pallet.setRadius(5);
    pallet.setFillColor(Color::Magenta);

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            if(grid[i][j]==4)
            {
                pallet.setPosition(j * 25 + 21, i * 25 + 21);
                window.draw(pallet);
            }
            if (grid[i][j] == 1)  //coins display
            {
                coins.setPosition(j * 25 + 21, i * 25 + 21);
                window.draw(coins);
            }
            else if (grid[i][j] == 0)  // walls display 
            {
                wall.setPosition(j * 25 + 15, i * 25 + 15);
                window.draw(wall);
            }
        }
    }
}

void eatpallet()
{
    if (grid[pacman.pacmanGridY][pacman.pacmanGridX] == 1){// normal coins collection
            grid[pacman.pacmanGridY][pacman.pacmanGridX] = 2;
            ++score;}
    if(ghost1.colide()) score += 200; 
    
    
}
void *producepallet(void *arg)
{
    while(!shouldExit)
    {
    while(pallet_count >= pallet_size)
        {  // do nothing 
        }
    consumed.lock();

    int i =rand()%30 , j =rand()%30;
     while(grid[i][j] == 0 )
     {
        i =rand()%30 , j =rand()%30;
     }
      grid[i][j] = 4;  // updating grid uhm uhm - yeh nahi hona chahiye idher 
      pallet_count++;

      consumed.unlock();
    }
    
pthread_exit(0);
}

void *incScore(void *arg)
{
    while (!shouldExit)
    {
        // READING the grid to see if coin is there
        while (pallet_count == 0)  // checking for empty buffer 
        {if (grid[pacman.pacmanGridY][pacman.pacmanGridX] == 1){// normal coins collection
            grid[pacman.pacmanGridY][pacman.pacmanGridX] = 2; ++score;}
        }
        // consume -> pallet
        if (grid[pacman.pacmanGridY][pacman.pacmanGridX] == 4 && allow)
        {         
            consumed.lock();   // mutex lock          
            ConsumptionTime = 0.0f;
            pallet_count--;
            allow = false; 
            grid[pacman.pacmanGridY][pacman.pacmanGridX] = 2;
            score += 20; 
            ghost1.ghostSprite.setColor(Color::Cyan); 
        //  critical section  
        while(!allow)  // jab tak doosra pallet lena is not allowed 
        {  
        ConsumptionTime += clock1.restart().asSeconds();       
         
           // leave critical section 
        if (ConsumptionTime >= PowerPalletTime)
        {
            ConsumptionTime = 0.0f;
            consumed.unlock();    // mutex unlock 
            ghost1.ghostSprite.setColor(Color::Magenta);
            allow = true;
        }           
           eatpallet();
        }
        } 

        // just eat the coins man 
        if (grid[pacman.pacmanGridY][pacman.pacmanGridX] == 1){// normal coins collection
            grid[pacman.pacmanGridY][pacman.pacmanGridX] = 2;
            ++score;}      
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
    pacman.x = 25 * pacman.pacmanGridX + 9;
    pacman.y = 25 * pacman.pacmanGridY + 9;
    //pacman.pacmanSprite.setRadius(13);
    //pacman.pacmanSprite.setColor(Color::Yellow);
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

    Text scText("Powerpallets: ", font, 20), scIntText(to_string(pallet_count), font, 20);
    scText.setFillColor(Color::White);
    scText.setPosition(28, 28);
    scIntText.setFillColor(Color::White);
    scIntText.setPosition(scText.getPosition().x + scText.getLocalBounds().width, 28);

    Text gameover("Game Over :(", font, 20);
    gameover.setFillColor(Color::Magenta);
    gameover.setPosition(38,38);

    Texture live;
    live.loadFromFile("heart.png");
    Sprite hearts;
    hearts.setTexture(live);
    sf::Vector2u lie = live.getSize();
   float scaleX = 50.f / lie.x;
   float scaleY = 50.f / lie.y;
   hearts.setScale(scaleX, scaleY);

    pthread_create(&ProducePallets, nullptr, &producepallet, nullptr);
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

        if(lives <= 0 )
        {window2.draw(gameover);}

        window2.draw(scText);
        scIntText.setString(to_string(pallet_count));
        window2.draw(scIntText);
        
        for(int i=0; i <lives; i++)
        {
            hearts.setPosition(0+ i*50, 750.0f);
            window.draw(hearts);
        }

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
        pacman.x = 25 * pacman.pacmanGridX + 9;
        pacman.y = 25 * pacman.pacmanGridY + 9;
        pacman.direction = prevDirection;

        while(respawned)
        {
            shield += clock1.restart().asSeconds();  
            if(shield >= respawntime)
            {   respawned = false;
               shield = 0;}
        }

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

void gohome(ghostStruct &ghost)
{
    // check for preferences 
  int rit = ghost.ghostGridX - 13;
  int up = ghost.ghostGridY - 15;
  if(up>0)  // up is prefered
  {
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
   // ghost1.ghostSprite.setSize(Vector2f(20, 20));
    ghost1.ghostSprite.setColor(Color::Magenta); // Set color to pink
    ghost1.ghostSprite.setPosition(ghost1.x, ghost1.y);

    int prevDirec, directionInt = 0, actualMovement = 0;

    while (!shouldExit)
    {
       // if(!ghost1.eaten)
        {
         ghost1.collide();
        if (ghost1.insideHome && ghost1.ghostGridX == 15 && ghost1.ghostGridY == 14)
        {
            // take that path and LEAVE (astaghfirullah that ghost is not a female bro. the ghost identify as a ghost)
            ghost1.insideHome = false;
            // NEVER enter again. or else...
            // ghost is supposed to enter again yes. 
        }

        directionInt = rand() % 4;

        if (directionInt == 0 && prevDirec != 1) // up
        {
            while (grid[ghost1.ghostGridY - 1][ghost1.ghostGridX] != 0)
            {
                --ghost1.ghostGridY;
                ghost1.x = 25 * ghost1.ghostGridX + 10;
                ghost1.y = 25 * ghost1.ghostGridY + 10;
                prevDirec = 0;
                usleep(uSleepTime + 50000);

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
                ghost1.x = 25 * ghost1.ghostGridX + 10;
                ghost1.y = 25 * ghost1.ghostGridY + 10;
                prevDirec = 1;
                usleep(uSleepTime + 50000);

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
                ghost1.x = 25 * ghost1.ghostGridX + 10;
                ghost1.y = 25 * ghost1.ghostGridY + 10;
                prevDirec = 2;
                usleep(uSleepTime + 50000);
            }

            while (ghost1.ghostGridX != 0 && grid[ghost1.ghostGridY][ghost1.ghostGridX - 1] != 0)
            {
                --ghost1.ghostGridX;
                ghost1.x = 25 * ghost1.ghostGridX + 10;
                ghost1.y = 25 * ghost1.ghostGridY + 10;
                prevDirec = 2;
                usleep(uSleepTime + 50000);

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
                ghost1.x = 25 * ghost1.ghostGridX + 10;
                ghost1.y = 25 * ghost1.ghostGridY + 10;
                prevDirec = 3;
                usleep(uSleepTime + 50000);
            }

            while (ghost1.ghostGridX != cols - 1 && grid[ghost1.ghostGridY][ghost1.ghostGridX + 1] != 0)
            {
                ++ghost1.ghostGridX;
                ghost1.x = 25 * ghost1.ghostGridX + 10;
                ghost1.y = 25 * ghost1.ghostGridY + 10;
                prevDirec = 3;
                usleep(uSleepTime + 50000);

                if (grid[ghost1.ghostGridY + 1][ghost1.ghostGridX] != 1 || grid[ghost1.ghostGridY - 1][ghost1.ghostGridX] != 1) 
                {
                    break;
                }
            }
        }
        }
      //  else
        {
            // just go to home 
       //     gohome(ghost1);
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
