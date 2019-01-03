#include "tr_local.h"

#ifdef __INSTANCED_MODELS__
void setupInstancedVertexAttributes(mdvModel_t *m);
void drawModelInstanced(mdvModel_t *m, GLuint count, vec3_t *positions, vec3_t *angles, matrix_t MVP);
#endif //__INSTANCED_MODELS__
