#include "bg_inventory.h"


#ifdef _GAME
#include "g_local.h"
#endif

#ifdef _CGAME
#include "../cgame/cg_local.h"
#endif


#if defined(rd_warzone_x86_EXPORTS)
#include "tr_local.h"
#endif

extern inventoryItem	*allInventoryItems[65536];

extern uint16_t			allInventoryItemsCount;
extern uint16_t			allInventoryReforgedWeaponsStart;
extern uint16_t			allInventoryReforgedWeaponsEnd;
extern uint16_t			allInventoryUniqueWeaponsStart;
extern uint16_t			allInventoryUniqueWeaponsEnd;

extern float StatRollForQuality(uint16_t quality);

extern stringID_table_t weaponStat1Strings[];
extern stringID_table_t weaponStat2Strings[];
extern stringID_table_t weaponStat3Strings[];

void GenerateReforgeWeaponVariant(int bgItemID, char *filename, char *thisReforgeName)
{
	//Com_Printf("Filename: %s. thisReforgeName: %s.\n", filename, thisReforgeName);

	//
	// Get name and description...
	//
	char name[64];
	strcpy(name, IniRead(filename, thisReforgeName, "name", ""));

	char description[64];
	strcpy(description, IniRead(filename, thisReforgeName, "description", ""));

	//
	// Get requires stats...
	//
	int numRequiredStats = 0;
	int requiredStat1 = WEAPON_STAT1_DEFAULT;
	int requiredStat2 = WEAPON_STAT2_DEFAULT;
	int requiredStat3 = WEAPON_STAT3_DEFAULT;

	char stat1[128] = { { 0 } };
	strcpy(stat1, IniRead(filename, thisReforgeName, "requiredStat1", "WEAPON_STAT1_DEFAULT"));
	for (int st = 1; st < WEAPON_STAT1_MAX; st++)
	{
		if (!strcmp(weaponStat1Strings[st].name, stat1))
		{
			requiredStat1 = st;
			numRequiredStats++;
			break;
		}
	}

	char stat2[128] = { { 0 } };
	strcpy(stat2, IniRead(filename, thisReforgeName, "requiredStat2", "WEAPON_STAT2_DEFAULT"));
	for (int st = 1; st < WEAPON_STAT2_MAX; st++)
	{
		if (!strcmp(weaponStat2Strings[st].name, stat2))
		{
			requiredStat2 = st;
			numRequiredStats++;
			break;
		}
	}

	char stat3[128] = { { 0 } };
	strcpy(stat3, IniRead(filename, thisReforgeName, "requiredStat3", "WEAPON_STAT3_DEFAULT"));
	for (int st = 1; st < WEAPON_STAT3_MAX; st++)
	{
		if (!strcmp(weaponStat3Strings[st].name, stat3))
		{
			requiredStat3 = st;
			numRequiredStats++;
			break;
		}
	}

	//
	// Get bonus stat - TODO: For now giving bonus to all stats...
	//
	/*
	char stat[128] = { { 0 } };
	strcpy(stat, IniRead(filename, thisReforgeName, "bonusStat", "WEAPON_BONUS_STAT_DEFAULT"));
	for (int st = 1; st < WEAPON_BONUS_STAT_MAX; st++)
	{
		if (!strcmp(weaponBonusStatStrings[st].name, stat))
		{
		bonusStat = st;
		break;
		}
	}
	*/

#ifdef _CGAME
	//
	// Custom model...
	//
	char model[128];
	strcpy(model, IniRead(filename, thisReforgeName, "model", ""));

	//
	// Custom EFX folder... TODO: For now using the defaults...
	//
	char efx[128];
	strcpy(efx, IniRead(filename, thisReforgeName, "efx", ""));

	//
	// Custom sounds folder... TODO: For now using the defaults...
	//
	char sounds[128];
	strcpy(sounds, IniRead(filename, thisReforgeName, "sounds", ""));
#endif //_CGAME

	if (numRequiredStats <= 0)
	{
		return;
	}

	//Com_Printf("name: %s.\n", name);
	//Com_Printf("description: %s.\n", description);
	//Com_Printf("stats: %s (%i), %s (%i), %s (%i).\n", stat1, requiredStat1, stat2, requiredStat2, stat3, requiredStat3);

	//
	// We now should have all the required data to generate this item... Dew it!
	//
	int quality = 3 + numRequiredStats;

	float roll = StatRollForQuality(quality);

	for (uint16_t crystal = ITEM_CRYSTAL_DEFAULT; crystal < ITEM_CRYSTAL_MAX; crystal++)
	{// We still need all possible crystals...
		inventoryItem *item = new inventoryItem(allInventoryItemsCount, bgItemID, (itemQuality_t)quality, SABER_MODELTYPE_DEFAULT, 1);

		item->setItemID(allInventoryItemsCount);
		item->setBaseItem(bgItemID);
		item->setQuality((itemQuality_t)quality);
		item->setCrystal((itemPowerCrystal_t)crystal);

		if (numRequiredStats >= 1) item->setStat1(requiredStat1, roll);
		if (numRequiredStats >= 2) item->setStat2(requiredStat2, roll*1.15);
		if (numRequiredStats >= 3) item->setStat3(requiredStat3, roll*1.25);

		item->setQuantity(1);

		item->setCustomType(ITEM_CUSTOMIZATION_REFORGE);
		item->setCustomName(name);
		item->setCustomDescription(description);
#ifdef _CGAME
		item->setCustomModel(model);
		item->setCustomEfx(efx);
		item->setCustomSounds(sounds);
#endif //_CGAME

		//#if defined(_GAME)
		//	//trap->Print("^1*** ^3INVENTORY-GAME^5: Weapon %i. Name: %s\n", allInventoryItemsCount, item->getName());
		//	//trap->Print("^1*** ^3INVENTORY-GAME^5: Weapon %i. Name: %s - %s\n", allInventoryItemsCount, item->getName(), va(weaponCrystalTooltips[crystal], roll * 100.0, roll * 100.0));
		//	trap->Print("^1*** ^3INVENTORY-GAME^5: Weapon %i. Name: %s\n", allInventoryItemsCount, item->getCustomName());
		//#endif

		allInventoryItems[allInventoryItemsCount] = item;
		allInventoryItemsCount++;
	}
}

