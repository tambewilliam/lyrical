
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


// Function which retrieve
// the line of source code
// from which the binary,
// at the offset given by
// the argument boff,
// was generated.
// The argument dbgfilepath
// is the path to the debugging
// information file to use.
// Null is returned if an error
// occured, or the binary offset
// given as argument is out of range.
lyricaldbg64getlineresult lyricaldbg64getline (u8* dbgfilepath, u64 boff) {
	
	lyricaldbg64getlineresult result = {
		.ltxt = 0,
		.path = 0,
		.lnum = 0
	};
	
	uint fid;
	
	if ((fid = open(dbgfilepath, O_RDONLY)) == -1) return result;
	
	// Variable which will hold the size
	// in bytes of section1 debug information.
	u64 dbginfosection1sz;
	
	// Read the size in bytes of section1 debug information.
	if (read(fid, &dbginfosection1sz, sizeof(u64)) != sizeof(u64)) {
		
		close(fid);
		
		return result;
	}
	
	// Compute the offset of
	// section2 debug information.
	u64 dbginfosection2offset = dbginfosection1sz + sizeof(u64);
	
	// Structure describing an entry
	// from section1 debug information.
	typedef struct  {
		
		u64 boff;
		u64 path;
		u64 lnum;
		u64 loff;
		
	} dbginfosection1;
	
	dbginfosection1 dbginfosection1entry;
	
	// This loop go through each entry
	// from section1 debug information,
	// and subsequently access
	// section2 debug information.
	do {
		if (read(fid, &dbginfosection1entry, sizeof(dbginfosection1)) != sizeof(dbginfosection1)) {
			
			close(fid);
			
			return result;
		}
		
		if (boff < dbginfosection1entry.boff) {
			// If I get here, the preceding entry
			// is the one for the boff value.
			
			if (lseek(fid, -2*sizeof(dbginfosection1), SEEK_CUR) == -1) {
				
				close(fid);
				
				return result;
			}
			
			if (read(fid, &dbginfosection1entry, sizeof(dbginfosection1)) != sizeof(dbginfosection1)) {
				
				close(fid);
				
				return result;
			}
			
			// Checking that the entry was not
			// the one used to mark the end
			// of section1 debug information.
			if (!dbginfosection1entry.lnum) {
				
				close(fid);
				
				return result;
			}
			
			// I retrieve the path from
			// section2 debug information.
			
			// Compute the offset of the path
			// within section2 debug information.
			u64 filepathoffset = dbginfosection2offset + sizeof(u64) + dbginfosection1entry.path;
			
			if (lseek(fid, filepathoffset, SEEK_SET) != filepathoffset) {
				
				close(fid);
				
				return result;
			}
			
			string path = stringnull;
			
			// This loop read each path character
			// until the null-terminating byte is found.
			while (1) {
				
				u8 c;
				
				if (read(fid, &c, 1) != 1) {
					
					if (path.ptr) mmfree(path.ptr);
					
					close(fid);
					
					return result;
				}
				
				if (!c) break;
				
				stringappend4(&path, c);
			}
			
			close(fid);
			
			// When I get here, I am done with
			// the debug information file.
			// I now open the source code file
			// and retrieve the line of text.
			
			string srclinetext = stringduplicate2("");
			
			if ((fid = open(path.ptr, O_RDONLY)) != -1) {
				
				if (lseek(fid, dbginfosection1entry.loff, SEEK_SET) == dbginfosection1entry.loff) {
					// This loop read each character
					// from the source code line.
					while (1) {
						
						u8 c;
						
						uint n;
						
						if ((n = read(fid, &c, 1)) == -1) break;
						
						if (!n || c == '\n') break;
						
						stringappend4(&srclinetext, c);
					}
					
					// When I get here, I am done retrieving
					// the line of text from the source code file.
				}
				
				close(fid);
			}
			
			result.ltxt = srclinetext.ptr;
			result.path = path.ptr;
			result.lnum = dbginfosection1entry.lnum;
			
			return result;
		}
		
	} while (dbginfosection1sz -= sizeof(dbginfosection1));
	
	close(fid);
	
	return result;
}
