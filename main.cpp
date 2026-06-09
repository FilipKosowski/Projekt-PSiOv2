#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include<random>
#include<ctime>
#include<algorithm>
#include <queue>
#include <cmath>
#include <sstream>

const int WINDOW_WIDTH = 1180;
const int WINDOW_HEIGHT = 820;


enum GameState{
    menu, game, lost, won
};


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

    for(int i = 0; i < level.mines && i < positions.size(); i ++){
        map[positions[i].first][positions[i].second].mine = true;
    }

    tempnamenvm(map, level);
}

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

class SaperGame{
private:
    sf::RenderWindow window;
    std::vector<std::vector<Cell>> map;


    GameState state;
    int playerRow = 0;
    int playerCol = 0;

    sf::Font font;
    bool fontLoaded = false;

    sf::Clock gameClock;
    bool timerStarted = false;
    float finalTime = 0.0f;

    bool levelgenerated = false;
    bool loss = false;
    bool win = false;


    std::vector<Difficultylvl> lvls = {
        {"Easy", 9, 9, 10},
        {"Medium", 12, 12, 25},
        {"Hard", 16, 16, 45}
    };
    Difficultylvl level = lvls[0];



public:
    SaperGame() :  window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Saper SFML"), state(menu){
        window.setFramerateLimit(60);
        loadFont();
    }

    void run(){
        while (window.isOpen()){
            handleEvents();
            draw();
        }
    }
    void setDif(const Difficultylvl& l){
        level = l;
        //map.clear();
        map = std::vector<std::vector<Cell>>(level.rows, std::vector<Cell>(level.cols));
        playerRow = 0;
        playerCol = 0;


        loss = false;
        win = false;
        levelgenerated = false;

        timerStarted = false;
        finalTime = 0.0f;

        state = game;
    }

    void setMap(const std::vector<std::vector<Cell>>& m){
        map = m;
    }
