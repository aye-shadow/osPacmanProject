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
int uSleepTime = 450, score = 0, lives = 3;
int uSleep = 100000;
float respawntime = 3.0f;
float shield = 0.0f;
bool respawned = false;
bool pauseGameBool = false, needsKey[] = {1, 1, 1, 1}, needsPermit[] = {1, 1, 1, 1}, keyAvailable = true, permitAvailable = true;

// PRODUCER (another thread) - CONSUMER (ScoreThread)
//  power pallet number = 4 in grid
mutex consumed;
int pallet_size = 4, pallet_count = 4;
sf::Clock clock1, clock2;
float PowerPalletTime = 10.0f; // Loop duration in seconds
float ConsumptionTime = 0.0f;
bool allow = true;

//READER - WRITER
int readers = 0;
bool tickets[4] = {0,0,0,0};
int priorities[4] = {0,0,0,0};

//GHOST HOUSE 
bool key[5] = {0,0,0,0,1};


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
    sf::Color color;
    int ghostGridX;
    int ghostGridY;
    Texture t;
    Sprite ghostSprite;
    bool insideHome = true;
    bool eaten = false;
    bool gohome = false;
    ghostStruct()
    {
        t.loadFromFile("hi.png");
        sf::Vector2u ts = t.getSize();
        ghostSprite.setTexture(t);
        float scaleX = 40.f / ts.x;
        float scaleY = 40.f / ts.y;
        ghostSprite.setScale(scaleX, scaleY);
    }

    bool colide() // pacman will eat ghost
    {
        if (ghostSprite.getGlobalBounds().intersects(pacman.pacmanSprite.getGlobalBounds()) && !this->eaten)
        {
            (this->eaten = true);
            gohome = true;
            return true;
        }
        return false;
    }
    bool collide() // this will eat pacman
    {
        if (ghostSprite.getGlobalBounds().intersects(pacman.pacmanSprite.getGlobalBounds()) && allow && !respawned)
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
    bool isInHome() 
    {
        if (ghostGridX >= 12 && ghostGridX <= 18 && ghostGridY >= 14 && ghostGridY <= 18)
        {
            return true;
        }
        else 
        {
            return false;
        }
    }

} ghost1, ghost2, ghost3, ghost4;

int m_p()
{
  int index = -1, max=0;
  for(int i=0; i<4; i++)
  {
    if(max < priorities[i])
       {
        index =i; max = priorities[i];
       }
  }
return index;
}

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
    wall.setFillColor(Color{131, 74, 51, 255}); // Set fill color
    coins.setFillColor(Color::Yellow);
    CircleShape pallet;
    pallet.setRadius(5);
    pallet.setFillColor(Color::Magenta);

    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            if (grid[i][j] == 4)
            {
                pallet.setPosition(j * 25 + 21, i * 25 + 21);
                window.draw(pallet);
            }
            if (grid[i][j] == 1) // coins display
            {
                coins.setPosition(j * 25 + 21, i * 25 + 21);
                window.draw(coins);
            }
            else if (grid[i][j] == 0) // walls display
            {
                wall.setPosition(j * 25 + 15, i * 25 + 15);
                window.draw(wall);
            }
        }
    }
}

void *producepallet(void *arg)
{
    while (!shouldExit)
    {
        while (pallet_count >= pallet_size)
        { // do nothing
        }
        consumed.lock();

        int i = rand() % 30, j = rand() % 30;
        while (grid[i][j] == 0)
        {
            i = rand() % 30, j = rand() % 30;
        }
        grid[i][j] = 4; // updating grid uhm uhm - yeh nahi hona chahiye idher
        pallet_count++;
        consumed.unlock();
    }

    pthread_exit(0);
}

