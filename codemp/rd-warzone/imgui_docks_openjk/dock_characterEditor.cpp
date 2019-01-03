#include "dock_characterEditor.h"
#include "../imgui_docks/dock_console.h"

DockCharacterEditor::DockCharacterEditor() {}

const char *DockCharacterEditor::label() {
	return "Character Editor";
}

/*
//Proper/desired hierarchy list
static const char *BonesNameList[20] =
{
	"lfemurYZ",
	"lfemurX",
	"ltibia",
	"ltalus",
	"ltarsal",

	"rfemurYZ",
	"rfemurX",
	"rtibia",
	"rtalus",
	"rtarsal",

	"lhumerus",
	"lhumerusX",
	"lradius",
	"lradiusX",
	"lhand",

	"rhumerus",
	"rhumerusX",
	"rradius",
	"rradiusX",
	"rhand"
};
*/

bool	boneScaleValuesInitialized = qfalse;
float	genericBoneScaleValues[20];
float	boneScaleValues[20];

void CharacterEditor_InitializeBoneScaleValues(void)
{
	if (boneScaleValuesInitialized) return;
	
	for (int ID = 0; ID < 20; ID++)
	{
		genericBoneScaleValues[ID] = 1.0;
		boneScaleValues[ID] = 1.0;
	}

	boneScaleValuesInitialized = qtrue;
}

void DockCharacterEditor::UpdateUI() {
	// Init if needed...
	CharacterEditor_InitializeBoneScaleValues();

#ifdef __EXPERIMETNAL_CHARACTER_EDITOR__
	if (ImGui::Button("Reset"))
	{
		for (int ID = 0; ID < 20; ID++)
		{
			boneScaleValues[ID] = 1.0;
		}
	}

	ImGui::Separator();

	if (backEnd.localPlayerEntity)
	{
		extern char *R_GetGhoul2ModelBoneName(trRefEntity_t *ent, int boneNum);

		for (int ID = 0; ID < 20; ID++)
		{
			ImGui::DragFloat(va("%s - Scale", R_GetGhoul2ModelBoneName(backEnd.localPlayerEntity, ID)/*BonesNameList[ID]*/), &boneScaleValues[ID], 0.01, 0.1, 5.0, "%.2f", 1.0F);
		}
	}
#endif //__EXPERIMETNAL_CHARACTER_EDITOR__
}

void DockCharacterEditor::imgui() {
	UpdateUI();
}