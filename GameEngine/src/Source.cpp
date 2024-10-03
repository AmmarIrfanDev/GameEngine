#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "Texture.h"

#include <iostream>
#include <chrono>
#include <cstdint>
#include <cfloat>
#include <thread>
#include <type_traits>
#include <cmath>
#include <algorithm>
#include <variant>
#include <map>
#include <any>
#include <utility>
#include <unordered_map>

bool quit_menu = false;

// Main loop flag
bool quit = true;

double roundToSignificantFigures(double num, int n) 
{
    if (num == 0.0) return 0.0; // Zero check
    // Calculate the scaling factor based on significant figures
    double magnitude = std::pow(10.0, n - std::ceil(std::log10(std::abs(num))));
    // Scale, round, and scale back
    return std::round(num * magnitude) / magnitude;
}

typedef Uint8 ColourT;

class StaticEntity;
class Entity;

class Player
{
public:
    // Power Ups:
    bool TripleJump;

    // position is bottom-left
    long long int x; // x: 0 to 192,000
    long long int y; // y: 0 to 108,000

    long long x_texture_offset;
    long long y_texture_offset;
    long long sizex_texture_offset;
    long long sizey_texture_offset;

    int jump_number; // allows double/triple jump

    long long int x_before;
    long long int y_before;

    double acceleration;
    double velocity;
    double velocity_pp_collision;
    double velocity_max; // technically speed (because it is scalar)

    double gravity_acceleration; // downward acceleration due to gravity
    double verticle_nongravity_acceleration;
    double verticle_velocity;
    double verticle_velocity_max; // terminal velocity

    unsigned int sizeX, sizeY;

    Uint8 Colour[4];

    Texture* texture = nullptr;

    void CollisionsOn();

    void CollisionsOff();

    Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity);

    Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity, const std::string& texturePath);

    Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity, const std::string& texturePath,
        int numFrames, int frameWidth, int frameHeight, int frameTime, int frameGap, int xOffset, int yOffset, int xEndOffset, int yEndOffset); // animated

    Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity, int resourceID, bool mapbg);

    Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity, ColourT r, ColourT g, ColourT b, ColourT a);

    ~Player();

    void draw_tex(long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add);

    void draw(long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add);

private:
    HMODULE* hModule = nullptr;
};

SDL_Renderer* renderer = nullptr;

enum EntityType : uint8_t
{
    ENTITY = 1,
    STATIC_ENTITY,
    PLAYER
};

std::vector<std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType>> AllEntities;

std::vector<StaticEntity*> StaticEntityCollisions;
std::vector<Player*> PlayerCollisions;
std::vector<Entity*> EntityCollisions;

std::vector<Player*> AllPlayers;

template<std::intmax_t FPS>
class Timer {
public:
    // initialize Timer with the current time point:
    Timer() : tp{ std::chrono::steady_clock::now() } {}

    // a fixed duration with a length of 1/FPS seconds
    static constexpr std::chrono::duration<double, std::ratio<1, FPS>>
        time_between_frames{ 1 };

    void sleep() {
        // add to the stored time point
        tp += time_between_frames;

        // and sleep almost until the new time point
        std::this_thread::sleep_until(tp - std::chrono::microseconds(100));

        // less than 100 microseconds busy wait
        while (std::chrono::steady_clock::now() < tp) {}
    }

private:
    // the time point we'll add to in every loop
    std::chrono::time_point<std::chrono::steady_clock,
        std::remove_const_t<decltype(time_between_frames)>> tp;
};


int SCREEN_X = 1920/2;
int SCREEN_Y = 1080/2;

const int max_X = 192000;
const int max_Y = 108000;

long long cameraX = 0;
long long cameraY = 0;
double camera_magnification = 1.00;

// Function to calculate squared distance between two players
long long squared_distance(Player* p1, Player* p2) {
    long long dx = p1->x - p2->x;
    long long dy = p1->y - p2->y;
    return dx * dx + dy * dy;
}

std::pair<long long, long long> find_midpoint()
{
    // Ensure there is at least one player
    if (AllPlayers.empty()) {
        return { 0, 0 }; // Default to origin if no players
    }

    double sum_x = 0;
    double sum_y = 0;
    int num_players_inscreen = AllPlayers.size();

    // Calculate the sum of all player coordinates
    for (const auto& player : AllPlayers) 
    {
        if (player->y_before > -75000)
        {
            sum_x += player->x_before + (player->sizeX / 2);
            sum_y += player->y_before + (player->sizeY / 2);
        }
        else
        {
            num_players_inscreen -= 1;
        }
    }

    // Calculate the mean midpoint
    long long mean_mid_x = sum_x / num_players_inscreen;
    long long mean_mid_y = sum_y / num_players_inscreen;

    return { mean_mid_x, mean_mid_y };
}

// Function to find the longest distance between any two players
double findLongestDistance() 
{
    double longestDistance = 0.0;

    // Iterate through all pairs of players
    for (size_t i = 0; i < AllPlayers.size(); ++i) {
        for (size_t j = i + 1; j < AllPlayers.size(); ++j) {
            // Calculate distance between AllPlayers[i] and AllPlayers[j]
            long long dx = AllPlayers[j]->x - AllPlayers[i]->x;
            long long dy = AllPlayers[j]->y - AllPlayers[i]->y;
            double distance = std::sqrt(dx * dx + dy * dy);

            // Update longestDistance if current distance is greater
            if (distance > longestDistance) {
                longestDistance = distance;
            }
        }
    }

    return longestDistance;
}

// Global variables for smooth camera movement
double targetCameraX = 0;
double targetCameraY = 0;
double cameraMoveSpeed = 0.01; // Adjust the speed of camera movement

void draw_original(bool tex, Texture* texture, ColourT Colour[4], long long x_in, long long y_in, long long sizeX_in, long long sizeY_in, 
    long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add)
{
    double sizeX, sizeY;
    sizeX = double(sizeX_in) * camera_magnification;
    sizeY = double(sizeY_in) * camera_magnification;

    double x, y;
    x = double(x_in) * camera_magnification;
    y = double(y_in) * camera_magnification;

    if (!quit) // not in menu
    {
        std::pair<long long, long long> mid = find_midpoint();

        // Update target camera position
        targetCameraX = (long long)((192000 / 2) - mid.first);
        targetCameraY = (long long)((108000 / 2) - mid.second);

        // Smoothly move the camera towards the target position
        cameraX += (targetCameraX - cameraX) * cameraMoveSpeed;
        if (targetCameraX - cameraX < 200)
        {
            cameraX = targetCameraX;
        }
        cameraY += (targetCameraY - cameraY) * cameraMoveSpeed;
        if (targetCameraY - cameraY < 200)
        {
            cameraY = targetCameraY;
        }
    }

    if (!tex)
    {
        SDL_SetRenderDrawColor(renderer, Colour[0], Colour[1], Colour[2], Colour[3]);
        SDL_Rect squareRect = {
             ((double(x + cameraX) / double(max_X)) * SCREEN_X),
             (((double(y + sizeY + cameraY) / double(max_Y)) * SCREEN_Y) - (SCREEN_Y)) * -1,
             (double(sizeX) / double(max_X)) * SCREEN_X,
             (double(sizeY) / double(max_Y)) * SCREEN_Y
        };
        SDL_RenderFillRect(renderer, &squareRect);
    }
    else if (tex)
    {
        texture->render(
            renderer,
             (double(x + x_in_add + cameraX) / double(max_X)) * SCREEN_X,
             ((((double(y + y_in_add + cameraY) + double(sizeY)) / double(max_Y)) * SCREEN_Y) - SCREEN_Y) * -1,
             (double(sizeX + sizex_in_add) / double(max_X)) * SCREEN_X,
             (double(sizeY + sizey_in_add) / double(max_Y)) * SCREEN_Y
        );
    }
}