void *incScore(void *arg)
{
    while (!shouldExit)
    {
        if(!tickets[0] && !tickets[1]&& !tickets[2] && !tickets[3])
         {       
        // READING the grid to see if coin is there
        while (pallet_count == 0) // checking for empty buffer
        {           
            if (grid[pacman.pacmanGridY][pacman.pacmanGridX] == 1)
            { // normal coins collection               
                grid[pacman.pacmanGridY][pacman.pacmanGridX] = 2;
                ++score;               
            }          
        }
        
        // consume -> pallet
        if (grid[pacman.pacmanGridY][pacman.pacmanGridX] == 4 && allow)
        {
            consumed.lock(); // mutex lock
            ConsumptionTime = 0.0f;
            pallet_count--;
            allow = false;            
            grid[pacman.pacmanGridY][pacman.pacmanGridX] = 2;
            score += 20;
            sf::Color c = Color{0, 150, 255, 255};
            ghost1.ghostSprite.setColor(c);
            ghost2.ghostSprite.setColor(c);
            ghost3.ghostSprite.setColor(c);
            ghost4.ghostSprite.setColor(c);
        }

            //  CRITICAL SECTION 
            if (!allow) // jab tak doosra pallet lena is not allowed
            {
                ConsumptionTime += clock1.restart().asSeconds();
                // leave critical section
                if (ConsumptionTime >= PowerPalletTime)
                {
                    ConsumptionTime = 0.0f;
                    consumed.unlock(); // mutex unlock
                    ghost1.ghostSprite.setColor(ghost1.color);
                    ghost2.ghostSprite.setColor(ghost2.color);
                    ghost3.ghostSprite.setColor(ghost3.color);
                    ghost4.ghostSprite.setColor(ghost4.color);
                    allow = true;
                }
                if (ghost1.colide() || ghost2.colide() || ghost3.colide() || ghost4.colide())
                    score += 200;
            }
        // just eat the coins man       
        if (grid[pacman.pacmanGridY][pacman.pacmanGridX] == 1)
        { // normal coins collection
            grid[pacman.pacmanGridY][pacman.pacmanGridX] = 2;
            ++score;           
        }

        for(int i = 0; i < 4; i++)
        { tickets[i] = 1; priorities[i] = i+1;}
         }
    usleep(uSleepTime);
    }
    pthread_exit(0);
}

void *mainthread(void *p)
{
    pacman.pacmanGridX = 29;
    pacman.pacmanGridY = 27;
    pacman.x = 25 * pacman.pacmanGridX + 9;
    pacman.y = 25 * pacman.pacmanGridY + 9;
    pacman.pacmanSprite.setPosition(pacman.x, pacman.y);

    while (!shouldExit)
    {
        ghost1.ghostSprite.setPosition(ghost1.x, ghost1.y);
        ghost2.ghostSprite.setPosition(ghost2.x, ghost2.y);
        ghost3.ghostSprite.setPosition(ghost3.x, ghost3.y);
        ghost4.ghostSprite.setPosition(ghost4.x, ghost4.y);
        
        if (!respawned)
        {
            ghost1.collide();
            ghost2.collide();
            ghost3.collide();
            ghost4.collide();
        }
        usleep(uSleep);
    }
    resetMaze();
    pthread_exit(0);
}

