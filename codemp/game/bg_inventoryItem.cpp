#ifdef _GAME
#include "g_local.h"
#endif

#ifdef _CGAME
#include "../cgame/cg_local.h"
#endif


#if defined(rd_warzone_x86_EXPORTS)
#include "tr_local.h"
#endif

//
// Configuration Defines...
//



//
// A quality based price scale modifier... Used internally... Matches levels of itemQuality_t.
//
float qualityPriceModifier[5] = {
	1.0,
	2.0,
	4.0,
	8.0,
	16.0
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
	"^P+%.1f% ^Nheat damage.\n",
	"^P+%.1f% ^2kinetic damage.\n",
	"^P+%.1f% ^4electric damage.\n",
	"^P+%.1f% ^7cold damage.\n",
	"^P+%.1f% ^Nheat^3, and ^P%.1f% ^2kinetic ^3damage.\n",
	"^P+%.1f% ^4electric^0, and ^P%.1f% ^Nheat ^0damage.\n",
	"^P+%.1f% ^7cold^9, and ^P%.1f% ^2kinetic ^9damage.\n",
	"^P+%.1f% ^Nheat^6, and ^P%.1f% ^7cold ^6damage.\n",
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
	"^P+%.1f% ^Nresistance to heat damage.\n",
	"^P+%.1f% ^3resistance to kinetic damage.\n",
	"^P+%.1f% ^4resistance to electric damage.\n",
	"^P+%.1f% ^7resistance to cold damage.\n",
	"^P+%.1f% ^3resistance to ^Nheat^3, and ^P+%.1f% ^2kinetic ^3damage.\n",
	"^P+%.1f% ^0resistance to ^4electric^0, and ^P+%.1f% ^Nheat ^0damage.\n",
	"^P+%.1f% ^9resistance to ^7cold^9, and ^P+%.1f% ^2kinetic ^9damage.\n",
	"^P+%.1f% ^6resistance to ^Nheat^6, and ^P+%.1f% ^7cold ^6damage.\n",
};

const char *weaponStat1Names[] = {
	"",								// Pistol
	"Accurate ",					// Sniper Rifle
	"Quickfire ",					// Blaster Rifle
	"Fast ",						// Assault Rifle
	"Cooled ",						// Heavy Blaster
};

const char *weaponStat1Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to accuracy.\n",
	"^P+%.1f% ^7bonus to rate of fire.\n",
	"^P+%.1f% ^7bonus to velocity.\n",
	"^P+%.1f% ^7reduction to heat accumulation.\n",
};

const char *weaponStat2Names[] = {
	"",
	"Deadly ",
	"Pinpointing ",
	"Charged ",
};

const char *weaponStat2Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to damage.\n",
	"^P+%.1f% ^7bonus to critical chance.\n",
	"^P+%.1f% ^7bonus to critical power.\n",
};

const char *weaponStat3Names[] = {
	"",
	"Ricochet ",
	"Explosive ",
	"Beam ",
	"Expanded ",
};

const char *weaponStat3Tooltips[] = {
	"",
	"^PRicochet.\n",
	"^PExplosive.\n",
	"^PBeam.\n",
	"^PWide Shot.\n",
};

const char *saberStat1Names[] = {
	"",
	"Parrying ",
	"Reflecting ",
	"Staggerring ",
};

const char *saberStat1Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to melee blocking chance.\n",
	"^P+%.1f% ^7bonus to ranged blocking chance.\n",
	"^P+%.1f% ^7bonus to stagger chance.\n",
};

const char *saberStat2Names[] = {
	"",
	"Deadly ",
	"Pinpointing ",
	"Charged ",
	"Penetrating ",
	"Vampyric ",
	"Leeching ",
};

const char *saberStat2Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to damage.\n",
	"^P+%.1f% ^7bonus to critical chance.\n",
	"^P+%.1f% ^7bonus to critical power.\n",
	"^P+%.1f% ^7bonus shield penetration.\n",
	"^P+%.1f% ^7bonus health drain.\n",
	"^P+%.1f% ^7bonus power drain.\n",
};

const char *saberStat3Names[] = {
	"",
	"Long ",
	"Fast ",
};