void Player::CollisionsOn()
{
    PlayerCollisions.push_back(this);
}

void Player::CollisionsOff()
{
    auto it = std::find(PlayerCollisions.begin(), PlayerCollisions.end(), this);
    if (it != PlayerCollisions.end())
    {
        PlayerCollisions.erase(it);  // This removes the element at 'it'
    }
}

Player::Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity)
{

    x_texture_offset = 0;
    y_texture_offset = 0;
    sizex_texture_offset = 0;
    sizey_texture_offset = 0;

    TripleJump = false;

    velocity = 0;
    velocity_max = MaximumVelocity;

    jump_number = 0;

    gravity_acceleration = -60;
    verticle_nongravity_acceleration = 0;
    verticle_velocity = 0;
    verticle_velocity_max = -5000;
    velocity_pp_collision = 0;

    Colour[0] = 0xFF;
    Colour[1] = 0x00;
    Colour[2] = 0x00;
    Colour[3] = 0xFF; // red

    x = X_POS;
    y = Y_POS;

    x_before = X_POS;
    y_before = Y_POS;

    sizeX = SizeX;
    sizeY = SizeY;

    EntityType ThisType = PLAYER;
    std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
    AllEntities.push_back(pushback);

    AllPlayers.push_back(this);
}

Player::Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity, const std::string& texturePath) // Texture
{

    x_texture_offset = 0;
    y_texture_offset = 0;
    sizex_texture_offset = 0;
    sizey_texture_offset = 0;

    texture = new Texture();
    texture->loadFromFile(renderer, texturePath);

    TripleJump = false;

    velocity = 0;
    velocity_max = MaximumVelocity;

    jump_number = 0;

    gravity_acceleration = -60;
    verticle_nongravity_acceleration = 0;
    verticle_velocity = 0;
    verticle_velocity_max = -5000;
    velocity_pp_collision = 0;

    Colour[0] = 0xFF;
    Colour[1] = 0x00;
    Colour[2] = 0x00;
    Colour[3] = 0xFF; // red

    x = X_POS;
    y = Y_POS;

    x_before = X_POS;
    y_before = Y_POS;

    sizeX = SizeX;
    sizeY = SizeY;

    EntityType ThisType = PLAYER;
    std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
    AllEntities.push_back(pushback);

    AllPlayers.push_back(this);
}

Player::Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity, const std::string& texturePath,
    int numFrames, int frameWidth, int frameHeight, int frameTime, int frameGap, int xOffset, int yOffset, int xEndOffset, int yEndOffset) // animated
{

    x_texture_offset = 0;
    y_texture_offset = 0;
    sizex_texture_offset = 0;
    sizey_texture_offset = 0;

    texture = new Texture();
    texture->loadFromFile(renderer, texturePath);
    texture->setAnimationFrames(numFrames, frameWidth, frameHeight, frameTime, frameGap, xOffset, yOffset, xEndOffset, yEndOffset);

    TripleJump = false;

    velocity = 0;
    velocity_max = MaximumVelocity;

    jump_number = 0;

    gravity_acceleration = -60;
    verticle_nongravity_acceleration = 0;
    verticle_velocity = 0;
    verticle_velocity_max = -5000;
    velocity_pp_collision = 0;

    Colour[0] = 0xFF;
    Colour[1] = 0x00;
    Colour[2] = 0x00;
    Colour[3] = 0xFF; // red

    x = X_POS;
    y = Y_POS;

    x_before = X_POS;
    y_before = Y_POS;

    sizeX = SizeX;
    sizeY = SizeY;

    EntityType ThisType = PLAYER;
    std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
    AllEntities.push_back(pushback);

    AllPlayers.push_back(this);
}


Player::Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity, int resourceID, bool mapbg)
{
    x_texture_offset = 0;
    y_texture_offset = 0;
    sizex_texture_offset = 0;
    sizey_texture_offset = 0;

    texture = new Texture();

    if (mapbg)
    {
        hModule = new HMODULE;
        if (resourceID > 100 && resourceID < 151)
        {
            *hModule = LoadLibrary(L"mapbg.dll");
        }
        else if (resourceID > 150 && resourceID < 201)
        {
            *hModule = LoadLibrary(L"mapbg2.dll");
        }
        else if (resourceID > 200 && resourceID < 251)
        {
            *hModule = LoadLibrary(L"mapbg3.dll");
        }
        else if (resourceID > 250 && resourceID < 301)
        {
            *hModule = LoadLibrary(L"mapbg4.dll");
        }
        else if (resourceID > 300 && resourceID < 351)
        {
            *hModule = LoadLibrary(L"mapbg5.dll");
        }

        if (hModule == NULL)
        {
            std::cerr << "Failed to load DLL!" << std::endl;
        }
        if (!texture->loadFromResourceDLL(renderer, *hModule, resourceID))
        {
            std::cerr << "Failed to load texture from DLL!" << std::endl;
        }
    }
    else
    {
        texture->loadFromResource(renderer, resourceID);
    }

    TripleJump = false;

    velocity = 0;
    velocity_max = MaximumVelocity;

    jump_number = 0;

    gravity_acceleration = -60;
    verticle_nongravity_acceleration = 0;
    verticle_velocity = 0;
    verticle_velocity_max = -5000;
    velocity_pp_collision = 0;

    Colour[0] = 0xFF;
    Colour[1] = 0x00;
    Colour[2] = 0x00;
    Colour[3] = 0xFF; // red

    x = X_POS;
    y = Y_POS;

    x_before = X_POS;
    y_before = Y_POS;

    sizeX = SizeX;
    sizeY = SizeY;

    EntityType ThisType = PLAYER;
    std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
    AllEntities.push_back(pushback);

    AllPlayers.push_back(this);
}

