#include "Game.h"

#include "Config.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace {
const sf::Color kBackgroundColor(228, 231, 236);
const sf::Color kPlayfieldColor(49, 57, 72);
const sf::Color kSidebarColor(240, 240, 242);
const sf::Color kPanelBorder(175, 180, 188);
const sf::Color kTextColor(40, 44, 54);
const sf::Color kSoftTextColor(84, 89, 99);
const sf::Color kShieldColor(70, 160, 255);
const sf::Color kPaddleColor(54, 63, 79);
const sf::Color kPaddleAccent(120, 214, 255);
const sf::Color kBallColor(245, 247, 252);

sf::String utf8(const char* s) {
    return sf::String::fromUtf8(s, s + std::strlen(s));
}
}

Game::Game()
    : m_window(sf::VideoMode(cfg::WindowWidth, cfg::WindowHeight), "ARKANOID (SFML)")
    , m_rng(std::random_device{}()) {
    m_window.setFramerateLimit(60);

    m_hasFont = m_font.loadFromFile("C:/Windows/Fonts/arial.ttf") ||
                m_font.loadFromFile("C:/Windows/Fonts/segoeui.ttf");

    m_paddle.rect = {(cfg::PlayfieldWidth - cfg::PaddleBaseWidth) * 0.5f, cfg::PaddleY, cfg::PaddleBaseWidth, cfg::PaddleHeight};
    m_paddle.speed = cfg::PaddleSpeed;
    m_paddle.targetWidth = cfg::PaddleBaseWidth;

    createLevel();
    resetRound(false);
}

Game::~Game() = default;

void Game::run() {
    sf::Clock clock;
    while (m_window.isOpen()) {
        processEvents();
        const float dt = clock.restart().asSeconds();
        if (!m_pause) {
            update(std::min(dt, 0.033f));
        }
        render();
    }
}

void Game::registerBlockHit() {
    ++m_score;
    ++m_hits;
}

void Game::notifyBlockDestroyed() {
    --m_remainingBreakable;
}

void Game::spawnBonus(std::unique_ptr<FallingBonus> bonus) {
    if (bonus) {
        m_fallingBonuses.push_back(std::move(bonus));
    }
}

void Game::growPaddle(float delta) {
    m_paddle.targetWidth = std::min(cfg::PaddleMaxWidth, m_paddle.targetWidth + delta);
}

void Game::shrinkPaddle(float delta) {
    m_paddle.targetWidth = std::max(cfg::PaddleMinWidth, m_paddle.targetWidth - delta);
}

void Game::multiplyBallSpeeds(float factor) {
    for (auto& ball : m_balls) {
        ball.velocity *= factor;
        ball.velocity = clampBallVelocity(ball.velocity);
    }
}

void Game::enableSticky(float durationSeconds) {
    m_stickyEnabled = true;
    m_stickyTimer = durationSeconds;
}

void Game::enableBottomShield() {
    m_bottomShieldAvailable = true;
}

void Game::spawnSecondBallFromActive() {
    if (m_balls.empty()) {
        return;
    }

    Ball ball;
    ball.radius = cfg::BallRadius;
    ball.position = m_balls.front().position + sf::Vector2f(18.0f, -8.0f);
    ball.velocity = clampBallVelocity({-220.0f, -cfg::BallBaseSpeed * 0.94f});
    m_balls.push_back(ball);
}

void Game::accelerateBall(Ball& ball, float factor) {
    ball.velocity *= factor;
    ball.velocity = clampBallVelocity(ball.velocity);
}

const sf::Font& Game::font() const {
    return m_font;
}

bool Game::hasFont() const {
    return m_hasFont;
}

void Game::processEvents() {
    sf::Event event{};
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        } else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                m_window.close();
            } else if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
                m_leftPressed = true;
            } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
                m_rightPressed = true;
            } else if (event.key.code == sf::Keyboard::Space) {
                launchStuckBalls();
            } else if (event.key.code == sf::Keyboard::P) {
                m_pause = !m_pause;
            } else if (event.key.code == sf::Keyboard::R) {
                createLevel();
                resetRound(false);
            }
        } else if (event.type == sf::Event::KeyReleased) {
            if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::A) {
                m_leftPressed = false;
            } else if (event.key.code == sf::Keyboard::Right || event.key.code == sf::Keyboard::D) {
                m_rightPressed = false;
            }
        }
    }
}

