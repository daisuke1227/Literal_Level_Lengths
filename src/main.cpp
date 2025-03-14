#include <Geode/modify/LevelInfoLayer.hpp>
#include <vector>
#include <regex>

using namespace geode::prelude;

std::map<int, double> sessionLengths = {};
const std::unordered_map<std::string, double> unitToMultiplier = {
	{"m", 1.0},
	{"km", 0.001},
	{"ft", 3.281},
	{"in", 39.37},
	{"yd", 1.093},
	{"mi", 0.000621371}
};
bool isBetterInfo = false;

double getMaxPos(GJGameLevel* level) {
	double max = 0;

	std::string decompString = ZipUtils::decompressString(level->m_levelString, true, 0);
	std::regex pattern("[^;]+");
	std::smatch matches;

	auto it = std::sregex_iterator(decompString.begin(), decompString.end(), pattern);
	++it;
	auto end = std::sregex_iterator();
	for (; it != end; ++it) {
		const std::smatch& match = *it;
		std::vector<std::string> elements;

		std::stringstream ss(match.str());
		std::string element;

		while (getline(ss, element, ',')) {
			elements.push_back(element);
		}

		if (3 < elements.size() && utils::numFromString<double>(elements[3]).unwrapOr(-1.0) > max)
			max = utils::numFromString<double>(elements[3]).unwrapOr(-1.0);
	}
	double length = (max + 15.0) / 30.0;
	sessionLengths[level->m_levelID.value()] = length;

	return length;
}

std::string getLengthString(GJGameLevel* level) {
	std::string str = "NA";

	const bool savedPreviously = sessionLengths.contains(level->m_levelID.value());
	const bool levelDownloaded = !static_cast<std::string>(level->m_levelString).empty();
	const bool stringIsInMap = unitToMultiplier.contains(Mod::get()->getSettingValue<std::string>("unitAsString"));
	const std::string& unitAsString = Mod::get()->getSettingValue<std::string>("unitAsString");
	const double multiplier = stringIsInMap ? unitToMultiplier.find(unitAsString)->second : 1.0;

	const double length = savedPreviously ? sessionLengths[level->m_levelID.value()] : (levelDownloaded ? getMaxPos(level) : 0.f);

	if ((levelDownloaded && !savedPreviously) || savedPreviously) {
		double convertedLength = length * multiplier;
		if (convertedLength != static_cast<int>(convertedLength)) {
			std::stringstream ss;
			ss << std::fixed << std::setprecision(2) << convertedLength;
			str = ss.str();
		} else str = std::to_string(static_cast<int>(convertedLength));

		str.append(stringIsInMap ? unitAsString : "m");
	}

	return str;
}

class $modify(LevelInfoLayer) {
	struct Fields {
		CCLabelBMFont* literalLengthLabel = nullptr;
	};
	bool init(GJGameLevel* level, bool p1) {
		if (!LevelInfoLayer::init(level, p1)) return false;

		if (!Mod::get()->getSettingValue<bool>("enabled")) return true;

		// positions stolen from better info so it looks similar
		if (!m_exactLengthLabel) return true;
		const auto fields = m_fields.self();

		fields->literalLengthLabel = CCLabelBMFont::create(getLengthString(level).c_str(), "bigFont.fnt");
		fields->literalLengthLabel->setPosition(m_exactLengthLabel->getPosition());
		fields->literalLengthLabel->setAnchorPoint({0, .5f});
		fields->literalLengthLabel->setScale(0.325f);
		fields->literalLengthLabel->setPositionY(fields->literalLengthLabel->getPositionY() - (m_exactLengthLabel->getContentHeight() * m_exactLengthLabel->getScale()));
		if (!level->isPlatformer()) fields->literalLengthLabel->setPositionY(fields->literalLengthLabel->getPositionY() + 6.f);
		if (!m_exactLengthLabel->isVisible() && !isBetterInfo) fields->literalLengthLabel->setPositionY(m_exactLengthLabel->getPositionY());
		this->addChild(fields->literalLengthLabel);
		fields->literalLengthLabel->setID("literal-length-label"_spr);
		
		return true;
	}
	void levelDownloadFinished(GJGameLevel* level) {
		LevelInfoLayer::levelDownloadFinished(level);

		if (!Mod::get()->getSettingValue<bool>("enabled") || !m_exactLengthLabel) return;
		if (sessionLengths.contains(level->m_levelID.value())) sessionLengths.erase(level->m_levelID.value());

		const auto fields = m_fields.self();
		if (!fields->literalLengthLabel) return;
		fields->literalLengthLabel->setString(getLengthString(level).c_str());
	}
};

$on_mod(Loaded) {
	isBetterInfo = Loader::get()->isModLoaded("cvolton.betterinfo");
}