#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include<random>
#include<ctime>
#include<algorithm>
#include <queue>

const int WINDOW_WIDTH = 1180;
const int WINDOW_HEIGHT = 820;




//colory
sf::Color TileColor(70, 190 ,100);
sf::Color TileColorR(70, 90 ,100);
sf::Color TileOutline(255 ,255,255);
sf::Color playerOutline(0, 0, 80);

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

//gracz
int playerRow = 0;
int playerCol = 0;

//aby pierwszy ruch byl safe
bool isinside(int r, int c, const Difficultylvl& level)
{
    return r >= 0 && r< level.rows && c >= 0 && c < level.cols;
}

void tempnamenvm(std::vector<std::vector<Cell>>& map, const Difficultylvl& level){
    for(int r = 0; r < level.rows; r++){
        for(int c = 0; c <level.cols; c++){
            if(map[r][c].mine) continue;

            int n = 0;

            for(int dr = -1; dr<=1; dr++){
                for (int dc= -1; dc <=1; dc++){
                    int nr = r +dr;
                    int nc = c +dc;
                    if(isinside(nr, nc, level) && map[nr][nc].mine){
                        n++;
                    }
                }
            }

            map[r][c].nearMines = n;

        }
    }

}

void generateBoard(std::vector<std::vector<Cell>>& map, const Difficultylvl& level, int sRow, int sCol)
{
    std::vector<std::pair<int, int>> positions;

    for (int r = 0; r < level.rows; r++){
        for (int c = 0; c < level.cols; c++){
            bool sZone = (std::abs(r - sRow) <= 1) && (std::abs(c - sCol) <=1);

            if (!sZone){ positions.push_back({r, c});
        }
    }
}

std::random_device rd;
std::mt19937 rng(rd());
std::shuffle(positions.begin(), positions.end(), rng);

for(int i = 0; i < level.mines; i ++){
    map[positions[i].first][positions[i].second].mine = true;
}

tempnamenvm(map, level);
}
// cos
bool levelgenerated = false;
bool loss = false;
bool win = false;

void floodReveal(std::vector<std::vector<Cell>>& map, const Difficultylvl& level, int row, int col)
{
    std::queue<std::pair<int, int>> q;
    q.push({row, col});

    while (!q.empty()) {
        auto [r, c] = q.front();
        q.pop();

        if (!isinside(r, c, level)) continue;
        if (map[r][c].revealed || map[r][c].flagged) continue;

        map[r][c].revealed = true;

        if(map[r][c].nearMines!= 0) continue;

        for(int dr = -1; dr <=1 ; dr++){
            for(int dc = -1; dc <= 1; dc++){
                if(dr != 0 || dc != 0){
                    q.push({r+ dr, c + dc});
                }
            }
        }
    }
}


bool isWin(const std::vector<std::vector<Cell>>& map, const Difficultylvl& level){
    for (int r = 0; r < level.rows; r ++){
        for(int c = 0; c <level.cols; c ++){
            if(!map[r][c].mine && !map[r][c].revealed) {
                return false;
            }
        }
    }
    return true;
}

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
            if(event.type == sf::Event::KeyPressed) {
                if((event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) && playerCol > 0) {
                    playerCol--;
                }
                else if((event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) && playerCol < level.cols - 1) {
                    playerCol++;
                }
                else if((event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) && playerRow > 0) {
                    playerRow--;
                }
                else if((event.key.code == sf::Keyboard::Down || event.key.code == sf::Keyboard::S) && playerRow < level.rows - 1) {
                    playerRow++;
                }
                if(event.key.code == sf::Keyboard::Space && !loss && !win) {
                    if(!levelgenerated){
                        generateBoard(map, level, playerRow, playerCol);
                        levelgenerated = true;
                    }

                    Cell& s = map[playerRow][playerCol];


                    if(s.mine){
                        s.revealed = true;
                        loss = true;
                    }
                    else {
                        floodReveal(map, level, playerRow, playerCol);

                        if(isWin(map, level)) win = true;
                    }

                }

            }
        }

        window.clear(sf::Color(0, 0, 0));
        //generowanie mapy
        int cellsize = 40;
        int startX = 350;
        int startY = 150;


        for(int r =0; r < level.rows; r++) {
            for (int c = 0; c < level.cols; c++){
                sf::RectangleShape tile(sf::Vector2f(cellsize -2, cellsize -2));

                tile.setPosition(startX + c * cellsize, startY + r *cellsize);
                if(map[r][c].revealed){
                      tile.setFillColor(TileColorR);
                }
                else  tile.setFillColor(TileColor);

                tile.setOutlineThickness(1);
                tile.setOutlineColor(TileOutline);
                window.draw(tile);
                if(map[r][c].revealed && map[r][c].mine)
                {
                    sf::CircleShape mine(cellsize * 0.25f);
                    mine.setPosition(startX +c * cellsize + cellsize *0.25f, startY +r * cellsize + cellsize *0.25f );
                    mine.setFillColor(sf::Color::Black);
                    window.draw(mine);
                }
            }
        }

        //palyer
        //movment


//draw mines


        //draw player
        sf::RectangleShape player(sf::Vector2f(cellsize-4, cellsize -4));
        player.setPosition( startX + playerCol * cellsize +1, startY + playerRow * cellsize +1);
        player.setFillColor(sf::Color::Transparent);
        player.setOutlineColor(playerOutline);
        player.setOutlineThickness(5);
        window.draw(player);










        window.display();
    }





    return 0;
}
