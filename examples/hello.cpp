#include <iostream>

int main(int argc, char** argv) {
    // Let's print a lot of new lines so we can see our text  more
    // easily in the middle of the simulation's log messages
    std::cout << std::endl
              << "Hello world from program `" << argv[0] << "` :)" 
              << std::endl
              << std::endl;

    return 0;
}
