#include "bg_inventoryItem.h"

extern int		bg_numItems;
extern gitem_t	bg_itemlist[];

#if defined(_GAME)
extern void BG_CreateRandomNPCInventory(int entityNum); // Sorry, don't have access to gentity_t here...
#endif //defined(_GAME)

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
