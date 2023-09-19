# PS1 Solo Noble

Solo Noble game implementation for PS1 using MIPS toolchain + Nugget + Psy-Q Dev Kit.


## How to Build ##

* Install Visual Studio Code + PSX.DEV extension (https://www.youtube.com/watch?v=KbAv-Ao7lzU)
* Clone this repository using:
```
git clone --recurse-submodules https://github.com/danx2016/ps1_solo_noble.git
```
<b>note:</b> psyq-iwyu and nugget folders inside third_party directory are git submodules (independent projects referenced in another locations), so you need to use '--recurse-submodules' flag to include them 
* Open the folder in vscode, then 'Ctrl + Shift + P -> PSX.Dev: Show panel > WELCOME > Restore Psy-Q'. This will restore the Psy-Q SDK files inside 'third_party/psyq' folder
* To build ps-exe, in the terminal, just type inside project directory:
```
make
```
* Finally, to generate the CD BIN/CUE:
```
mkpsxiso.exe -y isoconfig.xml  
```


## Special Thanks To ##

* @Nicolas Noble
* @spicyjpeg
* and all people from PSX.Dev
