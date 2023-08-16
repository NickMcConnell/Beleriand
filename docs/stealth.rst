=======
Stealth
=======

In the tale on which Sil is based, Beren and Luthien did manage to enter Angband, obtain a Silmaril, and escape. This they achieved not by force, but by subterfuge. Similarly it is possible to succeed in Sil by sneaking past your enemies rather than fighting them.

Mechanics
---------
At any time, an individual monster may be in one of three states:
Asleep
``````
* They cannot move and their evasion is set to [–5].
Unwary
``````
* They are awake and going about their business, but they do not know that you are there. Their evasion is halved.
Alert
`````
* This is further subdivided into whether they are Confident, Aggressive, or Fleeing, but this is a matter of morale rather than stealth.

This is modelled internally by an ‘Alertness’ score for the monster. Monsters always start out Asleep or Unwary, but may as a result of your actions notice you. An ‘Alertness’ of 0 or more means they are aware of you; between –1 and –10 means they are awake but unwary, and –11 or below usually means they are asleep. But be warned: not all inhabitants of Angband sleep!

Each round, you make a Stealth roll (your stealth score + 1d10), to see how stealthy and quiet you are. Each monster makes a Perception roll (their perception score +1d10), to see how observant they are. Various modifiers are applied, listed below. If at the end of this the monster's perception score is higher than your stealth score, their Alertness increases by difference which may make them move from Asleep to Unwary, or from Unwary to Alert.

Modifiers to Stealth
--------------------
Helpful factors:
````````````````
* Distance to monster: each square reduces monster perception by 1 point, and sound does not pass through stone or rubble at all.
* Each closed door on path reduces monster perception by 5.
* If you are in Stealth mode (press (S) to activate, but be warned that it makes you move more slowly than normal), you get a bonus of +5 to your stealth
* If you passed last turn (pressed (5) or (z), or (Z)), you get a bonus of +7 to your stealth (not cumulative with Stealth mode).

Hindrances:
```````````
* For each 10 lb of armour you are wearing (–1)
* Being attacked by one or more enemies (–2)
* Attacking one or more enemies (–2)
* Awake but unwary monsters which are in line of sight get some additional bonuses:

  - Doubles the modifiers for attacking or being attacked
  - For every adjacent passable square (–1)
  - So you may do well to stick to walls and corners.

* If you are singing, this reduces your stealth by a value equal to the noise level of the Song (see the list of Songs).
* Setting off some traps (–5 to –10)
* Landing from a leap (–5)

One-off effects that are not modified by stealth:
`````````````````````````````````````````````````
* Player actions:

  - Earthquakes (30), horns (10 to 40), smithing (10), bashing doors (5 or 10), tunneling (5 or 10), setting off some traps (–10 to 20).

* Monster abilities:

  - Earthquakes (30), arrows (5), boulders (10), crying out (10), breath weapons (10), unearthly screeching (20).

Stealth is thus quite abstract: a high stealth may mean little noise made, or that you kept in the shadows, or that you moved in an unobtrusive way, or let the noises that did occur sound like the natural background noises in the dungeon.

If you attack a monster, or an unaware monster tries to move into the square you are on, it will immediately notice you.

Once a monster has noticed you, it will become unwary again if you are out of sight of it, and it fails its perception check to spot you by 25 points or more. In this case, they lose a point of alertness for each point in excess of 25 that they fail by. This is very hard to achieve and will typically require you to get a fair distance from the monster and close some intervening doors. Note that this number is reduced to 15 if you have the ability ‘Vanish’. In addition, there are a few other ways in which you can make monsters become unwary:
* It is possible to put monsters to sleep with a Staff of Slumber or the Song of Lorien.
* Territorial monsters such as dragons will become unwary or even fall asleep if they have returned to their lairs and cannot see you.
