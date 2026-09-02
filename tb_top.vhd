----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 26.08.2026 23:00:34
-- Design Name: 
-- Module Name: tb_top - Behavioral
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

use std.env.finish;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
  use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity tb_top is
--  Port ( );
end tb_top;

architecture Behavioral of tb_top is
  signal I, enable, clk, rst_n, success : std_logic := '0';
  signal O : std_logic_vector(7 downto 0);
  signal mod10 : integer := 0;
  constant SEQ_C : std_logic_vector(120 downto 0) := (7|9|11|16|29|31|33|35|48|50|57|63|70|76|78|83|91|98|104|107|111|113 => '1', others => '0');
begin

  process begin
    wait for 5 ns;
    clk <= not clk;
  end process;


  u_dut : entity work.top
    port map(
        I => I,
        clk => clk,
        rst_n => rst_n,
        enable => enable,
        success => success,
        O => O
    );
    
  process(clk) begin
    if (rst_n = '0') then 
      mod10 <= 0;
    elsif rising_edge(clk) then
      if (enable = '1') then
      if (mod10 = (TO_UNSIGNED(10,4))) then 
        mod10 <= 0;
      else 
        mod10 <= mod10 + 1;
      end if;
    end if;
    end if;
  end process;
  
  process 
  
  begin 
  rst_n <= '0';
    wait for 1 ns;
    rst_n <= '1';
    wait until rising_edge(clk);
    enable <= '1';
    
    for loopcnt in 0 to 120 loop
      I <= SEQ_C(loopcnt);
      wait until rising_edge(clk);
    end loop;
    I <= '0';
    --for looop in 1 to 10 loop
    --    wait until mod10 = 0;
    --    I <= '1';
    --    for loopcnt in 1 to 2 loop
    --      wait until rising_edge(clk);
    --    end loop;
    --    I <= '0';
    --end loop;
    wait;
  end process;


  process begin 
    wait for 10000 ns;
    finish;
  end process;


end Behavioral;
