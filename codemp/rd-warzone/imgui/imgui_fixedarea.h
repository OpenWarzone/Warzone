
#ifndef IMGUI_FIXEDAREA_H
#define IMGUI_FIXEDAREA_H

#include "include_imgui.h"
//#include <kung/include_glm.h>

class FixedArea {
public:
	ImVec2 pos;
	ImVec2 size;
	int cols;
	int row = 0;
	FixedArea(ImVec2 pos_, ImVec2 size_, int cols_);
	ImVec2 SetPos(int row, int col);
	ImVec2 SetPosLeft(int row_, int col_);
	void NextRow();
	ImVec2 SetCol(int col);
	float GetColWidth();
};

namespace ImGui {
	// return 1 == changed
	int PrintMatrix(FixedArea *fa, float *matrix);
	int PrintVector3(FixedArea *fa, float *vector3);
	int PrintVector4(FixedArea *fa, float *vector4);
	int FastDragValue(float *address);
	int FastDragValue(int *address);
	//int PrintQuat(FixedArea *fa, glm::quat &quat);
}

#endif