void Game::update(float dt) {
    updatePaddle(dt);
    updateStickyState(dt);
    updateBalls(dt);
    updateBonuses(dt);
    handleBallLosses();

    if (m_remainingBreakable <= 0) {
        createLevel();
        resetRound(true);
    }
}

void Game::render() {
    m_window.clear(kBackgroundColor);

    sf::RectangleShape field({cfg::PlayfieldWidth, static_cast<float>(cfg::WindowHeight)});
    field.setPosition(0.0f, 0.0f);
    field.setFillColor(kPlayfieldColor);
    m_window.draw(field);

    sf::RectangleShape sidebar({cfg::SidebarWidth - 24.0f, static_cast<float>(cfg::WindowHeight) - 40.0f});
    sidebar.setPosition(cfg::PlayfieldWidth + 12.0f, 20.0f);
    sidebar.setFillColor(kSidebarColor);
    sidebar.setOutlineThickness(2.0f);
    sidebar.setOutlineColor(kPanelBorder);
    m_window.draw(sidebar);

    drawBlocks();
    drawBonuses();
    drawShield();
    drawPaddle();
    drawBalls();
    drawSidebar();

    m_window.display();
}

void Game::createLevel() {
    m_blocks.clear();
    m_fallingBonuses.clear();
    m_remainingBreakable = 0;

    const float usableWidth = cfg::PlayfieldWidth - 2.0f * cfg::BlockMarginX - (cfg::BlockCols - 1) * cfg::BlockGap;
    const float blockWidth = usableWidth / static_cast<float>(cfg::BlockCols);

    std::uniform_int_distribution<int> bonusChance(0, 99);
    std::uniform_int_distribution<int> healthDist(2, 4);

    for (int row = 0; row < cfg::BlockRows; ++row) {
        for (int col = 0; col < cfg::BlockCols; ++col) {
            const sf::FloatRect rect(
                cfg::BlockMarginX + col * (blockWidth + cfg::BlockGap),
                cfg::BlockTop + row * (cfg::BlockHeight + cfg::BlockGap),
                blockWidth,
                cfg::BlockHeight
            );

            if (row == 1 && col % 2 == 0) {
                m_blocks.push_back(std::make_unique<BonusBlock>(rect, BonusFactory::createRandom(m_rng, {0.0f, 0.0f})));
                ++m_remainingBreakable;
            } else if (row == 2 && col % 3 == 1) {
                m_blocks.push_back(std::make_unique<SpeedBlock>(rect));
                ++m_remainingBreakable;
            } else if (row == 3 && (col % 3 == 0 || col == cfg::BlockCols - 1)) {
                m_blocks.push_back(std::make_unique<UnbreakableBlock>(rect));
            } else {
                const int health = healthDist(m_rng);
                if (bonusChance(m_rng) < 18) {
                    m_blocks.push_back(std::make_unique<BonusBlock>(rect, BonusFactory::createRandom(m_rng, {0.0f, 0.0f})));
                } else {
                    m_blocks.push_back(std::make_unique<HealthBlock>(rect, health));
                }
                ++m_remainingBreakable;
            }
        }
    }
}

void Game::resetRound(bool keepScoreState) {
    if (!keepScoreState) {
        m_score = 0;
        m_hits = 0;
        m_losses = 0;
    }

    m_bottomShieldAvailable = false;
    m_stickyEnabled = false;
    m_stickyTimer = 0.0f;
    m_paddle.rect.width = cfg::PaddleBaseWidth;
    m_paddle.targetWidth = cfg::PaddleBaseWidth;
    m_paddle.rect.left = (cfg::PlayfieldWidth - m_paddle.rect.width) * 0.5f;
    resetBallsOnPaddle();
}

