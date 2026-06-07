#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include<algorithm>

const int WINDOW_WIDTH = 1180;
const int WINDOW_HEIGHT = 820;




//colory
sf::Color TileColor(70, 90 ,100);
sf::Color TileOutline(255 ,255,255);

struct Cell {
    bool mine = false;
    bool revealed = false;
    bool flagged = false;
    int nearMines = 0;
};

struct Difficultylvl {
    std::string name;
    int rows;
    int cols;
    int mines;
};














int main()
{
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Saper SFML");
    window.setFramerateLimit(60);

    Difficultylvl level{"ez", 9, 9, 10}; // s r c m
    std::vector<std::vector<Cell>> map(level.rows, std::vector<Cell>(level.cols));
    //mapa ma level.rows emelentow i kazdy element ma vector o wielkosci level.colls



    while (window.isOpen()){
        sf::Event event {};

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed){
                window.close();
            }
        }

        window.clear(sf::Color(0, 0, 0));


        //podzial na czesci
        int cellsize = 40;
        int startX = 350;
        int startY = 150;


        for(int r =0; r < level.rows; r++) {
            for (int c = 0; c < level.cols; c++){
                sf::RectangleShape tile(sf::Vector2f(cellsize -2, cellsize -2));
                tile.setPosition(startX + c * cellsize, startY + r *cellsize);
                tile.setFillColor(TileColor);
                tile.setOutlineThickness(1);
                tile.setOutlineColor(TileOutline);
                window.draw(tile);
            }
        }







        window.display();
    }





    return 0;
}