Player::Player(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, double MaximumVelocity, ColourT r, ColourT g, ColourT b, ColourT a) // Colour
{

    x_texture_offset = 0;
    y_texture_offset = 0;
    sizex_texture_offset = 0;
    sizey_texture_offset = 0;

    TripleJump = false;

    velocity = 0;
    velocity_max = MaximumVelocity;

    jump_number = 0;

    gravity_acceleration = -60;
    verticle_nongravity_acceleration = 0;
    verticle_velocity = 0;
    verticle_velocity_max = -5000;
    velocity_pp_collision = 0;

    Colour[0] = r;
    Colour[1] = g;
    Colour[2] = b;
    Colour[3] = a;

    x = X_POS;
    y = Y_POS;

    sizeX = SizeX;
    sizeY = SizeY;

    EntityType ThisType = ENTITY;
    std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
    AllEntities.push_back(pushback);

    AllPlayers.push_back(this);
}

Player::~Player()
{
    if (hModule != nullptr)
    {
        FreeLibrary(*hModule);
        delete hModule;
    }

    if (texture != nullptr)
    {
        delete texture;
    }

    EntityType ThisType = PLAYER;
    std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
    auto it = std::find(AllEntities.begin(), AllEntities.end(), pushback);

    // If the element was found, remove it
    if (it != AllEntities.end()) {
        AllEntities.erase(it);
    }

    auto it2 = std::find(AllPlayers.begin(), AllPlayers.end(), this);

    // If the element was found, remove it
    if (it2 != AllPlayers.end()) {
        AllPlayers.erase(it2);
    }
}

void Player::draw_tex(long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add)
{
    draw_original(true, texture, Colour, x, y, sizeX, sizeY, x_in_add, y_in_add, sizex_in_add, sizey_in_add);
}

void Player::draw(long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add)
{
    draw_original(false, nullptr, Colour, x, y, sizeX, sizeY, x_in_add, y_in_add, sizex_in_add, sizey_in_add);
}

class Entity
{
private:
    HMODULE* hModule = nullptr;
public:
    // position is bottom-left
    long long int x; // x: 0 to 192,000
    long long int y; // y: 0 to 108,000

    unsigned int sizeX, sizeY;


    long long x_texture_offset;
    long long y_texture_offset;
    long long sizex_texture_offset;
    long long sizey_texture_offset;

    Uint8 Colour[4];

    Texture* texture = nullptr;

    void CollisionsOn()
    {
        EntityCollisions.push_back(this);
    }

    void CollisionsOff()
    {
        auto it = std::find(EntityCollisions.begin(), EntityCollisions.end(), this);
        if (it != EntityCollisions.end())
        {
            EntityCollisions.erase(it);  // This removes the element at 'it'
        }
    }

    Entity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY)
    {

        x_texture_offset = 0;
        y_texture_offset = 0;
        sizex_texture_offset = 0;
        sizey_texture_offset = 0;

        Colour[0] = 0xFF; 
        Colour[1] = 0x00;
        Colour[2] = 0x00;
        Colour[3] = 0xFF; // red

        x = X_POS;
        y = Y_POS;
        sizeX = SizeX;
        sizeY = SizeY;

        EntityType ThisType = ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        AllEntities.push_back(pushback);
    }

    Entity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, const std::string& texturePath)
        : texture(new Texture())
    {

        x_texture_offset = 0;
        y_texture_offset = 0;
        sizex_texture_offset = 0;
        sizey_texture_offset = 0;

        texture->loadFromFile(renderer, texturePath);

        Colour[0] = 0xFF;
        Colour[1] = 0x00;
        Colour[2] = 0x00;
        Colour[3] = 0xFF; // red

        x = X_POS;
        y = Y_POS;
        sizeX = SizeX;
        sizeY = SizeY;

        EntityType ThisType = ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        AllEntities.push_back(pushback);
    }

    Entity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, int resourceID, bool mapbg)
        : texture(new Texture())
    {

        x_texture_offset = 0;
        y_texture_offset = 0;
        sizex_texture_offset = 0;
        sizey_texture_offset = 0;

        if (mapbg)
        {
            hModule = new HMODULE;
            if (resourceID > 100 && resourceID < 151)
            {
                *hModule = LoadLibrary(L"mapbg.dll");
            }
            else if (resourceID > 150 && resourceID < 201)
            {
                *hModule = LoadLibrary(L"mapbg2.dll");
            }
            else if (resourceID > 200 && resourceID < 251)
            {
                *hModule = LoadLibrary(L"mapbg3.dll");
            }
            else if (resourceID > 250 && resourceID < 301)
            {
                *hModule = LoadLibrary(L"mapbg4.dll");
            }
            else if (resourceID > 300 && resourceID < 351)
            {
                *hModule = LoadLibrary(L"mapbg5.dll");
            }

            if (hModule == NULL)
            {
                std::cerr << "Failed to load DLL!" << std::endl;
            }
            if (!texture->loadFromResourceDLL(renderer, *hModule, resourceID))
            {
                std::cerr << "Failed to load texture from DLL!" << std::endl;
            }
        }
        else
        {
            texture->loadFromResource(renderer, resourceID);
        }

        Colour[0] = 0xFF;
        Colour[1] = 0x00;
        Colour[2] = 0x00;
        Colour[3] = 0xFF; // red

        x = X_POS;
        y = Y_POS;
        sizeX = SizeX;
        sizeY = SizeY;

        EntityType ThisType = ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        AllEntities.push_back(pushback);
    }

    Entity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, ColourT r, ColourT g, ColourT b, ColourT a)
    {

        x_texture_offset = 0;
        y_texture_offset = 0;
        sizex_texture_offset = 0;
        sizey_texture_offset = 0;

        Colour[0] = r;
        Colour[1] = g;
        Colour[2] = b;
        Colour[3] = a;

        x = X_POS;
        y = Y_POS;
        sizeX = SizeX;
        sizeY = SizeY;

        EntityType ThisType = ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        AllEntities.push_back(pushback);
    }

    ~Entity()
    {
        if (hModule != nullptr)
        {
            FreeLibrary(*hModule);
            delete hModule;
        }

        if (texture != nullptr)
        {
            delete texture;
        }

        EntityType ThisType = ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        auto it = std::find(AllEntities.begin(), AllEntities.end(), pushback);

        // If the element was found, remove it
        if (it != AllEntities.end()) {
            AllEntities.erase(it);
        }
    }

    void draw_tex(long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add)
    {
        draw_original(true, texture, Colour, x, y, sizeX, sizeY, x_in_add, y_in_add, sizex_in_add, sizey_in_add);
    }

    void draw(long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add)
    {
        draw_original(false, nullptr, Colour, x, y, sizeX, sizeY, x_in_add, y_in_add, sizex_in_add, sizey_in_add);
    }

private:

};

