-------- config.lua --------
-- Config file for OTServ --
----------------------------

-- data directory location
datadir = "data/"

-- map location
map = "data/world/evolutions.otbm"

-- mapkind
-- options: OTBM for binary map, XML for OTX map
mapkind = "OTBM"

-- map store location (for XML only)
mapstore = "data/world/evolutions-mapstore.xml"

-- house store location (for XML only)
housestore = "data/world/evolutions-housestore.xml"

-- bans storage (for XML only)
banIdentifier = "data/bans.xml"

-- server name
servername = "Evolutions 7.92 RPG"

-- server location
location = "Holland"

-- server ip (the ip that server listens on)
ip = "auto"

-- server port (the port that server listens on)
port = "7171"

-- server url
url = "http://100498.leerling.lekenlinge.nl"

-- server owner name
ownername = "Xidaozu"

-- server owner email
owneremail = "Xidaozu@hotmail.com"

-- world type
-- options: pvp, no-pvp, pvp-enforced
worldtype = "pvp"

-- exhausted time in ms (1000 = 1 second)
exhausted = 1000

-- exhausted time in ms for non-aggressive spells (1000 = 1 second)
exhaustedheal = 1000

-- how many ms to add if the player is already exhausted and tries to cast a spell (1000 = 1 second)
exhaustedadd = 200

-- how long does the player has to stay out of fight to get pz unlocked in ms (1000 = 1 second)
pzlocked = 60*1000

-- house rent period
-- options: daily, weekly, monthly
houserentperiod = "weekly"

-- motd (the message box that you sometimes get before you choose characters)
motd = "Welcome to Evolutions 7.92 RPG. Please choose your character."
motdnum = "1"

-- login message
loginmsg = "Welcome to Evolutions 7.92 RPG. For help visit http://otfans.net/showthread.php?t=46553"

-- how many logins attempts until ip is temporary disabled 
-- set to 0 to disable
logintries = 0

-- how long the retry timeout until a new login can be made (without disabling the ip)
retrytimeout = 60*1000

-- how long the player need to wait until the ip is allowed again
logintimeout = 0

-- allow clones (multiple logins of the same char)
-- options: 0 (no), 1 (yes)
allowclones = 1

-- max number of players allowed
maxplayers = "100"

-- SQL type
-- options: mysql, sqlite
sql_type = "mysql"

--- MySQL part (ignore if you are using SQLite)
sql_host = "localhost"
sql_user = "root"
sql_pass = ""
sql_db   = "evolution"

--- SQLite part (ignore if you are using MySQL)
sqlite_db = "db.s3db"

-------------------------------------------------------------------------------------------------
---------------------------- Evolutions Basic Configuration ----------------------------
-------------------------------------------------------------------------------------------------

-- world name (shows in the character list)
worldname = "Evolutions 7.92 RPG"

-- time to save the server (default = 5)
autosave = 10

-- do you want to enable cap system? (yes/no)
capsystem = "no"

-- anti-afk - maximum idle time to kick player (1 = 1min)
kicktime = 15

-- how many summons player can have
maxsummons = 2

-- maximum items in depot
maxdepotitems = 1000

-- learn spells (yes/no)
learnspells = "no"

-- do you want everyone to have premium
freepremium = "no"

-- remove ammunation? (bolts/arrows)
removeammunation = "yes"

-- remove rune charges? (sd/hmm/gfb)
removerunecharges = "yes"

-- use item hotkeys? (yes/no)
itemhotkeys = "yes"

-- shoot trough battle window on players? (yes/no)
battlewindowplayers = "yes"

-- use account manager? (yes/no)
accountmanager = "yes"

-- summon follows master everywhere
summonsfollow = "yes"

-- allow outfit change
outfitchange = "yes"

-- damage to players with the same feet
feetdamage = "yes"

-- guild system type (SQL only)(ingame/online)
-- online guild system requires the latest Swelia AAC
guildsystem = "ingame"

-------------------------------------------------------------------------------------
----------------------------------- Multipliers -----------------------------------
-------------------------------------------------------------------------------------

-- experience multiplier (how much faster you got exp from monsters)
expmul = 10

-- experience multiplier for pvp-enforced (how much faster you got exp from players)
expmulpvp = 2

-- monster lootrating (how much faster you get items from monsters)
lootmul = 1

-- skill multiplier (another multiplier in data/vocations.xml)
skillmul = 1

-- manaspent multiplier  (another multiplier in data/vocations.xml)
manamul = 1

-- how many monsters spawn at a time in 1 spawn
spawnmul = 1

-- Price for each SQM when buying a house
houseprice = 200

-- level to buy a house
houselevel = 20

-- maximum death entries per player
maxdeathentries = 10

-- max message buffer (default = 4)
-- how fast you get muted
messagebuffer = 4

-- minimum action interval (default = 200)
minactioninterval = 200

-- protection for those under this level
protectionlimit = 50

-- critical damage and chance {chance, extra damage percent}
criticaldamage = {"0", "0"}

---------------------------------------------------------------------------------------
-------------------------- Skull System configuration -------------------------
---------------------------------------------------------------------------------------

-- time to lose a white skull (1 = 1 minute)
whitetime = 15

-- time to lose one frag (1 = 1 minute)
fragtime = 1*60

-- ban unjust, how many frags you need to get banned (1 = 1 frag)
banunjust = 6

-- red skull unjust, how many frags you need to get a red skull (1 = 1 frag)
redunjust = 3

-- bantime, for how long the player is banned (1 = 1 hour)
bantime = 24*1

--------------------------------------------------------------------------------------
------------------------------- GM access rights --------------------------------
--------------------------------------------------------------------------------------

-- access to walk into houses and open house doors
accesshouse = 3

-- access to login without waiting in the queue or when server is closed
accessenter = 1

-- access to ignore damage, exhaustion, cap limit and be ignored by monsters
accessprotect = 3

-- access to broadcast messages and talk in colors (#c blabla - in public channels)
accesstalk = 1

-- access to move distant items from/to distant locations
accessremote = 3

-- access to see id and position of the item you are looking at
accesslook = 2