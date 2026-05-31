#pragma once

#include <SFML/Graphics.hpp>
#include <random>
#include <string>
#include <vector>

class Game {
public:
    Game();
    void run();

    enum class BonusType {
        None,
        PaddleGrow,
        PaddleShrink,
        BallSpeedUp,
        BallSlowDown,
        Sticky,
        BottomShield,
        SecondBall
    };

private:
    enum class BlockType {
        Unbreakable,
        Bonus,
        Speed,
        Health
    };

    struct Ball {
        sf::Vector2f position{};
        sf::Vector2f velocity{};
        float radius = 10.0f;
        bool stuckToPaddle = false;
        float stuckOffsetX = 0.0f;
    };

    struct Paddle {
        sf::FloatRect rect{};
        float speed = 0.0f;
        float targetWidth = 0.0f;
    };

    struct Block {
        sf::FloatRect rect{};
        BlockType type = BlockType::Health;
        int health = 1;
        bool alive = true;
        BonusType hiddenBonus = BonusType::None;
    };

    struct FallingBonus {
        sf::Vector2f position{};
        BonusType type = BonusType::None;
        bool active = false;
        float radius = 14.0f;
    };

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

    void spawnBonus(const sf::Vector2f& center, BonusType type);
    void applyBonus(BonusType type);
    void launchStuckBalls();
    void triggerSecondBall(const sf::Vector2f& position);

    void markBlocksForDestruction(Block& block, Ball& ball);
    void startBottomShield();

    void drawBlocks();
    void drawBalls();
    void drawPaddle();
    void drawBonuses();
    void drawShield();
    void drawSidebar();
    void drawBonusIcon(sf::RenderTarget& target, const sf::Vector2f& center, BonusType type, float scale) const;

    sf::Color colorForBlock(const Block& block) const;
    sf::String textForBonus(BonusType type) const;
    char glyphForBonus(BonusType type) const;
    BonusType randomBonusType();

    static float length(const sf::Vector2f& value);
    static sf::Vector2f normalize(const sf::Vector2f& value);
    static float dot(const sf::Vector2f& a, const sf::Vector2f& b);
    static sf::Vector2f clampBallVelocity(const sf::Vector2f& velocity);

private:
    sf::RenderWindow m_window;
    sf::Font m_font;
    bool m_hasFont = false;

    Paddle m_paddle;
    std::vector<Ball> m_balls;
    std::vector<Block> m_blocks;
    std::vector<FallingBonus> m_fallingBonuses;

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