const char *saberStat3Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to blade length.\n",
	"^P+%.1f% ^7bonus to attack speed.\n",
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
	"^P+%.1f% ^7bonus to maximum health.\n",
	"^P+%.1f% ^7bonus to health regeneration.\n",
	"^P+%.1f% ^7bonus to maximum shields.\n",
	"^P+%.1f% ^7bonus to shield regeneration.\n",
	"^P+%.1f% ^7bonus to maximum force power.\n",
	"^P+%.1f% ^7bonus to force power regeneration.\n",
	"^P+%.1f% ^7bonus to strength.\n",
	"^P+%.1f% ^7bonus to evasion.\n",
	"^P+%.1f% ^7bonus to speed.\n",
	"^P+%.1f% ^7bonus to speed and %.1f% bonus to evasion.\n",
	"^P+%.1f% ^7bonus to blocking.\n",
	"^P+%.1f% ^7damage reduction.\n",
	"^P+%.1f% ^7shield penetration reduction.\n",
};

//
// Construction/Destruction...
//
inventoryItem::inventoryItem(int itemID)
{
	m_itemID = itemID;
	m_bgItemID = 0;
	m_quality = QUALITY_GREY;
	m_quantity = 0;

#if defined(_CGAME) || defined(rd_warzone_x86_EXPORTS)
	memset(&m_model, 0, sizeof(m_model));
	memset(&m_icon, 0, sizeof(m_icon));
#endif

	m_transmitted = qfalse;

	m_destroyTime = -1;

	m_crystal = 0;
	m_basicStat1 = 0;
	m_basicStat2 = 0;
	m_basicStat3 = 0;
	m_basicStat1value = 0.0;
	m_basicStat2value = 0.0;
	m_basicStat3value = 0.0;
	m_modStat1 = 0;
	m_modStat2 = 0;
	m_modStat3 = 0;
	m_modStatValue1 = 0.0;
	m_modStatValue2 = 0.0;
	m_modStatValue3 = 0.0;
}

inventoryItem::inventoryItem(int itemID, int bgItemID, itemQuality_t quality, int amount = 1, int destroyTime = -1)
{
	m_itemID = itemID;
	m_bgItemID = bgItemID;
	m_quality = quality;
	m_quantity = amount;

#if defined(_CGAME) || defined(rd_warzone_x86_EXPORTS)
	memset(&m_model, 0, sizeof(m_model));
	if (bg_itemlist[m_bgItemID].view_model && bg_itemlist[m_bgItemID].view_model[0])
	{
		strcpy(m_model, bg_itemlist[m_bgItemID].view_model);
	}
	memset(&m_icon, 0, sizeof(m_icon));
	if (bg_itemlist[m_bgItemID].icon && bg_itemlist[m_bgItemID].icon[0])
	{
		strcpy(m_icon, bg_itemlist[m_bgItemID].icon);
	}
#endif

	m_transmitted = qfalse;

	m_crystal = 0;
	m_basicStat1 = 0;
	m_basicStat2 = 0;
	m_basicStat3 = 0;
	m_basicStat1value = 0.0;
	m_basicStat2value = 0.0;
	m_basicStat3value = 0.0;
	m_modStat1 = 0;
	m_modStat2 = 0;
	m_modStat3 = 0;
	m_modStatValue1 = 0.0;
	m_modStatValue2 = 0.0;
	m_modStatValue3 = 0.0;

	m_destroyTime = destroyTime;
}

inventoryItem::~inventoryItem()
{

}

//
// Item Setup Functions...
//
void inventoryItem::setItemID(int itemID)
{
	m_itemID = itemID;
}

void inventoryItem::setBaseItem(int bgItemID)
{
	m_bgItemID = bgItemID;
}

void inventoryItem::setQuality(itemQuality_t quality)
{
	m_quality = quality;
}

void inventoryItem::setQuantity(int amount)
{
	m_quantity = amount;
}

void inventoryItem::setDestroyTime(int destroyTime)
{
	m_destroyTime = destroyTime;
}

void inventoryItem::setCrystal(int crystalType)
{
	m_crystal = crystalType;
}

void inventoryItem::setStat1(int statType, float statValue)
{
	if (!isModification() && m_quality <= QUALITY_GREY) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat2 || m_basicStat3)) return; // mods can only have 1 stat type.
	
	m_basicStat1 = statType;
	m_basicStat1value = statValue;
}

