//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiApplicationUpdater.cpp
//
//  Installs application updates.
//  Used in conjunction with ApplicationUpdater.
//

#include "guis/GuiApplicationUpdater.h"

#include "EmulationStation.h"
#include "guis/GuiTextEditKeyboardPopup.h"
#include "utils/PlatformUtil.h"

#include <filesystem>

GuiApplicationUpdater::GuiApplicationUpdater()
    : mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {4, 11}}
    , mLinuxAppImage {false}
    , mAbortDownload {false}
    , mDownloading {false}
    , mReadyToInstall {false}
    , mHasDownloaded {false}
    , mInstalling {false}
    , mHasInstalled {false}
{
    addChild(&mBackground);
    addChild(&mGrid);

    LOG(LogInfo) << "Starting Application Updater";

    mPackage = ApplicationUpdater::getInstance().getPackageInfo();
    mLinuxAppImage =
        (mPackage.name == "LinuxAppImage" || mPackage.name == "LinuxSteamDeckAppImage");
    setDownloadPath();

    // Set up grid.
    mTitle = std::make_shared<TextComponent>("APPLICATION UPDATER", Font::get(FONT_SIZE_LARGE),
                                             mMenuColorTitle, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true, glm::ivec2 {4, 1},
                   GridFlags::BORDER_BOTTOM);

    mStatusHeader = std::make_shared<TextComponent>(
        "INSTALLATION STEPS:", Font::get(FONT_SIZE_MINI), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mStatusHeader, glm::ivec2 {1, 1}, false, true, glm::ivec2 {2, 1});

    const std::string step1Text {mLinuxAppImage ? "DOWNLOAD NEW RELEASE" :
                                                  "DOWNLOAD NEW RELEASE TO THIS DIRECTORY:"};
    mProcessStep1 = std::make_shared<TextComponent>(step1Text, Font::get(FONT_SIZE_MEDIUM),
                                                    mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mProcessStep1, glm::ivec2 {1, 2}, false, true, glm::ivec2 {2, 1});

#if defined(_WIN64)
    const std::string step2Text {Utils::String::replace(
        Utils::FileSystem::getParent(mDownloadPackageFilename), , "/", "\\")};
#else
    const std::string step2Text {mLinuxAppImage ?
                                     "INSTALL PACKAGE" :
                                     Utils::FileSystem::getParent(mDownloadPackageFilename)};
#endif
    mProcessStep2 = std::make_shared<TextComponent>(step2Text, Font::get(FONT_SIZE_MEDIUM),
                                                    mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mProcessStep2, glm::ivec2 {1, 3}, false, true, glm::ivec2 {2, 1});

    const std::string step3Text {mLinuxAppImage ? "QUIT AND MANUALLY RESTART ES-DE" :
                                                  "QUIT AND MANUALLY UPGRADE ES-DE"};
    mProcessStep3 = std::make_shared<TextComponent>(step3Text, Font::get(FONT_SIZE_MEDIUM),
                                                    mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mProcessStep3, glm::ivec2 {1, 4}, false, true, glm::ivec2 {2, 1});

    mStatusMessageHeader = std::make_shared<TextComponent>(
        "STATUS MESSAGE:", Font::get(FONT_SIZE_MINI), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mStatusMessageHeader, glm::ivec2 {1, 6}, false, true, glm::ivec2 {2, 1});

    mStatusMessage = std::make_shared<TextComponent>("", Font::get(FONT_SIZE_SMALL),
                                                     mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mStatusMessage, glm::ivec2 {1, 7}, false, true, glm::ivec2 {2, 1});

    mChangelogMessage = std::make_shared<TextComponent>("", Font::get(FONT_SIZE_SMALL),
                                                        mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mChangelogMessage, glm::ivec2 {1, 8}, false, true, glm::ivec2 {2, 1});

    // Buttons.
    std::vector<std::shared_ptr<ButtonComponent>> buttons;

    mButton1 = std::make_shared<ButtonComponent>("DOWNLOAD", "download new release", [this]() {
        if (!mDownloading) {
            if (!mLinuxAppImage) {
                if (!Utils::FileSystem::exists(
                        Utils::FileSystem::getParent(mDownloadPackageFilename))) {
                    mMessage = "Download directory does not exist";
                    return;
                }
            }
            mMessage = "";
            mStatusMessage->setText(mMessage);
            mDownloading = true;
            if (mThread) {
                mThread->join();
                mThread.reset();
            }
            mThread = std::make_unique<std::thread>(&GuiApplicationUpdater::downloadPackage, this);
        }
    });

    buttons.push_back(mButton1);

    if (!mLinuxAppImage) {
        mButton2 = std::make_shared<ButtonComponent>(
            "CHANGE DIRECTORY", "change download directory", [this]() {
                if (mDownloading || mHasDownloaded)
                    return;
                std::string currentDownloadDirectory {
                    Utils::FileSystem::getParent(mDownloadPackageFilename)};
                mWindow->pushGui(new GuiTextEditKeyboardPopup(
                    getHelpStyle(), 0.0f, "ENTER DOWNLOAD DIRECTORY", currentDownloadDirectory,
                    [this, currentDownloadDirectory](std::string newDownloadDirectory) {
                        if (currentDownloadDirectory != newDownloadDirectory) {
                            newDownloadDirectory.erase(
                                // Remove trailing / and \ characters.
                                std::find_if(newDownloadDirectory.rbegin(),
                                             newDownloadDirectory.rend(),
                                             [](char c) { return c != '/' && c != '\\'; })
                                    .base(),
                                newDownloadDirectory.end());
#if defined(_WIN64)
                            newDownloadDirectory =
                                Utils::String::replace(newDownloadDirectory, "/", "\\");
#else
                    newDownloadDirectory = Utils::String::replace(newDownloadDirectory, "\\", "/");
#endif
                            Settings::getInstance()->setString(
                                "ApplicationUpdaterDownloadDirectory",
                                Utils::String::trim(newDownloadDirectory));
                            Settings::getInstance()->saveFile();
                            setDownloadPath();
                            mProcessStep2->setValue(
                                Utils::FileSystem::getParent(mDownloadPackageFilename));
                        }
                    },
                    false));
            });
        buttons.push_back(mButton2);
    }

    mButton3 = std::make_shared<ButtonComponent>("CANCEL", "cancel", [this]() {
        mAbortDownload = true;
        if (mDownloading) {
            mWindow->pushGui(
                new GuiMsgBox(getHelpStyle(), "DOWNLOAD ABORTED\nNO PACKAGE SAVED TO DISK", "OK",
                              nullptr, "", nullptr, "", nullptr, true, true,
                              (mRenderer->getIsVerticalOrientation() ?
                                   0.70f :
                                   0.45f * (1.778f / mRenderer->getScreenAspectRatio()))));
        }
        else if (mHasDownloaded && !mHasInstalled) {
            mWindow->pushGui(new GuiMsgBox(
                getHelpStyle(), "PACKAGE WAS DOWNLOADED AND\nCAN BE MANUALLY INSTALLED", "OK",
                nullptr, "", nullptr, "", nullptr, true, true,
                (mRenderer->getIsVerticalOrientation() ?
                     0.70f :
                     0.45f * (1.778f / mRenderer->getScreenAspectRatio()))));
        }
        delete this;
    });

    buttons.push_back(mButton3);

    mButtons = MenuComponent::makeButtonGrid(buttons);
    mGrid.setEntry(mButtons, glm::ivec2 {0, 10}, true, false, glm::ivec2 {4, 1},
                   GridFlags::BORDER_TOP);

    // Limit the width of the GUI on ultrawide monitors. The 1.778 aspect ratio value is
    // the 16:9 reference.
    const float aspectValue {1.778f / Renderer::getScreenAspectRatio()};
    const float width {glm::clamp(0.70f * aspectValue, 0.55f,
                                  (mRenderer->getIsVerticalOrientation() ? 0.95f : 0.85f)) *
                       mRenderer->getScreenWidth()};
    setSize(width,
            mTitle->getSize().y +
                (FONT_SIZE_MEDIUM * 1.5f * (mRenderer->getIsVerticalOrientation() ? 8.0f : 7.0f)) +
                mButtons->getSize().y);

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                std::round(mRenderer->getScreenHeight() * 0.13f));

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText("DOWNLOADING");
    mBusyAnim.onSizeChanged();
}

