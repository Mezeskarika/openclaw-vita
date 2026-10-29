#ifndef DIFFICULTY_H_
#define DIFFICULTY_H_

#include <string>

// Normal is the game as OpenClaw plays it. Easy is the original game's Easy mode (Claw Design
// Bible, section 7.1): Claw's sword, fist and kick attacks hit harder, nothing else changes
enum Difficulty
{
    Difficulty_Normal,
    Difficulty_Easy
};

inline const char* DifficultyToString(Difficulty difficulty)
{
    return difficulty == Difficulty_Easy ? "Easy" : "Normal";
}

inline bool DifficultyFromString(const std::string& name, Difficulty& outDifficulty)
{
    if (name == "Normal")
    {
        outDifficulty = Difficulty_Normal;
        return true;
    }
    if (name == "Easy")
    {
        outDifficulty = Difficulty_Easy;
        return true;
    }
    return false;
}

// Damage of Claw's melee swipe. Normal uses OpenClaw's values (10, ducking 5, on catnip 100); Easy
// scales them by the Bible's Easy/Normal ratios: swipe 5 -> 20, ducking swipe 2 -> 10, catnip 10 -> 20.
// Pistol, magic, dynamite and the sword powerups are projectiles and stay the same, as in the original
inline int GetClawMeleeDamage(Difficulty difficulty, bool isDucking, bool hasCatnip)
{
    bool isEasy = difficulty == Difficulty_Easy;
    if (hasCatnip)
    {
        return isEasy ? 200 : 100;
    }
    if (isDucking)
    {
        return isEasy ? 25 : 5;
    }
    return isEasy ? 40 : 10;
}

#endif
