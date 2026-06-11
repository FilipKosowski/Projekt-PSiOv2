#ifndef SAPERGAME_H
#define SAPERGAME_H

class SaperGameImpl;

class SaperGame
{
public:
    SaperGame();
    ~SaperGame();

    SaperGame(const SaperGame&) = delete;
    SaperGame& operator=(const SaperGame&) = delete;

    void run();

private:
    SaperGameImpl* impl;
};

#endif // SAPERGAME_H