void inventoryItem::setStat2(int statType, float statValue)
{
	if (!isModification() && m_quality <= QUALITY_WHITE) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat1 || m_basicStat3)) return; // mods can only have 1 stat type.

	m_basicStat2 = statType;
	m_basicStat2value = statValue;
}

void inventoryItem::setStat3(int statType, float statValue)
{
	if (!isModification() && m_quality <= QUALITY_GREEN) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat1 || m_basicStat2)) return; // mods can only have 1 stat type.

	m_basicStat3 = statType;
	m_basicStat3value = statValue;
}

void inventoryItem::setMod1(int statType, float statValue)
{
	if (m_quality <= QUALITY_BLUE) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification()) return; // mods can't have mods :)

	m_modStat1 = statType;
	m_modStatValue1 = statValue;
}

void inventoryItem::setMod2(int statType, float statValue)
{
	if (m_quality <= QUALITY_PURPLE) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification()) return; // mods can't have mods :)

	m_modStat2 = statType;
	m_modStatValue2 = statValue;
}

void inventoryItem::setMod3(int statType, float statValue)
{
	if (m_quality <= QUALITY_ORANGE) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification()) return; // mods can't have mods :)

	m_modStat3 = statType;
	m_modStatValue3 = statValue;
}

//
// Item Accessing Functions...
//
int inventoryItem::getItemID()
{
	return m_itemID;
}

gitem_t *inventoryItem::getBaseItem()
{
	return &bg_itemlist[m_bgItemID];
}

int inventoryItem::getBaseItemID()
{
	return m_bgItemID;
}

