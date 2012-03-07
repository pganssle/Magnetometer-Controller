/************** Array Utilities *************/ 
extern int string_in_array(char **array, char *string, int size);
extern int *strings_in_array(char **array, char **strings, int size1, int size2);
extern int int_in_array(int *array, int val, int size);
extern int double_in_array(double *array, double val, int size);

extern void remove_array_item(int *array, int index, int num_items);
extern void remove_array_item_void(void *array, int index, int num_items, int type);
extern int constant_array_double(double *array, int size);
extern int constant_array_int(int *array, int size);

/************** String Utilities *************/
extern char *generate_nc_string(char **strings, int numstrings, int *len);
extern char **get_nc_strings(char *string, int *ns);

extern char **generate_char_num_array(int first, int last, int *elems);
extern char *generate_expression_error_message(char *err_message, int *pos, int size);

/************** Memory Allocation *************/     
extern void *malloc_or_realloc(void *pointer, size_t size);  
extern void *realloc_if_needed(void *array, int *len, int new_len, int inc);

extern char **free_string_array(char **array, int size);
extern double **free_doubles_array(double **array, int size);
extern int **free_ints_array(int **array, int size);

/************** File I/O *************/     
extern int get_name(char *pathname, char *name, char *ending);
extern char *get_extension(char *pathname);

extern char *temp_file(char *extension);
extern char *temporary_filename(char *dir, char *extension);

extern int file_exists(char *filename);

/************** Sorting *************/     
extern int *add_item_to_sorted_array(int *array, int item, int size);
extern void sort_linked(int **array, int num_arrays, int size);

 /************** Linear indexing *************/  
extern int get_lindex(int *cstep, int *maxsteps, int size);
extern int get_cstep(int lindex, int *cstep, int *maxsteps, int size);

/************** Math *************/     
int calculate_units(double val);
int get_precision(double val, int total_num);
extern int get_bits(int in, int start, int end);
extern int get_bits_in_place(int in, int start, int end);
extern int move_bit_skip(int in, int skip, int to, int from);
extern int move_bit(int in, int to, int from);
