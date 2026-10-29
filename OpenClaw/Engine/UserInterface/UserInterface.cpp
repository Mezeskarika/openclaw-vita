#include "UserInterface.h"

#include "../Graphics2D/Image.h"
#include "../GameApp/BaseGameApp.h"
#include "../GameApp/BaseGameLogic.h"
#include "../GameApp/GameSaves.h"
#include "../Resource/Loaders/PcxLoader.h"
#include "../Resource/Loaders/PngLoader.h"
#include "../Resource/Loaders/PidLoader.h"
#include "../Resource/ResourceMgr.h"
#include "../Audio/Audio.h"

#include <cctype>
#include <cstdlib>
#include <sstream>

std::map<std::string, MenuPage> g_StringToMenuPageEnumMap =
{
    { "MenuPage_Main",                          MenuPage_Main },
    { "MenuPage_SinglePlayer",                  MenuPage_SinglePlayer },
    { "MenuPage_Multiplayer",                   MenuPage_Multiplayer },
    { "MenuPage_ReplayMovies",                  MenuPage_ReplayMovies },
    { "MenuPage_Options",                       MenuPage_Options },
    { "MenuPage_Credits",                       MenuPage_Credits },
    { "MenuPage_Help",                          MenuPage_Help },
    { "MenuPage_QuitGame",                      MenuPage_QuitGame },
    { "MenuPage_Help_TouchScreen",              MenuPage_Help_TouchScreen },
    { "MenuPage_SinglePlayer_NewGame",          MenuPage_SinglePlayer_NewGame },
    { "MenuPage_SinglePlayer_LoadGame",         MenuPage_SinglePlayer_LoadGame },
    { "MenuPage_SinglePlayer_LoadCustomLevel",  MenuPage_SinglePlayer_LoadCustomLevel },
    { "MenuPage_SinglePlayer_SaveGame",         MenuPage_SinglePlayer_SaveGame },
    { "MenuPage_SinglePlayer_UploadScores",     MenuPage_SinglePlayer_UploadScores },
    { "MenuPage_SinglePlayer_LoadGame_Level1",  MenuPage_SinglePlayer_LoadGame_Level1 },
    { "MenuPage_SinglePlayer_LoadGame_Level2",  MenuPage_SinglePlayer_LoadGame_Level2 },
    { "MenuPage_SinglePlayer_LoadGame_Level3",  MenuPage_SinglePlayer_LoadGame_Level3 },
    { "MenuPage_SinglePlayer_LoadGame_Level4",  MenuPage_SinglePlayer_LoadGame_Level4 },
    { "MenuPage_SinglePlayer_LoadGame_Level5",  MenuPage_SinglePlayer_LoadGame_Level5 },
    { "MenuPage_SinglePlayer_LoadGame_Level6",  MenuPage_SinglePlayer_LoadGame_Level6 },
    { "MenuPage_SinglePlayer_LoadGame_Level7",  MenuPage_SinglePlayer_LoadGame_Level7 },
    { "MenuPage_SinglePlayer_LoadGame_Level8",  MenuPage_SinglePlayer_LoadGame_Level8 },
    { "MenuPage_SinglePlayer_LoadGame_Level9",  MenuPage_SinglePlayer_LoadGame_Level9 },
    { "MenuPage_SinglePlayer_LoadGame_Level10", MenuPage_SinglePlayer_LoadGame_Level10 },
    { "MenuPage_SinglePlayer_LoadGame_Level11", MenuPage_SinglePlayer_LoadGame_Level11 },
    { "MenuPage_SinglePlayer_LoadGame_Level12", MenuPage_SinglePlayer_LoadGame_Level12 },
    { "MenuPage_SinglePlayer_LoadGame_Level13", MenuPage_SinglePlayer_LoadGame_Level13 },
    { "MenuPage_SinglePlayer_LoadGame_Level14", MenuPage_SinglePlayer_LoadGame_Level14 },
    { "MenuPage_Options_EditPlayers",           MenuPage_Options_EditPlayers },
    { "MenuPage_Options_Difficulty",            MenuPage_Options_Difficulty },
    { "MenuPage_Options_Controls",              MenuPage_Options_Controls },
    { "MenuPage_Options_Display",               MenuPage_Options_Display },
    { "MenuPage_Options_Gameplay",              MenuPage_Options_Gameplay },
    { "MenuPage_Options_Audio",                 MenuPage_Options_Audio },
    { "MenuPage_Multiplayer_LevelRacing",       MenuPage_Multiplayer_LevelRacing },
    { "MenuPage_Multiplayer_EditMacros",        MenuPage_Multiplayer_EditMacros },

    // Ingame menu
    { "MenuPage_EndGame",                       MenuPage_EndGame },
    { "MenuPage_EndLife",                       MenuPage_EndLife }
};

std::map<std::string, SDL_Scancode> g_StringToSDLKeyCodeMap =
{
    { "Escape", SDL_SCANCODE_ESCAPE },
    { "Space",  SDL_SCANCODE_SPACE },
    { "Enter",  SDL_SCANCODE_RETURN },
    { "A", SDL_SCANCODE_A },
    { "B", SDL_SCANCODE_B },
    { "C", SDL_SCANCODE_C },
    { "D", SDL_SCANCODE_D },
    { "E", SDL_SCANCODE_E },
    { "F", SDL_SCANCODE_F },
    { "G", SDL_SCANCODE_G },
    { "H", SDL_SCANCODE_H },
    { "I", SDL_SCANCODE_I },
    { "J", SDL_SCANCODE_J },
    { "K", SDL_SCANCODE_K },
    { "L", SDL_SCANCODE_L },
    { "M", SDL_SCANCODE_M },
    { "N", SDL_SCANCODE_N },
    { "O", SDL_SCANCODE_O },
    { "P", SDL_SCANCODE_P },
    { "Q", SDL_SCANCODE_Q },
    { "R", SDL_SCANCODE_R },
    { "S", SDL_SCANCODE_S },
    { "T", SDL_SCANCODE_T },
    { "U", SDL_SCANCODE_U },
    { "V", SDL_SCANCODE_V },
    { "W", SDL_SCANCODE_W },
    { "X", SDL_SCANCODE_X },
    { "Y", SDL_SCANCODE_Y },
    { "Z", SDL_SCANCODE_Z },
    { "0", SDL_SCANCODE_0 },
    { "1", SDL_SCANCODE_1 },
    { "2", SDL_SCANCODE_2 },
    { "3", SDL_SCANCODE_3 },
    { "4", SDL_SCANCODE_4 },
    { "5", SDL_SCANCODE_5 },
    { "6", SDL_SCANCODE_6 },
    { "7", SDL_SCANCODE_7 },
    { "8", SDL_SCANCODE_8 },
    { "LeftArrow", SDL_SCANCODE_LEFT },
    { "RightArrow", SDL_SCANCODE_RIGHT },
};

std::map<std::string, MenuItemType> g_StringToMenuItemType =
{
    { "Text",       MenuItemType_Text },
    { "Button",     MenuItemType_Button },
    { "Slider",     MenuItemType_Slider },
    { "Image",      MenuItemType_Image },
};

static SDL_Rect GetScreenRect()
{
    Point windowSize = g_pApp->GetWindowSize();
    Point scale = g_pApp->GetScale();
    int targetWidth = (int)(windowSize.x / scale.x);
    int targetHeight = (int)(windowSize.y / scale.y);

    return { 0, 0, targetWidth, targetHeight };
}

// Width of the menu layout: item positions are in the 640x480 space of the original menus
static const int MENU_LAYOUT_WIDTH = 640;

// Distance between the end stops of the Display page's play area slider
static const int PLAY_AREA_KNOB_TRAVEL = 195;

// Labels the game data has no image for are drawn with this font (Cinzel, SIL Open Font Licence;
// its lower case letters are small capitals, like the original labels)
static const char* MENU_LABEL_FONT = "fonts/Cinzel-Bold.ttf";

