"""Run with UnrealEditor-Cmd -run=pythonscript -script=.../build_sector.py."""
import unreal as u
import random
import math
random.seed(4104)
ROOT = '/Game/Pogonya'
assets = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()
edit = u.MaterialEditingLibrary
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
assets.make_directory(ROOT+'/Maps')
assets.make_directory(ROOT+'/Materials')

def material(name, color, rough=.65, metallic=0, glow=0, noise=False):
    path=ROOT+'/Materials/'+name
    m=assets.load_asset(path) if assets.does_asset_exist(path) else None
    if m: return m
    m=tools.create_asset(name, ROOT+'/Materials',u.Material,u.MaterialFactoryNew())
    c=edit.create_material_expression(m,u.MaterialExpressionConstant3Vector,-400,0)
    c.set_editor_property('constant',u.LinearColor(*color,1))
    if noise:
        n=edit.create_material_expression(m,u.MaterialExpressionNoise,-650,160)
        n.set_editor_property('scale',.025)
        n.set_editor_property('levels',3)
        lerp=edit.create_material_expression(m,u.MaterialExpressionLinearInterpolate,-180,0)
        d=edit.create_material_expression(m,u.MaterialExpressionConstant3Vector,-400,-120)
        d.set_editor_property('constant',u.LinearColor(*(x*.36 for x in color),1))
        edit.connect_material_expressions(d,'',lerp,'A')
        edit.connect_material_expressions(c,'',lerp,'B')
        edit.connect_material_expressions(n,'',lerp,'Alpha')
        edit.connect_material_property(lerp,'',u.MaterialProperty.MP_BASE_COLOR)
    else: edit.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
    for value,prop,y in [(rough,u.MaterialProperty.MP_ROUGHNESS,250),(metallic,u.MaterialProperty.MP_METALLIC,330)]:
        node=edit.create_material_expression(m,u.MaterialExpressionConstant,-150,y)
        node.set_editor_property('r',value)
        edit.connect_material_property(node,'',prop)
    if glow:
        mult=edit.create_material_expression(m,u.MaterialExpressionMultiply,-150,-180)
        mult.set_editor_property('const_b',glow)
        edit.connect_material_expressions(c,'',mult,'A')
        edit.connect_material_property(mult,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.recompile_material(m)
    assets.save_asset(path)
    return m

M={}
for name,c,r,metal,glow,noise in [
 ('Asphalt',(.075,.085,.09),.25,.12,0,True),
 ('Brick',(.24,.16,.12),.88,0,0,True),
 ('Concrete',(.29,.30,.28),.88,0,0,True),
 ('Steel',(.09,.12,.12),.4,.78,0,True),
 ('Container',(.075,.18,.17),.5,.65,0,True),
 ('Rust',(.28,.12,.055),.65,.6,0,True),
 ('Wood',(.22,.13,.065),.83,0,0,True),
 ('Rubber',(.018,.025,.029),.8,0,0,False),
 ('Hunter',(.42,.075,.035),.75,0,0,True),
 ('Runner',(.045,.21,.25),.76,0,0,True),
 ('Mask',(.7,.69,.58),.62,.1,0,False),
 ('Water',(.045,.07,.075),.065,.8,0,False),
 ('Lamp', (1,.49,.12),.25,.1,7,False),
 ('Red',(.9,.025,.008),.25,.1,5,False),
 ('Key',(.95,.58,.1),.3,.65,.8,False),
 ('Paint',(.79,.61,.24),.7,0,0,True),
 ('Window',(.12,.3,.32),.22,.45,.25,False),
]: M[name]=material('M_'+name,c,r,metal,glow,noise)

map_path=ROOT+'/Maps/L_Sector04'
if assets.does_asset_exist(map_path):
    level.load_level(map_path)
    if any(isinstance(old,u.StaticMeshActor) for old in actors.get_all_level_actors()):
        raise RuntimeError('Level already contains geometry. Preserve it; select a new map_path to generate another version.')
else:
    level.new_level(map_path)
cube=assets.load_asset('/Engine/BasicShapes/Cube')
cyl=assets.load_asset('/Engine/BasicShapes/Cylinder')
sphere=assets.load_asset('/Engine/BasicShapes/Sphere')
count=0
def mesh(label,p,size,mat='Steel',rot=(0,0,0),shape=None,collision=True,folder='Architecture'):
    global count
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*p),u.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
    a.set_actor_label(label)
    a.set_folder_path(folder)
    comp=a.static_mesh_component
    comp.set_static_mesh(shape or cube)
    comp.set_material(0,M[mat])
    a.set_actor_scale3d(u.Vector(*(x/100 for x in size)))
    if not collision:comp.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    count+=1
    return a

