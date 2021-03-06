//////////////////////////////////////////////////////////////////////
// 																	//
//	      				File Saving Library							//
//					Paul Ganssle, 03/05/2012						//
//																	//
//	This library is intended to create a simple binary file that	//
// saves the data and the programs that are passed to it, rather 	//
// than relying on over-complicated TDM files and the like.			//
// 																	//
//////////////////////////////////////////////////////////////////////

#include <ansi_c.h>


#include <FileSave.h>
#include <ErrorDefs.h>
#include <ErrorLib.h>
#include <General.h>


/*************** File Read-Write ********************/
int fwrite_fs(FILE *f, fsave *fs) {
	if(f == NULL) { return MCF_ERR_NOFILE; }
	if(fs == NULL) { return MCF_ERR_NOFSAVE; }
	
	int rv = 0;
	unsigned int bsize = get_fs_strlen(fs);
	char *buff = NULL;
	
	if(bsize > 0) {
		buff = malloc(bsize);	
	} else {
		rv = MCF_ERR_NOFSAVE;
		goto error;
	}
	
	print_fs(buff, fs);
	
	if(fwrite(buff, 1, bsize, f) < bsize) {
		rv = MCF_ERR_FILEWRITE;
		goto error;
	}
	
	error:
	
	if(buff != NULL) { free(buff); }
	
	return rv;
}

int fadd_fs_to_container(FILE *f, fsave *fs) {
	// Point file to an existing container fsave and pass this a new
	// fsave to be contained within the container. This function will
	// append the new fsave at the end of the container.
	
	if(f == NULL) { return MCF_ERR_NOFILE; }
	if(fs == NULL) { return MCF_ERR_NOFSAVE; }
	
	int rv = 0;
	char *buff = NULL;
	
	unsigned int bsize = get_fs_strlen(fs);
	if(bsize > 0) {
		buff = malloc(bsize);
		print_fs(buff, fs);
		
		if(rv = fappend_data_to_fs(f, buff, bsize)) { goto error; }
	} else {
		rv = MCF_ERR_NODATA;
		goto error;
	}
	
	error:
	if(buff != NULL) { free(buff); }
	
	return rv;
}

int fappend_data_to_fs(FILE *f, void *data, unsigned int data_size) {
	// Append data to an existing fsave. In this case, the file must
	// be pointing at the beginning of the fsave header.
	//
	// File pointer is returned to original position.
	
	if(f == NULL) { return MCF_ERR_NOFILE; }
	if(data == NULL || data_size == 0) { return MCF_ERR_NODATA; }
	
	int rv = 0;
	fsave bfs = null_fs();
	
	long pos = ftell(f);
	
	// Get the header - afterwards pointer will be at the beginning of the data.
	if((rv = get_fs_header_from_file(f, &bfs)) != 0) { goto error; }
	fseek(f, -1*sizeof(unsigned int), SEEK_CUR);
	unsigned int new_size = bfs.size + data_size;
	
	if(fwrite(&new_size, sizeof(unsigned int), 1, f) < 1) { 
		rv = MCF_ERR_FILEWRITE;
		goto error;
	}
	
	fseek(f, bfs.size, SEEK_CUR);
	
	if(rv = insert_into_file(f, data, data_size, -1)) { goto error; }
	
	error:
	free_fsave(&bfs);		
	fseek(f, pos, SEEK_SET);
	
	return rv;
}

int overwrite_fsave_in_file(FILE *f, fsave *fs) {
	// Pointer must be a pointer to the beginning of the fsave, so that
	// fsave checking can be done.
	// 
	// File will not be rewound.
	
	if(f == NULL) { return MCF_ERR_NOFILE;	}
	if(fs == NULL) { return MCF_ERR_NODATA; }
	
	int rv = 0;
	fsave fs_buff = null_fs();
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	
	if(rv = get_fs_header_from_file(f, &fs_buff)) { goto error; }
	
	if(fs->name == NULL || fs_buff.name == NULL || strcmp(fs->name, fs_buff.name) != 0) {
		rv = MCF_ERR_BAD_FSAVE_REPLACEMENT_NAME;
		goto error;
	}
	
	if(fs->type != fs_buff.type) {
		rv = MCF_ERR_FSAVE_TYPE_MISMATCH;
		goto error;
	}
	
	if(fs->size != fs_buff.size) {
		rv = MCF_ERR_FSAVE_SIZE_MISMATCH;
		goto error;
	}

	fwrite(fs->val.c, 1, fs->size, f); 

	error:

	fs_buff = free_fsave(&fs_buff);
	return rv;
}

