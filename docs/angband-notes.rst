=========================
Notes for Angband players
=========================

Sil borrows much from the looks and interface of Angband, but almost every game mechanic has changed. While there are many similarities, the design principle was to do things as the authors believe would be best for a new game, rather than to compromise between this and how things were done in the past. However the learning curve should be mostly remedied by this document. If you just want to dive in, then simply read the rest of this section, start a new game, create a character, then press (?) for a nice explanation of the commands in Sil. At that point you are ready to try it out. Then have a look at this manual when you want to know more about the combat system or the skill system or the abilities...

You might be interested to know that Sil is actually descended from a variant of Angband (NPPAngband). However, this is scarcely evident in gameplay as so many NPP features have been changed or removed. The main legacy is the 4GAI code.

Main changes
------------
* You are no longer trying to kill Morgoth

  * The aim is now to free a Silmaril from Morgoth’s crown, then escape

* The game is much shorter

  * Morgoth is found at 1,000 ft
  * A winning game should take about 10 hours

* There is no town or word of recall
* It has a much more coherent theme: The First Age of Middle Earth
* There are no classes, or experience levels

  * Instead there are 8 skills to improve
  * With a tree of special abilities for each

* The combat system is completely new

  * It is very thoroughly thought out and tested
  * It leads to many more interesting choices for the player

* Monster AI is much better

  * It uses 4GAI, plus numerous improvements

* The monsters, items, artefacts and almost completely rewritten
* There is much less explicit magic

  * e.g. no fireballs, no teleportation...

* There is more tactical depth in combat

  * Partly due to the absence of teleportation and other easy escapes

* There is an effective time limit

  * Over time you can no longer find your way to the shallower depths
  * You can thus play each level more than once, but not indefinitely

System changes
--------------
* Stats

  * Based around the average for the Men of the first age (set as 0)
  * There are no ‘stat potions’, so initial stats matter a lot more
  * Int/Wis/Cha are replaced with Grace

* Hit points don’t go up with experience

  * They are solely based on your Constitution
  * You die at 0 (instead of –1 in Angband)

* Elements

  * There are only four elemental attacks (fire, cold, poison, dark)
  * Resistances stack, reducing damage to 1⁄2, then 1⁄3, then 1⁄4, ...
  * Dark resistance is determined by your light level
  * Poison does no direct damage, but damages over time

* Speed is much less fine grained and less available

  * The only speeds are 1, 2, 3, 4
  * normal speed is 2, so +1 speed makes you move 50% faster

* The only ‘pseudo id’ is {special}, which covers artefacts and ‘ego-items’
* You can only tunnel with shovels and mattocks
