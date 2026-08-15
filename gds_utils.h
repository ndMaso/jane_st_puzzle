#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct {
  uint32_t x;
  uint32_t y;
} XY_t;

// Rect, for printing to JSON
typedef struct {
  uint8_t layer;
  uint8_t dtype;
  int32_t x1;
  int32_t x2;
  int32_t y1;
  int32_t y2;
} rect_t;

//a pin of a pcell, the drawing layer held in sref
typedef struct {
  int32_t x1;
  int32_t x2;
  int32_t y1;
  int32_t y2;
} pin_t;

typedef struct {
  int32_t x[16];    //x and y coordinates, placed at the center of some pin
  int32_t y[16];
  int num_copies;   // how many instances of the label have been seen.
                    // assume no more than 16.
  char pinname[64]; //unique pin name in the pcell
} pin_lbl_t;

// Structure, used to hold important parts pcells
// Contains a structure name used to reference later when referenced
// Contains a list of labelled pins.
typedef struct {
  int        n_pins;         //how many of the pins array are allocated
  int        n_lbls;         //how many unique labels are allocated.
  char       strname[64];    //structure name
  pin_t*     pins[64];       //room for 64 pin objects
  pin_lbl_t* pin_lbls[64];   //list of unique pin labels in the structure.
} sref_t;

typedef struct {
  sref_t* structures[512];
  int     num_structs;
} structure_list_t;

enum State {
  STREAM_S,
  FORMAT_S,
  STRUCTURE_S,
  ELEMENT_S,
  BOUNDARY_S,
  PATH_S,
  SREF_S,
  AREF_S,
  TEXT_S,
  NODE_S,
  BOX_S,
  TEXTBODY_S,
  STRANS_S,
  PROPERTY_S,
  END_S, //no remaining
  GOT_TOP_S //saw the top level strname.
};

enum RType {
  START = 255,
  HEADER = 0,
  BGNLIB = 1,
  LIBNAME = 2,
  UNITS = 3,
  ENDLIB = 4,
  BGNSTR = 5,
  STRNAME = 6,
  ENDSTR = 7,
  BOUNDARY = 8,
  PATH = 9,
  SREF = 10,
  AREF = 11,
  TEXT = 12,
  LAYER = 13,
  DATATYPE = 14,
  WIDTH = 15,
  XY = 16,
  ENDEL = 17,
  SNAME = 18,
  COLROW = 19,
  NODE = 21,
  TEXTTYPE = 22,
  PRESENTATION = 23,
  STRING = 25,
  STRANS = 26,
  MAG = 27,
  ANGLE = 28,
  REFLIBS = 31,
  FONTS = 32,
  PATHTYPE = 33,
  GENERATIONS = 34,
  ATTRTABLE = 35,
  ELFLAGS = 38,
  NODETTYPE = 42,
  PROPATTR = 43,
  PROPVALUE = 44,
  BOX = 45,
  BOXTYPE = 46,
  PLEX = 47,
  TAPENM = 50 ,
  APECODE = 51,
  FORMAT = 54,
  MASK = 55,
  ENDMASKS = 56,
};

enum DType {
  NO_DATA_T = 0,
  BIT_ARRAY_T = 1,
  INT16_T = 2,
  INT32_T = 3,
  FLOAT_T = 4,
  DOUBLE_T = 5,
  STRING_T = 6
};

//standardized format for adding boxes to a list
//targets a json or whatever format so python and parse it simply.
static inline void write_box(FILE* fd, rect_t* box, char* pin);

//breaks a path object down into boxes and writes them to the output file
void store_path(FILE* fd, XY_t path[]);

//call to read a structure definition into memory;
// FILE*   fd : file descriptor with cursor alligned to first record of a structure
// sref_t** s : Pointer to the next empty element of structure list
void process_struct(FILE* fd, sref_t** s) ;

//FILE*    fd : the output file
int write_units(FILE* fd, uint64_t unit1, uint64_t unit2);

void process_pin_name(char* name, sref_t* s, int32_t X, int32_t Y);

//Instead of jumping to the structure and continuing to interpret it, use the saved copy of just the pins.
//Translate the pins (assumes no transformation is used) and copy into the out file.
void copy_sref(char* ref, FILE* out_file, int x, int y);

//processes gds file until seeing the top level structure name, building pin lists of the pcells
int build_structures(FILE* in_file, FILE* out_file, structure_list_t* slist, char* top_strnames);

//instantiates pcells and draws routing rectangles as specified in the top level str name
void print_structure(FILE*in_file, FILE* out_file, structure_list_t* slist);