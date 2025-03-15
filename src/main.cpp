#include <Geode/modify/LevelInfoLayer.hpp>
#include <vector>
#include <regex>

using namespace geode::prelude;

std::map<int, float> sessionLengths = {};
const std::unordered_map<std::string, float> unitToMultiplier = {
	{"m", 1.f},
	{"km", 0.001f},
	{"ft", 3.281f},
	{"in", 39.37f},
	{"yd", 1.093f},
	{"mi", 0.000621371f},
	{"nmi", 0.000539957f},
	{"ch", 0.0497097f},
	{"lea", 0.000207123f}
};

bool isBetterInfo = false;

std::vector<std::string> splitByChar(std::string str, char splitChar) {
    std::vector<std::string> strs;
    strs.reserve(std::count(str.begin(), str.end(), splitChar) + 1);

    size_t start = 0;
    size_t end = str.find(splitChar);
    while (end != std::string::npos) {
        strs.emplace_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(splitChar, start);
    }
    strs.emplace_back(str.substr(start));

    return strs;
}

float getMaxPos(GJGameLevel* level) {
	float max = 0;

	std::string decompString = ZipUtils::decompressString(level->m_levelString, true, 0);

	for (const std::string& obj : splitByChar(decompString, ';')) {	
	
		std::vector<std::string> elements = splitByChar(obj, ',');

		if (elements.size() > 3 && utils::numFromString<float>(elements[3]).unwrapOr(-1.f) > max)
			max = utils::numFromString<float>(elements[3]).unwrapOr(-1.f);
	}
	float length = (max + 15.f) / 30.f;
	sessionLengths[level->m_levelID.value()] = length;

	return length;
}

std::string getLengthString(GJGameLevel* level) {
	std::string str = "NA";

	const bool savedPreviously = sessionLengths.contains(level->m_levelID.value());
	const bool levelDownloaded = !static_cast<std::string>(level->m_levelString).empty();
	const bool stringIsInMap = unitToMultiplier.contains(Mod::get()->getSettingValue<std::string>("unitAsString"));

	const std::string& unitAsString = Mod::get()->getSettingValue<std::string>("unitAsString");
	const float multiplier = stringIsInMap ? unitToMultiplier.find(unitAsString)->second : 1.f;

	const float length = savedPreviously ? sessionLengths[level->m_levelID.value()] : (levelDownloaded ? getMaxPos(level) : 0.f);

	if ((levelDownloaded && !savedPreviously) || savedPreviously) {
		float convertedLength = length * multiplier;
		if (convertedLength != static_cast<int>(convertedLength)) {
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << convertedLength;
			str = ss.str();
		} else str = std::to_string(static_cast<int>(convertedLength));

		str.append(stringIsInMap ? unitAsString : "m");
	}

	return str;
}

class $modify(ProLevelInfoLayer, LevelInfoLayer) {

	struct Fields {
		CCLabelBMFont* literalLengthLabel = nullptr;
	};

	void addLabel(std::string str) {
		Loader::get()->queueInMainThread([this, str] {
			const auto fields = m_fields.self();

			if (fields->literalLengthLabel) return fields->literalLengthLabel->setString(str.c_str());

			fields->literalLengthLabel = CCLabelBMFont::create(str.c_str(), "bigFont.fnt");
			fields->literalLengthLabel->setPosition(m_exactLengthLabel->getPosition());
			fields->literalLengthLabel->setAnchorPoint({0, .5f});
			fields->literalLengthLabel->setScale(0.325f);
			fields->literalLengthLabel->setPositionY(fields->literalLengthLabel->getPositionY() - (m_exactLengthLabel->getContentHeight() * m_exactLengthLabel->getScale()));

			
			if (!m_exactLengthLabel->isVisible() && !isBetterInfo)
				fields->literalLengthLabel->setPositionY(m_exactLengthLabel->getPositionY());
			else {
				m_exactLengthLabel->setScale(0.275f);
				m_exactLengthLabel->setPositionY(m_exactLengthLabel->getPositionY() + 2);

				fields->literalLengthLabel->setScale(0.275f);
				fields->literalLengthLabel->setPositionY(fields->literalLengthLabel->getPositionY() + 4);
			}


			this->addChild(fields->literalLengthLabel);
			fields->literalLengthLabel->setID("literal-length-label"_spr);
		});
	}

	bool init(GJGameLevel* level, bool p1) {
		if (!LevelInfoLayer::init(level, p1)) return false;

		if (!Mod::get()->getSettingValue<bool>("enabled")) return true;

		// positions stolen from better info so it looks similar
		if (!m_exactLengthLabel) return true;
		if (sessionLengths.contains(level->m_levelID.value())) {
			addLabel(getLengthString(level));
			return true;
		}

		std::thread([&, this, level]() {
			std::string levelString = getLengthString(level);
			
			auto startTime = std::chrono::high_resolution_clock::now();
			while (!m_enterTransitionFinished) {
				if (std::chrono::high_resolution_clock::now() - startTime > std::chrono::duration<double, std::milli>(5000))
					break;
	
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
			if (scene->getChildByType<LevelInfoLayer>(0) != this) return;

			Loader::get()->queueInMainThread([&, this, levelString] {
				addLabel(levelString);
			});

		}).detach();
		
		return true;
	}

	void levelDownloadFinished(GJGameLevel* level) {
		LevelInfoLayer::levelDownloadFinished(level);
		if (!Mod::get()->getSettingValue<bool>("enabled") || !m_exactLengthLabel) return;
		if (sessionLengths.contains(level->m_levelID.value())) sessionLengths.erase(level->m_levelID.value());

		std::thread([this, level]() {
			std::string levelString = getLengthString(level);

			auto startTime = std::chrono::high_resolution_clock::now();
			while (!m_enterTransitionFinished) {
				if (std::chrono::high_resolution_clock::now() - startTime > std::chrono::duration<double, std::milli>(5000))
					break;
	
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			CCScene* scene = CCDirector::sharedDirector()->getRunningScene();
			if (scene->getChildByType<LevelInfoLayer>(0) != this) return;

			Loader::get()->queueInMainThread([&, this, levelString] {
				addLabel(levelString);
			});

		}).detach();
	}
};

$on_mod(Loaded) {
	isBetterInfo = Loader::get()->isModLoaded("cvolton.betterinfo");
}