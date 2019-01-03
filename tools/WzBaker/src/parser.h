#include "objTypes.h"

void loadObj(const std::string& filename, Mesh& result);
void loadModel(const std::string& filename, Mesh& result);
void writeObj(const std::string& filename, const std::string matFilename, const Mesh& mesh);

#if defined(WIN32) || defined(WIN64)
#include <Windows.h>
#include <wincon.h>

enum concol
{
	concol_black = 0,
	concol_dark_blue = 1,
	concol_dark_green = 2,
	concol_dark_aqua, concol_dark_cyan = 3,
	concol_dark_red = 4,
	concol_dark_purple = 5, concol_dark_pink = 5, concol_dark_magenta = 5,
	concol_dark_yellow = 6,
	concol_dark_white = 7,
	concol_gray = 8, concol_grey = 8,
	concol_blue = 9,
	concol_green = 10,
	concol_aqua = 11, concol_cyan = 11,
	concol_red = 12,
	concol_purple = 13, concol_pink = 13, concol_magenta = 13,
	concol_yellow = 14,
	concol_white = 15
};

#define Q_COLOR_ESCAPE	'^'
#define Q_COLOR_BITS 0xF // was 7

// you MUST have the last bit on here about colour strings being less than 7 or taiwanese strings register as colour!!!!
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE && *((p)+1) <= '9' && *((p)+1) >= '0' )
// Correct version of the above for Q_StripColor
#define Q_IsColorStringExt(p)	((p) && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) >= '0' && *((p)+1) <= '9') // ^[0-9]


#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define COLOR_ORANGE	'8'
#define COLOR_GREY		'9'
#define ColorIndex(c)	( ( (c) - '0' ) & Q_COLOR_BITS )

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"
#define S_COLOR_ORANGE	"^8"
#define S_COLOR_GREY	"^9"
#endif

//extern inline void setcolor(concol textcol,concol backcol);
extern inline void setcolor(int textcol, int backcol);
//
