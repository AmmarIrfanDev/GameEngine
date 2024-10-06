#pragma once

#include "../../dep/SDL2-2.30.5/include/SDL.h"
#include "../../dep/SDL2_image-2.8.2/include/SDL_image.h"
#include <string>
#include <Windows.h>

class __declspec(dllexport) Texture 
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