private:
    float currentTime()
    {
        if(!timerStarted){
            return 0.0f;
        }

        return gameClock.getElapsedTime().asSeconds();
    }
    int flagCount(){
        int counter =0;

        for(int r =0; r < level.rows; r++){
            for(int c = 0; c < level.cols; c++){
                if(map[r][c].flagged){
                    counter++;
                }
            }
        }
        return counter;
    }
    void loadFont()
    {
        if(font.loadFromFile("C:/Windows/Fonts/arial.ttf")){
            fontLoaded = true;
        }
    }
    void drawText(const std::string& text, float x, float y, int size, sf::Color color){
        if(!fontLoaded){
            return;
        }

        sf::Text t;
        t.setFont(font);
        t.setString(text);
        t.setCharacterSize(size);
        t.setFillColor(color);
        t.setPosition(x, y);

        window.draw(t);
    }
    void drawHud(){
        sf::RectangleShape panel(sf::Vector2f(WINDOW_WIDTH, 90));
        panel.setPosition(0, 0);
        panel.setFillColor(sf::Color(25, 25, 25));
        window.draw(panel);

        drawText("SAPER", 30, 20, 36, sf::Color::White);

        drawText("Level: " + level.name, 230, 30, 24, sf::Color::White);

        int shownTime;

        if(loss || win){
            shownTime = static_cast<int>(finalTime);
        }
        else{
            shownTime = static_cast<int>(currentTime());
        }
        drawText("Time: " + std::to_string(shownTime) + " s", 450, 30, 24, sf::Color::White);

        int minesLeft = level.mines - flagCount();
        drawText("Mines: " + std::to_string(minesLeft), 650, 30, 24, sf::Color::White);

        drawText("R - restart", 850, 20, 20, sf::Color::White);
        drawText("ESC - menu", 850, 50, 20, sf::Color::White);

        drawText("Space - reveal  F - flag", 990, 35, 17, sf::Color::Yellow);
    }
    void handleEvents(){
        sf::Event event {};

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed){
                window.close();
            }
            if(event.type == sf::Event::KeyPressed) {
                if(state == menu){ //MENU wybieranie
                    if(event.key.code == sf::Keyboard::Num1) setDif(lvls[0]);
                    if(event.key.code == sf::Keyboard::Num2) setDif(lvls[1]);
                    if(event.key.code == sf::Keyboard::Num3) setDif(lvls[2]);
                    return;
                }
                if(state != game){
                    return;
                }
                if(loss || win){ // reset gamu
                    if(event.key.code == sf::Keyboard::R){
                        setDif(level);
                    }
                    if(event.key.code == sf::Keyboard::Escape){
                        state = menu;
                    }

                    return;
                } // ruchy podczas gry
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
                //Odkrywanie min win loss conditon
                if(event.key.code == sf::Keyboard::Space && !loss && !win) {

                    if(!levelgenerated){
                        generateBoard(map, level, playerRow, playerCol);
                        levelgenerated = true;

                        timerStarted = true;
                        gameClock.restart();
                    }

                    Cell& s = map[playerRow][playerCol];


                    if(!s.revealed && !s.flagged){
                        if(s.mine){
                            s.revealed = true;
                            loss = true;
                            finalTime = currentTime();
                        }
                        else {
                            floodReveal(map, level, playerRow, playerCol);
                        }
                    }

                    if(isWin(map, level)){
                        win = true;
                        finalTime = currentTime();
                    }
                }
                // flagowanie pol
                if(event.key.code == sf::Keyboard::F && !loss && !win){
                    if(levelgenerated){
                        Cell& s = map[playerRow][playerCol];

                        if(!s.revealed){
                            s.flagged = !s.flagged;
                        }
                    }
                }
            }
        }
    }

    void drawMenu(){

        window.clear(sf::Color(20,20,20));

        sf::Font font;
        font.loadFromFile("C:/Windows/fonts/arial.ttf");


        sf::Text title;
        title.setFont(font);
        title.setString("SAPER MENU");
        title.setCharacterSize(70);
        title.setFillColor(sf::Color::White);
        title.setPosition(470, 120);
        window.draw(title);

        sf::Text option1;
        option1.setFont(font);
        option1.setString("1 - Easy 9x9 10m");
        option1.setCharacterSize(35);
        option1.setFillColor(sf::Color::Green);
        option1.setPosition(390, 280);
        window.draw(option1);

        sf::Text option2;
        option2.setFont(font);
        option2.setString("2 - Medium 12x12 25m");
        option2.setCharacterSize(35);
        option2.setFillColor(sf::Color::Yellow);
        option2.setPosition(390, 350);
        window.draw(option2);

        sf::Text option3;
        option3.setFont(font);
        option3.setString("3 - Hard 16x16 45m");
        option3.setCharacterSize(35);
        option3.setFillColor(sf::Color::Red);
        option3.setPosition(390, 420);
        window.draw(option3);

        /*  sf::Text placeholder;
         option3.setFont(font);
        option3.setString("3 - Hard 16x16 45m");
        option3.setCharacterSize(35);
        option3.setFillColor(sf::Color::Red);
        option3.setPosition(WINDOW_WIDTH/2- 80, 280);
        window.draw(option3);
*/
        window.display();
    }
    void draw(){
        if(state == menu){
            drawMenu();
            return;
        }

        window.clear(sf::Color(0, 0, 0));

        drawHud();
        sf::Font font;
        font.loadFromFile("C:/Windows/fonts/arial.ttf");

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

                //RYSOWANIE MIN
                if(map[r][c].revealed && map[r][c].mine)
                {
                    sf::CircleShape mine(cellsize * 0.25f);
                    mine.setPosition(startX +c * cellsize + cellsize *0.25f, startY +r * cellsize + cellsize *0.25f );
                    mine.setFillColor(sf::Color::Black);
                    window.draw(mine);
                }
                //RYSOWANIE LICZBY MIN OBOK
                if(map[r][c].revealed && !map[r][c].mine && map[r][c].nearMines > 0)
                {
                    sf::Text number;
                    number.setFont(font);
                    number.setString(std::to_string(map[r][c].nearMines));
                    number.setCharacterSize(24);
                    number.setFillColor(sf::Color::Blue);
                    number.setPosition(startX +c * cellsize + cellsize *0.35f, startY +r * cellsize + cellsize *0.12f );
                    window.draw(number);
                }
                //RYSOWANIE FLAG
                if(!map[r][c].revealed && map[r][c].flagged)
                {
                    sf::CircleShape Flag(cellsize * 0.25f);
                    Flag.setPosition(startX +c * cellsize + cellsize *0.25f, startY +r * cellsize + cellsize *0.25f );
                    Flag.setFillColor(sf::Color::Red);
                    window.draw(Flag);
                }
            }
        }
        //draw player
        sf::RectangleShape player(sf::Vector2f(cellsize-4, cellsize -4));
        player.setPosition( startX + playerCol * cellsize +1, startY + playerRow * cellsize +1);
        player.setFillColor(sf::Color::Transparent);
        player.setOutlineColor(playerOutline);
        player.setOutlineThickness(5);
        window.draw(player);

        if(loss){
            sf::Text text;
            text.setFont(font);
            text.setString("Przegrana - R restart, ESC menu");
            text.setCharacterSize(35);
            text.setFillColor(sf::Color::Red);
            text.setPosition(330, 80);
            window.draw(text);
        }
        if(win){
            sf::Text text;
            text.setFont(font);
            text.setString("Wygrana - R restart, ESC menu");
            text.setCharacterSize(35);
            text.setFillColor(sf::Color::Green);
            text.setPosition(330, 80);
            window.draw(text);
        }
        window.display();
    }

};
//Menu i ustawienia




int main()
{
    SaperGame game;
    game.run();

    return 0;
}