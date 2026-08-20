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

#ifndef TOP
#define TOP
  #include <stdlib.h>
  #include <string.h>
  #include <stdio.h>
  #include <stdint.h>
  #include <math.h>
  #include <winsock2.h>
  #include <winsock.h>
  #include "gds_utils.h"
#endif

int main(int argc, char* argv[]) {
// Parsing
  if (argc != 4) {
    perror("gds2nets: usage - <infile> <outfile> <top_level_strname>\n");
  }
  //Initialize

  FILE* in_file = fopen(argv[1], "rb");

  if (in_file == NULL) {
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
  contact_list_t* clist = malloc(sizeof(contact_list_t));
  clist->num_contacts = 0;

//-------------------------------------------------------------------
// Main loop
  int error = build_structures(in_file, out_file, slist, polylist, argv[3]);
  if (error == 0) {
    //next stage
    printf("Got through to top struct.\n");
    for (int s = 0 ; s<slist->num_structs; s++) {
      sref_t* pcell = slist->structures[s];
      printf("%s\n ", pcell->strname);
      for (int pin = 0 ; pin < pcell->n_lbls; pin++) {
        //fprintf(out_file, " %s: (%d, %d), ", pcell->pin_lbls[pin]->pinname, pcell->pin_lbls[pin]->x[0], pcell->pin_lbls[pin]->y[0]);
        printf(" %s, ", pcell->pin_lbls[pin]->pinname);
      }
      printf("\n");
    }
  }

  //assume less than 8192 pins.
  int via_struct=0;
  int found = 0;
  for(; via_struct < slist->num_structs; via_struct++){
    if (strcmp("VIA_L1M1_PR_MR", slist->structures[via_struct]->strname) == 0) {
      found = 1;
      break;
    }
  }
  if (found==0) {
    error = 1;
    fprintf(stderr, "Did not find the via structure.\n");
  }

  if(error == 0) {
    clist->contacts = malloc(sizeof(contact_t) * 8192);
    error = build_contact_list(in_file, slist->structures[via_struct], clist);
    if (error != 0)
      printf("Built contact list, size %d.\n", clist->num_contacts);
  }

  if(error == 0) {
    error = label_contacts(in_file, out_file, clist, slist);
  }
  if(error == 0) {
    int no_label = 0;
    for (int contact = 0; contact < clist->num_contacts; contact++) {
      if (strcmp(clist->contacts[contact].pinname, "\0") == 0) {
          printf("Didn't label %d contacts.\n", no_label);
          printf("One of the contacts:\n");
          printf("x(%d, %d), y(%d, %d)\n", clist->contacts[contact].pin.x1, clist->contacts[contact].pin.x2,
                  clist->contacts[contact].pin.y1, clist->contacts[contact].pin.y2);
        no_label++;
      }
    }

    if(no_label == 0)
      printf("All contacts labelled.\n");
    else {
      error = 1;
      printf("%d unlabelled contacts.\n", no_label);
      for (int s = 0; s < slist->num_structs; s++) {
        if (strcmp(slist->structures[s]->strname, "sky130_fd_sc_hd__o21a_2") == 0) {
          pin_lbl_t* p = slist->structures[s]->pin_lbls[2];
          printf("%s: ", p->pinname);
          for (int i = 0 ;i < p->LI_poly.num_points; i++) {
            printf("(%d, %d)->", p->LI_poly.coords[i].x, p->LI_poly.coords[i].y);
          }
        }
      }
    }
  }

  if (error == 0) {
    //print contact list.
    fprintf(out_file, "{\n");
    write_contacts(out_file, clist);

    write_routing(in_file, out_file, slist);

    //terminate object
    fprintf(out_file, "}\n");
  }
  printf("JSON file write finished.\n");


  //free
  if(clist->contacts != NULL) free(clist->contacts);

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

  fclose(in_file);
  fclose(out_file);

  return 0;
}

