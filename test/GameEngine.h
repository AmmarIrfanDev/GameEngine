#pragma once
#include <Windows.h>

class __declspec(dllimport) Texture
{
public:
    // Constructor and destructor
    Texture();
    ~Texture();

    // Load texture from file
    bool loadFromFile(SDL_Renderer* renderer, const std::string& path);

    // Load texture from resource
    bool loadFromResource(SDL_Renderer* renderer, int resourceID);

    // Render texture at given point
    void render(SDL_Renderer* renderer, int x, int y, int width, int height);

    // Get texture dimensions
    int getWidth() const;
    int getHeight() const;

    // Set animation frames with gap and offsets
    void setAnimationFrames(int numFrames, int frameWidth, int frameHeight, int frameTime, int frameGap, int xOffset, int yOffset, int xEndOffset, int yEndOffset);

    void updateAnimation();
    void renderFrame(SDL_Renderer* renderer, int x, int y, int additionalWidth, int additionalHeight);

    // Set texture flip
    void setFlip(SDL_RendererFlip flip);

    bool loadFromResourceDLL(SDL_Renderer* renderer, HMODULE hModule, int resourceID);

private:
    // The actual hardware texture
    SDL_Texture* mTexture;

    // Image dimensions
    int mWidth;
    int mHeight;

    // Helper function to create texture from SDL_Surface
    bool createTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);

    // Animation properties
    bool animated;
    int mNumFrames;
    int mFrameWidth;
    int mFrameHeight;
    int mFrameTime;  // Time per frame in milliseconds
    int mCurrentFrame;
    int mFrameGap;
    Uint32 mLastFrameTime;  // Timestamp of the last frame update
    int mXOffset;
    int mYOffset;
    int mXEndOffset;
    int mYEndOffset;

    // Flip state
    SDL_RendererFlip mFlip;
};

class __declspec(dllexport) Player
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

class __declspec(dllexport) StaticEntity
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