
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


#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <stdtypes.h>
#include <mm.h>
#include <string.h>
#include <arrayu8.h>
#include <arrayuint.h>

#include "file.h"

#include "cwd.file.c"
#include "mkdir.file.c"
#include "is.file.c"
#include "remove.file.c"
#include "dirname.file.c"
#include "basename.file.c"
#include "timestamp.file.c"
#include "relativepath.file.c"
#include "absolutepath.file.c"
#include "canonicalpath.file.c"
#include "lsdir.file.c"
#include "read.file.c"
#include "write.file.c"
#include "getline.file.c"
