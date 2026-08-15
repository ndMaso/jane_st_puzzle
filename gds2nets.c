//////////////////////////////////////////////////////////////////////
//  gds2nets.c
//  Nicholasd - 2026
//
//////////////////////////////////////////////////////////////////////
//  Description
//  -
//  - There are no PLEXes, BOXes, NODEs, PROPATTRs
//
//

//TODO
//Associate what pins you can with a label when read end of structure.
//fill out the two level case tree, mostly should be setting state and calling function
// which consume relevant data sections and write rectangles to file.

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <winsock2.h>
#include <winsock.h>
#include "gds_utils.h"

//Special layer and datatypes that we care about.

#define LI_LAYER_C 67
#define LI_PIN_DTYPE 16
#define LI_TEXT_DTYPE 5
#define LI_DRAW_DTYPE 20
#define BOUND_MARGIN 86 //licon half width + 1

int main(int argc, char* argv[]) {
// Parsing
  if (argc != 4) {
    perror("gds2nets: usage - <infile> <outfile> <top_level_strname>\n");
  }
  //Initialize

  FILE* fd = fopen("data/puzzle.gds", "rb");

  if (fd == NULL) {
    perror("Error opening input file\n");
    return 1;
  }

  FILE* out_file = fopen(argv[2], "w");

  if (out_file == NULL) {
    perror("Error opening output file\n");
    return 1;
  }

//-------------------------------------------------------------------
// Memory Allocation

  //holds relevant info for later srefs and outfile write operations
  structure_list_t* slist = malloc(sizeof(structure_list_t));
  slist->num_structs = 0;
  //temp buffer of all the li shapes in a sref.
  polylist_t* polylist = malloc(sizeof(polylist_t));
  polylist->num_polys = 0;

//-------------------------------------------------------------------
// Main loop
  int ret = build_structures(fd, out_file, slist, polylist, argv[3]);
  if (ret == 0) {
    //next stage
    printf("Got through to top struct.\n");
    for (int s = 0 ; s<slist->num_structs; s++) {
      sref_t* pcell = slist->structures[s];
      fprintf(out_file, "%s\n ", pcell->strname);
      for (int pin = 0 ; pin < pcell->n_lbls; pin++) {
        //fprintf(out_file, " %s: (%d, %d), ", pcell->pin_lbls[pin]->pinname, pcell->pin_lbls[pin]->x[0], pcell->pin_lbls[pin]->y[0]);
        fprintf(out_file, " %s, ", pcell->pin_lbls[pin]->pinname);
      }
      fprintf(out_file, "\n");
    }
  }


  for (int i = 0; i < slist->num_structs; i++) {
    for (int j = 0; j < slist->structures[i]->n_lbls; j++) {
      free(slist->structures[i]->pin_lbls[j]);
    }
    for (int j = 0; j < slist->structures[i]->n_pins; j++) {
      free(slist->structures[i]->pins[j]);
    }
    free(slist->structures[i]);
  }
  free(slist);

  free(polylist);

  fclose(fd);
  fclose(out_file);

  return 0;
}