// Recolours the opaque pixels of a 32-bit surface with a vertical gradient, keeping their alpha
static void ApplyVerticalGradient(SDL_Surface* pSurface, SDL_Color top, SDL_Color bottom)
{
    SDL_LockSurface(pSurface);
    for (int y = 0; y < pSurface->h; y++)
    {
        float t = pSurface->h > 1 ? (float)y / (pSurface->h - 1) : 0.0f;
        Uint8 r = (Uint8)(top.r + (bottom.r - top.r) * t);
        Uint8 g = (Uint8)(top.g + (bottom.g - top.g) * t);
        Uint8 b = (Uint8)(top.b + (bottom.b - top.b) * t);

        Uint32* pRow = (Uint32*)((Uint8*)pSurface->pixels + y * pSurface->pitch);
        for (int x = 0; x < pSurface->w; x++)
        {
            Uint8 pr, pg, pb, pa;
            SDL_GetRGBA(pRow[x], pSurface->format, &pr, &pg, &pb, &pa);
            pRow[x] = SDL_MapRGBA(pSurface->format, r, g, b, pa);
        }
    }
    SDL_UnlockSurface(pSurface);
}

// A menu label in the style of the original ones: gold letters with a dark outline, brighter
// letters with a light outline when selected
static shared_ptr<Image> CreateLabelImage(const std::string& text, int fontSize, bool isActive)
{
    // One font per size, kept open for the lifetime of the game
    static std::map<int, TTF_Font*> s_Fonts;
    TTF_Font*& pFont = s_Fonts[fontSize];
    if (pFont == NULL)
    {
        std::string fontPath = g_pApp->GetGameConfig()->assetsFolder + MENU_LABEL_FONT;
        pFont = TTF_OpenFont(fontPath.c_str(), fontSize);
        if (pFont == NULL)
        {
            LOG_ERROR("Could not open menu font " + fontPath + ": " + std::string(TTF_GetError()));
            return nullptr;
        }
    }

    const int outline = 2;
    const SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color outlineColor = isActive ? SDL_Color{ 255, 250, 225, 255 } : SDL_Color{ 30, 14, 0, 255 };

    TTF_SetFontOutline(pFont, outline);
    SDL_Surface* pLabel = TTF_RenderUTF8_Blended(pFont, text.c_str(), outlineColor);
    TTF_SetFontOutline(pFont, 0);
    SDL_Surface* pLetters = TTF_RenderUTF8_Blended(pFont, text.c_str(), white);
    if (pLabel == NULL || pLetters == NULL)
    {
        LOG_ERROR("Could not draw menu label " + text + ": " + std::string(TTF_GetError()));
        SDL_FreeSurface(pLabel);
        SDL_FreeSurface(pLetters);
        return nullptr;
    }

    if (isActive)
    {
        ApplyVerticalGradient(pLetters, SDL_Color{ 255, 236, 120, 255 }, SDL_Color{ 235, 150, 20, 255 });
    }
    else
    {
        ApplyVerticalGradient(pLetters, SDL_Color{ 250, 220, 110, 255 }, SDL_Color{ 190, 120, 20, 255 });
    }

    SDL_Rect lettersRect = { outline, outline, pLetters->w, pLetters->h };
    SDL_BlitSurface(pLetters, NULL, pLabel, &lettersRect);
    SDL_Texture* pTexture = SDL_CreateTextureFromSurface(g_pApp->GetRenderer(), pLabel);
    SDL_FreeSurface(pLetters);
    SDL_FreeSurface(pLabel);
    if (pTexture == NULL)
    {
        LOG_ERROR("Could not create texture for menu label " + text);
        return nullptr;
    }

    return shared_ptr<Image>(new Image(pTexture));
}

static shared_ptr<Image> TryLoadPcxImageFromXmlElement(TiXmlElement* pElem)
{
    if (pElem == NULL)
    {
        return nullptr;
    }

    std::string imagePath = pElem->GetText();
    // This should not happen
    assert(!imagePath.empty());

    return PcxResourceLoader::LoadAndReturnImage(imagePath.c_str(), true, { 0, 0, 0, 0 });
}

static shared_ptr<Image> LoadImageFromXmlElement(TiXmlElement* pElem)
{
    if (pElem == NULL)
    {
        return nullptr;
    }

    std::string imagePath = pElem->GetText();
    // Length has to be atleast 5: A.BCD - one char for name, dot and 3 chars extension
    if (imagePath.length() <= 5)
    {
        return nullptr;
    }

    // Transform to lowercase to support all naming conventions
    std::transform(imagePath.begin(), imagePath.end(), imagePath.begin(), (int(*)(int)) std::tolower);

    // We are ASSUMING here that all PCXs are from original CLAW.REZ archive
    // For our own purpose we will use PNGs or JPEGs
    std::string extension = imagePath.substr(imagePath.length() - 4);
    shared_ptr<Image> pImage;
    if (extension == ".pcx")
    {
        pImage = PcxResourceLoader::LoadAndReturnImage(imagePath.c_str(), true, { 0, 0, 0, 0 });
    }
    else if (extension == ".png")
    {
        pImage = PngResourceLoader::LoadAndReturnImage(imagePath.c_str());
    }
    else if (extension == ".pid")
    {
        pImage = PidResourceLoader::LoadAndReturnImage(imagePath.c_str(), g_pApp->GetCurrentPalette());
    }
    else if (extension == ".jpg")
    {
        assert(false && "Unsupported");
    }
    else if (extension == ".jpeg")
    {
        assert(false && "Unsupported");
    }

    return pImage;
}

static MenuItemType StringToMenuItemTypeEnum(const std::string& str)
{
    if (str == "Text")
    {
        return MenuItemType_Text;
    }
    else if (str == "Button")
    {
        return MenuItemType_Button;
    }
    else if (str == "Slider")
    {
        return MenuItemType_Slider;
    }
    
    LOG_ERROR("Conflicting string: " + str);
    assert(false && "Unknown menu item type string");

    return MenuItemType_None;
}

static MenuPage StringToMenuPageEnum(const std::string& str)
{
    auto findIt = g_StringToMenuPageEnumMap.find(str);
    if (findIt == g_StringToMenuPageEnumMap.end())
    {
        LOG_ERROR("Conflicting string: " + str);
        assert(false && "Unknown menu page string");
    }

    return findIt->second;
}

static SDL_Scancode StringToSDLKeycode(const std::string& str)
{
    auto findIt = g_StringToSDLKeyCodeMap.find(str);
    if (findIt == g_StringToSDLKeyCodeMap.end())
    {
        LOG_ERROR("Conflicting string: " + str);
        assert(false && "Unknown hotkey string");
    }

    return findIt->second;
}

static MenuItemState StringToMenuItemStateEnum(const std::string& str)
{
    if (str == "Active")
    {
        return MenuItemState_Active;
    }
    else if (str == "Disabled")
    {
        return MenuItemState_Disabled;
    }
    else if (str == "Inactive")
    {
        return MenuItemState_Inactive;
    }

    LOG_ERROR("Conflicting string: " + str);
    assert(false && "Unknown menu item state string");

    return MenuItemState_None;
}

static MenuItemType StringToMenuItemType(const std::string& str)
{
    auto findIt = g_StringToMenuItemType.find(str);
    if (findIt == g_StringToMenuItemType.end())
    {
        LOG_ERROR("Conflicting string: " + str);
        assert(false && "Unknown menu item type");
    }

    return findIt->second;
}

