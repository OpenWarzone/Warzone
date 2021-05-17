#ifdef _GAME
#include "g_local.h"
#endif

#ifdef _CGAME
#include "../cgame/cg_local.h"
#endif


#if defined(rd_warzone_x86_EXPORTS)
#include "tr_local.h"
#endif

extern inventoryItem *BG_GetInventoryItemByID(uint16_t id);

//
// Configuration Defines...
//


//
// String versions of stats... Needs to match the tables in bg_inventoryItem.h
//
stringID_table_t itemPowerCrystalStrings[] =
{
	ENUM2STRING(ITEM_CRYSTAL_DEFAULT),
	ENUM2STRING(ITEM_CRYSTAL_RED),
	ENUM2STRING(ITEM_CRYSTAL_GREEN),
	ENUM2STRING(ITEM_CRYSTAL_BLUE),
	ENUM2STRING(ITEM_CRYSTAL_WHITE),
	ENUM2STRING(ITEM_CRYSTAL_YELLOW),
	ENUM2STRING(ITEM_CRYSTAL_PURPLE),
	ENUM2STRING(ITEM_CRYSTAL_ORANGE),
	ENUM2STRING(ITEM_CRYSTAL_PINK),
	ENUM2STRING(ITEM_CRYSTAL_MAX),
};

stringID_table_t weaponStat1Strings[] =
{
	ENUM2STRING(WEAPON_STAT1_DEFAULT),
	ENUM2STRING(WEAPON_STAT1_HEAVY_PISTOL),
	ENUM2STRING(WEAPON_STAT1_FIRE_ACCURACY_MODIFIER),
	ENUM2STRING(WEAPON_STAT1_FIRE_RATE_MODIFIER),
	ENUM2STRING(WEAPON_STAT1_VELOCITY_MODIFIER),
	ENUM2STRING(WEAPON_STAT1_HEAT_ACCUMULATION_MODIFIER),
	ENUM2STRING(WEAPON_STAT1_MAX),
};

stringID_table_t weaponStat2Strings[] =
{
	ENUM2STRING(WEAPON_STAT2_DEFAULT),
	ENUM2STRING(WEAPON_STAT2_FIRE_DAMAGE_MODIFIER),
	ENUM2STRING(WEAPON_STAT2_CRITICAL_CHANCE_MODIFIER),
	ENUM2STRING(WEAPON_STAT2_CRITICAL_POWER_MODIFIER),
	ENUM2STRING(WEAPON_STAT2_MAX),
};

stringID_table_t weaponStat3Strings[] =
{
	ENUM2STRING(WEAPON_STAT3_DEFAULT),
	ENUM2STRING(WEAPON_STAT3_SHOT_BOUNCE),
	ENUM2STRING(WEAPON_STAT3_SHOT_EXPLOSIVE),
	ENUM2STRING(WEAPON_STAT3_SHOT_BEAM),
	ENUM2STRING(WEAPON_STAT3_SHOT_WIDE),
	ENUM2STRING(WEAPON_STAT3_SHOT_REPEATING),
	ENUM2STRING(WEAPON_STAT3_MAX),
};

//
// A quality based price scale modifier... Used internally... Matches levels of itemQuality_t.
//
const float qualityPriceModifier[] = {
	1.0f,
	1.333f,
	2.0f,
	3.5f,
	5.75f,
	9.5f,
	12.0f,
};

const char *itemQualityTooltips[] = {
	"Poor",
	"Common",
	"Uncommon",
	"Rare",
	"Epic",
	"Legendary",
	"Artifact",
};

const char *weaponCrystalNames[] = {
	"",
	" of Burning",
	" of Force",
	" of Shock",
	" of Freezing",
	" of Molten Force",
	" of Plasma",
	" of Avalanches",
	" of Inversion",
};

const char *weaponCrystalTooltips[] = {
	"",
	"^B^P+%.1f%% ^Nheat^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^2kinetic^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^4electric^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7cold^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^Nheat^7, and ^P%.1f%% ^2kinetic^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^4electric^7, and ^P%.1f%% ^Nheat^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7cold^7, and ^P%.1f%% ^2kinetic^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^Nheat^7, and ^P%.1f%% ^7cold^7 damage from crystal.^b\n",
};

const char *itemCrystalNames[] = {
	"",
	" of Heat Resistance",
	" of Kinetic Resistance",
	" of Shock Resistance",
	" of Cold Resistance",
	" of Heat and Kinetic Resistance",
	" of Electric and Heat Resistance",
	" of Cold and Kinetic Resistance",
	" of Heat and Cold Resistance",
};

const char *itemCrystalTooltips[] = {
	"",
	"^B^P+%.1f%% ^7resistance to ^Nheat^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7resistance to ^3kinetic^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7resistance to ^4electric^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7resistance to ^7cold^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7resistance to ^Nheat^7, and ^P+%.1f%% ^2kinetic^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7resistance to ^4electric^7, and ^P+%.1f%% ^Nheat^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7resistance to ^7cold^7, and ^P+%.1f%% ^2kinetic^7 damage from crystal.^b\n",
	"^B^P+%.1f%% ^7resistance to ^Nheat^7, and ^P+%.1f%% ^7cold^7 damage from crystal.^b\n",
};

