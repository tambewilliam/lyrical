
// ---------------------------------------------------------------------
// Copyright (c) William Fonkou Tambe
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------


#include <stdtypes.h>
#include <byt.h>
#include <mm.h>

#include "array.h"

// Null value useful for initialization.
#define ARRAYTYPENULL ((ARRAYTYPE){.ptr = 0})

#include "duplicate.array.c"
#include "append.array.c"
#include "insert.array.c"
#include "overwrite.array.c"
#include "split.array.c"
#include "remove.array.c"
#include "truncate.array.c"
#include "chop.array.c"
#include "iterate.array.c"
#include "free.array.c"

#undef ARRAYTYPENULL