int build_structures(FILE* in_file, FILE* out_file, structure_list_t* slist, polylist_t* polylist, char* top_strname) {
  //string for error messages. Max 512 characters + terminator
  char* str_buf = malloc(513*sizeof(char));
  char* struct_name = malloc(64*sizeof(char));
  int32_t boundary_coords[128];

  enum State stream_state = STREAM_S;
  uint16_t record_len;
  uint8_t record_type = HEADER;
  uint8_t last_record = START;
  uint8_t data_type; //data type of the record
  uint16_t layernum; //layernum of the element
  uint16_t dtype;    //data type of the element
  //space to hold a coordinate for later
  int32_t XY_coords[2];
  //this element is a pin or text object, don't skip.
  int writing = 0;
  do {
    //process a new record
    //all records start with length and record type
    int cursor = ftell(in_file);
    //printf("Cursor : %d\n", cursor);
    size_t result = fread(&record_len, sizeof(uint16_t), 1, in_file);
    record_len = ntohs(record_len);
    if (result != 1) {
      perror("Error reading record length\n");
      return 1;
    }

    result = fread(&record_type, sizeof(uint8_t), 1, in_file);
    if (result != 1) {
      perror("Error reading record type\n");
      return 1;
    }
    //printf("Record length : %d\n", record_len);
    //printf("Record type : %d\n", record_type);

    result = fread(&data_type, sizeof(uint8_t), 1, in_file);
    if (result != 1) {
      perror("Error reading data type\n");
      return 1;
    }

    //Two level nested switch statement
    switch (stream_state) {
      case STREAM_S:
        switch (last_record) {

          case (START):
            if (record_type == HEADER) {last_record = HEADER;}
            else {
              sprintf(str_buf, "Record error: state = STREAM:START, got = %d expect HEADER\n", (char) record_type);
              perror(str_buf);
              return 1;}
            break;

          case(HEADER):
            if (record_type == BGNLIB) {last_record = BGNLIB;}
            else {
              sprintf(str_buf, "Record error: state = STREAM:HEADER,got = %d expect BGNLIB\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(HEADER)

          case (BGNLIB):
            if (record_type == LIBNAME) {last_record = LIBNAME;}
            else {
              sprintf(str_buf, "Record error: state = STREAM:BGNLIB,got = %d expect LIBNAME\n", (char) record_type);
              perror(str_buf);
              return 1;}
            break;

          case (LIBNAME):
            if (record_type == UNITS) {last_record = UNITS;
              uint64_t unit1 ,unit2;
              fread(&unit1, sizeof(uint64_t), 1, in_file);
              fread(&unit2, sizeof(uint64_t), 1, in_file);
              unit1 = ntohll(unit1);
              unit2 = ntohll(unit2);
              write_units(out_file, unit1, unit2);
            }
            else {
              sprintf(str_buf, "Record error: state = STREAM:LIBNAME, got = %d expect UNITS\n", (char) record_type);
              perror(str_buf);
              return 1;}
            break;

          case (UNITS) :
          case (ENDSTR) :
            if (record_type == ENDLIB) {last_record = ENDLIB;
              stream_state = END_S;}
            else if (record_type == BGNSTR) {last_record = BGNSTR;
              //no action
              stream_state = STRUCTURE_S;}
            else {
              sprintf(str_buf, "Record error: state = STREAM:UNITS, got = %d expect ENDLIB || BGNSTR\n", (char) record_type);
              perror(str_buf);
              return 1;}
            break;
          default:
        }
        break;

      case(STRUCTURE_S):
        switch (last_record) {
          case(BGNSTR):
            if (record_type == STRNAME) { last_record = STRNAME;
              if (record_len > 60) {
                perror("STRNAME is too long\n");
                return 1;
              }
              //good to start a new structure;
              slist->num_structs++;
              slist->structures[slist->num_structs - 1] = malloc(sizeof(sref_t));
              slist->structures[slist->num_structs - 1]->n_lbls = 0;
              slist->structures[slist->num_structs - 1]->n_pins = 0;

              //read the structure name into the structure
              fread(str_buf, sizeof(char), (record_len - 4), in_file);
              //terminate string
              str_buf[record_len - 4] = '\0';

              //check if this is the top structure
              if (strcmp(str_buf, top_strname) == 0) {
                //equality, set state to exit
                stream_state = GOT_TOP_S;
              } else {
                //copy the new structure name to the new slist element.
                strcpy(slist->structures[slist->num_structs - 1]->strname, str_buf);
              }
            } else {
              sprintf(str_buf, "Record error: state = STRUCTURE:STRNAME, got = %d expect STRNAME\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; // case(BGNSTR)

          case(STRNAME):
          case(ENDEL):
            if (record_type == ENDSTR) { last_record = ENDSTR;
              //associate all pin shapes with a pin label.
              assign_pins(slist->structures[slist->num_structs - 1], polylist);
              //exit structure;
              stream_state = STREAM_S;
              polylist->num_polys = 0;
            }
            else if (record_type == BOUNDARY) { last_record = BOUNDARY;
              stream_state = BOUNDARY_S;
            }
            else if (record_type == PATH) {last_record = PATH;
              stream_state = PATH_S;
            }
            else if (record_type == AREF) {//Hoping this isn't there i think
              perror("Saw an aref, terminating\n");
              return 1;
            }
            else if (record_type == TEXT) {last_record = TEXT;
              stream_state = TEXT_S;}
            else if (record_type == NODE) {//Unsupported
              perror("Saw a node, terminating\n");
              return 1;
            }
            else if (record_type == SREF) {//NO srefs until top struct
              perror("Saw an sref before the top structure\n");
              return 1;
            }
            else if (record_type == BOX) {//Unsupported;
              perror("Saw a box, terminating\n");
              return 1;
            }
            else {
              sprintf(str_buf, "Record error: got = %d expect STRUCTURE:ELEMENT || ENDSTR\n", (char) record_type);
              perror(str_buf);
              return 1;
            }

            break; // case(STRNAME)
          default:
        }

        break; // case(STRUCTURE)

      case(BOUNDARY_S):
        switch (last_record) {
          case(BOUNDARY):
            if (record_type == LAYER) {last_record = LAYER;
              //if LAYER is not li it's not a pin so we don't care


              fread(&layernum, sizeof(uint16_t), 1, in_file);
              layernum = ntohs(layernum);
              writing = (layernum == LI_LAYER_C) ? 1 : 0;

            } else {
              sprintf(str_buf, "Record error: state = BOUNDARY:BOUNDARY, got = %d expect LAYER\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(PATH)
          case(LAYER):
            if (record_type == DATATYPE) { last_record = DATATYPE;

              fread(&dtype, sizeof(uint16_t), 1, in_file);
              dtype = ntohs(dtype);
              //if its a pin box or a li.
              writing = (dtype == LI_PIN_DTYPE || dtype == LI_DRAW_DTYPE) ? writing : 0;
            } else {
              sprintf(str_buf, "Record error: state = BOUNDARY:LAYER, got = %d expect DATATYPE\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(LAYER)

          case(DATATYPE):
            if (record_type == XY) {last_record = XY;
              if (layernum != LI_LAYER_C) break; //only interested in this layer
              switch(dtype) {
                case(LI_PIN_DTYPE):
                  //Process the bounary, which is a box object.
                  //add a pin
                  int this_n_pins = slist->structures[slist->num_structs - 1]->n_pins;
                  slist->structures[slist->num_structs - 1]->pins[this_n_pins] = malloc(sizeof(pin_t));

                  //grab all the coordinates, technically only using 6.
                  fread(boundary_coords, sizeof(int32_t), 10, in_file);
                  for (int c = 0; c < 10; c++) {boundary_coords[c] = ntohl(boundary_coords[c]);}
                  int32_t x1 = min(min(boundary_coords[0], boundary_coords[2]), boundary_coords[4]);
                  int32_t x2 = max(max(boundary_coords[0], boundary_coords[2]), boundary_coords[4]);
                  int32_t y1 = min(min(boundary_coords[1], boundary_coords[3]), boundary_coords[5]);
                  int32_t y2 = max(max(boundary_coords[1], boundary_coords[3]), boundary_coords[5]);
                  //populate the pin coordinates.
                  *(slist->structures[slist->num_structs - 1]->pins[this_n_pins]) = (pin_t){x1,x2,y1,y2};
                  break; //case(LI_PIN_DTYPE);

                case(LI_DRAW_DTYPE):
                  //process the boundary, which is a li layer, there are record_len - 4 bytes of coords
                  fread(boundary_coords, sizeof(int32_t), (record_len - 4)/sizeof(int32_t), in_file);
                  for (int c = 0; c < (record_len-4)/sizeof(int32_t)/2; c++) {
                    //hard copy the coords into the polylist for this structure.
                    polylist->polys[polylist->num_polys].coords[c].x = ntohl(boundary_coords[2*c]);
                    polylist->polys[polylist->num_polys].coords[c].y = ntohl(boundary_coords[(2*c)+1]);
                  }
                  polylist->polys[polylist->num_polys].num_points = (record_len-4)/sizeof(int32_t)/2;
                  //assign winding and bound box.
                  config_poly(&(polylist->polys[polylist->num_polys]), BOUND_MARGIN);
                  polylist->num_polys++;
                  break;//case(LI_DRAW_DTYPE)
                default:
              }//switch(dtype)
            }
            else {
              sprintf(str_buf, "Record error: state = BOUNDARY:DATATYPE, got = %d, expected XY\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(DATATYPE)

          case(XY):
            if (record_type == ENDEL) { last_record = ENDEL;
              stream_state = STRUCTURE_S;
            } else {
              sprintf(str_buf, "Record error: state = BOUNDARY:XY, got = %d, expect ENDEL\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(XY)
          default:
        }
        break; //case(BOUNDARY)

      case(PATH_S):
        switch (last_record) {
          case(PATH):
            if (record_type == LAYER) {last_record = LAYER;
              //if LAYER is not li it's not a pin so we don't care

              uint16_t layernum;
              fread(&layernum, sizeof(uint16_t), 1, in_file);
              layernum = ntohs(layernum);

              writing = (layernum == LI_LAYER_C) ? 1 : 0;

            } else {
              sprintf(str_buf, "Record error: state = PATH:PATH, got = %d expect LAYER\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(PATH)
          case(LAYER):
            if (record_type == DATATYPE) { last_record = DATATYPE;
              fread(&dtype, sizeof(uint16_t), 1, in_file);
              dtype = ntohs(dtype);
              //Assume no path pins
if (dtype == LI_PIN_DTYPE && writing == 1) {
  perror("Turns out there are pins which are PATH elements: FIXME\n");
  return 1;
}
            } else {
              sprintf(str_buf, "Record error: state = PATH:LAYER, got = %d expect DATATYPE\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(LAYER)

          case(DATATYPE):
            if (record_type == PATHTYPE) { last_record = PATHTYPE;}
            else if (record_type == WIDTH) {last_record = WIDTH;}
            else if (record_type == XY) {last_record = XY;}
            else {
              sprintf(str_buf, "Record error: state = PATH:DATATYPE, got = %d, expected PATHTYPE || WIDTH || XY\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(DATATYPE)

          case(PATHTYPE):
            if (record_type == WIDTH) { last_record = WIDTH;}
            else if (record_type == XY) {last_record = XY;}
            else {
              sprintf(str_buf, "Record error: state = PATH:PATHTYPE, got = %d, expected WIDTH || XY\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(PATHTYPE)

          case(WIDTH):
            if (record_type == XY) { last_record = XY;
            } else {
              sprintf(str_buf, "Record error: state = PATH:WIDTH, got = %d, expect XY\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(WIDTH)

          case(XY):
            if (record_type == ENDEL) { last_record = ENDEL;
              stream_state = STRUCTURE_S;
            } else {
              sprintf(str_buf, "Record error: state = PATH:XY, got = %d, expect ENDEL\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(XY)
          default:
        }
        break; //case(PATH)

      case(TEXT_S):
        switch(last_record) {
          case(TEXT):
            if (record_type == LAYER) {last_record = LAYER;
              //if LAYER is not li it's not a pin so we don't care

              uint16_t layernum;
              fread(&layernum, sizeof(uint16_t), 1, in_file);
              layernum = ntohs(layernum);
              writing = (layernum == LI_LAYER_C) ? 1 : 0;
            } else {
              sprintf(str_buf, "Record error: state = BOUNDARY:BOUNDARY, got = %d expect LAYER\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(TEXT)

          case(LAYER):
            if (record_type == TEXTTYPE) {last_record = TEXTTYPE;
              //Only care about pin labels which are on texttype 5
              uint16_t texttype;
              fread(&texttype, sizeof(uint16_t), 1, in_file);
              texttype = ntohs(texttype);
              writing = (texttype == LI_TEXT_DTYPE) ? writing : 0;
            }
            else {
              sprintf(str_buf, "Record error: state = TEXT:LAYER, got = %d expect TEXTTYPE\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(LAYER)

          case(TEXTTYPE):
            if (record_type == PRESENTATION)  {last_record = PRESENTATION;} //no action
            else if (record_type == PATHTYPE) {last_record = PATHTYPE;} //no action
            else if (record_type == WIDTH)    {last_record = WIDTH;} //no action
            else if (record_type == STRANS)   {last_record = STRANS;}//no action required for text
            else if (record_type == XY)       {last_record = XY;
              if (writing == 1) {
                fread(XY_coords, sizeof(int32_t), 2, in_file);
                XY_coords[0] = ntohl(XY_coords[0]);
                XY_coords[1] = ntohl(XY_coords[1]);
              }
            }
            else {
              sprintf(str_buf, "Record error: state = TEXT:TEXTTYPE, got = %d expect PRESENTATION || PATHTYPE || WIDTH || STRANS || XY.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(TEXTTYPE)

          case(PRESENTATION):
            if (record_type == PATHTYPE)    {last_record = PATHTYPE;} //no action
            else if (record_type == WIDTH)  {last_record = WIDTH;} //no action
            else if (record_type == STRANS) {last_record = STRANS;} //no action required for text
            else if (record_type == XY)     {last_record = XY;
              if (writing == 1) {
                fread(XY_coords, sizeof(int32_t), 2, in_file);
                XY_coords[0] = ntohl(XY_coords[0]);
                XY_coords[1] = ntohl(XY_coords[1]);
              }
            } else {
              sprintf(str_buf, "Record error: state = TEXT:PRESENTATION, got = %d expect PATHTYPE || WIDTH || STRANS || XY.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(PRESENTATION)

          case(PATHTYPE):
            if (record_type == WIDTH)         {last_record = WIDTH;} //no action
            else if (record_type == STRANS)   {last_record = STRANS;}//no action required for text.
            else if (record_type == XY)       {last_record = XY;
              if (writing == 1) {
                fread(XY_coords, sizeof(int32_t), 2, in_file);
                XY_coords[0] = ntohl(XY_coords[0]);
                XY_coords[1] = ntohl(XY_coords[1]);
              }
            } else {
              sprintf(str_buf, "Record error: state = TEXT:PATHTYPE, got = %d expect WIDTH || STRANS || XY.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(PATHTYPE)

          case(WIDTH):
            if (record_type == STRANS)   {last_record = STRANS;}//no action required for text.
            else if (record_type == XY)       {last_record = XY;
              if (writing == 1) {
                fread(XY_coords, sizeof(int32_t), 2, in_file);
                XY_coords[0] = ntohl(XY_coords[0]);
                XY_coords[1] = ntohl(XY_coords[1]);
              }
            } else {
              sprintf(str_buf, "Record error: state = TEXT:WIDTH, got = %d expect WIDTH || XY.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(WIDTH)

          case(STRANS):
            if (record_type == MAG) {last_record = MAG;}
            else if (record_type == ANGLE) {last_record = ANGLE;}
            else if (record_type == XY) {last_record = XY;
              if (writing == 1) {
                fread(XY_coords, sizeof(int32_t), 2, in_file);
                XY_coords[0] = ntohl(XY_coords[0]);
                XY_coords[1] = ntohl(XY_coords[1]);
              }
            } else {
              sprintf(str_buf, "Record error: state = TEXT:STRANS, got = %d expect MAG || ANGLE || XY.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(STRANS)

          case(MAG):
            if (record_type == ANGLE) {last_record = ANGLE;}
            else if (record_type == XY) {last_record = XY;
              if (writing == 1) {
                fread(XY_coords, sizeof(int32_t), 2, in_file);
                XY_coords[0] = ntohl(XY_coords[0]);
                XY_coords[1] = ntohl(XY_coords[1]);
              }
            } else {
              sprintf(str_buf, "Record error: state = TEXT:MAG, got = %d expect ANGLE || XY.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(MAG)

          case(ANGLE):
            if (record_type == XY) {last_record = XY;
              if (writing == 1) {
                fread(XY_coords, sizeof(int32_t), 2, in_file);
                XY_coords[0] = ntohl(XY_coords[0]);
                XY_coords[1] = ntohl(XY_coords[1]);
              }
            } else {
              sprintf(str_buf, "Record error: state = TEXT:MAG, got = %d expect XY.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case(ANGLE)

          case(XY):
            if (record_type == STRING) {last_record = STRING;
              //get the string
              if (writing == 1) {
                fread(str_buf, sizeof(char), record_len - 4, in_file);
                //terminate
                str_buf[record_len - 4] = '\0';
                process_pin_name(str_buf, slist->structures[slist->num_structs - 1], XY_coords[0], XY_coords[1]);
              }
            } else {
              sprintf(str_buf, "Record error: state = TEXT:XY, got = %d expect STRING.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }

            break;//case(XY)

          case(STRING):
            if (record_type == ENDEL) {last_record = ENDEL;
              stream_state = STRUCTURE_S;
            } else {
              sprintf(str_buf, "Record error: state = TEXT:STRING, got = %d expect ENDEL.\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break;//case
          default:
        }
        break; //case(TEXT)

      case (GOT_TOP_S):
      default :
        //if GOT_TOP or unrecognised, should have broken out already
        perror("stream_state is GOT_TOP but did not break.\n");
        return 1;
    }

    //advance cursor to next record.
    //when finished, leave the cursor alligned to the first element of the top_structure
    fseek(in_file, cursor + sizeof(uint8_t) * record_len, SEEK_SET);
  }
  while (record_type != ENDLIB && stream_state != GOT_TOP_S);

  free(str_buf);
  free(struct_name);
  return 0;
}
