# Goals
I would say that from the start I could see about 80% of the final workload, with the last 20% being in the "we'll get there when we get there" category. Within these, the major milestones would be
* Planned
  1. Parse the gds file and bring the shapes used for routing (metals) and the logical inputs and outputs into some standard format.
  2. Group these objects into nets of all features that are in electrical contact i.e. their geometric features touch.
  3. Aggregate groups of nets associated with individual gates, and write these logical cells in a HDL format.
* We'll work it out later
  1. Work out how the design wants to be simulated and it's 
  2. Determine a good enough way of extracting meaning and purpose from a netlist, and what it takes to drive the success output high.
---
# Exploring the layout
I started out by just looking around the layout in the [TinyTapeout viewer](https://gds-viewer.tinytapeout.com/). At this point I was under the grim impression that I'd be analysing the circuit at the transistor level, and so I was refamiliarising myself with CMOS layouts. Following the clock input along the routing tracks, it ends up at a buffer in the center of the chip.

![clkbuf]()

In this image we can see some of the distinguishing layers of this semiconductor process. The red layer in the middle is typical polysilicon, used for the gate material of the mosfets. These sit above the active/high doped/diff silicon in the actual wafer, making up source and drain material. On top is a metal layer that isn't called a metal layer, the "local interconnect," which does the last mile routing from the metal layers above to the polysilicon contacts below. The local interconnect turns out to be the most important and fiddly part of the netlist extraction.

The clock buffer is pretty simple, its a medium sized inverted driving a big inverter. But assuming I would be doing lots of transistor level analysis, I moved onto the flip flop, and it only took about an hour and a half of looking at this...

![dff_layout]()

to come up with this... which was a good exercise but not a feasible way to approach the whole circuit.

![dff_schem]()

With some more clicking, I noticed that nearby objects are grouped and highlight on click, with an annotation appearing to reference a Skywater 130nm pcell. I was aware that digital design is not generally attempted at the transistor level, instead a library of pcells is used, containing cut and paste layouts for every gate you could ask for, which are placed in rows by one algorithm and their inputs routed together by another algorithm. These annotations indicate that some trace of this methodology is still present in the raw shapes at the bottom-most level of abstraction, the gds file.

![pcell_spotted]()

Hexdumping the file and searching for "sky" confirmed this suspicion
```
00000D30   07 EA 00 08 00 05 00 0E 00 0A 00 24 00 1E 06 06  .ê.........$....
00000D40   73 6B 79 31 33 30 5F 66 64 5F 73 63 5F 68 64 5F  sky130_fd_sc_hd_
00000D50   5F 6E 61 6E 64 33 62 5F 32 00 00 04 09 00 00 06  _nand3b_2.......
00000D60   0D 02 00 44 00 06 0E 02 00 14 00 06 21 02 00 00  ...D........!...
00000D70   00 08 0F 03 00 00 01 E0 00 14 10 03 00 00 00 00  .......à........
```

Looking up the gds2 format, you can see that as a compression method, it includes the ability to define structures at the top of the file that includes all their shapes, then later you just reference the name and provide a translation/reflection/transformation to create instances of it in other structures. In this file, all the pcells used in the design are present as structures. This is great news as it already associates all the transistors in a gate.

It turns out to be even better than that. Looking at the layout in KLayout this time we can see quite a bit more information.

![kl top]()

This includes some more layer, but also much more text. Looking at one of the structures, there seems to be two very important layers. 67/20 contains the local interconnect shapes and turns out to include every one of them, i.e. there is not routing on the LI layer that is defined in the top structure, it's all done in the pcells. Additionally there's a layer of text on 67/5 which provide the pin labels of the gate, which, critically, intersect with the LI shape of that pin. An additional layer (small squares) is a non-physical "pin" layer 67/15. 

These layers provide the opportunity to skip the transistors altogether, as we have the name of the gate, and the location of all it's pins. While parsing the pcell structures in the gds file, there are a number of simplifying assumptions that could be made to reduce how much data needs to be stored regarding the cell so it can be instantiated in the top structure. Try to guess which of these are true assumptions.

1. No extra LI shapes are added which join disjoint LI shapes in the pcell. -> The only LI shapes to store are those that intersect with a label
2. Each LI shape in the pcell intersects with no other LI shapes in the pcell. -> Exactly 1 LI shape intersects with each label
3. No vias from LI to metal 1 exist in the pcell, instead they are all added in the top structure. -> no L1M1 vias need to be stored.
4. Metal 1 is not used for routing signals within the pcell. -> No M1 shapes need to be stored.
5. All LI->M1 vias intersect with a square in the pin layer. -> Only the pin squares need to be stored instead of LI shapes, desirable as its faster to determine intersection with a square.
6. All pin labels sit in the middle of a pin layer square. -> Only have to find a pin that 
7. The gds object type of all LI shapes are BOUNDARY i.e. polygons, as opposed to RECT or PATH.

The weakest of these assumptions are the pin related ones, especially regarding routing as they have little to do with via placement. The assumptions regarding LI shapes are almost always true. Expect for one or two cases, each blob of LI is made of only 1 LI shape and since the exceptions are defined in structures it was easy to edit the file in KLayout to validate this assumption. The are two cells that have disjoint blobs of LI used for the same pin, with vias to metal 1 and a metal 1 routing shape to join them, these being the dfrtp_2 and xor2_2. This is annoying as now the top level structure can connect to this pin using an M2M1 via to the metal object.

![pcell li and text]()

# Extracting useful objects from the .gds 
I decided to do the gds parsing in C, for speed and because I have the most familiarity with file operations in C.

`gds_utils.h` defines some structures with shape information associated together, in line with what I need to build a netlist.
The general flow is to run several passes through the file using similar state machines that grab the required structures each pass. Some util functions check for intersection of different kinds of objects, and others use intersection as a criteria for LI shapes with pin labels and vias with pins etc. The output of the program is a .json file with all the necessary pin location and routing data to build a netlist.

On the first pass, process the records of the file until seeing a structure name 'puzzle' which is the top level structure name. Until that point, process the previous structure definitions, corresponding to pcells, vias, and also diode, taps and decoupling capacitors. After seeing a new structure definition, an sref_t struct is populated with the structure name, then as . A temporary list of all the LI shapes in the structure is made, then once hitting the end of the structure, each of the pin labels is checked for intersection with all the LI shapes, and when the intersecting shape is found, it is copied into the pin_lbl_t object. sref_t.extra contains the few cases of metal1 routing shapes that touch the named pins in the cell.
```C
typedef struct {
  char       strname[64];    //structure name
  pin_t*     pins[128];      //room for 64 pin objects.
  pin_lbl_t* pin_lbls[64];   //list of unique pin labels in the structure.
  poly_t*    extra[16];      //shapes in the pcell that need to go in the shape list.
  int        n_pins;         //how many of the pins array are allocated
  int        n_lbls;         //how many unique labels are allocated.
  int        n_extra;        //how many extra shapes
  int        next_uid;       //integer uid to attach to next pcell instance.
} sref_t;
```
---
### An aside on shape intersection

There are three classes of geometric object for which intersection needs to be defined. These are points, which I call pins, rectangles, which are used for via and path structures, and polygons which I refer to as shapes. Polygon shapes in gds are defined as lists of XY coordinates, where each two adjacent points forms an edge, and the last coord equals the first. Additionally, in most layouts, shapes consist of right angles only.

The most interesting case is that of a shape intersecting with a point. What is needed is some way of defining whether, by walking around the edge of the shape, if the path encircles the point and that the interior of this encircling consists of the inside of the shape.

The first thing that is needed is to know whether the interior of the shape is on the left or the right of the directed path around the perimeter, taken in the order the points are listed in the file. This is simple, find the left most vertical edge, if it is upward, the interior is on the right, otherwise on the left.

Now define left span and right span. These contain the smallest distance from the point to an edge in all four directions, where the edges considered are those passing the shape on the left or right, for left and right span, respectively.

In the case of a left interior shape, the point is inside if all four elements of the left span is strictly smaller than the right span.
Examples of the lspan and rspan are depicted below
![lspan]()

![rspan]()

---

Upon reaching the top level structure 'puzzle' All the pins of the structure. Note that one has VPWR and VGND, which are not desired. These nets are always present in the structure but only occasionally are they on the same layer as the other pins.
```
sky130_fd_sc_hd__or4bb_2 n_lbls 5
A X B D_N C_N 
sky130_fd_sc_hd__or4_2 n_lbls 5
C A X B D 
sky130_fd_sc_hd__conb_1 n_lbls 2
LO HI 
sky130_fd_sc_hd__buf_2 n_lbls 4
VPWR VGND X A 
sky130_fd_sc_hd__and4_2 n_lbls 5
X C A B D 
sky130_fd_sc_hd__and3_2 n_lbls 4
B X A C 
sky130_fd_sc_hd__and2b_2 n_lbls 3
X B A_N 
sky130_fd_sc_hd__nor4_2 n_lbls 5
C D Y A B 
sky130_fd_sc_hd__nand4_2 n_lbls 5
B A Y D C
...
```

The next stage is to find all the L1M1 vias in the puzzle structure, which are the main way of attaching metal layers to the pins of the cells. A state machine looks through the puzzle looking for structure references to 'VIA_L1M1_PR_MR' and places the via rectangle into an array.

Once all the contacts are collected, a second sweep of puzzle looks for structure references that begin with 'sky130' which are the pcells the contacts are trying to connect to. Each of the LI shapes in each instantiated structure is checked for intersection with every contact in the list. Sometimes contacts half overlap with the shape, so all four corners of the via are checked for intersection, as opposed to just the center. When intersection happens, a unique identifier is given to the contact of the from "<pcell>_<inst>/<pinname>" where inst is the number of this kind of pcell created so far. Additionally, on each instantiated structure, the extra shapes are written to the output.

After checking that all contacts have been labelled, they are written as a block to the output file.

At this point the number of each kind of pcell is known
```
sky130_fd_sc_hd__a31o_2: 26 insts
  
sky130_fd_sc_hd__dfrtp_2: 84 insts
  
sky130_fd_sc_hd__or4bb_2: 1 insts
  
sky130_fd_sc_hd__or4_2: 10 insts
  
sky130_fd_sc_hd__conb_1: 6 insts
```
84 flip flops with reset. Seems like a manageable design to analyse manually.

The final pass through the file looks for routing shapes and instantiations of different kinds of via. After writing these shapes, the inputs and outputs of the file are written.

# Recovering the netlist
So far we've done some aggregation of geometric features into simple structures, which was feasible in C. The next step is to aggregate thousands of these objects with different geometric classes like pin, rectangle, shape into ‘nets’ of all shapes that are in electrical contact and it’s at this point that we let Python handle our memory.

Here's what some of the JSON output by the first program looks like. The contact list contains all the driver and load pins.
```json
"clist" : [
  {"type":"pin", "driver":0, "pinname":"sky130_fd_sc_hd__o21a_2_1/A2", "loc":[121669,117809]},
  {"type":"pin", "driver":1, "pinname":"sky130_fd_sc_hd__or4_2_1/X", "loc":[116609,203149]},
  {"type":"pin", "driver":1, "pinname":"sky130_fd_sc_hd__a22o_2_1/X", "loc":[167209,203149]},
  {"type":"pin", "driver":0, "pinname":"sky130_fd_sc_hd__a32o_2_1/B1", "loc":[174669,278629]},
```

Shapes contains the routing structures.
```json
"shapes":[
{"type":"rect", "layer":71, "dtype":44, "x":[56040,56840], "y":[117130,117930]},
{"type":"rect", "layer":71, "dtype":44, "x":[86040,86840], "y":[117130,117930]},
{"type":"rect", "layer":68, "dtype":44, "x":[100435,100585], "y":[120115,120265]},
{"type":"rect", "layer":71, "dtype":44, "x":[26040,26840], "y":[117130,117930]},
{"type":"rect", "layer":68, "dtype":44, "x":[100435,100585], "y":[20155,20305]},
{"type":"rect", "layer":71, "dtype":44, "x":[176040,176840], "y":[117130,117930]},
```

IO contains extra driver and loads that don't come from pcells.
```json
"io": {
"I" : [300, 79220],
"O[0]" : [199700, 204340],
"O[1]" : [199700, 177140],
"O[2]" : [199700, 149940],
"O[3]" : [199700, 122740],
"O[4]" : [199700, 95540],
"O[5]" : [199700, 68340],
"O[6]" : [199700, 41140],
"O[7]" : [199700, 13940],
"clk" : [300, 238340],
"enable" : [300, 132260],
"rst_n" : [300, 185300],
"success" : [199700, 285940],
"VGND" : [30140, 149600],
"VPWR" : [26440, 149600],
"VGND" : [100050, 31230],
"VPWR" : [100050, 27530]}
```

`process_nets.py` is the script that processes this json to get the desired netlist. It begins by creating a container for each net in the design. Since this circuit contains no cells with tristate outputs i.e. output pins that can be turned off, every net must contain exactly one driver, hence we can just search for unique driver pin names, as well as inputs ("I", "clk", "rst_b", "enable"), and create one net for each. In this step care is needed to associate contacts with the same name together (caused by multiple vias intersecting with LI objects connected to the same pin) as metal routing can potential touch one of the contacts and jump to the other ones.

Once all the nets are created, a list of all other "net segments" that need to be put in those nets is created, including load pins, routing shapes and outputs of the circuit.

An object oriented approach is used to provide a simple interface for determining if one net segments touches another, even as some of the segments are single points, others are rectangles and others are polygons. The abstract class NetSeg defines the properties and functions it expects the.
```python
class NetSeg:
  def __init__(self, layer = None, ytop = None, xleft = None):
    self.layer = layer
    self.ytop = ytop
    self.xleft = xleft
    self.map_touches = {
      Rect : self._touch_rect,
      Pin  : self._touch_pin,
      Shape : self._touch_shape,
    }

  def _touch_rect(self, net):
    pass
  def _touch_pin(self, net):
    pass
  def _touch_shape(self,net):
    pass
  def touches(self, net):
    return self.map_touches[type(net)](net)
```

This way any object extending NetSeg can call self.touches(net) to determine intersection with any of the three classes extending NetSeg - Pin, Rect and Shape.

`ytop` and `xleft` are used to define an ordering of any net segment object. This is used to sort the jumbled list of segments, in the hopes this will speed up the next stage of the script.

The nets themselves are a separate class, including a name, a list of segments attached to the net and a list of those segements which are load pins/outputs. Additionally a bounding box enclosing all elements in the net allows a fast way of ruling out if a segment touches any other segment in the net. 

## Growing the nets
The sorted list is traversed first forward, then backward and so on until all net segments are assigned. Each unallocated net segment is checked for intersection with the bounding box of every unfinished net and if it is in bounds, it is then checked for intersection with any of the segments already in the net. A net is finished if it hasn't gotten any bigger in the most recent sweep through the net segments.
```json
{"obj": {"sky130_fd_sc_hd__inv_2_2/Y": ["sky130_fd_sc_hd__a32o_2_2/A1"], "sky130_fd_sc_hd__inv_2_4/Y": ["sky130_fd_sc_hd__a21boi_2_2/A1"],...
```

# Generating a HDL description
The goal of the next stage was to generate a HDL description of the circuit encoded by the netlist. The most straightforward way to approach this is to:

1. Declare every signal in the netlist. Due to our handling of multipins from before, every pin object in the netlist is unique, and so we simply iterate through the nets, declaring all loads and drivers.
2. Writing statements of the form `load <= driver` which encodes the electrical contact relationships implied by pins being in the same net.
3. Logical statements for example . For this we simply iterate through the list of drivers, extract the section of the name describing the gate e.g. a32o and the instance number in this function, relying on the [skywater pdk docs for their digital standard cells][sky130 cells]

We approach this with the same overloading trick from earlier. Note that "conb" is the only cell in the design with two outputs, a constant logic 1 or 0, and it may be only one output that is present in the design, so which output is desired needs to be checked to avoid driving an undeclared signal.
```python
def write_pcell(fd, fullname):
  [cellname,pin] = fullname.split("/")
  l = re.sub(r'^sky130_fd_sc_hd__', '', cellname).rsplit("_", 1)
  if l[0] == "conb_1":
    string = conb_1(l[1], pin)
  else:
    string = map_write[l[0]](l[1])
  if (string):
    fd.write(string + '\n')
```
The individual functions have this format
```python
def buf_2(inst):
  return f"buf_2_{inst}_X <= buf_2_{inst}_A;"
def and4_2(inst):
  return f"and4_2_{inst}_X <= and4_2_{inst}_A and and4_2_{inst}_B and and4_2_{inst}_C and and4_2_{inst}_D;"
```
And the `map_write` object takes the prefix of the pinname corresponding to the gate and returns the appropriate function
```python
map_write = {
  "diode_2" : diode_2,
  "decap_3" : decap_3,
  "tapvpwrvgnd_1" : tapvpwrvgnd_1,
  "inv_2" : inv_2,
  "mux2_1":mux2_1,
  "A22o_2":a22o_2, ...
```

This generates expressions like this for logic:
```vhdl
a31o_2_15_X <= (a31o_2_15_A1 and a31o_2_15_A2 and a31o_2_15_A3) or a31o_2_15_B1;
```
And like this for flip flops with asynchronous reset
```vhdl
process(dfrtp_2_53_RESET_B, dfrtp_2_53_CLK) begin
  if dfrtp_2_53_RESET_B = '0' then
    dfrtp_2_53_Q <= '0';
  elsif rising_edge(dfrtp_2_53_CLK) then
    dfrtp_2_53_Q <= dfrtp_2_53_D;
  end if;
end process;
```
 * *We do not need to check that the input pins exist, because this was already checked when making structure references back in the C program.*

A special case when declaring net names is that of clock nets. Every input and output of the clock buffers are logically identical and should be considered to have edges simultaneously. Were we to create a bunch of clock nets named `clkbuf_4_<x>_A`, `clkbuf_16_<x>_X` and so on and assign them all to eachother, the behaviour of any simulator I have seen is divergent from the physical expectation. Instead of all edges occurring at once, when a rising edge in the `clk` input occurs, all edge triggered processes depending on `clk` alone will update simultaneously, meaning, using values of their inputs from before the clock edge, then other clock signals will process their assignments `clkbuf_X <= clk` causing `clkbuf_X` to trigger it’s own set of edge sensitive processes, but whose inputs take values from after the processes sensitive to `clk` have resolved.
Instead of using signal assignments, using the alias keyword has an effect similar to a macro substitution, treating the clock pins as copies of the `clk` signal, and causing all flip flops to update together.

Another helpful option is to add “keep” attributes to all the flip flop outputs in the design, which ensures that the Vivado synthesis tool preserves that net and it’s driver (the flip flop) and doesn’t perform optimizations for timing that move flip flops around although there is a use for these optimizations later.

```vhdl
attribute keep of dfrtp_2_46_Q: signal is "true";
alias dfrtp_2_46_CLK: std_logic is clk;
```

# The RTL simulation framework
I wrote a basic test bench around the puzzle module, strobing reset then setting enable and for the moment just holding 'I' low. Surprisingly, this doesn't work, but there is a little activity in the 'O' signal after a while.

![Output]()

Zooming in we see a burst of a few very conspicuous numbers. Switching the radix of the 'O' signal to ascii shows a cute message, an empty sky with no stars.

![Output data]()

![Empty sky]()

# Exploring synthesis results

I had been hoping to use the schematic visualiser in Vivado to assist in reverse engineering the point of the circuit. However, while there are only 90 odd flip flops in the design there are several hundred other combinational pcells used, so instead of trying to see the original circuit, by adding "keep"s to all the output pin nets, I just synthesised the circuit and let it lump as much logic as it could into LUT6's, arbitrary 6 input function generators available on FPGAs.

The next stage is a "baba is you" level brain bender, demanding a lot from your working memory, pattern recognition and intuition. Thankfully there's a lot of a) reuse of a few flip flops and b) repetitive structures with minor tweaks. From a cursory look around there's a few standout structures.
1. a delay chain driven by the input

# Solving for x
To summarize the requirements of getting success high.
1. I must be asserted for exactly 22 of 121 cycles following reset.
2. I must be asserted for exactly 2 cycles between each rollover of a mod 11 counter.
3. I must be asserted for exactly 2 cycles coinciding with the mod 11 counter taking each of its values.
4. I must be asserted for exactly 2 cycles taken from each of 11 different subsets of cycles from 0 to 120
5. I must not be asserted 11 cycles after I was previously asserted.
6. I must not be asserted 1, 10 or 12 cycles after I was previously asserted UNLESS the current value of the mod 10 counter is 0.

In other words we have a number of constraints of the form (sum of cycles where I is high on a certain subset of cycles from 0 to 120 is exactly 2) and some other constraints of the form (sum of cycles where I is high during a subset of two cycles separated 1, 10, 11, or 12 apart is strictly less than 2).

Let the variables x_0 through x_120 take the values 0 or 1 depending on if I is high on a given cycle. These constraints then become linear equalities and inequalities of the variables. A linear programming solver with the cost function set arbitrarily to 0 can be used to find a solution to such a system.

Scipy offers a solver with an integrality constraint which is just what is needed. `find_sol.py` builds the required matrices and spits out the cycles on which to assert I.
```
[[  7]
 [  9]
 [ 11]
 [ 16]
 [ 29]
 [ 31]
 [ 33]
 [ 35]
 [ 48]
 [ 50]
 [ 57]
 [ 63]
 [ 70]
 [ 76]
 [ 78]
 [ 83]
 [ 91]
 [ 98]
 [104]
 [107]
 [111]
 [113]]
```

Plugging in this answer as SEQUENCE_C and running the sim, with only mild surprise we find it actually works.
```vhdl
constant SEQ_C : std_logic_vector(120 downto 0) := (7|9|11|16|29|31|33|35|48|50|57|63|70|76|78|83|91|98|104|107|111|113 => '1', others => '0');
...
for loopcnt in 0 to 120 loop
  I <=  SEQ_C(loopcnt);
  wait until rising_edge(clk);
end loop;
...
```



[sky130 cells]: https://sky130-unofficial.readthedocs.io/en/latest/contents/libraries/sky130_fd_sc_hd/README.html 