int find_and_overwrite_fsave_in_file(FILE *f, fsave *fs, unsigned int max_bytes) {
	// Same as overwrite_fsave_in_file, but it will first find the fsave
	// in the file. Does not do a tree search, so it must be a top-level
	// fsave.

	long pos = find_fsave_in_file(f, fs->name, max_bytes);
	
	if(pos < 0) {	// Error condition
		return pos;	
	}
	
	fseek(f, pos, SEEK_SET);
	return overwrite_fsave_in_file(f, fs);
}


/*************** FSave Functions ********************/
fsave make_fs(char *name) {
	if(name == NULL) {
		name = "";
	}
	
	fsave f = {
		.name = malloc(strlen(name)+1),
		.ns = strlen(name)+1	// Include null character
	};
	
	strcpy(f.name, name);

	return f;
}

int put_fs(fsave *fs, void *val, unsigned int type, unsigned int NumElements) {
	if(fs == NULL) { return MCF_ERR_NOFSAVE; }
	if(!is_valid_fs_type(type)) { return MCF_ERR_FS_NOTYPE; }
	
	fs->type = type;
	fs->size = get_fs_type_size(type)*NumElements;
	
	// Don't try to cast a null pointer.
	if(val == NULL) { 
		fs->val.c = NULL; 
		return 0;
	} else if(NumElements == 0) { 
		return MCF_ERR_EMPTYVAL; 
	}  
	
	// May be able to remove this step by adding a void* in the union.
	switch(type) {
		case FS_CHAR:
		case FS_CONTAINER:
		case FS_CUSTOM:
			fs->val.c = (char *)val;
			break;
		case FS_UCHAR:
			fs->val.uc = (unsigned char *)val;
			break;
		case FS_INT:
			fs->val.i = (int *)val;
			break;
		case FS_UINT:
			fs->val.ui = (unsigned int *)val;
			break;
		case FS_FLOAT:
			fs->val.f = (float *)val;
			break;
		case FS_DOUBLE:
			fs->val.d = (double *)val;
			break;
		case FS_INT64:
			fs->val.ll = (__int64 *)val;
			break;
		case FS_UINT64:
			fs->val.ull = (__int64 *)val;
			break;
	}
	
	// Memcpy it over to a new array so val can be freed later
	void *nval = NULL;
	nval = malloc(fs->size);
	memcpy(nval, fs->val.c, fs->size);
	fs->val.c = nval;
	
	return 0;
}

int put_fs_container(fsave *cont, fsave *fs, unsigned int NumElements) {
	char *c = NULL;
	int rv = 0;
	
	cont->type = FS_CONTAINER;
	
	unsigned int cs = 0;
	for(int i = 0; i < NumElements; i++) {
		cs += get_fs_strlen(&fs[i]);		
	}
	
	cont->size = cs;
	
	if(fs != NULL && NumElements >= 1) {
		c = malloc(cs);
	}
	
	char *pos = c;
	
	for(int i = 0; i < NumElements; i++) { 
		pos = print_fs(pos, &fs[i]);
		if(pos == NULL) {
			rv = -1;
			goto error;
		}
	}
	
	cont->val.c = c;
	
	error:
	if(rv) { 
		if(c != NULL) { free(c); }
		cont->val.c = NULL;
	}
	
	return rv;
}