void *sfmlWindow(void *p)
{

    RenderWindow window(VideoMode(1030, 750), "PAC-MAN");
    window.setPosition(Vector2i(380, 180));
    CircleShape coins;
    coins.setRadius(3);
    coins.setFillColor(Color::Cyan);

    Font font;
    font.loadFromFile("Alegreya-SemiBold.ttf");
    Text text;
    text.setFont(font);
    text.setCharacterSize(24);
    text.setFillColor(Color::White);

    Text scoreText("Score: ", font, 30), scoreIntText(to_string(score), font, 35);
    scoreText.setFillColor(Color::White);
    scoreText.setPosition(810, 10);
    scoreIntText.setFillColor(Color::White);
    scoreIntText.setPosition(scoreText.getPosition().x + scoreText.getLocalBounds().width, 10);

    Text scText("Pallets: ", font, 30), scIntText(to_string(pallet_count), font, 30);
    scText.setFillColor(Color::White);
    scText.setPosition(810, 45);
    scIntText.setFillColor(Color::White);
    scIntText.setPosition(scText.getPosition().x + scText.getLocalBounds().width, 45);

    Text gameover("Game Over :(", font, 30);
    gameover.setFillColor(Color::Magenta);
    gameover.setPosition(810, 75);

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

        // Check INPUTS
        //  while (window.pollEvent(event))
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
        // window.clear();

        displaycoins(window, coins);

        window.draw(text);

        window.draw(ghost1.ghostSprite);
        window.draw(ghost2.ghostSprite);
        window.draw(ghost3.ghostSprite);
        window.draw(ghost4.ghostSprite);

        window.draw(pacman.pacmanSprite);

        window.draw(scoreText);
        scoreIntText.setString(to_string(score));
        window.draw(scoreIntText);

        if (lives <= 0)
        {
            window.draw(gameover);
        }

        window.draw(scText);
        scIntText.setString(to_string(pallet_count));
        window.draw(scIntText);

        for (int i = 0; i < lives; i++)
        {
            hearts.setPosition(820 + i * 60, 100);
            window.draw(hearts);
        }
        window.display();
        // window2.display();

        usleep(uSleep);
    }
    window.close();
    // window2.close();
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

        // Dead -- with shield on
        if (respawned)
        {
            shield += clock2.restart().asSeconds();
            if (shield >= respawntime)
            {
                respawned = false;
                shield = 0;
            }
        }
                pacman.pacmanSprite.setPosition(pacman.x, pacman.y);

        usleep(uSleep);
    }
    pthread_exit(0);
}




void ghostup(ghostStruct &ghost)
{
    --ghost.ghostGridY;
    ghost.x = 25 * ghost.ghostGridX + 10;
    ghost.y = 25 * ghost.ghostGridY + 10;
    usleep(uSleepTime + 50000);
}
void ghostdown(ghostStruct &ghost)
{
    ++ghost.ghostGridY;
    ghost.x = 25 * ghost.ghostGridX + 10;
    ghost.y = 25 * ghost.ghostGridY + 10;
    usleep(uSleepTime + 50000);
}
void ghostleft(ghostStruct &ghost)
{
    --ghost.ghostGridX;
    ghost.x = 25 * ghost.ghostGridX + 10;
    ghost.y = 25 * ghost.ghostGridY + 10;

    if (ghost.ghostGridX == 0)
    {
        cout << "TELE1" << endl;
        ghost.ghostGridX = cols - 1;
        ghost.ghostGridY += 2;
        ghost.x = 25 * ghost.ghostGridX + 10;
        ghost.y = 25 * ghost.ghostGridY + 10;
    }
    usleep(uSleepTime + 50000);
}
void ghostright(ghostStruct &ghost)
{
    ++ghost.ghostGridX;
    ghost.x = 25 * ghost.ghostGridX + 10;
    ghost.y = 25 * ghost.ghostGridY + 10;   
    if (ghost.ghostGridX == cols - 1)
    {
        cout << "TELE2" << endl;
        ghost.ghostGridX = 0;
        ghost.ghostGridY -= 2;
        ghost.x = 25 * ghost.ghostGridX + 10;
        ghost.y = 25 * ghost.ghostGridY + 10;
    }
    usleep(uSleepTime + 50000);
}

int semi_Ai(ghostStruct &ghost, int prev, pair<int, int> pref)
{
    bool av[4] = {0, 0, 0, 0}; // up, down, left , right
                               // if (pref.first != 0 || pref.second != 0)
    {
        // available and preferred
        if (grid[ghost.ghostGridY - 1][ghost.ghostGridX] != 0 && prev != 1)
        {
            av[0] = 1;
            if (pref.first > 0) // available and preffered
            {
                //  go up
                ghostup(ghost);
                return 0;
            }
        }

        if (grid[ghost.ghostGridY][ghost.ghostGridX - 1] != 0 && prev != 3)
        {
            av[2] = 1;
            if (pref.second > 0) // available and preffered
            {
                //  go left
                ghostleft(ghost);
                return 2;
            }
        }
        if (grid[ghost.ghostGridY + 1][ghost.ghostGridX] != 0 && prev != 0)
        {
            av[1] = 1;
            if (pref.first < 0) // available and preffered
            {
                //  go down
                ghostdown(ghost);
                return 1;
            }
        }
        if (grid[ghost.ghostGridY][ghost.ghostGridX + 1] != 0 && prev != 2)
        {
            av[3] = 1;
            if (pref.second < 0) // available and preffered
            {
                //  go right
                ghostright(ghost);
                return 3;
            }
        }

        // else -> just available
        if (av[0] && prev != 1)
        {
            ghostup(ghost);
            return 0;
        }
        else if (av[1] && prev != 0)
        {
            //  go down
            ghostdown(ghost);
            return 1;
        }
        else if (av[2] && prev != 3)
        {
            //  go left
            ghostleft(ghost);
            return 2;
        }
        else
        { //  go right
            ghostright(ghost);
            return 3;
        }
    }
}