const char *weaponStat1Names[] = {
	"",								// Pistol
	"Skirmishing ",					// Pistol
	"Accurate ",					// Sniper Rifle
	"Quickfire ",					// Blaster Rifle
	"Fast ",						// Assault Rifle
	"Cooled ",						// Heavy Blaster
};

const char *weaponStat1Tooltips[] = {
	"",
	"^P+%.1f%% ^7bonus to damage at close range.\n",
	"^P+%.1f%% ^7bonus to accuracy.\n",
	"^P+%.1f%% ^7bonus to rate of fire.\n",
	"^P+%.1f%% ^7bonus to velocity.\n",
	"^P+%.1f%% ^7reduction to heat accumulation.\n",
};

const char *weaponStat2Names[] = {
	"",
	"Deadly ",
	"Pinpointing ",
	"Charged ",
};

const char *weaponStat2Tooltips[] = {
	"",
	"^P+%.1f%% ^7bonus to damage.\n",
	"^P+%.1f%% ^7bonus to critical chance.\n",
	"^P+%.1f%% ^7bonus to critical power.\n",
};

const char *weaponStat3Names[] = {
	"",
	"Ricochet ",
	"Explosive ",
	"Beam ",
	"Expanded ",
	"Repeating ",
};

const char *weaponStat3Tooltips[] = {
	"",
	"^PRicochet.\n",
	"^PExplosive.\n",
	"^PBeam.\n",
	"^PWide Shot.\n",
	"^PRepeating.\n",
};

const char *saberStat1Names[] = {
	"",
	"Parrying ",
	"Reflecting ",
	"Staggerring ",
};

const char *saberStat1Tooltips[] = {
	"",
	"^P+%.1f%% ^7bonus to melee blocking chance.\n",
	"^P+%.1f%% ^7bonus to ranged blocking chance.\n",
	"^P+%.1f%% ^7bonus to stagger chance.\n",
};

const char *saberStat2Names[] = {
	"",
	"Deadly ",
	"Pinpointing ",
	"Charged ",
	"Piercing ",
	"Vampyric ",
	"Leeching ",
};

const char *saberStat2Tooltips[] = {
	"",
	"^P+%.1f%% ^7bonus to damage.\n",
	"^P+%.1f%% ^7bonus to critical chance.\n",
	"^P+%.1f%% ^7bonus to critical power.\n",
	"^P+%.1f%% ^7bonus shield penetration.\n",
	"^P+%.1f%% ^7bonus health drain.\n",
	"^P+%.1f%% ^7bonus power drain.\n",
};

const char *saberStat3Names[] = {
	"",
	"Long ",
	"Fast ",
};

const char *saberStat3Tooltips[] = {
	"",
	"^P+%.1f%% ^7bonus to blade length.\n",
	"^P+%.1f%% ^7bonus to attack speed.\n",
};

const char *itemStatNames[] = {
	"",
	"Healthy ",
	"Regenerative ",
	"Shielded ",
	"Recharging ",
	"Enlightening ",
	"Replenishing ",
	"Strengthening ",
	"Evasive ",
	"Speedy ",
	"Nimble ",
	"Protecting ",
	"Armored ",
	"Covering ",
};

const char *itemStatTooltips[] = {
	"",
	"^P+%.1f%% ^7bonus to maximum health.\n",
	"^P+%.1f%% ^7bonus to health regeneration.\n",
	"^P+%.1f%% ^7bonus to maximum shields.\n",
	"^P+%.1f%% ^7bonus to shield regeneration.\n",
	"^P+%.1f%% ^7bonus to maximum force power.\n",
	"^P+%.1f%% ^7bonus to force power regeneration.\n",
	"^P+%.1f%% ^7bonus to strength.\n",
	"^P+%.1f%% ^7bonus to evasion.\n",
	"^P+%.1f%% ^7bonus to speed.\n",
	"^P+%.1f%% ^7bonus to speed and %.1f%% bonus to evasion.\n",
	"^P+%.1f%% ^7bonus to blocking.\n",
	"^P+%.1f%% ^7damage reduction.\n",
	"^P+%.1f%% ^7shield penetration reduction.\n",
};

//
// Construction/Destruction...
//
inventoryItem::inventoryItem(uint16_t itemID)
{
	m_itemID = itemID;
	m_bgItemID = 0;

	m_modelType = SABER_MODELTYPE_DEFAULT;
	m_quality = QUALITY_GREY;
	m_quantity = 0;

	m_crystal = 0;
	m_basicStat1 = 0;
	m_basicStat2 = 0;
	m_basicStat3 = 0;
	m_basicStat1value = 0.0;
	m_basicStat2value = 0.0;
	m_basicStat3value = 0.0;

	// Reforged and Unique items...
	m_customType = ITEM_CUSTOMIZATION_DEFAULT;
	setCustomName("");
	setCustomDescription("");
#ifdef _CGAME
	setCustomModel("");
	setCustomEfx("");
	setCustomSounds("");
#endif //_CGAME
}