int put_fs_custom(fsave *fs, void *val, unsigned int num_entries, unsigned int *types, char **names, unsigned int NumElements) {
	// Puts the values in val (saved as char-array) into an FS_CUSTOM fsave.
	//
	// Inputs;
	// fs 			-> The fsave to which to save the values
	// val			-> A char array containing the full array of the things.
	// num_entries	-> How many subtypes in the custom type
	// types		-> Array containing the types (with FS_TYPE convention)
	// names		-> The names of each field
	// NumElements	-> The number of elements in the array.
	
	if(val == NULL) { return MCF_ERR_NODATA; }
	if(num_entries == 0) { return MCF_ERR_CUST_NOENTRIES; } 
	
	int rv = 0, i;
	unsigned int *ns = malloc(sizeof(unsigned int)*num_entries);
	unsigned char *ctypes = malloc(sizeof(unsigned char)*num_entries);
	char *c = NULL;
	
	fs->type = FS_CUSTOM;
	
	unsigned int cs = 0;
	size_t size = 0, nsizesum = 0, si = sizeof(unsigned int), si8 = sizeof(unsigned char);

	// Figure out the base size of the element, plus the lengths of all the names.
	for(i = 0; i < num_entries; i++) {
		if(!is_valid_fs_type(types[i])) { rv = MCF_ERR_INVALID_TYPE; goto error; }
		
		if(names[i] != NULL) { 
			ns[i] = strlen(names[i])+1; 
		} else {
			ns[i] = 0;
		}
		
		nsizesum += ns[i];
		size += get_fs_type_size(types[i]);
		
		ctypes[i] = (unsigned char)types[i];
	}
	
	fs->size = size*NumElements + nsizesum; // The raw data contained in the fsave.
	fs->size += si*(num_entries + 1);		// Space for the name sizes, plus one for num_entries.
	fs->size += si8*num_entries; 			// Space for the type definitions

	// Create the char array.
	c = calloc(fs->size, 1);
	char *pos = c;
	
	memcpy(pos, &num_entries, si);	// Starts with the number of entries.
	pos += si;
	for(i = 0; i < num_entries; i++) {
		// Now the size of the name.
		memcpy(pos, &ns[i], si);
		pos += si;
		
		// Then the name itself
		if(ns[i] != 0) {
			memcpy(pos, names[i], ns[i]);
			pos += ns[i];
		}
		
		// Then the type, unsigned char
		memcpy(pos, &ctypes[i], si8);
		pos += si8;
	}
	
	// Now copy the value of the thing into the array.
	memcpy(pos, val, size*NumElements);
	fs->val.c = malloc(fs->size);
	
	// Move the array to the fsave and clear our pointer to it - don't free c.
	memcpy(fs->val.c, c, fs->size);
	c = NULL;
	
	error:
	// Some of these null checks are currently unnecessary, but I'm leaving them in
	// in case this function changes and we want to free these vars before this.
	if(c != NULL) { free(c); }
	if(ns != NULL) { free(ns); }
	if(ctypes != NULL) { free(ctypes); }
	
	if(rv != 0) {
		*fs = free_fsave(fs);
	}
	
	return rv;
}

long find_next_fsave_in_file(FILE *f, long max_bytes) {
	// Retrieve the location of the next fsave from the file, starting at the
	// position you've started at. File is set to that position (not rewound), and
	// the position from the beginning of the file is returned. Must start at the 
	// beginning of an fsave header.
	//
	// If max_bytes is MCF_EOF, will stop at end of file.
	// If max_bytes is MCF_WRAP, will wrap back to initial position.
	//
	// Returns MCF_ERR_FSAVE_NOT_FOUND if there are no more FSAVES.
	
	fsave fsb = null_fs();
	int rv = 0;
	
	if(f == NULL) { return MCF_ERR_NOFILE; }
	
	if(max_bytes <= 0) { 
		if(max_bytes != MCF_EOF && max_bytes != MCF_WRAP) { 
			return MCF_ERR_INVALID_MAX_BYTES;
		}
	}
	
	if(feof(f)) {
		if(max_bytes == MCF_WRAP) { rv = 0; goto error; }
		rv = MCF_ERR_EOF;
		goto error;
	}
	
	if(rv = get_fs_header_from_file(f, &fsb)) { goto error; }
	fseek(f, fsb.size, SEEK_CUR);
	
	if(feof(f)) {
		if(max_bytes == MCF_WRAP) {
			rv = 0;
			rewind(f);
		} else {
			rv = MCF_ERR_EOF;
		}
	} else {
		rv = ftell(f);
	}
	
	error:
	free_fsave(&fsb);
	
	return rv;
}

