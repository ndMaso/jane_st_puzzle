#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <winsock2.h>
#include "gds_utils.h"


//standardized format for adding boxes to a list
//targets a json or whatever format so python and parse it simply.
static inline void write_box(FILE* fd, rect_t* box, char* pin) {
  fprintf(fd, "{pin_name : %s, coords : [%d,%d,%d,%d]}\n", pin, box->x1, box->x2, box->y1, box->y2);
  return;
}

//sets poly->l_rb;
//sets poly->bound as a bounding box with margin
//returns value of l_rb or -1 on error.
int config_poly(poly_t* poly, int margin) {
  int32_t x1min, x2max, y1min, y2max;

  //down / up_bar
  //direction of the leftmost vertical edge of the shape.
  // 0 for up 1 for down.
  int down_upb = 0;
  if (poly->num_points < 5) {
    fprintf(stderr, "Polygon with too few points : %d\n", poly->num_points);
    return -1;
  }

  XY_t cur = poly->coords[0];
  x1min = cur.x;
  x2max = cur.x;
  y1min = cur.y;
  y2max = cur.y;
  //iterate through every point
  for (int p = 1; p < poly->num_points; p++) {
    XY_t this = poly->coords[p];
    x1min = (x1min < this.x) ? x1min : this.x;
    x2max = (x2max > this.x) ? x2max : this.x;
    y1min = (y1min < this.y) ? y1min : this.y;
    y2max = (y2max > this.y) ? y2max : this.y;

    if (cur.x == this.x) //vertical
    {
      if (this.x == x1min) //leftmost seen
      {
        down_upb = (this.y > cur.y) ? 0 : 1;
      }
    }
    cur = this;
  }
  //assign bounding box.
  poly->bound = (box_t) {x1min - margin, x2max + margin, y1min - margin, y2max + margin};

  //if the leftmost vertical edge is downward, then the interior is on the left
  poly->l_rb = down_upb;

  return down_upb;
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
      unique = 0;
      break;
    }
  }
  if (unique == 1) {
    //allocate a new pin label
    s->pin_lbls[s->n_lbls] = malloc(sizeof(pin_lbl_t));
    s->pin_lbls[s->n_lbls]->xy.x = X;
    s->pin_lbls[s->n_lbls]->xy.y = Y;
    strcpy(s->pin_lbls[s->n_lbls]->pinname, name);
    s->n_lbls++;
  }
  return;
}

//Creates an association between pin shapes, pin labels, and the LI shape of the pin.
//s : one structure, filled with pins unassigned to pin_lbls.
//p : list of LI shapes in the structure
int assign_pins(sref_t* s, polylist_t* pl) {

  for(int lbl = 0; lbl < s->n_lbls; lbl++) {
    //for each pinname, find the poly that it's labels sit in.
    int poly_index = 0;
    int found = 0;
    for (; poly_index < pl->num_polys; poly_index++) {
      if (inside_poly(s->pin_lbls[lbl]->xy, &(pl->polys[poly_index])) == 1) {
        //Copy this poly into the structure->pin
        memcpy(s->pin_lbls[lbl]->LI_poly.coords, pl->polys[poly_index].coords, sizeof(XY_t)*pl->polys[poly_index].num_points);
        s->pin_lbls[lbl]->LI_poly.l_rb = pl->polys[poly_index].l_rb;
        s->pin_lbls[lbl]->LI_poly.num_points = pl->polys[poly_index].num_points;
        memcpy(&s->pin_lbls[lbl]->LI_poly.bound, &pl->polys[poly_index].bound, sizeof(box_t));
        found = 1;
        break;
      }
    }
    if (found == 0) {
      fprintf(stderr, "Error: None of %d shapes cover Pin %s of Structure %s at (%d, %d)\n", pl->num_polys, s->pin_lbls[lbl]->pinname, s->strname, s->pin_lbls[lbl]->xy.x, s->pin_lbls[lbl]->xy.y);
      for (poly_index = 0; poly_index < pl->num_polys; poly_index++) {
        printf("Pin location (%d, %d), ", s->pin_lbls[lbl]->xy.x, s->pin_lbls[lbl]->xy.y);
        printf("Poly wound %d:", pl->polys[poly_index].l_rb);
        for (int ppp = 0; ppp < pl->polys[poly_index].num_points ; ppp++) {
          printf("(%d, %d)-> ", pl->polys[poly_index].coords[ppp].x, pl->polys[poly_index].coords[ppp].y);
        }
        printf("\n");
      }
    }
  }
  //find all pins that whose center is in that poly
  //if already assigned a label, skip this

  return 0;
}