void Game::resetBallsOnPaddle() {
    m_balls.clear();
    Ball ball;
    ball.radius = cfg::BallRadius;
    ball.position = {m_paddle.rect.left + m_paddle.rect.width * 0.5f, m_paddle.rect.top - ball.radius - 2.0f};
    ball.velocity = {cfg::BallBaseSpeed * 0.8f, -cfg::BallBaseSpeed};
    ball.stuckToPaddle = true;
    ball.stuckOffsetX = 0.0f;
    m_balls.push_back(ball);
}

void Game::updatePaddle(float dt) {
    float move = 0.0f;
    if (m_leftPressed) {
        move -= m_paddle.speed * dt;
    }
    if (m_rightPressed) {
        move += m_paddle.speed * dt;
    }

    m_paddle.rect.left += move;
    m_paddle.rect.left = std::clamp(m_paddle.rect.left, 0.0f, cfg::PlayfieldWidth - m_paddle.rect.width);

    if (std::abs(m_paddle.rect.width - m_paddle.targetWidth) > 0.1f) {
        const float center = m_paddle.rect.left + m_paddle.rect.width * 0.5f;
        m_paddle.rect.width += (m_paddle.targetWidth - m_paddle.rect.width) * std::min(1.0f, dt * 8.0f);
        m_paddle.rect.width = std::clamp(m_paddle.rect.width, cfg::PaddleMinWidth, cfg::PaddleMaxWidth);
        m_paddle.rect.left = center - m_paddle.rect.width * 0.5f;
        m_paddle.rect.left = std::clamp(m_paddle.rect.left, 0.0f, cfg::PlayfieldWidth - m_paddle.rect.width);
    }

    for (auto& ball : m_balls) {
        if (ball.stuckToPaddle) {
            ball.position.x = m_paddle.rect.left + m_paddle.rect.width * 0.5f + ball.stuckOffsetX;
            ball.position.y = m_paddle.rect.top - ball.radius - 2.0f;
        }
    }
}

void Game::updateBalls(float dt) {
    for (auto& ball : m_balls) {
        if (ball.stuckToPaddle) {
            continue;
        }

        ball.position += ball.velocity * dt;
        handleWallCollision(ball);
        handlePaddleCollision(ball);
        handleBlockCollisions(ball);
    }

    handleBallBallCollisions();
}

void Game::updateBonuses(float dt) {
    for (auto& bonus : m_fallingBonuses) {
        if (!bonus->isActive()) {
            continue;
        }

        bonus->update(dt, cfg::BonusFallSpeed);

        if (bonus->bounds().intersects(m_paddle.rect)) {
            bonus->apply(*this);
            bonus->deactivate();
        } else if (bonus->position().y - bonus->radius() > static_cast<float>(cfg::WindowHeight)) {
            bonus->deactivate();
        }
    }

    m_fallingBonuses.erase(
        std::remove_if(
            m_fallingBonuses.begin(),
            m_fallingBonuses.end(),
            [](const std::unique_ptr<FallingBonus>& bonus) { return !bonus->isActive(); }),
        m_fallingBonuses.end());
}

void Game::updateStickyState(float dt) {
    if (!m_stickyEnabled) {
        return;
    }

    m_stickyTimer -= dt;
    if (m_stickyTimer <= 0.0f) {
        m_stickyEnabled = false;
        m_stickyTimer = 0.0f;
    }
}

void Game::handleWallCollision(Ball& ball) {
    if (ball.position.x - ball.radius < 0.0f) {
        ball.position.x = ball.radius;
        ball.velocity.x = std::abs(ball.velocity.x);
    }
    if (ball.position.x + ball.radius > cfg::PlayfieldWidth) {
        ball.position.x = cfg::PlayfieldWidth - ball.radius;
        ball.velocity.x = -std::abs(ball.velocity.x);
    }
    if (ball.position.y - ball.radius < 0.0f) {
        ball.position.y = ball.radius;
        ball.velocity.y = std::abs(ball.velocity.y);
    }

    if (m_bottomShieldAvailable && ball.position.y + ball.radius >= cfg::BottomShieldY) {
        ball.position.y = cfg::BottomShieldY - ball.radius - 1.0f;
        ball.velocity.y = -std::abs(ball.velocity.y);
        m_bottomShieldAvailable = false;
    }
}

