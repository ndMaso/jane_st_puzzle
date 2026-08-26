
Header = """library ieee;
use ieee.std_logic_1164.all;

entity puzzle is
  port(
    clk     : in  std_logic;
    rst_n   : in  std_logic;
    enable  : in  std_logic;
    I       : in  std_logic;

    success : out std_logic;
    O       : out std_logic_vector(7 downto 0)
  );

end puzzle;

architecture rtl of puzzle is
  attribute keep : string;

"""

Begin = """
begin
"""


Footer = """
end rtl;
"""

import json
import re

def write_pcell(fd, fullname):
  [cellname,pin] = fullname.split("/")
  l = re.sub(r'^sky130_fd_sc_hd__', '', cellname).rsplit("_", 1)
  if l[0] == "conb_1":
    string = conb_1(l[1], pin)
  else:
    string = map_write[l[0]](l[1])
  if (string):
    fd.write(string + '\n')

def diode_2(inst):
  return ""

def decap_3(inst):
  return ""

def tapvpwrvgnd_1(inst):
  return ""

def clkbuf_16(inst):
  return f"clkbuf_16_{inst}_X <= clkbuf_16_{inst}_A;"

def clkbuf_8(inst):
  return f"clkbuf_8_{inst}_X <= clkbuf_8_{inst}_A;"

def clkbuf_4(inst):
  return f"clkbuf_4_{inst}_X <= clkbuf_4{inst}_A;"

def inv_2(inst):
  return f"inv_2_{inst}_Y <= not inv_2_{inst}_A;"

def mux2_1(inst):
  return f"mux2_1_{inst}_X <= mux2_1_{inst}_A1 when mux2_1_{inst}_S = '1' else mux2_1_{inst}_A0;"

def a22o_2(inst):
  return f"a22o_2_{inst}_X <= (a22o_2_{inst}_A1 and a22o_2_{inst}_A2) or (a22o_2_{inst}_B1 and a22o_2_{inst}_B2);"

def a221o_2(inst):
  return f"a221o_2_{inst}_X <= (a221o_2_{inst}_A1 and a221o_2_{inst}_A2) or (a221o_2_{inst}_B1 and a221o_2_{inst}_B2) or a221o_2_{inst}_C1;"

def a31o_2(inst):
  return f"a31o_2_{inst}_X <= (a31o_2_{inst}_A1 and a31o_2_{inst}_A2 and a31o_2_{inst}_A3) or a31o_2_{inst}_B1;"

def dfrtp_2(inst):
  return f"""
process(dfrtp_2_{inst}_RESET_B, dfrtp_2_{inst}_CLK) begin
  if dfrtp_2_{inst}_RESET_B = '0' then
    dfrtp_2_{inst}_Q <= '0';
  elsif rising_edge(dfrtp_2_{inst}_CLK) then
    dfrtp_2_{inst}_Q <= dfrtp_2_{inst}_D;
  end if;
end process;
"""

def or4bb_2(inst):
  return f"or4bb_2_{inst}_X <= or4bb_2_{inst}_A or or4bb_2_{inst}_B or not or4bb_2_{inst}_C_N or not or4bb_2_{inst}_D_N;"

def or4_2(inst):
  return f"or4_2_{inst}_X <= or4_2_{inst}_A or or4_2_{inst}_B or or4_2_{inst}_C or or4_2_{inst}_D;"

def conb_1(inst, pin):
  if pin == "LO" : return f"conb_1_{inst}_LO <= '0';"
  else: return f"conb_1_{inst}_HI <= '1';"


def buf_2(inst):
  return f"buf_2_{inst}_X <= buf_2_{inst}_A;"

def and4_2(inst):
  return f"and4_2_{inst}_X <= and4_2_{inst}_A and and4_2_{inst}_B and and4_2_{inst}_C and and4_2_{inst}_D;"

def and3_2(inst):
  return f"and3_2_{inst}_X <= and3_2_{inst}_A and and3_2_{inst}_B and and3_2_{inst}_C;"

