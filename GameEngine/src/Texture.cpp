#include "Texture.h"
#include <iostream>
#include <Windows.h>

// Constructor
Texture::Texture()
    : mTexture(nullptr), mFrameGap(0), mWidth(0), mHeight(0),
    mNumFrames(1), mFrameWidth(0), mFrameHeight(0),
    mFrameTime(0), mCurrentFrame(0), mLastFrameTime(0),
    mXOffset(0), mYOffset(0), mXEndOffset(0), mYEndOffset(0),
    mFlip(SDL_FLIP_NONE)
{
    animated = false;
}

void Texture::setFlip(SDL_RendererFlip flip) {
    mFlip = flip;
}

// Destructor
Texture::~Texture() {
    if (mTexture != nullptr) {
        SDL_DestroyTexture(mTexture);
        mTexture = nullptr;
    }
}

// Helper function to create texture from SDL_Surface
bool Texture::createTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) {
    // Create texture from surface pixels
    mTexture = SDL_CreateTextureFromSurface(renderer, surface);
    if (mTexture == nullptr) {
        std::cerr << "Unable to create texture from surface! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Get image dimensions
    mWidth = surface->w;
    mHeight = surface->h;

    return true;
}

// Load texture from file
bool Texture::loadFromFile(SDL_Renderer* renderer, const std::string& path) {
    // Free any pre-existing texture
    if (mTexture != nullptr) {
        SDL_DestroyTexture(mTexture);
        mTexture = nullptr;
        mWidth = 0;
        mHeight = 0;
    }

    // Load image at specified path
    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to load image " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    // Create texture from surface pixels
    bool success = createTextureFromSurface(renderer, loadedSurface);

    // Free old loaded surface
    SDL_FreeSurface(loadedSurface);

    return success;
}

// Load texture from resource
bool Texture::loadFromResource(SDL_Renderer* renderer, int resourceID) {
    // Free any pre-existing texture
    if (mTexture != nullptr) {
        SDL_DestroyTexture(mTexture);
        mTexture = nullptr;
        mWidth = 0;
        mHeight = 0;
    }

    // Load resource
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resourceID), RT_RCDATA); // Use RT_RCDATA for raw data
    if (hRes == NULL) {
        std::cerr << "Failed to find resource with ID: " << resourceID << std::endl;
        return false;
    }

    HGLOBAL hResLoad = LoadResource(NULL, hRes);
    if (hResLoad == NULL) {
        std::cerr << "Failed to load resource with ID: " << resourceID << std::endl;
        return false;
    }

    void* pResData = LockResource(hResLoad);
    DWORD resSize = SizeofResource(NULL, hRes);

    // Create SDL_RWops from resource data
    SDL_RWops* rw = SDL_RWFromMem(pResData, resSize);
    if (rw == nullptr) {
        std::cerr << "Unable to create SDL_RWops from resource data! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Load PNG surface from SDL_RWops
    SDL_Surface* loadedSurface = IMG_Load_RW(rw, 1); // 1 for auto-close
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to create SDL_Surface from PNG resource data! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    // Create texture from surface pixels
    bool success = createTextureFromSurface(renderer, loadedSurface);

    // Free old loaded surface
    SDL_FreeSurface(loadedSurface);

    return success;
}

// Get texture dimensions
int Texture::getWidth() const {
    return mWidth;
}

int Texture::getHeight() const {
    return mHeight;
}


// animations:

void Texture::setAnimationFrames(int numFrames, int frameWidth, int frameHeight, int frameTime, int frameGap, int xOffset, int yOffset, int xEndOffset, int yEndOffset)
{
    mNumFrames = numFrames;
    mFrameWidth = frameWidth;
    mFrameHeight = frameHeight;
    mFrameTime = frameTime;
    mFrameGap = frameGap;
    mCurrentFrame = 0;
    mLastFrameTime = SDL_GetTicks();
    animated = true;
    mXOffset = xOffset;
    mYOffset = yOffset;
    mXEndOffset = xEndOffset;
    mYEndOffset = yEndOffset;
}

// Update animation frame
void Texture::updateAnimation() {
    if (mNumFrames > 1) {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime > mLastFrameTime + mFrameTime) {
            mCurrentFrame++;
            if (mCurrentFrame >= mNumFrames) {
                mCurrentFrame = 0;  // Restart the animation
            }
            mLastFrameTime = currentTime;
        }
    }
}


// Render texture at given point
void Texture::render(SDL_Renderer* renderer, int x, int y, int width, int height)
{
    if (!animated)
    {
        SDL_Rect renderQuad = { x, y, width, height };
        SDL_RenderCopyEx(renderer, mTexture, nullptr, &renderQuad, 0, nullptr, mFlip);
    }
    else
    {
        updateAnimation();
        renderFrame(renderer, x, y, width, height);
    }
}

void Texture::renderFrame(SDL_Renderer* renderer, int x, int y, int width, int height) {
    // Calculate the number of frames per row
    int framesPerRow = (mWidth - mXOffset - mXEndOffset + mFrameGap) / (mFrameWidth + mFrameGap);

    // Calculate the current frame's position
    int frameRow = mCurrentFrame / framesPerRow;
    int frameCol = mCurrentFrame % framesPerRow;

    // Calculate the position of the current frame within the sprite sheet
    int srcX = mXOffset + frameCol * (mFrameWidth + mFrameGap);
    int srcY = mYOffset + frameRow * (mFrameHeight + mFrameGap);

    // Ensure the source rectangle is within the bounds of the texture
    if (srcX < mXOffset || srcY < mYOffset || srcX + mFrameWidth > mWidth - mXEndOffset || srcY + mFrameHeight > mHeight - mYEndOffset) {
        std::cerr << "Source rectangle is out of bounds." << std::endl;
        return;
    }

    SDL_Rect srcRect = { srcX, srcY, mFrameWidth, mFrameHeight };

    // Set the destination rectangle to the specified width and height
    SDL_Rect dstRect = { x, y, width, height };

    // Render the current frame of the animation
    SDL_RenderCopyEx(renderer, mTexture, &srcRect, &dstRect, 0, nullptr, mFlip);
}

// Load texture from resource within a DLL
bool Texture::loadFromResourceDLL(SDL_Renderer* renderer, HMODULE hModule, int resourceID) 
{
    if (mTexture != nullptr) {
        SDL_DestroyTexture(mTexture);
        mTexture = nullptr;
        mWidth = 0;
        mHeight = 0;
    }

    HRSRC hRes = FindResource(hModule, MAKEINTRESOURCE(resourceID), RT_RCDATA); // Use RT_RCDATA for raw data
    if (hRes == NULL) {
        std::cerr << "Failed to find resource with ID: " << resourceID << std::endl;
        return false;
    }

    HGLOBAL hResLoad = LoadResource(hModule, hRes);
    if (hResLoad == NULL) {
        std::cerr << "Failed to load resource with ID: " << resourceID << std::endl;
        return false;
    }

    void* pResData = LockResource(hResLoad);
    DWORD resSize = SizeofResource(hModule, hRes);

    SDL_RWops* rw = SDL_RWFromMem(pResData, resSize);
    if (rw == nullptr) {
        std::cerr << "Unable to create SDL_RWops from resource data! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_Surface* loadedSurface = IMG_Load_RW(rw, 1); // 1 for auto-close
    if (loadedSurface == nullptr) {
        std::cerr << "Unable to create SDL_Surface from PNG resource data! SDL_image Error: " << IMG_GetError() << std::endl;
        return false;
    }

    bool success = createTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);

    return success;
}