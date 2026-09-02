----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 26.08.2026 22:23:13
-- Design Name: 
-- Module Name: top - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;
use work.all;

entity top is
  Port ( clk     : in  std_logic;
    rst_n   : in  std_logic;
    enable  : in  std_logic;
    I       : in  std_logic;

    success : out std_logic;
    O       : out std_logic_vector(7 downto 0)
    );
end top;

architecture rtl of top is
 
begin

u_dut : entity work.puzzle
    port map(
        I => I,
        clk => clk,
        rst_n => rst_n,
        enable => enable,
        success => success,
        O => O
    );

end rtl;