long find_nth_fsave_in_file(FILE *f, unsigned int num, long max_bytes) {
	// Retrieve the location in file of the Nth fsave, starting at the position
	// you've started at. File is set to that position (not rewound), and the
	// position from beginning of file is returned.
	//
	// If max_bytes is MCF_EOF, will stop at the end of the file
	// If max_bytes is MCF_WRAP, will wrap back to the initial position.
	// Other negative values to max_bytes will be interpreted as errors.
	//
	// Returns MCF_ERR_FSAVE_NOT_FOUND if the number of fsaves is less
	// than the requested number.
	
	int rv = MCF_ERR_FSAVE_NOT_FOUND;
	long pos, init_pos = pos = ftell(f), bytes = 0;
	fsave fsb = null_fs();
	
	if(f == NULL) { return MCF_ERR_NOFILE; }
	if(max_bytes <= 0) { 
		if(max_bytes != MCF_EOF && max_bytes != MCF_WRAP) { 
			return MCF_ERR_INVALID_MAX_BYTES;
		}
	}
	
	int i = 0;
	int wrapped = 0;
	while(i < num && !feof(f)) {
		if(rv = get_fs_header_from_file(f, &fsb)) { pos = rv; goto error; }
		
		fseek(f, fsb.size, SEEK_CUR);
		i++;
		
		pos = ftell(f);

		bytes = pos-init_pos;
		if(max_bytes >= 0) {
			if(bytes >= max_bytes) { break; }
		} else {
			if(feof(f) && max_bytes == MCF_WRAP) {
				if(!wrapped) { 
					rewind(f);
					wrapped = 1;
				} else {
					break;
				}
			} 
		}
	}
	
	if(bytes >= max_bytes || feof(f)) {
		pos = MCF_ERR_FSAVE_NOT_FOUND;	
	}
	
	error:
	free_fsave(&fsb);
	
	return pos;
}

int replace_fsaves(FILE *f, char *parent, fsave *list, int nfs, int *found) {
	// To replace nfs fsaves in the top-level container container_name,
	// within a file. For now, if you want to replace a list within a
	// non-top-level container, just position the file at the beginning
	// of its parent container (the file won't be rewound).
	//
	// This function only works with constant-size fsaves, so if you use this
	// make sure it's pre-allocated or you'll corrupt the file.
	
	
	int i, rv = 0;
	fsave fp = null_fs();
	flocs inf = null_flocs();
	int null_f = (found == NULL);
	int *locs = NULL;
	
	char **names = NULL;
	
	if(f == NULL) { rv = MCF_ERR_NOFILE; goto error; }
	if(list == NULL || nfs < 1) { rv = MCF_ERR_FLOC_NAME; goto error; }

	long pos = find_fsave_in_file(f, parent, MCF_WRAP);
	if(pos < 0) { rv = pos; goto error; }
	
	fseek(f, pos, SEEK_SET);

	if(rv = get_fs_header_from_file(f, &fp)) { goto error; }
	
	names = calloc(nfs, sizeof(char *));
	for(i = 0; i < nfs; i++) {
		names[i] = malloc(list[i].ns);
		memcpy(names[i], list[i].name, list[i].ns);	
	}
	
	inf = read_flocs_from_file(f, &rv, fp.size);
	
	locs = strings_in_array(inf.name, names, inf.num, nfs);
	if(null_f) {
		found = calloc(nfs, sizeof(int));	
	} else {
		memset(found, 0, sizeof(int)*nfs);	
	}							
	
	for(i = 0; i < nfs; i++) {
		if(!(found[i] = (locs[i] >= 0))) { continue; }
		
		fseek(f, inf.pos[locs[i]], SEEK_SET);
		fwrite(list[i].val.c, sizeof(char), list[i].size, f);
	}
	
	error:
	if(locs != NULL) { free(locs); }
	if(null_f && found != NULL) { free(found); }
	
	free_string_array(names, nfs);
	free_fsave(&fp);
	free_flocs(&inf);
	
	return rv;
}