def text(label,p,size=70,color=(.8,.65,.34),rot=(0,180,0)):
    a=actors.spawn_actor_from_class(u.TextRenderActor,u.Vector(*p),u.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
    a.set_actor_label('Sign / '+label)
    a.set_folder_path('Signage')
    c=a.text_render
    c.set_text(label)
    c.set_world_size(size)
    c.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    c.set_text_render_color(u.Color(*(int(x*255) for x in color),255))
    return a

def light(p,color=(1,.55,.23),power=5500,radius=1100):
    a=actors.spawn_actor_from_class(u.PointLight,u.Vector(*p))
    a.set_folder_path('Lighting')
    c=a.point_light_component
    c.set_editor_property('intensity',power)
    c.set_editor_property('attenuation_radius',radius)
    c.set_light_color(u.LinearColor(*color,1))
    c.set_editor_property('source_radius',18)
    return a

def container(x,y,z=145,yaw=0,shade='Container',label='Cargo'):
    # Local axes: width 260, length 620. Corrugated walls, framed double doors.
    t=math.radians(yaw)
    def at(px,py,pz):return (x+px*math.cos(t)-py*math.sin(t),y+px*math.sin(t)+py*math.cos(t),z+pz)
    mesh(label,at(0,0,0),(260,620,290),shade,(0,yaw,0),folder='Containers')
    for side in [-1,1]:
        for k in range(25):mesh('Corrugated rib',at(side*132,-294+k*24,0),(6,8,270),shade,(0,yaw,0),folder='Containers',collision=False)
        for h in [-137,137]:mesh('Container edge',at(side*130,0,h),(12,636,12),'Rust',(0,yaw,0),folder='Containers')
    for px in [-62,62]:
        mesh('Cargo door',at(px,-313,0),(121,8,270),shade,(0,yaw,0),folder='Containers')
        mesh('Locking bar',at(px,-320,0),(5,5,245),'Steel',(0,yaw,0),folder='Containers',collision=False)
        mesh('Handle',at(px+16,-326,-12),(32,8,6),'Rust',(0,yaw,0),folder='Containers',collision=False)

def pallet(x,y,yaw=0):
    for k in range(5):mesh('Pallet slat',(x-48+k*24,y,15),(17,110,9),'Wood',(0,yaw,0),folder='Props')
    for off in [-42,42]:mesh('Pallet foot',(x,y+off,7),(110,13,14),'Wood',(0,yaw,0),folder='Props')

mesh('Wet asphalt foundation',(0,0,-35),(4600,5800,70),'Asphalt')
for side in [-1,1]:
    # Tall brick warehouses enclose the yard.
    mesh('Warehouse facade',(side*2420,0,850),(420,6000,1700),'Brick')
    mesh('Concrete plinth',(side*2195,0,95),(35,5900,190),'Concrete')
    for y in range(-2600,2900,620):
        mesh('Facade pier',(side*2190,y,750),(50,65,1500),'Concrete')
        mesh('Recessed factory window',(side*2170,y+260,900),(15,370,310),'Rubber')
        for dz in [-115,0,115]:mesh('Window horizontal mullion',(side*2157,y+260,900+dz),(8,350,7),'Steel')
        for dy in [-130,0,130]:mesh('Window vertical mullion',(side*2155,y+260+dy,900),(8,7,290),'Steel')
        mesh('Window glass',(side*2162,y+260,900),(8,340,280),'Window',collision=False)
    for h in [400,1270,1650]:mesh('Facade ledge',(side*2170,0,h),(75,5940,22),'Concrete')
    for y in [-1800,600,2200]:
        mesh('Drainpipe',(side*2130,y,600),(16,16,1200),'Rust',shape=cyl)
        for h in [200,600,1000]:mesh('Pipe clamp',(side*2130,y,h),(25,25,12),'Steel',shape=cyl,collision=False)
    for y in [-2000,-400,1200]:
        mesh('Wall lamp housing',(side*2140,y,540),(70,85,22),'Steel')
        mesh('Wall lamp diffuser',(side*2140,y,526),(55,66,5),'Lamp',collision=False)
        light((side*2030,y,490),power=6500,radius=1250)
mesh('South boundary',(0,-2900,520),(4400,100,1040),'Brick')
mesh('North boundary left',(-1440,2880,550),(1550,100,1100),'Brick')
mesh('North boundary right',(1440,2880,550),(1550,100,1100),'Brick')
# Escape gate is a closed backdrop: interaction at its threshold is the extraction.
mesh('Exit blast door',(0,2860,260),(1260,45,520),'Steel',folder='Exit')
for x in [-635,635]:mesh('Gate concrete frame',(x,2815,310),(85,150,620),'Concrete',folder='Exit')
mesh('Gate lintel',(0,2820,605),(1360,160,90),'Concrete',folder='Exit')
for x in [-300,300]:
    mesh('Gate leaf',(x,2828,255),(580,16,480),'Rust',folder='Exit')
    for z in [80,430]:mesh('Gate brace',(x,2814,z),(570,12,14),'Steel',folder='Exit')
    mesh('Gate diagonal',(x,2800,255),(660,12,15),'Steel',(35,0,0),folder='Exit')
mesh('Gate pedestrian door',(-330,2804,155),(140,16,280),'Steel',folder='Exit')
mesh('Gate office hatch',(270,2800,270),(190,16,90),'Window',folder='Exit')
for x in [-500,500]:
    mesh('Gate floodlight',(x,2760,640),(110,35,35),'Lamp',folder='Exit',collision=False)
    light((x,2600,570),power=11000,radius=1400)
text('SECTOR 04  /  EXTRACTION',(0,2790,480),56,rot=(0,-90,0))
text('AUTHORIZED PERSONNEL ONLY',(0,2780,395),27,rot=(0,-90,0))
text('04',(-1980,-2800,430),130,rot=(0,90,0))

# Three connected lanes; cross aisles remain open for chases.
for x,y,yaw in [(-1900,-1850,0),(-1900,-1100,0),(-1900,1000,0),(-1900,1800,0),(1900,-1600,0),(1900,-850,0),(1900,1050,0),(1900,1800,0),(-650,-750,90),(650,700,90),(-670,1660,90),(650,-1650,90)]:
    container(x,y,yaw=yaw)
for x,y in [(-1900,-1850),(-1900,1000),(1900,-850),(1900,1800)]:container(x,y,445,shade='Rust',label='Stacked cargo')
# Elevated service pipes over the cross aisles.
for y in [-1250,1450]:
    for x in [-2070,2070]:mesh('Pipe support',(x,y,370),(24,24,740),'Steel')
    for offset in [-45,0,45]:mesh('Overhead pipe',(0,y+offset,730),(27,27,4260),'Rust',(90,0,0),shape=cyl)
    for x in range(-1900,2000,420):mesh('Pipe rack',(x,y,703),(30,190,16),'Steel')

# Security booth with lit windows near the exit.
mesh('Security booth',(-1160,2310,190),(490,370,380),'Concrete')
mesh('Booth roof',(-1160,2310,395),(550,420,30),'Steel')
mesh('Booth front window',(-1160,2118,240),(300,8,140),'Lamp',collision=False)
mesh('Window mullion',(-1160,2108,240),(9,15,145),'Steel')
mesh('Booth side window',(-910,2310,240),(8,220,140),'Window')
mesh('Booth door',(-1310,2110,135),(80,15,260),'Steel')
light((-1110,2050,290),(1,.67,.3),2200,650)
text('CONTROL',(-1150,2100,365),35,rot=(0,-90,0))
for x,y in [(-1500,-2200),(1500,-2200),(-1400,500),(1400,100),(-1550,2050),(1650,2350),(-350,1100),(550,-1000)]:
    pallet(x,y)
    for dx,dy in [(0,80),(70,65)]:
        mesh('Oil drum',(x+dx,y+dy,46),(58,58,92),'Rust',shape=cyl,folder='Props')
        for z in [12,78]:mesh('Drum ring',(x+dx,y+dy,z),(61,61,5),'Steel',shape=cyl,folder='Props',collision=False)
    mesh('Supply crate',(x-80,y+110,45),(90,85,90),'Wood',folder='Props')
for i in range(35):
    x=random.uniform(-2050,2050);y=random.uniform(-2600,2650)
    mesh('Rain puddle',(x,y,1.0),(random.uniform(80,360),random.uniform(100,500),1.5),'Water',shape=sphere,collision=False,folder='Ground details')
for i in range(48):
    side=random.choice([-1,1]);x=side*random.uniform(2020,2080);y=random.uniform(-2600,2600)
    mesh('Scattered debris',(x,y,6),(random.uniform(8,25),random.uniform(12,40),random.uniform(3,10)),'Concrete',(0,random.uniform(0,180),0),folder='Ground details')
for y in range(-2500,2550,270):
    mesh('Faded lane marking',(0,y,.8),(10,120,1),'Paint',collision=False,folder='Ground details')
for x in [-800,800]:mesh('Drain channel',(x,0,.8),(22,5500,1),'Rubber',collision=False,folder='Ground details')

# Distant silhouettes and red warning beacons.
for x,y,h in [(-3000,3400,2300),(2700,3300,1900),(-3200,-3400,1900),(3100,-3200,2400)]:
    mesh('Factory skyline',(x,y,h/2),(900,850,h),'Concrete',folder='Skyline')
    mesh('Chimney',(x+100,y,h+350),(130,130,700),'Brick',shape=cyl,folder='Skyline')
    light((x+100,y,h+720),(1,.03,.01),700,600)
    mesh('Chimney beacon',(x+100,y,h+710),(28,28,20),'Red',shape=cyl,collision=False,folder='Skyline')

sun=actors.spawn_actor_from_class(u.DirectionalLight,u.Vector(0,0,1800),u.Rotator(pitch=-38,yaw=-25,roll=0))
sun.get_component_by_class(u.DirectionalLightComponent).set_editor_property('intensity',.65)
sun.get_component_by_class(u.DirectionalLightComponent).set_light_color(u.LinearColor(.28,.43,.58,1))
sky=actors.spawn_actor_from_class(u.SkyLight,u.Vector(0,0,1000))
sky.get_component_by_class(u.SkyLightComponent).set_editor_property('intensity',.28)
sky.get_component_by_class(u.SkyLightComponent).set_editor_property('real_time_capture',True)
actors.spawn_actor_from_class(u.SkyAtmosphere,u.Vector(0,0,-100))
fog=actors.spawn_actor_from_class(u.ExponentialHeightFog,u.Vector(0,0,0))
fc=fog.get_component_by_class(u.ExponentialHeightFogComponent)
fc.set_editor_property('fog_density',.035)
fc.set_editor_property('fog_height_falloff',.25)
fc.set_editor_property('fog_inscattering_luminance',u.LinearColor(.055,.085,.1,1))
fc.set_editor_property('enable_volumetric_fog',True)
post=actors.spawn_actor_from_class(u.PostProcessVolume,u.Vector(0,0,0))
post.set_editor_property('unbound',True)
settings=post.get_editor_property('settings')
for k,v in [('override_auto_exposure_min_brightness',True),('auto_exposure_min_brightness',.4),('override_auto_exposure_max_brightness',True),('auto_exposure_max_brightness',.4),('override_bloom_intensity',True),('bloom_intensity',.45),('override_vignette_intensity',True),('vignette_intensity',.3)]:
    settings.set_editor_property(k,v)
post.set_editor_property('settings',settings)
for i in range(6):
    actors.spawn_actor_from_class(u.PlayerStart,u.Vector(-750+i*300,-2350,100),u.Rotator(pitch=0,yaw=90,roll=0))
mode=u.load_class(None,'/Script/Pogonya.ChaseMode')
world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',mode)
world.get_world_settings().set_editor_property('tags',[u.Name('Pogonya_RotationsV2')])
u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(u.Vector(140,-2450,300),u.Rotator(pitch=-6,yaw=87,roll=0))
level.save_current_level()
assets.save_directory(ROOT,only_if_is_dirty=True,recursive=True)
u.log('POGONYA: Sector04 generated successfully. Mesh actors: '+str(count))
