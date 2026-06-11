#include "SaperGame.h"

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
#include <SFML/Audio.hpp>



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

class DifficultyLevel {
public:
    virtual ~DifficultyLevel() = default;

    virtual std::string name() const = 0;
    virtual int rows() const = 0;
    virtual int cols() const = 0;
    virtual int mines() const = 0;
};

class EasyLevel : public DifficultyLevel {
public:
    std::string name() const override { return "Easy"; }
    int rows() const override { return 9; }
    int cols() const override { return 9; }
    int mines() const override { return 10; }
};

class MediumLevel : public DifficultyLevel {
public:
    std::string name() const override { return "Medium"; }
    int rows() const override { return 12; }
    int cols() const override { return 12; }
    int mines() const override { return 25; }
};

class HardLevel : public DifficultyLevel {
public:
    std::string name() const override { return "Hard"; }
    int rows() const override { return 16; }
    int cols() const override { return 16; }
    int mines() const override { return 45; }
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
//Jedna z najważniejszych funckji jeżeli chodzi o logike gry, odkrywa pola które nie mają żadnych min obok siebie
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

// Funckja sprawdajaca czy wszyskie pola bez miny zostaly odkryte
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

class SaperGameImpl{
private:
    sf::RenderWindow window;
    std::vector<std::vector<Cell>> map;


    GameState state;
    int playerRow = 0;
    int playerCol = 0;

    sf::Font font;
    bool fontLoaded = false;

    sf::Clock frameClock;
    sf::Clock gameClock;
    sf::Clock animationClock;
    bool timerStarted = false;
    float finalTime = 0.0f;
    float explosionTime = 0.0f;

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

    sf::SoundBuffer explosionBuffer;
    sf::SoundBuffer winBuffer;
    sf::Sound explosionSound;
    sf::Sound winSound;


public:
    SaperGameImpl() :  window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Saper SFML"), state(menu){
        window.setFramerateLimit(60);
        loadFont();
        createSounds();
    }

