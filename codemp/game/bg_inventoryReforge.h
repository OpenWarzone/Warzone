#include "bg_inventoryItem.h"

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
