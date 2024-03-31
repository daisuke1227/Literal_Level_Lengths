#include <Geode/loader/SettingNode.hpp>

using namespace geode::prelude;

const int unitsSize = 6;

const std::string units[unitsSize] = {
    "m", "km", "ft", "in", "yd", "mi"
};

const float multipliers[unitsSize] = {
    1.f, 0.001f, 3.281f, 39.37f, 1.093f, 0.000621371f
};

class unit {
    unit() {}
public:
    int currentUnit = 0;

    static auto& get() {
        static unit instance;
        return instance;
    }
};

class ButtonCustomSettingValue;

class ButtonCustomSettingValue : public SettingValue {
protected:
    std::string m_placeholder;
public:
    ButtonCustomSettingValue(std::string const& key, std::string const& modID, std::string const& placeholder)
        : SettingValue(key, modID), m_placeholder(placeholder) {}

    bool load(matjson::Value const& json) override {
        return true;
    }

    bool save(matjson::Value& json) const override {
        return true;
    }

    SettingNode* createNode(float width) override;
};


class ButtonCustomSettingNode : public SettingNode {
    CCLabelBMFont* unitLabel = nullptr;
protected:
    bool init(ButtonCustomSettingValue* value, float width) {
        if (!SettingNode::init(value))
            return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        this->setContentSize({ width, 35.f });
        auto menu = CCMenu::create();
        menu->setPosition({ 0, 0 });

        auto label = CCLabelBMFont::create("Measurement Unit", "bigFont.fnt");
        label->setScale(0.58f);
        label->setPosition({18, 17});
        label->setAnchorPoint({ 0.f, 0.5f });
        this->addChild(label);

        auto spr = CCSprite::createWithSpriteFrameName("edit_leftBtn_001.png");
        spr->setScale(0.8f);

        auto btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(ButtonCustomSettingNode::switchUnit)
        );
        btn->setPosition({240, 17});
        btn->setID("left");
        menu->addChild(btn);

        spr = CCSprite::createWithSpriteFrameName("edit_rightBtn_001.png");
        spr->setScale(0.8f);
        btn = CCMenuItemSpriteExtra::create(
            spr,
            this,
            menu_selector(ButtonCustomSettingNode::switchUnit)
        );
        btn->setPosition({316, 17});
        btn->setID("right");
        menu->addChild(btn);

        unitLabel = CCLabelBMFont::create(units[unit::get().currentUnit].c_str(), "bigFont.fnt");
        unitLabel->setScale(0.6f);
        unitLabel->setPosition({278, 17});
        unitLabel->setAnchorPoint({ 0.5, 0.5 });
        this->addChild(unitLabel);

        this->addChild(menu);

        return true;
    }

public:

    void switchUnit(CCObject* obj) {
        if (static_cast<CCLabelBMFont*>(obj)->getID() == "left")
            unit::get().currentUnit--;
        else
            unit::get().currentUnit++;

        if (unit::get().currentUnit == -1) unit::get().currentUnit = unitsSize - 1;
        else if (unit::get().currentUnit == unitsSize) unit::get().currentUnit = 0;

        unitLabel->setString(units[unit::get().currentUnit].c_str());
        Mod::get()->setSavedValue<int64_t>("unit", unit::get().currentUnit);
    }

    void commit() override {
        this->dispatchCommitted();
    }

    bool hasUncommittedChanges() override {
        return false;
    }

    bool hasNonDefaultValue() override {
        return true;
    }

    void resetToDefault() override {

    }

    static ButtonCustomSettingNode* create(ButtonCustomSettingValue* value, float width) {
        auto ret = new ButtonCustomSettingNode;
        if (ret && ret->init(value, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class SectionSettingValue : public SettingValue {
protected:
    std::string m_placeholder;
public:
    SectionSettingValue(std::string const& key, std::string const& modID, std::string const& placeholder)
        : SettingValue(key, modID), m_placeholder(placeholder) {}
    bool load(matjson::Value const& json) override { return true; }
    bool save(matjson::Value& json) const override { return true; }
    SettingNode* createNode(float width) override;
};

class SectionSettingNode : public SettingNode {
protected:
    bool init(SectionSettingValue* value, float width) {
        if (!SettingNode::init(value))
            return false;
        this->setContentSize({ width, 40.f });
        std::string name = Mod::get()->getSettingDefinition(value->getKey())->get<CustomSetting>()->json->get<std::string>("name");

        auto theMenu = CCMenu::create();
        auto theLabel = CCLabelBMFont::create(name.c_str(), "bigFont.fnt");

        theLabel->setScale(.5);
        theLabel->setPositionX(0);
        theMenu->addChild(theLabel);
        theMenu->setPosition(width / 2, 20.f);

        this->addChild(theMenu);

        return true;
    }

public:
    void commit() override {
        this->dispatchCommitted();
    }
    bool hasUncommittedChanges() override {
        return false;
    }
    bool hasNonDefaultValue() override {
        return true;
    }
    void resetToDefault() override {}

    static SectionSettingNode* create(SectionSettingValue* value, float width) {
        auto ret = new SectionSettingNode();
        if (ret && ret->init(value, width)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};