long find_fsave_in_file(FILE *f, char *fs_name, long max_bytes) {
	// If max_bytes is MCF_EOF, will stop at the end of the file
	// If max_bytes is MCF_WRAP, will wrap back to the initial position.
	// Other negative values to max_bytes will be interpreted as errors.
	//
	// Returns MCF_ERR_FSAVE_NOT_FOUND on missing.


	if(f == NULL) { return MCF_ERR_NOFILE; }
	if(fs_name == NULL) { return MCF_ERR_FLOC_NAME; }
	
	if(max_bytes <= 0) {
		if(max_bytes != MCF_EOF && max_bytes != MCF_WRAP) {
			return MCF_ERR_INVALID_MAX_BYTES;
		}
	}
	
	long init_pos = ftell(f);
	long pos = MCF_ERR_FSAVE_NOT_FOUND;
	int wrapped = 0;
	fsave fsb = null_fs();
	
	if(feof(f) && max_bytes == MCF_WRAP) {
		rewind(f);
		wrapped = 1;
	}
	
	int ns;
	int nlen = 0;
	
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	
	int rv = 0;
	while(!feof(f)) {
		pos = ftell(f);
		
		if(rv = get_fs_header_from_file(f, &fsb)) {
			pos = rv;
			break;
		}
		
		if(fsb.name != NULL && strcmp(fsb.name, fs_name) == 0) {
			break;
		} else {
			pos = MCF_ERR_FSAVE_NOT_FOUND;
		}
		
		fseek(f, fsb.size, SEEK_CUR);
		
		if(fgetc(f) != EOF) { fseek(f, -1, SEEK_CUR); } // Check if we're one away from EOF.
		
		if(feof(f) && max_bytes == MCF_WRAP) {
			if(wrapped) {
				break;
			} else {
				rewind(f);
				wrapped = 1;
			}
		}
		
		if(wrapped && pos >= init_pos) {	// Wrapped is only set if MCF_WRAP.
			pos = MCF_ERR_FSAVE_NOT_FOUND;
			break;
		}
	}
	
	error:
	free_fsave(&fsb);
	
	fseek(f, init_pos, SEEK_SET);
	
	return pos;
}

fsave read_fsave_from_file(FILE *f, int *ev) {
	if(f == NULL) { *ev = MCF_ERR_NOFILE; return null_fs(); }
	
	int rv = 0;
	fsave fs = null_fs();
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	
	if(fread(&(fs.ns), si, 1, f) < 1) {
		rv = MCF_ERR_BADFNAME;
		goto error;
	}
	
	fs.name = malloc(fs.ns);
	if(fread(fs.name, 1, fs.ns, f) != fs.ns) { 
		rv = MCF_ERR_BADFNAME;
		goto error;
	}
	
	if(fread(&(fs.type), si8, 1, f) < 1) {
		rv = MCF_ERR_FS_NOTYPE;
		goto error;
	}
	
	if(fread(&(fs.size), si, 1, f) < 1) {
		rv = MCF_ERR_FS_NOSIZE;
		goto error;
	}
	
	if(fs.size < 1) {
		fs.val.c = NULL;	
	} else {
		fs.val.c = malloc(fs.size);
		if(fread(fs.val.c, 1, fs.size, f) < fs.size) {
			rv = MCF_ERR_FS_BADCONTENTS;
			goto error;
		}
	}
	
	error:
	if(rv < 0) {
		fs = free_fsave(&fs);
	}
	
	*ev = rv;
	return fs;
}

fsave read_fsave_from_char(char *array, int *ev) {
	if(array == NULL) { *ev = MCF_ERR_NOARRAY; return null_fs(); }
	
	int rv = 0;
	char *pos = array;
	fsave fs = null_fs();
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	
	// Name length
	memcpy(&(fs.ns), pos, si);
	pos += si;
	
	// Get the name
	if(fs.ns > 0) {
		fs.name = malloc(fs.ns);
		memcpy(fs.name, pos, fs.ns);
		pos += fs.ns;
		
		if(fs.name[fs.ns-1] != '\0') {
			rv = MCF_ERR_BADFNAME;
			goto error;
		}
		
	} else {
		fs.name = NULL;
	}
	
	// Get the type
	memcpy(&(fs.type), pos, si8);
	pos += si8;
	
	// Get the size
	memcpy(&(fs.size), pos, si);
	pos += si;
	
	// Get the contents
	if(fs.size < 1) {
		fs.val.c = NULL;	
	} else {
		fs.val.c = malloc(fs.size);
		memcpy(fs.val.c, pos, fs.size);
		pos +=fs.size;
	}
	
	error:
	if(rv) {
		fs = free_fsave(&fs);	
	}
	
	*ev = rv;
	return fs;
}

fsave *read_all_fsaves_from_char(char *array, int *ev, unsigned int *fs_size, size_t size) {
	if(array == NULL) { *ev = MCF_ERR_NOARRAY; return NULL; }
	
	int rv =0, i;
	char *pos = array;
	fsave *fs = NULL;
	*fs_size = 0;
	unsigned int fss = 0;
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	
	fss = 1;
	for(i = 0; i < fss; i++) {
		fs = malloc_or_realloc(fs, sizeof(fsave)*fss);
		
		fs[i] = read_fsave_from_char(pos, &rv);
		if(rv != 0) { goto error; }
		
		pos += get_fs_strlen(&fs[i]);
		if((pos-array) >= size) { break; }
		fss++;
	}
	
	error:
	if(rv != 0) {
		free_fsave_array(fs, fss);
		fss = 0;
	}
	
	*ev = rv;
	*fs_size = fss;
	return fs;
}

