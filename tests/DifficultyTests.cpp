#include "Catch.hpp"

#include "../OpenClaw/Engine/GameApp/Difficulty.h"

TEST_CASE("----- DIFFICULTY -----")
{
    SECTION("Normal keeps OpenClaw's melee damage")
    {
        REQUIRE(GetClawMeleeDamage(Difficulty_Normal, false, false) == 10);
        REQUIRE(GetClawMeleeDamage(Difficulty_Normal, true, false) == 5);
        REQUIRE(GetClawMeleeDamage(Difficulty_Normal, false, true) == 100);
        REQUIRE(GetClawMeleeDamage(Difficulty_Normal, true, true) == 100);
    }

    SECTION("Easy scales it by the original game's Easy/Normal ratios")
    {
        REQUIRE(GetClawMeleeDamage(Difficulty_Easy, false, false) == 40);
        REQUIRE(GetClawMeleeDamage(Difficulty_Easy, true, false) == 25);
        REQUIRE(GetClawMeleeDamage(Difficulty_Easy, false, true) == 200);
    }

    SECTION("Names round-trip, unknown names are rejected")
    {
        Difficulty difficulty = Difficulty_Normal;
        REQUIRE(DifficultyFromString("Easy", difficulty));
        REQUIRE(difficulty == Difficulty_Easy);
        REQUIRE(std::string(DifficultyToString(difficulty)) == "Easy");
        REQUIRE(DifficultyFromString("Normal", difficulty));
        REQUIRE(difficulty == Difficulty_Normal);
        REQUIRE_FALSE(DifficultyFromString("Hard", difficulty));
        REQUIRE(difficulty == Difficulty_Normal);
    }
}