const char *inventoryItem::getName()
{
	//return getBaseItem()->name;
	int giType = getBaseItem()->giType;

	static std::string tooltipText;
	tooltipText.clear();

	switch (giType)
	{
	case IT_WEARABLE:
		tooltipText = va("%s%s%s%s%s (%s)", getColorStringForQuality(), itemStatNames[getBasicStat1()], itemStatNames[getBasicStat2()], itemStatNames[getBasicStat3()], getBaseItem()->name, itemCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		break;
	case IT_ITEM_MODIFICATION:
		tooltipText = va("%s%s%s%s%s (%s)", getColorStringForQuality(), itemStatNames[getBasicStat1()], itemStatNames[getBasicStat2()], itemStatNames[getBasicStat3()], getBaseItem()->name, itemQualityTooltips[getQuality()]);
		break;
	case IT_ITEM_CRYSTAL:
		tooltipText = va("%s%s%s (%s)", getColorStringForQuality(), getBaseItem()->name, itemCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		break;
	case IT_WEAPON_MODIFICATION:
		tooltipText = va("%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], getBaseItem()->name, itemQualityTooltips[getQuality()]);
		break;
	case IT_WEAPON_CRYSTAL:
		tooltipText = va("%s%s%s (%s)", getColorStringForQuality(), getBaseItem()->name, weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		break;
	case IT_SABER_MODIFICATION:
		tooltipText = va("%s%s%s%s%s (%s)", getColorStringForQuality(), saberStat1Names[getBasicStat1()], saberStat2Names[getBasicStat2()], saberStat3Names[getBasicStat3()], getBaseItem()->name, itemQualityTooltips[getQuality()]);
		break;
	case IT_SABER_CRYSTAL:
		tooltipText = va("%s%s%s (%s)", getColorStringForQuality(), getBaseItem()->name, weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		break;
	case IT_WEAPON:
		if (getBaseItem()->giTag == WP_SABER)
		{
			tooltipText = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), saberStat1Names[getBasicStat1()], saberStat2Names[getBasicStat2()], saberStat3Names[getBasicStat3()], getBaseItem()->name, weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
		}
		else
		{
			switch (getBasicStat1())
			{
			case WEAPON_STAT1_DEFAULT:
			default:
				tooltipText = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Pistol", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_FIRE_ACCURACY_MODIFIER:
				tooltipText = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Sniper Rifle", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_FIRE_RATE_MODIFIER:
				tooltipText = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Blaster Rifle", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_VELOCITY_MODIFIER:
				tooltipText = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Assault Rifle", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			case WEAPON_STAT1_HEAT_ACCUMULATION_MODIFIER:
				tooltipText = va("%s%s%s%s%s%s (%s)", getColorStringForQuality(), weaponStat1Names[getBasicStat1()], weaponStat2Names[getBasicStat2()], weaponStat3Names[getBasicStat3()], "Heavy Blaster", weaponCrystalNames[getCrystal()], itemQualityTooltips[getQuality()]);
				break;
			}
		}
		break;
	default:
		// hmm. shouldn't happen, hopefully...
		break;
	}

	return tooltipText.c_str();
}

char *inventoryItem::getDescription()
{
	return getBaseItem()->description;
}

char *inventoryItem::getModel()
{
#if defined(_CGAME) || defined(rd_warzone_x86_EXPORTS)
	return m_model;
#else
	return "";
#endif
}

char *inventoryItem::getIcon()
{
#if defined(_CGAME) || defined(rd_warzone_x86_EXPORTS)
	return m_icon;
#else
	return "";
#endif
}

itemQuality_t inventoryItem::getQuality()
{
	return m_quality;
}

int inventoryItem::getQuantity()
{
	return m_quantity;
}

int inventoryItem::getBasicStat1()
{
	return m_basicStat1;
}

int inventoryItem::getBasicStat2()
{
	return m_basicStat2;
}

int inventoryItem::getBasicStat3()
{
	return m_basicStat3;
}

float inventoryItem::getBasicStat1Value()
{
	return m_basicStat1value;
}

float inventoryItem::getBasicStat2Value()
{
	return m_basicStat2value;
}

float inventoryItem::getBasicStat3Value()
{
	return m_basicStat3value;
}

int inventoryItem::getCrystal()
{
	return m_crystal;
}

int inventoryItem::getMod1Stat()
{
	return m_modStat1;
}

int inventoryItem::getMod2Stat()
{
	return m_modStat2;
}

int inventoryItem::getMod3Stat()
{
	return m_modStat3;
}

float inventoryItem::getMod1Value()
{
	return m_modStatValue1;
}

float inventoryItem::getMod2Value()
{
	return m_modStatValue2;
}

float inventoryItem::getMod3Value()
{
	return m_modStatValue3;
}

double inventoryItem::getCost()
{// Apply multipliers based on how many extra stats this item has...
	double crystalCostMultiplier = getCrystal() ? 1.5 : 0.0;
	double statCostMultiplier1 = getBasicStat1() ? 1.25 : 0.0;
	double statCostMultiplier2 = getBasicStat1() ? 1.25 : 0.0;
	double statCostMultiplier3 = getBasicStat1() ? 1.25 : 0.0;
	double modCostMultiplier1 = getMod1Stat() ? 1.25 : 0.0;
	double modCostMultiplier2 = getMod2Stat() ? 1.25 : 0.0;
	double modCostMultiplier3 = getMod3Stat() ? 1.25 : 0.0;
	return getBaseItem()->price * statCostMultiplier1 * statCostMultiplier2 * statCostMultiplier3 * modCostMultiplier1 * modCostMultiplier2 * modCostMultiplier3 * crystalCostMultiplier * qualityPriceModifier[m_quality];
}

double inventoryItem::getStackCost()
{// Apply multipliers based on how many extra stats this item has...
	double crystalCostMultiplier = getCrystal() ? 1.5 : 0.0;
	double statCostMultiplier1 = getBasicStat1() ? 1.25 : 0.0;
	double statCostMultiplier2 = getBasicStat1() ? 1.25 : 0.0;
	double statCostMultiplier3 = getBasicStat1() ? 1.25 : 0.0;
	double modCostMultiplier1 = getMod1Stat() ? 1.25 : 0.0;
	double modCostMultiplier2 = getMod2Stat() ? 1.25 : 0.0;
	double modCostMultiplier3 = getMod3Stat() ? 1.25 : 0.0;
	return getBaseItem()->price * m_quantity * statCostMultiplier1 * statCostMultiplier2 * statCostMultiplier3 * modCostMultiplier1 * modCostMultiplier2 * modCostMultiplier3 * crystalCostMultiplier * qualityPriceModifier[m_quality];
}

int inventoryItem::getDestroyTime()
{
	return m_destroyTime;
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

const char *inventoryItem::getTooltip()
{
	int giType = getBaseItem()->giType;
	
	static std::string tooltipText;
	tooltipText.clear();

	switch (giType)
	{
	case IT_WEARABLE:
		tooltipText = va("^B%s^b\n", getName());
		tooltipText.append("^PClothing\n");
		tooltipText.append(" \n");
		tooltipText.append(va("^5%s\n", getDescription()));
		tooltipText.append(" \n");
		tooltipText.append(itemCrystalTooltips[getCrystal()]);
		tooltipText.append(va(itemStatTooltips[getBasicStat1()], getBasicStat1Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat2()], getBasicStat2Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat3()], getBasicStat3Value() * 100.0));
		tooltipText.append(" \n");
		tooltipText.append(va(itemStatTooltips[getMod1Stat()], getMod1Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod2Stat()], getMod2Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod3Stat()], getMod3Value() * 100.0));
		tooltipText.append(" \n");
		tooltipText.append(va("^5Value: ^P%i.\n", getStackCost()));
		break;
	case IT_ITEM_MODIFICATION:
	case IT_ITEM_CRYSTAL:
		tooltipText = va("^B%s^b\n", getName());
		tooltipText.append(va("^PItem %s\n", IT_ITEM_CRYSTAL ? "Crystal" : "Modification"));
		tooltipText.append(" \n");
		tooltipText.append(va("^5%s\n", getDescription()));
		tooltipText.append(" \n");
		tooltipText.append(itemCrystalTooltips[getCrystal()]);
		tooltipText.append(va(itemStatTooltips[getBasicStat1()], getBasicStat1Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat2()], getBasicStat2Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getBasicStat3()], getBasicStat3Value() * 100.0));
		tooltipText.append(" \n");
		tooltipText.append(va(itemStatTooltips[getMod1Stat()], getMod1Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod2Stat()], getMod2Value() * 100.0));
		tooltipText.append(va(itemStatTooltips[getMod3Stat()], getMod3Value() * 100.0));
		tooltipText.append(" \n");
		tooltipText.append(va("^5Value: ^P%i.\n", getStackCost()));
		break;
	case IT_WEAPON_MODIFICATION:
	case IT_WEAPON_CRYSTAL:
		tooltipText = va("^B%s^b\n", getName());
		tooltipText.append(va("^PWeapon %s\n", IT_WEAPON_CRYSTAL ? "Crystal" : "Modification"));
		tooltipText.append(" \n");
		tooltipText.append(va("^5%s\n", getDescription()));
		tooltipText.append(" \n");
		tooltipText.append(weaponCrystalTooltips[getCrystal()]);
		tooltipText.append(va(weaponStat1Tooltips[getBasicStat1()], getBasicStat1Value() * 100.0));
		tooltipText.append(va(weaponStat2Tooltips[getBasicStat2()], getBasicStat2Value() * 100.0));
		tooltipText.append(va(weaponStat3Tooltips[getBasicStat3()], getBasicStat3Value() * 100.0));
		tooltipText.append(" \n");
		tooltipText.append(va(weaponStat1Tooltips[getMod1Stat()], getMod1Value() * 100.0));
		tooltipText.append(va(weaponStat2Tooltips[getMod2Stat()], getMod2Value() * 100.0));
		tooltipText.append(va(weaponStat3Tooltips[getMod3Stat()], getMod3Value() * 100.0));
		tooltipText.append(" \n");
		tooltipText.append(va("^5Value: ^P%i.\n", getStackCost()));
		break;
	case IT_SABER_MODIFICATION:
	case IT_SABER_CRYSTAL:
		tooltipText = va("^B%s^b\n", getName());
		tooltipText.append(va("^PLightsaber %s\n", IT_SABER_CRYSTAL ? "Crystal" : "Modification"));
		tooltipText.append(" \n");
		tooltipText.append(va("^5%s\n", getDescription()));
		tooltipText.append(" \n");
		tooltipText.append(weaponCrystalTooltips[getCrystal()]);
		tooltipText.append(va(saberStat1Tooltips[getBasicStat1()], getBasicStat1Value() * 100.0));
		tooltipText.append(va(saberStat2Tooltips[getBasicStat2()], getBasicStat2Value() * 100.0));
		tooltipText.append(va(saberStat3Tooltips[getBasicStat3()], getBasicStat3Value() * 100.0));
		tooltipText.append(" \n");
		tooltipText.append(va(saberStat1Tooltips[getMod1Stat()], getMod1Value() * 100.0));
		tooltipText.append(va(saberStat2Tooltips[getMod2Stat()], getMod2Value() * 100.0));
		tooltipText.append(va(saberStat3Tooltips[getMod3Stat()], getMod3Value() * 100.0));
		tooltipText.append(" \n");
		tooltipText.append(va("^5Value: ^P%i.\n", getStackCost()));
		break;
	case IT_WEAPON:
		if (getBaseItem()->giTag == WP_SABER)
		{
			tooltipText = va("^B%s^b\n", getName());
			tooltipText.append("^POne handed weapon, Lightsaber\n");
			tooltipText.append(" \n");
			tooltipText.append(va("^5%s\n", getDescription()));
			tooltipText.append(" \n");
			tooltipText.append("^7Scaling Attribute: ^PStrength\n");
			tooltipText.append("^7Damage: ^P78-102 ^8(^P40.5 DPS^8).\n");
			tooltipText.append("^7Attacks per Second: ^P0.45\n");
			tooltipText.append("^7Crit Chance: ^P+11.5%\n");
			tooltipText.append("^7Crit Power: ^P+41.0%\n");
			tooltipText.append(" \n");
			tooltipText.append(weaponCrystalTooltips[getCrystal()]);
			tooltipText.append(va(saberStat1Tooltips[getBasicStat1()], getBasicStat1Value() * 100.0));
			tooltipText.append(va(saberStat2Tooltips[getBasicStat2()], getBasicStat2Value() * 100.0));
			tooltipText.append(va(saberStat3Tooltips[getBasicStat3()], getBasicStat3Value() * 100.0));
			tooltipText.append(" \n");
			tooltipText.append(va(saberStat1Tooltips[getMod1Stat()], getMod1Value() * 100.0));
			tooltipText.append(va(saberStat2Tooltips[getMod2Stat()], getMod2Value() * 100.0));
			tooltipText.append(va(saberStat3Tooltips[getMod3Stat()], getMod3Value() * 100.0));
			tooltipText.append(" \n");
			tooltipText.append(va("^5Value: ^P%i.\n", getStackCost()));
		}
		else
		{
			tooltipText = va("^B%s^b\n", getName());
			tooltipText.append("^POne handed weapon, Gun\n");
			tooltipText.append(" \n");
			tooltipText.append(va("^5%s\n", getDescription()));
			tooltipText.append(" \n");
			tooltipText.append("^7Scaling Attribute: ^PStrength\n");
			tooltipText.append("^7Damage: ^P78-102 ^8(^P40.5 DPS^8).\n");
			tooltipText.append("^7Attacks per Second: ^P0.45\n");
			tooltipText.append("^7Crit Chance: ^P+11.5%\n");
			tooltipText.append("^7Crit Power: ^P+41.0%\n");
			tooltipText.append(" \n");
			tooltipText.append(weaponCrystalTooltips[getCrystal()]);
			tooltipText.append(va(weaponStat1Tooltips[getBasicStat1()], getBasicStat1Value() * 100.0));
			tooltipText.append(va(weaponStat2Tooltips[getBasicStat2()], getBasicStat2Value() * 100.0));
			tooltipText.append(va(weaponStat3Tooltips[getBasicStat3()], getBasicStat3Value() * 100.0));
			tooltipText.append(" \n");
			tooltipText.append(va(weaponStat1Tooltips[getMod1Stat()], getMod1Value() * 100.0));
			tooltipText.append(va(weaponStat2Tooltips[getMod2Stat()], getMod2Value() * 100.0));
			tooltipText.append(va(weaponStat3Tooltips[getMod3Stat()], getMod3Value() * 100.0));
			tooltipText.append(" \n");
			tooltipText.append(va("^5Value: ^P%i.\n", getStackCost()));
		}
		break;
	default:
		// hmm. shouldn't happen, hopefully...
		break;
	}

	return tooltipText.c_str();
}

qboolean inventoryItem::isModification()
{
	int giType = getBaseItem()->giType;

	if (giType == IT_ITEM_MODIFICATION || giType == IT_WEAPON_MODIFICATION || giType == IT_SABER_MODIFICATION)
		return qtrue;

	return qfalse;
}

qboolean inventoryItem::isCrystal()
{
	int giType = getBaseItem()->giType;

	if (giType == IT_SABER_CRYSTAL || giType == IT_WEAPON_CRYSTAL || giType == IT_ITEM_CRYSTAL)
		return qtrue;

	return qfalse;
}