fsave *read_all_fsaves_from_file(FILE *f, int *ev, unsigned int *fs_size, long max_bytes) {
	if(f == NULL) { *ev = MCF_ERR_NOFILE; return NULL; }
	
	int rv = 0, i;
	fsave *fs = NULL;
	*fs_size = 0; 
	long init_pos = ftell(f), pos;	
	
	unsigned int fss = 0;
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	fss = 1;
	
	for(i = 0; i < fss; i++) {
		fs = malloc_or_realloc(fs, sizeof(fsave)*fss);
		
		fs[i] = read_fsave_from_file(f, &rv);
		if(rv != 0) { goto error; }
		
		pos = ftell(f) - init_pos;
		if(max_bytes <= 0 && feof(f)) {
			break; 
		} else if(pos >= max_bytes) {
			break;
		}
		fss++;
	}
	
	error:
	if(rv != 0) { 
		fs = free_fsave_array(fs, fss);
		fss = 0;
	}
	
	*ev = rv;
	*fs_size = fss;
	return fs;
}

char *print_fs(char *array, fsave *fs) {
	// Print an FS into an array and advance the array.
	// These will always have the same form:
	// UInt -> Length of the name of the thing
	// Char[] -> The name
	// UChar -> The type of the value (saved as UChar, cast from UInt);
	// UInt -> Size of the value in bytes					 
	// Char - > The value itself (implicit cast to Char via Union)
	
	if(array == NULL) { return NULL; }
	char *pos = array;
	
	size_t si = sizeof(unsigned int);
	
	// First the field name
	memcpy(pos, &(fs->ns), si);
	pos += si;
	
	memcpy(pos, fs->name, fs->ns);
	pos += fs->ns;
	
	// Now the field value.
	memcpy(pos, &(fs->type), sizeof(unsigned char));
	pos += sizeof(unsigned char);
	
	memcpy(pos, &(fs->size), si);
	pos += si;
	
	// If there are no vals, nothing to copy.
	if(fs->val.c == NULL) { 
		if(fs->type != FS_CONTAINER) { return NULL; }
		
		return pos;
	}
	
	memcpy(pos, fs->val.c, fs->size); 
	pos += fs->size;
	
	return pos;
}

size_t get_fs_type_size(unsigned int type) {
	switch(type) {
		case FS_CHAR:
		case FS_UCHAR:
		case FS_CUSTOM:
		case FS_CONTAINER:
			return sizeof(char);
		case FS_INT:
		case FS_UINT:
			return sizeof(int);
		case FS_FLOAT:
			return sizeof(float);
		case FS_DOUBLE:
			return sizeof(double);
		case FS_INT64:
		case FS_UINT64:
			return sizeof(__int64);
	}
	return 0;
		
}

unsigned int get_fs_strlen(fsave *fs) {
	// Static sizes:	
	// UInt -> Length of name of thing
	// UInt -> Size of the value in bytes
	// UChar -> Type of the thing
	//
	// Dynamic values
	// fs.ns -> Name of the thing
	// fs.size -> Full size of the thing.
	
	return (sizeof(unsigned int)*2 + sizeof(unsigned char) + fs->ns + fs->size);
}

int is_valid_fs_type(unsigned int type) {
	switch(type) {
		case FS_CHAR:
		case FS_UCHAR:
		case FS_INT:
		case FS_UINT:
		case FS_FLOAT:
		case FS_DOUBLE:
		case FS_INT64:
		case FS_UINT64:
		case FS_CONTAINER:
		case FS_CUSTOM:
			return 1;
	}
	
	return 0;
}

fsave free_fsave(fsave *fs) {
	if(fs != NULL) {
		if(fs->val.c != NULL) { free(fs->val.c); }
		if(fs->name != NULL) { free(fs->name); }
		
	}
	
	return null_fs();
}

fsave *free_fsave_array(fsave *fs, int size) {
	if(fs != NULL) {
		for(int i = 0; i < size; i++) {
			free_fsave(&(fs[i]));
		}
		
		free(fs);
	}
	
	return NULL;
}