static IEventDataPtr XmlElemToGeneratedEvent(TiXmlElement* pElem)
{
    if (pElem == NULL)
    {
        return nullptr;
    }

    IEventDataPtr pEventData;
    std::string eventType;
    ParseValueFromXmlElem(&eventType, pElem->FirstChildElement("Type"));
    if (eventType == "SwitchPage")
    {
        std::string pageName;
        ParseValueFromXmlElem(&pageName, pElem->FirstChildElement("PageName"));

        pEventData.reset(new EventData_Menu_SwitchPage(pageName));
    }
    else if (eventType == "LoadGame")
    {
        bool isNewGame;
        int levelNumber, checkpointNumber;
        ParseValueFromXmlElem(&isNewGame, pElem->FirstChildElement("IsNewGame"));
        ParseValueFromXmlElem(&levelNumber, pElem->FirstChildElement("LevelNumber"));
        ParseValueFromXmlElem(&checkpointNumber, pElem->FirstChildElement("CheckpointNumber"));

        // The menus' LoadGame events are new games or the Load Game pages
        pEventData.reset(new EventData_Menu_LoadGame(levelNumber, isNewGame, checkpointNumber, !isNewGame));
        //pEventData.reset(new EventData_Finished_Level());
    }
    else if (eventType == "QuitGame")
    {
        pEventData.reset(new EventData_Quit_Game());
    }
    else if (eventType == "ChangeSoundEnabled")
    {
        bool isEnabled;
        bool isMusic;
        ParseValueFromXmlElem(&isEnabled, pElem->FirstChildElement("IsEnabled"));
        ParseValueFromXmlElem(&isMusic, pElem->FirstChildElement("IsMusic"));

        pEventData.reset(new EventData_Sound_Enabled_Changed(isEnabled, isMusic));
    }
    else if (eventType == "PlayMovies")
    {
        // <Movies>1,2,3</Movies>: indices in CLAW.FEC, played one after the other
        std::string moviesStr;
        ParseValueFromXmlElem(&moviesStr, pElem->FirstChildElement("Movies"));
        std::vector<int> movies;
        std::istringstream moviesStream(moviesStr);
        std::string movie;
        while (std::getline(moviesStream, movie, ','))
        {
            movies.push_back(std::atoi(movie.c_str()));
        }

        pEventData.reset(new EventData_Play_Movies(movies));
    }
    else if (eventType == "SetOption")
    {
        std::string option;
        std::string value;
        ParseValueFromXmlElem(&option, pElem->FirstChildElement("Option"));
        ParseValueFromXmlElem(&value, pElem->FirstChildElement("Value"));

        pEventData.reset(new EventData_Menu_Set_Option(option, value));
    }
    else if (eventType == "ModifyMenuItemVisibility")
    {
        std::string menuItemName;
        bool isVisible;
        ParseValueFromXmlElem(&menuItemName, pElem->FirstChildElement("MenuItemName"));
        ParseValueFromXmlElem(&isVisible, pElem->FirstChildElement("IsVisible"));

        pEventData.reset(new EventData_Menu_Modifiy_Item_Visibility(menuItemName, isVisible));
    }
    else if (eventType == "PlaySound")
    {
        std::string soundName;
        int volume = 100;
        bool isMusic;
        int loops = 0;
        ParseValueFromXmlElem(&soundName, pElem->FirstChildElement("SoundName"));
        ParseValueFromXmlElem(&volume, pElem->FirstChildElement("Volume"));
        ParseValueFromXmlElem(&isMusic, pElem->FirstChildElement("IsMusic"));
        ParseValueFromXmlElem(&loops, pElem->FirstChildElement("Loops"));

        SoundInfo soundInfo(soundName);
        soundInfo.soundVolume = volume;
        soundInfo.isMusic = isMusic;
        soundInfo.loops = loops;
        pEventData.reset(new EventData_Request_Play_Sound(soundInfo));
    }
    else if (eventType == "ModifyMenuItemState")
    {
        std::string menuItemName;
        std::string menuItemState;
        ParseValueFromXmlElem(&menuItemName, pElem->FirstChildElement("MenuItemName"));
        ParseValueFromXmlElem(&menuItemState, pElem->FirstChildElement("State"));

        pEventData.reset(new EventData_Menu_Modify_Item_State(menuItemName, menuItemState));
    }
    else if (eventType == "ModifyVolume")
    {
        int deltaVolume;
        if (!ParseValueFromXmlElem(&deltaVolume, pElem->FirstChildElement("DeltaVolume")))
        {
            LOG_ERROR("No delta volume defined in ModifyVolume event !");
            return nullptr;
        }
        bool isMusic;
        if (!ParseValueFromXmlElem(&isMusic, pElem->FirstChildElement("IsMusic")))
        {
            LOG_ERROR("No sound specification (sound X music) defined in ModifyVolume event !");
            return nullptr;
        }

        pEventData.reset(new EventData_Set_Volume(deltaVolume, true, isMusic));
    }
    else if (eventType == "ResumeGame")
    {
        pEventData.reset(new EventData_IngameMenu_Resume_Game());
    }
    else if (eventType == "EndLife")
    {
        pEventData.reset(new EventData_IngameMenu_End_Life());
    }
    else if (eventType == "EndGame")
    {
        pEventData.reset(new EventData_IngameMenu_End_Game());
    }
    else
    {
        return nullptr;
    }

    return pEventData;
}

Point g_MenuScale = Point(1.0, 1.0);

//-----------------------------------------------------------------------------
//
// ScreenElementMenu implementation
//
//-----------------------------------------------------------------------------

ScreenElementMenu::ScreenElementMenu(SDL_Renderer* pRenderer)
    :
    m_pRenderer(pRenderer),
    m_bIsVisible(true),
    m_MenuType(MenuType_None)
{
    IEventMgr::Get()->VAddListener(MakeDelegate(
        this, &ScreenElementMenu::SwitchPageDelegate), EventData_Menu_SwitchPage::sk_EventType);
    IEventMgr::Get()->VAddListener(MakeDelegate(
        this, &ScreenElementMenu::ModifyMenuItemVisibilityDelegate), EventData_Menu_Modifiy_Item_Visibility::sk_EventType);
    IEventMgr::Get()->VAddListener(MakeDelegate(
        this, &ScreenElementMenu::ModifyMenuItemStateDelegate), EventData_Menu_Modify_Item_State::sk_EventType);
}

ScreenElementMenu::~ScreenElementMenu()
{
    m_MenuPageMap.clear();

    IEventMgr::Get()->VRemoveListener(MakeDelegate(
        this, &ScreenElementMenu::SwitchPageDelegate), EventData_Menu_SwitchPage::sk_EventType);
    IEventMgr::Get()->VRemoveListener(MakeDelegate(
        this, &ScreenElementMenu::ModifyMenuItemVisibilityDelegate), EventData_Menu_Modifiy_Item_Visibility::sk_EventType);
    IEventMgr::Get()->VRemoveListener(MakeDelegate(
        this, &ScreenElementMenu::ModifyMenuItemStateDelegate), EventData_Menu_Modify_Item_State::sk_EventType);

    if (m_MenuType == MenuType_IngameMenu)
    {
        IEventMgr::Get()->VRemoveListener(MakeDelegate(
            this, &ScreenElementMenu::IngameMenuResumeGameDelegate), EventData_IngameMenu_Resume_Game::sk_EventType);
        IEventMgr::Get()->VRemoveListener(MakeDelegate(
            this, &ScreenElementMenu::IngameMenuEndLifeDelegate), EventData_IngameMenu_End_Life::sk_EventType);
        IEventMgr::Get()->VRemoveListener(MakeDelegate(
            this, &ScreenElementMenu::IngameMenuEndGameDelegate), EventData_IngameMenu_End_Game::sk_EventType);
    }
}

void ScreenElementMenu::VOnUpdate(uint32 msDiff)
{
    if (m_bIsVisible && m_MenuType == MenuType_IngameMenu)
    {
        if (g_pApp && g_pApp->GetGameLogic())
        {
            g_pApp->GetGameLogic()->SetRunning(false);
        }
    }

    assert(m_pActiveMenuPage);
    m_pActiveMenuPage->VOnUpdate(msDiff);
}

void ScreenElementMenu::VOnRender(uint32 msDiff)
{
    // Menu DOES NOT use renderer scaling
    SDL_RenderSetScale(m_pRenderer, 1.0f, 1.0f);

    assert(m_pBackground != nullptr);
    assert(m_pBackground->GetTexture() != NULL);

    SDL_Rect backgroundRect = GetScreenRect();
    SDL_RenderCopy(m_pRenderer, m_pBackground->GetTexture(), &backgroundRect, NULL);

    assert(m_pActiveMenuPage);
    m_pActiveMenuPage->VOnRender(msDiff);
    RenderSelectionCoins(msDiff);

    // Restore scale
    Point scale = Point(g_pApp->GetGameConfig()->scale, g_pApp->GetGameConfig()->scale);
    SDL_RenderSetScale(m_pRenderer, (float)scale.x, (float)scale.y);
}

