#pragma once

#include "GameTypes.h"

#include <SFML/Graphics.hpp>
#include <memory>

class Game;
class FallingBonus;

class Block {
public:
    explicit Block(const sf::FloatRect& rect);
    virtual ~Block() = default;

    const sf::FloatRect& bounds() const;
    bool isAlive() const;
    bool isBreakable() const;

    virtual void onHit(Game& game, Ball& ball) = 0;
    virtual void draw(sf::RenderTarget& target, const sf::Font& font, bool hasFont) const = 0;

protected:
    void destroy();
    const sf::FloatRect m_rect;
    bool m_alive = true;
};

class UnbreakableBlock final : public Block {
public:
    explicit UnbreakableBlock(const sf::FloatRect& rect);
    void onHit(Game& game, Ball& ball) override;
    void draw(sf::RenderTarget& target, const sf::Font& font, bool hasFont) const override;
};

class SpeedBlock final : public Block {
public:
    explicit SpeedBlock(const sf::FloatRect& rect);
    void onHit(Game& game, Ball& ball) override;
    void draw(sf::RenderTarget& target, const sf::Font& font, bool hasFont) const override;
};

class HealthBlock final : public Block {
public:
    HealthBlock(const sf::FloatRect& rect, int health);
    void onHit(Game& game, Ball& ball) override;
    void draw(sf::RenderTarget& target, const sf::Font& font, bool hasFont) const override;

private:
    int m_health = 1;
};

class BonusBlock final : public Block {
public:
    BonusBlock(const sf::FloatRect& rect, std::unique_ptr<FallingBonus> hiddenBonus);
    ~BonusBlock() override;

    void onHit(Game& game, Ball& ball) override;
    void draw(sf::RenderTarget& target, const sf::Font& font, bool hasFont) const override;

private:
    std::unique_ptr<FallingBonus> m_hiddenBonus;
};
