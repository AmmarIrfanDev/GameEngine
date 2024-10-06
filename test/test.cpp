#include <iostream>

#include <GameEngine.h>

int main()
{

    init();

    StaticEntity platform2(100000, 1500, 50000, 20000);
    platform2.CollisionsOn();

    StaticEntity platform3(65000, 35000, 50000, 20000);
    platform3.CollisionsOn();

    main_loop();

    std::cout << "Hello World!\n";
}