def and2b_2(inst):
  return f"and2b_2_{inst}_X <= and2b_2_{inst}_B and not and2b_2_{inst}_A_N;"

def nor4_2(inst):
  return f"nor4_2_{inst}_Y <= not (nor4_2_{inst}_A or nor4_2_{inst}_B or nor4_2_{inst}_C or nor4_2_{inst}_D);"

def nand4_2(inst):
  return f"nand4_2_{inst}_Y <= not (nand4_2_{inst}_A and nand4_2_{inst}_B and nand4_2_{inst}_C and nand4_2_{inst}_D);"

def o21a_2(inst):
  return f"o21a_2_{inst}_X <= o21a_2_{inst}_B1 and (o21a_2_{inst}_A1 or o21a_2_{inst}_A2);"

def nand2b_2(inst):
  return f"nand2b_2_{inst}_Y <= nand2b_2_{inst}_A_N or not nand2b_2_{inst}_B;"

def and2_2(inst):
  return f"and2_2_{inst}_X <= and2_2_{inst}_A and and2_2_{inst}_B;"

def or4b_2(inst):
  return f"or4b_2_{inst}_X <= or4b_2_{inst}_A or or4b_2_{inst}_B or or4b_2_{inst}_C or not or4b_2_{inst}_D_N;"

def nand2_2(inst):
  return f"nand2_2_{inst}_Y <= not (nand2_2_{inst}_A and nand2_2_{inst}_B);"

def nor2_2(inst):
  return f"nor2_2_{inst}_Y <= not (nor2_2_{inst}_A or nor2_2_{inst}_B);"

def a21o_2(inst):
  return f"a21o_2_{inst}_X <= a21o_2_{inst}_B1 or (a21o_2_{inst}_A1 and a21o_2_{inst}_A2);"

def and4bb_2(inst):
  return f"and4bb_2_{inst}_X <= and4bb_2_{inst}_D and and4bb_2_{inst}_C and not (and4bb_2_{inst}_A_N or and4bb_2_{inst}_B_N);"

def and4b_2(inst):
  return f"and4b_2_{inst}_X <= and4b_2_{inst}_D and and4b_2_{inst}_C and and4b_2_{inst}_B and not and4b_2_{inst}_A_N;"

def xnor2_2(inst):
  return f"xnor2_2_{inst}_Y <= not (xnor2_2_{inst}_A xor xnor2_2_{inst}_B);"

def a21oi_2(inst):
  return f"a21oi_2_{inst}_Y <= not (a21oi_2_{inst}_B1 or (a21oi_2_{inst}_A1 and a21oi_2_{inst}_A2));"

def a211oi_2(inst):
  return f"a211oi_2_{inst}_Y <= not (a211oi_2_{inst}_C1 or a211oi_2_{inst}_B1 or (a211oi_2_{inst}_A1 and a211oi_2_{inst}_A2));"

def xor2_2(inst):
  return f"xor2_2_{inst}_X <= xor2_2_{inst}_A xor xor2_2_{inst}_B;"

def a41oi_2(inst):
  return f"a41oi_2_{inst}_Y <= not (a41oi_2_{inst}_B1 or (a41oi_2_{inst}_A1 and a41oi_2_{inst}_A2 and a41oi_2_{inst}_A3 and a41oi_2_{inst}_A4));"

def a221oi_2(inst):
  return f"a221oi_2_{inst}_Y <= not ((a221oi_2_{inst}_A1 and a221oi_2_{inst}_A2) or (a221oi_2_{inst}_C1 or (a221oi_2_{inst}_B1 and a221oi_2_{inst}_B2)));"

def nor3b_2(inst):
  return f"nor3b_2_{inst}_Y <= nor3b_2_{inst}_C_N and not (nor3b_2_{inst}_A and nor3b_2_{inst}_B);"

def nor3_2(inst):
  return f"nor3_2_{inst}_Y <= not (nor3_2_{inst}_A or nor3_2_{inst}_B or nor3_2_{inst}_C);"

