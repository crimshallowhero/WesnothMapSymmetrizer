# WesnothMapSymmetrizer
Utility program which makes Wesnoth maps symmetric.
(designed for 4 players maps)

# Usage
1. Launch.
2. Enter *.map file path.
3. Enter angle in degrees that determines which quarter of map is used as a template (so it should be divisible by 90).
   0 / empty - lower right;
   90 - lower left;
   180 - upper left;
   270 - upper right.
   Template quarter should have player starting point in it otherwise results might be unsatisfactory.
4. Resulting map will be dropped into the same directory with "sym_" prefix.
5. Enjoy.
