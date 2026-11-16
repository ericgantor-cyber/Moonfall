extends RefCounted
## Americas wildlife catalog for Moonfall's six existing biomes.
## These are named, habitat-grouped species profiles using inherited animal rigs.
## Catalog entries do NOT imply new custom species models/animations or all extant
## species of the Americas. Never allocate an actor for every catalog entry.
const TAXON_LABELS:Dictionary={"M":"Mammal","B":"Bird","R":"Reptile","A":"Amphibian"}
const HABITATS:Array[Dictionary]=[
 {"habitat":"Grasslands and tundra","families":[
  {"archetype":"boar","taxon":"M","names":"American bison|Wood bison|Collared peccary|White-lipped peccary|American badger|Giant anteater|Giant armadillo|Southern tamandua"},
  {"archetype":"elk","taxon":"M","names":"Pronghorn|American elk|Moose|Caribou|Musk ox|Bighorn sheep|Pampas deer|Guanaco|White-tailed deer|Mule deer"},
  {"archetype":"fox","taxon":"M","names":"Swift fox|Arctic fox|Black-footed ferret|Black-tailed prairie dog|White-tailed prairie dog|Eastern cottontail|Long-tailed weasel|American ermine|Thirteen-lined ground squirrel"},
  {"archetype":"wolf","taxon":"M","names":"Gray wolf|Mexican wolf|Red wolf|Coyote|Maned wolf|Arctic wolf"},
  {"archetype":"owl","taxon":"B","names":"Burrowing owl|Snowy owl|Short-eared owl|Ferruginous hawk|Northern harrier|American kestrel|Greater prairie chicken|Sandhill crane|Turkey vulture|Red-tailed hawk"}
 ]},
 {"habitat":"Temperate and cloud forests","families":[
  {"archetype":"wolf","taxon":"M","names":"Eastern wolf|Timber wolf|Bush dog|Mexican gray wolf|Crab-eating fox|Coyote"},
  {"archetype":"panther","taxon":"M","names":"Cougar|Bobcat|Canada lynx|Ocelot|Margay|Jaguarundi|Geoffroy's cat|Kodkod"},
  {"archetype":"boar","taxon":"M","names":"American black bear|Brown bear|Spectacled bear|North American porcupine|North American beaver|Coatimundi|American marten|Fisher"},
  {"archetype":"elk","taxon":"M","names":"White-tailed deer|Black-tailed deer|Roosevelt elk|Red brocket deer|Southern pudu|Northern pudu|Marsh deer"},
  {"archetype":"fox","taxon":"M","names":"Red fox|Gray fox|Raccoon|Striped skunk|Virginia opossum|Wolverine|Ringtail|Kinkajou|American mink"},
  {"archetype":"owl","taxon":"B","names":"Great horned owl|Barred owl|Northern spotted owl|Pileated woodpecker|Bald eagle|Golden eagle|Blue jay|Northern cardinal|Scarlet tanager|Resplendent quetzal"}
 ]},
 {"habitat":"Wetlands rivers and coasts","families":[
  {"archetype":"serpent","taxon":"R","names":"American alligator|American crocodile|Spectacled caiman|Black caiman|Yacare caiman|Green anaconda|Yellow anaconda|Boa constrictor|Cottonmouth|Common snapping turtle|Mata mata turtle|Green iguana"},
  {"archetype":"boar","taxon":"M","names":"Capybara|Lowland tapir|Baird's tapir|Giant river otter|North American river otter|Muskrat|Marsh rice rat|West Indian manatee"},
  {"archetype":"fox","taxon":"M","names":"Neotropical otter|American mink|Marsh rabbit|Swamp rabbit|Crab-eating raccoon|Sea otter"},
  {"archetype":"elk","taxon":"M","names":"Marsh deer|Brown brocket deer|Red brocket deer|Water opossum"},
  {"archetype":"owl","taxon":"B","names":"Roseate spoonbill|Great blue heron|Scarlet ibis|Snowy egret|Wood stork|Great egret|American flamingo|Black-necked stilt|Anhinga|Green kingfisher|Brown pelican"},
  {"archetype":"serpent","taxon":"A","names":"Axolotl|Hellbender|Tiger salamander|Red-eyed tree frog|American bullfrog|Glass frog|Poison dart frog"}
 ]},
 {"habitat":"Mountains and alpine tundra","families":[
  {"archetype":"elk","taxon":"M","names":"Mountain goat|Dall sheep|Stone sheep|Desert bighorn sheep|Rocky Mountain elk|Vicuña|Llama|Alpaca|Guanaco|Mountain tapir"},
  {"archetype":"panther","taxon":"M","names":"Puma|Andean mountain cat|Geoffroy's cat|Canada lynx|Ocelot|Jaguar"},
  {"archetype":"boar","taxon":"M","names":"Andean bear|Hoary marmot|Yellow-bellied marmot|Mountain vizcacha|Chinchilla|Wolverine|Mountain paca"},
  {"archetype":"fox","taxon":"M","names":"Culpeo|Andean fox|Hoary fox|Mountain cottontail|American pika|Patagonian mara|Patagonian weasel"},
  {"archetype":"owl","taxon":"B","names":"Andean condor|California condor|Golden eagle|Peregrine falcon|Mountain bluebird|Andean flicker|Rufous-collared sparrow|Harris's hawk|Violet-green swallow"}
 ]},
 {"habitat":"Deserts and scrublands","families":[
  {"archetype":"serpent","taxon":"R","names":"Western diamondback rattlesnake|Eastern diamondback rattlesnake|Mojave rattlesnake|Sidewinder|Gila monster|Desert tortoise|Chuckwalla|Texas horned lizard|Desert iguana|Arizona coral snake|Green rattlesnake"},
  {"archetype":"fox","taxon":"M","names":"Kit fox|Desert cottontail|Black-tailed jackrabbit|Antelope jackrabbit|Merriam's kangaroo rat|Desert woodrat|Ringtail|Gray fox|Pygmy rabbit|Spotted skunk"},
  {"archetype":"boar","taxon":"M","names":"Nine-banded armadillo|Southern three-banded armadillo|Six-banded armadillo|Collared peccary|Giant armadillo"},
  {"archetype":"elk","taxon":"M","names":"Desert bighorn sheep|Pronghorn|Mule deer|Guanaco|Pampas deer"},
  {"archetype":"wolf","taxon":"M","names":"Coyote|Mexican wolf|Maned wolf|Hoary fox|South American gray fox"},
  {"archetype":"owl","taxon":"B","names":"Greater roadrunner|Cactus wren|Burrowing owl|Elf owl|Crested caracara|Turkey vulture|Harris's hawk|Gambel's quail|Vermilion flycatcher|Lesser nighthawk"}
 ]},
 {"habitat":"Tropical rainforests","families":[
  {"archetype":"panther","taxon":"M","names":"Jaguar|Ocelot|Margay|Jaguarundi|Oncilla|Puma|Southern tiger cat"},
  {"archetype":"boar","taxon":"M","names":"Lowland tapir|Baird's tapir|Collared peccary|White-lipped peccary|Giant anteater|Northern tamandua|Brown-throated sloth|Linnaeus's two-toed sloth|Central American agouti|Lowland paca"},
  {"archetype":"fox","taxon":"M","names":"Coati|Kinkajou|Tayra|Crab-eating raccoon|Bush dog|Red-handed tamarin|Common marmoset|Squirrel monkey|Capuchin monkey"},
  {"archetype":"elk","taxon":"M","names":"Red brocket deer|Gray brocket deer|Brown brocket deer|Southern pudu|Black howler monkey|Spider monkey|Woolly monkey"},
  {"archetype":"owl","taxon":"B","names":"Scarlet macaw|Blue-and-yellow macaw|Green-winged macaw|Keel-billed toucan|Toco toucan|Harpy eagle|Hoatzin|Resplendent quetzal|Blue-crowned motmot|Great curassow|Andean cock-of-the-rock|Ruby-topaz hummingbird"},
  {"archetype":"serpent","taxon":"R","names":"Green anaconda|Boa constrictor|Emerald tree boa|Bushmaster|Eyelash viper|Green iguana|Caiman lizard|Rainbow boa"},
  {"archetype":"serpent","taxon":"A","names":"Golden poison frog|Strawberry poison dart frog|Red-eyed tree frog|Glass frog|Cane toad|Amazon milk frog"}
 ]}
]

static func species_for_region(region:int)->Array[Dictionary]:
 var result:Array[Dictionary]=[]
 if region<0 or region>=HABITATS.size():return result
 var habitat:Dictionary=HABITATS[region]
 for family in habitat["families"]:
  var names:PackedStringArray=String(family["names"]).split("|",false)
  for name in names:
   result.append({"name":name,"archetype":str(family["archetype"]),
    "taxon":str(family["taxon"]),"habitat":str(habitat["habitat"])})
 return result

static func species_for_archetype(region:int,archetype:String)->Array[Dictionary]:
 var result:Array[Dictionary]=[]
 for species in species_for_region(region):
  if str(species["archetype"])==archetype:result.append(species)
 return result

static func total_profiles()->int:
 var total:int=0
 for region in range(HABITATS.size()):total+=species_for_region(region).size()
 return total

static func distinct_species_count()->int:
 var names:Dictionary={}
 for region in range(HABITATS.size()):
  for species in species_for_region(region):names[str(species["name"])]=true
 return names.size()

static func select_for_archetype(region:int,archetype:String,offset:int)->Dictionary:
 var choices:Array[Dictionary]=species_for_archetype(region,archetype)
 if choices.is_empty():return {}
 return choices[posmod(offset,choices.size())]