GuiApplicationUpdater::~GuiApplicationUpdater()
{
    mAbortDownload = true;

    if (mThread)
        mThread->join();
}

void GuiApplicationUpdater::setDownloadPath()
{
    if (mLinuxAppImage) {
        mDownloadPackageFilename = Utils::FileSystem::getParent(Utils::FileSystem::getEsBinary()) +
                                   "/" + mPackage.filename + "_" + mPackage.version;
    }
    else {
#if defined(_WIN64)
        const std::string downloadDirectory {Utils::String::replace(
            Settings::getInstance()->getString("ApplicationUpdaterDownloadDirectory"), "\\", "/")};
#else
        const std::string downloadDirectory {
            Settings::getInstance()->getString("ApplicationUpdaterDownloadDirectory")};
#endif
        if (downloadDirectory == "")
            mDownloadPackageFilename = Utils::FileSystem::getSystemHomeDirectory() + "/Downloads/";
        else
            mDownloadPackageFilename = Utils::FileSystem::expandHomePath(downloadDirectory) + "/";

        mDownloadPackageFilename = Utils::String::replace(mDownloadPackageFilename, "//", "/");
        mDownloadPackageFilename.append(mPackage.filename);
    }
}

bool GuiApplicationUpdater::downloadPackage()
{
    mStatus = ASYNC_IN_PROGRESS;
    mRequest = std::unique_ptr<HttpReq>(std::make_unique<HttpReq>(mPackage.url));
    LOG(LogDebug) << "GuiApplicationUpdater::downloadPackage(): Starting download of \""
                  << mPackage.filename << "\"";

    while (!mAbortDownload) {
        HttpReq::Status reqStatus {mRequest->status()};
        if (reqStatus == HttpReq::REQ_SUCCESS) {
            mStatus = ASYNC_DONE;
            break;
        }
        else if (reqStatus != HttpReq::REQ_IN_PROGRESS) {
            std::string errorMessage {"Network error (status: "};
            errorMessage.append(std::to_string(reqStatus))
                .append(") - ")
                .append(mRequest->getErrorMsg());
            mRequest.reset();
            std::unique_lock<std::mutex> lock {mMutex};
            mMessage = errorMessage;
            return true;
        }
    }

    if (mAbortDownload) {
        if (mAbortDownload) {
            LOG(LogInfo) << "Aborted package download";
        }
        mRequest.reset();
        return false;
    }

    std::string fileContents {mRequest->getContent()};
    mRequest.reset();

    if (Utils::Math::md5Hash(fileContents) != mPackage.md5) {
        const std::string errorMessage {"Downloaded file does not match expected MD5 checksum"};
        LOG(LogError) << errorMessage;
        std::unique_lock<std::mutex> lock {mMutex};
        mMessage = "Error: " + errorMessage;
        return true;
    }

    if (mLinuxAppImage) {
        LOG(LogDebug)
            << "GuiApplicationUpdater::downloadPackage(): Package downloaded, writing it to \""
            << mDownloadPackageFilename << "\"";

        if (Utils::FileSystem::isRegularFile(mDownloadPackageFilename)) {
            LOG(LogInfo) << "Temporary package file already exists, deleting it";
            Utils::FileSystem::removeFile(mDownloadPackageFilename);
            if (Utils::FileSystem::exists(mDownloadPackageFilename)) {
                const std::string errorMessage {
                    "Couldn't delete temporary package file, permission problems?"};
                LOG(LogError) << errorMessage;
                std::unique_lock<std::mutex> lock {mMutex};
                mMessage = "Error: " + errorMessage;
                return true;
            }
        }
    }

    std::ofstream writeFile;
    writeFile.open(mDownloadPackageFilename.c_str(), std::ofstream::binary);

    if (writeFile.fail()) {
        const std::string errorMessage {"Couldn't write package file, permission problems?"};
        LOG(LogError) << errorMessage;
        std::unique_lock<std::mutex> lock {mMutex};
        mMessage = "Error: " + errorMessage;
        return true;
    }

    writeFile.write(&fileContents[0], fileContents.length());
    writeFile.close();

    fileContents.clear();

    if (mLinuxAppImage) {
        std::filesystem::permissions(
            mDownloadPackageFilename,
            std::filesystem::perms::owner_all | std::filesystem::perms::group_all |
                std::filesystem::perms::others_read | std::filesystem::perms::others_exec);

        if (std::filesystem::status(mDownloadPackageFilename).permissions() !=
            (std::filesystem::perms::owner_all | std::filesystem::perms::group_all |
             std::filesystem::perms::others_read | std::filesystem::perms::others_exec)) {
            Utils::FileSystem::removeFile(mDownloadPackageFilename);
            const std::string errorMessage {"Couldn't set permissions on AppImage file"};
            LOG(LogError) << errorMessage;
            std::unique_lock<std::mutex> lock {mMutex};
            mMessage = "Error: " + errorMessage;
            return true;
        }
    }

    LOG(LogInfo) << "Successfully downloaded package file \"" << mDownloadPackageFilename << "\"";

    std::unique_lock<std::mutex> lock {mMutex};
    mMessage = "Downloaded " + Utils::FileSystem::getFileName(mDownloadPackageFilename);

    mDownloading = false;
    mReadyToInstall = true;

    return false;
}

