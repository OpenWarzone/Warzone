#include "dock_postprocess.h"
#include "../imgui_docks/dock_console.h"

DockPostProcess::DockPostProcess() {}

const char *DockPostProcess::label() {
	return "PostProcess";
}


namespace ImGui {
	// roses are red, qboolean is no bool
	extern bool Checkbox(char *label, qboolean *var);
}

int			ImGuiNumCvars = 0;
cvar_t		*ImGuiCvars[128] = { 0 };
int			ImGuiMax[128] = { 1 };
int			ImGuiValue[128] = { 1 };

void DockPostProcess::ClearFrame() {
	ImGuiNumCvars = 0;
}

void DockPostProcess::AddCvar(cvar_t *cvar, int max) {
	ImGuiCvars[ImGuiNumCvars] = cvar;
	ImGuiMax[ImGuiNumCvars] = max;
	ImGuiValue[ImGuiNumCvars] = cvar->integer;
	ImGuiNumCvars++;
}

void DockPostProcess::UpdateCvars() {
	for (int i = 0; i < ImGuiNumCvars; i++)
	{
		if (ImGuiValue[i] > ImGuiMax[i])
			ImGuiValue[i] = ImGuiMax[i];

		if (ImGuiValue[i] != ImGuiCvars[i]->integer)
		{
			ri->Cvar_Set(ImGuiCvars[i]->name, va("%i", ImGuiValue[i]));
		}
	}
}

void DockPostProcess::AddCheckBox(int ID) {
	if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->displayName && ImGuiCvars[ID]->displayName[0])
	{
		ImGui::Checkbox(ImGuiCvars[ID]->displayName, (qboolean *)&ImGuiValue[ID]);
		
		//ImGui::Text(ImGuiCvars[ID]->displayName); ImGui::SameLine();
		//ImGui::Text(":"); ImGui::SameLine();
		//ImGui::Checkbox("", (qboolean *)&ImGuiValue[ID]);

		if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->description && ImGuiCvars[ID]->description[0])
		{
			if (ImGui::IsItemHovered(0))
				ImGui::SetTooltip(ImGuiCvars[ID]->description);
		}
	}
	else
	{
		ImGui::Checkbox(ImGuiCvars[ID]->name, (qboolean *)&ImGuiValue[ID]);

		//ImGui::Text(ImGuiCvars[ID]->name); ImGui::SameLine();
		//ImGui::Text(":"); ImGui::SameLine();
		//ImGui::Checkbox("", (qboolean *)&ImGuiValue[ID]);

		if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->description && ImGuiCvars[ID]->description[0])
		{
			if (ImGui::IsItemHovered(0))
				ImGui::SetTooltip(ImGuiCvars[ID]->description);
		}
	}
}