class StaticEntity
{
private:
    HMODULE* hModule = nullptr;

public:
    // position is bottom-left
    long long int x; // x: 0 to 192,000
    long long int y; // y: 0 to 108,000

    unsigned int sizeX, sizeY;

    long long x_texture_offset;
    long long y_texture_offset;
    long long sizex_texture_offset;
    long long sizey_texture_offset;


    Uint8 Colour[4];

    Texture* texture = nullptr;


    void CollisionsOn()
    {
        auto it = std::find(StaticEntityCollisions.begin(), StaticEntityCollisions.end(), this);
        if (it == StaticEntityCollisions.end())
        {
            StaticEntityCollisions.push_back(this);
        }
    }

    void CollisionsOff()
    {
        auto it = std::find(StaticEntityCollisions.begin(), StaticEntityCollisions.end(), this);
        if (it != StaticEntityCollisions.end())
        {
            StaticEntityCollisions.erase(it);  // This removes the element at 'it'
        }
    }

    StaticEntity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY)
    {

        x_texture_offset = 0;
        y_texture_offset = 0;
        sizex_texture_offset = 0;
        sizey_texture_offset = 0;

        Colour[0] = 0x00;
        Colour[1] = 0x00;
        Colour[2] = 0xFF;
        Colour[3] = 0xFF; // blue

        x = X_POS;
        y = Y_POS;
        sizeX = SizeX;
        sizeY = SizeY;

        EntityType ThisType = STATIC_ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        AllEntities.push_back(pushback);
    }

    StaticEntity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, const std::string& texturePath)
        : texture(new Texture())
    {

        x_texture_offset = 0;
        y_texture_offset = 0;
        sizex_texture_offset = 0;
        sizey_texture_offset = 0;

        texture->loadFromFile(renderer, texturePath);

        Colour[0] = 0x00;
        Colour[1] = 0x00;
        Colour[2] = 0xFF;
        Colour[3] = 0xFF; // blue

        x = X_POS;
        y = Y_POS;
        sizeX = SizeX;
        sizeY = SizeY;

        EntityType ThisType = STATIC_ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        AllEntities.push_back(pushback);
    }

    StaticEntity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, int resourceID, bool mapbg)
        : texture(new Texture())
    {

        x_texture_offset = 0;
        y_texture_offset = 0;
        sizex_texture_offset = 0;
        sizey_texture_offset = 0;

        if (mapbg)
        {
            hModule = new HMODULE;
            if (resourceID > 100 && resourceID < 151)
            {
                *hModule = LoadLibrary(L"mapbg.dll");
            }
            else if (resourceID > 150 && resourceID < 201)
            {
                *hModule = LoadLibrary(L"mapbg2.dll");
            }
            else if (resourceID > 200 && resourceID < 251)
            {
                *hModule = LoadLibrary(L"mapbg3.dll");
            }
            else if (resourceID > 250 && resourceID < 301)
            {
                *hModule = LoadLibrary(L"mapbg4.dll");
            }
            else if (resourceID > 300 && resourceID < 351)
            {
                *hModule = LoadLibrary(L"mapbg5.dll");
            }

            if (hModule == NULL)
            {
                std::cerr << "Failed to load DLL!" << std::endl;
            }
            if (!texture->loadFromResourceDLL(renderer, *hModule, resourceID))
            {
                std::cerr << "Failed to load texture from DLL!" << std::endl;
            }
        }
        else
        {
            texture->loadFromResource(renderer, resourceID);
        }

        Colour[0] = 0x00;
        Colour[1] = 0x00;
        Colour[2] = 0xFF;
        Colour[3] = 0xFF; // blue

        x = X_POS;
        y = Y_POS;
        sizeX = SizeX;
        sizeY = SizeY;

        EntityType ThisType = STATIC_ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        AllEntities.push_back(pushback);
    }

    StaticEntity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, ColourT r, ColourT g, ColourT b, ColourT a)
    {

        x_texture_offset = 0;
        y_texture_offset = 0;
        sizex_texture_offset = 0;
        sizey_texture_offset = 0;

        Colour[0] = r;
        Colour[1] = g;
        Colour[2] = b;
        Colour[3] = a;

        x = X_POS;
        y = Y_POS;
        sizeX = SizeX;
        sizeY = SizeY;

        EntityType ThisType = STATIC_ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        AllEntities.push_back(pushback);
    }

    ~StaticEntity()
    {

        if (hModule != nullptr)
        {
            FreeLibrary(*hModule);
            delete hModule;
        }

        if (texture != nullptr)
        {
            delete texture;
        }

        EntityType ThisType = STATIC_ENTITY;
        std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType> pushback(this, ThisType);
        auto it = std::find(AllEntities.begin(), AllEntities.end(), pushback);

        // If the element was found, remove it
        if (it != AllEntities.end()) {
            AllEntities.erase(it);
        }
    }

    void draw_tex(long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add)
    {
        draw_original(true, texture, Colour, x, y, sizeX, sizeY, x_in_add, y_in_add, sizex_in_add, sizey_in_add);
    }

    void draw(long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add)
    {
        draw_original(false, nullptr, Colour, x, y, sizeX, sizeY, x_in_add, y_in_add, sizex_in_add, sizey_in_add);
    }

private:

};

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

