# GameEngine
This 2D game development framework, built using C++ and SDL2, is designed to simplify the process of creating customized, physics-based games. It provides a robust foundation for game developers, offering essential components for handling player input, physics simulation, and rendering. Whether you’re creating a fast-paced platformer or a more complex 2D game, this framework offers the flexibility and control you need to bring your vision to life.



https://github.com/user-attachments/assets/6014c581-8f1a-4e08-b916-8fd12dd21042



_This is example useage of the library_

## Key Features

✅Comprehensive Input Handling
Supports keyboard and game controller inputs, ensuring smooth and responsive gameplay. The system includes deadzone correction to prevent joystick drift, making it ideal for accurate movement controls in high-stakes scenarios.

✅Advanced Physics Engine
Features collision detection and realistic physics simulations, allowing developers to implement platform-based mechanics, dynamic jumping, and other physics-driven gameplay features.

✅Efficient Rendering
Leveraging SDL2’s hardware-accelerated rendering capabilities, the framework delivers high-performance graphics rendering. It ensures consistent frame rates across different platforms by integrating frame rate capping for smoother gameplay.

✅Entity and Collision Management
Provides tools for managing static and dynamic entities, along with comprehensive collision handling, enabling the creation of detailed and interactive game worlds.

## Future Enhancements
Looking ahead, the project aims to incorporate additional features and improvements, such as:

1) **Static and Dynamic linking will be added soon. Currently, you must modify the code yourself using ```__declspec(dllexport)``` to export the functions you need, and use ```__declspec(dllimport)``` in your own code to import the functions you need.**

2) Enhanced Graphics Support: Plans to include support for advanced graphical techniques, such as shaders and lighting effects, to elevate the visual quality of games developed with the engine.

3) Networking Capabilities: Future versions may introduce multiplayer support, enabling developers to create online games and expand their game worlds.

4) Asset Pipeline: An improved asset pipeline to streamline the process of importing and managing game assets, including textures, sounds, and music.

## Integration & Customisation
The engine offers seamless integration and extensive customization capabilities, empowering developers to adapt the framework to their unique project requirements. Its component-based architecture allows for easy assembly of game entities by combining various reusable components, enabling tailored gameplay mechanics without starting from scratch. Developers can integrate third-party libraries and tools, enhancing functionality such as AI, networking, and audio systems. With support for customizable configuration files, users can adjust game settings, tweak performance parameters, and modify entity properties on the fly, ensuring that the engine can evolve alongside the project. This flexibility fosters creativity, allowing developers to experiment with different gameplay styles and mechanics while maintaining a robust underlying structure.

## Prerequisites (included in ```git clone```) 

1) ~~GLEW (The OpenGL Extension Wrangler Library)~~
2) ~~GLFW~~
3) SDL2
4) SDL2_IMAGE

# Installation:
~~1) Download Binaries from Releases menu~~

~~2) Link either dynamically or statically (static linking is recommended)~~

~~3) add include directory~~

1) ```git clone https://github.com/AmmarIrfanDev/GameEngine.git```

2) Modify the Main loop and create your Game there.

**or**

2) Modify the code yourself using ```__declspec(dllexport)``` to export the functions you need, and use ```__declspec(dllimport)``` in your own code to import the functions you need.
