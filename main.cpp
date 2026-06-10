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
#include <fstream>



const int WINDOW_WIDTH = 1180;
const int WINDOW_HEIGHT = 820;


enum GameState{
    menu, game, lost, won
};


//colory
sf::Color TileColor(69, 90 ,51);
sf::Color TileColor2(59, 78 ,45);
sf::Color TileColorR(156, 137 ,92);
sf::Color TileColorR2(168, 150 ,103);
sf::Color TileOutline(32 ,38,27);
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
    //random
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
    bool resultSaved = false;


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
        resultSaved = false;

        timerStarted = false;
        finalTime = 0.0f;

        state = game;
    }
    // po co to?
    void setMap(const std::vector<std::vector<Cell>>& m){
        map = m;
    }
private:

    int boardY()
    {
        return 120;
    }
    int boardX()
    {
        int boardWidth = level.cols * cellSize();
        return (WINDOW_WIDTH - boardWidth) / 2;
    }
    int cellSize()
    {
        int sizeWidth = (WINDOW_HEIGHT - 90) / level.cols;
        int sizeHeight = (WINDOW_WIDTH - 70) / level.rows;

        int size = std::min(40, std::min(sizeWidth, sizeHeight));

        return size;
    }
    void saveResult(bool playerWon)
    {
        if(resultSaved){
            return;
        }
        resultSaved= true;

        std::ofstream file("wyniki.txt", std::ios::app);

        if(!file.is_open()){
            return;
        }

        if(playerWon){
            file << "Wygrana";
        }
        else{
            file<< "Przegrana";
        }
        file <<" | Tryb: "<< level.name;
        file <<" | Czas: "<< static_cast<int>(finalTime)<<" s";
        file <<"\n";
        file.close();
    }
    float currentTime()
    {
        if(!timerStarted){
            return 0.0f;
        }

        return gameClock.getElapsedTime().asSeconds();
    }
    void revealAllMines(){
        for(int r = 0; r < level.rows; r ++){
            for(int c =0; c <level.cols; c ++){
                if(map[r][c].mine){
                    map[r][c].revealed = true;
                }
            }
        }

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
    void drawLine(float x1, float y1, float x2, float y2, sf::Color color, float thickness = 1.0f) {
        sf::Vector2f p1(x1, y1);
        sf::Vector2f p2(x2, y2);
        sf::Vector2f d = p2 - p1;
        float len = std::sqrt(d.x * d.x + d.y * d.y);
        if (len == 0) return;
        sf::RectangleShape line(sf::Vector2f(len, thickness));
        line.setOrigin(0, thickness / 2.0f);
        line.setPosition(p1);
        line.setRotation(std::atan2(d.y, d.x) * 180.0f / 3.14159f);
        line.setFillColor(color);
        window.draw(line);
    }

    void drawSoldierIcon(float x, float y, float size) {
        sf::CircleShape head(size * 0.14f);
        head.setOrigin(size * 0.14f, size * 0.14f);
        head.setPosition(x + size * 0.5f, y + size * 0.28f);
        head.setFillColor(sf::Color(217, 183, 132));
        window.draw(head);

        sf::CircleShape helmet(size * 0.18f, 20);
        helmet.setScale(1.35f, 0.65f);
        helmet.setOrigin(size * 0.18f, size * 0.18f);
        helmet.setPosition(x + size * 0.5f, y + size * 0.19f);
        helmet.setFillColor(sf::Color(45, 75, 38));
        window.draw(helmet);

        sf::RectangleShape body(sf::Vector2f(size * 0.36f, size * 0.34f));
        body.setOrigin(size * 0.18f, size * 0.17f);
        body.setPosition(x + size * 0.5f, y + size * 0.58f);
        body.setFillColor(sf::Color(53, 86, 43));
        body.setOutlineThickness(1);
        body.setOutlineColor(sf::Color(25, 40, 24));
        window.draw(body);

        drawLine(x + size * 0.66f, y + size * 0.47f, x + size * 0.92f, y + size * 0.37f, sf::Color(40, 30, 20), size * 0.05f);
        drawLine(x + size * 0.38f, y + size * 0.76f, x + size * 0.34f, y + size * 0.96f, sf::Color(35, 50, 32), size * 0.08f);
        drawLine(x + size * 0.58f, y + size * 0.76f, x + size * 0.66f, y + size * 0.96f, sf::Color(35, 50, 32), size * 0.08f);
    }

    void drawSoldier() {
        int size = cellSize();
        float x = static_cast<float>(boardX() + playerCol * size);
        float y = static_cast<float>(boardY() + playerRow * size);
        drawSoldierIcon(x, y, static_cast<float>(size));
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
        drawText("Wyniki zapisuja sie do wyniki.txt", 850, 70, 15, sf::Color::Yellow);

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
                    // zamykanie aplikacji pod escape
                    if(event.key.code == sf::Keyboard::Escape) window.close();
                    return;
                }
                // funkcje do restartu lub wyjscia po state != game
                if(state == lost || state == won){

                    if(event.key.code == sf::Keyboard::R){
                        setDif(level);
                    }
                    if(event.key.code == sf::Keyboard::Escape){
                        state = menu;
                    }

                    return;
                }
                if(state != game){
                    return;
                }              // ruchy podczas gry
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

                    // Miny i odkrywanie
                    if(!s.revealed && !s.flagged){
                        if(s.mine){
                            s.revealed = true;
                            loss = true;
                            state = lost;
                            finalTime = currentTime();
                            revealAllMines();
                            saveResult(false);
                        }
                        else {
                            floodReveal(map, level, playerRow, playerCol);
                        }
                    }
                    // win state set
                    if(isWin(map, level)){
                        win = true;
                        state = won;
                        finalTime = currentTime();
                        saveResult(true);
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


        drawText("SAPER", 470, 120, 70, sf::Color::White);
        drawText("1 - Easy 9x9 10m" , 390, 280, 35, sf::Color::Green);
        drawText("2 - Medium 9x9 10m" , 390, 350, 35, sf::Color::Yellow);
        drawText("3 - Hard 9x9 10m" , 390, 420, 35, sf::Color::Red);

        drawText("Wybierz poziom trudnosci uzywajac klawiatuyr", 395, 530, 25, sf::Color::White);


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


        //generowanie mapy
        int cellsize = cellSize();
        int startX = boardX();
        int startY = boardY();

        //generowanie ramki
        sf::RectangleShape boardFrame(
            sf::Vector2f(level.cols * cellsize + 20, level.rows * cellsize + 20)
            );
        boardFrame.setPosition(startX -10, startY- 10);
        boardFrame.setFillColor(sf::Color(35,35,35));
        boardFrame.setOutlineColor(sf::Color(160,160,160));
        boardFrame.setOutlineThickness(4);
        window.draw(boardFrame);

        //generowanie planszy
        for(int r =0; r < level.rows; r++) {
            for (int c = 0; c < level.cols; c++){
                sf::RectangleShape tile(sf::Vector2f(cellsize -2, cellsize -2));
                tile.setPosition(startX + c * cellsize, startY + r *cellsize);

                if(map[r][c].revealed){
                    if((r+c)% 2 == 0){
                    tile.setFillColor(TileColorR);
                    }
                    else{
                    tile.setFillColor(TileColorR2);
                    }
                }
                else{
                    if((r+c)% 2 == 0){
                        tile.setFillColor(TileColor);
                    }
                    else{
                        tile.setFillColor(TileColor2);
                    }
                }
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
                    drawText(
                    std::to_string(map[r][c].nearMines),
                    startX +c * cellsize + cellsize *0.35f, startY +r * cellsize + cellsize *0.12f ,
                    24,
                    sf::Color::Blue
                        );
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

        drawSoldier();
        //Draw hud po win/loss
        if(loss){
            drawText("Przegrana - R restart, ESC menu", 330, 100, 35, sf::Color::Red);
        }


        if(win){
            drawText("Wygrana - R restart, ESC menu", 330, 100, 35, sf::Color::Green);
        }
        window.display();
    }

};





int main()
{
    SaperGame game;
    game.run();

    return 0;
}