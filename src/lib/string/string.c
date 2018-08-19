
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


#include <stdarg.h>

#include <stdtypes.h>

#include <mm.h>
#include <string.h>
#include <byt.h>

// When this macro is defined,
// an actual copy is done when
// duplicating a string instead
// of using mmrefup().
//#define STRINGNOREFCNT

#include "size.string.c"
#include "duplicate.string.c"
#include "append.string.c"
#include "insert.string.c"
#include "overwrite.string.c"
#include "remove.string.c"
#include "truncate.string.c"
#include "chop.string.c"
#include "split.string.c"
#include "isequal.string.c"
#include "search.string.c"
#include "lowercase.string.c"
#include "uppercase.string.c"
#include "convert.string.c"
#include "fmt.string.c"
