#ifdef _GAME
#include "g_local.h"
#endif

#ifdef _CGAME
#include "../cgame/cg_local.h"
#endif

#ifdef _GAME
//
// Global Stuff... This is for the server to keep track of unique item instances.
//
int					INVENTORY_ITEM_INSTANCES_COUNT = 0;	// TODO: This should be initialized to database's highest value at map load... Or even better, always store the max id to it's own DB field.
inventoryItem		*INVENTORY_ITEM_INSTANCES[8388608];
#endif

//
// Configuration Defines...
//

//#define __DATABASE_ENABLED__


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

const char *weaponStat1Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to accuracy.\n",
	"^P+%.1f% ^7bonus to rate of fire.\n",
	"^P+%.1f% ^7bonus to velocity.\n",
	"^P+%.1f% ^7reduction to heat accumulation.\n",
};

const char *weaponStat2Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to damage.\n",
	"^P+%.1f% ^7bonus to critical chance.\n",
	"^P+%.1f% ^7bonus to critical power.\n",
};

const char *weaponStat3Tooltips[] = {
	"",
	"^PRicochet.\n",
	"^PExplosive.\n",
	"^PBeam.\n",
	"^PWide Shot.\n",
};

const char *saberStat1Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to melee blocking chance.\n",
	"^P+%.1f% ^7bonus to ranged blocking chance.\n",
	"^P+%.1f% ^7bonus to stagger chance.\n",
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

const char *saberStat3Tooltips[] = {
	"",
	"^P+%.1f% ^7bonus to blade length.\n",
	"^P+%.1f% ^7bonus to attack speed.\n",
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
inventoryItem::inventoryItem()
{
#ifdef _GAME
	m_itemID = INVENTORY_ITEM_INSTANCES_COUNT;
#endif
	m_bgItemID = 0;
	m_quality = QUALITY_GREY;
	m_quantity = 0;
	m_playerID = -1;

	m_icon = NULL;

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

#ifdef _GAME
	INVENTORY_ITEM_INSTANCES[INVENTORY_ITEM_INSTANCES_COUNT] = this;
	INVENTORY_ITEM_INSTANCES_COUNT++;
#endif
}

inventoryItem::inventoryItem(int bgItemID, itemQuality_t quality, int amount = 1, int destroyTime = -1)
{
#ifdef _GAME
	m_itemID = INVENTORY_ITEM_INSTANCES_COUNT;
#endif

	m_bgItemID = bgItemID;
	m_quality = quality;
	m_quantity = amount;
	m_playerID = -1;

	m_icon = NULL;

#ifdef _CGAME
	m_icon = trap->R_RegisterShader(bg_itemlist[m_bgItemID].icon);
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

#ifdef _GAME
	INVENTORY_ITEM_INSTANCES[INVENTORY_ITEM_INSTANCES_COUNT] = this;
	INVENTORY_ITEM_INSTANCES_COUNT++;
#endif
}

inventoryItem::inventoryItem(int itemID)
{
#ifdef _GAME
	m_itemID = itemID;
#endif
	m_bgItemID = 0;
	m_quality = QUALITY_GREY;
	m_quantity = 0;
	m_playerID = -1;

	m_icon = NULL;

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

#ifdef _GAME
	INVENTORY_ITEM_INSTANCES[INVENTORY_ITEM_INSTANCES_COUNT] = this;
	INVENTORY_ITEM_INSTANCES_COUNT++;
#endif
}

inventoryItem::~inventoryItem()
{

}

#ifdef _CGAME
inventoryItem *createInventoryItem(int bgItemID, itemQuality_t quality, int amount)
{
	inventoryItem *item = new inventoryItem(bgItemID, quality, amount);

	clientInfo_t *client = &cgs.clientinfo[cg.clientNum];
	client->inventory[client->inventoryCount] = item;

	return item;
}
#endif

#ifdef _GAME
inventoryItem *createInventoryItem(int bgItemID, itemQuality_t quality, int amount, int clientNum, int destroyTime)
{
	inventoryItem *item = new inventoryItem(bgItemID, quality, amount, destroyTime);

	if (clientNum >= 0)
	{
		gclient_t *client = g_entities[clientNum].client;
		client->inventory[client->inventoryCount] = item;
		item->setDestroyTime(-1); // Don't time out and destroy stuff in a player's inventory...
	}

	return item;
}
#endif

void destroyInventoryItem(inventoryItem *item)
{
	delete[] item;
}

#ifdef _GAME
void destroyOldLootItems(void)
{// Call this periodically to remove items players never looted after they expire... (only needed if we ever decide to apply inventory item info and mods pre-looting).
	for (int i = 0; i < INVENTORY_ITEM_INSTANCES_COUNT; i++)
	{
		inventoryItem *item = INVENTORY_ITEM_INSTANCES[i];

		if (item)
		{
			int destroyTime = item->getDestroyTime();

			if (destroyTime >= 0 && destroyTime >= level.time)
			{
				destroyInventoryItem(item);
			}
		}
	}
}
#endif

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

void inventoryItem::setPlayerID(int playerID)
{
	m_playerID = playerID;
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
	if (m_quality <= QUALITY_GREY) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat2 || m_basicStat3)) return; // mods can only have 1 stat type.
	
	m_basicStat1 = statType;
	m_basicStat1value = statValue;
}