void ScreenElementMenu::RenderSelectionCoins(uint32 msDiff)
{
    SDL_Rect itemRect;
    if (m_SelectionCoinFrames.empty() || !m_pActiveMenuPage->GetSelectedButtonRect(itemRect))
    {
        return;
    }

    m_SelectionCoinTime += msDiff;
    const size_t frame = (m_SelectionCoinTime / m_SelectionCoinFrameTime) % m_SelectionCoinFrames.size();
    shared_ptr<Image> pCoin = m_SelectionCoinFrames[frame];

    // The frames get narrower as the coin turns; each one is centred where the full coin would be
    int coinWidth = 0;
    for (shared_ptr<Image> pFrame : m_SelectionCoinFrames)
    {
        coinWidth = pFrame->GetWidth() > coinWidth ? pFrame->GetWidth() : coinWidth;
    }
    const int gap = 6;
    const int centreY = itemRect.y + itemRect.h / 2;
    const int centresX[2] = { itemRect.x - gap - coinWidth / 2, itemRect.x + itemRect.w + gap + coinWidth / 2 };
    for (int centreX : centresX)
    {
        SDL_Rect renderRect;
        renderRect.x = (int)((centreX - pCoin->GetWidth() / 2) * g_MenuScale.x);
        renderRect.y = (int)((centreY - pCoin->GetHeight() / 2) * g_MenuScale.y);
        renderRect.w = (int)(pCoin->GetWidth() * g_MenuScale.x);
        renderRect.h = (int)(pCoin->GetHeight() * g_MenuScale.y);
        SDL_RenderCopy(m_pRenderer, pCoin->GetTexture(), NULL, &renderRect);
    }
}

// <SelectionCoins>: <Images> with %d for the frame number (from 1), <FrameCount>, <FrameTime> in ms,
// and <PaletteFrom>, the PCX whose palette the PID frames use (the menu background's)
bool ScreenElementMenu::LoadSelectionCoins(TiXmlElement* pElem)
{
    std::string imagesPattern, palettePath;
    int frameCount = 0;
    m_SelectionCoinFrameTime = 75;
    if (!ParseValueFromXmlElem(&imagesPattern, pElem->FirstChildElement("Images")) ||
        !ParseValueFromXmlElem(&palettePath, pElem->FirstChildElement("PaletteFrom")) ||
        !ParseValueFromXmlElem(&frameCount, pElem->FirstChildElement("FrameCount")))
    {
        LOG_WARNING("SelectionCoins needs Images, PaletteFrom and FrameCount");
        return false;
    }
    ParseValueFromXmlElem(&m_SelectionCoinFrameTime, pElem->FirstChildElement("FrameTime"));
    if (m_SelectionCoinFrameTime == 0)
    {
        m_SelectionCoinFrameTime = 75;
    }

    // A 256-colour PCX ends with its palette
    Resource pcxResource(palettePath);
    std::vector<char> pcxData;
    if (!g_pApp->GetResourceCache()->GetRawResource(&pcxResource, pcxData) || pcxData.size() < WAP_PALETTE_SIZE_BYTES)
    {
        LOG_WARNING("No palette for the selection coins in " + palettePath);
        return false;
    }
    std::unique_ptr<WapPal> pPalette(WAP_PalLoadFromData(&pcxData[pcxData.size() - WAP_PALETTE_SIZE_BYTES],
        WAP_PALETTE_SIZE_BYTES));
    if (!pPalette)
    {
        return false;
    }

    for (int i = 1; i <= frameCount; i++)
    {
        char framePath[256];
        snprintf(framePath, sizeof(framePath), imagesPattern.c_str(), i);
        shared_ptr<Image> pFrame = PidResourceLoader::LoadAndReturnImage(framePath, pPalette.get());
        if (!pFrame)
        {
            LOG_WARNING(std::string("Missing selection coin frame: ") + framePath);
            m_SelectionCoinFrames.clear();
            return false;
        }
        m_SelectionCoinFrames.push_back(pFrame);
    }
    return true;
}

bool ScreenElementMenu::VOnEvent(SDL_Event& evt)
{
    return m_pActiveMenuPage->VOnEvent(evt);
}

void ScreenElementMenu::VSetVisible(bool visible)
{
    if (visible == m_bIsVisible)
    {
        return;
    }

    if (visible)
    {
        if (g_pApp && g_pApp->GetGameLogic())
        {
            g_pApp->GetGameLogic()->SetRunning(false);
        }

        if (!m_MenuEnterSound.empty())
        {
            // Play some music
            SoundInfo soundInfo(m_MenuEnterSound);
            IEventMgr::Get()->VTriggerEvent(IEventDataPtr(
                new EventData_Request_Play_Sound(soundInfo)));
        }

        // Use SwitchPage event ?
        m_pActiveMenuPage = m_MenuPageMap[m_DefaultMenuPage];
        m_pActiveMenuPage->OnPageLoaded();
    }
    else
    {
        if (g_pApp && g_pApp->GetGameLogic())
        {
            g_pApp->GetGameLogic()->SetRunning(true);
        }
    }

    m_bIsVisible = visible;
}

bool ScreenElementMenu::Initialize(TiXmlElement* pElem)
{
    assert(m_pRenderer != NULL);

    std::string menuTypeStr;
    DO_AND_CHECK(ParseValueFromXmlElem(&menuTypeStr, pElem->FirstChildElement("MenuType")));
    if (menuTypeStr == "MenuType_MainMenu")
    {
        m_MenuType = MenuType_MainMenu;
    }
    else if (menuTypeStr == "MenuType_IngameMenu")
    {
        m_MenuType = MenuType_IngameMenu;
    }
    
    assert(m_MenuType != MenuType_None);

    if (m_MenuType == MenuType_IngameMenu)
    {
        IEventMgr::Get()->VAddListener(MakeDelegate(
            this, &ScreenElementMenu::IngameMenuResumeGameDelegate), EventData_IngameMenu_Resume_Game::sk_EventType);
        IEventMgr::Get()->VAddListener(MakeDelegate(
            this, &ScreenElementMenu::IngameMenuEndLifeDelegate), EventData_IngameMenu_End_Life::sk_EventType);
        IEventMgr::Get()->VAddListener(MakeDelegate(
            this, &ScreenElementMenu::IngameMenuEndGameDelegate), EventData_IngameMenu_End_Game::sk_EventType);
    }

    std::string defaultMenuPageName;
    ParseValueFromXmlElem(&defaultMenuPageName, pElem->FirstChildElement("StartingPage"));
    m_DefaultMenuPage = StringToMenuPageEnum(defaultMenuPageName);

    //
    // ---------- Background Image ----------
    //
    std::string backgroundImagePath;
    ParseValueFromXmlElem(&backgroundImagePath, pElem->FirstChildElement("BackgroundImage"));

    // This is hack for now
    if (backgroundImagePath == "IngameMenuBackground")
    {
        m_pBackground = shared_ptr<Image>(new Image(Util::CreateSDLTextureRect(640, 480, COLOR_BLACK, m_pRenderer, 127)));
    }
    else
    {
        m_pBackground = PcxResourceLoader::LoadAndReturnImage(backgroundImagePath.c_str());
    }
    assert(m_pBackground != nullptr);

    //
    // ---------- Background Music ----------
    //
    std::string backgroundMusicPath;
    ParseValueFromXmlElem(&backgroundMusicPath, pElem->FirstChildElement("BackgroundMusic"));

    if (!backgroundMusicPath.empty())
    {
        // Play some music
        SoundInfo soundInfo(backgroundMusicPath);
        soundInfo.loops = -1;
        IEventMgr::Get()->VTriggerEvent(IEventDataPtr(
            new EventData_Request_Play_Sound(soundInfo)));
    }

    m_SelectionCoinTime = 0;
    if (TiXmlElement* pCoinsElem = pElem->FirstChildElement("SelectionCoins"))
    {
        LoadSelectionCoins(pCoinsElem);
    }

    //
    // ---------- Enter Menu Sound ----------
    //
    ParseValueFromXmlElem(&m_MenuEnterSound, pElem->FirstChildElement("MenuEnterSound"));

    // Load all menu pages
    for (TiXmlElement* pMenuPageElem = pElem->FirstChildElement("MenuPage");
        pMenuPageElem != NULL;
        pMenuPageElem = pMenuPageElem->NextSiblingElement("MenuPage"))
    {
        std::string pageName;
        ParseValueFromXmlElem(&pageName, pMenuPageElem->FirstChildElement("PageName"));
        assert(!pageName.empty());
        MenuPage pageType = StringToMenuPageEnum(pageName);

        shared_ptr<ScreenElementMenuPage> pPage(new ScreenElementMenuPage(m_pRenderer));
        if (!pPage->Initialize(pMenuPageElem))
        {
            LOG_ERROR("Could not initialize menu page: " + pageName);
            return false;
        }

        m_MenuPageMap.insert(std::make_pair(pageType, pPage));
    }

    m_pActiveMenuPage = m_MenuPageMap[m_DefaultMenuPage];
    assert(m_pActiveMenuPage != nullptr);
    LOG("Menu built: " + ToStr((int)m_MenuPageMap.size()) + " pages");

    // Lets just assume that the background will take the whole screen
    // From that we can calculate current scale (can't use predefined scale here)
    Point windowSize = g_pApp->GetWindowSize();
    g_MenuScale.Set(windowSize.x / m_pBackground->GetWidth(), windowSize.y / m_pBackground->GetHeight());

    return true;
}

