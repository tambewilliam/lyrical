
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


u8* savedcurpos = curpos;

while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;

parsefilepath:;

string modulename = stringnull;
string modulepath = stringnull;

if (*curpos != '"') {
	curpos = savedcurpos;
	throwerror("expecting a double-quoted module/file path");
}

savedcurpos = curpos;

string filepath = readstringconstant(STRINGUSEDBYPREPROCESSOR);

if (!filepath.ptr) throwerror("expecting a module/file path");

string duplicatedfilepath = stringduplicate1(filepath);

u8* savedsource = compileargsource;

// Variables which get set to a non-null
// value to signify that an attempt
// to include a corresponding .lyc file
// is to be done.
// The value they are set to is 1 or the index+1
// within compilearg->standardparths when
// including from a standard include directory.
uint trytoincludemodulelyc = 0;
uint ismodule = 0;

// Variable which get set to 1 if
// the file to include is an .lyx file.
uint islyx = 0;

includefilepath:

if (modulename.ptr) {
	mmrefdown(modulename.ptr);
	modulename = stringnull;
}

// Function used to check whether the path
// given as argument already exist among
// the list of included modules.
uint ismoduleincludedalready (string path) {
	
	string s = stringduplicate1(path);
	
	// Prepend the current working directory
	// if the path is not absolute.
	if (s.ptr[0] != '/') {
		stringinsert1(&s, initialcwd, 0);
	}
	
	if (modulepath.ptr) mmrefdown(modulepath.ptr);
	
	modulepath = filecanonicalpath(s.ptr);
	
	mmrefdown(s.ptr);
	
	uint n = arrayuintsz(alreadyincludedmodules);
	
	while (n) {
		if (stringiseq3(modulepath.ptr, (u8*)alreadyincludedmodules.ptr[--n])) return 1;
	}
	
	return 0;
}