fsave null_fs() {
	fsave n = {.name = NULL, .ns = 0, .val.c = NULL, .type = FS_NULL, .size = 0};
	return n;
}

/*************** FLocs Functions ********************/ 
int get_fs_header_from_file(FILE *f, fsave *fs) {
	// Must pass a file where the current position is the beginning of the fsave
	// fs should be null_fs(), or at least have fs->name not allocated.
	//
	// Does not move the pointer back.
	
	if(f == NULL) { return MCF_ERR_NOFILE; }
	if(fs == NULL) { return MCF_ERR_NOFLOCS; }
	
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	
	if(fread(&(fs->ns), si, 1, f) < 1) {
		return MCF_ERR_FILEREAD;		
	}
	
	if(fs->name != NULL) { free(fs->name); }
	
	if(fs->ns > 0) {
		fs->name = malloc(fs->ns);
		if(fread(fs->name, si8, fs->ns, f) < fs->ns) {
			return MCF_ERR_FILEREAD;	
		}
	} else {
		fs->name = malloc(1);
		fs->name[0] = '\0';
	}
	
	if(fread(&(fs->type), si8, 1, f) < 1) {
		return MCF_ERR_FS_NOTYPE;	
	}
	
	if(fread(&(fs->size), si, 1, f) < 1) {
		return MCF_ERR_FS_NOSIZE;	
	}
	
	return 0;
}

int get_fs_header_size_from_ns(int ns) {
	// Size is 2 ints -> size and name size, 1 unsigned char -> type, plus the name   
	return ns +  sizeof(unsigned int)*2 + sizeof(unsigned char);
}

int get_fs_header_size_from_file(FILE *f) {
	// Must pass a file where the current position is the beginning of the fsave
	// File will be rewound to initial position.
	if(f == NULL) { return MCF_ERR_NOFILE; }
	
	long pos = ftell(f);
	int rv = 0;
	
	int size;
	if(fread(&size, sizeof(unsigned int), 1, f) < 1 || size < 0) {
		rv = MCF_ERR_BADFNAME;
		goto error;
	}
	
	rv = get_fs_header_size_from_ns(size);
	
	error:
	
	fseek(f, pos, SEEK_SET);
	return rv;
}

int get_fs_header_size_from_char(char *array) {
	if(array == NULL) { return MCF_ERR_NOARRAY; }
	
	int size;
	memcpy(&size, array, sizeof(unsigned int));
	
	if(size < 0) {
		return MCF_ERR_BADFNAME;	
	}
	
	return get_fs_header_size_from_ns(size);
}

flocs read_flocs_from_file(FILE *f, int *ev, long max_bytes) {
	// Create the flocs from a file, getting the list of containers
	int rv = 0;
	flocs fl = null_flocs();
	
	if(f == NULL) {
		rv = MCF_ERR_NOFILE;
		goto error;
	}
	
	size_t nlen, clen;
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	unsigned int type;

	// Read the first bit.
	unsigned long int pos;
	size_t count = fread(&nlen, si, 1, f);
	if(count != 1 || feof(f)) {
		rv = MCF_ERR_FILEREAD;
		goto error;
	}

	fl.num = 1;
	long init_pos = ftell(f);
	long bytes = 0;
	
	if(max_bytes <= 0 && max_bytes != MCF_EOF) {
		rv = MCF_ERR_INVALID_MAX_BYTES;
		goto error;
	}
	
	for(int i = 0; i < fl.num; i++) {
		// First read out the one we've already found.
		fl.name = malloc_or_realloc(fl.name, sizeof(char*)*fl.num);
		fl.ns = malloc_or_realloc(fl.ns, sizeof(unsigned int)*fl.num);
		fl.size = malloc_or_realloc(fl.size, sizeof(unsigned int)*fl.num);
		fl.hpos = malloc_or_realloc(fl.hpos, sizeof(unsigned int)*fl.num);
		fl.pos = malloc_or_realloc(fl.pos, sizeof(unsigned int)*fl.num);
		
		fl.type = malloc_or_realloc(fl.type, sizeof(unsigned char)*fl.num);
		
		fl.name[i] = calloc(nlen, 1);

		fl.ns[i] = nlen;
		fl.hpos[i] = ftell(f)-si; // We've already read one 
			
		count = fread(fl.name[i], 1, nlen, f);
		if(count != nlen) {
			rv = MCF_ERR_FLOC_NAME;
			goto error;
		}
		
		count = fread(&(fl.type[i]), si8, 1, f);
		if(count != 1) { 
			rv = MCF_ERR_FLOC_TYPE;
			goto error;
		}
		
		count = fread(&(fl.size[i]),si, 1, f);
		if(count != 1) {
			rv = MCF_ERR_FLOC_SIZE;
			goto error;
		}
		
		fl.pos[i] = ftell(f);	// Save the position.
		// Now seek the next one.
		fseek(f, fl.size[i], SEEK_CUR);
		
		if(!(fgetc(f) == EOF)) { fseek(f, -1, SEEK_CUR); }
		if(feof(f) || (max_bytes != MCF_EOF && ftell(f)-init_pos > max_bytes)) { break; }
		
		count = fread(&nlen, si, 1, f);
		if(count != 1) {
			rv MCF_ERR_FILEREAD;
			goto error;
		}
		
		fl.num++;
	}
	
	error:
	if(rv) {
		fl = free_flocs(&fl);
	}
	
	*ev = rv;
	return fl;
}