void ScreenElementMenu::SwitchPageDelegate(IEventDataPtr pEventData)
{
    shared_ptr<EventData_Menu_SwitchPage> pCastEventData = 
        static_pointer_cast<EventData_Menu_SwitchPage>(pEventData);

    MenuPage pageType = StringToMenuPageEnum(pCastEventData->GetNewPageName());
    auto findIt = m_MenuPageMap.find(pageType);
    if (findIt != m_MenuPageMap.end())
    {
        m_pActiveMenuPage = findIt->second;
        m_pActiveMenuPage->OnPageLoaded();
    }
    else
    {
        LOG_ERROR("Could not switch to page: " + pCastEventData->GetNewPageName() + ", Probably not implemented yet");
    }
}

void ScreenElementMenu::ModifyMenuItemVisibilityDelegate(IEventDataPtr pEventData)
{
    shared_ptr<EventData_Menu_Modifiy_Item_Visibility> pCastEventData =
        static_pointer_cast<EventData_Menu_Modifiy_Item_Visibility>(pEventData);

    std::string menuItemName = pCastEventData->GetMenuItemName();
    bool isVisible = pCastEventData->GetIsVisible();

    shared_ptr<ScreenElementMenuItem> pMenuItem = m_pActiveMenuPage->FindMenuItemByName(menuItemName);
    if (pMenuItem)
    {
        pMenuItem->VSetVisible(isVisible);
    }
    else
    {
        LOG_ERROR("Could not find menu item: " + menuItemName);
    }
}

void ScreenElementMenu::ModifyMenuItemStateDelegate(IEventDataPtr pEventData)
{
    shared_ptr<EventData_Menu_Modify_Item_State> pCastEventData =
        static_pointer_cast<EventData_Menu_Modify_Item_State>(pEventData);

    MenuItemState state = StringToMenuItemStateEnum(pCastEventData->GetMenuItemState());
    shared_ptr<ScreenElementMenuItem> pMenuItem =
        m_pActiveMenuPage->FindMenuItemByName(pCastEventData->GetMenuItemName());
    if (pMenuItem)
    {
        pMenuItem->SetState(state);
    }
    else
    {
        LOG_ERROR("Could not find menu item: " + pCastEventData->GetMenuItemName());
    }
}

void ScreenElementMenu::IngameMenuResumeGameDelegate(IEventDataPtr pEventData)
{
    VSetVisible(false);
}

void ScreenElementMenu::IngameMenuEndLifeDelegate(IEventDataPtr pEventData)
{
    VSetVisible(false);
}

void ScreenElementMenu::IngameMenuEndGameDelegate(IEventDataPtr pEventData)
{
    VSetVisible(false);
}

//-----------------------------------------------------------------------------
//
// ScreenElementMenuPage implementation
//
//-----------------------------------------------------------------------------

ScreenElementMenuPage::ScreenElementMenuPage(SDL_Renderer* pRenderer)
    :
    m_pBackground(NULL),
    m_pRenderer(pRenderer)
{

}

ScreenElementMenuPage::~ScreenElementMenuPage()
{
    m_MenuItems.clear();
}

void ScreenElementMenuPage::VOnUpdate(uint32 msDiff)
{
    for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
    {
        pMenuItem->VOnUpdate(msDiff);
    }
}

void ScreenElementMenuPage::VOnRender(uint32 msDiff)
{
    if (m_pBackground)
    {
        assert(m_pBackground->GetTexture() != NULL);

        SDL_Rect backgroundRect = GetScreenRect();
        SDL_RenderCopy(m_pRenderer, m_pBackground->GetTexture(), &backgroundRect, NULL);
    }

    for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
    {
        if (pMenuItem->VIsVisible())
        {
            pMenuItem->VOnRender(msDiff);
        }
    }
}

bool ScreenElementMenuPage::VOnEvent(SDL_Event& evt)
{
    int activeMenuItemIdx = GetActiveMenuItemIdx();
    if (m_MenuItems.size() > 0)
    {
        assert(activeMenuItemIdx >= 0);
    }

    // Cannot be switch-case since I have to check the "repeat" aswell
    if (evt.type == SDL_KEYDOWN)
    {
        if (evt.key.repeat != 0)
        {
            return false;
        }

        SDL_Keycode keyCode = SDL_GetScancodeFromKey(evt.key.keysym.sym);
        if (keyCode == SDL_SCANCODE_DOWN)
        {
            MoveToMenuItemIdx(activeMenuItemIdx, 1);
            return true;
        }
        else if (keyCode == SDL_SCANCODE_UP)
        {
            MoveToMenuItemIdx(activeMenuItemIdx, -1);
            return true;
        }
        else if (m_KeyToEventMap.find(keyCode) != m_KeyToEventMap.end())
        {
            // HACK:
            if (keyCode == SDL_SCANCODE_ESCAPE ||
                keyCode == SDL_SCANCODE_SPACE || 
                keyCode == SDL_SCANCODE_RETURN)
            {
                SoundInfo soundInfo(SOUND_MENU_SELECT_MENU_ITEM);
                IEventMgr::Get()->VTriggerEvent(IEventDataPtr(
                    new EventData_Request_Play_Sound(soundInfo)));
            }
            IEventMgr::Get()->VQueueEvent(m_KeyToEventMap[keyCode]);
            return true;
        }
        else if (keyCode == SDL_SCANCODE_SPACE ||
                 keyCode == SDL_SCANCODE_RETURN ||
                 keyCode == SDL_SCANCODE_KP_ENTER)
        {
            if (shared_ptr<ScreenElementMenuItem> pActiveMenuItem = GetActiveMenuItem())
            {
                if (!pActiveMenuItem->Press())
                {
                    LOG_WARNING("No event is assigned to button: " + pActiveMenuItem->GetName());
                }
                return true;
            }
            else
            {
                LOG_WARNING("Could not find any active menu item !");
            }
        }
    }
    else if (evt.type == SDL_MOUSEBUTTONDOWN)
    {
        if (evt.button.button == SDL_BUTTON_LEFT)
        {
            SDL_Rect clickRect;
            clickRect.x = (int)(evt.button.x / g_MenuScale.x);
            clickRect.y = (int)(evt.button.y / g_MenuScale.y);
            clickRect.w = 1;
            clickRect.h = 1;

            for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
            {
                SDL_Rect itemRect = pMenuItem->GetMenuItemRect();
                if (SDL_HasIntersection(&clickRect, &itemRect))
                {
                    if (pMenuItem->CanBeFocused())
                    {
                        DeactivateAllMenuItems();
                        pMenuItem->Press();
                        return true;
                    }
                }
            }
        }
    }

    for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
    {
        if (pMenuItem->VOnEvent(evt))
        {
            return true;
        }
    }

    return false;
}

