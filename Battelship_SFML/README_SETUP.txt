BATTLESHIP (SFML 3.0.2 version) - how to build in Visual Studio
===============================================================

1) Copy ALL files from this folder into your project folder (replace the old ones) and
   delete the OLD files that are no longer used:
   Board.cpp/.h, Ships.cpp/.h, Globals.cpp/.h, Ui.cpp/.h
   (Game, Score and Main are replaced by the new versions.)
   Then open Battelship.slnx.

2) Put SFML next to the project (easiest):
   Unzip SFML-3_0_2.zip so that you get a folder called  SFML-3.0.2  that contains
   include, lib and bin - and place that folder in the SAME folder as Battelship.vcxproj.
   Nothing else to configure.

   If you unzip it somewhere else (for example C:\SFML-3.0.2 it also works automatically),
   or any other place: open Battelship.vcxproj in Notepad and change the SFML_DIR line
   to that folder.

3) In Visual Studio choose  x64  and  Debug  (or Release), then press F5.
   The SFML DLLs are copied next to the exe automatically.

The leaderboard is saved in leaderboard.txt (in the folder where the game runs).

Controls: mouse for everything.  R / right-click = rotate ship.  M = sound on/off.  ESC = back / quit.