void DockPostProcess::AddInt(int ID) {
	if (ImGuiMax[ID] < 2)
	{// Single on/off option, use checkbox instead...
		AddCheckBox(ID);
	}
	else if (ImGuiMax[ID] < 8)
	{// Use Radio Buttons for low max options...
		if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->displayName && ImGuiCvars[ID]->displayName[0])
		{
			// Sigh... It bases everything on the name??? Can't reuse anything like "1" "2" "3" "Off"... That sucks...
			//ImGui::Text(ImGuiCvars[ID]->displayName); ImGui::SameLine();
			//ImGui::Text(":"); ImGui::SameLine();

			for (int i = 0; i <= ImGuiMax[ID]; i++)
			{
				//ImGui::RadioButton(i == 0 ? "Off" : va("%i", i), &ImGuiValue[ID], i); ImGui::SameLine();
				ImGui::RadioButton(i == 0 ? va("%s Off", ImGuiCvars[ID]->displayName) : va("%s %i", ImGuiCvars[ID]->displayName, i), &ImGuiValue[ID], i); ImGui::SameLine();

				if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->description && ImGuiCvars[ID]->description[0])
				{
					if (ImGui::IsItemHovered(0))
						ImGui::SetTooltip(ImGuiCvars[ID]->description);
				}
			}

			ImGui::NewLine();
		}
		else
		{
			//ImGui::Text(ImGuiCvars[ID]->name); ImGui::SameLine();
			//ImGui::Text(":"); ImGui::SameLine();

			for (int i = 0; i <= ImGuiMax[ID]; i++)
			{
				//ImGui::RadioButton(i == 0 ? "Off" : va("%i", i), &ImGuiValue[ID], i); ImGui::SameLine();
				ImGui::RadioButton(i == 0 ? va("%s Off", ImGuiCvars[ID]->name) : va("%s %i", ImGuiCvars[ID]->name, i), &ImGuiValue[ID], i); ImGui::SameLine();

				if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->description && ImGuiCvars[ID]->description[0])
				{
					if (ImGui::IsItemHovered(0))
						ImGui::SetTooltip(ImGuiCvars[ID]->description);
				}
			}

			ImGui::NewLine();
		}
	}
	else
	{// Lots of possibilities, use a slider...
		if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->displayName && ImGuiCvars[ID]->displayName[0])
		{
			ImGui::DragInt(ImGuiCvars[ID]->displayName, &ImGuiValue[ID], 1.0, 0, ImGuiMax[ID]);

			if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->description && ImGuiCvars[ID]->description[0])
			{
				if (ImGui::IsItemHovered(0))
					ImGui::SetTooltip(ImGuiCvars[ID]->description);
			}
		}
		else
		{
			ImGui::DragInt(ImGuiCvars[ID]->name, &ImGuiValue[ID], 1.0, 0, ImGuiMax[ID]);

			if (ImGuiCvars[ID]->displayInfoSet && ImGuiCvars[ID]->description && ImGuiCvars[ID]->description[0])
			{
				if (ImGui::IsItemHovered(0))
					ImGui::SetTooltip(ImGuiCvars[ID]->description);
			}
		}
	}
}

void DockPostProcess::UpdateUI() {
	for (int i = 0; i < ImGuiNumCvars; i++)
	{
		if (ImGuiMax[i] == 1)
			AddCheckBox(i);
		else
			AddInt(i);
	}
}

void DockPostProcess::MakeCvarList() {
	AddCvar(r_dynamicGlow, 1);
	AddCvar(r_bloom, 2);
	AddCvar(r_anamorphic, 1);
	AddCvar(r_ssdm, 2);
	AddCvar(r_ao, 5);
#ifdef __SSRTGI__
	AddCvar(r_ssrtgi, 1);
#endif //__SSRTGI__
	AddCvar(r_cartoon, 3);
#ifdef __SSDO__
	AddCvar(r_ssdo, 1);
#endif //__SSDO__
	//AddCvar(r_sss, 1);
	AddCvar(r_deferredLighting, 1);
	//AddCvar(r_fastLighting, 1);
	//AddCvar(r_ssr, 1);
	//AddCvar(r_sse, 1);
	AddCvar(r_magicdetail, 1);
	//AddCvar(r_hbao, 1);
	AddCvar(r_glslWater, 3);
	AddCvar(r_fogPost, 1);
	AddCvar(r_multipost, 1);
	AddCvar(r_dof, 3);
	AddCvar(r_lensflare, 1);
	AddCvar(r_testshader, 1);
	AddCvar(r_esharpening, 1);
	//AddCvar(r_esharpening2, 1);
	//AddCvar(r_darkexpand, 1);
	//AddCvar(r_distanceBlur, 5);
	AddCvar(r_volumeLight, 1);
	AddCvar(r_cloudshadows, 2);
	AddCvar(r_cloudQuality, 4);
	AddCvar(r_fxaa, 1);
	AddCvar(r_txaa, 1);
	AddCvar(r_showdepth, 1);
	AddCvar(r_shownormals, 4);
	AddCvar(r_trueAnaglyph, 2);
	AddCvar(r_toneMap, 1);
	AddCvar(r_occlusion, 1);
}

void DockPostProcess::imgui() {
	ClearFrame();
	MakeCvarList();
	UpdateUI();
	UpdateCvars();
}