#pragma once

namespace cfg {
    constexpr unsigned WindowWidth = 1180;
    constexpr unsigned WindowHeight = 820;
    constexpr float PlayfieldWidth = 860.0f;
    constexpr float SidebarWidth = WindowWidth - PlayfieldWidth;

    constexpr float PaddleY = 760.0f;
    constexpr float PaddleHeight = 18.0f;
    constexpr float PaddleBaseWidth = 140.0f;
    constexpr float PaddleMinWidth = 80.0f;
    constexpr float PaddleMaxWidth = 240.0f;
    constexpr float PaddleSpeed = 560.0f;

    constexpr float BallRadius = 10.0f;
    constexpr float BallBaseSpeed = 310.0f;
    constexpr float BallMaxSpeed = 650.0f;

    constexpr int BlockRows = 7;
    constexpr int BlockCols = 10;
    constexpr float BlockMarginX = 14.0f;
    constexpr float BlockMarginY = 16.0f;
    constexpr float BlockGap = 8.0f;
    constexpr float BlockTop = 60.0f;
    constexpr float BlockHeight = 28.0f;

    constexpr float BonusFallSpeed = 180.0f;
    constexpr float BottomShieldY = WindowHeight - 12.0f;
}