bool ScreenElementMenuPage::Initialize(TiXmlElement* pElem)
{
    std::string pageName;
    ParseValueFromXmlElem(&pageName, pElem->FirstChildElement("PageName"));
    m_PageType = StringToMenuPageEnum(pageName);

    // This can be nullptr
    m_pBackground = LoadImageFromXmlElement(pElem->FirstChildElement("BackgroundImage"));

    // Load all menu items
    for (TiXmlElement* pMenuItemElem = pElem->FirstChildElement("MenuItem");
        pMenuItemElem != NULL;
        pMenuItemElem = pMenuItemElem->NextSiblingElement("MenuItem"))
    {
        shared_ptr<ScreenElementMenuItem> pItem(new ScreenElementMenuItem(m_pRenderer));
        if (!pItem->Initialize(pMenuItemElem))
        {
            LOG_ERROR("Could not initialize menu item");
            return false;
        }

        m_MenuItems.push_back(pItem);
    }

    // Load all key events
    for (TiXmlElement* pKeyboardEvent = pElem->FirstChildElement("KeyboardEvent");
        pKeyboardEvent != NULL;
        pKeyboardEvent = pKeyboardEvent->NextSiblingElement("KeyboardEvent"))
    {
        std::string keyStr;
        DO_AND_CHECK(ParseValueFromXmlElem(&keyStr, pKeyboardEvent->FirstChildElement("KeyType")));

        auto findIt = g_StringToSDLKeyCodeMap.find(keyStr);
        if (findIt == g_StringToSDLKeyCodeMap.end())
        {
            LOG_ERROR("Failed to find corresponding SDL key to : " + keyStr);
            return false;
        }

        IEventDataPtr pGeneratedEvent = 
            XmlElemToGeneratedEvent(pKeyboardEvent->FirstChildElement("GeneratedEvent"));
        if (pGeneratedEvent == nullptr)
        {
            LOG_ERROR("Failed to create generated event for menu keyboard key: " + keyStr);
            return false;
        }

        m_KeyToEventMap.insert(std::make_pair(findIt->second, pGeneratedEvent));
    }

    return true;
}

void ScreenElementMenuPage::DeactivateAllMenuItems()
{
    for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
    {
        if (pMenuItem->GetState() == MenuItemState_Active)
        {
            pMenuItem->SetState(MenuItemState_Inactive);
        }
    }
}

int ScreenElementMenuPage::GetActiveMenuItemIdx()
{
    int idx = 0;
    for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
    {
        if (pMenuItem->GetState() == MenuItemState_Active)
        {
            return idx;
        }
        idx++;
    }

    return -1;
}

bool ScreenElementMenuPage::GetSelectedButtonRect(SDL_Rect& outRect)
{
    for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
    {
        // Sliders' knobs light up with their button; only the button counts
        if (pMenuItem->GetState() == MenuItemState_Active && pMenuItem->CanBeFocused())
        {
            outRect = pMenuItem->GetMenuItemRect();
            return outRect.w > 0;
        }
    }
    return false;
}

shared_ptr<ScreenElementMenuItem> ScreenElementMenuPage::GetActiveMenuItem()
{
    for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
    {
        if (pMenuItem->GetState() == MenuItemState_Active)
        {
            return pMenuItem;
        }
    }

    return nullptr;
}

bool ScreenElementMenuPage::MoveToMenuItemIdx(int oldIdx, int idxIncrement, bool playSound)
{
    if (m_MenuItems.empty())
    {
        return false;
    }

    DeactivateAllMenuItems();

    // Moving up
    int buttonIdx = oldIdx + idxIncrement;
    int tryCount = 0;

    while (true)
    {
        assert(tryCount <= (int)m_MenuItems.size() && "Could not find any button to switch focus to");

        // At first button and we pressed "up" -> move to last button
        if (buttonIdx < 0 && idxIncrement < 0)
        {
            buttonIdx = m_MenuItems.size() - 1;
        }
        // At last button
        if (buttonIdx >= (int)m_MenuItems.size() && idxIncrement > 0)
        {
            buttonIdx = 0;
        }

        if (m_MenuItems[buttonIdx]->Focus())
        {
            break;
        }

        buttonIdx += idxIncrement;
        tryCount++;
    }

    if (playSound)
    {
        // Play sound
        SoundInfo soundInfo(SOUND_MENU_CHANGE_MENU_ITEM);
        IEventMgr::Get()->VTriggerEvent(IEventDataPtr(
            new EventData_Request_Play_Sound(soundInfo)));
    }

    return true;
}

void ScreenElementMenuPage::OnPageLoaded()
{
    // Focus on first active button
    MoveToMenuItemIdx(m_MenuItems.size() - 1, 1, false);
}

