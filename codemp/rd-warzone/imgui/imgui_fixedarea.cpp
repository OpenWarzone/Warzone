#include "imgui_fixedarea.h"

FixedArea::FixedArea(ImVec2 pos_, ImVec2 size_, int cols_) {
	pos = pos_;
	size = size_;
	cols = cols_;
}

ImVec2 FixedArea::SetPos(int row_, int col_) {
	float x = (size.x / cols) * col_;
	float y = row_ * 20;
	row = row_; // just that NextRow() works
	ImGui::SetCursorPos(pos + ImVec2(x, y));
	return ImVec2(size.x / cols, 20);
}

ImVec2 FixedArea::SetPosLeft(int row_, int col_) {
	float x = (size.x / cols) * col_;
	float y = row * 20; // not row_, that argument needs to be deleted...
	//row = row_; // just that NextRow() works
	//ImGui::SetCursorPosX(pos.x + x);
	ImGui::SetCursorPos(pos + ImVec2(x, y));
	return ImVec2(size.x / cols, 20);
}

void FixedArea::NextRow()
{
	row++;
}

ImVec2 FixedArea::SetCol(int col_)
{
	return SetPos(row, col_);
}

float FixedArea::GetColWidth()
{
	return size.x / cols;
}



namespace ImGui {
	// return 1 == changed
	int PrintMatrix(FixedArea *fa, float *matrix) {
		int changed = 0;
		ImGui::PushItemWidth(fa->GetColWidth());
		for (int i=0; i<4; i++) {
			for (int j=0; j<4; j++) {
				fa->SetPosLeft(i, j);
				// the matrix itself is correct in memory, but we need to display it "column mayor"
				// http://stackoverflow.com/questions/17717600/confusion-between-c-and-opengl-matrix-order-row-major-vs-column-major
				float *address = matrix + (j*4) + i;
				ImGui::PushID(address);
				changed += (int)ImGui::DragFloat("", address, 0.1f, 0.0f, 0.0f, "%.4f");
				ImGui::PopID();
			}
			fa->NextRow();
		}
		ImGui::PopItemWidth();
		return changed;
	}

	// return 1 == changed
	int PrintVector4(FixedArea *fa, float *vector4) {
		int changed = 0;
		ImGui::PushItemWidth(fa->GetColWidth());
		for (int i=0; i<4; i++) {
			fa->SetPos(0, i);
			float *address = vector4 + i;
			ImGui::PushID(address);
			changed += (int)ImGui::DragFloat("", address, 0.1f, 0.0f, 0.0f, "%.4f");
			ImGui::PopID();
		}
		ImGui::PopItemWidth();
		return changed;
	}

	// return 1 == changed
	int PrintVector3(FixedArea *fa, float *vector3) {
		int changed = 0;
		ImGui::PushItemWidth(fa->GetColWidth());
		for (int i=0; i<3; i++) {
			fa->SetPosLeft(0, i);
			float *address = vector3 + i;
			ImGui::PushID(address);
			changed += (int)ImGui::DragFloat("", address, 0.1f, 0.0f, 0.0f, "%.4f");
			ImGui::PopID();
		}
		ImGui::PopItemWidth();
		return changed;
	}

		// return 1 == changed
	//int PrintQuat(FixedArea *fa, glm::quat &quat) {
	//	int changed = 0;
	//	ImGui::PushItemWidth(fa->GetColWidth());
	//	for (int i=0; i<4; i++) {
	//		fa->SetPos(0, i);
	//		float *address;
	//		switch (i) {
	//			case 0: address = &quat.w; break;
	//			case 1: address = &quat.x; break;
	//			case 2: address = &quat.y; break;
	//			case 3: address = &quat.z; break;
	//		}
	//		ImGui::PushID(address);
	//		changed += (int)ImGui::DragFloat("", address, 0.1f, 0.0f, 0.0f, "%.4f");
	//		ImGui::PopID();
	//	}
	//	ImGui::PopItemWidth();
	//	return changed;
	//}




	int FastDragValue(float *address) {
		ImGui::PushID(address);
		bool changed = ImGui::DragFloat("", address);
		ImGui::PopID();
		return changed;
	}
	int FastDragValue(int *address) {
		ImGui::PushID(address);
		bool changed = ImGui::DragInt("", address);
		ImGui::PopID();
		return changed;
	}


}