// If the file path start with "/", the file
// to include is loaded from the root of the filesystem.
// If the file path start with "./" or "../",
// the file to include is loaded from the current
// directory containing the file in which
// the preprocessor directive is used.
// If the file path do not start with either
// "/", "./" or "../", the file is searched
// in the standard directories.
if (filepath.ptr[0] == '/') {
	// If the file to include is a folder,
	// a file inside itself with the same name
	// and extension .lyh is included; and attempt
	// to load the corresponding .lyc is done.
	if (trytoincludemodulelyc || fileis2(filepath.ptr) == FILEISFOLDER) {
		
		modulename = filebasename(filepath.ptr);
		
		if (!trytoincludemodulelyc && ismoduleincludedalready(filepath)) goto skipincluding;
		
		if (filepath.ptr[stringmmsz(filepath)-1] != '/') stringappend4(&filepath, '/');
		
		stringappend1(&filepath, modulename);
		
		if (trytoincludemodulelyc) stringappend2(&filepath, ".lyc");
		else {
			stringappend2(&filepath, ".lyh");
			
			ismodule = 1;
		}
	}
	
	compileargsource = fileread1(filepath.ptr).ptr;
	
	if (compileargsource) {
		// 4 stand for the length of ".lyx" .
		islyx = stringiseq3(filepath.ptr+(stringmmsz(filepath)-4), ".lyx");
		
	} else {
		// Skip the inclusion of a module .lyc file if it do not exist.
		if (trytoincludemodulelyc && stringiseq3(filepath.ptr+(stringmmsz(filepath)-4), ".lyc"))
			goto skipincluding;
		
		curpos = savedcurpos;
		
		compileargsource = savedsource;
		
		throwerror("could not include");
	}
	
} else if ((filepath.ptr[0] == '.' && filepath.ptr[1] == '/') ||
	(filepath.ptr[0] == '.' && filepath.ptr[1] == '.' && filepath.ptr[2] == '/')) {
	
	string s;
	
	// For a nicier result, if the path in cwd
	// is ".", I set cwd to stringnull.
	if (stringmmsz(cwd) == 1 && cwd.ptr[0] == '.') {
		
		mmrefdown(cwd.ptr);
		
		cwd = stringnull;
		
		s = stringnull;
		
	} else s = stringduplicate1(cwd);
	
	if (s.ptr && s.ptr[stringmmsz(s)-1] != '/') stringappend4(&s, '/');
	
	stringappend1(&s, filepath);
	
	mmrefdown(filepath.ptr);
	
	filepath = s;
	
	// If the file to include is a folder,
	// a file inside itself with the same name
	// and extension .lyh is included; and attempt
	// to load the corresponding .lyc is done.
	if (trytoincludemodulelyc || fileis2(filepath.ptr) == FILEISFOLDER) {
		
		modulename = filebasename(filepath.ptr);
		
		if (!trytoincludemodulelyc && ismoduleincludedalready(filepath)) goto skipincluding;
		
		if (filepath.ptr[stringmmsz(filepath)-1] != '/') stringappend4(&filepath, '/');
		
		stringappend1(&filepath, modulename);
		
		if (trytoincludemodulelyc) stringappend2(&filepath, ".lyc");
		else {
			stringappend2(&filepath, ".lyh");
			
			ismodule = 1;
		}
	}
	
	compileargsource = fileread1(filepath.ptr).ptr;
	
	if (compileargsource) {
		// 4 stand for the length of ".lyx" .
		islyx = stringiseq3(filepath.ptr+(stringmmsz(filepath)-4), ".lyx");
		
	} else {
		// Skip the inclusion of a module .lyc file if it do not exist.
		if (trytoincludemodulelyc && stringiseq3(filepath.ptr+(stringmmsz(filepath)-4), ".lyc"))
			goto skipincluding;
		
		curpos = savedcurpos;
		
		compileargsource = savedsource;
		
		throwerror("could not include");
	}
	
} else if (compilearg->standardpaths) {
	
	u8* path;
	
	uint retrymoduleinclude = 0;
	
	retrymoduleinclusion:;
	
	uint i = (trytoincludemodulelyc ? trytoincludemodulelyc-1 : 0);
	
	while (path = compilearg->standardpaths[i]) {
		// Within this loop I iterate through all elements
		// of the array compilearg->standardpaths attempting
		// to load the file to include.
		
		string s;
		
		// ### This check is not necessary
		// ### as a standard path must be
		// ### an absolute path.
		if (*path != '/') {
			// Note that '/' has already
			// been appended to initialcwd.
			s = stringduplicate1(initialcwd);
			
			stringappend2(&s, path);
			
		} else s = stringduplicate2(path);
		
		if (s.ptr[stringmmsz(s)-1] != '/') stringappend4(&s, '/');
		
		stringappend1(&s, filepath);
		
		// If the file to include is a folder,
		// a file inside itself with the same name
		// and extension .lyh is included; and attempt
		// to load the corresponding .lyc is done.
		if (trytoincludemodulelyc || fileis2(s.ptr) == FILEISFOLDER) {
			
			modulename = filebasename(s.ptr);
			
			if (!trytoincludemodulelyc && ismoduleincludedalready(s)) {
				
				mmrefdown(s.ptr);
				
				goto skipincluding;
			}
			
			if (s.ptr[stringmmsz(s)-1] != '/') stringappend4(&s, '/');
			
			stringappend1(&s, modulename);
			
			if (trytoincludemodulelyc) stringappend2(&s, ".lyc");
			else {
				stringappend2(&s, ".lyh");
				
				ismodule = i+1;
			}
		}
		
		compileargsource = fileread1(s.ptr).ptr;
		
		if (compileargsource) {
			// 4 stand for the length of ".lyx" .
			islyx = stringiseq3(filepath.ptr+(stringmmsz(filepath)-4), ".lyx");
			
			mmrefdown(filepath.ptr);
			
			filepath = s;
			
			goto standardpathinclusiondone;
			
		} else if (trytoincludemodulelyc) {
			// Skip the inclusion of a module .lyc file when it do not exist.
			
			mmrefdown(s.ptr);
			
			goto skipincluding;
			
		} else mmrefdown(s.ptr);
		
		if (modulename.ptr) {
			
			mmrefdown(modulename.ptr);
			
			modulename = stringnull;
		}
		
		++i;
	}
	
	// If I get here, I could not find the file
	// to include among the standard paths.
	
	if (!retrymoduleinclude && !trytoincludemodulelyc) {
		
		retrymoduleinclude = 1;
		
		if (compilearginstallmissingmodule) {
			if (compilearginstallmissingmodule(filepath.ptr))
				goto retrymoduleinclusion;
		}
	}
	
	curpos = savedcurpos;
	
	compileargsource = savedsource;
	
	throwerror("could not include");
	
	standardpathinclusiondone:;
	
} else {
	
	curpos = savedcurpos;
	
	throwerror("no standard include directory available");
}

// Prepend the current working directory.
// If the path is not absolute.
if (filepath.ptr[0] != '/') {
	stringinsert1(&filepath, initialcwd, 0);
}
string savedfilepath = filepath;
filepath = filecanonicalpath(savedfilepath.ptr);
mmrefdown(savedfilepath.ptr);

stringappend1(&compileresult.srcfilepaths, filepath);
stringappend4(&compileresult.srcfilepaths, '\n');

// This structure represent an included file.
typedef struct includedfile {
	// This field is set to a string that
	// is the filepath and linenumber where
	// the include directive was used. ei:
	// filepath.lyc:123.
	string origin;
	
	// This field is set to the filepath.
	string filepath;
	
	// The field prev of the first element
	// of the linkedlist is null.
	struct includedfile* prev;
	
} includedfile;

// This variable keep in a linkedlist,
// includedfile that represent files currently
// being included and still being preprocessed.
static includedfile* includedfiles = 0;