void Game::handlePaddleCollision(Ball& ball) {
    const sf::FloatRect ballRect(ball.position.x - ball.radius, ball.position.y - ball.radius, ball.radius * 2.0f, ball.radius * 2.0f);
    if (!ballRect.intersects(m_paddle.rect)) {
        return;
    }

    if (ball.velocity.y <= 0.0f && !ball.stuckToPaddle) {
        return;
    }

    const float paddleCenter = m_paddle.rect.left + m_paddle.rect.width * 0.5f;
    const float offset = (ball.position.x - paddleCenter) / (m_paddle.rect.width * 0.5f);

    ball.position.y = m_paddle.rect.top - ball.radius - 1.0f;

    if (m_stickyEnabled) {
        ball.stuckToPaddle = true;
        ball.stuckOffsetX = std::clamp(offset * (m_paddle.rect.width * 0.4f), -m_paddle.rect.width * 0.45f, m_paddle.rect.width * 0.45f);
        ball.velocity = {cfg::BallBaseSpeed * 0.8f, -cfg::BallBaseSpeed};
        return;
    }

    const float speed = length(ball.velocity);
    ball.velocity.x = speed * offset;
    ball.velocity.y = -std::sqrt(std::max(120.0f * 120.0f, speed * speed - ball.velocity.x * ball.velocity.x));
    ball.velocity = clampBallVelocity(ball.velocity);
}

void Game::handleBlockCollisions(Ball& ball) {
    const sf::FloatRect ballRect(ball.position.x - ball.radius, ball.position.y - ball.radius, ball.radius * 2.0f, ball.radius * 2.0f);

    for (auto& block : m_blocks) {
        if (!block->isAlive()) {
            continue;
        }
        if (!ballRect.intersects(block->bounds())) {
            continue;
        }

        const auto& rect = block->bounds();
        const float overlapLeft = (ball.position.x + ball.radius) - rect.left;
        const float overlapRight = (rect.left + rect.width) - (ball.position.x - ball.radius);
        const float overlapTop = (ball.position.y + ball.radius) - rect.top;
        const float overlapBottom = (rect.top + rect.height) - (ball.position.y - ball.radius);

        const float minX = std::min(overlapLeft, overlapRight);
        const float minY = std::min(overlapTop, overlapBottom);

        if (minX < minY) {
            if (overlapLeft < overlapRight) {
                ball.position.x = rect.left - ball.radius - 1.0f;
                ball.velocity.x = -std::abs(ball.velocity.x);
            } else {
                ball.position.x = rect.left + rect.width + ball.radius + 1.0f;
                ball.velocity.x = std::abs(ball.velocity.x);
            }
        } else {
            if (overlapTop < overlapBottom) {
                ball.position.y = rect.top - ball.radius - 1.0f;
                ball.velocity.y = -std::abs(ball.velocity.y);
            } else {
                ball.position.y = rect.top + rect.height + ball.radius + 1.0f;
                ball.velocity.y = std::abs(ball.velocity.y);
            }
        }

        block->onHit(*this, ball);
        break;
    }
}