def or3_2(inst):
  return f"or3_2_{inst}_X <= or3_2_{inst}_A or or3_2_{inst}_B or or3_2_{inst}_C;"

def o31a_2(inst):
  return f"o31a_2_{inst}_X <= o31a_2_{inst}_B1 and (o31a_2_{inst}_A1 or o31a_2_{inst}_A2 or o31a_2_{inst}_A3);"

def o211a_2(inst):
  return f"o211a_2_{inst}_X <= o211a_2_{inst}_C1 and o211a_2_{inst}_B1 and (o211a_2_{inst}_A1 or o211a_2_{inst}_A2);"

def a2111oi_2(inst):
  return f"a2111oi_2_{inst}_Y <= not ((a2111oi_2_{inst}_A1 and a2111oi_2_{inst}_A2) or (a2111oi_2_{inst}_B1 or a2111oi_2_{inst}_C1 or a2111oi_2_{inst}_D1));"

def or2_2(inst):
  return f"or2_2_{inst}_X <= or2_2_{inst}_A or or2_2_{inst}_B;"

def and3b_2(inst):
  return f"and3b_2_{inst}_X <= (not and3b_2_{inst}_A_N) and and3b_2_{inst}_B and and3b_2_{inst}_C;"

def o221a_2(inst):
  return f"o221a_2_{inst}_X <= o221a_2_{inst}_C1 and (o221a_2_{inst}_A1 or o221a_2_{inst}_A2) and (o221a_2_{inst}_B1 or o221a_2_{inst}_B2);"

def o21ai_2(inst):
  return f"o21ai_2_{inst}_Y <= not (o21ai_2_{inst}_B1 and (o21ai_2_{inst}_A2 or o21ai_2_{inst}_A1));"

def a21boi_2(inst):
  return f"a21boi_2_{inst}_Y <= not ((not a21boi_2_{inst}_B1_N) or (a21boi_2_{inst}_A1 and a21boi_2_{inst}_A2));"

def o32a_2(inst):
  return f"o32a_2_{inst}_X <= (o32a_2_{inst}_B1 or o32a_2_{inst}_B2) and (o32a_2_{inst}_A1 or o32a_2_{inst}_A2 or o32a_2_{inst}_A3);"

def o22a_2(inst):
  return f"o22a_2_{inst}_X <= (o22a_2_{inst}_A1 or o22a_2_{inst}_A2) and (o22a_2_{inst}_B1 or o22a_2_{inst}_B2);"

def dfstp_2(inst):
  return f"""
process(dfstp_2_{inst}_SET_B, dfstp_2_{inst}_CLK) begin
  if dfstp_2_{inst}_SET_B = '0' then
    dfstp_2_{inst}_Q <= '1';
  elsif rising_edge(dfstp_2_{inst}_CLK) then
    dfstp_2_{inst}_Q <= dfstp_2_{inst}_D;
  end if;
end process;
"""

def a211o_2(inst):
  return f"a211o_2_{inst}_X <= a211o_2_{inst}_B1 or a211o_2_{inst}_C1 or (a211o_2_{inst}_A1 and a211o_2_{inst}_A2);"

def or3b_2(inst):
  return f"or3b_2_{inst}_X <= or3b_2_{inst}_A or or3b_2_{inst}_B or not or3b_2_{inst}_C_N;"

def a22oi_2(inst):
  return f"a22oi_2_{inst}_Y <= (not (a22oi_2_{inst}_A1 and a22oi_2_{inst}_A2)) and (not (a22oi_2_{inst}_B2 and a22oi_2_{inst}_B1));"

def nor4b_2(inst):
  return f"nor4b_2_{inst}_Y <= not ((not nor4b_2_{inst}_D_N) or nor4b_2_{inst}_C or nor4b_2_{inst}_B or nor4b_2_{inst}_A);"

def o21bai_2(inst):
  return f"o21bai_2_{inst}_Y <= not ((o21bai_2_{inst}_A1 or o21bai_2_{inst}_A2) and not o21bai_2_{inst}_B1_N);"