uint16_t LoadReforgedWeapons(void)
{
	//
	// Weapons...
	//

	allInventoryReforgedWeaponsStart = allInventoryItemsCount;

	// Reforged weapons...
	int bgItemID = 39;

	int numFiles;
	char filename[MAX_QPATH];
	int i;
	
#if defined(rd_warzone_x86_EXPORTS)
	// OMG... Renderer just has to be different... like usual...
	char **filelist = ri->FS_ListFiles("reforgedWeapons", ".weaponInfo", &numFiles);

	ri->Printf(PRINT_ALL, "numFiles: %i.\n", numFiles);

	for (i = 0; i < numFiles; i++)
	{
		Com_sprintf(filename, sizeof(filename), "reforgedWeapons/%s", filelist[i]);
		//ri->Printf(PRINT_ALL, "Filename: %s.\n", filename);
#else
	int filelen;
	char* fileptr;
	char filelist[4096];
	numFiles = trap->FS_GetFileList("reforgedWeapons/", ".weaponInfo", filelist, sizeof(filelist));

	fileptr = filelist;

	for (i = 0; i < numFiles; i++, fileptr += filelen + 1)
	{
		filelen = strlen(fileptr);
		Q_strncpyz(filename, "reforgedWeapons/", sizeof(filename));
		Q_strcat(filename, sizeof(filename), fileptr);
#endif
		
		/*
		[weaponReforge2]
		name=Enhanced Blastech E-19 Prototype
		description=A prototype of the Blastech E-19.
		requiredStat1=WEAPON_STAT1_HEAVY_PISTOL
		requiredStat2=WEAPON_STAT2_CRITICAL_CHANCE_MODIFIER
		bonusStat=WEAPON_BONUSSTAT_E19_REFORGE2
		model=models/weapons/e-19/e-19b.md3
		efx=efx/e-19b/
		sounds=sounds/weapons/e-19b/
		*/

		for (int f = 1; f < 3; f++)
		{// Loop through the max possible variants of this reforge weapon...
			GenerateReforgeWeaponVariant(bgItemID, filename, va("weaponReforge%i", f));
		}

		//
		// And now generate the [weaponFinal] version, this should always exist...
		//

		/*
		[weaponFinal]
		name=Blastech E-19
		description=A Blastech E-19.
		requiredStat1=WEAPON_STAT1_HEAVY_PISTOL
		requiredStat2=WEAPON_STAT2_CRITICAL_CHANCE_MODIFIER
		requiredStat3=WEAPON_STAT3_SHOT_REPEATING
		bonusStat=WEAPON_BONUSSTAT_E19_FINAL
		model=models/weapons/e-19/e-19.md3
		efx=efx/e-19final/
		sounds=sounds/weapons/e-19final/
		*/

		GenerateReforgeWeaponVariant(bgItemID, filename, "weaponFinal");
	}

	allInventoryReforgedWeaponsEnd = Q_max(allInventoryItemsCount - 1, allInventoryReforgedWeaponsStart); // Q_max here to account for nothing found...

	if (allInventoryReforgedWeaponsEnd == allInventoryReforgedWeaponsStart)
	{// Make sure it's zero's...
		allInventoryReforgedWeaponsStart = 0;
		allInventoryReforgedWeaponsEnd = 0;
	}

#if defined(rd_warzone_x86_EXPORTS)
	ri->FS_FreeFileList(filelist);
#endif

	return allInventoryReforgedWeaponsEnd - allInventoryReforgedWeaponsStart;
}

uint16_t LoadUniqueWeapons(void)
{
	// TODO
	allInventoryUniqueWeaponsStart = 0;
	allInventoryUniqueWeaponsEnd = 0;
	return 0;
}