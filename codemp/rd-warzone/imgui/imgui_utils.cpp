#include "imgui_utils.h"
#include "include_imgui.h"

namespace ImGui {

	void DragVector3(float *vec3) {
		ImGui::PushItemWidth(200);
		ImGui::PushID(vec3);
		ImGui::DragFloat("x", vec3 + 0);
		ImGui::SameLine();
		ImGui::DragFloat("y", vec3 + 1);
		ImGui::SameLine();
		ImGui::DragFloat("z", vec3 + 2);
		ImGui::PopID();
		ImGui::PopItemWidth();
	}
	void DragMatrix3x3(float mat[3][3]) {
		ImGui::PushItemWidth(200);
		ImGui::PushID(mat);

		ImGui::DragFloat("00", &mat[0][0]);
		ImGui::SameLine();
		ImGui::DragFloat("01", &mat[0][1]);
		ImGui::SameLine();
		ImGui::DragFloat("02", &mat[0][2]);

		ImGui::DragFloat("10", &mat[1][0]);
		ImGui::SameLine();
		ImGui::DragFloat("11", &mat[1][1]);
		ImGui::SameLine();
		ImGui::DragFloat("12", &mat[1][2]);

		ImGui::DragFloat("20", &mat[2][0]);
		ImGui::SameLine();
		ImGui::DragFloat("21", &mat[2][1]);
		ImGui::SameLine();
		ImGui::DragFloat("22", &mat[2][2]);

		ImGui::PopID();
		ImGui::PopItemWidth();
	}

}