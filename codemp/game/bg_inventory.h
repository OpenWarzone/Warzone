#include "bg_inventoryItem.h"


//
// Inventory System Global Defines...
//

//#define __SEND_FULL_WEAPON_INFO_WITH_BOLT__				// This would send all the information about the player's inventory system item (and it's mods) to the clients... Should we need it...



//
// bg_itemlist - TODO: Replace/Extend with ini files...
//

extern int		bg_numItems;
extern gitem_t	bg_itemlist[];


//
// Inventory System Usage Functions for GAME, CGAME, and RENDERER...
//

// Basic lookups stuff... Returns whole item info...
extern inventoryItem *BG_GetInventoryItemByID(uint16_t id);
extern inventoryItem *BG_EquippedWeapon(playerState_t *ps);
extern inventoryItem *BG_EquippedMod1(playerState_t *ps);
extern inventoryItem *BG_EquippedMod2(playerState_t *ps);
extern inventoryItem *BG_EquippedMod3(playerState_t *ps);

// Look up specific visual type info...
extern qboolean BG_EquippedWeaponIsTwoHanded(playerState_t *ps);
extern uint16_t BG_EquippedWeaponVisualType1(playerState_t *ps);
extern uint16_t BG_EquippedWeaponVisualType2(playerState_t *ps);
extern uint16_t BG_EquippedWeaponVisualType3(playerState_t *ps);
extern uint16_t BG_EquippedWeaponCrystal(playerState_t *ps);
