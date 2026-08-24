#ifndef TOP
#define TOP
  #include <stdlib.h>
  #include <string.h>
  #include <stdio.h>
  #include <stdint.h>
  #include <math.h>
  #include <winsock2.h>
  #include "gds_utils.h"


#define LI_LAYER_C 67
#define M1_LAYER_C 68
#define M2_LAYER_C 69
#define M3_LAYER_C 70
#define M4_LAYER_C 71
#define M5_LAYER_C 72

#define LI_PIN_DTYPE 16
#define LI_TEXT_DTYPE 5
#define DRAW_DTYPE 20
#define CONTACT_DTYPE 44
#endif

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
  printf("{database_unit : %f, abs_unit : %f}\n", database_unit, abs_unit);
  return 0;
}

void process_pin_name(char* name, sref_t* s, int32_t X, int32_t Y) {
  if(strcmp(name, "\0") == 0) {
    printf("Null pin name.\n");
  }
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
    s->pin_lbls[s->n_lbls]->n_contacts = 0;
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
      //skip non pin shapes
      if (pl->polys[poly_index].layer != LI_LAYER_C || pl->polys[poly_index].dtype != DRAW_DTYPE) continue;
      if (inside_poly(s->pin_lbls[lbl]->xy, &(pl->polys[poly_index])) != 0) {
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
        printf("Label location (%d, %d), ", s->pin_lbls[lbl]->xy.x, s->pin_lbls[lbl]->xy.y);
        printf("Poly wound %d:", pl->polys[poly_index].l_rb);
        for (int ppp = 0; ppp < pl->polys[poly_index].num_points ; ppp++) {
          printf("(%d, %d)-> ", pl->polys[poly_index].coords[ppp].x, pl->polys[poly_index].coords[ppp].y);
        }
        printf("\n");
      }
    }
  }
  return 0;
}