//given a gds file pointing at the top level struct, builds a list of licon contacts and returns the cursor
//when done. Sorts the list by increasing y coordinate.
//FILE* in_file : the gds stream
//sref_t* via_ref : structure element corresponding to the LICON via.
//return - buf : large buffer with room for all via contacts drawn by the top structure.
contact_t* build_contact_list(FILE* in_file, sref_t* via_ref) {
  contact_t* buf = malloc(sizeof(contact_t) * 8192);
  int num_contacts = 0;
  //run the state machine to the end, but only look for sref elements matching the via_ref name

  //put the required info into the buffer
  //REMEMBER TO NULL THE PINNAME.

  return buf;
}

//
int inside_poly(XY_t xy, poly_t* p) {
  //how far the point is from the nearest edge passing it on the left or right
  //span_*[0] is the distance to the edge on the points left, then right, down, up;
  int32_t span_left[4] = {INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX};
  int32_t span_right[4]= {INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX};
  int h_vb = 0; //horizontal, not vertical edge
  //walk through poly and set the spans
  XY_t cur = p->coords[0];
  XY_t next = p->coords[1];
  if ((next.y - cur.y) != 0) h_vb = 0;

  for (int edge = 1; edge < p->num_points; edge++) {
    XY_t this = p->coords[edge];
    if (h_vb == 1)
    {
      //check if X coord is in horizontal extend
      if (xy.x >= min(cur.x, this.x) && xy.x <= max(cur.x, this.x)) {
        //if you drive through the point, it's inside
        if (xy.y == this.y) return 1;
        //work out which side of the directed edge it's on.
        int left = ((this.x < cur.x) == (xy.y < this.y)) ? 1 : 0;
        //update the extents
        if (left == 1) { //update span_left
          if (xy.y > this.y) //update down span
            span_left[2] = min(span_left[2], xy.y - this.y);
          else  //update up span
            span_left[3] = min(span_left[3], this.y - xy.y);
        } else { //update span_right
          if (xy.y > this.y) //update down span
            span_right[2] = min(span_right[2], xy.y - this.y);
          else  //update up span
            span_right[3] = min(span_right[3], this.y - xy.y);
        }
      }
      h_vb = 0; //prepare for vertical edge
    } else //h_vb == 0
    {
      //check if Y coord is in vertical extend
      if (xy.y >= min(cur.y, this.y) && xy.y <= max(cur.y, this.y)) {
        //if you drive through the point, it's inside.
        if (xy.x == this.x) return 1;
        //work out which side of the directed edge it's on.
        int left = ((this.y > cur.y) == (xy.x < this.x)) ? 1 : 0;
        //update the extents
        if (left == 1) { //update span_left
          if (xy.x > this.x) //update down span
            span_left[0] = min(span_left[0], xy.x - this.x);
          else  //update up span
            span_left[1] = min(span_left[1], this.x - xy.x);
        } else { //update span_right
          if (xy.x > this.x) //update down span
            span_right[0] = min(span_right[0], xy.x - this.x);
          else  //update up span
            span_right[1] = min(span_right[1], this.x - xy.x);
        }
      }
      h_vb = 1; //prepare for horizontal edge
    }
    cur = this;
  }

  //having completed the loop, the point is inside if the all of the
  //spans coresponding to the winding are <inf and < the other span.
  //note >= would imply two edges through the same points in space
  if (p->l_rb == 1) {
    if ((span_left[0] < span_right[0]) && (span_left[1] < span_right[1]) &&
        (span_left[2] < span_right[2]) && (span_left[3] < span_right[3])){
          return 1;
        }

  }
  else {
    if ((span_left[0] > span_right[0]) && (span_left[1] > span_right[1]) &&
        (span_left[2] > span_right[2]) && (span_left[3] > span_right[3])){
          return 1;
        }
  }

  return 0;
}