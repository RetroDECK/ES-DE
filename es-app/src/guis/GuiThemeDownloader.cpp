//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiThemeDownloader.cpp
//
//  Theme downloader.
//  Currently only a skeleton with some JSON configuration parsing.
//

#include "guis/GuiThemeDownloader.h"

#include "resources/ResourceManager.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

GuiThemeDownloader::GuiThemeDownloader()
    : mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {3, 2}}
{
    addChild(&mBackground);
    addChild(&mGrid);

    // Set up grid.
    mTitle = std::make_shared<TextComponent>("THEME DOWNLOADER", Font::get(FONT_SIZE_LARGE),
                                             0x555555FF, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {1, 0}, false, true, glm::ivec2 {1, 1});

    float width {mRenderer->getScreenWidth() * 0.85f};
    float height {mRenderer->getScreenHeight() * 0.70f};

    setSize(width, height);
    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    parseThemesList();
}

void GuiThemeDownloader::parseThemesList()
{
    // Obviously a temporary location for testing purposes...
    const std::string themesFile {Utils::FileSystem::getHomePath() +
                                  "/.emulationstation/themes.json"};

    if (!Utils::FileSystem::exists(themesFile)) {
        LOG(LogInfo) << "GuiThemeDownloader: No themes.json file found";
        return;
    }

    const ResourceData& themesFileData {ResourceManager::getInstance().getFileData(themesFile)};
    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<const char*>(themesFileData.ptr.get()), themesFileData.length);

    if (doc.HasParseError()) {
        LOG(LogWarning) << "GuiThemeDownloader: Couldn't parse the themes.json file";
        return;
    }

    std::vector<std::string> themeObjects {"themeSets", "legacyThemeSets"};

    for (auto& themeObject : themeObjects) {
        if (doc.HasMember(themeObject.c_str()) && doc[themeObject.c_str()].IsArray()) {
            const rapidjson::Value& themeSets {doc[themeObject.c_str()]};
            for (int i {0}; i < static_cast<int>(themeSets.Size()); ++i) {
                ThemeEntry themeEntry;
                const rapidjson::Value& theme {themeSets[i]};

                if (theme.HasMember("name") && theme["name"].IsString())
                    themeEntry.name = theme["name"].GetString();

                if (theme.HasMember("reponame") && theme["reponame"].IsString())
                    themeEntry.reponame = theme["reponame"].GetString();

                if (theme.HasMember("url") && theme["url"].IsString())
                    themeEntry.url = theme["url"].GetString();

                if (theme.HasMember("bundled") && theme["bundled"].IsBool())
                    themeEntry.bundled = theme["bundled"].GetBool();

                if (theme.HasMember("variants") && theme["variants"].IsArray()) {
                    const rapidjson::Value& variants {theme["variants"]};
                    for (int i {0}; i < static_cast<int>(variants.Size()); ++i)
                        themeEntry.variants.emplace_back(variants[i].GetString());
                }

                if (theme.HasMember("colorSchemes") && theme["colorSchemes"].IsArray()) {
                    const rapidjson::Value& colorSchemes {theme["colorSchemes"]};
                    for (int i {0}; i < static_cast<int>(colorSchemes.Size()); ++i)
                        themeEntry.colorSchemes.emplace_back(colorSchemes[i].GetString());
                }

                if (theme.HasMember("aspectRatios") && theme["aspectRatios"].IsArray()) {
                    const rapidjson::Value& aspectRatios {theme["aspectRatios"]};
                    for (int i {0}; i < static_cast<int>(aspectRatios.Size()); ++i)
                        themeEntry.aspectRatios.emplace_back(aspectRatios[i].GetString());
                }

                if (theme.HasMember("transitions") && theme["transitions"].IsArray()) {
                    const rapidjson::Value& transitions {theme["transitions"]};
                    for (int i {0}; i < static_cast<int>(transitions.Size()); ++i)
                        themeEntry.transitions.emplace_back(transitions[i].GetString());
                }

                if (theme.HasMember("screenshots") && theme["screenshots"].IsArray()) {
                    const rapidjson::Value& screenshots {theme["screenshots"]};
                    for (int i {0}; i < static_cast<int>(screenshots.Size()); ++i) {
                        Screenshot screenshotEntry;
                        if (screenshots[i].HasMember("image") && screenshots[i]["image"].IsString())
                            screenshotEntry.image = screenshots[i]["image"].GetString();

                        if (screenshots[i].HasMember("caption") &&
                            screenshots[i]["caption"].IsString())
                            screenshotEntry.caption = screenshots[i]["caption"].GetString();

                        if (screenshotEntry.image != "" && screenshotEntry.caption != "")
                            themeEntry.screenshots.emplace_back(screenshotEntry);
                    }
                }

                if (themeObject == "themeSets")
                    mThemeSets.emplace_back(themeEntry);
                else
                    mLegacyThemeSets.emplace_back(themeEntry);
            }
        }
    }

    LOG(LogInfo) << "GuiThemeDownloader: Parsed " << mThemeSets.size() << " theme sets and "
                 << mLegacyThemeSets.size() << " legacy theme sets";
}

void GuiThemeDownloader::onSizeChanged()
{
    const float screenSize {mRenderer->getIsVerticalOrientation() ? mRenderer->getScreenWidth() :
                                                                    mRenderer->getScreenHeight()};
    mGrid.setRowHeightPerc(0, (mTitle->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y /
                                  2.0f);

    mGrid.setColWidthPerc(0, 0.04f);
    mGrid.setColWidthPerc(2, 0.04f);

    mGrid.setSize(mSize);
    mBackground.fitTo(mSize, glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec2 {-32.0f, -32.0f});
}

bool GuiThemeDownloader::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("b", input) && input.value) {
        delete this;
        return true;
    }

    return true;
    // return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiThemeDownloader::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    //    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};
    prompts.push_back(HelpPrompt("b", "back (cancel)"));

    return prompts;
}
