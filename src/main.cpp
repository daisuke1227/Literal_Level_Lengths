
#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include "custom_setting.hpp"
#include <vector>
#include <regex>

using namespace geode::prelude;

std::map<int, float> sessionLengths;

float getMaxPos(GJGameLevel* level) {
	float max = 0;

	std::string decompString = ZipUtils::decompressString(level->m_levelString.c_str(), true, 0);
	std::regex pattern("[^;]+");
	std::smatch matches;

	auto it = std::sregex_iterator(decompString.begin(), decompString.end(), pattern);
	++it;
	auto end = std::sregex_iterator();
	for (; it != end; ++it) {
		std::smatch match = *it;
		std::vector<std::string> elements;

		std::stringstream ss(match.str());
		std::string element;

		while (getline(ss, element, ',')) {
			elements.push_back(element);
		}

		if (3 < elements.size()) {
			if (std::stoi(elements[3]) > max)
				max = std::stoi(elements[3]);
		}
	}
	float length = (max + 15) / 30.f;
	sessionLengths[level->m_levelID.value()] = length;

	return length;
}

std::string getLengthString(GJGameLevel* level) {
	std::string str = "NA";

	bool savedPreviously = sessionLengths.contains(level->m_levelID.value());
	bool levelDownloaded = static_cast<std::string>(level->m_levelString).length() > 0;

	float length = savedPreviously ? sessionLengths[level->m_levelID.value()] : (levelDownloaded ? getMaxPos(level) : 0.f);

	if ((levelDownloaded && !savedPreviously) || savedPreviously) {
		float convertedLength = length * multipliers[unit::get().currentUnit];
		if (convertedLength != static_cast<int>(convertedLength)) {
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << convertedLength;
			str = ss.str();
		}
		else
			str = std::to_string(static_cast<int>(convertedLength));

		str += units[unit::get().currentUnit];
	}

	return str;
}

class $modify(LevelInfoLayer) {
	int m_maxPosX = 0;
	CCLabelBMFont* literalLengthLabel = nullptr;

	bool init(GJGameLevel * level, bool p1) {
		if (!LevelInfoLayer::init(level, p1)) return false;

		if (!Mod::get()->getSettingValue<bool>("enabled")) return true;

		// positions stolen from better info so it looks similar
		auto label = m_lengthLabel;
		if (label) {
			m_fields->literalLengthLabel = CCLabelBMFont::create(getLengthString(level).c_str(), "bigFont.fnt");
			m_fields->literalLengthLabel->setPosition({ label->getPositionX() + 1, label->getPositionY() - (Loader::get()->isModLoaded("cvolton.betterinfo") ? 11.f : 2.f) });
			m_fields->literalLengthLabel->setAnchorPoint({ 0,1 });
			m_fields->literalLengthLabel->setScale(0.325f);

			addChild(m_fields->literalLengthLabel);
			if (!Loader::get()->isModLoaded("cvolton.betterinfo")) label->setPositionY(label->getPositionY() + 6.f);

		}
		return true;
	}

	void levelDownloadFinished(GJGameLevel * level) {
		LevelInfoLayer::levelDownloadFinished(level);

		if (!Mod::get()->getSettingValue<bool>("enabled")) return;

		if (m_fields->literalLengthLabel)
			m_fields->literalLengthLabel->setString(getLengthString(level).c_str());
	}

};

$execute{
	unit::get().currentUnit = Mod::get()->getSavedValue<int64_t>("unit");
}

$on_mod(Loaded) {
	Mod::get()->addCustomSetting<ButtonCustomSettingValue>("unit", "none");
}