int semi_Ai1(ghostStruct &ghost, int prev, pair<int, int> pref)
{
    bool av[4] = {0, 0, 0, 0}; // up, down, left , right
                               // if (pref.first != 0 || pref.second != 0)
    {
        // available and preferred
       
        if (grid[ghost.ghostGridY + 1][ghost.ghostGridX] != 0 && prev != 0)
        {
            av[1] = 1;
            if (pref.first < 0) // available and preffered
            {
                //  go down
                ghostdown(ghost);
                return 1;
            }
        }

            if (grid[ghost.ghostGridY][ghost.ghostGridX + 1] != 0 && prev != 2)
        {
            av[3] = 1;
            if (pref.second < 0) // available and preffered
            {
                //  go right
                ghostright(ghost);
                return 3;
            }
        }
  
        if (grid[ghost.ghostGridY][ghost.ghostGridX - 1] != 0 && prev != 3)
        {
            av[2] = 1;
            if (pref.second > 0) // available and preffered
            {
                //  go left
                ghostleft(ghost);
                return 2;
            }
        }
     
  if (grid[ghost.ghostGridY - 1][ghost.ghostGridX] != 0 && prev != 1)
        {
            av[0] = 1;
            if (pref.first > 0) // available and preffered
            {
                //  go up
                ghostup(ghost);
                return 0;
            }
        }

        // else -> just available
        if (av[0] && prev != 1)
        {
            ghostup(ghost);
            return 0;
        }
       
        else if (av[2] && prev != 3)
        {
            //  go left
            ghostleft(ghost);
            return 2;
        }       
       
        else if (av[1] && prev != 0)
        {
            //  go down
            ghostdown(ghost);
            return 1;
        }
       
        else
        { //  go right
            ghostright(ghost);
            return 3;
        }
    }
}

int go_home(ghostStruct &ghost, int prev)
{
    // check for preferences  home = ( 13, 15 )
    pair<int, int> pref = make_pair(ghost.ghostGridY - 13, ghost.ghostGridX - 15);
    bool av[4] = {0, 0, 0, 0}; // up, down, left , right

    if (pref.first != 0 || pref.second != 0)
    {
        // ghost.ghostSprite.setColor(Color{255,255,0,200});
        ghost.ghostSprite.setColor(Color::White);
        return semi_Ai(ghost, prev, pref);
    }
    else
    { // reached home
        ghost.eaten = false;
        ghost.ghostSprite.setColor(ghost.color);
        ghostdown(ghost);
        return 1;
    }
}

