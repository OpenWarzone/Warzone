#include "dock_models.h"

#include "../imgui_docks/dock_console.h"
#include "../imgui_docks_openjk/dock_mdxm.h"
#include "../imgui_docks_openjk/dock_mdxa.h"
//#include <renderergl2/tr_model_kung.h>
#include "rd-warzone/tr_glsl.h"
#include "../tr_local.h"
#include "../compose_models.h"

DockModels::DockModels() {}

const char *DockModels::label() {
	return "Models";
}

void imgui_mdxm(model_t *mod) {
	mdxmData_t *glm = mod->data.glm;
	mdxmHeader_t *header = glm->header;
	ImGui::Text("ident=%d", header->ident);
	ImGui::Text("version=%d", header->version);
	ImGui::Text("name=%s", header->name);
	ImGui::Text("animName=%s", header->animName);
	ImGui::Text("animIndex=%d", header->animIndex);
	ImGui::Text("numBones=%d", header->numBones);
	ImGui::Text("numLODs=%d", header->numLODs);
	ImGui::Text("ofsLODs=%d", header->ofsLODs);
	ImGui::Text("numSurfaces=%d", header->numSurfaces);
	ImGui::Text("ofsSurfHierarchy=%d", header->ofsSurfHierarchy);
	ImGui::Text("ofsEnd=%d", header->ofsEnd);
	if (ImGui::Button("Open")) {
		new DockMDXM(mod);
	}
}
void imgui_mdxa(model_t *mod) {
	mdxaHeader_t *header = mod->data.gla;

	//int			ident;				// 	"IDP3" = MD3, "RDM5" = MDR, "2LGA"(GL2 Anim) = MDXA
	//int			version;			// 1,2,3 etc as per format revision
	//char		name[MAX_QPATH];	// GLA name (eg "skeletons/marine")	// note: extension missing
	//float		fScale;				// will be zero if build before this field was defined, else scale it was built with
	//// frames and bones are shared by all levels of detail
	//int			numFrames;
	//int			ofsFrames;			// points at mdxaFrame_t array
	//int			numBones;			// (no offset to these since they're inside the frames array)
	//int			ofsCompBonePool;	// offset to global compressed-bone pool that all frames use
	//int			ofsSkel;			// offset to mdxaSkel_t info
	//int			ofsEnd;				// EOF, which of course gives overall file size

	ImGui::Text("ident=%d version=%d name=%s fScale=%f numFrames=%d ofsFrames=%d numBones=%d ofsCompBonePool=%d ofsSkel=%d ofsEnd=%d",
		header->ident,
		header->version,
		header->name,
		header->fScale,
		header->numFrames,
		header->ofsFrames,
		header->numBones,
		header->ofsCompBonePool,
		header->ofsSkel,
		header->ofsEnd
	);

	if (ImGui::Button("Open")) {
		new DockMDXA(mod);
	}
}

void DockModels::imgui() {
	for (int i=0; i<tr.numModels; i++) {
		auto model = tr.models[i];
		char buf[512];
		sprintf(buf, "model[%d] name=%s type=%s", i, model->name, toString(model->type));
		if (ImGui::CollapsingHeader(buf)) {
			ImGui::PushID(model);
			switch (model->type) {
				case MOD_MDXM: imgui_mdxm(model); break;
				case MOD_MDXA: imgui_mdxa(model); break;
			}
			ImGui::PopID();
		}
	}
}