void physics()
{
    for (int i1 = 0; i1 < PlayerCollisions.size(); ++i1) {
        bool inCollision = false; // Flag to check if the current player is in collision

        for (int i2 = 0; i2 < PlayerCollisions.size(); ++i2) {
            if (i1 != i2) {
                // Check for collisions in the x direction only
                if (PlayerCollisions[i1]->x < PlayerCollisions[i2]->x + PlayerCollisions[i2]->sizeX &&
                    PlayerCollisions[i1]->x + PlayerCollisions[i1]->sizeX > PlayerCollisions[i2]->x &&
                    PlayerCollisions[i1]->y < PlayerCollisions[i2]->y + PlayerCollisions[i2]->sizeY &&
                    PlayerCollisions[i1]->y + PlayerCollisions[i1]->sizeY > PlayerCollisions[i2]->y) {

                    // Players are colliding horizontally
                    inCollision = true;

                    // Calculate the overlap in the x direction
                    double overlapX = std::min(PlayerCollisions[i1]->x + PlayerCollisions[i1]->sizeX, PlayerCollisions[i2]->x + PlayerCollisions[i2]->sizeX) - std::max(PlayerCollisions[i1]->x, PlayerCollisions[i2]->x);

                    // Apply a gradual separation force
                    double separationForce = overlapX * 0.5; // Gradual separation force
                    double separationSpeed = 0.1; // Adjust the speed of separation

                    if (PlayerCollisions[i1]->x < PlayerCollisions[i2]->x) {
                        // Player i1 is on the left side of Player i2
                        PlayerCollisions[i1]->x -= separationForce * separationSpeed;
                        PlayerCollisions[i2]->x += separationForce * separationSpeed;
                    }
                    else {
                        // Player i1 is on the right side of Player i2
                        PlayerCollisions[i1]->x += separationForce * separationSpeed;
                        PlayerCollisions[i2]->x -= separationForce * separationSpeed;
                    }

                    // Apply a horizontal velocity response to each player
                    double relativeVelocityX = PlayerCollisions[i1]->velocity - PlayerCollisions[i2]->velocity;
                    double collisionForceX = relativeVelocityX * 0.05; // Simple collision force

                    PlayerCollisions[i1]->velocity -= collisionForceX;
                    PlayerCollisions[i2]->velocity += collisionForceX;
                }
            }
        }

        // Apply deceleration if not in collision
        if (!inCollision) {
            PlayerCollisions[i1]->velocity_pp_collision *= 0.9; // Rapidly reduce velocity
            if (abs(PlayerCollisions[i1]->velocity_pp_collision) < 0.1) {
                PlayerCollisions[i1]->velocity_pp_collision = 0;
            }
        }
    }

    for (int i1 = 0; i1 < AllPlayers.size(); ++i1)
    {

        // Horizontal Player Movement:

        AllPlayers[i1]->velocity += AllPlayers[i1]->acceleration;
        if (AllPlayers[i1]->velocity > 100)
        {
            AllPlayers[i1]->velocity -= 30; // How slippery the movement is
        }
        else if (AllPlayers[i1]->velocity > 0)
        {
            AllPlayers[i1]->velocity -= AllPlayers[i1]->velocity / 2;
        }
        else if (AllPlayers[i1]->velocity < -100)
        {
            AllPlayers[i1]->velocity += 30; // How slippery the movement is
        }
        else if (AllPlayers[i1]->velocity < 0)
        {
            AllPlayers[i1]->velocity -= AllPlayers[i1]->velocity / 2;
        }
        else if (AllPlayers[i1]->velocity > -2 && AllPlayers[i1]->velocity < 2)
        {
            AllPlayers[i1]->velocity = 0;
        }

        if (AllPlayers[i1]->velocity > AllPlayers[i1]->velocity_max)
        {
            AllPlayers[i1]->velocity = AllPlayers[i1]->velocity_max;
        }
        else if (AllPlayers[i1]->velocity < -1 * AllPlayers[i1]->velocity_max)
        {
            AllPlayers[i1]->velocity = -1 * AllPlayers[i1]->velocity_max;
        }
        AllPlayers[i1]->x += AllPlayers[i1]->velocity + AllPlayers[i1]->velocity_pp_collision;

        // Player Gravity:

        AllPlayers[i1]->verticle_velocity += AllPlayers[i1]->gravity_acceleration + AllPlayers[i1]->verticle_nongravity_acceleration;
        AllPlayers[i1]->verticle_nongravity_acceleration /= 2;
        if (AllPlayers[i1]->verticle_nongravity_acceleration < 2)
        {
            AllPlayers[i1]->verticle_nongravity_acceleration = 0;
        }
        if (AllPlayers[i1]->verticle_velocity < AllPlayers[i1]->verticle_velocity_max) // both are negative
        {
            AllPlayers[i1]->verticle_velocity = AllPlayers[i1]->verticle_velocity_max;
        }
        AllPlayers[i1]->y += AllPlayers[i1]->verticle_velocity;
    }


    // Player-Platform Collision
    for (int i1 = 0; i1 < PlayerCollisions.size(); ++i1)
    {
        for (int i2 = 0; i2 < StaticEntityCollisions.size(); ++i2)
        {

            if (PlayerCollisions[i1]->y < StaticEntityCollisions[i2]->y + StaticEntityCollisions[i2]->sizeY && // top of platform collision
                (PlayerCollisions[i1]->y_before > StaticEntityCollisions[i2]->y + StaticEntityCollisions[i2]->sizeY) &&
                (PlayerCollisions[i1]->x + PlayerCollisions[i1]->sizeX >= StaticEntityCollisions[i2]->x && PlayerCollisions[i1]->x <= StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX))
            {
                PlayerCollisions[i1]->jump_number = 0; // player touches ground so can restore jumps
                PlayerCollisions[i1]->y = StaticEntityCollisions[i2]->y + StaticEntityCollisions[i2]->sizeY;
                if (PlayerCollisions[i1]->verticle_velocity < 0)
                {
                    PlayerCollisions[i1]->verticle_velocity = 0;
                }
            }
            else if ((PlayerCollisions[i1]->x + PlayerCollisions[i1]->sizeX >= StaticEntityCollisions[i2]->x && PlayerCollisions[i1]->x <= StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX) &&
                PlayerCollisions[i1]->y_before == StaticEntityCollisions[i2]->y + StaticEntityCollisions[i2]->sizeY &&
                PlayerCollisions[i1]->y < StaticEntityCollisions[i2]->y + StaticEntityCollisions[i2]->sizeY)
            {
                PlayerCollisions[i1]->jump_number = 0; // player touches ground so can restore jumps
                if (PlayerCollisions[i1]->verticle_velocity < 0)
                {
                    PlayerCollisions[i1]->verticle_velocity = 0;
                }
                PlayerCollisions[i1]->y = PlayerCollisions[i1]->y_before;
            }
            else if (PlayerCollisions[i1]->x < StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX && // right of platform collision
                PlayerCollisions[i1]->x_before > StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX &&
                PlayerCollisions[i1]->y < StaticEntityCollisions[i2]->y + (StaticEntityCollisions[i2]->sizeY) &&
                (PlayerCollisions[i1]->y > StaticEntityCollisions[i2]->y || PlayerCollisions[i1]->y + PlayerCollisions[i1]->sizeY > StaticEntityCollisions[i2]->y) &&
                PlayerCollisions[i1]->x > StaticEntityCollisions[i2]->x)
            {
                PlayerCollisions[i1]->x = StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX;
                if (PlayerCollisions[i1]->velocity < 0)
                {
                    PlayerCollisions[i1]->velocity = 0;
                }
            }
            else if (PlayerCollisions[i1]->x < StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX &&
                PlayerCollisions[i1]->x_before == StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX &&
                PlayerCollisions[i1]->y < StaticEntityCollisions[i2]->y + (StaticEntityCollisions[i2]->sizeY) &&
                (PlayerCollisions[i1]->y > StaticEntityCollisions[i2]->y || PlayerCollisions[i1]->y + PlayerCollisions[i1]->sizeY > StaticEntityCollisions[i2]->y) &&
                PlayerCollisions[i1]->x > StaticEntityCollisions[i2]->x)
            {
                PlayerCollisions[i1]->x = StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX;
                if (PlayerCollisions[i1]->velocity < 0)
                {
                    PlayerCollisions[i1]->velocity = 0;
                }
            }
            else if (PlayerCollisions[i1]->x + PlayerCollisions[i1]->sizeX > StaticEntityCollisions[i2]->x && // left of platform collision.
                PlayerCollisions[i1]->x_before + PlayerCollisions[i1]->sizeX < StaticEntityCollisions[i2]->x &&
                PlayerCollisions[i1]->y < StaticEntityCollisions[i2]->y + (StaticEntityCollisions[i2]->sizeY) &&
                (PlayerCollisions[i1]->y > StaticEntityCollisions[i2]->y || PlayerCollisions[i1]->y + PlayerCollisions[i1]->sizeY > StaticEntityCollisions[i2]->y) &&
                PlayerCollisions[i1]->x < StaticEntityCollisions[i2]->x)
            {
                PlayerCollisions[i1]->x = StaticEntityCollisions[i2]->x - PlayerCollisions[i1]->sizeX;
                if (PlayerCollisions[i1]->velocity > 0)
                {
                    PlayerCollisions[i1]->velocity = 0;
                }
            }
            else if (PlayerCollisions[i1]->x + PlayerCollisions[i1]->sizeX > StaticEntityCollisions[i2]->x &&
                PlayerCollisions[i1]->x_before + PlayerCollisions[i1]->sizeX == StaticEntityCollisions[i2]->x &&
                PlayerCollisions[i1]->y < StaticEntityCollisions[i2]->y + (StaticEntityCollisions[i2]->sizeY) &&
                (PlayerCollisions[i1]->y > StaticEntityCollisions[i2]->y || PlayerCollisions[i1]->y + PlayerCollisions[i1]->sizeY > StaticEntityCollisions[i2]->y) &&
                PlayerCollisions[i1]->x < StaticEntityCollisions[i2]->x)
            {
                PlayerCollisions[i1]->x = StaticEntityCollisions[i2]->x - PlayerCollisions[i1]->sizeX;
                if (PlayerCollisions[i1]->velocity > 0)
                {
                    PlayerCollisions[i1]->velocity = 0;
                }
            } // bottom collision:
            else if ((PlayerCollisions[i1]->x + PlayerCollisions[i1]->sizeX >= StaticEntityCollisions[i2]->x && PlayerCollisions[i1]->x <= StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX) &&
                PlayerCollisions[i1]->y + PlayerCollisions[i1]->sizeY > StaticEntityCollisions[i2]->y &&
                PlayerCollisions[i1]->y_before + PlayerCollisions[i1]->sizeY < StaticEntityCollisions[i2]->y)
            {
                PlayerCollisions[i1]->y = StaticEntityCollisions[i2]->y - PlayerCollisions[i1]->sizeY;
                if (PlayerCollisions[i1]->verticle_velocity > 0)
                {
                    PlayerCollisions[i1]->verticle_velocity = 0;
                }
            }
            else if ((PlayerCollisions[i1]->x + PlayerCollisions[i1]->sizeX >= StaticEntityCollisions[i2]->x && PlayerCollisions[i1]->x <= StaticEntityCollisions[i2]->x + StaticEntityCollisions[i2]->sizeX) &&
                PlayerCollisions[i1]->y + PlayerCollisions[i1]->sizeY > StaticEntityCollisions[i2]->y &&
                PlayerCollisions[i1]->y_before + PlayerCollisions[i1]->sizeY == StaticEntityCollisions[i2]->y)
            {
                if (PlayerCollisions[i1]->verticle_velocity > 0)
                {
                    PlayerCollisions[i1]->verticle_velocity = 0;
                }
                PlayerCollisions[i1]->y = PlayerCollisions[i1]->y_before;
            }

        }
    }

    for (int i1 = 0; i1 < AllPlayers.size(); ++i1)
    {
        // IMPORTANT:::
        AllPlayers[i1]->x_before = AllPlayers[i1]->x;
        AllPlayers[i1]->y_before = AllPlayers[i1]->y;
    }
}