int random_move(ghostStruct &ghost, int prevDirec)
{
    int prevDirc, directionInt = 0, actualMovement = 0;

    if (ghost.insideHome && ghost.ghostGridX == 15 && ghost.ghostGridY == 14)
    {
        // take that path and LEAVE (astaghfirullah that ghost is not a female bro. the ghost identify as a ghost)
        ghost.insideHome = false;
        // NEVER enter again. or else...
        // ghost is supposed to enter again yes.
    }

    directionInt = rand() % 4;

    if (directionInt == 0 && prevDirec != 1) // up
    {
        while (grid[ghost.ghostGridY - 1][ghost.ghostGridX] != 0)
        {
            ghostup(ghost);
            prevDirec = 0;

            if (grid[ghost.ghostGridY][ghost.ghostGridX + 1] != 1 || grid[ghost.ghostGridY][ghost.ghostGridX - 1] != 1)
            {
                break;
            }
        }
    }
    else if (directionInt == 1 && prevDirec != 0) // down
    {
        while (grid[ghost.ghostGridY + 1][ghost.ghostGridX] != 0)
        {
            if (ghost.ghostGridY + 1 == 14 && ghost.ghostGridX == 15)
            {
                break;
            }
            ghostdown(ghost);
            prevDirec = 1;

            if (grid[ghost.ghostGridY][ghost.ghostGridX + 1] != 1 || grid[ghost.ghostGridY][ghost.ghostGridX - 1] != 1)
            {
                break;
            }
        }
    }
    else if (directionInt == 2 && prevDirec != 3) // left
    {

        while (ghost.ghostGridX != 0 && grid[ghost.ghostGridY][ghost.ghostGridX - 1] != 0)
        {
            ghostleft(ghost);
            prevDirec = 2;

            if (grid[ghost.ghostGridY + 1][ghost.ghostGridX] != 1 || grid[ghost.ghostGridY - 1][ghost.ghostGridX] != 1)
            {
                break;
            }
        }
    }
    else if (directionInt == 3 && prevDirec != 2) // right
    {

        while (ghost.ghostGridX != cols - 1 && grid[ghost.ghostGridY][ghost.ghostGridX + 1] != 0)
        {
            ghostright(ghost);
            prevDirec = 3;

            if (grid[ghost.ghostGridY + 1][ghost.ghostGridX] != 1 || grid[ghost.ghostGridY - 1][ghost.ghostGridX] != 1)
            {
                break;
            }
        }
    }

    return prevDirec;
}

int leaveHouse(int ghostID, int pr, ghostStruct &ghost)
{
    int p = pr;
   if(key[4] && key[ghostID+1])
   {
       int x = 15, y = 12;
       key[4] = 0;
     pair<int, int> pref = make_pair(ghost.ghostGridY - y, ghost.ghostGridX - x);

        while(ghost.ghostGridY - y !=0 ||  ghost.ghostGridX - x != 0 )
        {
        pref = make_pair(ghost.ghostGridY - y, ghost.ghostGridX - x);
        p = semi_Ai(ghost, p, pref);
        }
       key[ghostID] = 1;
       key[4] =1;
   }

   else 
   {
        p = random_move(ghost, pr);
   }

return p;
}

void *ghost1Movement(void *arg)
{
    ghost1.color = (Color::Magenta);
    ghost1.ghostGridX = 15;
    ghost1.ghostGridY = 15;
    ghost1.x = 25 * ghost1.ghostGridX + 15;
    ghost1.y = 25 * ghost1.ghostGridY + 15;
    ghost1.ghostSprite.setColor(Color::Magenta);
    ghost1.ghostSprite.setPosition(ghost1.x, ghost1.y);

    int prevDirec, directionInt = 0, actualMovement = 0;

    while (!shouldExit)
    {
        if( tickets[0])
     {
        if(ghost1.isInHome())
        {
         prevDirec = leaveHouse(0,prevDirec, ghost1);
        }
         else if (!ghost1.eaten)
        {
            prevDirec = random_move(ghost1, prevDirec);
        }
        else
        {
            // just go to home
            prevDirec = go_home(ghost1, prevDirec);
        }
       priorities[0] = 0; 
       tickets[0] = 0; 
    }
    
    }

    pthread_exit(0);
}

void *ghost2Movement(void *arg)
{
    ghost2.color = (Color::Red);
    ghost2.ghostGridX = 15;
    ghost2.ghostGridY = 15;
    ghost2.x = 25 * ghost2.ghostGridX + 15;
    ghost2.y = 25 * ghost2.ghostGridY + 15;
    ghost2.ghostSprite.setColor(Color{250, 0, 0, 255});
    ghost2.ghostSprite.setPosition(ghost2.x, ghost2.y);

    int prevDirec, directionInt = 0, actualMovement = 0;

    while (!shouldExit)
    {
      if( tickets[1])
     {
        if(ghost2.isInHome())
        {
         prevDirec = leaveHouse(1,prevDirec, ghost2);
        }
        else
        if (!ghost2.eaten)//&& !respawned)
        {
            pair<int, int> pref = make_pair(ghost2.ghostGridY - pacman.pacmanGridY, ghost2.ghostGridX - pacman.pacmanGridX);
            prevDirec = semi_Ai(ghost2, prevDirec, pref);
        }
        else
        {
            // just go to home
            prevDirec = go_home(ghost2, prevDirec);
        }
        tickets[1]=0;
        priorities[1]=0;
     }
    }

    pthread_exit(0);
}

