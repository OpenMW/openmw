/*  Support routines for the Pawn Abstract Machine
 *
 *  Copyright (c) ITB CompuPhase, 2003-2011
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy
 *  of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations
 *  under the License.
 *
 *  Version: $Id: amxaux.h 4523 2011-06-21 15:03:47Z thiadmer $
 */
#ifndef AMXAUX_H_INCLUDED
#define AMXAUX_H_INCLUDED

#include "amx.h"

#ifdef  __cplusplus
extern  "C" {
#endif

/* loading and freeing programs */
size_t AMXAPI aux_ProgramSize(const char *filename);
int AMXAPI aux_LoadProgram(AMX *amx, const char *filename, void *memblock);
int AMXAPI aux_FreeProgram(AMX *amx);

/* a readable error message from an error code */
char * AMXAPI aux_StrError(int errnum);

enum {
  CODE_SECTION,
  DATA_SECTION,
  HEAP_SECTION,
  STACK_SECTION,
  /* ----- */
  NUM_SECTIONS
};
int AMXAPI aux_GetSection(const AMX *amx, int section, cell **start, size_t *size);

#ifdef  __cplusplus
}
#endif

#endif /* AMXAUX_H_INCLUDED */
