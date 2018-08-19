
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


// File which implement Lyrical syscalls.

// Function which implement Lyrical syscalls.
#if defined(LYRICALX86) || defined(LYRICALX86LINUX) || defined(LYRICALX86CYGWIN)
__attribute__((cdecl))
#elif defined(LYRICALX64) || defined(LYRICALX64LINUX) || defined(LYRICALX64CYGWIN)
__attribute__((sysv_abi))
#endif
uint lyricalsyscalls (
	uint syscallarg0, uint syscallarg1, uint syscallarg2,
	uint syscallarg3, uint syscallarg4, uint syscallarg5,
	uint syscallarg6) {
	
	switch (syscallarg0) {
		
		case LYRICALSYSCALLEXIT: {
			
			exit(syscallarg1);
			
			break;
		}
		
		case LYRICALSYSCALLCREAT: {
			
			return creat(
				(char*)syscallarg1,
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLOPEN: {
			
			return open(
				(char*)syscallarg1,
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLCLOSE: {
			
			return close(
				syscallarg1
			);
			
			break;
		}
		
		case LYRICALSYSCALLREAD: {
			
			return read(
				syscallarg1,
				(void*)syscallarg2,
				syscallarg3
			);
			
			break;
		}
		
		case LYRICALSYSCALLWRITE: {
			
			return write(
				syscallarg1,
				(void*)syscallarg2,
				syscallarg3
			);
			
			break;
		}
		
		case LYRICALSYSCALLSEEK: {
			
			return lseek(
				syscallarg1,
				syscallarg2,
				syscallarg3
			);
			
			break;
		}
		
		case LYRICALSYSCALLMMAP: {
			
			return (uint)mmap(
				(void*)syscallarg1,
				syscallarg2,
				syscallarg3,
				syscallarg4,
				syscallarg5,
				syscallarg6
			);
			
			break;
		}
		
		case LYRICALSYSCALLMUNMAP: {
			
			return munmap(
				(void*)ROUNDDOWNTOPOWEROFTWO(syscallarg1, -0x1000), // PAGESIZE == 0x1000
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLMPROTECT: {
			
			return mprotect(
				(void*)syscallarg1,
				syscallarg2,
				syscallarg3
			);
			
			break;
		}
		
		case LYRICALSYSCALLGETCWD: {
			
			return (uint)getcwd(
				(char*)syscallarg1,
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLMKDIR: {
			
			return mkdir(
				(char*)syscallarg1,
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLCHDIR: {
			
			return chdir(
				(char*)syscallarg1
			);
			
			break;
		}
		
		case LYRICALSYSCALLFCHDIR: {
			
			return fchdir(
				syscallarg1
			);
			
			break;
		}
		
		case LYRICALSYSCALLRENAME: {
			
			return rename(
				(char*)syscallarg1,
				(char*)syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLLINK: {
			
			return link(
				(char*)syscallarg1,
				(char*)syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLSYMLINK: {
			
			return symlink(
				(char*)syscallarg1,
				(char*)syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLREADLINK: {
			
			return readlink(
				(char*)syscallarg1,
				(char*)syscallarg2,
				syscallarg3
			);
			
			break;
		}
		
		case LYRICALSYSCALLFORK: {
			
			return fork();
			
			break;
		}
		
		case LYRICALSYSCALLVFORK: {
			
			return vfork();
			
			break;
		}
		
		case LYRICALSYSCALLUNLINK: {
			
			return unlink(
				(char*)syscallarg1
			);
			
			break;
		}
		
		case LYRICALSYSCALLRMDIR: {
			
			return rmdir(
				(char*)syscallarg1
			);
			
			break;
		}
		
		case LYRICALSYSCALLSTAT: {
			
			return stat(
				(char*)syscallarg1,
				(struct stat*)syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLFSTAT: {
			
			return fstat(
				syscallarg1,
				(struct stat*)syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLLSTAT: {
			
			return lstat(
				(char*)syscallarg1,
				(struct stat*)syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLACCESS: {
			
			return access(
				(char*)syscallarg1,
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLCHOWN: {
			
			return chown(
				(char*)syscallarg1,
				syscallarg2,
				syscallarg3
			);
			
			break;
		}
		
		case LYRICALSYSCALLFCHOWN: {
			
			return fchown(
				syscallarg1,
				syscallarg2,
				syscallarg3
			);
			
			break;
		}
		
		case LYRICALSYSCALLLCHOWN: {
			
			return lchown(
				(char*)syscallarg1,
				syscallarg2,
				syscallarg3
			);
			
			break;
		}
		
		case LYRICALSYSCALLCHMOD: {
			
			return chmod(
				(char*)syscallarg1,
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLFCHMOD: {
			
			return fchmod(
				syscallarg1,
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLTRUNCATE: {
			
			return truncate(
				(char*)syscallarg1,
				syscallarg2
			);
			
			break;
		}
		
		case LYRICALSYSCALLFTRUNCATE: {
			
			return ftruncate(
				syscallarg1,
				syscallarg2
			);
			
			break;
		}
	}
	
	return -1;
}

// Install Lyrical syscalls.
globalvarregion[2] = (void*)lyricalsyscalls;