void draw_screen()
{
    for (int i = 0; i < AllEntities.size(); i++)
    {
        if (AllEntities[i].second == ENTITY)
        {
            Entity** entityPtr = std::get_if<Entity*>(&AllEntities[i].first);

            if ((**entityPtr).texture != nullptr)
            {
                (**entityPtr).draw_tex((**entityPtr).x_texture_offset, (**entityPtr).y_texture_offset, (**entityPtr).sizex_texture_offset, (**entityPtr).sizey_texture_offset);
            }
            else
            {
                (**entityPtr).draw((**entityPtr).x_texture_offset, (**entityPtr).y_texture_offset, (**entityPtr).sizex_texture_offset, (**entityPtr).sizey_texture_offset);
            }

        }
        else if (AllEntities[i].second == STATIC_ENTITY)
        {
            StaticEntity** entityPtr = std::get_if<StaticEntity*>(&AllEntities[i].first);

            if ((**entityPtr).texture != nullptr)
            {
                (**entityPtr).draw_tex((**entityPtr).x_texture_offset, (**entityPtr).y_texture_offset, (**entityPtr).sizex_texture_offset, (**entityPtr).sizey_texture_offset);
            }
            else
            {
                (**entityPtr).draw((**entityPtr).x_texture_offset, (**entityPtr).y_texture_offset, (**entityPtr).sizex_texture_offset, (**entityPtr).sizey_texture_offset);
            }

        }
        else if (AllEntities[i].second == PLAYER)
        {
            Player** entityPtr = std::get_if<Player*>(&AllEntities[i].first);

            if ((**entityPtr).texture != nullptr)
            {
                (**entityPtr).draw_tex((**entityPtr).x_texture_offset, (**entityPtr).y_texture_offset, (**entityPtr).sizex_texture_offset, (**entityPtr).sizey_texture_offset);
            }
            else
            {
                (**entityPtr).draw((**entityPtr).x_texture_offset, (**entityPtr).y_texture_offset, (**entityPtr).sizex_texture_offset, (**entityPtr).sizey_texture_offset);
            }

        }
    }
}

// Event handler
SDL_Event e;

SDL_GameController* controller = nullptr;
std::vector<SDL_GameController*> controllers;

std::vector<SDL_GameController*> controllers_playing;