void Game::handleBallBallCollisions() {
    for (std::size_t i = 0; i < m_balls.size(); ++i) {
        for (std::size_t j = i + 1; j < m_balls.size(); ++j) {
            if (m_balls[i].stuckToPaddle || m_balls[j].stuckToPaddle) {
                continue;
            }

            const sf::Vector2f delta = m_balls[j].position - m_balls[i].position;
            const float dist = length(delta);
            const float minDist = m_balls[i].radius + m_balls[j].radius;
            if (dist <= 0.001f || dist >= minDist) {
                continue;
            }

            const sf::Vector2f normal = delta / dist;
            const float penetration = minDist - dist;
            m_balls[i].position -= normal * (penetration * 0.5f);
            m_balls[j].position += normal * (penetration * 0.5f);

            const sf::Vector2f tangent(-normal.y, normal.x);
            const float v1n = dot(normal, m_balls[i].velocity);
            const float v1t = dot(tangent, m_balls[i].velocity);
            const float v2n = dot(normal, m_balls[j].velocity);
            const float v2t = dot(tangent, m_balls[j].velocity);

            m_balls[i].velocity = tangent * v1t + normal * v2n;
            m_balls[j].velocity = tangent * v2t + normal * v1n;

            m_balls[i].velocity = clampBallVelocity(m_balls[i].velocity);
            m_balls[j].velocity = clampBallVelocity(m_balls[j].velocity);
        }
    }
}

void Game::handleBallLosses() {
    std::vector<Ball> kept;
    kept.reserve(m_balls.size());

    for (auto& ball : m_balls) {
        if (ball.position.y - ball.radius <= static_cast<float>(cfg::WindowHeight)) {
            kept.push_back(ball);
            continue;
        }

        ++m_losses;
        m_paddle.targetWidth = std::max(cfg::PaddleMinWidth, m_paddle.targetWidth - 12.0f);
    }

    m_balls = std::move(kept);

    if (m_balls.empty()) {
        resetBallsOnPaddle();
    }
}

void Game::launchStuckBalls() {
    for (auto& ball : m_balls) {
        if (!ball.stuckToPaddle) {
            continue;
        }

        ball.stuckToPaddle = false;
        const float ratio = (ball.position.x - (m_paddle.rect.left + m_paddle.rect.width * 0.5f)) / (m_paddle.rect.width * 0.5f);
        ball.velocity = {220.0f * ratio, -cfg::BallBaseSpeed};
        ball.velocity = clampBallVelocity(ball.velocity);
    }
}

void Game::drawBlocks() {
    for (const auto& block : m_blocks) {
        if (block->isAlive()) {
            block->draw(m_window, m_font, m_hasFont);
        }
    }
}

void Game::drawBalls() {
    for (const auto& ball : m_balls) {
        sf::CircleShape shape(ball.radius);
        shape.setOrigin(ball.radius, ball.radius);
        shape.setPosition(ball.position);
        shape.setFillColor(kBallColor);
        shape.setOutlineThickness(2.0f);
        shape.setOutlineColor(sf::Color(96, 104, 124));
        m_window.draw(shape);

        sf::CircleShape shine(ball.radius * 0.33f);
        shine.setOrigin(shine.getRadius(), shine.getRadius());
        shine.setPosition(ball.position.x - ball.radius * 0.35f, ball.position.y - ball.radius * 0.35f);
        shine.setFillColor(sf::Color(255, 255, 255, 180));
        m_window.draw(shine);
    }
}

void Game::drawPaddle() {
    sf::RectangleShape paddle({m_paddle.rect.width, m_paddle.rect.height});
    paddle.setPosition(m_paddle.rect.left, m_paddle.rect.top);
    paddle.setFillColor(kPaddleColor);
    paddle.setOutlineThickness(2.0f);
    paddle.setOutlineColor(kPaddleAccent);
    m_window.draw(paddle);

    sf::RectangleShape accent({m_paddle.rect.width * 0.7f, 4.0f});
    accent.setPosition(m_paddle.rect.left + m_paddle.rect.width * 0.15f, m_paddle.rect.top + 4.0f);
    accent.setFillColor(kPaddleAccent);
    m_window.draw(accent);
}

void Game::drawBonuses() {
    for (const auto& bonus : m_fallingBonuses) {
        if (bonus->isActive()) {
            bonus->draw(m_window);
        }
    }
}

void Game::drawShield() {
    if (!m_bottomShieldAvailable) {
        return;
    }

    sf::RectangleShape shield({cfg::PlayfieldWidth, 8.0f});
    shield.setPosition(0.0f, cfg::BottomShieldY);
    shield.setFillColor(kShieldColor);
    m_window.draw(shield);
}

