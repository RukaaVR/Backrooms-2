# Canonical Content IDs

Shared vocabulary between level generators, the material library, the audio
bank, prefabs and gameplay. Owned by the Technical Director: teams may ADD ids
(append to the relevant table in their branch), never rename or remove.

## Conventions
* UVs produced by generators are **world-space metres** (MeshData builders use
  uvScale = 1). A material's `MaterialDesc::uvScale` therefore equals
  `1 / physical size of one texture repeat in metres`.
* Albedo is sRGB; normal maps are tangent-space, OpenGL convention (+Y/green
  up); ORM = R ambient occlusion, G roughness, B metallic.
* Light intensities follow `render::LightDesc` (1.0 ≈ 100 lm source).

## Materials (`MaterialRegistry` names)

### Level 0 — The Lobby
| id | description |
|---|---|
| `level0.wallpaper` | mono-yellow patterned wallpaper, slightly damp, subtle seams/discolouration |
| `level0.wallpaper_stained` | water-damaged variant (tide marks near floor, mould spotting) |
| `level0.carpet` | damp mustard/tan low-pile commercial carpet |
| `level0.carpet_wet` | soaked variant: darker, lower roughness |
| `level0.ceiling_tile` | 0.6 m fissured acoustic drop-ceiling tile (grid seams in texture) |
| `level0.ceiling_tile_stained` | water-stained tile variant |
| `level0.ceiling_grid` | painted white steel T-bar |
| `level0.light_panel` | fluorescent troffer prismatic diffuser, emissive |
| `level0.light_panel_off` | same diffuser, unlit |
| `level0.light_housing` | painted sheet-metal troffer housing |
| `level0.baseboard` | painted MDF/vinyl baseboard, scuffed |
| `level0.outlet` | beige plastic outlet plate |

### Shared (`common.*`)
`common.concrete`, `common.concrete_painted`, `common.concrete_wet`, `common.plaster_painted`,
`common.brick`, `common.metal_painted`, `common.metal_rusty`, `common.steel_brushed`,
`common.aluminium`, `common.brass`, `common.wood_oak`, `common.wood_painted`, `common.plywood`,
`common.glass_clear`, `common.glass_frosted`, `common.plastic_white`, `common.plastic_black`,
`common.rubber_black`, `common.cardboard`, `common.paper`, `common.fabric_grey`,
`common.tile_white`, `common.asphalt`, `common.grass`, `common.dirt`, `common.gravel`,
`common.rock`, `common.emissive_white`, `common.emissive_warm`

### Level-specific
| level | ids |
|---|---|
| 1 Habitable Zone | `level1.concrete_floor`, `level1.concrete_wall`, `level1.concrete_ceiling`, `level1.puddle` |
| 2 Pipe Dreams | `level2.pipe`, `level2.pipe_rusty`, `level2.wall_grime`, `level2.floor_grating`, `level2.valve_red` |
| 3 Electrical Station | `level3.panel_grey`, `level3.cable_black`, `level3.machine_green`, `level3.floor_epoxy`, `level3.hazard_stripe` |
| 4 Abandoned Office | `level4.carpet_tile`, `level4.cubicle_fabric`, `level4.desk_laminate`, `level4.wall_paint`, `level4.ceiling_tile`, `level4.window_glass` |
| 5 Terror Hotel | `level5.wallpaper_damask`, `level5.carpet_pattern`, `level5.wood_panel`, `level5.marble`, `level5.brass`, `level5.velvet_red` |
| 6 Lights Out | `level6.concrete_dark`, `level6.wall_dark` |
| 7 Thalassophobia | `level7.tile_dark`, `level7.wall_algae` |
| 8 Cave System | `level8.rock`, `level8.rock_wet`, `level8.limestone`, `level8.mud` |
| 9 Darkened Suburbs | `level9.siding`, `level9.roof_shingle`, `level9.lawn`, `level9.sidewalk`, `level9.road`, `level9.brick`, `level9.window_dark` |
| 10 Bumper Crop | `level10.wheat`, `level10.dirt_road`, `level10.barn_red`, `level10.hay`, `level10.farmhouse_siding` |
| 11 Endless City | `level11.facade_concrete`, `level11.facade_glass`, `level11.road`, `level11.sidewalk`, `level11.storefront` |
| 37 Poolrooms | `level37.pool_tile`, `level37.pool_tile_wet`, `level37.pool_floor_tile`, `level37.grout` |
| 94 Motion | `level94.grass`, `level94.house_paint`, `level94.roof` |
| 188 Courtyard of Windows | `level188.facade`, `level188.window_frame`, `level188.window_glass`, `level188.paving` |
| Fun Party Rooms | `levelfun.wallpaper`, `levelfun.carpet`, `levelfun.balloon_red`, `levelfun.balloon_blue`, `levelfun.balloon_yellow`, `levelfun.tablecloth`, `levelfun.confetti` |

## Sounds (audio bank ids)
| group | ids |
|---|---|
| fixtures | `fixture.hum`, `fixture.buzz`, `fixture.flicker_click`, `fixture.fail_pop` |
| footsteps | `foot.<surface>` for every `physics::Surface` (e.g. `foot.carpet`, `foot.wet_carpet`, `foot.concrete`, `foot.tile`, `foot.water`) ; `foot.land`, `foot.jump` |
| impacts | `impact.<surface>.light`, `impact.<surface>.heavy` |
| doors | `door.open`, `door.close`, `door.creak`, `door.locked`, `door.slam` |
| interaction | `ui.pickup`, `ui.drop`, `ui.throw`, `switch.click`, `item.drink`, `item.battery`, `flashlight.on`, `flashlight.off`, `paper.rustle` |
| ambience | `amb.level0`, `amb.level1`, ... `amb.level37`, `amb.level94`, `amb.level188`, `amb.levelfun` |
| emitters | `emit.hvac`, `emit.drip`, `emit.pipe_creak`, `emit.steam`, `emit.generator`, `emit.water_lap`, `emit.wind`, `emit.party_music`, `emit.crickets`, `emit.streetlight` |
| entities | `ent.hound.growl`, `ent.hound.bark`, `ent.hound.run`, `ent.smiler.hum`, `ent.smiler.whisper`, `ent.faceling.murmur`, `ent.skinstealer.click`, `ent.partygoer.giggle`, `ent.partygoer.step` |
| horror | `horror.sting_low`, `horror.distant_steps`, `horror.knock`, `horror.metal_bang`, `horror.whisper`, `horror.power_down`, `horror.power_up` |
| player | `player.heartbeat`, `player.breath_calm`, `player.breath_heavy`, `player.hurt`, `player.swim`, `noclip.glitch` |
| ui | `ui.hover`, `ui.click`, `ui.back`, `ui.open`, `ui.error` |

## Prefabs
`fixture.troffer_2x4`, `fixture.tube_batten`, `fixture.bulkhead`, `fixture.bulb`, `fixture.streetlight`,
`fixture.chandelier`, `fixture.sconce`, `door.wood`, `door.metal`, `prop.crate_wood`, `prop.cardboard_box`,
`prop.chair_office`, `prop.chair_wood`, `prop.desk_office`, `prop.table_round`, `prop.filing_cabinet`,
`prop.shelf_metal`, `prop.barrel`, `prop.bucket`, `prop.traffic_cone`, `prop.vending_machine`

## Items
`flashlight`, `battery`, `almond_water`, `medkit`, `bandage`, `key_generic`,
`key_<id>`, `document_<id>`, `energy_bar`, `crowbar`, `fuse`, `lighter`

## Entities
`hound`, `smiler`, `faceling`, `skin_stealer`, `partygoer`
