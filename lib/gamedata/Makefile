MKPATH=../../mk/
include $(MKPATH)buildsys.mk

SUBDIRS = rivers
CONFIG = a_cursed.txt a_normal.txt activation.txt artifact.txt body.txt \
 blow_methods.txt blow_effects.txt brand.txt bravado.txt chest_trap.txt \
 class.txt constants.txt curse.txt dungeon_profile.txt ego_item.txt flavor.txt \
 history.txt landmark.txt monster.txt monster_base.txt \
 monster_spell.txt names.txt object.txt object_base.txt object_property.txt \
 p_race.txt pain.txt pit.txt player_property.txt player_timed.txt \
 player_unarmed.txt projection.txt quest.txt race_relations.txt realm.txt \
 region.txt river.txt room_template.txt set_item.txt shape.txt slay.txt \
 summon.txt surface_profile.txt terrain.txt trap.txt ui_entry.txt \
 ui_entry_base.txt ui_entry_renderer.txt vault.txt visuals.txt w_cursed.txt \
 w_normal.txt

R1 = $(GAMEDATA_IN_LIB:true=$(FILES))
DATA = $(R1:false=)
R2 = $(GAMEDATA_IN_LIB:false=$(FILES))
CONFIG = $(R2:true=)

