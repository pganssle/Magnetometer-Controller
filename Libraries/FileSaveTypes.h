/********************* File Saving Types *************************/
#ifndef FILE_SAVE_TYPES_H
#define FILE_SAVE_TYPES_H

// Data saving library prototypes.
// Types
typedef struct fsave {
	char *name;					// Name of the field
	unsigned int ns;			// Size of the name of the field
	
	union {
		char *c;				// Type 1
		unsigned char *uc;		// Type 2
		
		int *i;					// Type 3
		unsigned int *ui;		// Type 4
		
		float *f;				// Type 5
		double *d;				// Type 6
		
		__int64 *ll;				// Type 7
		unsigned __int64 *ull;	// Type 8
	} val;
	
	unsigned int type;
	unsigned int size;			// Total size (in bytes)
} fsave;

typedef struct flocs {
	char **name;				// An array of the names of each thing
	unsigned int *ns;			// Name size.
	
	unsigned int *size;			// The size of the data (header can be inferred from the name 
	unsigned int *hpos;			// Position of the header.
	unsigned int *pos;			// Position of the data 
	unsigned char *type;		// Data type
	
	int num;
} flocs;

// The Types
#define FS_NULL 0
#define FS_CHAR 1
#define FS_UCHAR 2
#define FS_INT 3
#define FS_UINT 4
#define FS_FLOAT 5
#define FS_DOUBLE 6
#define FS_INT64 7
#define FS_UINT64 8
#define FS_CONTAINER 32
#define FS_CUSTOM 64
#endif