def a31oi_2(inst):
  return f"a31oi_2_{inst}_Y <= not (a31oi_2_{inst}_B1 or (a31oi_2_{inst}_A1 and a31oi_2_{inst}_A2 and a31oi_2_{inst}_A3));"

def a311o_2(inst):
  return f"a311o_2_{inst}_X <= a311o_2_{inst}_B1 or a311o_2_{inst}_C1 or (a311o_2_{inst}_A1 and a311o_2_{inst}_A2 and a311o_2_{inst}_A3);"

def o32ai_2(inst):
  return f"o32ai_2_{inst}_Y <= (not (o32ai_2_{inst}_B1 or o32ai_2_{inst}_B2)) or (not (o32ai_2_{inst}_A1 or o32ai_2_{inst}_A2 or o32ai_2_{inst}_A3));"

def a32o_2(inst):
  return f"a32o_2_{inst}_X <= (a32o_2_{inst}_B1 and a32o_2_{inst}_B2) or (a32o_2_{inst}_A1 and a32o_2_{inst}_A2 and a32o_2_{inst}_A3);"

def o2bb2a_2(inst):
  return f"o2bb2a_2_{inst}_X <= (o2bb2a_2_{inst}_B1 or o2bb2a_2_{inst}_B2) and not (o2bb2a_2_{inst}_A2_N and o2bb2a_2_{inst}_A1_N);"

def o22ai_2(inst):
  return f"o22ai_2_{inst}_Y <= (not (o22ai_2_{inst}_A1 or o22ai_2_{inst}_A2)) or (not (o22ai_2_{inst}_B1 or o22ai_2_{inst}_B2));"

def o311a_2(inst):
  return f"o311a_2_{inst}_X <= o311a_2_{inst}_C1 and o311a_2_{inst}_B1 and (o311a_2_{inst}_A1 or o311a_2_{inst}_A2 or o311a_2_{inst}_A3);"

def a21bo_2(inst):
  return f"a21bo_2_{inst}_X <= not (a21bo_2_{inst}_B1_N and not (a21bo_2_{inst}_A2 and a21bo_2_{inst}_A1));"

def o31ai_2(inst):
  return f"o31ai_2_{inst}_Y <= not (o31ai_2_{inst}_B1 and (o31ai_2_{inst}_A1 or o31ai_2_{inst}_A2 or o31ai_2_{inst}_A3));"

def o211ai_2(inst):
  return f"o211ai_2_{inst}_Y <= not (o211ai_2_{inst}_B1 and o211ai_2_{inst}_C1 and (o211ai_2_{inst}_A1 or o211ai_2_{inst}_A2));"

def o21ba_2(inst):
  return f"o21ba_2_{inst}_X <= not (o21ba_2_{inst}_B1_N or not (o21ba_2_{inst}_A1 or o21ba_2_{inst}_A2));"

def nand3_2(inst):
  return f"nand3_2_{inst}_Y <= not (nand3_2_{inst}_A and nand3_2_{inst}_B and nand3_2_{inst}_C);"

def dfxtp_2(inst):
  return f"""
process(dfxtp_2_{inst}_CLK) begin
  if rising_edge(dfxtp_2_{inst}_CLK) then
    dfxtp_2_{inst}_Q <= dfxtp_2_{inst}_D;
  end if;
end process;
"""

def nand3b_2(inst):
  return f"nand3b_2_{inst}_Y <= not (nand3b_2_{inst}_C and nand3b_2_{inst}_B and not nand3b_2_{inst}_A_N);"


