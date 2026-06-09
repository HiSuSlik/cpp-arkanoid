#pragma once

#include "Block.h"
#include "Bonus.h"
#include "GameTypes.h"

#include <SFML/Graphics.hpp>
#include <memory>
#include <random>
#include <vector>

class Game {
public:
    Game();
    ~Game();

    void run();

    void registerBlockHit();
    void notifyBlockDestroyed();
    void spawnBonus(std::unique_ptr<FallingBonus> bonus);

    void growPaddle(float delta);
    void shrinkPaddle(float delta);
    void multiplyBallSpeeds(float factor);
    void enableSticky(float durationSeconds);
    void enableBottomShield();
    void spawnSecondBallFromActive();

    void accelerateBall(Ball& ball, float factor);

    const sf::Font& font() const;
    bool hasFont() const;

    static float length(const sf::Vector2f& value);
    static sf::Vector2f normalize(const sf::Vector2f& value);
    static float dot(const sf::Vector2f& a, const sf::Vector2f& b);
    static sf::Vector2f clampBallVelocity(const sf::Vector2f& velocity);

private:
    void processEvents();
    void update(float dt);
    void render();

    void createLevel();
    void resetRound(bool keepScoreState);
    void resetBallsOnPaddle();

    void updatePaddle(float dt);
    void updateBalls(float dt);
    void updateBonuses(float dt);
    void updateStickyState(float dt);

    void handleWallCollision(Ball& ball);
    void handlePaddleCollision(Ball& ball);
    void handleBlockCollisions(Ball& ball);
    void handleBallBallCollisions();
    void handleBallLosses();

    void launchStuckBalls();

    void drawBlocks();
    void drawBalls();
    void drawPaddle();
    void drawBonuses();
    void drawShield();
    void drawSidebar();

private:
    sf::RenderWindow m_window;
    sf::Font m_font;
    bool m_hasFont = false;

    Paddle m_paddle;
    std::vector<Ball> m_balls;
    std::vector<std::unique_ptr<Block>> m_blocks;
    std::vector<std::unique_ptr<FallingBonus>> m_fallingBonuses;

    bool m_leftPressed = false;
    bool m_rightPressed = false;
    bool m_pause = false;
    bool m_bottomShieldAvailable = false;
    bool m_stickyEnabled = false;
    float m_stickyTimer = 0.0f;

    int m_score = 0;
    int m_hits = 0;
    int m_losses = 0;
    int m_remainingBreakable = 0;

    std::mt19937 m_rng;
};