flocs read_flocs_from_char(char *array, int *ev, size_t size) {
	// Generates flocs from a char array you've already read out.
	if(array == NULL) { *ev = MCF_ERR_NOSTRING; return null_flocs(); }
	
	// Indicator for reading from char array.
	int rv = 0;
	char *cpos = array;
	size_t si = sizeof(unsigned int), si8 = sizeof(unsigned char);
	unsigned int nlen;
	unsigned char type;
	flocs fl = null_flocs();
	
	fl.num = 1;
	
	// Read out all the flocs until there are no more, reallocating arrays as we go.
	for(int i = 0; i < fl.num; i++) {
		memcpy(&nlen, cpos, si);
		cpos += si;
		
		if(nlen > cpos-array+size) {
			rv = MCF_ERR_BADFNAME;
			goto error;
		}
		
		fl.name = malloc_or_realloc(fl.name, sizeof(char *)*fl.num);
		fl.ns = malloc_or_realloc(fl.ns, sizeof(unsigned int)*fl.num);
		fl.hpos = malloc_or_realloc(fl.hpos, sizeof(unsigned int)*fl.num);
		fl.pos = malloc_or_realloc(fl.pos, sizeof(unsigned int)*fl.num);
		fl.size = malloc_or_realloc(fl.pos, si*fl.num);
		fl.type = malloc_or_realloc(fl.pos, si8*fl.num);
		
		if(nlen > 0) { 
			fl.name[i] = malloc(nlen);
		} else { 
			fl.name[i] = NULL;
		}
		
		fl.ns[i] = nlen;
		fl.hpos[i] = (unsigned int)(cpos - array - si);
		
		memcpy(&(fl.name[i]), cpos, nlen);
		cpos += nlen;
		
		if(fl.name[nlen-1] != '\0') {
			rv = MCF_ERR_BADFNAME;
			goto error;
		}
		
		memcpy(&(fl.type[i]), cpos, si8);
		cpos += si8;
	
		memcpy(&(fl.size[i]), cpos, si);
		cpos += si;
		
		fl.pos[i] = (unsigned int)(cpos-array);
		
		if((fl.pos[i] + fl.size[i]) >= size) { break; }
		
		cpos += fl.size[i];
		fl.num++;
	}
	
	error:
	if(rv != 0) {
		free_flocs(&fl);
		fl = null_flocs();
	}
	
	*ev = rv;
	return fl;
}

flocs null_flocs() {
	int rv = 0;
	flocs fl = {.name = NULL,
				.ns = NULL,
				.size = NULL,
				.hpos = NULL,
				.pos = NULL,
				.type = NULL,
				.num = 0 }; 

	return fl;
}

flocs free_flocs(flocs *fl) {
	if(fl != NULL) { 
		if(fl->name != NULL) { free_string_array(fl->name, fl->num); }
		if(fl->ns != NULL) { free(fl->ns); fl->ns = NULL; }
		if(fl->size != NULL) { free(fl->size); fl->size = NULL; }
		if(fl->hpos != NULL) { free(fl->hpos); fl->hpos = NULL; }
		if(fl->pos != NULL) { free(fl->pos); fl->pos = NULL; }
		if(fl->type != NULL) { free(fl->type); fl->type = NULL; }
	}
	
	return null_flocs();
}
