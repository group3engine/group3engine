#ifndef CONFIG_MENU_HPP
#define CONFIG_MENU_HPP

#include "BaseMenu.hpp"
#include "SampleGLTFFilePaths.hpp"

class UIManager;
class Context;

class ConfigGameMenu : public BaseMenu
{
  public:
    explicit ConfigGameMenu(Context &context, UIManager &uiManager);
    void Render(ImVec2 screenSize) override;
    void Resize() override;
    size_t NumPlayers;


    const std::vector<std::filesystem::path> scenePaths = {
        Sample::SampleObbyTestScene,
        Sample::ArrowSample,
        Sample::AxeSample,
        Sample::TileSample,
        Sample::SpikePitSample,
        Sample::LadderSample,
        Sample::SinkingSample,
        Sample::LeverSample,
        Sample::BoulderSample,
        Sample::SpikeTrapSample,
        Sample::DisappearingPlatformSample,
    };

    std::filesystem::path m_SelectedLevelPath = Sample::SampleObbyTestScene; // Default path is test scene
};

#endif // CONFIG_MENU_HPP
