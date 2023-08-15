======
Morale
======

Sil has a sophisticated morale model for its enemies. Other than mindless creatures (which always just attack you) most enemies take stock of the situation and retreat if needed. They have a morale level which changes depending on the circumstances.

Bonuses:
--------
* The base for morale (6)
* Appearing after their usual depth (+1 per 50 ft)
* The player is escaping Angband with a Silmaril (+2, instead of the depth modifier)
* The player is blind, hallucinating, or stunned (+2 each)
* The player is confused, slow, afraid, or heavily stunned (+4 each)
* The player is entranced or knocked out (+8 each)
* The player has ≤ 75% health (+2)
* The player has ≤ 50% health (+4)
* The player has ≤ 25% health (+8)
* The monster is hasted (+4)
* Similar creatures in line of sight that are not fleeing (+1 each, or +4 for leaders)

Penalties:
----------
* Appearing before their usual depth (–1 per 50 ft)
* The monster is stunned (–2)
* The monster has ≤ 75% health (–2)
* The monster has ≤ 50% health (–4)
* The monster has ≤ 25% health (–8)
* The monster is already fleeing and ≤ 75% health (–2)
* Light susceptible and the player’s square has more than 3 light (–1 per level in excess)
* A non-unique monster is carrying items (–2 per item)
* The player has the ability Majesty (–1 per 4 points of player’s Will)
* The player has the ability Bane (–x, where x is the player’s Bane bonus)
* Similar creatures in line of sight that are fleeing (–1 each, or –4 for leaders)

Temporary modifiers (which decay by 10% each turn):
---------------------------------------------------
* Unable to escape (+6)
* Just stopped fleeing (+6)
* Just started fleeing (–6)
* It (or a similar creature) is hit by a slaying weapon (–2)
* It (or a similar creature) is hit by an elemental brand it is especially vulnerable to (–2)
* It (or a similar creature) is hit by a Cruel Blow (–2)
* A similar creature is killed in line of sight (–4, or –16 for a leader)
* Song of Elbereth (–1 per point by which you won the skill check)
* Horn of Terror, Staff of Majesty (–2 per point by which you won the skill check)

The morale level of an enemy determines its ‘stance’ which can be one of the following three:

Aggressive
----------
* (Morale > 20)
* Aggressive monsters simply attack you.
* Mindless creatures are always Aggressive.
* Trolls are Aggressive instead of Confident.
* Enemies who have been angered are Aggressive instead of Confident.
* Items of Wrath make creatures Aggressive instead of Confident.

Confident
---------
* (0 > Morale ≥ 20)
* Confident monsters may use tactics while attacking you (such as lurking in rooms waiting for you).

Fleeing
-------
* (Morale ≤ 0)
* Fleeing monsters try to get as far away from you as possible.
* Creatures that are immune to (non-magical) fear stay Confident instead of fleeing