void Game::drawSidebar() {
    if (!m_hasFont) {
        return;
    }

    const float left = cfg::PlayfieldWidth + 34.0f;
    float top = 48.0f;

    auto drawLine = [&](const sf::String& text, unsigned size, const sf::Color& color, float extraGap = 0.0f) {
        sf::Text line(text, m_font, size);
        line.setFillColor(color);
        line.setPosition(left, top);
        m_window.draw(line);
        top += line.getLocalBounds().height + 16.0f + extraGap;
    };

    drawLine("ARKANOID", 31, kTextColor, 10.0f);
    drawLine(utf8("Попадания"), 18, kSoftTextColor, -10.0f);
    drawLine(sf::String(std::to_string(m_hits)), 26, kTextColor, 8.0f);
    drawLine(utf8("Очки"), 18, kSoftTextColor, -10.0f);
    drawLine(sf::String(std::to_string(m_score)), 26, kTextColor, 8.0f);
    drawLine(utf8("Потери"), 18, kSoftTextColor, -10.0f);
    drawLine(sf::String(std::to_string(m_losses)), 26, kTextColor, 8.0f);
    drawLine(utf8("Блоков осталось"), 18, kSoftTextColor, -10.0f);
    drawLine(sf::String(std::to_string(std::max(0, m_remainingBreakable))), 26, kTextColor, 18.0f);

    drawLine(utf8("Бонусы"), 22, kTextColor, 6.0f);
    auto shown = BonusFactory::createLegendSamples();

    for (const auto& item : shown) {
        const sf::Vector2f center(left + 18.0f, top + 12.0f);
        sf::CircleShape icon(12.0f);
        icon.setOrigin(12.0f, 12.0f);
        icon.setPosition(center);
        icon.setFillColor(item->color());
        icon.setOutlineThickness(2.0f);
        icon.setOutlineColor(sf::Color(36, 40, 48));
        m_window.draw(icon);
        item->drawIcon(m_window, center, 0.82f);

        sf::Text label(item->label(), m_font, 16);
        label.setFillColor(kSoftTextColor);
        label.setPosition(left + 38.0f, top - 1.0f);
        m_window.draw(label);
        top += 30.0f;
    }

    top += 12.0f;
    drawLine(utf8("Управление"), 22, kTextColor, 4.0f);

    const float controlLeft = left;
    auto drawControl = [&](const sf::String& text) {
        sf::Text line(text, m_font, 14);
        line.setFillColor(kSoftTextColor);
        line.setPosition(controlLeft, top);
        m_window.draw(line);
        top += 22.0f;
    };

    drawControl(utf8("A / D или ← / → — движение"));
    drawControl(utf8("Пробел — запуск прилипшего мяча"));
    drawControl(utf8("P — пауза"));
    drawControl(utf8("R — перезапуск уровня"));
}

float Game::length(const sf::Vector2f& value) {
    return std::sqrt(value.x * value.x + value.y * value.y);
}

sf::Vector2f Game::normalize(const sf::Vector2f& value) {
    const float len = length(value);
    if (len <= 0.0001f) {
        return {0.0f, 0.0f};
    }
    return value / len;
}

float Game::dot(const sf::Vector2f& a, const sf::Vector2f& b) {
    return a.x * b.x + a.y * b.y;
}

sf::Vector2f Game::clampBallVelocity(const sf::Vector2f& velocity) {
    sf::Vector2f result = velocity;
    float speed = length(result);
    if (speed < 120.0f) {
        speed = 120.0f;
    }
    if (speed > cfg::BallMaxSpeed) {
        speed = cfg::BallMaxSpeed;
    }

    const sf::Vector2f dir = normalize(result);
    if (std::abs(dir.x) < 0.18f) {
        result.x = (dir.x >= 0.0f ? 1.0f : -1.0f) * speed * 0.18f;
        result.y = (dir.y >= 0.0f ? 1.0f : -1.0f) * std::sqrt(std::max(0.0f, speed * speed - result.x * result.x));
    } else {
        result = dir * speed;
    }
    return result;
}