void PlayerSelection(bool remove)
{
    static std::vector<std::pair<StaticEntity*, int>> player_boxes;

    // Create a separate vector to collect entities to be removed
    std::vector<std::pair<StaticEntity*, int>> entitiesToRemove;

    // Loop through controllers
    for (size_t i = 0; i < controllers.size(); ++i)
    {
        bool controller_playing = std::find(controllers_playing.begin(), controllers_playing.end(), controllers[i]) != controllers_playing.end();

        // Check if controller is playing
        if (controller_playing)
        {
            // Check if player_boxes already contains an entry for this controller
            bool found = false;
            for (auto& pair : player_boxes)
            {
                if (pair.second == i)
                {
                    found = true;
                    break;
                }
            }

            // If not found, add it to player_boxes
            if (!found)
            {
                StaticEntity* box = nullptr;
                switch (i)
                {
                case 0:
                    box = new StaticEntity(0, 54000, 48000, 54000, 120, 100, 240, 255);
                    break;
                case 1:
                    box = new StaticEntity(48000, 54000, 48000, 54000, 130, 90, 240, 255);
                    break;
                case 2:
                    box = new StaticEntity(96000, 54000, 48000, 54000, 140, 80, 240, 255);
                    break;
                case 3:
                    box = new StaticEntity(144000, 54000, 48000, 54000, 150, 70, 240, 255);
                    break;
                case 4:
                    box = new StaticEntity(0, 0, 48000, 54000, 160, 60, 240, 255);
                    break;
                case 5:
                    box = new StaticEntity(48000, 0, 48000, 54000, 170, 50, 240, 255);
                    break;
                case 6:
                    box = new StaticEntity(96000, 0, 48000, 54000, 180, 40, 240, 255);
                    break;
                case 7:
                    box = new StaticEntity(144000, 0, 48000, 54000, 190, 30, 240, 255);
                    break;
                default:
                    break;
                }

                if (box)
                {
                    std::pair<StaticEntity*, int> r(box, i);
                    player_boxes.push_back(r);
                }
            }
        }
        else // Controller is not playing
        {
            // Collect entities to be removed instead of removing directly
            for (auto& pair : player_boxes)
            {
                if (pair.second == i)
                {
                    entitiesToRemove.push_back(pair);
                }
            }
        }
    }

    // Remove collected entities outside the iteration loop
    for (auto& entity : entitiesToRemove)
    {
        auto it = std::find_if(player_boxes.begin(), player_boxes.end(), [&entity](const std::pair<StaticEntity*, int>& pair) {
            return pair.first == entity.first && pair.second == entity.second;
            });

        if (it != player_boxes.end())
        {
            delete it->first;
            player_boxes.erase(it);
        }
    }

    // Handle removal of all player_boxes entries
    if (remove)
    {
        for (auto& pair : player_boxes)
        {
            delete pair.first;
        }
        player_boxes.clear();
    }
}

// Create a global or class-level map to store button states for each controller
std::unordered_map<SDL_JoystickID, std::unordered_map<SDL_GameControllerButton, bool>> buttonStates;

void jump(Player* player)
{

    if (player->jump_number == 0)
    {
        player->verticle_nongravity_acceleration = 700;
    }
    else if (player->jump_number == 1) // double jump
    {
        player->verticle_velocity = 0;
        player->verticle_nongravity_acceleration = 500;
    }
    else if (player->jump_number == 2 && player->TripleJump == true)
    {
        player->verticle_velocity = 0;
        player->verticle_nongravity_acceleration = 450;
    }

    player->jump_number += 1;
}

void handleControllerEventsMenu(SDL_GameController* controller)
{
    if (controller)
    {
        // Get the unique ID for the controller
        SDL_JoystickID controllerID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));

        // Initialize the button state for this controller if it doesn't exist
        if (buttonStates.find(controllerID) == buttonStates.end())
        {
            buttonStates[controllerID] = {
                {SDL_CONTROLLER_BUTTON_START, false},
                {SDL_CONTROLLER_BUTTON_A, false},
                {SDL_CONTROLLER_BUTTON_B, false},
                {SDL_CONTROLLER_BUTTON_BACK, false}
            };
        }

        // Check the state of each button
        SDL_GameControllerButton buttons[] = { SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_BACK };
        const char* buttonNames[] = { "Start", "A", "B", "Back" };

        for (int i = 0; i < 4; ++i) {
            SDL_GameControllerButton button = buttons[i];
            bool isButtonPressed = SDL_GameControllerGetButton(controller, button);

            // Check for button press
            if (isButtonPressed && !buttonStates[controllerID][button])
            {
                std::cout << buttonNames[i] << " button pressed on controller " << controllerID << std::endl;
                // Perform specific actions based on which button is pressed
                if (button == SDL_CONTROLLER_BUTTON_START)
                {
                    // Handle Start button press
                    if (std::find(controllers_playing.begin(), controllers_playing.end(), controller) == controllers_playing.end()) 
                    {
                        controllers_playing.push_back(controller);
                    }
                    else
                    {
                        quit_menu = true;
                    }
                }
                else if (button == SDL_CONTROLLER_BUTTON_A)
                {
                    // Handle Start button press
                    if (std::find(controllers_playing.begin(), controllers_playing.end(), controller) == controllers_playing.end())
                    {
                        controllers_playing.push_back(controller);
                    }
                    else
                    {
                        quit_menu = true;
                    }
                }
                else if (button == SDL_CONTROLLER_BUTTON_B)
                {
                    // Handle B button press
                    auto it = std::find(controllers_playing.begin(), controllers_playing.end(), controller);
                    // If the element was found, remove it
                    if (it != controllers_playing.end()) {
                        controllers_playing.erase(it);
                    }

                }
                else if (button == SDL_CONTROLLER_BUTTON_BACK)
                {
                    // Handle Back button press
                    auto it = std::find(controllers_playing.begin(), controllers_playing.end(), controller);
                    // If the element was found, remove it
                    if (it != controllers_playing.end()) {
                        controllers_playing.erase(it);
                    }
                }
            }

            // Check for button release
            if (!isButtonPressed && buttonStates[controllerID][button])
            {
                std::cout << buttonNames[i] << " button released on controller " << controllerID << std::endl;
                // Perform specific actions based on which button is released
                if (button == SDL_CONTROLLER_BUTTON_START)
                {
                    // Handle Start button release
                }
                else if (button == SDL_CONTROLLER_BUTTON_A)
                {
                    // Handle A button release
                }
                else if (button == SDL_CONTROLLER_BUTTON_B)
                {
                    // Handle B button release
                }
                else if (button == SDL_CONTROLLER_BUTTON_BACK)
                {
                    // Handle Back button release
                }
            }

            // Update button state
            buttonStates[controllerID][button] = isButtonPressed;
        }
    }
}

