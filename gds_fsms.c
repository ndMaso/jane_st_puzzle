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

#define BOUND_MARGIN 86 //licon half width + 1

#define IO_LNUM 70      //There's boundary objects at the inputs/outputs
#define IO_DTYPE 20


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
                //good to start a new structure;
                slist->num_structs++;
                slist->structures[slist->num_structs - 1] = malloc(sizeof(sref_t));
                slist->structures[slist->num_structs - 1]->n_lbls = 0;
                slist->structures[slist->num_structs - 1]->n_pins = 0;
                slist->structures[slist->num_structs - 1]->next_uid = 0;
                slist->structures[slist->num_structs - 1]->n_extra = 0;
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
              printf("%s n lbls %d\n", slist->structures[slist->num_structs - 1]->strname, slist->structures[slist->num_structs - 1]->n_lbls);
              for (int i = 0; i < slist->structures[slist->num_structs - 1]->n_lbls; i++) {
                printf("%s ", slist->structures[slist->num_structs - 1]->pin_lbls[i]->pinname);
              }
              printf("\n");
              //store LI_DRAW shapes connected to a pin label with the label
              assign_pins(slist->structures[slist->num_structs - 1], polylist);
              //look for floating contacts already in the pcell for whatever reason. (dftrp, xor2)
              store_extra_contacts(slist->structures[slist->num_structs - 1], polylist);
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
              fread(&layernum, sizeof(uint16_t), 1, in_file);
              layernum = ntohs(layernum);

            } else {
              sprintf(str_buf, "Record error: state = BOUNDARY:BOUNDARY, got = %d expect LAYER\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(BOUNDARY)
          case(LAYER):
            if (record_type == DATATYPE) { last_record = DATATYPE;

              fread(&dtype, sizeof(uint16_t), 1, in_file);
              dtype = ntohs(dtype);
              //if its a pin box or a li.
            } else {
              sprintf(str_buf, "Record error: state = BOUNDARY:LAYER, got = %d expect DATATYPE\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(LAYER)

          case(DATATYPE):
            if (record_type == XY) {last_record = XY;
              //shape needed only for li draw and metal 1.
              if((layernum == LI_LAYER_C || layernum == M1_LAYER_C) && dtype == DRAW_DTYPE) {

                //process the boundary, which is a li layer, there are record_len - 4 bytes of coords
                fread(boundary_coords, sizeof(int32_t), (record_len - 4)/sizeof(int32_t), in_file);
                for (int c = 0; c < (record_len-4)/sizeof(int32_t)/2; c++) {
                  //hard copy the coords into the polylist for this structure.
                  polylist->polys[polylist->num_polys].coords[c].x = ntohl(boundary_coords[2*c]);
                  polylist->polys[polylist->num_polys].coords[c].y = ntohl(boundary_coords[(2*c)+1]);
                }
                polylist->polys[polylist->num_polys].num_points = (record_len-4)/sizeof(int32_t)/2;
                polylist->polys[polylist->num_polys].layer = layernum; //distinguish between LI and M1
                polylist->polys[polylist->num_polys].dtype = dtype;
                //assign winding and bound box.
                config_poly(&(polylist->polys[polylist->num_polys]), BOUND_MARGIN);
                polylist->num_polys++;
              } else { //else put vias /contacts in the pins
                if (dtype == CONTACT_DTYPE) {
                  //Process the bounary, which is a box object.
                  //add a pin
                  int this_n_pins = slist->structures[slist->num_structs - 1]->n_pins++;
                  if (this_n_pins >= 128) {
                    printf("Error: %s has too many pins\n", slist->structures[slist->num_structs - 1]->strname);
                    return 1;
                  }
                  slist->structures[slist->num_structs - 1]->pins[this_n_pins] = malloc(sizeof(pin_t));
                  //grab all the coordinates, technically only using 6.
                  fread(boundary_coords, sizeof(int32_t), 10, in_file);
                  for (int c = 0; c < 10; c++) {boundary_coords[c] = ntohl(boundary_coords[c]);}
                  int32_t x1 = min(min(boundary_coords[0], boundary_coords[2]), boundary_coords[4]);
                  int32_t x2 = max(max(boundary_coords[0], boundary_coords[2]), boundary_coords[4]);
                  int32_t y1 = min(min(boundary_coords[1], boundary_coords[3]), boundary_coords[5]);
                  int32_t y2 = max(max(boundary_coords[1], boundary_coords[3]), boundary_coords[5]);
                  //populate the pin coordinates.
                  slist->structures[slist->num_structs - 1]->pins[this_n_pins]->pin.x1 = x1;
                  slist->structures[slist->num_structs - 1]->pins[this_n_pins]->pin.x2 = x2;
                  slist->structures[slist->num_structs - 1]->pins[this_n_pins]->pin.y1 = y1;
                  slist->structures[slist->num_structs - 1]->pins[this_n_pins]->pin.y2 = y2;
                  slist->structures[slist->num_structs - 1]->pins[this_n_pins]->dtype = dtype;
                  slist->structures[slist->num_structs - 1]->pins[this_n_pins]->layernum = layernum;
                }
              }
            } else {
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

              uint16_t layernum;
              fread(&layernum, sizeof(uint16_t), 1, in_file);
              layernum = ntohs(layernum);

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
              if (dtype == LI_PIN_DTYPE && layernum == LI_LAYER_C) {
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
            } else {
              sprintf(str_buf, "Record error: state = BOUNDARY:BOUNDARY, got = %d expect LAYER\n", (char) record_type);
              perror(str_buf);
              return 1;
            }
            break; //case(TEXT)

          case(LAYER):
            if (record_type == TEXTTYPE) {last_record = TEXTTYPE;
              //Only care about pin labels which are on texttype 5
              fread(&dtype, sizeof(uint16_t), 1, in_file);
              dtype = ntohs(dtype);
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
              if (dtype == LI_TEXT_DTYPE && layernum == LI_LAYER_C) {
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
              if (dtype == LI_TEXT_DTYPE && layernum == LI_LAYER_C) {
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
              if (dtype == LI_TEXT_DTYPE && layernum == LI_LAYER_C) {
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
              if (dtype == LI_TEXT_DTYPE && layernum == LI_LAYER_C) {
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
              if (dtype == LI_TEXT_DTYPE && layernum == LI_LAYER_C) {
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
              if (dtype == LI_TEXT_DTYPE && layernum == LI_LAYER_C) {
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
              if (dtype == LI_TEXT_DTYPE && layernum == LI_LAYER_C) {
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
              if (dtype == LI_TEXT_DTYPE && layernum == LI_LAYER_C) {
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


//given a gds file pointing at the top level struct, builds a list of licon contacts and returns the cursor
//when done. Sorts the list by increasing y coordinate.
//FILE* in_file : the gds stream
//sref_t* via_ref : structure element corresponding to the LICON via.
//return - buf : large buffer with room for all via contacts drawn by the top structure.
int build_contact_list(FILE* in_file, sref_t* via_ref, contact_list_t* buf) {
  //hold the structure name desired locally
  char strname[strlen(via_ref->strname) + 1]; //+1 for term
  memcpy(strname, via_ref->strname, strlen(via_ref->strname) + 1);
  //run the state machine to the end, but only look for sref elements matching the via_ref name

  char* strbuf = malloc(sizeof(char)*256);
  enum State stream_state = STRUCTURE_S;
  enum RType last_record = STRNAME;
  uint16_t record_len;
  uint8_t record_type;
  uint8_t data_type;
  int reflect = 0;
  int rotate = 0;    //1,2,3 = 90,180,270 degrees

  //save state of cursor so can move it back at EOF
  uint64_t start_cursor = ftell(in_file);
  int angle;
  XY_t shift;

  do {
    //process a new record
    //all records start with length and record type
    uint64_t cursor = ftell(in_file);
    //printf("Cursor : %d\n", cursor);
    size_t result = fread(&record_len, sizeof(uint16_t), 1, in_file);
    record_len = ntohs(record_len);
    if (result != 1) {
      fprintf(stderr, "Error reading record length, last_record = %d, cursor = %lu\n", last_record, ftell(in_file));
      return 1;
    }

    result = fread(&record_type, sizeof(uint8_t), 1, in_file);
    if (result != 1) {
      perror("Error reading record type\n");
      return 1;
    }

    result = fread(&data_type, sizeof(uint8_t), 1, in_file);
    if (result != 1) {
      perror("Error reading data type\n");
      return 1;
    }

    switch(stream_state) {
      case(STREAM_S):
        //Should just be ENDLIB left
        switch(last_record) {
          case(ENDSTR):
            if(record_type == ENDLIB) {last_record=ENDLIB;
            }
            else {
              sprintf(strbuf, "Record error: state = STREAM:ENDSTR, got = %d expect ENDLIB.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(ENDSTR)
          default:
        } //switch(last_record)
        break;//case(STREAM_S)

      case(STRUCTURE_S):
        switch(last_record) {
          case(ENDEL):   //a sref end el
          case(STRNAME): //entry point
            if(record_type == ENDSTR) {last_record = ENDSTR;
              stream_state = STREAM_S;
            }
            else if (record_type == SREF){last_record = SREF;
              stream_state = SREF_S;
            }
            else {
              //do nothing, records will continue to process until an sref is seen.
            }
            break;//case(STRNAME) || case(ENDEL)
          default:
        }//switch(last_record)
        break; //case(STRUCTURE_S)

      case(SREF_S):
        switch(last_record) {
          case(SREF):
            if(record_type = SNAME) {
              //check if the name matches
              fread(strbuf, sizeof(char), record_len - 4, in_file);
              strbuf[record_len - 4] = '\0';
              if(strcmp(strbuf, strname) == 0) {
                last_record = SNAME;
              } else {
                //if no match, jump back to looking for SREF
                stream_state = STRUCTURE_S;
                last_record = ENDEL;
              }
            } else {
              sprintf(strbuf, "Record error: state = SREF:SREF, got = %d expect SNAME.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//cas(SREF)
          case(SNAME):
            if(record_type == STRANS) {last_record = STRANS;
              //check for x axis reflect
              uint8_t strans_rec;
              fread(&strans_rec, sizeof(uint8_t), 1, in_file);
              if ((strans_rec & ((uint8_t)1 << 7)) != 0)
                reflect = 1;
            } else if (record_type == XY){last_record = XY;
              //grab the single pin of the via and draw it to the contact list.
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_copy_contacts(shift, via_ref, &buf->contacts[buf->num_contacts], reflect, rotate) != 0)
                return 1;
              buf->num_contacts++;
            } else {
              sprintf(strbuf, "Record error: state = SREF:SNAME, got = %d expect STRANS || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(SNAME)
          case(STRANS):
            if(record_type == MAG){last_record = MAG;
              fprintf(stderr, "Error: unimplemented feature, SREF:MAG\n");
              return 1;
            }
            else if (record_type == ANGLE) {last_record = ANGLE;
              //Check if it's 180 flipped.
              uint8_t angle_rec;
              fseek(in_file, 1, SEEK_CUR);
              fread(&angle_rec, sizeof(uint8_t), 1, in_file);
              //check against rotations of 90, 180, and 270 degrees
              switch(angle_rec){
                case(90):
                  rotate = 1;
                break;
                case(180):
                  rotate = 2;
                break;
                default:
                  fprintf(stderr, "Unsupported angle detected SREF:ANGLE\n");
              }//switch(angle_rec)
            }
            else if (record_type == XY) {last_record = XY;
              //same as above
              //grab the single pin of the via and draw it to the contact list.
              XY_t shift;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_copy_contacts(shift, via_ref, &buf->contacts[buf->num_contacts], reflect, rotate) != 0)
                return 1;
              buf->num_contacts++;
            } else {
              sprintf(strbuf, "Record error: state = SREF:STRANS, got = %d expect MAG || ANGLE || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(STRANS)
          case(MAG):
            if (record_type = ANGLE) {last_record = ANGLE;
              //Check if it's 180 flipped.
              uint8_t angle_rec;
              fseek(in_file, 1, SEEK_CUR);
              fread(&angle_rec, sizeof(uint8_t), 1, in_file);
              //check against rotations of 90, 180, and 270 degrees
              switch(angle_rec){
                case(90):
                  rotate = 1;
                break;
                case(180):
                  rotate = 2;
                break;
                default:
                  fprintf(stderr, "Unsupported angle detected SREF:ANGLE\n");
              }//switch(angle_rec)
            }
            else if(record_type == XY) {last_record=XY;
              XY_t shift;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_copy_contacts(shift, via_ref, &buf->contacts[buf->num_contacts], reflect, rotate) !=0)
                return 1;
              buf->num_contacts++;
            }
            else{
              sprintf(strbuf, "Record error: state = SREF:MAG, got = %d expect ANGLE || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(MAG)
          case(ANGLE):
            if(record_type == XY) {last_record=XY;
              XY_t shift;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_copy_contacts(shift, via_ref, &buf->contacts[buf->num_contacts], reflect, rotate) != 0)
                return 1;
              buf->num_contacts++;
            }
            break;//case(ANGLE)
          case(XY):
            if(record_type == ENDEL){last_record=ENDEL;
              stream_state = STRUCTURE_S;
              reflect = 0;
              rotate = 0;
            }
            else {
              sprintf(strbuf, "Record error: state = SREF:XY, got = %d expect ENDEL.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(XY)
          default:
        }//switch(last_record)
        break;//case(SREF_S)
      default:
    } //switch(stream_state)
    //advance cursor to next record.
    //when finished, leave the cursor alligned to the first element of the top_structure
    fseek(in_file, cursor + sizeof(uint8_t) * record_len, SEEK_SET);
  }
  while(last_record != ENDLIB);

  //put the required info into the buffer
  //REMEMBER TO NULL THE PINNAME.
  fseek(in_file, start_cursor, SEEK_SET);
  free(strbuf);
  return 0;
}

int label_contacts(FILE* in_file, FILE* out_file, contact_list_t* clist, structure_list_t* slist) {
  char* strbuf = malloc(sizeof(char)*256);
  enum State stream_state = STRUCTURE_S;
  enum RType last_record = STRNAME;
  uint16_t record_len;
  uint8_t record_type;
  uint8_t data_type;
  int reflect = 0;
  int rotate = 0;    //1,2,3 = 90,180,270 degrees

  //save state of cursor so can move it back at EOF
  uint64_t start_cursor = ftell(in_file);
  int angle;
  XY_t shift;

  sref_t* pcell;
  //open the extra objs object


  do {
    //process a new record
    //all records start with length and record type
    uint64_t cursor = ftell(in_file);
    //printf("Cursor : %d\n", cursor);
    size_t result = fread(&record_len, sizeof(uint16_t), 1, in_file);
    record_len = ntohs(record_len);
    if (result != 1) {
      fprintf(stderr, "Error reading record length, last_record = %d, cursor = %lu\n", last_record, ftell(in_file));
      return 1;
    }

    result = fread(&record_type, sizeof(uint8_t), 1, in_file);
    if (result != 1) {
      perror("Error reading record type\n");
      return 1;
    }

    result = fread(&data_type, sizeof(uint8_t), 1, in_file);
    if (result != 1) {
      perror("Error reading data type\n");
      return 1;
    }
    switch(stream_state) {
      case(STREAM_S):
        //Should just be ENDLIB left
        switch(last_record) {
          case(ENDSTR):
            if(record_type == ENDLIB) {last_record=ENDLIB;
            }
            else {
              sprintf(strbuf, "Record error: state = STREAM:ENDSTR, got = %d expect ENDLIB.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(ENDSTR)
          default:
        } //switch(last_record)
        break;//case(STREAM_S)

      case(STRUCTURE_S):
        switch(last_record) {
          case(ENDEL):   //a sref end el
          case(STRNAME): //entry point
            if(record_type == ENDSTR) {last_record = ENDSTR;
              stream_state = STREAM_S;
            }
            else if (record_type == SREF){last_record = SREF;
              stream_state = SREF_S;
            }
            else {
              //do nothing, records will continue to process until an sref is seen.
            }
            break;//case(STRNAME) || case(ENDEL)
          default:
        }//switch(last_record)
        break; //case(STRUCTURE_S)

      case(SREF_S):
        switch(last_record) {
          case(SREF):
            if(record_type = SNAME) {
              //check if the name matches
              fread(strbuf, sizeof(char), record_len - 4, in_file);
              strbuf[record_len - 4] = '\0';

              //Search for the structure
              int found;
              for (int cell =0; cell < slist->num_structs; cell++) {
                if (strcmp(slist->structures[cell]->strname, strbuf) == 0) {
                  pcell = slist->structures[cell];
                  pcell->next_uid++;
                  found = 1;
                  break;
                }
              }
              if (found == 0) {
                fprintf(stderr, "Did not find referenced structure \"%s\".\n", strbuf);
                return 1;
              }
              //all pcells start with sky130
              if(strncmp(strbuf, "sky130", 6) == 0 && found == 1) {
                last_record = SNAME;
              } else {
                //if no match, jump back to looking for SREF
                stream_state = STRUCTURE_S;
                last_record = ENDEL;
              }
            } else {
              sprintf(strbuf, "Record error: state = SREF:SREF, got = %d expect SNAME.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//cas(SREF)
          case(SNAME):
            if(record_type == STRANS) {last_record = STRANS;
              //check for x axis reflect
              uint8_t strans_rec;
              fread(&strans_rec, sizeof(uint8_t), 1, in_file);
              if ((strans_rec & ((uint8_t)1 << 7)) != 0)
                reflect = 1;
            } else if (record_type == XY){last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_copy_shapes(out_file, shift, pcell, clist, reflect, rotate) != 0)
                return 1;
            } else {
              sprintf(strbuf, "Record error: state = SREF:SNAME, got = %d expect STRANS || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(SNAME)
          case(STRANS):
            if(record_type == MAG){last_record = MAG;
              fprintf(stderr, "Error: unimplemented feature, SREF:MAG\n");
              return 1;
            }
            else if (record_type == ANGLE) {last_record = ANGLE;
              //Check if it's 180 flipped.
              uint8_t angle_rec;
              fseek(in_file, 1, SEEK_CUR);
              fread(&angle_rec, sizeof(uint8_t), 1, in_file);
              //check against rotations of 90, 180, and 270 degrees
              switch(angle_rec){
                case(90):
                  rotate = 1;
                break;
                case(180):
                  rotate = 2;
                break;
                default:
                  fprintf(stderr, "Unsupported angle detected SREF:ANGLE\n");
              }//switch(angle_rec)
            }
            else if (record_type == XY) {last_record = XY;
              //same as above
              XY_t shift;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_copy_shapes(out_file, shift, pcell, clist, reflect, rotate) != 0)
                return 1;
            } else {
              sprintf(strbuf, "Record error: state = SREF:STRANS, got = %d expect MAG || ANGLE || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(STRANS)
          case(MAG):
            if (record_type = ANGLE) {last_record = ANGLE;
              //Check if it's 180 flipped.
              uint8_t angle_rec;
              fseek(in_file, 1, SEEK_CUR);
              fread(&angle_rec, sizeof(uint8_t), 1, in_file);
              //check against rotations of 90, 180, and 270 degrees
              switch(angle_rec){
                case(90):
                  rotate = 1;
                break;
                case(180):
                  rotate = 2;
                break;
                default:
                  fprintf(stderr, "Unsupported angle detected SREF:ANGLE\n");
              }//switch(angle_rec)
            }
            else if(record_type == XY) {last_record=XY;
              XY_t shift;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_copy_shapes(out_file, shift, pcell, clist, reflect, rotate) != 0)
                return 1;
            }
            else{
              sprintf(strbuf, "Record error: state = SREF:MAG, got = %d expect ANGLE || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(MAG)
          case(ANGLE):
            if(record_type == XY) {last_record=XY;
              XY_t shift;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_copy_shapes(out_file, shift, pcell, clist, reflect, rotate) != 0)
                return 1;
            }
            break;//case(ANGLE)
          case(XY):
            if(record_type == ENDEL){last_record=ENDEL;
              stream_state = STRUCTURE_S;
              reflect = 0;
              rotate = 0;
            }
            else {
              sprintf(strbuf, "Record error: state = SREF:XY, got = %d expect ENDEL.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(XY)
          default:
        }//switch(last_record)
        break;//case(SREF_S)
      default:
    } //switch(stream_state)
    //advance cursor to next record.
    //when finished, leave the cursor alligned to the first element of the top_structure
    fseek(in_file, cursor + sizeof(uint8_t) * record_len, SEEK_SET);
  }
  while(last_record != ENDLIB);
  //Check if the referenced struture has both pin labels and LI shapes
  fseek(in_file, start_cursor, SEEK_SET);
  free(strbuf);

  return 0;
}

//ignores tap structures, decaps, boundaries EXCEPT io boxes
int write_routing(FILE*in_file, FILE* out_file, structure_list_t* slist) {
  char* strbuf = malloc(sizeof(char)*256);
  //collection of strings with locations
  pin_lbl_t* iobuf = malloc(sizeof(pin_lbl_t) * 16);
  int num_ios = 0;
  enum State stream_state = STRUCTURE_S;
  enum RType last_record = STRNAME;
  uint16_t record_len;
  uint8_t record_type;
  uint8_t data_type;

  uint16_t layernum;
  uint16_t dtype;

  uint16_t pathtype = 0;
  int32_t width = 0;
  int32_t bx = 0;
  int32_t ex = 0;
  int reflect = 0;
  int rotate = 0;    //1,2,3 = 90,180,270 degrees

  //save state of cursor so can move it back at EOF
  uint64_t start_cursor = ftell(in_file);
  int angle;
  XY_t shift;

  sref_t* pcell;

  fprintf(out_file, "\"shapes\":[\n");
do {
    //process a new record
    //all records start with length and record type
    uint64_t cursor = ftell(in_file);
    //printf("Cursor : %d\n", cursor);
    size_t result = fread(&record_len, sizeof(uint16_t), 1, in_file);
    record_len = ntohs(record_len);
    if (result != 1) {
      fprintf(stderr, "Error reading record length, last_record = %d, cursor = %lu\n", last_record, ftell(in_file));
      return 1;
    }

    result = fread(&record_type, sizeof(uint8_t), 1, in_file);
    if (result != 1) {
      perror("Error reading record type\n");
      return 1;
    }

    result = fread(&data_type, sizeof(uint8_t), 1, in_file);
    if (result != 1) {
      perror("Error reading data type\n");
      return 1;
    }

    //last state machine
    switch(stream_state) {
      case(STREAM_S):
        switch(last_record) {
          case(ENDSTR):
            if(record_type == ENDLIB) {last_record = ENDLIB;}
            else {
              sprintf(strbuf, "Record error: state = STREAM_S:ENDSTR, got = %d expect ENDLIB\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;
            default:
        }
        break;//case(STREAM_S)

      case(STRUCTURE_S):
        switch(last_record) {
          case(ENDEL):   //a sref end el
          case(STRNAME): //entry point
            if(record_type == ENDSTR) {last_record = ENDSTR;
              stream_state = STREAM_S;
            }
            else if (record_type == SREF){last_record = SREF;
              stream_state = SREF_S;
            }
            else if (record_type == BOUNDARY) {last_record = BOUNDARY;
              stream_state = BOUNDARY_S;
            }
            else if (record_type == PATH) {last_record = PATH;
              stream_state = PATH_S;
            }
            else if (record_type == TEXT) {last_record = TEXT;
              stream_state = TEXT_S;
            }
            else {
              //do nothing, records will continue to process until a desired element is seen.
            }
            break;//case(STRNAME) || case(ENDEL)
          default:
        }//switch(last_record)
        break; //case(STRUCTURE_S)

      case(BOUNDARY_S): //perhaps some other structures
        switch(last_record){
          case(BOUNDARY):
            if (record_type == LAYER) {last_record = LAYER;
              fread(&layernum, sizeof(uint16_t), 1, in_file);
              layernum = ntohs(layernum);

            } else {
              sprintf(strbuf, "Record error: state = BOUNDARY:BOUNDARY, got = %d expect LAYER\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break; //case(BOUNDARY)
          case(LAYER):
            if (record_type == DATATYPE) {last_record = DATATYPE;
              fread(&dtype, sizeof(uint16_t), 1, in_file);
              dtype = ntohs(dtype);
            } else {
              sprintf(strbuf, "Record error: state = BOUNDARY:LAYER, got = %d expect DATATYPE\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break; //case(LAYER)

          case(DATATYPE):
            if (record_type == XY) {last_record = XY;
              if (layernum == M1_LAYER_C || layernum == M2_LAYER_C || layernum == M3_LAYER_C ||
                  layernum == M4_LAYER_C || layernum == M5_LAYER_C || layernum == IO_LNUM) {
                //only interested in metal layers now + the io shapes
                //just write whatever it is to the file
                write_poly(in_file, out_file, (record_len-4)/sizeof(XY_t), layernum, dtype);
              }
            }
            else {
              sprintf(strbuf, "Record error: state = BOUNDARY:DATATYPE, got = %d expect XY\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(DATATYPE)
          case(XY):
            if (record_type == ENDEL) {last_record = ENDEL;
              stream_state = STRUCTURE_S;
            }
            else {
              sprintf(strbuf, "Record error: state = BOUNDARY:XY, got = %d expect ENDEL\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(XY)
        }//switch(last_record)

        break;//case(BOUNDARY_S)

      case(PATH_S): //routing tracks
        switch(last_record) {
          case(PATH):
            if (record_type == LAYER) {last_record = LAYER;
              fread(&layernum, sizeof(uint16_t), 1, in_file);
              layernum = ntohs(layernum);

            } else {
              sprintf(strbuf, "Record error: state = PATH:PATH, got = %d expect LAYER\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break; //case(BOUNDARY)
          case(LAYER):
            if (record_type == DATATYPE) {last_record = DATATYPE;
              fread(&dtype, sizeof(uint16_t), 1, in_file);
              dtype = ntohs(dtype);
            } else {
              sprintf(strbuf, "Record error: state = PATH:LAYER, got = %d expect DATATYPE\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break; //case(LAYER)
          case(DATATYPE):
            if(record_type == PATHTYPE) {last_record = PATHTYPE;
              fread(&pathtype, sizeof(uint16_t), 1, in_file);
              pathtype = ntohs(pathtype);
            }
            else if(record_type == WIDTH){last_record = WIDTH;
              fread(&width, sizeof(int32_t), 1, in_file);
              width = ntohl(width);
            }
            else if(record_type == XY){last_record = XY;
              //assume we care about all path objects.
              write_path(in_file, out_file, (record_len - 4)/sizeof(XY_t), layernum, dtype, pathtype, width, bx, ex);
            }
            else {
              sprintf(strbuf, "Record error: state = PATH:DATATYPE, got = %d expect PATHTYPE || WIDTH || XY\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break; //case(DATATYPE)

          case(PATHTYPE):
            if(record_type == WIDTH){last_record = WIDTH;
              fread(&width, sizeof(int32_t), 1, in_file);
              width = ntohl(width);
            }
            else if(record_type == XY){last_record = XY;
              write_path(in_file, out_file, (record_len - 4)/sizeof(XY_t), layernum, dtype, pathtype, width, bx, ex);
            }
            else {
              sprintf(strbuf, "Record error: state = PATH:PATHTYPE, got = %d expect WIDTH || XY\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(PATHTYPE)

          case(WIDTH):
            if(record_type == BX){last_record = BX;
              if (pathtype != 4) {
                fprintf(stderr, "Record error: BX record seen with pathtype != 4.\n");
                return 1;
              }
              fread(&bx, sizeof(int32_t), 1, in_file);
              bx = ntohl(bx);
            } else if (record_type == EX){last_record = EX;
              if (pathtype != 4) {
                fprintf(stderr, "Record error: EX record seen with pathtype != 4.\n");
                return 1;
              }
              fread(&ex, sizeof(int32_t), 1, in_file);
              ex = ntohl(ex);
            } else if(record_type == XY){last_record = XY;
              write_path(in_file, out_file, (record_len - 4)/sizeof(XY_t), layernum, dtype, pathtype, width, bx, ex);
            }
            else {
              sprintf(strbuf, "Record error: state = PATH:WIDTH, got = %d expect BX || EX || XY", record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(WIDTH)
          case(BX):
            if (record_type == EX){last_record = EX;
               if (pathtype != 4) {
                fprintf(stderr, "Record error: EX record seen with pathtype != 4.\n");
                return 1;
              }
              fread(&ex, sizeof(int32_t), 1, in_file);
              ex = ntohl(ex);
            } else if(record_type == XY){last_record = XY;
              write_path(in_file, out_file, (record_len - 4)/sizeof(XY_t), layernum, dtype, pathtype, width, bx, ex);
            }
            else {
              sprintf(strbuf, "Record error: state = PATH:BX, got = %d expect EX || XY", record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(BX)
          case(EX):
            if(record_type == XY){last_record = XY;
              write_path(in_file, out_file, (record_len - 4)/sizeof(XY_t), layernum, dtype, pathtype, width, bx, ex);
            }
            else {
              sprintf(strbuf, "Record error: state = PATH:BX, got = %d expect XY\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(EX)
          case(XY):
            if(record_type == ENDEL) {last_record = ENDEL;
              //reset optional fields to defaults;
              pathtype = 0;
              width = 0;
              bx = 0;
              ex = 0;
              stream_state = STRUCTURE_S;
            }
            break;

        }//switch(last_record)
        break;//case(PATH_S)

      case(SREF_S): // vias
        switch(last_record) {
          case(SREF):
            if(record_type = SNAME) {
              //check if the name matches
              fread(strbuf, sizeof(char), record_len - 4, in_file);
              strbuf[record_len - 4] = '\0';
              //avoid pcells and contact vias which have already been handled.
              if(strncmp(strbuf, "sky130", 6) != 0 && strncmp(strbuf, "VIA_L1M1_PR_MR", 14) != 0){
                last_record = SNAME;
                //Search for the structure
                int found;
                for (int cell =0; cell < slist->num_structs; cell++) {
                  if (strcmp(slist->structures[cell]->strname, strbuf) == 0) {
                    pcell = slist->structures[cell];
                    found = 1;
                    break;
                  }
                }
                if (found == 0) {
                  fprintf(stderr, "Did not find referenced structure \"%s\".\n", strbuf);
                  return 1;
                }
              } else {
                //if no match, jump back to looking for SREF
                stream_state = STRUCTURE_S;
                last_record = ENDEL;
              }
            } else {
              sprintf(strbuf, "Record error: state = SREF:SREF, got = %d expect SNAME.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//cas(SREF)
          case(SNAME):
            if(record_type == STRANS) {last_record = STRANS;
              //check for x axis reflect
              uint8_t strans_rec;
              fread(&strans_rec, sizeof(uint8_t), 1, in_file);
              if ((strans_rec & ((uint8_t)1 << 7)) != 0)
                reflect = 1;
            } else if (record_type == XY){last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_write_shapes(out_file, shift, pcell, reflect, rotate) != 0)
                return 1;
            } else {
              sprintf(strbuf, "Record error: state = SREF:SNAME, got = %d expect STRANS || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(SNAME)
          case(STRANS):
            if(record_type == MAG){last_record = MAG;
              fprintf(stderr, "Error: unimplemented feature, SREF:MAG\n");
              return 1;
            }
            else if (record_type == ANGLE) {last_record = ANGLE;
              //Check if it's 180 flipped.
              uint8_t angle_rec;
              fseek(in_file, 1, SEEK_CUR);
              fread(&angle_rec, sizeof(uint8_t), 1, in_file);
              //check against rotations of 90, 180, and 270 degrees
              switch(angle_rec){
                case(90):
                  rotate = 1;
                break;
                case(180):
                  rotate = 2;
                break;
                default:
                  fprintf(stderr, "Unsupported angle detected SREF:ANGLE\n");
              }//switch(angle_rec)
            }
            else if (record_type == XY) {last_record = XY;
              //same as above
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_write_shapes(out_file, shift, pcell, reflect, rotate) != 0)
                return 1;
            } else {
              sprintf(strbuf, "Record error: state = SREF:STRANS, got = %d expect MAG || ANGLE || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(STRANS)
          case(MAG):
            if (record_type = ANGLE) {last_record = ANGLE;
              //Check if it's 180 flipped.
              uint8_t angle_rec;
              fseek(in_file, 1, SEEK_CUR);
              fread(&angle_rec, sizeof(uint8_t), 1, in_file);
              //check against rotations of 90, 180, and 270 degrees
              switch(angle_rec){
                case(90):
                  rotate = 1;
                break;
                case(180):
                  rotate = 2;
                break;
                default:
                  fprintf(stderr, "Unsupported angle detected SREF:ANGLE\n");
              }//switch(angle_rec)
            }
            else if(record_type == XY) {last_record=XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_write_shapes(out_file, shift, pcell, reflect, rotate) != 0)
                return 1;
            }
            else{
              sprintf(strbuf, "Record error: state = SREF:MAG, got = %d expect ANGLE || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(MAG)
          case(ANGLE):
            if(record_type == XY) {last_record=XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
              if(translate_and_write_shapes(out_file, shift, pcell, reflect, rotate) != 0)
                return 1;
            }
            break;//case(ANGLE)
          case(XY):
            if(record_type == ENDEL){last_record=ENDEL;
              stream_state = STRUCTURE_S;
              reflect = 0;
              rotate = 0;
            }
            else {
              sprintf(strbuf, "Record error: state = SREF:XY, got = %d expect ENDEL.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(XY)
          default:
        }//switch(last_record)
        break;//case(SREF_S)
      case(TEXT_S):
        switch(last_record) {
          case(TEXT):
            if (record_type == LAYER) {last_record = LAYER;
              //don't care
            } else {
              sprintf(strbuf, "Record error: state = TEXT:TEXT, got = %d expect LAYER\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break; //case(TEXT)

          case(LAYER):
            if (record_type == TEXTTYPE) {last_record = TEXTTYPE;
              //Don't care
            }
            else {
              sprintf(strbuf, "Record error: state = TEXT:LAYER, got = %d expect TEXTTYPE\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(LAYER)

          case(TEXTTYPE):
            if (record_type == PRESENTATION)  {last_record = PRESENTATION;} //no action
            else if (record_type == PATHTYPE) {last_record = PATHTYPE;} //no action
            else if (record_type == WIDTH)    {last_record = WIDTH;} //no action
            else if (record_type == STRANS)   {last_record = STRANS;}//no action required for text
            else if (record_type == XY)       {last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
            }
            else {
              sprintf(strbuf, "Record error: state = TEXT:TEXTTYPE, got = %d expect PRESENTATION || PATHTYPE || WIDTH || STRANS || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(TEXTTYPE)

          case(PRESENTATION):
            if (record_type == PATHTYPE)    {last_record = PATHTYPE;} //no action
            else if (record_type == WIDTH)  {last_record = WIDTH;} //no action
            else if (record_type == STRANS) {last_record = STRANS;} //no action required for text
            else if (record_type == XY)     {last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
            } else {
              sprintf(strbuf, "Record error: state = TEXT:PRESENTATION, got = %d expect PATHTYPE || WIDTH || STRANS || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(PRESENTATION)

          case(PATHTYPE):
            if (record_type == WIDTH)         {last_record = WIDTH;} //no action
            else if (record_type == STRANS)   {last_record = STRANS;}//no action required for text.
            else if (record_type == XY)       {last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
            } else {
              sprintf(strbuf, "Record error: state = TEXT:PATHTYPE, got = %d expect WIDTH || STRANS || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(PATHTYPE)

          case(WIDTH):
            if (record_type == STRANS)   {last_record = STRANS;}//no action required for text.
            else if (record_type == XY)  {last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
            } else {
              sprintf(strbuf, "Record error: state = TEXT:WIDTH, got = %d expect WIDTH || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(WIDTH)

          case(STRANS):
            if (record_type == MAG) {last_record = MAG;}
            else if (record_type == ANGLE) {last_record = ANGLE;}
            else if (record_type == XY) {last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
            } else {
              sprintf(strbuf, "Record error: state = TEXT:STRANS, got = %d expect MAG || ANGLE || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(STRANS)

          case(MAG):
            if (record_type == ANGLE) {last_record = ANGLE;}
            else if (record_type == XY) {last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
            } else {
              sprintf(strbuf, "Record error: state = TEXT:MAG, got = %d expect ANGLE || XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(MAG)

          case(ANGLE):
            if (record_type == XY) {last_record = XY;
              fread(&shift, sizeof(XY_t), 1, in_file);
              shift.x = ntohl(shift.x);
              shift.y = ntohl(shift.y);
            } else {
              sprintf(strbuf, "Record error: state = TEXT:MAG, got = %d expect XY.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(ANGLE)

          case(XY):
            if (record_type == STRING) {last_record = STRING;
              //get the string
              fread(iobuf[num_ios].pinname, sizeof(char), record_len - 4, in_file);
              //terminate
              iobuf[num_ios].pinname[record_len - 4] = '\0';
              iobuf[num_ios].xy = shift;
              num_ios++;
            } else {
              sprintf(strbuf, "Record error: state = TEXT:XY, got = %d expect STRING.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;//case(XY)

          case(STRING):
            if (record_type == ENDEL) {last_record = ENDEL;
              stream_state = STRUCTURE_S;
            } else {
              sprintf(strbuf, "Record error: state = TEXT:STRING, got = %d expect ENDEL.\n", (char) record_type);
              perror(strbuf);
              return 1;
            }
            break;
            default:
          }
      break;//case(TEXT_S)
      default:
    }//switch(stream_state)


    //advance cursor to next record.
    //when finished, leave the cursor alligned to the first element of the top_structure
    fseek(in_file, cursor + sizeof(uint8_t) * record_len, SEEK_SET);
  }
  while(last_record != ENDLIB);
  fprintf(out_file, "{}],\n");

  //print ios
  fprintf(out_file, "\"io\": {\n");
  for (int io = 0; io < num_ios; io++) {
    if(io) fprintf(out_file, ",\n");
    fprintf(out_file, "\"%s\" : [%d, %d]", iobuf[io].pinname, iobuf[io].xy.x, iobuf[io].xy.y);
  }
  fprintf(out_file, "}\n");

  return 0;
}