int store_extra_contacts(sref_t* s, polylist_t* polylist) {
  char* covered = calloc(s->n_pins, sizeof(char));
  for (int p = 0; p < s->n_pins; p++) {
    if (covered[p]) continue;
    if (s->pins[p]->layernum == LI_LAYER_C && s->pins[p]->dtype == CONTACT_DTYPE) {
      XY_t centre = (XY_t){(s->pins[p]->pin.x1 + s->pins[p]->pin.x2)/2, (s->pins[p]->pin.y1 + s->pins[p]->pin.y2)/2};
      //check if centre of pin is in any LI shape stored with pinlbl
      for (int pinl = 0; pinl < s->n_lbls; pinl++) {
        if(strcmp(s->pin_lbls[pinl]->pinname, "VGND") == 0 || strcmp(s->pin_lbls[pinl]->pinname, "VPWR") == 0) continue;
        if(s->pins[p]->layernum != LI_LAYER_C) continue;
        if (inside_poly(centre, &s->pin_lbls[pinl]->LI_poly)){
          covered[p] = 1;
          //add to pin_lbl.contacts
          s->pin_lbls[pinl]->contacts[s->pin_lbls[pinl]->n_contacts] = centre;
          s->pin_lbls[pinl]->n_contacts++;
          //check if any metal1 shapes in the object
          for (int poly = 0; poly < polylist->num_polys; poly++) {
            if(polylist->polys[poly].layer == M1_LAYER_C && polylist->polys[poly].dtype == DRAW_DTYPE) {
              if(inside_poly(centre, &polylist->polys[poly])) {
                //copy the shape
                s->extra[s->n_extra] = malloc(1*sizeof(poly_t));
                *s->extra[s->n_extra] = polylist->polys[poly];
                s->n_extra++;

                //now search for any contacts other than p touching THIS shape..........
                for (int p2 = 0; p2 < s->n_pins; p2++) {
                  if (covered[p2] || s->pins[p2]->layernum != LI_LAYER_C) continue;
                  XY_t centre2 = (XY_t){(s->pins[p2]->pin.x1 + s->pins[p2]->pin.x2)/2, (s->pins[p2]->pin.y1 + s->pins[p2]->pin.y2)/2};
                  if (inside_poly(centre2, &polylist->polys[poly])) {
                    covered[p2] = 1;
                    printf("%d, %s x[%d, %d], y[%d, %d]\n", p2, s->pin_lbls[pinl]->pinname, s->pins[p2]->pin.x1 , s->pins[p2]->pin.x2, s->pins[p2]->pin.y1 , s->pins[p2]->pin.y2);
                    s->pin_lbls[pinl]->contacts[s->pin_lbls[pinl]->n_contacts] = centre2;
                    s->pin_lbls[pinl]->n_contacts++;
                  }
                }
                break;
              }
            }
          }
        }
      }
    }
  }
  free(covered);
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
  else h_vb = 1;

  for (int edge = 1; edge < p->num_points; edge++) {
    XY_t this = p->coords[edge];
    if (h_vb == 1)
    {
      //check if X coord is in horizontal extent
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
      //check if Y coord is in vertical extent
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
int translate_and_copy_contacts(XY_t shift, sref_t* sref, contact_t* out, int reflect, int rotate) {
  int32_t x1;
  int32_t x2;
  int32_t y1;
  int32_t y2;

  x1   = sref->pins[0]->pin.x1;
  x2   = sref->pins[0]->pin.x2;
  y1   = sref->pins[0]->pin.y1 * ((reflect == 1) ? -1 : 1);
  y2   = sref->pins[0]->pin.y2 * ((reflect == 1) ? -1 : 1);

  switch(rotate){
    case(0):
      out->pin.x1 = x1 + shift.x;
      out->pin.x2 = x2 + shift.x;
      out->pin.y1 = y1 + shift.y;
      out->pin.y2 = y2 + shift.y;
      break;
    case(1):
      out->pin.x1 = -y1 + shift.x;
      out->pin.x2 = -y2 + shift.x;
      out->pin.y1 = x2 + shift.y;
      out->pin.y2 = x1 + shift.y;
      break;
    case(2):
      out->pin.x1 = -x2 + shift.x;
      out->pin.x2 = -x1 + shift.x;
      out->pin.y1 = -y2 + shift.y;
      out->pin.y2 = -y1 + shift.y;
      break;
    default:
      fprintf(stderr, "Invalid rotate value specified (accepted values {0,1,2}).\n");
      return 1;
  }

  out->pinname[0] = '\0';
  out->dtype    = sref->pins[0]->dtype;
  out->layernum = sref->pins[0]->layernum;

  return 0;
}

XY_t rot90(XY_t xy) {
  XY_t o;
  o.x = -xy.y;
  o.y = xy.x;
  return o;
}

XY_t rot180(XY_t xy) {
  XY_t o;
  o.x = -xy.x;
  o.y = -xy.y;
  return o;
}

int translate_and_copy_shapes(FILE* out_file, XY_t shift, sref_t * sref, contact_list_t* clist, int reflect, int rotate) {
  poly_t test;
  char uid[16];
  int TEST = 0;
  //ignore vias
  if (sref->n_lbls == 0)
    return 0;
  //for each polygon instance, transform and check for intersection with each unlabelled contact
  for (int shape = 0; shape < sref->n_lbls; shape++) {
    memcpy(test.coords, sref->pin_lbls[shape]->LI_poly.coords,
       sizeof(XY_t)*sref->pin_lbls[shape]->LI_poly.num_points);
    test.l_rb = sref->pin_lbls[shape]->LI_poly.l_rb;
    test.l_rb = (reflect != 0) ? (! test.l_rb) : test.l_rb;
    test.num_points = sref->pin_lbls[shape]->LI_poly.num_points;
    //don't care about test.bound
    transform_shape(&test, shift, reflect, rotate);

    //lazy sweep through whole contact list, not tooooo big
    for (int contact = 0; contact < clist->num_contacts; contact++) {
      //check for unlabelled contact
      if (strcmp(clist->contacts[contact].pinname, "\0") == 0) {
        //check all four corners of the contact for intersection
        box_t box = clist->contacts[contact].pin;
        if (inside_poly((XY_t) {box.x1, box.y1}, &test) ||
          inside_poly((XY_t) {box.x1, box.y2}, &test) ||
          inside_poly((XY_t) {box.x2, box.y1}, &test) ||
          inside_poly((XY_t) {box.x2, box.y2}, &test)) {
          //found the pin
          snprintf(uid, 15, "_%d/", sref->next_uid);
          strcat(clist->contacts[contact].pinname, sref->strname);
          strcat(clist->contacts[contact].pinname, uid);
          strcat(clist->contacts[contact].pinname, sref->pin_lbls[shape]->pinname);
        }
      }
    }
  }

  //copy extra shapes into output.
  for (int shape = 0; shape < sref->n_extra; shape++) {
    memcpy(test.coords, sref->extra[shape]->coords, sizeof(XY_t)*sref->extra[shape]->num_points);
    test.l_rb = sref->extra[shape]->l_rb;
    test.l_rb = (reflect != 0) ? (! test.l_rb) : test.l_rb;
    test.num_points = sref->extra[shape]->num_points;
    fprintf(out_file, "{\"type\":\"shape\", \"layer\":%d, \"dtype\":%d, \"coords\":[\n",
      sref->extra[shape]->layer,sref->extra[shape]->dtype);
    transform_shape(&test, shift, reflect, rotate);
    for (int point = 0; point < test.num_points; point++) {
      fprintf(out_file, "[%d,%d],", test.coords[point].x, test.coords[point].y);
    }
    fseek(out_file, -1, SEEK_CUR);
    fprintf(out_file, "]},\n");
  }
  //write extra contacts as well
  char full_pinname[128];
  int total_pins = 0;
  for(int pin = 0; pin < sref->n_lbls; pin++) {
    int driver = 0;
    char *pinlbl = sref->pin_lbls[pin]->pinname;
    sprintf(full_pinname, "%s_%d/%s", sref->strname, sref->next_uid, sref->pin_lbls[pin]->pinname);
    if (strcmp(pinlbl,"Q") == 0 || strcmp(pinlbl,"Y") == 0 || strcmp(pinlbl,"X") == 0 ||
      strcmp(pinlbl,"LO") == 0 || strcmp(pinlbl,"HI") == 0  )  {
      driver = 1;
    }
    //printf(", contacts %d", sref->pin_lbls[pin]->n_contacts);
    XY_t point;
    for(int contact = 0; contact < sref->pin_lbls[pin]->n_contacts; contact++, total_pins++) {
      point = sref->pin_lbls[pin]->contacts[contact];
      point = transform_point(point, shift, reflect, rotate);
      fprintf(out_file, "{\"type\":\"pin\", \"driver\":%d, \"pinname\":\"%s\", \"loc\":[%d,%d]},\n",
        driver, full_pinname, point.x, point.y);
    }
  }

  return 0;
}

XY_t transform_point(XY_t p, XY_t shift, int reflect, int rotate) {
  switch(rotate){
      case(0):
          if (reflect != 0) p.y = - p.y;
        break;
      case(1): //90 degrees
          if (reflect != 0) p.y = - p.y;
          p = rot90(p);
        break;
      case(2): //180 degrees
          if (reflect != 0) p.y = - p.y;
          p = rot180(p);
        break;
      default:
        fprintf(stderr, "Error: invalid angle option \"%d\".", rotate);
    }//switch(rotate)
      p.x = p.x + shift.x;
      p.y = p.y + shift.y;
  return p;
}

void transform_shape(poly_t* p, XY_t shift, int reflect, int rotate) {
  switch(rotate){
      case(0):
        for (int coord = 0; coord < p->num_points; coord++) {
          if (reflect != 0) p->coords[coord].y = - p->coords[coord].y;
        }
        break;
      case(1): //90 degrees
        for (int coord = 0; coord < p->num_points; coord++) {
          if (reflect != 0) p->coords[coord].y = - p->coords[coord].y;
          p->coords[coord] = rot90(p->coords[coord]);
        }
        break;
      case(2): //180 degrees
        for (int coord = 0; coord < p->num_points; coord++) {
          if (reflect != 0) p->coords[coord].y = - p->coords[coord].y;
          p->coords[coord] = rot180(p->coords[coord]);
        }
        break;
      default:
        fprintf(stderr, "Error: invalid angle option \"%d\".", rotate);
    }//switch(rotate)
    for (int coord = 0; coord < p->num_points; coord++) {
      p->coords[coord].x = p->coords[coord].x + shift.x;
      p->coords[coord].y = p->coords[coord].y + shift.y;
    }
}

int write_contacts(FILE* out_file, contact_list_t* clist) {
  char pinname_copy[128];
  fprintf(out_file, "\"clist\" : [\n"); //open object, open array
  for(int contact = 0; contact < clist->num_contacts; contact++) {
    //properties to label
    //  pin : true          // designated the net segment as a driver/load,
                            // pin objects will not be trimmed from the final net object
    //  driver : <boolean>  // true/false depending on pin name
    //  pinname : <string>  // used to label nets/ build logic
    //  loc    : <xy_t>     // design rules require complete overlap of contact by metal,
                            // so just centre is enough to determine intersection.
    int driver = 0;
    strcpy(pinname_copy, clist->contacts[contact].pinname);
    char *pinlbl = strtok(pinname_copy, "/");

    pinlbl = strtok(NULL, "/");
    if (strcmp(pinlbl,"Q") == 0 || strcmp(pinlbl,"Y") == 0 || strcmp(pinlbl,"X") == 0 ||
        strcmp(pinlbl,"LO") == 0 || strcmp(pinlbl,"HI") == 0  )  {
      driver = 1;
    }
    XY_t centre = (XY_t) {clist->contacts[contact].pin.x1/2+clist->contacts[contact].pin.x2/2,
                          clist->contacts[contact].pin.y1/2+clist->contacts[contact].pin.y2/2};
    if(contact) fprintf(out_file, ",\n");
    fprintf(out_file, "  {\"type\":\"pin\", \"driver\":%d, \"pinname\":\"%s\", \"loc\":[%d,%d]}", driver, clist->contacts[contact].pinname, centre.x, centre.y);

  }
  fprintf(out_file, "],\n");
  return 0;
}

//in_file points to the first byte of the XY data
int write_poly(FILE* in_file, FILE* out_file, int coords, uint16_t lnum, uint16_t dtype) {
  if (lnum > 0) {
    //open an object
    XY_t xy;
    if (coords ==5 ) {
      box_t box;
      fread(&xy, sizeof(XY_t), 1, in_file);
      xy.x = ntohl(xy.x);
      xy.y = ntohl(xy.y);
      box.x1 = xy.x; box.x2 = xy.x; box.y1 = xy.y; box.y2 = xy.y;
      for (int c = 1; c < coords; c++) {
        fread(&xy, sizeof(XY_t), 1, in_file);
        xy.x = ntohl(xy.x);
        xy.y = ntohl(xy.y);
        box.x1 = (xy.x < box.x1) ? xy.x : box.x1;
        box.x2 = (xy.x > box.x2) ? xy.x : box.x2;
        box.y1 = (xy.y < box.y1) ? xy.y : box.y1;
        box.y2 = (xy.y > box.y2) ? xy.y : box.y2;
      }
      fprintf(out_file, "{\"type\": \"rect\", \"layer\":%d, \"dtype\":%d,\"x\":[%d,%d], \"y\":[%d,%d]},\n", lnum, dtype, box.x1, box.x2, box.y1, box.y2);
      //print a rect
    }else {
      //print a shape
      fprintf(out_file, "{\"type\": \"shape\", \"layer\":%d, \"dtype\":%d, \"coords\": [\n", lnum, dtype);
      for (int c = 0; c < coords; c++) {
        if (c) fprintf(out_file, ",");
        fread(&xy, sizeof(XY_t), 1, in_file);
        xy.x = ntohl(xy.x);
        xy.y = ntohl(xy.y);
        fprintf(out_file, "[%d,%d]", xy.x, xy.y);
      }
    fprintf(out_file, "]},\n");
    }
  }
  return 0;
}

//breaks a path into rectangles
int write_path(FILE* in_file, FILE* out_file, int len, uint16_t lnum, uint16_t dtype, uint16_t pathtype, int32_t width,
  int32_t bx, int32_t ex) {
  if (len != 2) {
    fprintf(stderr, "Pathtype with not 2 points: %d\n", len);
  }
  if(lnum > 0) {
    XY_t xy_start;
    fprintf(out_file, "{\"type\": \"rect\", \"layer\":%d, \"dtype\":%d,", lnum, dtype);
    //grab first point
    fread(&xy_start, sizeof(XY_t), 1, in_file);
    xy_start.x = ntohl(xy_start.x);
    xy_start.y = ntohl(xy_start.y);

    XY_t xy_end;
    fread(&xy_end, sizeof(XY_t), 1, in_file);
    xy_end.x = ntohl(xy_end.x);
    xy_end.y = ntohl(xy_end.y);

    int32_t x1, x2, y1, y2; //conventional box parameters
    int direction = (xy_end.y == xy_start.y); // 1 for horizontal
    if (pathtype == 4) {
      if (direction) {
        if (xy_start.x < xy_end.x) { // rightward
          xy_start.x -= bx;
          xy_end.x += ex;
        } else { //leftward
          xy_start.x += bx;
          xy_end.x   -= ex;
        }
      }else {
        if(xy_start.y < xy_end.y) { //upward
          xy_start.y -= bx;
          xy_end.y   += ex;
        } else { //downward
          xy_start.y += bx;
          xy_end.y   -= ex;
        }
      }
    }
    switch(pathtype) {
      case(4):
      case(0):
        if(direction) { //horizontal
          x1 = min(xy_start.x, xy_end.x);
          x2 = max(xy_start.x, xy_end.x);
          y1 = min(xy_start.y, xy_end.y) - width /2;
          y2 = max(xy_start.y, xy_end.y) + width /2;
          if(pathtype == 4) {
            x1 -= bx;
          }
          fprintf(out_file, "\"x\" : [%d,%d], \"y\": [%d,%d]}", x1,x2,y1,y2);
        } else {
          x1 = min(xy_start.x, xy_end.x) - width /2;
          x2 = max(xy_start.x, xy_end.x) + width /2;
          y1 = min(xy_start.y, xy_end.y);
          y2 = max(xy_start.y, xy_end.y);
          fprintf(out_file, "\"x\" : [%d,%d], \"y\": [%d,%d]}", x1,x2,y1,y2);
        }
        break;
      case(2):
        if(direction) { //horizontal
          x1 = min(xy_start.x, xy_end.x) - width /2;
          x2 = max(xy_start.x, xy_end.x) + width /2;
          y1 = min(xy_start.y, xy_end.y) - width /2;
          y2 = max(xy_start.y, xy_end.y) + width /2;
          fprintf(out_file, "\"x\" : [%d,%d], \"y\": [%d,%d]}", x1,x2,y1,y2);
        } else {
          x1 = min(xy_start.x, xy_end.x) - width /2;
          x2 = max(xy_start.x, xy_end.x) + width /2;
          y1 = min(xy_start.y, xy_end.y) - width /2;
          y2 = max(xy_start.y, xy_end.y) + width /2;
          fprintf(out_file, "\"x\" : [%d,%d], \"y\": [%d,%d]}", x1,x2,y1,y2);
        }
        break;
      default:
        fprintf(stderr, "write_path: Invalid pathtype.\n");
    }
    fprintf(out_file, ",\n");
  }
  return 0;
}

//these are all box types, being vias
int translate_and_write_shapes(FILE* out_file, XY_t shift, sref_t * sref, int reflect, int rotate) {
  //iterate through shapes in sref, transform and copy to output.
  if(strcmp(sref->strname, "sky130_fd_sc_hd__tapvpwrvgnd_1") == 0) {
    printf("printing a tap\n");
  }
  int32_t reflect_factor = (reflect == 0) ? 1 : -1;
  box_t box;

  for (int shape = 0; shape < sref->n_pins; shape++) {
    box = sref->pins[shape]->pin;
    //careful to preserve min/max
    if (reflect) {box.y1 = - sref->pins[shape]->pin.y2; box.y2 = -sref->pins[shape]->pin.y1;};
    switch(rotate) {
      case(0):
        break;
      case(1):
        box = rot90_box(box);
        break;
      case(2):
        box = rot180_box(box);
        break;
      default:
        fprintf(stderr, "translate_and_write_shapes: Inalid rotation argn.\n");
    }

        fprintf(out_file, "{\"type\":\"rect\", \"layer\":%d, \"dtype\":%d, \"x\":[%d,%d], \"y\":[%d,%d]},\n", sref->pins[shape]->layernum,
          sref->pins[shape]->dtype, box.x1 + shift.x, box.x2 + shift.x,
          box.y1 + shift.y, box.y2 + shift.y);
      }

  return 0;
}

//90 degrees counter clockwise in Cartesian coords
box_t rot90_box(box_t b) {
  box_t o;
  o.x1 = -b.y2;
  o.x2 = -b.y1;
  o.y1 = b.x1;
  o.y2 = b.x2;
}

//180 degrees counter clockwise in Cartesian coords
box_t rot180_box(box_t b) {
  box_t o;
  o.y1 = -b.y2;
  o.y2 = -b.y1;
  o.x1 = -b.x2;
  o.x2 = -b.x1;
}