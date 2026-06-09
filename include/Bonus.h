#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <random>
#include <vector>

class Game;

class FallingBonus {
public:
    explicit FallingBonus(const sf::Vector2f& position, float radius = 14.0f);
    virtual ~FallingBonus() = default;

    void update(float dt, float fallSpeed);
    bool isActive() const;
    void deactivate();
    float radius() const;
    const sf::Vector2f& position() const;
    sf::FloatRect bounds() const;

    void draw(sf::RenderTarget& target) const;

    virtual void apply(Game& game) const = 0;
    virtual void drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const = 0;
    virtual sf::String label() const = 0;
    virtual sf::Color color() const = 0;
    virtual std::unique_ptr<FallingBonus> cloneAt(const sf::Vector2f& position) const = 0;

protected:
    sf::Vector2f m_position{};
    bool m_active = true;
    float m_radius = 14.0f;
};

class PaddleGrowBonus final : public FallingBonus {
public:
    explicit PaddleGrowBonus(const sf::Vector2f& position);
    void apply(Game& game) const override;
    void drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const override;
    sf::String label() const override;
    sf::Color color() const override;
    std::unique_ptr<FallingBonus> cloneAt(const sf::Vector2f& position) const override;
};

class PaddleShrinkBonus final : public FallingBonus {
public:
    explicit PaddleShrinkBonus(const sf::Vector2f& position);
    void apply(Game& game) const override;
    void drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const override;
    sf::String label() const override;
    sf::Color color() const override;
    std::unique_ptr<FallingBonus> cloneAt(const sf::Vector2f& position) const override;
};

class BallSpeedUpBonus final : public FallingBonus {
public:
    explicit BallSpeedUpBonus(const sf::Vector2f& position);
    void apply(Game& game) const override;
    void drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const override;
    sf::String label() const override;
    sf::Color color() const override;
    std::unique_ptr<FallingBonus> cloneAt(const sf::Vector2f& position) const override;
};

class BallSlowDownBonus final : public FallingBonus {
public:
    explicit BallSlowDownBonus(const sf::Vector2f& position);
    void apply(Game& game) const override;
    void drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const override;
    sf::String label() const override;
    sf::Color color() const override;
    std::unique_ptr<FallingBonus> cloneAt(const sf::Vector2f& position) const override;
};

class StickyBonus final : public FallingBonus {
public:
    explicit StickyBonus(const sf::Vector2f& position);
    void apply(Game& game) const override;
    void drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const override;
    sf::String label() const override;
    sf::Color color() const override;
    std::unique_ptr<FallingBonus> cloneAt(const sf::Vector2f& position) const override;
};

class BottomShieldBonus final : public FallingBonus {
public:
    explicit BottomShieldBonus(const sf::Vector2f& position);
    void apply(Game& game) const override;
    void drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const override;
    sf::String label() const override;
    sf::Color color() const override;
    std::unique_ptr<FallingBonus> cloneAt(const sf::Vector2f& position) const override;
};

class SecondBallBonus final : public FallingBonus {
public:
    explicit SecondBallBonus(const sf::Vector2f& position);
    void apply(Game& game) const override;
    void drawIcon(sf::RenderTarget& target, const sf::Vector2f& center, float scale) const override;
    sf::String label() const override;
    sf::Color color() const override;
    std::unique_ptr<FallingBonus> cloneAt(const sf::Vector2f& position) const override;
};

class BonusFactory {
public:
    static std::unique_ptr<FallingBonus> createRandom(std::mt19937& rng, const sf::Vector2f& position);
    static std::vector<std::unique_ptr<FallingBonus>> createLegendSamples();
};