bool GuiApplicationUpdater::installAppImage()
{
    LOG(LogDebug) << "GuiApplicationUpdater::installAppImage(): Attempting to install new package";

    mReadyToInstall = false;
    mInstalling = true;

    const std::string packageTargetFile {Utils::FileSystem::getEsBinary()};

    if (packageTargetFile !=
        Utils::FileSystem::getParent(Utils::FileSystem::getEsBinary()) + "/" + mPackage.filename) {
        LOG(LogWarning) << "Running AppImage seems to have a non-standard filename: \""
                        << packageTargetFile << "\"";
    }

    if (Utils::FileSystem::isSymlink(packageTargetFile)) {
        LOG(LogInfo)
            << "Target file is a symbolic link, this will be followed and the actual symlink file "
            << "will not be touched ";
    }

    // Extra precaution, make sure that the file was actually correctly written to disk.
    std::ifstream readFile;
    readFile.open(mDownloadPackageFilename.c_str(), std::ofstream::binary);

    if (readFile.fail()) {
        const std::string errorMessage {"Couldn't open AppImage update file for reading"};
        LOG(LogError) << errorMessage;
        mMessage = "Error: " + errorMessage;
        mHasDownloaded = false;
        return true;
    }

    readFile.seekg(0, std::ios::end);
    const long fileLength {static_cast<long>(readFile.tellg())};
    readFile.seekg(0, std::ios::beg);
    std::string fileData(fileLength, 0);
    readFile.read(&fileData[0], fileLength);
    readFile.close();

    if (Utils::Math::md5Hash(fileData) != mPackage.md5) {
        const std::string errorMessage {"Downloaded file does not match expected MD5 checksum"};
        LOG(LogError) << errorMessage;
        mMessage = "Error: " + errorMessage;
        mHasDownloaded = false;
        return true;
    }

    const std::string packageOldFile {packageTargetFile + "_" + PROGRAM_VERSION_STRING + ".OLD"};

    if (Utils::FileSystem::renameFile(packageTargetFile, packageOldFile, true)) {
        const std::string errorMessage {
            "Couldn't rename running AppImage file, permission problems?"};
        LOG(LogError) << errorMessage;
        mMessage = "Error: " + errorMessage;
        LOG(LogInfo) << "Attempting to rename \"" << packageOldFile
                     << "\" back to running AppImage";
        Utils::FileSystem::renameFile(packageOldFile, packageTargetFile, true);
        mInstalling = false;
        return true;
    }

    LOG(LogInfo) << "Renamed running AppImage to \"" << packageOldFile << "\"";

    if (Utils::FileSystem::renameFile(mDownloadPackageFilename, packageTargetFile, true)) {
        const std::string errorMessage {
            "Couldn't replace running AppImage file, permission problems?"};
        LOG(LogError) << errorMessage;
        mMessage = "Error: " + errorMessage;
        LOG(LogInfo) << "Attempting to rename \"" << packageOldFile
                     << "\" back to running AppImage";
        Utils::FileSystem::renameFile(packageOldFile, packageTargetFile, true);
        mInstalling = false;
        return true;
    }

    LOG(LogInfo) << "Package was successfully installed as \"" << packageTargetFile << "\"";

    std::unique_lock<std::mutex> lock {mMutex};
    mMessage = "Successfully installed as " + Utils::FileSystem::getFileName(packageTargetFile);
    mHasInstalled = true;

    return false;
}

