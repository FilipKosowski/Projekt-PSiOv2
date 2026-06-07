#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <vector>


const int WINDOW_WIDTH = 1180;
const int WINDOW_HEIGHT = 820;


int main()
{
    sf::RenderWindow window(
        sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT),
        "Saper SFML"
        );
    window.setFramerateLimit(60);

    while (window.isOpen()){
        sf::Event event {};

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed){
                window.close();
            }
        }

        window.clear(sf::Color(20, 25, 18));
        window.display();
    }





    return 0;
}