void handleControllerEvents(SDL_GameController* controller, Player* player)
{
    if (controller) 
    {
        // Deadzone threshold to prevent drift
        const int DEADZONE = 8000;

        // Read left stick axis values (range: -32768 to 32767)
        int16_t axisX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t axisY = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

        // Scale factor for movement speed
        const double SCALE = 1.0 / 32768.0;

        // Apply deadzone and move entity
        if (std::abs(axisX) > DEADZONE) {
            // Axis values are in range -32768 to 32767, scale them to match movement speed
           
            player->acceleration = roundToSignificantFigures(static_cast<double>(axisX * SCALE)*160, 2);
            //std::cout << player->acceleration << std::endl;
        }
        else
        {
            player->acceleration = 0;
        }

        if (std::abs(axisY) > DEADZONE) {
            // Inverted axis for correct directional input
            
        }


        // Get the unique ID for the controller
        SDL_JoystickID controllerID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));

        // Initialize the button state for this controller if it doesn't exist
        if (buttonStates.find(controllerID) == buttonStates.end()) {
            buttonStates[controllerID] = {
                {SDL_CONTROLLER_BUTTON_A, false},
                {SDL_CONTROLLER_BUTTON_B, false},
                {SDL_CONTROLLER_BUTTON_X, false},
                {SDL_CONTROLLER_BUTTON_Y, false},
                {SDL_CONTROLLER_BUTTON_START, false},
                {SDL_CONTROLLER_BUTTON_BACK, false},
                {SDL_CONTROLLER_BUTTON_DPAD_UP, false},
                {SDL_CONTROLLER_BUTTON_DPAD_DOWN, false},
                {SDL_CONTROLLER_BUTTON_DPAD_LEFT, false},
                {SDL_CONTROLLER_BUTTON_DPAD_RIGHT, false}
            };
        }

        // List of buttons to check
        SDL_GameControllerButton buttons[] = { 
            SDL_CONTROLLER_BUTTON_A,
            SDL_CONTROLLER_BUTTON_B, 
            SDL_CONTROLLER_BUTTON_X, 
            SDL_CONTROLLER_BUTTON_Y,
            SDL_CONTROLLER_BUTTON_START,
            SDL_CONTROLLER_BUTTON_BACK,
            SDL_CONTROLLER_BUTTON_DPAD_UP,
            SDL_CONTROLLER_BUTTON_DPAD_DOWN,
            SDL_CONTROLLER_BUTTON_DPAD_LEFT,
            SDL_CONTROLLER_BUTTON_DPAD_RIGHT
        };
        const char* buttonNames[] = { "A", "B", "X", "Y", "Start", "Back", "DPad Up", "DPad Down", "DPad Left", "DPad Right"};

        // Check the state of each button
        for (int i = 0; i < 10; ++i) {
            SDL_GameControllerButton button = buttons[i];
            bool isButtonPressed = SDL_GameControllerGetButton(controller, button);

            // Check for button press
            if (isButtonPressed && !buttonStates[controllerID][button]) 
            {
                if (buttonNames[i] == "A")
                {
                    jump(player);
                }
                std::cout << buttonNames[i] << " button pressed on controller " << controllerID << std::endl;
            }

            // Check for button release
            if (!isButtonPressed && buttonStates[controllerID][button]) {
                std::cout << buttonNames[i] << " button released on controller " << controllerID << std::endl;
            }

            // Update the button state
            buttonStates[controllerID][button] = isButtonPressed;
        }

    }
}

std::unordered_map<SDL_Scancode, bool> keyStates;
void handleEvents(Player* player)
{


    const Uint8* state = SDL_GetKeyboardState(NULL);

    // Initialize the key state for W if it doesn't exist
    if (keyStates.find(SDL_SCANCODE_W) == keyStates.end()) {
        keyStates[SDL_SCANCODE_W] = false;
    }

    // Check the current state of the W key
    bool isWKeyPressed = state[SDL_SCANCODE_W];

    // Check for key press
    if (isWKeyPressed && !keyStates[SDL_SCANCODE_W]) 
    {
        std::cout << "W key pressed" << std::endl;
        jump(player);
    }

    // Check for key release
    if (!isWKeyPressed && keyStates[SDL_SCANCODE_W]) 
    {
        std::cout << "W key released" << std::endl;
    }

    // Update the previous state
    keyStates[SDL_SCANCODE_W] = isWKeyPressed;

    if (state[SDL_SCANCODE_S])
    {

    }

    if (state[SDL_SCANCODE_A]) {
        player->acceleration = -160;
    }
    else if (state[SDL_SCANCODE_D]) {
        player->acceleration = 160;
    }
    else
    {
        //player->acceleration = 0;
    }

    // Check for SDL_QUIT event
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }
}

void handleEventsMenu()
{


    const Uint8* state = SDL_GetKeyboardState(NULL);



    if (state[SDL_SCANCODE_S]) {

    }

    if (state[SDL_SCANCODE_A])
    {

    }
    
    if (state[SDL_SCANCODE_D]) 
    {

    }


    // Check for SDL_QUIT event
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        }
    }
}

int main(int argc, char* argv[]) 
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) 
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return -1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Gun Mayhem",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_X, SCREEN_Y,
        SDL_WINDOW_SHOWN);

    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    // Create a renderer for the window
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "Number of Controllers: " << SDL_NumJoysticks() << std::endl;

    // Initialize the controller
    if (SDL_NumJoysticks() > 0) {
        for (int i = 0; i < SDL_NumJoysticks(); ++i)
        {
            if (SDL_IsGameController(i)) {
                controllers.push_back(SDL_GameControllerOpen(i));
                if (controllers[i])
                {
                    std::cout << "Controller connected: " << SDL_GameControllerName(controllers[i]) << std::endl;
                }
                else 
                {
                    std::cerr << "Could not open game controller! SDL_Error: " << SDL_GetError() << std::endl;
                }
            }
        }
    }


    //StaticEntity menu_background(0, 0, 192000, 108000, BRICK_TEX, false);

    Timer<60> menu_fps_cap_timer;
    while (!quit_menu)
    {
        handleEventsMenu();
        for (int i = 0; i < SDL_NumJoysticks(); ++i)
        {
            // Handle events on queue
            handleControllerEventsMenu(controllers[i]);
        }

        PlayerSelection(false);

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0xF0, 0x00, 0xF0, 0xFF);
        SDL_RenderClear(renderer);

        draw_screen();

        // Update screen
        SDL_RenderPresent(renderer);

        menu_fps_cap_timer.sleep();
    }
    PlayerSelection(true);


    StaticEntity platform2(100000, 1500, 50000, 20000);
    platform2.CollisionsOn();

    StaticEntity platform3(65000, 35000, 50000, 20000);
    platform3.CollisionsOn();

    Timer<60> fps_cap_timer;
    // While application is running
    quit = false;
    while (!quit) 
    {
        // Handle events on queue
        for (int i = 0; i < controllers_playing.size(); ++i)
        {
            handleControllerEvents(controllers_playing[i], AllPlayers[i]);
        }

        // Clear screen
        SDL_SetRenderDrawColor(renderer, 0xF0, 0x00, 0xF0, 0xFF);
        SDL_RenderClear(renderer);

        physics();

        draw_screen();

        // Update screen
        SDL_RenderPresent(renderer);

        fps_cap_timer.sleep();
    }

    // Destroy renderer and window
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    IMG_Quit();

    // Quit SDL subsystems
    SDL_Quit();

    return 0;
}