inventoryItem::inventoryItem(uint16_t itemID, uint16_t bgItemID, itemQuality_t quality, uint16_t type = 0, uint16_t amount = 1)
{
	m_itemID = itemID;
	m_bgItemID = bgItemID;

	m_modelType = type;
	m_quality = quality;
	m_quantity = amount;

	m_crystal = 0;
	m_basicStat1 = 0;
	m_basicStat2 = 0;
	m_basicStat3 = 0;
	m_basicStat1value = 0.0;
	m_basicStat2value = 0.0;
	m_basicStat3value = 0.0;

	// Reforged and Unique items...
	m_customType = ITEM_CUSTOMIZATION_DEFAULT;
	setCustomName("");
	setCustomDescription("");
#ifdef _CGAME
	setCustomModel("");
	setCustomEfx("");
	setCustomSounds("");
#endif //_CGAME
}

inventoryItem::~inventoryItem()
{

}

//
// Item Setup Functions...
//
void inventoryItem::setItemID(uint16_t itemID)
{
	m_itemID = itemID;
}

void inventoryItem::setBaseItem(uint16_t bgItemID)
{
	m_bgItemID = bgItemID;
}

void inventoryItem::setModelType(uint16_t type)
{
	m_modelType = type;
}

void inventoryItem::setQuality(itemQuality_t quality)
{
	m_quality = quality;
}

void inventoryItem::setQuantity(uint16_t amount)
{
	m_quantity = amount;
}

void inventoryItem::setCrystal(uint16_t crystalType)
{
	m_crystal = crystalType;
}

void inventoryItem::setStat1(uint16_t statType, float statValue)
{
	if (!isModification() && m_quality <= QUALITY_GREY) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat2 || m_basicStat3)) return; // mods can only have 1 stat type.
	
	m_basicStat1 = statType;
	m_basicStat1value = statValue;
}

void inventoryItem::setStat2(uint16_t statType, float statValue)
{
	if (!isModification() && m_quality <= QUALITY_WHITE) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat1 || m_basicStat3)) return; // mods can only have 1 stat type.

	m_basicStat2 = statType;
	m_basicStat2value = statValue;
}

void inventoryItem::setStat3(uint16_t statType, float statValue)
{
	if (!isModification() && m_quality <= QUALITY_GREEN) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat1 || m_basicStat2)) return; // mods can only have 1 stat type.

	m_basicStat3 = statType;
	m_basicStat3value = statValue;
}

// Reforged/Unique weapon information...
itemCustomType_t inventoryItem::getCustomType()
{
	return m_customType;
}

void inventoryItem::setCustomType(itemCustomType_t type)
{
	m_customType = type;
}

char *inventoryItem::getCustomName()
{
	return m_customName;
}

void inventoryItem::setCustomName(char *name)
{
	Q_strncpyz(m_customName, name, 64);
}

char *inventoryItem::getCustomDescription()
{
	return m_customDescription;
}

void inventoryItem::setCustomDescription(char *description)
{
	Q_strncpyz(m_customDescription, description, 256);
}

#ifdef _CGAME
char *inventoryItem::getCustomModel()
{
	return m_customModel;
}

void inventoryItem::setCustomModel(char *model)
{
	Q_strncpyz(m_customModel, model, 128);
}

char *inventoryItem::getCustomEfx()
{
	return m_customEfx;
}

void inventoryItem::setCustomEfx(char *efxFolder)
{
	Q_strncpyz(m_customEfx, efxFolder, 128);
}

char *inventoryItem::getCustomSounds()
{
	return m_customSounds;
}

void inventoryItem::setCustomSounds(char *soundFolder)
{
	Q_strncpyz(m_customSounds, soundFolder, 128);
}
#endif //_CGAME

//
// Item Accessing Functions...
//
uint16_t inventoryItem::getItemID()
{
	return m_itemID;
}

gitem_t *inventoryItem::getBaseItem()
{
	return &bg_itemlist[m_bgItemID];
}

uint16_t inventoryItem::getBaseItemID()
{
	return m_bgItemID;
}

uint16_t inventoryItem::getModelType()
{
	return m_modelType;
}