if (includedfiles) {
	// If I get here, I make sure that the file
	// inclusion is not a recursive inclusion.
	
	includedfile* i = includedfiles;
	
	do {
		if (stringiseq1(filepath, i->filepath)) {
			
			curpos = savedcurpos;
			
			compileargsource = savedsource;
			
			// Code disabled, as throwerror() already
			// generate the include backtrace.
			#if 0
			string s = stringduplicate2("error; file included from:\n");
			
			i = includedfiles;
			
			do {
				stringappend4(&s, '\t');
				stringappend1(&s, i->origin);
				stringappend4(&s, '\n');
				
			} while (i = i->prev);
			
			stringappend2(&s, "recursive include");
			
			throwerror(s.ptr);
			#endif
			
			throwerror("recursive include");
			
			// The string pointed by s.ptr will be freed
			// when throwerror() throw labelforerror.
		}
		
	} while (i = i->prev);
}

// This function create a new includedfile
// and add it to the linkedlist pointed
// by the variable includedfiles.
void addincludedfile () {
	
	includedfile* newincludedfile = mmalloc(sizeof(includedfile));
	
	newincludedfile->filepath = filepath;
	
	newincludedfile->origin = stringfmt("%s:%d", currentfilepath.ptr, countlines(savedsource, savedcurpos));
	
	newincludedfile->prev = includedfiles;
	
	includedfiles = newincludedfile;
}

addincludedfile();

string savedcwd = cwd;

cwd = filedirname(filepath.ptr);

u8* savedcurpos2 = curpos;

curpos = compileargsource;

string savedcurrentfilepath = currentfilepath;

currentfilepath = filepath;

macro* savedmacros = macros;

chunk* c;

if (c = preprocessor(islyx ? PREPROCESSINCLUDELYX : PREPROCESSINCLUDE)) {
	#if 0
	// ### Not sure whether this code is still necessary.
	// To the last chunk of the preprocessing result,
	// I append a space for safety; in fact it allow to
	// properly display the location of an error whenever
	// a file unexpectedly end and curpos is set
	// right after the error. ei: Ending a file right
	// after the closing double quote character of
	// a constant string will yield curpos set right
	// after the double quote character which is
	// the last character of the file.
	stringappend2(&c->prev->content, " ");
	#endif
	
	c->prev->first = c;
	
	if (trytoincludemodulelyc) {
		// The first chunk from an included file
		// is always a chunk created for the purpose
		// of inserting useful characters; in this case,
		// I append a string to open the scope that will
		// nest the content of the .lyc file.
		// I also insert the character sequence
		// to infer exporting of
		// functions/operators definitions.
		stringappend2(&c->content, "\n#+e{\n");
		// To the last chunk, I append a string
		// to close the scope that was used
		// to nest the content of the .lyc file.
		// I also insert the character sequence
		// to stop export inference
		// on functions/operators definitions.
		stringappend2(&c->prev->content, "\n}#-e\n");
	}
	
	// I attach the returned linkedlist
	// to the appropriate chunk linkedlist.
	if (ismodule || trytoincludemodulelyc)
		attachmodulechunks(c);
	else attachchunks(c);
}

// ### Block of code commented-out
// ### but kept as reference code.
// ### macros defined within
// ### a module .lyc file must not
// ### be freed unless locally defined.
/*if (trytoincludemodulelyc) {
	// Free macros that were defined
	// within the module .lyc file,
	// as they must not be visible
	// outside of that file.
	while (macros != savedmacros)
		freemacro(macros);
	
} else */{
	// Free macros that were locally
	// defined within the included file,
	// as they must not be visible
	// outside of that file.
	
	macro* m = macros;
	macro* oldm = 0;
	
	while (m != savedmacros) {
		if (m->islocal) {
			freemacro(m);
			m = oldm ?: macros;
		} else {
			oldm = m;
			m = m->next;
		}
	}
}

mmrefdown(compileargsource);

currentfilepath = savedcurrentfilepath;

curpos = savedcurpos2;

mmrefdown(cwd.ptr);

cwd = savedcwd;

// I remove the includedfile that was created
// for the file that has been included.
includedfile* savedincludedfile = includedfiles;
includedfiles = includedfiles->prev;
mmrefdown(savedincludedfile->origin.ptr);
mmrefdown(savedincludedfile);

if (ismodule && !trytoincludemodulelyc) {
	
	trytoincludemodulelyc = ismodule; // Note that ismodule can be a value other than 1.
	ismodule = 0;
	
	mmrefdown(filepath.ptr);
	
	filepath = stringduplicate1(duplicatedfilepath);
	
	// I register the module inclusion
	// for use by ismoduleincludedalready().
	*arrayuintappend1(&alreadyincludedmodules) = (uint)modulepath.ptr;
	modulepath = stringnull;
	
	goto includefilepath;
}

skipincluding:

compileargsource = savedsource;

mmrefdown(duplicatedfilepath.ptr);

mmrefdown(filepath.ptr);

if (modulename.ptr) mmrefdown(modulename.ptr);

if (modulepath.ptr) mmrefdown(modulepath.ptr);

while (*curpos == '\x0d' || *curpos == ' ' || *curpos == '\t') ++curpos;

if (*curpos) {
	
	if (*curpos != '\n' &&
		(action != PREPROCESSINCLUDELYX ||
			curpos[0] != '%' || curpos[1] != '>')) {
		goto parsefilepath;
	}
}