void GuiApplicationUpdater::update(int deltaTime)
{
    {
        std::unique_lock<std::mutex> lock {mMutex};
        if (mMessage != "") {
            mStatusMessage->setText(mMessage);
            mDownloading = false;
        }
    }

    if (mDownloading)
        mBusyAnim.update(deltaTime);
    else if (mLinuxAppImage && mReadyToInstall) {
        mProcessStep1->setText(ViewController::TICKMARK_CHAR + " " + mProcessStep1->getValue());
        mProcessStep1->setColor(mMenuColorGreen);
        mButton1->setText("INSTALL", "install package");
        mButton1->setPressedFunc([this] {
            if (!mInstalling) {
                mMessage = "";
                mStatusMessage->setText(mMessage);
                installAppImage();
            }
        });
        mReadyToInstall = false;
        mHasDownloaded = true;
    }
    else if ((mLinuxAppImage && mHasInstalled) || (!mLinuxAppImage && mReadyToInstall)) {
        if (mLinuxAppImage) {
            mProcessStep2->setText(ViewController::TICKMARK_CHAR + " " + mProcessStep2->getValue());
            mProcessStep2->setColor(mMenuColorGreen);
        }
        else {
            mProcessStep1->setText(ViewController::TICKMARK_CHAR + " " + mProcessStep1->getValue());
            mProcessStep1->setColor(mMenuColorGreen);
        }
        mChangelogMessage->setText("Find the detailed changelog at https://es-de.org");
        mGrid.removeEntry(mButtons);
        mGrid.setEntry(MenuComponent::makeButtonGrid(std::vector<std::shared_ptr<ButtonComponent>> {
                           std::make_shared<ButtonComponent>("QUIT", "quit application",
                                                             [this]() {
                                                                 delete this;
                                                                 Utils::Platform::quitES();
                                                             })}),
                       glm::ivec2 {0, 10}, true, false, glm::ivec2 {4, 1}, GridFlags::BORDER_TOP);
        mGrid.moveCursorTo(0, 10);
        mReadyToInstall = false;
        mHasInstalled = false;
        mHasDownloaded = true;
    }
}