    void run(){
        while (window.isOpen()){
            handleEvents();
            float dt = frameClock.restart().asSeconds();
            update(dt);
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
        animationClock.restart();
        state = game;
    }

    void setMap(const std::vector<std::vector<Cell>>& m){
        map = m;
    }
private:
    sf::FloatRect menuButtonRect(int i) const {
        return sf::FloatRect(420.0f, 330.0f + i * 100.0f, 350.0f, 72.0f);
    }
    void update(float dt) {
        if (state == lost) explosionTime += dt;
    }
    // funckja do wybeirania opcji menu myszka
    void handleMenuMouse(int mouseX, int mouseY) {
        for (int i = 0; i < 3; i++) {
            if (menuButtonRect(i).contains(static_cast<float>(mouseX), static_cast<float>(mouseY))) {
                setDif(lvls[i]);
            }
        }
    }
    //Tworzenie dziewkow
    void createSounds() {
        const unsigned sampleRate = 44100;
        std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
        std::uniform_int_distribution<int> noise(-29000, 29000);

        std::vector<sf::Int16> explosionSamples(sampleRate / 2);
        for (std::size_t i = 0; i < explosionSamples.size(); i++) {
            float fade = 1.0f - static_cast<float>(i) / explosionSamples.size();
            explosionSamples[i] = static_cast<sf::Int16>(noise(rng) * fade);
        }
        explosionBuffer.loadFromSamples(explosionSamples.data(), explosionSamples.size(), 1, sampleRate);
        explosionSound.setBuffer(explosionBuffer);

        std::vector<sf::Int16> winSamples(sampleRate / 2);
        for (std::size_t i = 0; i < winSamples.size(); i++) {
            float t = static_cast<float>(i) / sampleRate;
            float wave = std::sin(2.0f * 3.14159f * 523.0f * t) +
                         0.6f * std::sin(2.0f * 3.14159f * 784.0f * t);
            winSamples[i] = static_cast<sf::Int16>(wave * 9000);
        }
        winBuffer.loadFromSamples(winSamples.data(), winSamples.size(), 1, sampleRate);
        winSound.setBuffer(winBuffer);
    }
    //Wspolrzedne do rysowania
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
    void drawLeftHud() {
        sf::RectangleShape panel(sf::Vector2f(238, 630));
        panel.setPosition(18, 124);
        panel.setFillColor(sf::Color(29, 34, 25, 235));
        panel.setOutlineThickness(3);
        panel.setOutlineColor(sf::Color(126, 113, 72));
        window.draw(panel);

        drawText("RAPORT MISJI", 45, 145, 24, sf::Color(239, 224, 160));
        drawText("Zolnierz-saper musi", 38, 190, 17, sf::Color(218, 218, 202));
        drawText("oczyscic pole bitwy.", 38, 215, 17, sf::Color(218, 218, 202));
        drawText("Nie wchodz na miny!", 38, 240, 17, sf::Color(255, 211, 130));

        sf::RectangleShape map(sf::Vector2f(180, 150));
        map.setPosition(47, 290);
        map.setFillColor(sf::Color(52, 58, 39));
        map.setOutlineThickness(2);
        map.setOutlineColor(sf::Color(107, 96, 64));
        window.draw(map);

        for (int i = 0; i < 7; i++) {
            drawLine(55, 305 + i * 20, 220, 295 + i * 18, sf::Color(83, 91, 62), 2);
        }
        sf::CircleShape mine(cellSize() * 0.25f);
        mine.setPosition(83 ,328);
        mine.setFillColor(sf::Color::Black);
        window.draw(mine);


        drawSoldierIcon(102, 382, 45);

        drawText("STEROWANIE", 52, 482, 22, sf::Color(239, 224, 160));
        drawText("<-  ->/WASD  ruch", 55, 522, 18, sf::Color(218, 218, 202));
        drawText("SPACE odkryj", 55, 552, 18, sf::Color(218, 218, 202));
        drawText("F  -  flaga", 55, 582, 18, sf::Color(218, 218, 202));
        drawText("R  -  restart", 55, 612, 18, sf::Color(218, 218, 202));
        drawText("Pierwszy ruch bezpieczny!", 38, 692, 16, sf::Color(255, 222, 143));



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
    void drawBackground()
    {
        sf::RectangleShape sky(sf::Vector2f(WINDOW_WIDTH, 120));
        sky.setPosition(0, 0);
        sky.setFillColor(sf::Color(37, 43, 39));
        window.draw(sky);

        sf::RectangleShape ground(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT - 120));
        ground.setPosition(0, 120);
        ground.setFillColor(sf::Color(37, 45, 28));
        window.draw(ground);

        for(int i = 0; i < 12; i++){
            sf::CircleShape smoke(35.0f + i * 2.0f);
            smoke.setPosition(
                60.0f + i * 95.0f,
                35.0f + (i % 3) * 18.0f
                );
            smoke.setFillColor(sf::Color(70, 72, 68, 70));
            window.draw(smoke);
        }

        for(int i = 0; i < 18; i++){
            sf::CircleShape dirt(14.0f + (i % 4) * 5.0f);
            dirt.setPosition(
                static_cast<float>((i * 73) % WINDOW_WIDTH),
                160.0f + static_cast<float>((i * 41) % 560)
                );
            dirt.setFillColor(sf::Color(55, 48, 35, 80));
            window.draw(dirt);
        }
    }
    void drawTextInRect(const std::string& text, const sf::FloatRect& rect, int size, sf::Color color) {
        if (!fontLoaded) return;
        sf::Text t;
        t.setFont(font);
        t.setString(text);
        t.setCharacterSize(size);
        t.setFillColor(color);
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition(rect.left + (rect.width - b.width) / 2.0f - b.left,
                      rect.top + (rect.height - b.height) / 2.0f - b.top - 4.0f);
        window.draw(t);
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
    void drawCenteredText(const std::string& text, float y, int size, sf::Color color) {
        if (!fontLoaded) return;
        sf::Text t;
        t.setFont(font);
        t.setString(text);
        t.setCharacterSize(size);
        t.setFillColor(color);
        sf::FloatRect b = t.getLocalBounds();
        t.setPosition((WINDOW_WIDTH - b.width) / 2.0f - b.left, y);
        window.draw(t);
    }
    void drawEndScreen(bool win) {
        sf::RectangleShape overlay(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        overlay.setFillColor(sf::Color(0, 0, 0, 155));
        window.draw(overlay);

        sf::RectangleShape box(sf::Vector2f(640, 250));
        box.setPosition(270, 285);
        box.setFillColor(win ? sf::Color(42, 85, 42, 245) : sf::Color(90, 28, 24, 245));
        box.setOutlineThickness(5);
        box.setOutlineColor(sf::Color(230, 211, 140));
        window.draw(box);

        if (win) {
            drawCenteredText("MISJA WYKONANA", 323, 46, sf::Color(229, 255, 211));
            drawCenteredText("Teren zostal oczyszczony", 382, 24, sf::Color(235, 235, 225));
        } else {
            drawCenteredText("MISJA NIEUDANA", 323, 46, sf::Color(255, 210, 205));
            drawCenteredText("Zolnierz trafil na mine", 382, 24, sf::Color(235, 235, 225));
        }

        drawCenteredText("Czas: " + std::to_string(static_cast<int>(finalTime)) + " s", 430, 26, sf::Color(255, 225, 145));
        drawCenteredText("R - restart     ENTER / ESC - menu", 478, 23, sf::Color(235, 235, 225));
    }

    void drawHud(){
        drawLeftHud();
        drawLine(0,86, WINDOW_WIDTH, 86, sf::Color(160,140,80), 4);
        sf::RectangleShape panel(sf::Vector2f(WINDOW_WIDTH, 90));
        panel.setPosition(0, 0);
        panel.setFillColor(sf::Color(25, 25, 25, 220));
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
    void drawExplosion() {
        int size = cellSize();
        float centerX = boardX() + playerCol * size + size / 2.0f;
        float centerY = boardY() + playerRow * size + size / 2.0f;
        float radius = 30.0f + explosionTime * 230.0f;

        for (int i = 0; i < 4; i++) {
            float r = radius - i * 28.0f;
            if (r < 4.0f) continue;
            sf::CircleShape circle(r);
            circle.setOrigin(r, r);
            circle.setPosition(centerX, centerY);
            if (i == 0) circle.setFillColor(sf::Color(130, 25, 20, 115));
            if (i == 1) circle.setFillColor(sf::Color(218, 66, 32, 145));
            if (i == 2) circle.setFillColor(sf::Color(255, 157, 42, 165));
            if (i == 3) circle.setFillColor(sf::Color(255, 236, 89, 185));
            window.draw(circle);
        }

        for (int i = 0; i < 18; i++) {
            float angle = i * 3.14159f * 2.0f / 18.0f;
            float dist = 28.0f + explosionTime * (110.0f + (i % 4) * 30.0f);
            sf::CircleShape blood(4.0f + i % 3);
            blood.setPosition(centerX + std::cos(angle) * dist, centerY + std::sin(angle) * dist);
            blood.setFillColor(sf::Color(126, 0, 0, 190));
            window.draw(blood);
        }
    }



    void handleEvents(){
        sf::Event event {};

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed){
                window.close();
            }

                if (event.type == sf::Event::MouseButtonPressed && state == menu) {
                    handleMenuMouse(event.mouseButton.x, event.mouseButton.y);
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
                if(event.key.code == sf::Keyboard::Escape){
                    state = menu;
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
                            explosionTime = 0.0f;
                            explosionSound.play();

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
                        winSound.play();
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



    void drawMenu() {
        drawBackground();
        sf::RectangleShape titleBox(sf::Vector2f(820, 118));
        titleBox.setPosition(180, 74);
        titleBox.setFillColor(sf::Color(28, 34, 26, 230));
        titleBox.setOutlineThickness(4);
        titleBox.setOutlineColor(sf::Color(168, 145, 82));
        window.draw(titleBox);

        drawCenteredText("SAPER", 86, 70, sf::Color(236, 222, 162));
        drawCenteredText("OPERACJA MINOWA", 154, 30, sf::Color(201, 214, 164));


        drawCenteredText("WYBIERZ TRYB MISJI", 255, 28, sf::Color(230, 230, 220));

        sf::Vector2i mouse = sf::Mouse::getPosition(window);
        for (int i = 0; i < 3; i++) {
            sf::FloatRect rect = menuButtonRect(i);
            bool hover = rect.contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));

            sf::RectangleShape shadow(sf::Vector2f(rect.width, rect.height));
            shadow.setPosition(rect.left + 7, rect.top + 8);
            shadow.setFillColor(sf::Color(0, 0, 0, 130));
            window.draw(shadow);

            sf::RectangleShape button(sf::Vector2f(rect.width, rect.height));
            button.setPosition(rect.left, rect.top);
            button.setFillColor(hover ? sf::Color(102, 112, 62) : sf::Color(72, 83, 48));
            button.setOutlineThickness(3);
            button.setOutlineColor(hover ? sf::Color(235, 198, 95) : sf::Color(155, 132, 77));
            window.draw(button);

            sf::RectangleShape numberBox(sf::Vector2f(58, 58));
            numberBox.setPosition(rect.left + 8, rect.top + 7);
            numberBox.setFillColor(sf::Color(38, 45, 31));
            numberBox.setOutlineThickness(2);
            numberBox.setOutlineColor(sf::Color(185, 164, 93));
            window.draw(numberBox);
            drawTextInRect(std::to_string(i + 1), sf::FloatRect(rect.left + 8, rect.top + 7, 58, 58), 32, sf::Color(255, 227, 143));

            drawText(lvls[i].name, rect.left + 82, rect.top + 11, 27, sf::Color(245, 240, 220));


            std::string info = std::to_string(lvls[i].rows) + "x" + std::to_string(lvls[i].cols) +
                               "  MINY: " + std::to_string(lvls[i].mines);
            drawText(info, rect.left + 220, rect.top + 25, 18, sf::Color(255, 221, 136));
        }

        sf::RectangleShape controlBox(sf::Vector2f(780, 76));
        controlBox.setPosition(200, 650);
        controlBox.setFillColor(sf::Color(23, 29, 22, 230));
        controlBox.setOutlineThickness(2);
        controlBox.setOutlineColor(sf::Color(126, 116, 75));
        window.draw(controlBox);

        drawCenteredText("Strzalki - ruch zolnierza     SPACE - odkryj pole     F - flaga", 665, 21, sf::Color(229, 229, 214));
        drawCenteredText("Plansza generuje sie dopiero po pierwszym odkryciu pola", 696, 19, sf::Color(255, 222, 143));
        window.display();
    }

    void draw(){
        if(state == menu){
            drawMenu();
            return;
        }

        window.clear(sf::Color(0, 0, 0));
        drawBackground();
        drawHud();



        // stale do generowania mapy itp.
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

        // animacja po przegranej
        if(state == lost) drawExplosion();
        // wyswietlanie ekranu koncowego
        if(state !=game)
            drawEndScreen(win);
        window.display();
    }

};



//Ukrywanie klasy aby nie wyswietlac jej szczegow w .h

SaperGame::SaperGame()
    : impl(new SaperGameImpl())
{
}

SaperGame::~SaperGame()
{
    delete impl;
}

void SaperGame::run()
{
    impl->run();
}