void *ghost3Movement(void *arg)
{
    ghost3.color = (Color::Cyan);
    ghost3.ghostGridX = 16;
    ghost3.ghostGridY = 15;
    ghost3.x = 25 * ghost3.ghostGridX + 15;
    ghost3.y = 25 * ghost3.ghostGridY + 15;
    ghost3.ghostSprite.setColor(Color::Cyan);
    ghost3.ghostSprite.setPosition(ghost3.x, ghost3.y);

    int prevDirec, directionInt = 0, actualMovement = 0;

    while (!shouldExit)
    {
         if(tickets[2])
     {
        if(ghost3.isInHome())
        {
         prevDirec = leaveHouse(2,prevDirec, ghost3);
        }
         else
        if (!ghost3.eaten)
        {
            prevDirec = random_move(ghost3, prevDirec);
        }
        else
        {
            // just go to home
            prevDirec = go_home(ghost3, prevDirec);
        }
        tickets[2] =0;
        priorities[2]=0;
     }
    }
    pthread_exit(0);
}

void *ghost4Movement(void *arg)
{
    ghost4.color = (Color::Yellow);
    ghost4.ghostGridX = 15;
    ghost4.ghostGridY = 16;
    ghost4.x = 25 * ghost4.ghostGridX + 15;
    ghost4.y = 25 * ghost4.ghostGridY + 15;
    ghost4.ghostSprite.setColor(Color::Yellow);
    ghost4.ghostSprite.setPosition(ghost4.x, ghost4.y);

    int prevDirec =0, directionInt = 0, actualMovement = 0;

    while (!shouldExit)
    {
         if( tickets[3])
     {
        if(ghost4.isInHome())
        {
         prevDirec = leaveHouse(3,prevDirec, ghost4);
        }
        else
        if (!ghost4.eaten )//&& !respawned)
        {
            pair<int, int> pref = make_pair(ghost4.ghostGridY - pacman.pacmanGridY, ghost4.ghostGridX - pacman.pacmanGridX);
            prevDirec = semi_Ai1(ghost4, prevDirec, pref);
        }
        else
        {
            // just go to home
            prevDirec = go_home(ghost4, prevDirec);
        }
        tickets[3]=0;
        priorities[3]=0;
     }
    }
    pthread_exit(0);
}

int main()
{
    srand(time(0));

    mutex only1GhostCanLeave;
    int gID1 = 1, gID2 = 2, gID3 = 3, gID4 = 4;

    pthread_t pacmanThread, ghost1Thread, ghost2Thread, ghost3Thread, ghost4Thread, sfmlThread;
    pthread_t main_thread;
    pthread_create(&main_thread, nullptr, &mainthread, nullptr);
    pthread_create(&sfmlThread, nullptr, &sfmlWindow, nullptr);
    pthread_create(&pacmanThread, nullptr, &pacmanMovement, nullptr);

    // ghost with random movemnt
    pthread_create(&ghost1Thread, nullptr, &ghost1Movement, nullptr);

    pthread_create(&ghost2Thread, nullptr, &ghost2Movement, nullptr);
    pthread_create(&ghost3Thread, nullptr, &ghost3Movement, nullptr);
    pthread_create(&ghost4Thread, nullptr, &ghost4Movement, nullptr);
    pthread_join(main_thread, NULL);
    pthread_join(sfmlThread, nullptr);
    pthread_join(pacmanThread, NULL);
    pthread_join(ghost1Thread, nullptr);
    pthread_join(ghost2Thread, nullptr);
    pthread_join(ghost3Thread, nullptr);
    pthread_join(scoreThread, nullptr);

    return 0;
}