void GuiApplicationUpdater::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    renderChildren(trans);

    if (mDownloading)
        mBusyAnim.render(trans);
}

void GuiApplicationUpdater::onSizeChanged()
{
    const float screenSize {mRenderer->getIsVerticalOrientation() ? mRenderer->getScreenWidth() :
                                                                    mRenderer->getScreenHeight()};
    mGrid.setRowHeightPerc(0, (mTitle->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y /
                                  2.0f);
    mGrid.setRowHeightPerc(1, (mStatusHeader->getFont()->getLetterHeight() + screenSize * 0.2f) /
                                  mSize.y / 4.0f);
    mGrid.setRowHeightPerc(2, (mProcessStep1->getFont()->getLetterHeight() + screenSize * 0.2f) /
                                  mSize.y / 4.0f);
    mGrid.setRowHeightPerc(3, (mProcessStep2->getFont()->getLetterHeight() + screenSize * 0.2f) /
                                  mSize.y / 4.0f);
    mGrid.setRowHeightPerc(4, (mProcessStep3->getFont()->getLetterHeight() + screenSize * 0.2f) /
                                  mSize.y / 4.0f);
    mGrid.setRowHeightPerc(
        5,
        (mStatusMessageHeader->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y / 4.0f);
    mGrid.setRowHeightPerc(
        6,
        (mStatusMessageHeader->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y / 4.0f);
    mGrid.setRowHeightPerc(7, (mStatusMessage->getFont()->getLetterHeight() + screenSize * 0.15f) /
                                  mSize.y / 4.0f);
    mGrid.setRowHeightPerc(
        8, (mChangelogMessage->getFont()->getLetterHeight() + screenSize * 0.15f) / mSize.y / 4.0f);
    mGrid.setRowHeightPerc(10, mButtons->getSize().y / mSize.y);

    mGrid.setColWidthPerc(0, 0.01f);
    mGrid.setColWidthPerc(3, 0.01f);

    mGrid.setSize(mSize);
    mBackground.fitTo(mSize);
}

std::vector<HelpPrompt> GuiApplicationUpdater::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};
    return prompts;
}