map_write = {
  "diode_2" : diode_2,
  "decap_3" : decap_3,
  "tapvpwrvgnd_1" : tapvpwrvgnd_1,
  "clkbuf_4": clkbuf_4,
  "clkbuf_16" : clkbuf_16,
  "clkbuf_8" : clkbuf_8,
  "inv_2" : inv_2,
  "mux2_1":mux2_1,
  "a22o_2":a22o_2,
  "a221o_2":a221o_2,
  "a31o_2":a31o_2,
  "dfrtp_2":dfrtp_2,
  "or4bb_2":or4bb_2,
  "or4_2":or4_2,
  "conb_1":conb_1,
  "buf_2":buf_2,
  "and4_2":and4_2,
  "and3_2":and3_2,
  "and2b_2":and2b_2,
  "nor4_2":nor4_2,
  "nand4_2":nand4_2,
  "o21a_2":o21a_2,
  "nand2b_2":nand2b_2,
  "and2_2":and2_2,
  "or4b_2":or4b_2,
  "nand2_2":nand2_2,
  "nor2_2":nor2_2,
  "a21o_2":a21o_2,
  "and4bb_2":and4bb_2,
  "and4b_2":and4b_2,
  "xnor2_2":xnor2_2,
  "a21oi_2":a21oi_2,
  "a211oi_2":a211oi_2,
  "xor2_2":xor2_2,
  "a41oi_2":a41oi_2,
  "a221oi_2":a221oi_2,
  "nor3b_2":nor3b_2,
  "nor3_2":nor3_2,
  "or3_2":or3_2,
  "o31a_2":o31a_2,
  "o211a_2":o211a_2,
  "a2111oi_2":a2111oi_2,
  "or2_2":or2_2,
  "and3b_2":and3b_2,
  "o221a_2":o221a_2,
  "o21ai_2":o21ai_2,
  "a21boi_2":a21boi_2,
  "o32a_2":o32a_2,
  "o22a_2":o22a_2,
  "dfstp_2":dfstp_2,
  "a211o_2":a211o_2,
  "or3b_2":or3b_2,
  "a22oi_2":a22oi_2,
  "nor4b_2":nor4b_2,
  "o21bai_2":o21bai_2,
  "a31oi_2":a31oi_2,
  "a311o_2":a311o_2,
  "o32ai_2":o32ai_2,
  "a32o_2":a32o_2,
  "o2bb2a_2":o2bb2a_2,
  "o22ai_2":o22ai_2,
  "o311a_2":o311a_2,
  "a21bo_2":a21bo_2,
  "o31ai_2":o31ai_2,
  "o211ai_2":o211ai_2,
  "o21ba_2":o21ba_2,
  "nand3_2":nand3_2,
  "dfxtp_2":dfxtp_2,
  "nand3b_2": nand3b_2
}


with open("netlist.json") as in_file:
  objects = json.load(in_file)

net_names = objects["obj"].keys()

ios = ['I','clk','rst_n','enable','success', 'VGND', 'VPWR']
ios.extend([f'O[{i}]' for i in range(8)])

with open("puzzle.vhd", 'w') as fd :
  fd.write(Header)
  #write every driver that's not an io or VPWR, VGND
  for net in net_names:
    cell_nets = [re.sub(r'^sky130_fd_sc_hd__', '', net).replace('/','_')] if net not in ios else []
    cell_nets.extend([re.sub(r'^sky130_fd_sc_hd__', '', i).replace('/','_') for i in objects["obj"][net] if i not in ios])
    if (cell_nets): fd.write("signal " + ", ".join(cell_nets) + ": std_logic;\n")
  for net in net_names:
    if net.startswith('sky130_fd_sc_hd__df'):
      fd.write(f'attribute keep of {re.sub(r"^sky130_fd_sc_hd__", "", net).replace("/","_")}: signal is "true";\n')
  fd.write(Begin)

  #drive the driver into its loads
  for net in net_names:
    if(net in ["VPWR", "VGND"]): continue
    vhd_net = net.replace('[', '(').replace(']',')')
    fd.write("\n".join([f"{re.sub(r'^sky130_fd_sc_hd__', '', i).replace('/','_').replace('[', '(').replace(']',')')} <= {re.sub(r'^sky130_fd_sc_hd__', '', vhd_net).replace('/','_')};" for i in objects["obj"][net]]))
    fd.write("\n")

  #driver the driver based on its type and inputs
  for net in net_names:
    if net not in ios:
      write_pcell(fd, net)
  fd.write(Footer)