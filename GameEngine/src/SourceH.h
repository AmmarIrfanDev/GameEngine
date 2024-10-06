#pragma once
#define SDL_MAIN_HANDLED
#include "../../dep/SDL2-2.30.5/include/SDL.h"

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

#ifdef LIB21
#define GAME_ENGINE_API __declspec(dllexport)
#endif

class StaticEntity;
class Entity;
class Player;

enum EntityType : uint8_t
{
    ENTITY = 1,
    STATIC_ENTITY,
    PLAYER
};


typedef Uint8 ColourT;

bool quit_menu = false;

// Main loop flag
bool quit = true;

double roundToSignificantFigures(double num, int n);

SDL_Renderer* renderer = nullptr;

std::vector<std::pair<std::variant<Entity*, StaticEntity*, Player*>, EntityType>> AllEntities;

std::vector<StaticEntity*> StaticEntityCollisions;
std::vector<Player*> PlayerCollisions;
std::vector<Entity*> EntityCollisions;

std::vector<Player*> AllPlayers;

int SCREEN_X = 1920 / 2;
int SCREEN_Y = 1080 / 2;

const int max_X = 192000;
const int max_Y = 108000;

long long cameraX = 0;
long long cameraY = 0;
double camera_magnification = 1.00;



// Global variables for smooth camera movement
double targetCameraX = 0;
double targetCameraY = 0;
double cameraMoveSpeed = 0.01; // Adjust the speed of camera movement

// Event handler
SDL_Event e;

SDL_GameController* controller = nullptr;
std::vector<SDL_GameController*> controllers;

std::vector<SDL_GameController*> controllers_playing;


// Create a global or class-level map to store button states for each controller
std::unordered_map<SDL_JoystickID, std::unordered_map<SDL_GameControllerButton, bool>> buttonStates;

SDL_Window* window;

std::unordered_map<SDL_Scancode, bool> keyStates;

class GAME_ENGINE_API Player
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


void draw_original(bool tex, Texture* texture, ColourT Colour[4], long long x_in, long long y_in, long long sizeX_in, long long sizeY_in,
    long long x_in_add, long long y_in_add, long long sizex_in_add, long long sizey_in_add);

long long squared_distance(Player* p1, Player* p2);


class GAME_ENGINE_API Entity
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

    Entity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY);

    Entity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, const std::string& texturePath);

    Entity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, int resourceID, bool mapbg);

    Entity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, ColourT r, ColourT g, ColourT b, ColourT a);

    ~Entity();

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


class GAME_ENGINE_API StaticEntity
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

    StaticEntity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY);

    StaticEntity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, const std::string& texturePath);

    StaticEntity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, int resourceID, bool mapbg);

    StaticEntity(long long int X_POS, long long int Y_POS, unsigned int SizeX, unsigned int SizeY, ColourT r, ColourT g, ColourT b, ColourT a);

    ~StaticEntity();

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


void GAME_ENGINE_API init();

void GAME_ENGINE_API main_loop();