void inventoryItem::getName(std::string &name, uint16_t modItemID1)
{
	int giType = getBaseItem()->giType;

	name.clear();

	if (giType == IT_WEAPON && getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
	{// Reforge and Unique only supports weapons, for now...
		char *cname = getCustomName();

		if (cname && cname[0] && strlen(cname) > 0)
		{
			name = va("%s%s%s (%s)", getColorStringForQuality(), cname, weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
			return;
		}
	}

	switch (giType)
	{
	case IT_WEARABLE:
		name = va("%s%s%s%s%s (%s)", getColorStringForQuality(), itemStatNames[getBasicStat1()], itemStatNames[getBasicStat2()], itemStatNames[getBasicStat3()], getBaseItem()->name, itemCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		break;
	case IT_ITEM_MODIFICATION:
		name = va("%s%s%s%s%s (%s)", getColorStringForQuality(), itemStatNames[getBasicStat1()], itemStatNames[getBasicStat2()], itemStatNames[getBasicStat3()], getBaseItem()->name, itemQualityTooltips[getQuality()]);
		break;
	case IT_ITEM_CRYSTAL:
		name = va("%s%s%s (%s)", getColorStringForQuality(), getBaseItem()->name, itemCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		break;
	case IT_WEAPON_MODIFICATION:
		name = va("%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], getBaseItem()->name, itemQualityTooltips[getQuality()]);
		break;
	case IT_WEAPON_CRYSTAL:
		name = va("%s%s%s (%s)", getColorStringForQuality(), getBaseItem()->name, weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		break;
	case IT_SABER_MODIFICATION:
		name = va("%s%s%s%s%s (%s)", getColorStringForQuality(), saberStat1Names[getBasicStat1()], saberStat2Names[getBasicStat2()], saberStat3Names[getBasicStat3()], getBaseItem()->name, itemQualityTooltips[getQuality()]);
		break;
	case IT_SABER_CRYSTAL:
		name = va("%s%s%s (%s)", getColorStringForQuality(), getBaseItem()->name, weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		break;
	case IT_WEAPON:
		if (getBaseItem()->giTag == WP_SABER)
		{
			if (getModelType() == SABER_MODELTYPE_ELECTROSTAFF)
				name = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), saberStat1Names[getBasicStat1()], saberStat2Names[getBasicStat2()], saberStat3Names[getBasicStat3()], "Electrostaff", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
			else
				name = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), saberStat1Names[getBasicStat1()], saberStat2Names[getBasicStat2()], saberStat3Names[getBasicStat3()], getBaseItem()->name, weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		}
		else
		{
			switch (getVisualType1(modItemID1))
			{
			case WEAPON_STAT1_DEFAULT:
			default:
				name = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Pistol", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_HEAVY_PISTOL:
				name = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Heavy Pistol", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_FIRE_ACCURACY_MODIFIER:
				name = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Sniper Rifle", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_FIRE_RATE_MODIFIER:
				name = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Blaster Rifle", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_VELOCITY_MODIFIER:
				name = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Assault Rifle", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_HEAT_ACCUMULATION_MODIFIER:
				name = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Heavy Blaster", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			}
		}
		break;
	default:
		// hmm. shouldn't happen, hopefully...
		break;
	}
}

char *inventoryItem::getDescription()
{
	if (getBaseItem()->giType == IT_WEAPON && getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
	{// Reforge and Unique only supports weapons, for now...
		char *cdescription = getCustomDescription();

		if (cdescription && cdescription[0] && strlen(cdescription) > 0)
		{
			return cdescription;
		}
	}

	return getBaseItem()->description;
}

itemQuality_t inventoryItem::getQuality()
{
	return m_quality;
}

uint16_t inventoryItem::getQuantity()
{
	return m_quantity;
}

uint16_t inventoryItem::getBasicStat1()
{
	return m_basicStat1;
}

uint16_t inventoryItem::getBasicStat2()
{
	return m_basicStat2;
}

uint16_t inventoryItem::getBasicStat3()
{
	return m_basicStat3;
}

float inventoryItem::getBasicStat1Value()
{
	if (getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
	{
		if (getCustomType() == ITEM_CUSTOMIZATION_REFORGE)
		{
			return m_basicStat1value * 1.15f;
		}
		else if (getCustomType() == ITEM_CUSTOMIZATION_UNIQUE)
		{
			return m_basicStat1value * 1.25f;
		}
	}

	return m_basicStat1value;
}

float inventoryItem::getBasicStat2Value()
{
	if (getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
	{
		if (getCustomType() == ITEM_CUSTOMIZATION_REFORGE)
		{
			return m_basicStat2value * 1.15f;
		}
		else if (getCustomType() == ITEM_CUSTOMIZATION_UNIQUE)
		{
			return m_basicStat2value * 1.25f;
		}
	}

	return m_basicStat2value;
}

float inventoryItem::getBasicStat3Value()
{
	if (getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
	{
		if (getCustomType() == ITEM_CUSTOMIZATION_REFORGE)
		{
			return m_basicStat3value * 1.15f;
		}
		else if (getCustomType() == ITEM_CUSTOMIZATION_UNIQUE)
		{
			return m_basicStat3value * 1.25f;
		}
	}

	return m_basicStat3value;
}

uint16_t inventoryItem::getCrystal()
{
	return m_crystal;
}

uint16_t inventoryItem::getMod1Stat(uint16_t modItemID)
{
	return BG_GetInventoryItemByID(modItemID)->getBasicStat1();
}

uint16_t inventoryItem::getMod2Stat(uint16_t modItemID)
{
	return BG_GetInventoryItemByID(modItemID)->getBasicStat2();
}

uint16_t inventoryItem::getMod3Stat(uint16_t modItemID)
{
	return BG_GetInventoryItemByID(modItemID)->getBasicStat3();
}

float inventoryItem::getMod1Value(uint16_t modItemID)
{
	return BG_GetInventoryItemByID(modItemID)->getBasicStat1Value();
}

float inventoryItem::getMod2Value(uint16_t modItemID)
{
	return BG_GetInventoryItemByID(modItemID)->getBasicStat2Value();
}

float inventoryItem::getMod3Value(uint16_t modItemID)
{
	return BG_GetInventoryItemByID(modItemID)->getBasicStat3Value();
}

uint16_t inventoryItem::getVisualType1(uint16_t modItemID)
{
	return getMod1Stat(modItemID) ? getMod1Stat(modItemID) : m_basicStat1;
}

uint16_t inventoryItem::getVisualType2(uint16_t modItemID)
{
	return getMod2Stat(modItemID) ? getMod2Stat(modItemID) : m_basicStat2;
}

uint16_t inventoryItem::getVisualType3(uint16_t modItemID)
{
	return getMod3Stat(modItemID) ? getMod3Stat(modItemID) : m_basicStat3;
}

float inventoryItem::getCrystalPower(void)
{
	if (getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
	{
		if (getCustomType() == ITEM_CUSTOMIZATION_REFORGE)
		{
			return 0.025f * pow(float(m_quality + 1), 1.1f) * 1.15f;
		}
		else if (getCustomType() == ITEM_CUSTOMIZATION_UNIQUE)
		{
			return 0.025f * pow(float(m_quality + 1), 1.1f) * 1.25f;
		}
	}

	return 0.025 * pow(float(m_quality + 1), 1.1);
}

float inventoryItem::getCost(uint16_t modItemID1, uint16_t modItemID2, uint16_t modItemID3)
{// Apply multipliers based on how many extra stats this item has...
	float crystalCostMultiplier = getCrystal() ? 1.5 * (1.0 + getCrystalPower()) : 1.0;
	float dualbladeCostMultiplier = (getBaseItem()->giTag == WP_SABER && getModelType() == SABER_MODELTYPE_STAFF) ? 1.5 * (1.0 + getCrystalPower()) : 1.0;
	float statCostMultiplier1 = getBasicStat1() ? 1.25 * (1.0 + getBasicStat1Value()) : 1.0;
	float statCostMultiplier2 = getBasicStat1() ? 1.25 * (1.0 + getBasicStat2Value()) : 1.0;
	float statCostMultiplier3 = getBasicStat1() ? 1.25 * (1.0 + getBasicStat3Value()) : 1.0;
	float modCostMultiplier1 = getMod1Stat(modItemID1) ? 1.25 * (1.0 + getMod1Value(modItemID1)) : 1.0;
	float modCostMultiplier2 = getMod2Stat(modItemID2) ? 1.25 * (1.0 + getMod2Value(modItemID2)) : 1.0;
	float modCostMultiplier3 = getMod3Stat(modItemID3) ? 1.25 * (1.0 + getMod3Value(modItemID3)) : 1.0;

	if (getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
	{
		if (getCustomType() == ITEM_CUSTOMIZATION_REFORGE)
		{
			crystalCostMultiplier *= 1.15f;
			dualbladeCostMultiplier *= 1.15f;
			statCostMultiplier1 *= 1.15f;
			statCostMultiplier2 *= 1.15f;
			statCostMultiplier3 *= 1.15f;
			modCostMultiplier1 *= 1.15f;
			modCostMultiplier2 *= 1.15f;
			modCostMultiplier3 *= 1.15f;

			return getBaseItem()->price * 1.15f * statCostMultiplier1 * statCostMultiplier2 * statCostMultiplier3 * modCostMultiplier1 * modCostMultiplier2 * modCostMultiplier3 * crystalCostMultiplier * qualityPriceModifier[m_quality];
		}
		else if (getCustomType() == ITEM_CUSTOMIZATION_UNIQUE)
		{
			crystalCostMultiplier *= 1.25f;
			dualbladeCostMultiplier *= 1.25f;
			statCostMultiplier1 *= 1.25f;
			statCostMultiplier2 *= 1.25f;
			statCostMultiplier3 *= 1.25f;
			modCostMultiplier1 *= 1.25f;
			modCostMultiplier2 *= 1.25f;
			modCostMultiplier3 *= 1.25f;

			return getBaseItem()->price * 1.25f * statCostMultiplier1 * statCostMultiplier2 * statCostMultiplier3 * modCostMultiplier1 * modCostMultiplier2 * modCostMultiplier3 * crystalCostMultiplier * qualityPriceModifier[m_quality];
		}
	}

	return getBaseItem()->price * statCostMultiplier1 * statCostMultiplier2 * statCostMultiplier3 * modCostMultiplier1 * modCostMultiplier2 * modCostMultiplier3 * crystalCostMultiplier * qualityPriceModifier[m_quality];
}

float inventoryItem::getStackCost(uint16_t modItemID1, uint16_t modItemID2, uint16_t modItemID3)
{// Apply multipliers based on how many extra stats this item has...
	return getCost(modItemID1, modItemID2, modItemID3) * (m_quantity <= 1) ? 1 : m_quantity;
}

bool inventoryItem::getIsTwoHanded(uint16_t modItemID1)
{
	if (getBaseItem()->giTag == WP_SABER)
	{
		switch (getModelType())
		{
		case SABER_MODELTYPE_DEFAULT:
		default:
			return qfalse;
			break;
		case SABER_MODELTYPE_STAFF:
		case SABER_MODELTYPE_ELECTROSTAFF:
			return qtrue;
			break;
		}

		return qfalse;
	}

	bool		isTwoHanded = true;
	uint16_t	stat1Type = (modItemID1 && getMod1Stat(modItemID1)) ? getMod1Stat(modItemID1) : getBasicStat1();

	switch (stat1Type)
	{
	case WEAPON_STAT1_DEFAULT:
	case WEAPON_STAT1_HEAVY_PISTOL:
		isTwoHanded = false;
		break;
	default:
		isTwoHanded = true;
		break;
	}

	return isTwoHanded;
}

const char *inventoryItem::getColorStringForQuality()
{
	switch (getQuality())
	{// TODO: Not use Q3 strings so we can have more colors... The GUI lib we use probably allows for color and formatting stings anyway...
	case QUALITY_GREY:
		return "^8";
		break;
	case QUALITY_WHITE:
		return "^7";
		break;
	case QUALITY_GREEN:
		return "^2";
		break;
	case QUALITY_BLUE:
		return "^4";
		break;
	case QUALITY_PURPLE:
		return "^0";
		break;
	case QUALITY_ORANGE:
		return "^9";
		break;
	case QUALITY_GOLD:
		return "^3";
		break;
	default:
		break;
	}

	return "^5";
}

void inventoryItem::getTooltip(std::string &tooltipText, uint16_t modItemID1, uint16_t modItemID2, uint16_t modItemID3)
{
	int giType = getBaseItem()->giType;

	static std::string itemName;
	itemName.clear();
	tooltipText.clear();

	getName(itemName, modItemID1);

	switch (giType)
	{
	case IT_WEARABLE:
		tooltipText = va("^B%s^b\n", itemName.c_str());
		tooltipText.append("^PClothing\n");
		tooltipText.append(" \n");
		tooltipText.append(va("^5%s\n", getDescription()));
		tooltipText.append(" \n");
		tooltipText.append(va(itemCrystalTooltips[getCrystal()], getCrystalPower() * 100.0, getCrystalPower() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat1()], getBasicStat1Value() * 100.0, getBasicStat1Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat2()], getBasicStat2Value() * 100.0, getBasicStat2Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat3()], getBasicStat3Value() * 100.0, getBasicStat3Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod1Stat(modItemID1)], getMod1Value(modItemID1) * 100.0, getMod1Value(modItemID1) * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod2Stat(modItemID2)], getMod2Value(modItemID2) * 100.0, getMod2Value(modItemID2) * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod3Stat(modItemID3)], getMod3Value(modItemID3) * 100.0, getMod3Value(modItemID3) * 100.0));
		tooltipText.append(" \n");
		if (m_quantity > 1)
			tooltipText.append(va("^5Value: ^P%i (%i per item).\n", (int)getStackCost(modItemID1, modItemID2, modItemID3), (int)getCost(modItemID1, modItemID2, modItemID3)));
		else
			tooltipText.append(va("^5Value: ^P%i.\n", (int)getCost(modItemID1, modItemID2, modItemID3)));
		break;
	case IT_ITEM_MODIFICATION:
	case IT_ITEM_CRYSTAL:
		tooltipText = va("^B%s^b\n", itemName.c_str());
		tooltipText.append(va("^PItem %s\n", IT_ITEM_CRYSTAL ? "Crystal" : "Modification"));
		tooltipText.append(" \n");
		tooltipText.append(va("^5%s\n", getDescription()));
		tooltipText.append(" \n");
		tooltipText.append(va(itemCrystalTooltips[getCrystal()], getCrystalPower() * 100.0, getCrystalPower() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat1()], getBasicStat1Value() * 100.0, getBasicStat1Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat2()], getBasicStat2Value() * 100.0, getBasicStat2Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat3()], getBasicStat3Value() * 100.0, getBasicStat3Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod1Stat(modItemID1)], getMod1Value(modItemID1) * 100.0, getMod1Value(modItemID1) * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod2Stat(modItemID2)], getMod2Value(modItemID2) * 100.0, getMod2Value(modItemID2) * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod3Stat(modItemID3)], getMod3Value(modItemID3) * 100.0, getMod3Value(modItemID3) * 100.0));
		tooltipText.append(" \n");
		if (m_quantity > 1)
			tooltipText.append(va("^5Value: ^P%i (%i per item).\n", (int)getStackCost(modItemID1, modItemID2, modItemID3), (int)getCost(modItemID1, modItemID2, modItemID3)));
		else
			tooltipText.append(va("^5Value: ^P%i.\n", (int)getCost(modItemID1, modItemID2, modItemID3)));
		break;
	case IT_WEAPON_MODIFICATION:
	case IT_WEAPON_CRYSTAL:
		tooltipText = va("^B%s^b\n", itemName.c_str());
		tooltipText.append(va("^PWeapon %s\n", IT_WEAPON_CRYSTAL ? "Crystal" : "Modification"));
		tooltipText.append(" \n");
		tooltipText.append(va("^5%s\n", getDescription()));
		tooltipText.append(" \n");
		tooltipText.append(va(weaponCrystalTooltips[getCrystal()], getCrystalPower() * 100.0, getCrystalPower() * 100.0));
		tooltipText.append(va(weaponStat1Tooltips[getBasicStat1()], getBasicStat1Value() * 100.0, getBasicStat1Value() * 100.0));
		tooltipText.append(va(weaponStat2Tooltips[getBasicStat2()], getBasicStat2Value() * 100.0, getBasicStat2Value() * 100.0));
		tooltipText.append(va(weaponStat3Tooltips[getBasicStat3()], getBasicStat3Value() * 100.0, getBasicStat3Value() * 100.0));
		tooltipText.append(va(weaponStat1Tooltips[getMod1Stat(modItemID1)], getMod1Value(modItemID1) * 100.0, getMod1Value(modItemID1) * 100.0));
		tooltipText.append(va(weaponStat2Tooltips[getMod2Stat(modItemID2)], getMod2Value(modItemID2) * 100.0, getMod2Value(modItemID2) * 100.0));
		tooltipText.append(va(weaponStat3Tooltips[getMod3Stat(modItemID3)], getMod3Value(modItemID3) * 100.0, getMod3Value(modItemID3) * 100.0));
		tooltipText.append(" \n");
		if (m_quantity > 1)
			tooltipText.append(va("^5Value: ^P%i (%i per item).\n", (int)getStackCost(modItemID1, modItemID2, modItemID3), (int)getCost(modItemID1, modItemID2, modItemID3)));
		else
			tooltipText.append(va("^5Value: ^P%i.\n", (int)getCost(modItemID1, modItemID2, modItemID3)));
		break;
	case IT_SABER_MODIFICATION:
	case IT_SABER_CRYSTAL:
		tooltipText = va("^B%s^b\n", itemName.c_str());
		tooltipText.append(va("^PLightsaber %s\n", IT_SABER_CRYSTAL ? "Crystal" : "Modification"));
		tooltipText.append(" \n");
		tooltipText.append(va("^5%s\n", getDescription()));
		tooltipText.append(" \n");
		tooltipText.append(va(weaponCrystalTooltips[getCrystal()], getCrystalPower() * 100.0, getCrystalPower() * 100.0));
		tooltipText.append(va(saberStat1Tooltips[getBasicStat1()], getBasicStat1Value() * 100.0, getBasicStat1Value() * 100.0));
		tooltipText.append(va(saberStat2Tooltips[getBasicStat2()], getBasicStat2Value() * 100.0, getBasicStat2Value() * 100.0));
		tooltipText.append(va(saberStat3Tooltips[getBasicStat3()], getBasicStat3Value() * 100.0, getBasicStat3Value() * 100.0));
		tooltipText.append(va(saberStat1Tooltips[getMod1Stat(modItemID1)], getMod1Value(modItemID1) * 100.0, getMod1Value(modItemID1) * 100.0));
		tooltipText.append(va(saberStat2Tooltips[getMod2Stat(modItemID2)], getMod2Value(modItemID2) * 100.0, getMod2Value(modItemID2) * 100.0));
		tooltipText.append(va(saberStat3Tooltips[getMod3Stat(modItemID3)], getMod3Value(modItemID3) * 100.0, getMod3Value(modItemID3) * 100.0));
		tooltipText.append(" \n");
		if (m_quantity > 1)
			tooltipText.append(va("^5Value: ^P%i (%i per item).\n", (int)getStackCost(modItemID1, modItemID2, modItemID3), (int)getCost(modItemID1, modItemID2, modItemID3)));
		else
			tooltipText.append(va("^5Value: ^P%i.\n", (int)getCost(modItemID1, modItemID2, modItemID3)));
		break;
	case IT_WEAPON:
		if (getBaseItem()->giTag == WP_SABER)
		{
			tooltipText = va("^B%s^b\n", itemName.c_str());
			
			if (getModelType() == SABER_MODELTYPE_STAFF)
				tooltipText.append("^PTwo handed weapon, Lightsaber\n");
			else if (getModelType() == SABER_MODELTYPE_ELECTROSTAFF)
				tooltipText.append("^PTwo handed weapon, Electrostaff\n");
			else
				tooltipText.append("^POne handed weapon, Lightsaber\n");

			tooltipText.append(" \n");
			tooltipText.append(va("^5%s\n", getDescription()));
			tooltipText.append(" \n");
			tooltipText.append("^7Scaling Attribute: ^PStrength\n");
			tooltipText.append("^7Damage: ^P78-102 ^8(^P30.2 DPS^8).\n");
			tooltipText.append("^7Attacks per Second: ^P1.85\n");
			tooltipText.append("^7Crit Chance: ^P+9.5%\n");
			tooltipText.append("^7Crit Power: ^P+58.3%\n");
			tooltipText.append(" \n");
			tooltipText.append(va(weaponCrystalTooltips[getCrystal()], getCrystalPower() * 100.0, getCrystalPower() * 100.0));
			tooltipText.append(va(saberStat1Tooltips[getBasicStat1()], getBasicStat1Value() * 100.0, getBasicStat1Value() * 100.0));
			tooltipText.append(va(saberStat2Tooltips[getBasicStat2()], getBasicStat2Value() * 100.0, getBasicStat2Value() * 100.0));
			tooltipText.append(va(saberStat3Tooltips[getBasicStat3()], getBasicStat3Value() * 100.0, getBasicStat3Value() * 100.0));
			tooltipText.append(va(saberStat1Tooltips[getMod1Stat(modItemID1)], getMod1Value(modItemID1) * 100.0, getMod1Value(modItemID1) * 100.0));
			tooltipText.append(va(saberStat2Tooltips[getMod2Stat(modItemID2)], getMod2Value(modItemID2) * 100.0, getMod2Value(modItemID2) * 100.0));
			tooltipText.append(va(saberStat3Tooltips[getMod3Stat(modItemID3)], getMod3Value(modItemID3) * 100.0, getMod3Value(modItemID3) * 100.0));
			if (getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
			{
				if (getCustomType() == ITEM_CUSTOMIZATION_REFORGE)
				{
					tooltipText.append(" \n");
					tooltipText.append(va("^B%s^b\n", "Reforged Weapon"));
					tooltipText.append(va("^P+%.2f%% ^7bonus to all stats.\n", 1.15f));
				}
				else if (getCustomType() == ITEM_CUSTOMIZATION_UNIQUE)
				{
					tooltipText.append(" \n");
					tooltipText.append(va("^B%s^b\n", "Unique Weapon"));
					tooltipText.append(va("^P+%.2f%% ^7bonus to all stats.\n", 1.25f));
				}
			}
			tooltipText.append(" \n");
			if (m_quantity > 1)
				tooltipText.append(va("^5Value: ^P%i (%i per item).\n", (int)getStackCost(modItemID1, modItemID2, modItemID3), (int)getCost(modItemID1, modItemID2, modItemID3)));
			else
				tooltipText.append(va("^5Value: ^P%i.\n", (int)getCost(modItemID1, modItemID2, modItemID3)));
		}
		else
		{
			tooltipText = va("^B%s^b\n", itemName.c_str());
			switch (getVisualType1(modItemID1))
			{
			case WEAPON_STAT1_DEFAULT:
			default:
				tooltipText.append("^POne handed weapon, Pistol\n");
				break;
			case WEAPON_STAT1_HEAVY_PISTOL:
				tooltipText.append("^POne handed weapon, Heavy Pistol\n");
				break;
			case WEAPON_STAT1_FIRE_ACCURACY_MODIFIER:
				tooltipText.append("^PTwo handed weapon, Sniper Rifle\n");
				break;
			case WEAPON_STAT1_FIRE_RATE_MODIFIER:
				tooltipText.append("^PTwo handed weapon, Blaster Rifle\n");
				break;
			case WEAPON_STAT1_VELOCITY_MODIFIER:
				tooltipText.append("^PTwo handed weapon, Assault Rifle\n");
				break;
			case WEAPON_STAT1_HEAT_ACCUMULATION_MODIFIER:
				tooltipText.append("^PTwo handed weapon, Heavy Blaster\n");
				break;
			}
			
			tooltipText.append(" \n");
			tooltipText.append(va("^5%s\n", getDescription()));
			tooltipText.append(" \n");
			tooltipText.append("^7Scaling Attribute: ^PDexterity\n");
			tooltipText.append("^7Damage: ^P78-102 ^8(^P40.5 DPS^8).\n");
			tooltipText.append("^7Attacks per Second: ^P0.45\n");
			tooltipText.append("^7Crit Chance: ^P+11.5%\n");
			tooltipText.append("^7Crit Power: ^P+41.0%\n");
			tooltipText.append(" \n");
			tooltipText.append(va(weaponCrystalTooltips[getCrystal()], getCrystalPower() * 100.0, getCrystalPower() * 100.0));
			tooltipText.append(va(weaponStat1Tooltips[getBasicStat1()], getBasicStat1Value() * 100.0, getBasicStat1Value() * 100.0));
			tooltipText.append(va(weaponStat2Tooltips[getBasicStat2()], getBasicStat2Value() * 100.0, getBasicStat2Value() * 100.0));
			tooltipText.append(va(weaponStat1Tooltips[getMod1Stat(modItemID1)], getMod1Value(modItemID1) * 100.0, getMod1Value(modItemID1) * 100.0));
			tooltipText.append(va(weaponStat2Tooltips[getMod2Stat(modItemID2)], getMod2Value(modItemID2) * 100.0, getMod2Value(modItemID2) * 100.0));
			tooltipText.append(va(weaponStat3Tooltips[getBasicStat3()], getBasicStat3Value() * 100.0, getBasicStat3Value() * 100.0));
			tooltipText.append(va(weaponStat3Tooltips[getMod3Stat(modItemID3)], getMod3Value(modItemID3) * 100.0, getMod3Value(modItemID3) * 100.0));
			if (getCustomType() > ITEM_CUSTOMIZATION_DEFAULT)
			{
				if (getCustomType() == ITEM_CUSTOMIZATION_REFORGE)
				{
					tooltipText.append(" \n");
					tooltipText.append(va("^B%s^b\n", "Reforged Weapon"));
					tooltipText.append(va("^P+%.2f%% ^7bonus to all stats.\n", 1.15f));
				}
				else if (getCustomType() == ITEM_CUSTOMIZATION_UNIQUE)
				{
					tooltipText.append(" \n");
					tooltipText.append(va("^B%s^b\n", "Unique Weapon"));
					tooltipText.append(va("^P+%.2f%% ^7bonus to all stats.\n", 1.25f));
				}
			}
			tooltipText.append(" \n");
			if (m_quantity > 1)
				tooltipText.append(va("^5Value: ^P%i (%i per item).\n", (int)getStackCost(modItemID1, modItemID2, modItemID3), (int)getCost(modItemID1, modItemID2, modItemID3)));
			else
				tooltipText.append(va("^5Value: ^P%i.\n", (int)getCost(modItemID1, modItemID2, modItemID3)));
		}
		break;
	default:
		// hmm. shouldn't happen, hopefully...
		break;
	}
}

qboolean inventoryItem::isModification()
{
	uint16_t giType = getBaseItem()->giType;

	if (giType == IT_ITEM_MODIFICATION || giType == IT_WEAPON_MODIFICATION || giType == IT_SABER_MODIFICATION)
		return qtrue;

	return qfalse;
}

qboolean inventoryItem::isCrystal()
{
	uint16_t giType = getBaseItem()->giType;

	if (giType == IT_SABER_CRYSTAL || giType == IT_WEAPON_CRYSTAL || giType == IT_ITEM_CRYSTAL)
		return qtrue;

	return qfalse;
}