void inventoryItem::setStat2(int statType, float statValue)
{
	if (m_quality <= QUALITY_WHITE) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat1 || m_basicStat3)) return; // mods can only have 1 stat type.

	m_basicStat2 = statType;
	m_basicStat2value = statValue;
}

void inventoryItem::setStat3(int statType, float statValue)
{
	if (m_quality <= QUALITY_GREEN) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification() && (m_basicStat1 || m_basicStat2)) return; // mods can only have 1 stat type.

	m_basicStat3 = statType;
	m_basicStat3value = statValue;
}

void inventoryItem::setMod1(int statType, float statValue)
{
	if (m_quality <= QUALITY_GREEN) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification()) return; // mods can't have mods :)

	m_modStat1 = statType;
	m_modStatValue1 = statValue;
}

void inventoryItem::setMod2(int statType, float statValue)
{
	if (m_quality <= QUALITY_BLUE) return; // Not available...
	if (isCrystal()) return; // crystals can not have stats.
	if (isModification()) return; // mods can't have mods :)

	m_modStat2 = statType;
	m_modStatValue2 = statValue;
}

void inventoryItem::setMod3(int statType, float statValue)
{
	if (m_quality <= QUALITY_PURPLE) return; // Not available...
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

char *inventoryItem::getName()
{
	return getBaseItem()->name;
}

char *inventoryItem::getDescription()
{
	return getBaseItem()->description;
}

qhandle_t inventoryItem::getIcon()
{
	return m_icon;
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

int inventoryItem::getPlayerID()
{
	return m_playerID;
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
	
	std::string tooltipText = "";

	/*
	sprintf(tooltipString, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
			, va("%s^BMy schwartz is bigger than yours (%s)^b\n", ColorStringForQuality(tooltipQuality), itemQualityNames[tooltipQuality])
			, "^POne handed weapon, Lightsaber\n"
			, " \n"
			, "^7Scaling Attribute: ^PStrength\n"
			, "^7Damage: ^P78-102 ^8(^P40.5 DPS^8).\n"
			, "^7Attacks per Second: ^P0.45\n"
			, "^7Crit Chance: ^P+11.5%\n"
			, "^7Crit Power: ^P+41.0%\n"
			, " \n"
			, "^0Purple Crystal: ^P+12.0% ^4electric^0, and ^P+12.0% ^Nheat ^0damage.\n"
			, "^N-42.0% ^7Dexterity.\n"
			, "^N-97.0% ^7Intelligence.\n"
			, "^N+33.0% ^7Weight.\n"
			, " \n"
			, "^P+15.0% ^2bonus to trip over your own feet.\n"
			, "^P+50.0% ^2bonus to asking dumb questions.\n"
			, "^P+20.0% ^2bonus to epeen trolling.\n"
			, " \n"
			, "^5Value: Priceless.\n");
	*/

	switch (giType)
	{
	case IT_WEARABLE:
		tooltipText += va("%s^B%s (%s)^b\n", getColorStringForQuality(), getName(), itemQualityTooltips[getQuality()]);
		tooltipText += "^PClothing\n";
		tooltipText += " \n";
		tooltipText += va("^5%s\n", getDescription());
		tooltipText += " \n";
		tooltipText += itemCrystalTooltips[getCrystal()];
		tooltipText += va(itemStatTooltips[getBasicStat1()], getBasicStat1Value());
		tooltipText += va(itemStatTooltips[getBasicStat2()], getBasicStat2Value());
		tooltipText += va(itemStatTooltips[getBasicStat3()], getBasicStat3Value());
		tooltipText += " \n";
		tooltipText += va(itemStatTooltips[getMod1Stat()], getMod1Value());
		tooltipText += va(itemStatTooltips[getMod2Stat()], getMod2Value());
		tooltipText += va(itemStatTooltips[getMod3Stat()], getMod3Value());
		tooltipText += " \n";
		tooltipText += va("^5Value: ^P%i.\n", getStackCost());
		break;
	case IT_ITEM_MODIFICATION:
	case IT_ITEM_CRYSTAL:
		tooltipText += va("%s^B%s (%s)^b\n", getColorStringForQuality(), getName(), itemQualityTooltips[getQuality()]);
		tooltipText += va("^PItem %s\n", IT_ITEM_CRYSTAL ? "Crystal" : "Modification");
		tooltipText += " \n";
		tooltipText += va("^5%s\n", getDescription());
		tooltipText += " \n";
		tooltipText += itemCrystalTooltips[getCrystal()];
		tooltipText += va(itemStatTooltips[getBasicStat1()], getBasicStat1Value());
		tooltipText += va(itemStatTooltips[getBasicStat2()], getBasicStat2Value());
		tooltipText += va(itemStatTooltips[getBasicStat3()], getBasicStat3Value());
		tooltipText += " \n";
		tooltipText += va(itemStatTooltips[getMod1Stat()], getMod1Value());
		tooltipText += va(itemStatTooltips[getMod2Stat()], getMod2Value());
		tooltipText += va(itemStatTooltips[getMod3Stat()], getMod3Value());
		tooltipText += " \n";
		tooltipText += va("^5Value: ^P%i.\n", getStackCost());
		break;
	case IT_WEAPON_MODIFICATION:
	case IT_WEAPON_CRYSTAL:
		tooltipText += va("%s^B%s (%s)^b\n", getColorStringForQuality(), getName(), itemQualityTooltips[getQuality()]);
		tooltipText += va("^PWeapon %s\n", IT_WEAPON_CRYSTAL ? "Crystal" : "Modification");
		tooltipText += " \n";
		tooltipText += va("^5%s\n", getDescription());
		tooltipText += " \n";
		tooltipText += weaponCrystalTooltips[getCrystal()];
		tooltipText += va(weaponStat1Tooltips[getBasicStat1()], getBasicStat1Value());
		tooltipText += va(weaponStat2Tooltips[getBasicStat2()], getBasicStat2Value());
		tooltipText += va(weaponStat3Tooltips[getBasicStat3()], getBasicStat3Value());
		tooltipText += " \n";
		tooltipText += va(weaponStat1Tooltips[getMod1Stat()], getMod1Value());
		tooltipText += va(weaponStat2Tooltips[getMod2Stat()], getMod2Value());
		tooltipText += va(weaponStat3Tooltips[getMod3Stat()], getMod3Value());
		tooltipText += " \n";
		tooltipText += va("^5Value: ^P%i.\n", getStackCost());
		break;
	case IT_SABER_MODIFICATION:
	case IT_SABER_CRYSTAL:
		tooltipText += va("%s^B%s (%s)^b\n", getColorStringForQuality(), getName(), itemQualityTooltips[getQuality()]);
		tooltipText += va("^PLightsaber %s\n", IT_SABER_CRYSTAL ? "Crystal" : "Modification");
		tooltipText += " \n";
		tooltipText += va("^5%s\n", getDescription());
		tooltipText += " \n";
		tooltipText += weaponCrystalTooltips[getCrystal()];
		tooltipText += va(saberStat1Tooltips[getBasicStat1()], getBasicStat1Value());
		tooltipText += va(saberStat2Tooltips[getBasicStat2()], getBasicStat2Value());
		tooltipText += va(saberStat3Tooltips[getBasicStat3()], getBasicStat3Value());
		tooltipText += " \n";
		tooltipText += va(saberStat1Tooltips[getMod1Stat()], getMod1Value());
		tooltipText += va(saberStat2Tooltips[getMod2Stat()], getMod2Value());
		tooltipText += va(saberStat3Tooltips[getMod3Stat()], getMod3Value());
		tooltipText += " \n";
		tooltipText += va("^5Value: ^P%i.\n", getStackCost());
		break;
	case IT_WEAPON:
		if (getBaseItem()->giTag == WP_SABER)
		{
			tooltipText += va("%s^B%s (%s)^b\n", getColorStringForQuality(), getName(), itemQualityTooltips[getQuality()]);
			tooltipText += "^POne handed weapon, Lightsaber\n";
			tooltipText += " \n";
			tooltipText += va("^5%s\n", getDescription());
			tooltipText += " \n";
			tooltipText += "^7Scaling Attribute: ^PStrength\n";
			tooltipText += "^7Damage: ^P78-102 ^8(^P40.5 DPS^8).\n";
			tooltipText += "^7Attacks per Second: ^P0.45\n";
			tooltipText += "^7Crit Chance: ^P+11.5%\n";
			tooltipText += "^7Crit Power: ^P+41.0%\n";
			tooltipText += " \n";
			tooltipText += weaponCrystalTooltips[getCrystal()];
			tooltipText += va(saberStat1Tooltips[getBasicStat1()], getBasicStat1Value());
			tooltipText += va(saberStat2Tooltips[getBasicStat2()], getBasicStat2Value());
			tooltipText += va(saberStat3Tooltips[getBasicStat3()], getBasicStat3Value());
			tooltipText += " \n";
			tooltipText += va(saberStat1Tooltips[getMod1Stat()], getMod1Value());
			tooltipText += va(saberStat2Tooltips[getMod2Stat()], getMod2Value());
			tooltipText += va(saberStat3Tooltips[getMod3Stat()], getMod3Value());
			tooltipText += " \n";
			tooltipText += va("^5Value: ^P%i.\n", getStackCost());
		}
		else
		{
			tooltipText += va("%s^B%s (%s)^b\n", getColorStringForQuality(), getName(), itemQualityTooltips[getQuality()]);
			tooltipText += "^POne handed weapon, Gun\n";
			tooltipText += " \n";
			tooltipText += va("^5%s\n", getDescription());
			tooltipText += " \n";
			tooltipText += "^7Scaling Attribute: ^PStrength\n";
			tooltipText += "^7Damage: ^P78-102 ^8(^P40.5 DPS^8).\n";
			tooltipText += "^7Attacks per Second: ^P0.45\n";
			tooltipText += "^7Crit Chance: ^P+11.5%\n";
			tooltipText += "^7Crit Power: ^P+41.0%\n";
			tooltipText += " \n";
			tooltipText += weaponCrystalTooltips[getCrystal()];
			tooltipText += va(weaponStat1Tooltips[getBasicStat1()], getBasicStat1Value());
			tooltipText += va(weaponStat2Tooltips[getBasicStat2()], getBasicStat2Value());
			tooltipText += va(weaponStat3Tooltips[getBasicStat3()], getBasicStat3Value());
			tooltipText += " \n";
			tooltipText += va(weaponStat1Tooltips[getMod1Stat()], getMod1Value());
			tooltipText += va(weaponStat2Tooltips[getMod2Stat()], getMod2Value());
			tooltipText += va(weaponStat3Tooltips[getMod3Stat()], getMod3Value());
			tooltipText += " \n";
			tooltipText += va("^5Value: ^P%i.\n", getStackCost());
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

//
// Database functions... TODO...
//
#ifdef _GAME
void inventoryItem::storeInventoryItemToDatabaseInstant()
{// Will be called when an item has been modified or looted... For server shutdown, instant transmission.
#ifdef __DATABASE_ENABLED__
 // TODO: Write to DB here...
#endif //__DATABASE_ENABLED__
}

void storeInventoryToDatabaseInstant(int clientNum, int playerID)
{// Will be called on server, before game shutdown/reset...
#ifdef __DATABASE_ENABLED__
	gclient_t *client = g_entities[clientNum].client;

	for (int i = 0; i < client->inventoryCount; i++)
	{
		inventoryItem *item = client->inventory[i];

		if (item)
		{
			item->storeInventoryItemToDatabaseInstant();
		}
	}
#endif //__DATABASE_ENABLED__
}

void inventoryItem::storeInventoryItemToDatabaseQueued()
{// Will be called when an item has been modified or looted... Actually, the SQL should be queued, then run in another thread every X minutes.
#ifdef __DATABASE_ENABLED__
 // TODO: Write to DB here...
#endif //__DATABASE_ENABLED__
}

void storeInventoryToDatabaseQueued(int clientNum, int playerID)
{// Will be called on server, when player logs out...
#ifdef __DATABASE_ENABLED__
	gclient_t *client = g_entities[clientNum].client;

	for (int i = 0; i < client->inventoryCount; i++)
	{
		inventoryItem *item = client->inventory[i];

		if (item)
		{
			item->storeInventoryItemToDatabaseQueued();
		}
	}
#endif //__DATABASE_ENABLED__
}

void loadInventoryFromDatabase(int clientNum, int playerID)
{// Will be called on server when client logs in...
#ifdef __DATABASE_ENABLED__
	gclient_t *client = g_entities[clientNum].client;
	client->inventoryCount = 0;

	// Load all instances of playerID's inventory from SQL server for the online players, and dump them into their INVENTORY_ITEM_INSTANCES[x] instances.
	int numItems = 0;
	int itemIDs[64] = { -1 };
	int baseItemIDs[64] = { 0 };
	int qualities[64] = { 0 };
	int quantites[64] = { 0 };
	int crystals[64] = { 0 };
	int stat1[64] = { 0 };
	int stat2[64] = { 0 };
	int stat3[64] = { 0 };
	float statValue1[64] = { 0.0 };
	float statValue2[64] = { 0.0 };
	float statValue3[64] = { 0.0 };
	int mod1[64] = { 0 };
	int mod2[64] = { 0 };
	int mod3[64] = { 0 };
	float modValue1[64] = { 0.0 };
	float modValue2[64] = { 0.0 };
	float modValue3[64] = { 0.0 };
	// ... load them here, into the above values, from SQL...

	// TODO...

	// Then add them to the player's inventories...
	for (int i = 0; i < numItems; i++)
	{// TODO: lookup DB - eg: get all item SQL entries for playerID.
		inventoryItem *item = inventoryItem(itemIDs[i]);

		item->setItemID(itemIDs[i]);
		item->setBaseItem(baseItemIDs[i]);
		item->setQuality((itemQuality_t)qualities[i]);
		item->setCrystal(crystals[i]);

		item->setStat1(stat1[i], statValue1[i]);
		item->setStat2(stat2[i], statValue2[i]);
		item->setStat3(stat3[i], statValue3[i]);

		item->setMod1(mod1[i], modValue1[i]);
		item->setMod2(mod2[i], modValue2[i]);
		item->setMod3(mod3[i], modValue3[i]);

		item->setQuantity(quantites[i]);

		INVENTORY_ITEM_INSTANCES[itemID] = item;
		client->inventory[client->inventoryCount] = item;
		client->inventoryCount++;
	}
#else //!__DATABASE_ENABLED__
	gclient_t *client = g_entities[clientNum].client;
	client->inventoryCount = 0;

	for (int i = 0; i < INVENTORY_ITEM_INSTANCES_COUNT; i++)
	{
		client->inventory[i] = NULL;
	}
#endif //__DATABASE_ENABLED__
}

void inventoryItem::removeFromDatabase()
{// Will be called when an item has been destroyed...
#ifdef __DATABASE_ENABLED__
 // TODO: Write to DB here...
#endif //__DATABASE_ENABLED__
}
#endif

//
// Upload items to clients...
//
#ifdef _GAME
void sendInventoryItemToClient(int clientNum, inventoryItem *item)
{// To be called when a new item is looted, or an item is modified.
	gclient_t *client = g_entities[clientNum].client;
	std::string command = "inv";
	command += va(" \"%i\"", item->m_transmitted ? 0 : 1);
	command += va(" \"%i\"", item->getBaseItem());
	command += va(" \"%i\"", item->getQuality());
	command += va(" \"%i\"", item->getQuantity());
	command += va(" \"%i\"", item->getCrystal());
	command += va(" \"%i\"", item->getBasicStat1());
	command += va(" \"%i\"", item->getBasicStat2());
	command += va(" \"%i\"", item->getBasicStat3());
	command += va(" \"%f\"", item->getBasicStat1Value());
	command += va(" \"%f\"", item->getBasicStat2Value());
	command += va(" \"%f\"", item->getBasicStat3Value());
	command += va(" \"%i\"", item->getMod1Stat());
	command += va(" \"%i\"", item->getMod2Stat());
	command += va(" \"%i\"", item->getMod3Stat());
	command += va(" \"%f\"", item->getMod1Value());
	command += va(" \"%f\"", item->getMod2Value());
	command += va(" \"%f\"", item->getMod3Value());
	trap->SendServerCommand(clientNum, command.c_str());
	item->m_transmitted = qtrue;
}

void sendFullInventoryToClient(int clientNum)
{// To be called at login, or on map change...
	gclient_t *client = g_entities[clientNum].client;

	for (int i = 0; i < client->inventoryCount; i++)
	{
		if (client->inventory[i])
		{
			sendInventoryItemToClient(clientNum, client->inventory[i]);
		}
	}
}
#endif

//
// Recieve new items from server...
//
#ifdef _CGAME
void RevieveInventoryPacket()
{
	// TODO: Deconstruct char string the server sent, and add the item to inventory...
	clientInfo_t *client = &cgs.clientinfo[cg.clientNum];

	qboolean isNewitem = (qboolean)atoi(CG_Argv(2));
	int itemID = atoi(CG_Argv(3));
	int baseItemID = atoi(CG_Argv(4));
	int quality = atoi(CG_Argv(5));
	int quantity = atoi(CG_Argv(6));
	int crystal = atoi(CG_Argv(7));
	int basicStat1 = atoi(CG_Argv(8));
	int basicStat2 = atoi(CG_Argv(9));
	int basicStat3 = atoi(CG_Argv(10));
	int basicStat1Value = atof(CG_Argv(11));
	int basicStat2Value = atof(CG_Argv(12));
	int basicStat3Value = atof(CG_Argv(13));
	int mod1Type = atoi(CG_Argv(14));
	int mod2Type = atoi(CG_Argv(15));
	int mod3Type = atoi(CG_Argv(16));
	int mod1Value = atof(CG_Argv(17));
	int mod2Value = atof(CG_Argv(18));
	int mod3Value = atof(CG_Argv(19));

	if (isNewitem)
	{// Brand spanking new item...
		inventoryItem *item = createInventoryItem(baseItemID, (itemQuality_t)quality, quantity);
		
		item->setItemID(itemID);

		item->setCrystal(crystal);

		item->setStat1(basicStat1, basicStat1Value);
		item->setStat2(basicStat2, basicStat2Value);
		item->setStat3(basicStat3, basicStat3Value);
		
		item->setMod1(mod1Type, mod1Value);
		item->setMod2(mod3Type, mod2Value);
		item->setMod3(mod2Type, mod3Value);

		client->inventory[client->inventoryCount] = item;
		client->inventoryCount++;
	}
	else
	{// Update to old item...
		inventoryItem *item = NULL;
		
		for (int i = 0; i < client->inventoryCount; i++)
		{
			inventoryItem *thisItem = client->inventory[i];

			if (thisItem && thisItem->getItemID() == itemID)
			{
				item = thisItem;
				break;
			}
		}

		if (item)
		{
			item->setItemID(itemID);
			item->setBaseItem(baseItemID);
			item->setQuality((itemQuality_t)quality);
			item->setCrystal(crystal);

			item->setStat1(basicStat1, basicStat1Value);
			item->setStat2(basicStat2, basicStat2Value);
			item->setStat3(basicStat3, basicStat3Value);

			item->setMod1(mod1Type, mod1Value);
			item->setMod2(mod3Type, mod2Value);
			item->setMod3(mod2Type, mod3Value);

			item->setQuantity(quantity);
		}
		else
		{
			trap->Print("Failed to update inventory item. It does not exist on the client.\n");
		}
	}
}
#endif