shared_ptr<ScreenElementMenuItem> ScreenElementMenuPage::FindMenuItemByName(std::string name)
{
    for (shared_ptr<ScreenElementMenuItem> pMenuItem : m_MenuItems)
    {
        if (pMenuItem->GetName() == name)
        {
            return pMenuItem;
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------------
//
// ScreenElementMenuItem implementation
//
//-----------------------------------------------------------------------------

ScreenElementMenuItem::ScreenElementMenuItem(SDL_Renderer* pRenderer)
    :
    m_pRenderer(pRenderer),
    m_State(MenuItemState_None),
    m_Hotkey(SDL_SCANCODE_UNKNOWN),
    m_bVisible(true)
{

}

ScreenElementMenuItem::~ScreenElementMenuItem()
{

}

void ScreenElementMenuItem::VOnUpdate(uint32 msDiff)
{
    // This is a bit hacky but implementing this via XML would be pain in the ass right now...
    if (m_Name == "SOUND_KNOB")
    {
        int soundVolume = g_pApp->GetAudio()->GetSoundVolume();
        m_Position.SetX(m_DefaultPosition.x + (soundVolume / 10) * 20);
    }
    else if (m_Name == "MUSIC_KNOB")
    {
        int musicVolume = g_pApp->GetAudio()->GetMusicVolume();
        m_Position.SetX(m_DefaultPosition.x + (musicVolume / 2) * 20);
    }
    else if (m_Name == "PLAY_AREA_KNOB")
    {
        // Left: zoomed in (smaller play area), as in the original; then the full view, then zoomed out
        int stop = BaseGameApp::FindViewZoomStop(g_pApp->GetGameConfig()->scale);
        m_Position.SetX(m_DefaultPosition.x + stop * PLAY_AREA_KNOB_TRAVEL / (BaseGameApp::VIEW_ZOOM_STOP_COUNT - 1));
    }
}

void ScreenElementMenuItem::VOnRender(uint32 msDiff)
{
    bool darken = false;
    shared_ptr<Image> pCurrImage = GetCurrentImage(&darken);
    if (pCurrImage == nullptr)
    {
        LOG_ERROR("Offending button: " + m_Name + " for state: " + ToStr(m_State));
        return;
    }

    SDL_Rect renderRect;
    renderRect.x = (int)((m_Position.x + pCurrImage->GetOffsetX()) * g_MenuScale.x);
    renderRect.y = (int)(m_Position.y * g_MenuScale.y);
    renderRect.w = (int)(pCurrImage->GetWidth() * g_MenuScale.x);
    renderRect.h = (int)(pCurrImage->GetHeight() * g_MenuScale.y);

    if (darken)
    {
        SDL_SetTextureColorMod(pCurrImage->GetTexture(), 90, 90, 90);
    }

    SDL_RenderCopy(m_pRenderer, pCurrImage->GetTexture(), NULL, &renderRect);

    if (darken)
    {
        SDL_SetTextureColorMod(pCurrImage->GetTexture(), 255, 255, 255);
    }
}

bool ScreenElementMenuItem::VOnEvent(SDL_Event& evt)
{
    if (evt.type == SDL_KEYDOWN && 
        SDL_GetScancodeFromKey(evt.key.keysym.sym) == m_Hotkey &&
        m_bVisible)
    {
        return Press();
    }

    // Does not make sense to try to process event on inactive menu item
    if (m_State != MenuItemState_Active)
    {
        return false;
    }

    if (evt.type == SDL_KEYDOWN)
    {
        auto findIt = m_KeyToEventMap.find(SDL_GetScancodeFromKey(evt.key.keysym.sym));
        if (findIt != m_KeyToEventMap.end())
        {
            for (IEventDataPtr pEvent : findIt->second)
            {
                IEventMgr::Get()->VQueueEvent(pEvent);
            }
        }
    }

    return false;
}

bool ScreenElementMenuItem::Initialize(TiXmlElement* pElem)
{
    SetPointIfDefined(&m_Position, pElem->FirstChildElement("Position"), "x", "y");
    SetPointIfDefined(&m_DefaultPosition, pElem->FirstChildElement("Position"), "x", "y");
    ParseValueFromXmlElem(&m_Name, pElem->FirstChildElement("Name"));

    std::string menuItemStateStr;
    if (!ParseValueFromXmlElem(&menuItemStateStr, pElem->FirstChildElement("State")))
    {
        LOG_ERROR("No default state for menu item: " + m_Name);
        return false;
    }
    m_State = StringToMenuItemStateEnum(menuItemStateStr);

    if (TiXmlElement* pStateConditionElem = pElem->FirstChildElement("StateCondition"))
    {
        std::string conditionForStateStr, conditionTypeStr;
        ParseValueFromXmlElem(&conditionForStateStr, pStateConditionElem->FirstChildElement("ConditionForState"));
        ParseValueFromXmlElem(&conditionTypeStr, pStateConditionElem->FirstChildElement("ConditionType"));
        if (conditionTypeStr == "CheckpointReached")
        {
            int level = -1;
            int checkpoint = -1;
            ParseValueFromXmlElem(&level, pStateConditionElem->FirstChildElement("Level"));
            ParseValueFromXmlElem(&checkpoint, pStateConditionElem->FirstChildElement("Checkpoint"));
            assert(level != -1 && checkpoint != -1);

            if (g_pApp->GetGameLogic()->GetGameSaveMgr()->HasCheckpointSave(level, checkpoint))
            {
                ParseValueFromXmlElem(&menuItemStateStr, pStateConditionElem->FirstChildElement("ConditionForState"));
                m_State = StringToMenuItemStateEnum(menuItemStateStr);
            }
        }
        else if (conditionTypeStr == "Option")
        {
            // <Option>MoviesAvailable</Option><Value>Yes</Value>: the state applies while the option has this value
            std::string option;
            std::string value;
            ParseValueFromXmlElem(&option, pStateConditionElem->FirstChildElement("Option"));
            ParseValueFromXmlElem(&value, pStateConditionElem->FirstChildElement("Value"));
            if (g_pApp->GetMenuOption(option) == value)
            {
                m_State = StringToMenuItemStateEnum(conditionForStateStr);
            }
        }
    }

    if (TiXmlElement* pVisibilityConditionElem = pElem->FirstChildElement("VisibilityCondition"))
    {
        std::string conditionTypeStr;
        ParseValueFromXmlElem(&conditionTypeStr, pVisibilityConditionElem->FirstChildElement("ConditionType"));
        
        // This element is visible only when certain condition is met
        // so it is safe to assume it is not visible by default
        m_bVisible = false;

        if (conditionTypeStr == "SoundOn")
        {
            if (g_pApp->GetAudio()->IsSoundActive())
            {
                m_bVisible = true;
            }
        }
        else if (conditionTypeStr == "SoundOff")
        {
            if (!g_pApp->GetAudio()->IsSoundActive())
            {
                m_bVisible = true;
            }
        }
        else if (conditionTypeStr == "MusicOn")
        {
            if (g_pApp->GetAudio()->IsMusicActive())
            {
                m_bVisible = true;
            }
        }
        else if (conditionTypeStr == "MusicOff")
        {
            if (!g_pApp->GetAudio()->IsMusicActive())
            {
                m_bVisible = true;
            }
        }
        // TODO: Implement these ones
        else if (conditionTypeStr == "VoiceOn")
        {
            m_bVisible = true;
        }
        else if (conditionTypeStr == "VoiceOn")
        {
            m_bVisible = false;
        }
        else if (conditionTypeStr == "AmbientOn")
        {
            m_bVisible = true;
        }
        else if (conditionTypeStr == "AmbientOff")
        {
            m_bVisible = false;
        }
        else if (conditionTypeStr == "Option")
        {
            // <Option>Difficulty</Option><Value>Easy</Value>: visible while the option has this value
            std::string option;
            std::string value;
            ParseValueFromXmlElem(&option, pVisibilityConditionElem->FirstChildElement("Option"));
            ParseValueFromXmlElem(&value, pVisibilityConditionElem->FirstChildElement("Value"));
            m_bVisible = (g_pApp->GetMenuOption(option) == value);
        }
    }
    std::string menuItemTypeStr;
    ParseValueFromXmlElem(&menuItemTypeStr, pElem->FirstChildElement("Type"));
    m_Type = StringToMenuItemType(menuItemTypeStr);

    ParseValueFromXmlElem(&m_bVisible, pElem->FirstChildElement("IsVisible"));

    if (shared_ptr<Image> pImage = LoadImageFromXmlElement(pElem->FirstChildElement("DisabledImage")))
    {
        m_Images.insert(std::make_pair(MenuItemState_Disabled, pImage));
    }
    if (shared_ptr<Image> pImage = LoadImageFromXmlElement(pElem->FirstChildElement("InactiveImage")))
    {
        m_Images.insert(std::make_pair(MenuItemState_Inactive, pImage));
    }
    if (shared_ptr<Image> pImage = LoadImageFromXmlElement(pElem->FirstChildElement("ActiveImage")))
    {
        m_Images.insert(std::make_pair(MenuItemState_Active, pImage));
    }

    // <Label size="22">Text</Label> instead of the images: drawn with the menu font
    if (TiXmlElement* pLabelElem = pElem->FirstChildElement("Label"))
    {
        int fontSize = 22;
        pLabelElem->Attribute("size", &fontSize);
        std::string label = pLabelElem->GetText() ? pLabelElem->GetText() : "";
        shared_ptr<Image> pInactive = CreateLabelImage(label, fontSize, false);
        shared_ptr<Image> pActive = CreateLabelImage(label, fontSize, true);
        if (pInactive == nullptr || pActive == nullptr)
        {
            return false;
        }
        m_Images[MenuItemState_Inactive] = pInactive;
        m_Images[MenuItemState_Active] = pActive;
    }

    // <Position x="center">: centred horizontally on the menu
    if (TiXmlElement* pPositionElem = pElem->FirstChildElement("Position"))
    {
        const char* x = pPositionElem->Attribute("x");
        auto inactiveIt = m_Images.find(MenuItemState_Inactive);
        if (x != NULL && std::string(x) == "center" && inactiveIt != m_Images.end())
        {
            double centeredX = (MENU_LAYOUT_WIDTH - inactiveIt->second->GetWidth()) / 2;
            m_Position.SetX(centeredX);
            m_DefaultPosition.SetX(centeredX);
        }
    }

    /*for (TiXmlElement* pMenuItemImageElem = pElem->FirstChildElement("MenuItemImage");
        pMenuItemImageElem != NULL;
        pMenuItemImageElem = pMenuItemImageElem->NextSiblingElement("MenuItemImage"))
    {
        shared_ptr<MenuItemImage> pMenuItemImage(new MenuItemImage);
        pMenuItemImage->pImage = LoadImageFromXmlElement(pMenuItemImageElem->FirstChildElement("Image"));
        ParseValueFromXmlElem(&pMenuItemImage->imageTag, pMenuItemImageElem->FirstChildElement("ImageTag"));

        std::string stateStr;
        pMenuItemImage->currentState = StringToMenuItemStateEnum(stateStr);

        if (pMenuItemImage->pImage == nullptr)
        {
            LOG_ERROR("Failed to load image with tag: " + pMenuItemImage->imageTag);
            return false;
        }

        m_MenuItemImageList.push_back(pMenuItemImage);
    }*/

    std::string hotkeyStr;
    if (ParseValueFromXmlElem(&hotkeyStr, pElem->FirstChildElement("Hotkey")))
    {
        m_Hotkey = StringToSDLKeycode(hotkeyStr);
    }

    for (TiXmlElement* pGeneratedEventElem = pElem->FirstChildElement("GeneratedEvent");
         pGeneratedEventElem != NULL;
         pGeneratedEventElem = pGeneratedEventElem->NextSiblingElement("GeneratedEvent"))
    {
        IEventDataPtr pEvent = XmlElemToGeneratedEvent(pGeneratedEventElem);
        if (pEvent == nullptr)
        {
            LOG_ERROR("Failed to create generated event for menu item: " + m_Name);
            return false;
        }
        m_GeneratedEventList.push_back(pEvent);
    }

    if (TiXmlElement* pStateEnterEventsElem = pElem->FirstChildElement("StateEnterEvents"))
    {
        for (TiXmlElement* pEventElem = pStateEnterEventsElem->FirstChildElement("Event");
            pEventElem != NULL;
            pEventElem = pEventElem->NextSiblingElement("Event"))
        {
            std::string forStateStr;
            if (!ParseValueFromXmlElem(&forStateStr, pEventElem->FirstChildElement("ForState")))
            {
                LOG_ERROR("StateEnterEvent is missing state element !");
                return false;
            }
            MenuItemState forState = StringToMenuItemStateEnum(forStateStr);

            IEventDataPtr pEvent = XmlElemToGeneratedEvent(pEventElem);
            if (pEvent == nullptr)
            {
                LOG_ERROR("Failed to create generated event for menu item: " + m_Name);
                return false;
            }
            
            m_StateEnterEventMap[forState].push_back(pEvent);
        }
    }

    if (TiXmlElement* pStateEnterEventsElem = pElem->FirstChildElement("StateLeaveEvents"))
    {
        for (TiXmlElement* pEventElem = pStateEnterEventsElem->FirstChildElement("Event");
            pEventElem != NULL;
            pEventElem = pEventElem->NextSiblingElement("Event"))
        {
            std::string forStateStr;
            if (!ParseValueFromXmlElem(&forStateStr, pEventElem->FirstChildElement("ForState")))
            {
                LOG_ERROR("StateEnterEvent is missing state element !");
                return false;
            }
            MenuItemState forState = StringToMenuItemStateEnum(forStateStr);

            IEventDataPtr pEvent = XmlElemToGeneratedEvent(pEventElem);
            if (pEvent == nullptr)
            {
                LOG_ERROR("Failed to create generated event for menu item: " + m_Name);
                return false;
            }

            m_StateLeaveEventMap[forState].push_back(pEvent);
        }
    }

    if (TiXmlElement* pKeyAllKeyEventsElem = pElem->FirstChildElement("KeyEvents"))
    {
        for (TiXmlElement* pKeyEventElem = pKeyAllKeyEventsElem->FirstChildElement("KeyEvent");
            pKeyEventElem != NULL;
            pKeyEventElem = pKeyEventElem->NextSiblingElement("KeyEvent"))
        {
            std::string keyStr;
            if (!ParseValueFromXmlElem(&keyStr, pKeyEventElem->FirstChildElement("Key")))
            {
                LOG_ERROR("KeyEvent has no Key defined !");
                return false;
            }
            SDL_Scancode keyCode = StringToSDLKeycode(keyStr);

            for (TiXmlElement* pEventElem = pKeyEventElem->FirstChildElement("Event");
                pEventElem != NULL;
                pEventElem = pEventElem->NextSiblingElement("Event"))
            {
                IEventDataPtr pEvent = XmlElemToGeneratedEvent(pEventElem);
                if (pEvent == nullptr)
                {
                    LOG_ERROR("Failed to create KeyEvent for menu item: " + m_Name);
                    return false;
                }

                m_KeyToEventMap[keyCode].push_back(pEvent);
            }
        }
    }

    if (m_Images.empty())
    {
        LOG_ERROR("No images for menu item: " + m_Name);
        return false;
    }

    return true;
}

bool ScreenElementMenuItem::Press()
{
    if (CanBeFocused())
    {
        if (m_GeneratedEventList.empty())
        {
            LOG_WARNING("No press events for item: " + m_Name);
        }

        Focus();

        for (IEventDataPtr pEvent : m_GeneratedEventList)
        {
            IEventMgr::Get()->VQueueEvent(pEvent);
        }
        
        SoundInfo soundInfo(SOUND_MENU_SELECT_MENU_ITEM);
        IEventMgr::Get()->VTriggerEvent(IEventDataPtr(
            new EventData_Request_Play_Sound(soundInfo)));

        return true;
    }

    return false;
}

bool ScreenElementMenuItem::SetState(MenuItemState state)
{
    if (m_Images.find(state) != m_Images.end())
    {
        if (state != m_State)
        {
            OnStateChanged(state, m_State);
        }

        m_State = state;
        return true;
    }

    return false;
}

bool ScreenElementMenuItem::CanBeFocused()
{
    if (!m_bVisible)
    {
        return false;
    }

    // Only buttons at this time can be focued
    if (m_Type != MenuItemType_Button)
    {
        return false;
    }

    return (m_Images.find(MenuItemState_Active) != m_Images.end() &&
            m_State != MenuItemState_Disabled);
}

bool ScreenElementMenuItem::Focus()
{
    if (!CanBeFocused())
    {
        return false;
    }

    SetState(MenuItemState_Active);
    return true;
}

void ScreenElementMenuItem::OnStateChanged(MenuItemState newState, MenuItemState oldState)
{
    auto stateEnterFindIt = m_StateEnterEventMap.find(newState);
    if (stateEnterFindIt != m_StateEnterEventMap.end())
    {
        for (IEventDataPtr pEvent : stateEnterFindIt->second)
        {
            IEventMgr::Get()->VQueueEvent(pEvent);
        }
    }

    auto stateLeaveFindIt = m_StateLeaveEventMap.find(oldState);
    if (stateLeaveFindIt != m_StateLeaveEventMap.end())
    {
        for (IEventDataPtr pEvent : stateLeaveFindIt->second)
        {
            IEventMgr::Get()->VQueueEvent(pEvent);
        }
    }
}

shared_ptr<Image> ScreenElementMenuItem::GetCurrentImage(bool* outDarken) const
{
    if (outDarken)
    {
        *outDarken = false;
    }

    auto findIt = m_Images.find(m_State);
    if (findIt != m_Images.end() && findIt->second)
    {
        return findIt->second;
    }

    // Most buttons have no greyed-out image in the game data, so a disabled one is drawn as its
    // inactive image, darkened
    if (m_State == MenuItemState_Disabled)
    {
        findIt = m_Images.find(MenuItemState_Inactive);
        if (findIt != m_Images.end() && findIt->second)
        {
            if (outDarken)
            {
                *outDarken = true;
            }
            return findIt->second;
        }
    }
    return nullptr;
}

SDL_Rect ScreenElementMenuItem::GetMenuItemRect()
{
    // An empty rect never matches a tap
    SDL_Rect itemRect = { (int)m_Position.x, (int)m_Position.y, 0, 0 };

    // Taps on a page with a greyed-out item crashed here before (0.4.0, 0.5.0): its state had no image
    shared_ptr<Image> pImage = GetCurrentImage();
    if (pImage)
    {
        SDL_QueryTexture(pImage->GetTexture(), NULL, NULL, &itemRect.w, &itemRect.h);
    }
    return itemRect;
}