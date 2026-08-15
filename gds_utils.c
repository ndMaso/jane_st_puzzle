#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gds_utils.h"


//standardized format for adding boxes to a list
//targets a json or whatever format so python and parse it simply.
static inline void write_box(FILE* fd, rect_t* box, char* pin) {
  fprintf(fd, "{pin_name : %s, coords : [%d,%d,%d,%d]}\n", pin, box->x1, box->x2, box->y1, box->y2);
  return;
}

//processes a path type down into rectangles
void store_path(FILE* fd, XY_t path[]) {

  return;
}

//call to read a structure definition into memory;
// FILE*   fd : file descriptor with cursor alligned to first record of a structure
// sref_t** s : pointer to the next empty element of structure list
void process_struct(FILE* fd, sref_t** s) {
  return;
}

//assumes big endianness
int write_units(FILE* fd, uint64_t unit1, uint64_t unit2) {
  printf("Units binary: %llu, %llu\n", unit1, unit2);

  uint8_t top_byte = unit1>>(64-8);
  uint64_t one = 1;
  uint8_t exp = top_byte & ((one<<7) - 1);
  uint8_t sign = exp >> 7;

  //bottom 3 bytes
  uint64_t mantissa = unit1 & ((one<<56) - 1);
  if (mantissa == 0) {
    perror("Invalid unit 1\n");
    return 1;
  }
  //checking for needs normalization
  uint64_t mask = ((one<<4) - 1) << (56-4);
  while ((mantissa & mask) == 0 && (exp != 0)) {
    mantissa = mantissa << 4;
    exp--;
  }

  if(exp == 0) {
    perror("Unit 1 exp == 0??\n");
    return 1;
  }

  uint64_t mantissa_for_print = mantissa >> 48;
  int16_t   true_exp = (exp - 64);
  double database_unit = ((double) mantissa_for_print / 256.0) * pow(16.0, (double) true_exp);


  top_byte = unit2>>(64-8);
  exp = top_byte & ((one<<7) - 1);
  sign = exp >> 7;

  //bottom 3 bytes
  mantissa = unit2 & ((one<<56) - 1);
  if (mantissa == 0) {
    perror("Invalid unit 1\n");
    return 1;
  }
  //checking for needs normalization
  mask = ((one<<4) - 1) << (56-4);
  while ((mantissa & mask) == 0 && (exp != 0)) {
    mantissa = mantissa << 4;
    exp--;
  }

  if(exp == 0) {
    perror("Unit 2 exp == 0??\n");
    return 1;
  }

  mantissa_for_print = mantissa >> 48;
  true_exp = (exp - 64);
  double abs_unit = ((double) mantissa_for_print / 256.0) * pow(16.0, (double) true_exp);
  fprintf(fd, "{database_unit : %f, abs_unit : %f}\n", database_unit, abs_unit);
  return 0;
}

void process_pin_name(char* name, sref_t* s, int32_t X, int32_t Y) {
  int unique = 1;
  //check for uniqueness of the pin name in the structure
  for (int name_inst = 0; name_inst < s->n_lbls; name_inst++) {
    if (strcmp(name, s->pin_lbls[name_inst]->pinname) == 0) {
      //name is not unique.
      int copy_ind = s->pin_lbls[name_inst]->num_copies++;
      //put the new coordinate of the label in the array.
      s->pin_lbls[name_inst]->x[copy_ind] = X;
      s->pin_lbls[name_inst]->y[copy_ind] = Y;
      unique = 0;
      break;
    }
  }


  if (unique == 1) {
    //allocate a new pin label
    s->pin_lbls[s->n_lbls] = malloc(sizeof(pin_lbl_t));
    s->pin_lbls[s->n_lbls]->x[0] = X;
    s->pin_lbls[s->n_lbls]->y[0] = Y;
    s->pin_lbls[s->n_lbls]->num_copies = 1;
    strcpy(s->pin_lbls[s->n_lbls]->pinname, name);
    s->n_lbls++;
  }
  return;
}